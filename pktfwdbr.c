#include <json-glib/json-glib.h>
#include <string.h>

#include "pktfwdbr.h"
#include "lorawan.h"
#include "join.h"

void pktfwdbr_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts) {

	char* gatewayid = splittopic[1];
	char* direction = splittopic[2];

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

	uint8_t type = (*pkt >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK;
	switch (type) {
	case MHDR_MTYPE_JOINREQ:
		join_processjoinrequest(cntx, gatewayid, data, datalen);
		break;
	default:
		g_message("unhandled message type %d", (int) type);
		break;
	}

	out: if (data != NULL)
		g_free(data);
}
