#pragma once

#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
#include <json-glib/json-glib.h>
#include "tlwbe.h"
#endif

#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
/* Basic topic format
 *
 * pktfwdbr/<gatewayid>/[rx|tx]
 *
 * for RX packets
 *
 * The topics for RX packets include some bonus info
 * to make filtering the type/source of packets easier
 * for anyone that happens to be listening.
 *
 *
 * pktfwdbr/<gatewayid>/rx/join/<appeui>/<deveui>
 *
 */

#define PKTFWDBR_TOPIC_ROOT		"pktfwdbr"
#define PKTFWDBR_TOPIC_RX		"rx"
#define PKTFWDBR_TOPIC_TX		"tx"
#define PKTFWDBR_TOPIC_TXACK	"txack"

void pktfwdbr_onbrokerconnect(const struct context* cntx);
void pktfwdbr_onmsg(struct context* cntx, const JsonObject* rootobj,
		char** splittopic, int numtopicparts);
#endif
