#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>
#include <gio/gio.h>
#include <mosquitto.h>
#include <string.h>

#include "tlwbe.h"
#include "pktfwdbr.h"
#include "database.h"
#include "control.h"
#include "uplink.h"
#include "downlink.h"
#include "config.h"

typedef void (*interface_onconnected)(const struct context* cntx);
typedef void (*interface_onmsg)(struct context* cntx, char** splittopic,
		int numtopicparts, const JsonObject* rootobj);

struct interface {
	const gchar* subtopic;
	const interface_onconnected onconnected;
	const interface_onmsg onmsg;
};

static const struct interface interfaces[] = { { CONTROL_SUBTOPIC,
		control_onbrokerconnect, control_onmsg }, { UPLINK_SUBTOPIC_UPLINKS,
		uplink_onbrokerconnect, uplink_onmsg }, { DOWNLINK_SUBTOPIC,
		downlink_onbrokerconnect, downlink_onmsg } };

static gboolean messagecallback(MosquittoClient* client,
		const struct mosquitto_message* msg, gpointer userdata) {
	struct context* cntx = (struct context*) userdata;
	JsonParser* jsonparser = NULL;
	JsonObject* rootobj = NULL;
	char** splittopic;
	int numtopicparts;
	mosquitto_sub_topic_tokenise(msg->topic, &splittopic, &numtopicparts);

	if (numtopicparts < 3) {
		g_message("unexpected number of topic parts: %d", numtopicparts);
		goto out;
	}

	char* roottopic = splittopic[0];

	if (msg->payloadlen != 0) {
		jsonparser = json_parser_new_immutable();
		if (!json_parser_load_from_data(jsonparser, msg->payload,
				msg->payloadlen,
				NULL)) {
			g_message("failed to parse message json");
			goto out;
		}
		JsonNode* root = json_parser_get_root(jsonparser);
		if (json_node_get_node_type(root) != JSON_NODE_OBJECT) {
			g_message("json message should always be enclosed in an object");
		}
		rootobj = json_node_get_object(root);
	}

	if (strcmp(roottopic, PKTFWDBR_TOPIC_ROOT) == 0)
		pktfwdbr_onmsg(cntx, rootobj, splittopic, numtopicparts);
	else if (strcmp(roottopic, TLWBE_TOPICROOT) == 0) {
		char* subtopic = splittopic[1];
		char** interfacetopic = &splittopic[2];
		int numinterfacetopicparts = numtopicparts - 2;

		gboolean handled = FALSE;
		for (int i = 0; i < G_N_ELEMENTS(interfaces); i++)
			if (strcmp(interfaces[i].subtopic, subtopic) == 0) {
				interfaces[i].onmsg(cntx, interfacetopic,
						numinterfacetopicparts, rootobj);
				handled = TRUE;
				break;
			}

		if (!handled)
			g_message("unexpected sub topic: %s", subtopic);
	} else {
		g_message("unexpected topic root: %s", roottopic);
		goto out;
	}

	out: if (splittopic != NULL)
		mosquitto_sub_topic_tokens_free(&splittopic, numtopicparts);
	if (jsonparser != NULL)
		g_object_unref(jsonparser);
	return TRUE;
}

static void connectedcallback(MosquittoClient* client, void* something,
		gpointer userdata) {
	struct context* cntx = (struct context*) userdata;
	for (int i = 0; i < G_N_ELEMENTS(interfaces); i++)
		interfaces[i].onconnected(cntx);
	pktfwdbr_onbrokerconnect(cntx);
}

int main(int argc, char** argv) {

	struct context cntx = { 0 };
	int ret = 0;

	gchar* databasepath = TLWBE_DATABASE;
	gchar* regionalparamspath = TLWBE_REGIONALPARAMETERS;

	gchar* mqttid = NULL;
	gchar* mqtthost = "localhost";
	guint mqttport = 1883;
	gchar* mqttrootcert = NULL;
	gchar* mqttdevicecert = NULL;
	gchar* mqttdevicekey = NULL;

	gchar* region = NULL;

	GOptionEntry entries[] = { //
			MQTTOPTS, //
			{ "regionalparameters_path", 0, 0, G_OPTION_ARG_STRING,
					&regionalparamspath, "", "" }, //
					{ "database_path", 0, 0, G_OPTION_ARG_STRING, &databasepath,
							"", "" }, //
					{ "region", 0, 0, G_OPTION_ARG_STRING, &region, "", "" }, //
					{ NULL } };

	GOptionContext* context = g_option_context_new("");
	GError* error = NULL;
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		goto out;
	}

	downlink_init(&cntx);

	stats_init(&cntx.stats);

	if (!regional_init(&cntx.regional, regionalparamspath, region))
		goto out;

	if (!database_init(&cntx, databasepath)) {
		g_message("failed to init database");
		ret = 1;
		goto out;
	}

	cntx.mosqclient = mosquitto_client_new_plaintext(mqttid, mqtthost,
			mqttport);

	g_signal_connect(cntx.mosqclient, MOSQUITTO_CLIENT_SIGNAL_CONNECTED,
			(GCallback )connectedcallback, &cntx);

	g_signal_connect(cntx.mosqclient, MOSQUITTO_CLIENT_SIGNAL_MESSAGE,
			(GCallback )messagecallback, &cntx);

	g_timeout_add(5 * 60 * 1000, uplink_cleanup, &cntx);
	g_timeout_add(5 * 60 * 1000, downlink_cleanup, &cntx);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	out: return 0;
}
