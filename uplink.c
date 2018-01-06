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
	utils_hex2bin(keyparts->key, key, sizeof(key));
	uint32_t appnonce = utils_hex2u24(keyparts->appnonce);
	uint32_t netid = 0;
	uint16_t devnonce = utils_hex2u16(keyparts->devnonce);

	g_message("appnonce %"G_GINT32_MODIFIER"x ""devnonce %"G_GINT32_MODIFIER"x",
			appnonce, devnonce);

	keys->appeui = g_strdup(keyparts->appeui);
	keys->deveui = g_strdup(keyparts->deveui);

	keys->nwksk[0] = 0xaa;

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

	struct packet_unpacked unpacked;
	packet_unpack(data, datalen, &unpacked);

	guchar* datastart = data;
	guchar* dataend = data + (datalen - MICLEN);
	gsize datasizewithoutmic = dataend - datastart;

	gboolean confirm = unpacked.type == MHDR_MTYPE_CNFUP;

	gchar devaddrascii[DEVADDRASCIILEN];
	sprintf(devaddrascii, "%08"G_GINT32_MODIFIER"x", unpacked.data.devaddr);

	uint8_t b0[BLOCKLEN];
	guint32 fullfcnt = unpacked.data.framecount;
	crypto_fillinblock_updownlink(b0, 0, unpacked.data.devaddr, fullfcnt,
			datasizewithoutmic);

	struct sessionkeys keys = { 0 };
	database_keyparts_get(cntx, devaddrascii, uplink_process_rowcallback,
			&keys);
	if (!keys.found) {
		g_message("didn't find session keys");
		return;
	}

	packet_pack(&unpacked, &keys);

	guint32 calcedmic = crypto_mic_2(keys.nwksk, KEYLEN, b0, sizeof(b0),
			datastart, datasizewithoutmic);

	if (unpacked.mic == calcedmic) {
		uint8_t* key = (uint8_t*) (
				unpacked.data.port == 0 ? &keys.nwksk : &keys.appsk);

		uint8_t decrypted[128];
		crypto_endecryptpayload(key, false, unpacked.data.devaddr, fullfcnt,
				unpacked.data.payload, decrypted, unpacked.data.payloadlen);

		gchar* decryptedhex = utils_bin2hex(decrypted,
				unpacked.data.payloadlen);
		g_message("decrypted payload: %s", decryptedhex);
		g_free(decryptedhex);

		//TODO we should OR this onto the top 16 bits AFAIK
		database_framecounter_up_set(cntx, devaddrascii,
				unpacked.data.framecount);

		struct uplink ul = { .timestamp = g_get_real_time(), .appeui =
				keys.appeui, .deveui = keys.deveui, .port = unpacked.data.port,
				.payload = decrypted, .payloadlen = unpacked.data.payloadlen };
		database_uplink_add(cntx, &ul);

		uplink_process_publish(cntx, keys.appeui, keys.deveui,
				unpacked.data.port, decrypted, unpacked.data.payloadlen);

		int queueddownlinks = database_downlinks_count(cntx, keys.appeui,
				keys.deveui);
		g_message("have %d queued downlinks", queueddownlinks);

		if (confirm) {
			gsize cnfpktlen;
			gint64 framecounter = database_framecounter_down_getandinc(cntx,
					devaddrascii);
			guint8* cnfpkt = packet_build_dataack(MHDR_MTYPE_UNCNFDN,
					unpacked.data.devaddr, framecounter, &keys, &cnfpktlen);
			packet_debug(cnfpkt, cnfpktlen);
			downlink_dodownlink(cntx, gateway, cnfpkt, cnfpktlen, rxpkt,
					RXW_R1);
			g_free(cnfpkt);
		}
	} else
		g_message("bad mic, dropping");

	g_free(keys.appeui);
	g_free(keys.deveui);
}

gboolean uplink_cleanup(gpointer data) {
	guint64 timeout = (((24 * 60) * 60) * (guint64) 1000000);
	struct context* cntx = data;
	database_uplinks_clean(cntx, g_get_real_time() - timeout);
	return TRUE;
}
