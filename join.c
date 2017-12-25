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

static void join_processjoinrequest_rowcallback(const struct dev* d, void* data) {
	uint8_t* key = data;
	utils_hex2bin(d->key, key, KEYLEN);
}

static void join_processjoinrequest_createsession(struct context* cntx,
		const gchar* deveui, struct session* s) {
	//remove any existing session
	database_session_del(cntx, deveui);

	//create a new session
	uint8_t nonce[APPNONCELEN];
	uint8_t devaddr[DEVADDRLEN];
	crypto_randbytes(nonce, sizeof(nonce));
	crypto_randbytes(devaddr, sizeof(devaddr));
	gchar* noncestr = utils_bin2hex(nonce, sizeof(nonce));
	gchar* devaddrstr = utils_bin2hex(devaddr, sizeof(devaddr));

	s->deveui = deveui;
	s->appnonce = noncestr;
	s->devaddr = devaddrstr;

	g_message("new session for %s, %s %s", s->deveui, s->appnonce, s->devaddr);

	database_session_add(cntx, s);
}

static void join_processjoinrequest_createjoinresponse(const struct session* s,
		const char* appkey, uint8_t* pkt) {

	uint8_t plainpkt[LORAWAN_PAYLOADWITHMIC(sizeof(struct lorawan_joinaccept))];
	memset(plainpkt, 0, sizeof(plainpkt));

	struct lorawan_joinaccept* response = &plainpkt;
	utils_hex2bin(s->appnonce, &response->appnonce, sizeof(response->appnonce));
	utils_hex2bin(s->devaddr, &response->devaddr, sizeof(response->devaddr));

	uint32_t mic = crypto_mic(appkey, KEYLEN, plainpkt,
			sizeof(plainpkt) - MICLEN);
	memcpy(plainpkt + sizeof(*response), &mic, sizeof(mic));

	pkt[0] = (MHDR_MTYPE_JOINACK << MHDR_MTYPE_SHIFT);
	crypto_encryptfordevice(appkey, plainpkt, sizeof(plainpkt), pkt + 1);
}

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen, const struct pktfwdpkt* rxpkt) {
	guchar* pkt = data + 1;
	int joinreqlen = 1 + sizeof(struct lorawan_joinreq) + 4;
	if (datalen == joinreqlen) {
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
		uint8_t key[KEYLEN];
		database_dev_get(cntx, asciieui, join_processjoinrequest_rowcallback,
				key);

		guint32 calcedmic = crypto_mic(key, sizeof(key), data,
				sizeof(*joinreq) + 1);

	if (calcedmic == inmic) {
		struct session s;
		join_processjoinrequest_createsession(cntx, asciieui, &s);

		uint8_t pkt[LORAWAN_PKTSZ(sizeof(struct lorawan_joinaccept))];
		join_processjoinrequest_createjoinresponse(&s, key, pkt);

		g_free(s.appnonce);
		g_free(s.devaddr);

		gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
		gsize payloadlen;
		gchar* payload = downlink_createtxjson(pkt, sizeof(pkt), &payloadlen, 5000000, rxpkt);
		mosquitto_publish(cntx->mosq,NULL, topic, payloadlen, payload, 0, false);
		g_free(topic);
	}
	else
	g_message("mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x", inmic, calcedmic);

} else
	g_message("join request should be %d bytes long, have %d", joinreqlen,
			datalen);
}

