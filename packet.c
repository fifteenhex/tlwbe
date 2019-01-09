#include <string.h>
#include <lorawan/crypto.h>

#include "config.h"
#include "packet.h"
#include "utils.h"

guint8* packet_build_data(guint8 type, guint32 devaddr, gboolean adr,
		gboolean ack, guint32 framecounter, guint8 port, const guint8* payload,
		gsize payloadlen, struct sessionkeys* keys, gsize* pktlen) {

	uint8_t dir =
			(type == MHDR_MTYPE_CNFDN || type == MHDR_MTYPE_UNCNFDN) ? 1 : 0;

	GByteArray* pkt = g_byte_array_new();
	guint8 mhdr = (type << MHDR_MTYPE_SHIFT);
	g_byte_array_append(pkt, &mhdr, sizeof(mhdr));

	g_byte_array_append(pkt, (void*) &devaddr, sizeof(devaddr));

	guint8 fctrl = 0;
	if (adr)
		fctrl |= LORAWAN_FHDR_FCTRL_ADR;
	if (ack)
		fctrl |= LORAWAN_FHDR_FCTRL_ACK;
	g_byte_array_append(pkt, &fctrl, sizeof(fctrl));

	guint16 fcnt = framecounter & 0xffff;
	g_byte_array_append(pkt, (const guint8*) &fcnt, sizeof(fcnt));

	if (payload != NULL) {
		guint8 encryptedpayload[128];
		payloadlen = MIN(sizeof(encryptedpayload), payloadlen);
		uint8_t* key = (uint8_t*) (port == 0 ? &keys->nwksk : &keys->appsk);
		crypto_endecryptpayload(key, true, devaddr, framecounter, payload,
				encryptedpayload, payloadlen);
		g_byte_array_append(pkt, &port, sizeof(port));
		g_byte_array_append(pkt, encryptedpayload, payloadlen);
	}

	uint8_t b0[BLOCKLEN];

	crypto_fillinblock_updownlink(b0, dir, devaddr, framecounter, pkt->len);
	guint32 mic = crypto_mic_2(keys->nwksk, KEYLEN, b0, sizeof(b0), pkt->data,
			pkt->len);
	g_message("pkt len %d", pkt->len);

	g_byte_array_append(pkt, (const guint8*) &mic, sizeof(mic));

	*pktlen = pkt->len;
	return g_byte_array_free(pkt, FALSE);
}

#define PRINTBOOL(b) (b ? "true":"false")

void packet_debug(guint8* data, gsize len) {
#if TLWBE_DEBUG_PRINTPACKETS
	static const char* types[] = { "join request", "join accept",
			"unconfirmed data up", "unconfirmed data down", "confirmed data up",
			"confirmed data down", "RFU", "proprietary" };

	struct packet_unpacked up;
	packet_unpack(data, len, &up);

	gchar* datahex = utils_bin2hex(data, len);

	switch (up.type) {
	case MHDR_MTYPE_JOINREQ: {
		g_message(
				"type: %s\n" //
				"\tappeui: %"G_GINT64_MODIFIER"x\n"//
				"\tdeveui: %"G_GINT64_MODIFIER"x\n"//
				"\tdevnonce: %"G_GINT16_MODIFIER"x\n"//
				"\tmic: %"G_GINT32_MODIFIER"x\n"//
				"\traw: %s",//
				types[up.type], up.joinreq.appeui, up.joinreq.deveui,
				up.joinreq.devnonce, up.mic, datahex);
	}
		break;
	case MHDR_MTYPE_JOINACK: {
		g_message("type: %s\n", types[up.type]);
	}
		break;
	case MHDR_MTYPE_UNCNFUP:
	case MHDR_MTYPE_UNCNFDN:
	case MHDR_MTYPE_CNFUP:
	case MHDR_MTYPE_CNFDN: {
		g_message(
				"type: %s\n" //
				"\tdevaddr: %"G_GINT32_MODIFIER"x\n"//
				"\tadr: %s, adrackreq: %s, ack: %s, pending: %s\n"//
				"\tfoptscount: %"G_GINT32_MODIFIER"x\n"//
				"\tframecount: %"G_GINT32_MODIFIER"d\n"//
				"\tport: %d\n"//
				"\tpayloadlen: %"G_GSIZE_FORMAT"\n"//
				"\tmic: %"G_GINT32_MODIFIER"x\n"//
				"\traw: %s",//
				types[up.type], up.data.devaddr, PRINTBOOL(up.data.adr),
				PRINTBOOL(up.data.adrackreq), PRINTBOOL(up.data.ack),
				PRINTBOOL(up.data.pending), up.data.foptscount,
				up.data.framecount, (int ) up.data.port, up.data.payloadlen,
				up.mic, datahex);
	}
		break;
	default:
		g_message("type: %s\n" //
				"\tmic: %"G_GINT32_MODIFIER"x\n"//
				"\traw: %s",//
				types[up.type], up.mic, datahex);
		break;
	}

	g_free(datahex);
#endif
}
