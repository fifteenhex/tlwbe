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
#include "packet.h"

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

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen, const struct pktfwdpkt* rxpkt) {

	struct packet_unpacked unpacked;
	packet_unpack(data, datalen, &unpacked);

	g_message(
			"handling join request for app %"G_GINT64_MODIFIER "x from %"G_GINT64_MODIFIER"x " "nonce %"G_GINT16_MODIFIER"x",
			unpacked.joinreq.appeui, unpacked.joinreq.deveui,
			unpacked.joinreq.devnonce);

	char asciieui[EUIASCIILEN];
	sprintf(asciieui, "%016"G_GINT64_MODIFIER"x", unpacked.joinreq.deveui);

	char asciidevnonce[DEVNONCEASCIILEN];
	sprintf(asciidevnonce, "%04"G_GINT16_MODIFIER"x",
			unpacked.joinreq.devnonce);

	uint8_t* key = NULL;
	database_dev_get(cntx, asciieui, join_processjoinrequest_rowcallback, &key);
	if (key == NULL) {
		g_message("couldn't find key for device %s", asciieui);
		goto out;
	}

	guint32 calcedmic = crypto_mic(key, KEYLEN, data, datalen - 4);

	if (calcedmic != unpacked.mic) {
		g_message(
				"mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x",
				unpacked.mic, calcedmic);
		goto out;
	}

	struct session s;
	join_processjoinrequest_createsession(cntx, asciieui, asciidevnonce, &s);

	gsize joinrespktlen;
	guint8* joinrespkt = packet_build_joinresponse(&s, key, &joinrespktlen);

	printsessionkeys(key, &s);

	g_free((void*) s.appnonce);
	g_free((void*) s.devaddr);

	downlink_dodownlink(cntx, gateway, joinrespkt, joinrespktlen, rxpkt,
			RXW_J2);

	out: if (key != NULL)
		g_free(key);
}

