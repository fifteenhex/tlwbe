#pragma once

#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
#include <json-glib/json-glib.h>
#include "tlwbe.h"
#endif

struct pktfwdpkt_rfparams {
	// basic rf bits
	const gchar* modulation;
	gdouble frequency;
	gint16 rssi;
	// lora specifc stuff
	const gchar* datarate;
	const gchar* coderate;
	// misc
	guint32 rfchain;
};

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

#define PKTFWDBR_JSON_TXPK_ROOT	"txpk"
#define PKTFWDBR_JSON_TXPK_IMME "imme"
#define PKTFWDBR_JSON_TXPK_TMST	"tmst"
#define PKTFWDBR_JSON_TXPK_TMMS "tmms"
#define PKTFWDBR_JSON_TXPK_FREQ	"freq"
#define PKTFWDBR_JSON_TXPK_RFCH	"rfch"
#define PKTFWDBR_JSON_TXPK_POWE	"powe"
#define PKTFWDBR_JSON_TXPK_MODU	"modu"
#define PKTFWDBR_JSON_TXPK_DATR	"datr"
#define PKTFWDBR_JSON_TXPK_CODR	"codr"
#define PKTFWDBR_JSON_TXPK_FDEV "fdev"
#define PKTFWDBR_JSON_TXPK_IPOL	"ipol"
#define PKTFWDBR_JSON_TXPK_PREA	"prea"
#define PKTFWDBR_JSON_TXPK_SIZE	"size"
#define PKTFWDBR_JSON_TXPK_DATA "data"
#define PKTFWDBR_JSON_TXPK_NCRC	"ncrc"
#define PKTFWDBR_JSON_TXPK_RSSI	"rssi"

#define PKTFWDBR_JSON_ERROR						"error"
#define PKTFWDBR_JSON_ERROR_TOO_LATE			"TOO_LATE"
#define PKTFWDBR_JSON_ERROR_TOO_EARLY			"TOO_EARLY"
#define PKTFWDBR_JSON_ERROR_COLLISION_PACKET	"COLLISION_PACKET"
#define PKTFWDBR_JSON_ERROR_COLLISION_BEACON	"COLLISION_BEACON"
#define PKTFWDBR_JSON_ERROR_TX_FREQ				"TX_FREQ"
#define PKTFWDBR_JSON_ERROR_TX_POWER			"TX_POWER"
#define PKTFWDBR_JSON_ERROR_GPS_UNLOCKED		"GPS_UNLOCKED"

enum pktfwdbr_txack_error {
	PKTFWDBR_TXACK_ERROR_NONE
};

struct pktfwdpkt {
	struct pktfwdpkt_rfparams rfparams;
	const gchar* data;
	gsize size;
	guint32 timestamp;
};

void pktfwdbr_onbrokerconnect(const struct context* cntx);
void pktfwdbr_onmsg(struct context* cntx, const JsonObject* rootobj,
		char** splittopic, int numtopicparts);
#endif
