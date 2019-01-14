#include <lorawan/packet.h>

#include "config.h"
#include "packet.h"
#include "utils.h"

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
		g_message("type: %s", types[up.type]);
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
