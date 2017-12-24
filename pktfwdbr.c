#include <json-glib/json-glib.h>
#include <string.h>

#include "pktfwdbr.h"
#include "lorawan.h"
#include "join.h"

static gboolean pktfwdbr_onmsg_parsepkt(JsonObject* rootobj,
		struct pktfwdpkt* pkt) {

	if (!(json_object_has_member(rootobj, PKTFWDBR_JSON_TXPK_MODU))
			|| !(json_object_has_member(rootobj, PKTFWDBR_JSON_TXPK_FREQ))
			|| !(json_object_has_member(rootobj, PKTFWDBR_JSON_TXPK_DATA))
			|| !(json_object_has_member(rootobj, PKTFWDBR_JSON_TXPK_SIZE))) {
		g_message("no data field");
		return FALSE;
	}

	// pull out the rf stuff
	pkt->modulation = json_object_get_string_member(rootobj,
	PKTFWDBR_JSON_TXPK_MODU);
	pkt->frequency = json_object_get_double_member(rootobj,
	PKTFWDBR_JSON_TXPK_FREQ);
	pkt->rfchannel = json_object_get_int_member(rootobj,
	PKTFWDBR_JSON_TXPK_RFCH);

	// pull out lora stuff
	pkt->datarate = json_object_get_string_member(rootobj,
	PKTFWDBR_JSON_TXPK_DATR);
	pkt->coderate = json_object_get_string_member(rootobj,
			PKTFWDBR_JSON_TXPK_CODR);

	// pull out the timing stuff
	pkt->timestamp = json_object_get_int_member(rootobj,
	PKTFWDBR_JSON_TXPK_TMST);

	pkt->data = json_object_get_string_member(rootobj, PKTFWDBR_JSON_TXPK_DATA);
	pkt->size = json_object_get_int_member(rootobj, PKTFWDBR_JSON_TXPK_SIZE);

	return TRUE;
}

void pktfwdbr_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts) {

	JsonParser* jsonparser = NULL;
	char* gatewayid = splittopic[1];
	char* direction = splittopic[2];

	if (strcmp(direction, PKTFWDBR_TOPIC_RX) != 0) {
		g_message("unexpected message direction: %s", direction);
		goto out;
	}

	jsonparser = json_parser_new_immutable();
	if (!json_parser_load_from_data(jsonparser, msg->payload, msg->payloadlen,
	NULL)) {
		g_message("failed to parse json");
	}

	JsonNode* root = json_parser_get_root(jsonparser);
	JsonObject* rootobj = json_node_get_object(root);

	struct pktfwdpkt pkt;
	pktfwdbr_onmsg_parsepkt(rootobj, &pkt);

	const gchar* b64data = pkt.data;
	gsize datalen;
	guchar* data = g_base64_decode(b64data, &datalen);

	if (data == NULL) {
		g_message("failed to decode data");
		goto out;
	}

	uint8_t type = (*data >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK;
	switch (type) {
	case MHDR_MTYPE_JOINREQ:
		join_processjoinrequest(cntx, gatewayid, data, datalen, &pkt);
		break;
	default:
		g_message("unhandled message type %d", (int) type);
		break;
	}

	out: if (data != NULL)
		g_free(data);
	if (jsonparser != NULL)
		g_object_unref(jsonparser);
}
