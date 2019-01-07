#include <string.h>
#include <lorawan/lorawan.h>

#include "pktfwdbr.h"
#include "pktfwdbr_rx.h"
#include "pktfwdbr_txack.h"
#include "pktfwdbr_txack.json.h"
#include "pktfwdbr_rx.json.h"
#include "join.h"
#include "uplink.h"
#include "mac.h"
#include "packet.h"
#include "downlink.h"

void pktfwdbr_onbrokerconnect(const struct context* cntx) {
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, PKTFWDBR_TOPIC_ROOT"/+/"PKTFWDBR_TOPIC_RX"/#", 0);
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, PKTFWDBR_TOPIC_ROOT"/+/"PKTFWDBR_TOPIC_TXACK"/#", 0);
}

void pktfwdbr_onmsg(struct context* cntx, const JsonObject* rootobj,
		char** splittopic, int numtopicparts) {

	if (numtopicparts < 3) {
		g_message("not enough topic parts");
		return;
	}

	char* gatewayid = splittopic[1];
	char* direction = splittopic[2];
	guchar* data = NULL;

	if (strcmp(direction, PKTFWDBR_TOPIC_RX) == 0) {
		struct pktfwdbr_rx pkt;
		if (__jsongen_pktfwdbr_rx_from_json(&pkt, rootobj)) {
			const gchar* b64data = pkt.data;
			gsize datalen;
			data = g_base64_decode(b64data, &datalen);

			if (data == NULL) {
				g_message("failed to decode data");
				goto out;
			}

			packet_debug(data, datalen);

			uint8_t type = (*data >> MHDR_MTYPE_SHIFT) & MHDR_MTYPE_MASK;
			switch (type) {
			case MHDR_MTYPE_JOINREQ:
				join_processjoinrequest(cntx, gatewayid, data, datalen, &pkt);
				break;
			case MHDR_MTYPE_UNCNFUP:
			case MHDR_MTYPE_CNFUP:
				mac_process(cntx, gatewayid, data, datalen, &pkt);
				uplink_process(cntx, gatewayid, data, datalen, &pkt);
				break;
			default:
				g_message("unhandled message type %d", (int ) type);
				break;
			}
		} else
			g_message("failed to parse rx message");
	} else if (strcmp(direction, PKTFWDBR_TOPIC_TXACK) == 0) {
		const gchar* token = NULL;
		if (numtopicparts >= 4)
			token = splittopic[3];

		enum pktfwdbr_txack_error error = PKTFWDBR_TXACK_ERROR_NONE;
		if (rootobj != NULL) {
			struct pktfwdbr_txack_wrapper wrapper;
			__jsongen_pktfwdbr_txack_wrapper_from_json(&wrapper, rootobj);
			error = wrapper.tx_ack.error;
		}
		downlink_process_txack(cntx, token, error);
	} else
		g_message("unexpected action: %s", direction);

	out: if (data != NULL)
		g_free(data);
}
