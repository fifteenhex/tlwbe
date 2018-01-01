#include <stdio.h>
#include <string.h>

#include "uplink.h"
#include "utils.h"
#include "lorawan.h"
#include "database.h"
#include "crypto.h"
#include "packet.h"
#include "downlink.h"

static void uplink_process_publish(struct context* cntx, const gchar* appeui,
		const gchar* deveui, int port, const guint8* payload, gsize payloadlen) {
	GString* topicstr = g_string_new(TLWBE_TOPICROOT"/");
	g_string_append(topicstr, "uplink");
	g_string_append(topicstr, "/");
	g_string_append(topicstr, appeui);
	g_string_append(topicstr, "/");
	g_string_append(topicstr, deveui);
	g_string_append(topicstr, "/");
	g_string_append_printf(topicstr, "%d", port);
	gchar* topic = g_string_free(topicstr, FALSE);
	mosquitto_publish(cntx->mosq, NULL, topic, payloadlen, payload, 0, false);
	g_free(topic);
}

static void uplink_process_rowcallback(const struct keyparts* keyparts,
		void* data) {
	g_message("found session, %s %s %s", keyparts->key, keyparts->appnonce,
			keyparts->devnonce);

	struct sessionkeys* keys = data;

	uint8_t key[KEYLEN];
	uint8_t appnonce[APPNONCELEN];
	uint8_t netid[NETIDLEN] = { 0 };
	uint8_t devnonce[DEVNONCELEN];

	utils_hex2bin(keyparts->key, key, sizeof(key));
	utils_hex2bin(keyparts->appnonce, appnonce, sizeof(appnonce));
	utils_hex2bin(keyparts->devnonce, devnonce, sizeof(devnonce));

	keys->appeui = g_strdup(keyparts->appeui);
	keys->deveui = g_strdup(keyparts->deveui);

	crypto_calculatesessionkeys(key, appnonce, netid, devnonce, keys->nwksk,
			keys->appsk);

	gchar* nskhex = utils_bin2hex(&keys->nwksk, sizeof(keys->nwksk));
	gchar* askhex = utils_bin2hex(&keys->appsk, sizeof(keys->appsk));

	g_message("network session key; %s, application session key; %s", nskhex,
			askhex);

	g_free(nskhex);
	g_free(askhex);

	keys->found = TRUE;
}

void uplink_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdpkt* rxpkt) {

	gchar* datahex = utils_bin2hex(data, datalen);
	g_message("raw uplink %s", datahex);
	g_free(datahex);

	guchar* datastart = data;
	guchar* dataend = data + (datalen - MICLEN);
	gsize datasizewithoutmic = dataend - datastart;

	gboolean confirm = LORAWAN_TYPE(*data++) == MHDR_MTYPE_CNFUP;

	struct lorawan_fhdr* fhdr = (struct lorawan_fhdr*) data;
	data += sizeof(fhdr);

	guint32 devaddr = GUINT32_FROM_LE(fhdr->devaddr);
	gchar devaddrascii[DEVADDRASCIILEN];
	sprintf(devaddrascii,"%08"G_GINT32_MODIFIER"x", devaddr);

	int numfopts = fhdr->fctrl & LORAWAN_FHDR_FCTRL_FOPTLEN_MASK;
	uint8_t* fopts = data;
	data += numfopts;

	guint16 fcnt = GUINT16_FROM_LE(fhdr->fcnt);

	int fport = *data++;

	gsize payloadlen = dataend - data;
	guchar* payload = data;

	guint32 mic;
	memcpy(&mic, dataend, sizeof(mic));

	uint8_t b0[BLOCKLEN];
	guint32 fullfcnt = fcnt;
	crypto_fillinblock_updownlink(b0, 0, devaddr, fullfcnt, datasizewithoutmic);

	struct sessionkeys keys = { 0 };
	database_keyparts_get(cntx, devaddrascii, uplink_process_rowcallback,
			&keys);
	if (!keys.found) {
		g_message("didn't find session keys");
		return;
	}

	guint32 calcedmic = crypto_mic_2(keys.nwksk, KEYLEN, b0, sizeof(b0),
			datastart, datasizewithoutmic);
	g_message("uplink from %s to port %d (%d fopts, %d fcnt, %d bytes of payload, mic %08"
			G_GINT32_MODIFIER"x calcedmic %08"G_GINT32_MODIFIER"x)",devaddrascii, fport, numfopts, (int) fcnt, payloadlen, mic, calcedmic);

	if (mic == calcedmic) {
		uint8_t* key = (uint8_t*) (fport == 0 ? &keys.nwksk : &keys.appsk);
		uint8_t decrypted[128];
		crypto_decryptpayload(key, devaddr, fullfcnt, payload, decrypted,
				payloadlen);
		gchar* decryptedhex = utils_bin2hex(decrypted, payloadlen);
		g_message("decrypted payload: %s", decryptedhex);
		g_free(decryptedhex);

		uplink_process_publish(cntx, keys.appeui, keys.deveui, fport, decrypted,
				payloadlen);

		if (confirm) {
			gsize cnfpktlen;
			guint8* cnfpkt = packet_build(MHDR_MTYPE_UNCNFDN, devaddr, &keys,
					&cnfpktlen);
			gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
			gsize payloadlen;
			gchar* payload = downlink_createtxjson(cnfpkt, cnfpktlen,
					&payloadlen, 1000000, rxpkt);
			mosquitto_publish(cntx->mosq, NULL, topic, payloadlen, payload, 0,
			false);
			g_free(topic);
			g_free(cnfpkt);
		}
	} else
		g_message("bad mic, dropping");

	g_free(keys.appeui);
	g_free(keys.deveui);
}
