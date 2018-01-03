#include <string.h>

#include "packet.h"
#include "lorawan.h"
#include "crypto.h"
#include "utils.h"

guint8* packet_build_data(guint8 type, guint32 devaddr, gboolean adr,
		gboolean ack, guint32 framecounter, guint8 port, guint8* payload,
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
		g_byte_array_append(pkt, &port, sizeof(port));
		g_byte_array_append(pkt, payload, payloadlen);
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
guint8* packet_build_dataack(guint8 type, guint32 devaddr, guint32 framecounter,
		struct sessionkeys* keys, gsize* pktlen) {
	return packet_build_data(type, devaddr, FALSE, TRUE, framecounter, 0, NULL,
			0, keys, pktlen);
}

#define COPYANDINC(dst, src)	memcpy(dst, src, sizeof(*dst));\
									src += sizeof(*dst)

void packet_unpack(guint8* data, gsize len, struct packet_unpacked* result) {
	guint8* dataend = data + (len - sizeof(result->mic));

	guint8 mhdr = *data++;
	result->type = LORAWAN_TYPE(mhdr);

	switch (result->type) {
	case MHDR_MTYPE_JOINREQ: {
		COPYANDINC(&result->joinreq.appeui, data);
		COPYANDINC(&result->joinreq.deveui, data);
		COPYANDINC(&result->joinreq.devnonce, data);
	}
		break;
	case MHDR_MTYPE_UNCNFUP:
	case MHDR_MTYPE_UNCNFDN:
	case MHDR_MTYPE_CNFUP:
	case MHDR_MTYPE_CNFDN: {
		COPYANDINC(&result->data.devaddr, data);

		// parse fctrl byte
		guint8 fctrl = *data++;
		result->data.foptscount = fctrl & LORAWAN_FHDR_FCTRL_FOPTLEN_MASK;
		result->data.adr = (fctrl & LORAWAN_FHDR_FCTRL_ADR) ? 1 : 0;
		result->data.adrackreq = (fctrl & LORAWAN_FHDR_FCTRL_ADRACKREQ) ? 1 : 0;
		result->data.ack = (fctrl & LORAWAN_FHDR_FCTRL_ACK) ? 1 : 0;
		result->data.pending = (fctrl & LORAWAN_FHDR_FCTRL_FPENDING) ? 1 : 0;

		COPYANDINC(&result->data.framecount, data);
		for (int i = 0; i < result->data.foptscount; i++)
			result->data.fopts[i] = *data++;

		// port and payload are optional
		if (data != dataend) {
			result->data.port = *data++;
			result->data.payload = data;
			result->data.payloadlen = dataend - data;
			data += result->data.payloadlen;
		} else {
			result->data.port = 0;
			result->data.payloadlen = 0;
		}
	}
		break;
	default:
		g_message("don't know how to unpack packet type %d", result->type);
		break;
	}

	COPYANDINC(&result->mic, data);
}

void packet_pack(struct packet_unpacked* unpacked, struct sessionkeys* keys) {
	gsize pktlen;
	guint8* pkt = packet_build_data(unpacked->type, unpacked->data.devaddr,
			unpacked->data.adr, unpacked->data.ack, unpacked->data.framecount,
			unpacked->data.port, unpacked->data.payload,
			unpacked->data.payloadlen, keys, &pktlen);
	packet_debug(pkt, pktlen);
	g_free(pkt);
}

#define PRINTBOOL(b) (b ? "true":"false")

void packet_debug(guint8* data, gsize len) {
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
}
