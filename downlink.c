#include <json-glib/json-glib.h>
#include <string.h>

#include "downlink.h"
#include "pktfwdbr.h"
#include "utils.h"
#include "tlwbe.h"
#include "regional.h"
#include "database.h"

#include "json-glib-macros/jsonbuilderutils.h"

#include "downlink.json.h"
#include "downlink_rpc.rpc.h"

static gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		guint64 delay, gdouble frequency, const gchar* dr, guint8 txpower,
		const struct pktfwdbr_rx* rxpkt) {

	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_ROOT);
	json_builder_begin_object(jsonbuilder);

	// add in the rf stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_MODU);
	json_builder_add_string_value(jsonbuilder, rxpkt->rfparams.modulation);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_FREQ);
	json_builder_add_double_value(jsonbuilder, frequency);
	JSONBUILDER_ADD_INT(jsonbuilder, PKTFWDBR_JSON_TXPK_RFCH, 0);

	// if this is not set it seems the first tx_lut entry is used
	// and that usually results in a very weak signal
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_POWE);
	json_builder_add_int_value(jsonbuilder, txpower);

	// add in lora stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_DATR);
	json_builder_add_string_value(jsonbuilder, dr);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_CODR);
	json_builder_add_string_value(jsonbuilder, rxpkt->rfparams.coderate);

	// all current regions have inverted downlinks
	// beacons aren't inverted but we don't do beacons
	json_builder_set_member_name(jsonbuilder, "ipol");
	json_builder_add_boolean_value(jsonbuilder, TRUE);

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

	char* json = jsonbuilder_freetostring(jsonbuilder, length, FALSE);

	g_free(b64txdata);

	return json;
}

void downlink_dodownlink(struct context* cntx, const gchar* gateway,
		const gchar* token, guint8* pkt, gsize pktlen,
		const struct pktfwdbr_rx* rxpkt, enum RXWINDOW rxwindow) {
	gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, token, NULL);
	gsize payloadlen;
	gchar* payload = downlink_createtxjson(pkt, pktlen, &payloadlen,
			regional_getwindowdelay(rxwindow),
			regional_getfrequency(&cntx->regional, rxwindow, rxpkt),
			regional_getdatarate(&cntx->regional, rxwindow, rxpkt),
			cntx->regional.txpower, rxpkt);
	mosquitto_publish(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, topic, payloadlen, payload, 0, false);
	g_free(payload);
	g_free(topic);
}

void downlink_dorxwindowdownlink(struct context* cntx, const gchar* gateway,
		const gchar* token, guint8* pkt, gsize pktlen,
		const struct pktfwdbr_rx* rxpkt) {
	downlink_dodownlink(cntx, gateway, token, pkt, pktlen, rxpkt, RXW_R1);
	//downlink_dodownlink(cntx, gateway, token, pkt, pktlen, rxpkt, RXW_R2);
}

void downlink_onbrokerconnect(const struct context* cntx) {
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL,
	TLWBE_TOPICROOT "/" DOWNLINK_SUBTOPIC "/" DOWNLINK_ACTION_SCHEDULE "/#", 0);
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, TLWBE_TOPICROOT "/" DOWNLINK_SUBTOPIC "/" DOWNLINK_ACTION_QUERY "/#",
			0);
}

static int __rpcgen_downlink_schedule(struct context* context,
		const gchar* appeui, const gchar* deveui, guint8 port,
		const gchar* token, const JsonObject* request,
		const JsonBuilder* response) {

	int ret = RPCGEN_ERR_NONE;

	struct downlink downlink = { 0 };
	if (__jsongen_downlink_from_json(&downlink, request)) {
		downlink.timestamp = g_get_real_time();
		downlink.deadline = ((24 * 60) * 60);
		downlink.appeui = appeui;
		downlink.deveui = deveui;
		downlink.port = (guint8) (port & 0xff);
		downlink.token = token;

		g_message("have downlink for app %s, dev %s, port %d, token %s",
				downlink.appeui, downlink.deveui, (int )port, downlink.token);

		database_downlink_add(context, &downlink);

		struct downlink_schedule_result result = { 0 };
		JsonBuilder* builder = json_builder_new_immutable();
		__jsongen_downlink_schedule_result_to_json(&result, builder);

	} else {
		g_message("failed to parse downlink message");
		ret = RPCGEN_ERR_BADREQUEST;
	}

	return ret;

}

static int __rpcgen_downlink_cancel(struct context* context, const gchar* token,
		const JsonObject* request, const JsonBuilder* response) {
	return RPCGEN_ERR_NONE;
}

static int __rpcgen_downlink_query(struct context* context, const gchar* token,
		const JsonObject* request, const JsonBuilder* response) {
	struct downlink_query_result result = { 0 };
	JsonBuilder* builder = json_builder_new_immutable();
	__jsongen_downlink_query_result_to_json(&result, builder);
	return RPCGEN_ERR_NONE;
}

void downlink_onmsg(struct context* cntx, char** splittopic, int numtopicparts,
		const JsonObject* rootobj) {
	JsonBuilder* response = json_builder_new();
	json_builder_begin_object(response);
	__rpcgen_downlink_dispatch(cntx, splittopic, numtopicparts, rootobj,
			response);
	json_builder_end_object(response);
	gchar* topic = mosquitto_client_createtopic(TLWBE_TOPICROOT, "downlink",
			"result", splittopic[numtopicparts - 1], NULL);
	mosquitto_client_publish_json_builder(cntx->mosqclient, response, topic,
	TRUE);
	g_free(topic);
}

gboolean downlink_cleanup(gpointer data) {
	struct context* cntx = data;
	database_downlinks_clean(cntx, g_get_real_time());
	return TRUE;
}

void downlink_announce_sent(const struct context* cntx, const gchar* token) {
	gchar* topic = mosquitto_client_createtopic(TLWBE_TOPICROOT, "downlink",
			"sent", token, NULL);
	JsonBuilder* jsonbuilder = json_builder_new();
	struct downlink_announce_sent msg;
	__jsongen_downlink_announce_sent_to_json(&msg, jsonbuilder);
	mosquitto_client_publish_json_builder(cntx->mosqclient, jsonbuilder, topic,
	TRUE);
	g_free(topic);
}

void downlink_process_txack(const struct context* cntx, const gchar* token,
		enum pktfwdbr_txack_error error) {
	if (token != NULL) {
		switch (error) {
		case PKTFWDBR_TXACK_ERROR_NONE:
			database_downlinks_delete_by_token(cntx, token);
			downlink_announce_sent(cntx, token);
			break;
		default:
			g_message("forwarder didn't schedule downlink");
			break;
		}
	}
}

void downlink_init(struct context* cntx) {

}
