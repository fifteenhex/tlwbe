#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>

#include "tlwbe.h"
#include "pktfwdbr.h"
#include "lorawan.h"
#include "join.h"

static gboolean handlemosq(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	struct mosquitto* mosq = (struct mosquitto*) data;
	mosquitto_loop_read(mosq, 1);
	return TRUE;
}

static gboolean mosq_idle(gpointer data) {
	struct context* cntx = (struct context*) data;

	bool connected = false;

	// This seems like the only way to work out if
	// we ever connected or got disconnected at
	// some point
	if (mosquitto_loop_misc(cntx->mosq) == MOSQ_ERR_NO_CONN) {
		if (cntx->mosqchan != NULL) {
			g_source_remove(cntx->mosqsource);
			// g_io_channel_shutdown doesn't work :/
			close(mosquitto_socket(cntx->mosq));
			cntx->mosqchan = NULL;
		}

		if (mosquitto_connect(cntx->mosq, cntx->mqtthost, cntx->mqttport, 60)
				== MOSQ_ERR_SUCCESS) {
			int mosqfd = mosquitto_socket(cntx->mosq);
			cntx->mosqchan = g_io_channel_unix_new(mosqfd);
			cntx->mosqsource = g_io_add_watch(cntx->mosqchan, G_IO_IN,
					handlemosq, cntx->mosq);
			g_io_channel_unref(cntx->mosqchan);

			mosquitto_subscribe(cntx->mosq, NULL,
			PKTFWDBR_TOPIC_ROOT"/+/"PKTFWDBR_TOPIC_RX"/#", 0);

			connected = true;
		}
	} else
		connected = true;

	if (connected) {
		mosquitto_loop_read(cntx->mosq, 1);
		mosquitto_loop_write(cntx->mosq, 1);
	}
	return TRUE;
}

static void mosq_log(struct mosquitto* mosq, void* userdata, int level,
		const char* str) {
	g_message(str);
}

static void mosq_message(struct mosquitto* mosq, void* userdata,
		const struct mosquitto_message* msg) {

	struct context* cntx = (struct context*) userdata;

	char** splittopic;
	int numtopicparts;
	mosquitto_sub_topic_tokenise(msg->topic, &splittopic, &numtopicparts);

	if (numtopicparts < 3) {
		g_message("unexpected number of topic parts: %d", numtopicparts);
		goto out;
	}

	char* roottopic = splittopic[0];
	char* gatewayid = splittopic[1];
	char* direction = splittopic[2];

	if (strcmp(roottopic, PKTFWDBR_TOPIC_ROOT) != 0) {
		g_message("unexpected topic root: %s", roottopic);
		goto out;
	}

	if (strcmp(direction, PKTFWDBR_TOPIC_RX) != 0) {
		g_message("unexpected message direction: %s", direction);
		goto out;
	}

	JsonParser* jsonparser = json_parser_new_immutable();
	if (!json_parser_load_from_data(jsonparser, msg->payload, msg->payloadlen,
	NULL)) {
		g_message("failed to parse json");
	}

	JsonNode* root = json_parser_get_root(jsonparser);
	JsonObject* rootobj = json_node_get_object(root);

	if (!(json_object_has_member(rootobj, "data"))) {
		g_message("no data field");
		goto out;
	}

	const gchar* b64data = json_object_get_string_member(rootobj, "data");
	gsize datalen;
	guchar* data = g_base64_decode(b64data, &datalen);

	if (data == NULL) {
		g_message("failed to decode data");
		goto out;
	}

	guchar* pkt = data;

	uint8_t type = (*pkt++ >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK;
	switch (type) {
	case MHDR_MTYPE_JOINREQ:
		join_processjointrequest(cntx, gatewayid, data, datalen);
		break;
	default:
		g_message("unhandled message type %d", (int) type);
		break;
	}

	g_free(data);

	out: if (splittopic != NULL)
		mosquitto_sub_topic_tokens_free(&splittopic, numtopicparts);

}

int main(int argc, char** argv) {

	struct context cntx = { .mosq = NULL, .mqtthost = "localhost", .mqttport =
			1883 };

	GOptionEntry entries[] = { //
			{ "mqtthost", 'h', 0, G_OPTION_ARG_STRING, &cntx.mqtthost, "", "" }, //
					{ "mqttport", 'p', 0, G_OPTION_ARG_INT, &cntx.mqttport, "",
							"" }, //
					{ NULL } };

	GOptionContext* context = g_option_context_new("");
	GError* error = NULL;
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		goto out;
	}

	mosquitto_lib_init();
	cntx.mosq = mosquitto_new(NULL, true, &cntx);
	mosquitto_log_callback_set(cntx.mosq, mosq_log);
	mosquitto_message_callback_set(cntx.mosq, mosq_message);

	g_timeout_add(500, mosq_idle, &cntx);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	out: return 0;
}
