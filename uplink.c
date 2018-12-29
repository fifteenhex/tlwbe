#include <stdio.h>
#include <string.h>

#include "config.h"
#include "uplink.h"
#include "utils.h"
#include "lorawan.h"
#include "database.h"
#include "crypto.h"
#include "packet.h"
#include "downlink.h"
#include "flags.h"

#include "json-glib-macros/jsonbuilderutils.h"

#include "uplink.json.h"

static void uplink_process_publish(struct context* cntx,
		const struct uplink* uplink, const struct flags* flags) {
	GString* topicstr = g_string_new(TLWBE_TOPICROOT"/");
	g_string_append(topicstr, "uplink");
	g_string_append(topicstr, "/");
	g_string_append(topicstr, uplink->appeui);
	g_string_append(topicstr, "/");
	g_string_append(topicstr, uplink->deveui);
	g_string_append(topicstr, "/");
	g_string_append_printf(topicstr, "%d", uplink->port);
	gchar* topic = g_string_free(topicstr, FALSE);

	if (flags->uplink.raw)
		mosquitto_publish(
				mosquitto_client_getmosquittoinstance(cntx->mosqclient), NULL,
				topic, uplink->payloadlen, uplink->payload, 0, false);
	else {
		JsonBuilder* jsonbuilder = json_builder_new();
		__jsongen_uplink_to_json(uplink, jsonbuilder);
		mosquitto_client_publish_json_builder(cntx->mosqclient, jsonbuilder,
				topic);
	}

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

#if TLWBE_DEBUG_REPACKPACKETS
	packet_pack(&unpacked, &keys);
#endif

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
		memcpy(&ul.rfparams, &rxpkt->rfparams, sizeof(ul.rfparams));

		struct flags flags;
		flags_forapp(cntx, ul.appeui, &flags);

		if (!flags.uplink.nostore)
			database_uplink_add(cntx, &ul);

		if (!flags.uplink.norealtime)
			uplink_process_publish(cntx, &ul, &flags);

		int queueddownlinks = database_downlinks_count(cntx, keys.appeui,
				keys.deveui);
		g_message("have %d queued downlinks", queueddownlinks);

		// if we need to ack or have a downlink to send we need to
		// build a packet and send it
		//fixme this shouldn't be in here
		if (confirm || queueddownlinks > 0) {

			struct downlink downlink;
			gboolean senddownlink = FALSE;
			gboolean confirmdownlink = FALSE;
			if (queueddownlinks > 0) {
				if (database_downlinks_get_first(cntx, keys.appeui, keys.deveui,
						&downlink)) {
					g_message(
							"going to transmit %"G_GSIZE_FORMAT" byte downlink %s to port %d",
							downlink.payloadlen, downlink.token, downlink.port);
					gchar* messagehex = utils_bin2hex(downlink.payload,
							downlink.payloadlen);
					g_message("%s", messagehex);
					senddownlink = TRUE;
					confirmdownlink = downlink.confirm;
				}
			}

			gsize pktlen;
			gint64 framecounter = database_framecounter_down_getandinc(cntx,
					devaddrascii);
			guint8* pkt = packet_build_data(
					confirmdownlink ? MHDR_MTYPE_CNFDN : MHDR_MTYPE_UNCNFDN,
					unpacked.data.devaddr, FALSE, confirm, framecounter,
					(senddownlink ? downlink.port : 0),
					(senddownlink ? downlink.payload : NULL),
					(senddownlink ? downlink.payloadlen : 0), &keys, &pktlen);

			packet_debug(pkt, pktlen);
			downlink_dorxwindowdownlink(cntx, gateway, pkt, pktlen, rxpkt);
			g_free(pkt);

			if (senddownlink) {
				database_downlinks_delete_by_token(cntx, downlink.token);
				downlink_announce_sent(cntx, downlink.token);
				g_free(downlink.appeui);
				g_free(downlink.deveui);
				g_free(downlink.payload);
				g_free(downlink.token);
			}

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

void uplink_onbrokerconnect(const struct context* cntx) {
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL,
			TLWBE_TOPICROOT "/" UPLINK_SUBTOPIC_UPLINKS "/" UPLINK_SUBTOPIC_UPLINKS_QUERY "/#",
			0);
}

static void uplink_onmsg_rowcallback(const struct uplink* uplink, void* data) {
	JsonBuilder* jsonbuilder = data;
	__jsongen_uplink_to_json(uplink, jsonbuilder);
}

void uplink_onmsg(struct context* cntx, char** splittopic, int numtopicparts,
		const JsonObject* rootobj) {

	if (numtopicparts != 2) {
		g_message("expected 2 topic parts, got %d", numtopicparts);
		return;
	}

	char* action = splittopic[0];
	if (strcmp(action, UPLINK_SUBTOPIC_UPLINKS_QUERY) == 0) {
		const gchar* deveui = json_object_get_string_member(rootobj, "deveui");

		char* token = splittopic[1];
		JsonBuilder* jsonbuilder = json_builder_new();
		json_builder_begin_object(jsonbuilder);
		json_builder_set_member_name(jsonbuilder, "uplinks");
		json_builder_begin_array(jsonbuilder);

		database_uplinks_get(cntx, deveui, uplink_onmsg_rowcallback,
				jsonbuilder);
		json_builder_end_array(jsonbuilder);
		json_builder_end_object(jsonbuilder);

		gchar* topic = mosquitto_client_createtopic(TLWBE_TOPICROOT,
		UPLINK_SUBTOPIC_UPLINKS, UPLINK_SUBTOPIC_UPLINKS_RESULT, token, NULL);
		mosquitto_client_publish_json_builder(cntx->mosqclient, jsonbuilder,
				topic);
		g_free(topic);
	} else {
		g_message("unexpected action %s", action);
	}
}
