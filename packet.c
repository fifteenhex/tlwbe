#include "packet.h"
#include "lorawan.h"
#include "crypto.h"

guint8* packet_build(guint8 type, guint32 devaddr, struct sessionkeys* keys,
		gsize* pktlen) {

	GByteArray* cnfpkt = g_byte_array_new();
	guint8 mhdr = (type << MHDR_MTYPE_SHIFT);
	g_byte_array_append(cnfpkt, &mhdr, sizeof(mhdr));

	g_byte_array_append(cnfpkt, &devaddr, sizeof(devaddr));

	guint8 fctrl = LORAWAN_FHDR_FCTRL_ACK;
	g_byte_array_append(cnfpkt, &fctrl, sizeof(fctrl));

	guint16 fcnt = 0;
	g_byte_array_append(cnfpkt, &fcnt, sizeof(fcnt));

	uint8_t b0[BLOCKLEN];
	guint32 fullfcnt = 0;

	crypto_fillinblock_updownlink(b0, 1, devaddr, fullfcnt, cnfpkt->len);
	guint32 mic = crypto_mic_2(keys->nwksk, KEYLEN, b0, sizeof(b0),
			cnfpkt->data, cnfpkt->len);

	g_byte_array_append(cnfpkt, &mic, sizeof(mic));

	*pktlen = cnfpkt->len;
	return g_byte_array_free(cnfpkt, FALSE);
}
