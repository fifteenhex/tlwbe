#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>
#include <gio/gio.h>
#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>

#include "lorawan.h"
#include "crypto.h"

struct context {
	struct mosquitto* mosq;
	const gchar* mqtthost;
	gint mqttport;
	GIOChannel* mosqchan;
	guint mosqsource;
};

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

			mosquitto_subscribe(cntx->mosq, NULL, "pktfwdbr/#", 0);

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

	JsonParser* jsonparser = json_parser_new_immutable();
	if (!json_parser_load_from_data(jsonparser, msg->payload, msg->payloadlen,
	NULL)) {
		g_message("failed to parse json");
	}

	JsonNode* root = json_parser_get_root(jsonparser);
	JsonObject* rootobj = json_node_get_object(root);
	const gchar* b64data = json_object_get_string_member(rootobj, "data");
	gsize datalen;
	guchar* data = g_base64_decode(b64data, &datalen);

	guchar* pkt = data;

	uint8_t type = (*pkt++ >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK;
	switch (type) {
	case MHDR_MTYPE_JOINREQ: {
		int joinreqlen = 1 + sizeof(struct lorawan_joinreq) + 4;
		if (datalen == joinreqlen) {

			struct lorawan_joinreq* joinreq = (struct lorawan_joinreq*) pkt;
			guint64 appeui = GUINT64_FROM_LE(joinreq->appeui);
			guint64 deveui = GUINT64_FROM_LE(joinreq->deveui);
			guint16 devnonce = GUINT16_FROM_LE(joinreq->devnonce);
			g_message("handling join request for app %"G_GINT64_MODIFIER
					"x from %"G_GINT64_MODIFIER"x "
					"nonce %"G_GINT16_MODIFIER"x", appeui, deveui, devnonce);

			pkt += sizeof(*joinreq);
			guint32 inmic;
			memcpy(&inmic, pkt, sizeof(inmic));

			uint8_t key[] = { 0x6E, 0xE8, 0x9C, 0x07, 0x4F, 0x32, 0x68, 0x13,
					0x9A, 0xBE, 0xF6, 0x31, 0x29, 0xD9, 0xE9, 0xA9 };

			guint32 calcedmic = crypto_mic(key, sizeof(key), joinreq,
					sizeof(*joinreq));

		g_message("mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x", inmic, calcedmic);
	} else
		g_message("join request should be %d bytes long, have %d", joinreqlen,
				datalen);
}
	break;
default:
	g_message("unhandled message type %d", (int) type);
	break;
	}

	g_free(data);
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
	cntx.mosq = mosquitto_new(NULL, true, NULL);
	mosquitto_log_callback_set(cntx.mosq, mosq_log);
	mosquitto_message_callback_set(cntx.mosq, mosq_message);

	g_timeout_add(500, mosq_idle, &cntx);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	out: return 0;
}
