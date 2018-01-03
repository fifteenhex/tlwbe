#include <json-glib/json-glib.h>
#include <string.h>

#include "downlink.h"
#include "pktfwdbr.h"
#include "utils.h"
#include "tlwbe.h"

static gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		guint64 delay, const struct pktfwdpkt* rxpkt) {

	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_ROOT);
	json_builder_begin_object(jsonbuilder);

	// add in the rf stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_MODU);
	json_builder_add_string_value(jsonbuilder, rxpkt->modulation);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_FREQ);
	json_builder_add_double_value(jsonbuilder, rxpkt->frequency);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_RFCH);
	json_builder_add_int_value(jsonbuilder, rxpkt->rfchannel);

	// add in lora stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_DATR);
	json_builder_add_string_value(jsonbuilder, rxpkt->datarate);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_CODR);
	json_builder_add_string_value(jsonbuilder, rxpkt->coderate);

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
		enum DOWNLINK_RXWINDOW rxwindow) {
	gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
	gsize payloadlen;
	gchar* payload = downlink_createtxjson(pkt, pktlen, &payloadlen, 2000000,
			rxpkt);
	mosquitto_publish(cntx->mosq, NULL, topic, payloadlen, payload, 0,
	false);
	g_free(topic);
}

void downlink_onbrokerconnect(struct context* cntx) {
	mosquitto_subscribe(cntx->mosq, NULL,
	TLWBE_TOPICROOT "/" DOWNLINK_SUBTOPIC "/#", 0);
}

void downlink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts) {

	if (numtopicparts != 5) {
		g_message("need 5 topic parts, got %d", numtopicparts);
		return;
	}

	const char* appeui = splittopic[2];
	const char* deveui = splittopic[3];
	const char* port = splittopic[4];

	g_message("have downlink for app %s, dev %s, port %s", appeui, deveui,
			port);
}
