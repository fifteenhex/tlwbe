#include <json-glib/json-glib.h>
#include <string.h>

#include "downlink.h"
#include "pktfwdbr.h"
#include "utils.h"
#include "tlwbe.h"
#include "regional.h"
#include "database.h"

static gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		guint64 delay, gdouble frequency, const struct pktfwdpkt* rxpkt) {

	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_ROOT);
	json_builder_begin_object(jsonbuilder);

	// add in the rf stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_MODU);
	json_builder_add_string_value(jsonbuilder, rxpkt->modulation);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_FREQ);
	json_builder_add_double_value(jsonbuilder, frequency);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_RFCH);
	json_builder_add_int_value(jsonbuilder, rxpkt->rfchannel);

	// add in lora stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_DATR);
	json_builder_add_string_value(jsonbuilder, rxpkt->datarate);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_CODR);
	json_builder_add_string_value(jsonbuilder, rxpkt->coderate);

	// needed apparently :/
	json_builder_set_member_name(jsonbuilder, "ipol");
	json_builder_add_boolean_value(jsonbuilder, true);

	// add in timing stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_TMST);
	json_builder_add_int_value(jsonbuilder, rxpkt->timestamp + delay);

	gchar* b64txdata = g_base64_encode(data, datalen);

	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_DATA);
	json_builder_add_string_value(jsonbuilder, b64txdata);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_SIZE);
	json_builder_add_int_value(jsonbuilder, datalen);

	json_builder_end_object(jsonbuilder);
	json_builder_end_object(jsonbuilder);

	char* json = utils_jsonbuildertostring(jsonbuilder, length);

	g_object_unref(jsonbuilder);
	g_free(b64txdata);

	return json;
}

void downlink_dodownlink(struct context* cntx, const gchar* gateway,
		guint8* pkt, gsize pktlen, const struct pktfwdpkt* rxpkt,
		enum RXWINDOW rxwindow) {
	gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
	gsize payloadlen;
	gchar* payload = downlink_createtxjson(pkt, pktlen, &payloadlen,
			regional_getwindowdelay(rxwindow),
			regional_getfrequency(rxwindow, rxpkt), rxpkt);
	mosquitto_publish(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, topic, payloadlen, payload, 0, false);
	g_free(topic);
}

void downlink_onbrokerconnect(struct context* cntx) {
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, TLWBE_TOPICROOT "/" DOWNLINK_SUBTOPIC "/" DOWNLINK_QUEUE "/#", 0);
}

void downlink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts) {

	if (numtopicparts != 7) {
		g_message("need 7 topic parts, got %d", numtopicparts);
		return;
	}

	guint64 port;
	if (!g_ascii_string_to_unsigned(splittopic[5], 10, 0, 255, &port, NULL)) {
		g_message("port number invalid");
		return;
	}

	struct downlink downlink = { .timestamp = g_get_real_time(), .deadline =
			((24 * 60) * 60), .appeui = splittopic[3], .deveui = splittopic[4],
			.port = (guint8) (port & 0xff), .payload = msg->payload,
			.payloadlen = msg->payloadlen, .token = splittopic[6] };

	g_message("have downlink for app %s, dev %s, port %d, token %s",
			downlink.appeui, downlink.deveui, (int )port, downlink.token);

	database_downlink_add(cntx, &downlink);
}

gboolean downlink_cleanup(gpointer data) {
	struct context* cntx = data;
	database_downlinks_clean(cntx, g_get_real_time());
	return TRUE;
}
