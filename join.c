#include <glib.h>
#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>
#include <stdio.h>

#include "join.h"
#include "tlwbe.h"
#include "lorawan.h"
#include "pktfwdbr.h"
#include "crypto.h"
#include "utils.h"
#include "database.h"
#include "downlink.h"

static void printsessionkeys(const uint8_t* key, const struct session* s) {
	uint8_t nsk[SESSIONKEYLEN];
	uint8_t ask[SESSIONKEYLEN];

	uint8_t appnonce[APPNONCELEN];
	utils_hex2bin(s->appnonce, appnonce, sizeof(appnonce));
	uint8_t netid[NETIDLEN] = { 0 };
	uint8_t devnonce[DEVNONCELEN];
	utils_hex2bin(s->devnonce, devnonce, sizeof(devnonce));

	crypto_calculatesessionkeys(key, appnonce, netid, devnonce, nsk, ask);

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

static void join_processjoinrequest_createjoinresponse(const struct session* s,
		const char* appkey, uint8_t* pkt, gsize* pktlen) {

	gboolean cflist = FALSE;

	uint8_t plainpkt[LORAWAN_PKTSZ(sizeof(struct lorawan_joinaccept))];
	*pktlen = sizeof(plainpkt) - (cflist ? 0 : 16);

	memset(plainpkt, 0, *pktlen);
	plainpkt[0] = (MHDR_MTYPE_JOINACK << MHDR_MTYPE_SHIFT);

	struct lorawan_joinaccept* response =
			(struct lorawan_joinaccept*) &plainpkt[1];
	utils_hex2bin(s->appnonce, &response->appnonce, sizeof(response->appnonce));
	utils_hex2bin(s->devaddr, &response->devaddr, sizeof(response->devaddr));

	uint32_t mic = crypto_mic(appkey, KEYLEN, plainpkt, *pktlen - MICLEN);
	memcpy((plainpkt + *pktlen) - sizeof(mic), &mic, sizeof(mic));

	pkt[0] = (MHDR_MTYPE_JOINACK << MHDR_MTYPE_SHIFT);
	crypto_encryptfordevice(appkey, plainpkt + 1, *pktlen - 1, pkt + 1);

	gchar* plainpkthex = utils_bin2hex(plainpkt, *pktlen);
	g_message("plaintext packet: %s", plainpkthex);
	g_free(plainpkthex);
	gchar* cryptedpkthex = utils_bin2hex(pkt, *pktlen);
	g_message("crypted packet: %s", cryptedpkthex);
	g_free(cryptedpkthex);
}

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen, const struct pktfwdpkt* rxpkt) {

	guchar* pkt = data + 1;
	int joinreqlen = 1 + sizeof(struct lorawan_joinreq) + 4;
	if (datalen != joinreqlen) {
		g_message("join request should be %d bytes long, have %d", joinreqlen,
				datalen);
		goto out;
	}

	struct lorawan_joinreq* joinreq = (struct lorawan_joinreq*) pkt;
	guint64 appeui = GUINT64_FROM_LE(joinreq->appeui);
	guint64 deveui = GUINT64_FROM_LE(joinreq->deveui);
	guint16 devnonce = GUINT16_FROM_LE(joinreq->devnonce);
	g_message("handling join request for app %"G_GINT64_MODIFIER
			"x from %"G_GINT64_MODIFIER"x "
			"nonce %"G_GINT16_MODIFIER"x", appeui, deveui, devnonce);

	pkt += sizeof(*joinreq);
	guint32 inmic;
	memcpy(&inmic, pkt, sizeof(inmic));

	char asciieui[EUIASCIILEN];
	sprintf(asciieui, "%016"G_GINT64_MODIFIER"x", deveui);

	char asciidevnonce[DEVNONCEASCIILEN];
	sprintf(asciidevnonce, "%04"G_GINT16_MODIFIER"x", devnonce);

	uint8_t* key = NULL;
	database_dev_get(cntx, asciieui, join_processjoinrequest_rowcallback, &key);
	if (key == NULL) {
		g_message("couldn't find key for device %s", asciieui);
		goto out;
	}

	guint32 calcedmic = crypto_mic(key, KEYLEN, data, sizeof(*joinreq) + 1);

	if (calcedmic != inmic) {
		g_message("mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x", inmic, calcedmic);
		goto out;
	}

	struct session s;
	join_processjoinrequest_createsession(cntx, asciieui, asciidevnonce, &s);

	uint8_t joinpkt[LORAWAN_PKTSZ(sizeof(struct lorawan_joinaccept))];
	gsize joinpktlen;
	join_processjoinrequest_createjoinresponse(&s, key, joinpkt, &joinpktlen);

	printsessionkeys(key, &s);

	g_free((void*) s.appnonce);
	g_free((void*) s.devaddr);

	gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
	gsize payloadlen;
	gchar* payload = downlink_createtxjson(joinpkt, joinpktlen, &payloadlen,
			5000000, rxpkt);
	mosquitto_publish(cntx->mosq, NULL, topic, payloadlen, payload, 0, false);
	g_free(topic);

	out: if (key != NULL)
		g_free(key);
}

