#include <stdio.h>
#include <string.h>

#include "uplink.h"
#include "utils.h"
#include "lorawan.h"
#include "database.h"
#include "crypto.h"

struct sessionkeys {
	uint8_t nwksk[SESSIONKEYLEN];
	uint8_t appsk[SESSIONKEYLEN];
};

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

	crypto_calculatesessionkeys(key, appnonce, netid, devnonce, keys->nwksk,
			keys->appsk);

	gchar* nskhex = utils_bin2hex(&keys->nwksk, sizeof(keys->nwksk));
	gchar* askhex = utils_bin2hex(&keys->appsk, sizeof(keys->appsk));

	g_message("network session key; %s, application session key; %s", nskhex,
			askhex);

	g_free(nskhex);
	g_free(askhex);
}

void uplink_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdpkt* rxpkt) {

	gchar* datahex = utils_bin2hex(data, datalen);
	g_message("raw uplink %s", datahex);
	g_free(datahex);

	guchar* datastart = data;
	guchar* dataend = data + (datalen - MICLEN);
	gsize datasizewithoutmic = dataend - datastart;

	struct lorawan_fhdr* fhdr = data + 1;
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
	crypto_fillinblock(b0, 0x49, 0, devaddr, fullfcnt, datasizewithoutmic);

	struct sessionkeys keys;
	database_keyparts_get(cntx, devaddrascii, uplink_process_rowcallback,
			&keys);

	guint32 calcedmic = crypto_mic_2(keys.nwksk, KEYLEN, b0, sizeof(b0),
			datastart, datasizewithoutmic);

	g_message("uplink from %s to port %d (%d fopts, %d fcnt, %d bytes of payload, mic %08"
			G_GINT32_MODIFIER"x calcedmic %08"G_GINT32_MODIFIER"x)",devaddrascii, fport, numfopts, (int) fcnt, payloadlen, mic, calcedmic);

	if (mic == calcedmic) {

	} else
		g_message("bad mic, dropping");
}
