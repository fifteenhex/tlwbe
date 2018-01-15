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

#if TLWBE_DEBUG_MOSQUITTO
static void mosq_log(struct mosquitto* mosq, void* userdata, int level,
		const char* str) {
	g_message(str);
}
#endif

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

	if (strcmp(roottopic, PKTFWDBR_TOPIC_ROOT) == 0)
		pktfwdbr_onmsg(cntx, msg, splittopic, numtopicparts);
	else if (strcmp(roottopic, TLWBE_TOPICROOT) == 0) {
		char* subtopic = splittopic[1];
		if (strcmp(subtopic, CONTROL_SUBTOPIC) == 0)
			control_onmsg(cntx, msg, splittopic, numtopicparts);
		else if (strcmp(subtopic, UPLINK_SUBTOPIC_UPLINKS) == 0)
			uplink_onmsg(cntx, msg, splittopic, numtopicparts);
		else if (strcmp(subtopic, DOWNLINK_SUBTOPIC) == 0)
			downlink_onmsg(cntx, msg, splittopic, numtopicparts);
	} else {
		g_message("unexpected topic root: %s", roottopic);
		goto out;
	}

	out: if (splittopic != NULL)
		mosquitto_sub_topic_tokens_free(&splittopic, numtopicparts);

}

static void connectcallback(struct mosquitto* mosq, void* data) {
	struct context* cntx = data;
	mosquitto_subscribe(mosq, NULL,
	PKTFWDBR_TOPIC_ROOT"/+/"PKTFWDBR_TOPIC_RX"/#", 0);
	control_onbrokerconnect(cntx);
	downlink_onbrokerconnect(cntx);
	uplink_onbrokerconnect(cntx);
}

int main(int argc, char** argv) {

	struct context cntx = { 0 };

	gchar* databasepath = "./tlwbe.sqlite";

	gchar* mqtthost = "localhost";
	guint mqttport = 1883;

	GOptionEntry entries[] = { //
			{ "mqtthost", 'h', 0, G_OPTION_ARG_STRING, &mqtthost, "", "" }, //
					{ "mqttport", 'p', 0, G_OPTION_ARG_INT, &mqttport, "", "" }, //
					{ NULL } };

	GOptionContext* context = g_option_context_new("");
	GError* error = NULL;
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("option parsing failed: %s\n", error->message);
		goto out;
	}

	database_init(&cntx, databasepath);

	mosquittomainloop(&cntx.mosqcntx, mqtthost, mqttport, connectcallback,
			&cntx);
#if TLWBE_DEBUG_MOSQUITTO
	mosquitto_log_callback_set(cntx.mosqcntx.mosq, mosq_log);
#endif
	mosquitto_message_callback_set(cntx.mosqcntx.mosq, mosq_message);

	g_timeout_add(5 * 60 * 1000, uplink_cleanup, &cntx);
	g_timeout_add(5 * 60 * 1000, downlink_cleanup, &cntx);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	out: return 0;
}
