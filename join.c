#include <glib.h>
#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>
#include <stdio.h>

#include <lorawan/crypto.h>
#include <lorawan/packet.h>

#include "join.h"
#include "tlwbe.h"
#include "pktfwdbr.h"
#include "utils.h"
#include "database.h"
#include "downlink.h"
#include "packet.h"

#include "json-glib-macros/jsonbuilderutils.h"

#include "join.json.h"

static void printsessionkeys(const uint8_t* key, const struct session* s) {
	uint8_t nsk[SESSIONKEYLEN];
	uint8_t ask[SESSIONKEYLEN];

	uint32_t appnonce = utils_hex2u24(s->appnonce);
	uint32_t netid = 0;
	uint16_t devnonce = utils_hex2u16(s->devnonce);

	lorawan_crypto_calculatesessionkeys(key, appnonce, netid, devnonce, nsk,
			ask);

	gchar* nskhex = utils_bin2hex(nsk, sizeof(nsk));
	gchar* askhex = utils_bin2hex(ask, sizeof(ask));

	g_message("network session key; %s, application session key; %s", nskhex,
			askhex);
}

static void join_processjoinrequest_rowcallback(const struct dev* d, void* data) {
	uint8_t** key = data;
	*key = g_malloc(KEYLEN);
	utils_hex2bin(d->key, *key, KEYLEN);
}

static void join_processjoinrequest_createsession(struct context* cntx,
		const gchar* deveui, const gchar* devnonce, struct session* s) {
	//remove any existing session
	database_session_del(cntx, deveui);

	//create a new session
	guint32 appnonce;
	guint32 devaddr;
	crypto_randbytes(&appnonce, sizeof(appnonce));
	appnonce &= 0xffffff; // appnonce is only 3 bytes
	crypto_randbytes(&devaddr, sizeof(devaddr));

	gchar* appnoncestr = g_malloc(APPNONCEASCIILEN);
	sprintf(appnoncestr, "%06"G_GINT32_MODIFIER"x", appnonce);
	gchar* devaddrstr = g_malloc(DEVADDRASCIILEN);
	sprintf(devaddrstr, "%08"G_GINT32_MODIFIER"x", devaddr);

	s->deveui = deveui;
	s->devnonce = devnonce;
	s->appnonce = appnoncestr;
	s->devaddr = devaddrstr;

	g_message("new session for %s; devnonce: %s, appnonce: %s, devaddr: %s",
			s->deveui, s->devnonce, s->appnonce, s->devaddr);

	database_session_add(cntx, s);
}

static void join_announce(struct context* cntx, const gchar* appeui,
		const gchar* deveui) {
	gchar* topic = mosquitto_client_createtopic(TLWBE_TOPICROOT, "join", appeui,
			deveui, NULL);
	JsonBuilder* jsonbuilder = json_builder_new_immutable();
	struct joinannounce msg = { .timestamp = g_get_real_time() };
	__jsongen_joinannounce_to_json(&msg, jsonbuilder);
	mosquitto_client_publish_json_builder(cntx->mosqclient, jsonbuilder, topic,
	TRUE);
	g_free(topic);
}

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen, const struct pktfwdbr_rx* rxpkt) {

	struct packet_unpacked unpacked;
	packet_unpack(data, datalen, &unpacked);

	g_message(
			"handling join request for app %"G_GINT64_MODIFIER "x from %"G_GINT64_MODIFIER"x " "nonce %"G_GINT16_MODIFIER"x",
			unpacked.joinreq.appeui, unpacked.joinreq.deveui,
			unpacked.joinreq.devnonce);

	char asciieui[EUIASCIILEN];
	sprintf(asciieui, "%016"G_GINT64_MODIFIER"x", unpacked.joinreq.deveui);
	char asciiappeui[EUIASCIILEN];
	sprintf(asciiappeui, "%016"G_GINT64_MODIFIER"x", unpacked.joinreq.appeui);

	char asciidevnonce[DEVNONCEASCIILEN];
	sprintf(asciidevnonce, "%04"G_GINT16_MODIFIER"x",
			unpacked.joinreq.devnonce);

	uint8_t* key = NULL;
	database_dev_get(cntx, asciieui, NULL, join_processjoinrequest_rowcallback,
			&key);
	if (key == NULL) {
		g_message("couldn't find key for device %s", asciieui);
		goto out;
	}

	uint32_t packetmic;
	uint32_t calcedmic;
	if (!lorawan_packet_verifymic(key, data, datalen, &packetmic, &calcedmic)) {
		g_message(
				"mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x",
				packetmic, calcedmic);
		goto out;
	}

	struct session s;
	join_processjoinrequest_createsession(cntx, asciieui, asciidevnonce, &s);
	printsessionkeys(key, &s);
	gsize joinrespktlen;

	GByteArray* joinackpkt = g_byte_array_new();
	int pktbldret;
	if ((pktbldret = packet_build_joinresponse(utils_hex2u24(s.appnonce),
			utils_hex2u32(s.devaddr), cntx->regional.extrachannels, key,
			utils_gbytearray_writer, joinackpkt)) != LORAWAN_NOERR) {
		g_message("failed to build join ack: %d", pktbldret);
		goto out;
	}
	packet_debug(joinackpkt->data, joinackpkt->len);

	g_free((void*) s.appnonce);
	g_free((void*) s.devaddr);

	downlink_dodownlink(cntx, gateway, NULL, joinackpkt->data, joinackpkt->len,
			rxpkt, RXW_J1);
	//downlink_dodownlink(cntx, gateway, joinackpkt->data, joinackpkt->len, rxpkt,
	//		RXW_J2);
	g_byte_array_free(joinackpkt, TRUE);

	join_announce(cntx, asciiappeui, asciieui);

	out: if (key != NULL)
		g_free(key);
}

