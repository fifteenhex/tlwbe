#define GETTEXT_PACKAGE "gtk20"
#include <glib.h>
#include <gio/gio.h>
#include <mosquitto.h>
#include <string.h>

#include "tlwbe.h"
#include "pktfwdbr.h"
#include "database.h"
#include "control.h"
#include "downlink.h"
#include "config.h"

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

			control_onbrokerconnect(cntx);
			downlink_onbrokerconnect(cntx);
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
		else if (strcmp(subtopic, DOWNLINK_SUBTOPIC) == 0)
			downlink_onmsg(cntx, msg, splittopic, numtopicparts);
	} else {
		g_message("unexpected topic root: %s", roottopic);
		goto out;
	}

	out: if (splittopic != NULL)
		mosquitto_sub_topic_tokens_free(&splittopic, numtopicparts);

}

int main(int argc, char** argv) {

	struct context cntx = { .mosq = NULL, .mqtthost = "localhost", .mqttport =
			1883 };

	gchar* databasepath = "./tlwbe.sqlite";

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

	database_init(&cntx, databasepath);

	mosquitto_lib_init();
	cntx.mosq = mosquitto_new(NULL, true, &cntx);
#if TLWBE_DEBUG_MOSQUITTO
	mosquitto_log_callback_set(cntx.mosq, mosq_log);
#endif
	mosquitto_message_callback_set(cntx.mosq, mosq_message);

	g_timeout_add(500, mosq_idle, &cntx);

	GMainLoop* mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	out: return 0;
}
