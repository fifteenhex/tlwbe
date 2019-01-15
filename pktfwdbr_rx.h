#pragma once

#include <codegen/glibwrapper.h>

#define PKTFWDBR_JSON_TXPK_ROOT	"txpk"
#define PKTFWDBR_JSON_TXPK_IMME "imme"
#define PKTFWDBR_JSON_TXPK_TMST	"tmst"//
#define PKTFWDBR_JSON_TXPK_TMMS "tmms"
#define PKTFWDBR_JSON_TXPK_FREQ	"freq"//
#define PKTFWDBR_JSON_TXPK_RFCH	"rfch"//
#define PKTFWDBR_JSON_TXPK_POWE	"powe"
#define PKTFWDBR_JSON_TXPK_MODU	"modu"//
#define PKTFWDBR_JSON_TXPK_DATR	"datr"//
#define PKTFWDBR_JSON_TXPK_CODR	"codr"//
#define PKTFWDBR_JSON_TXPK_FDEV "fdev"
#define PKTFWDBR_JSON_TXPK_IPOL	"ipol"
#define PKTFWDBR_JSON_TXPK_PREA	"prea"
#define PKTFWDBR_JSON_TXPK_SIZE	"size"
#define PKTFWDBR_JSON_TXPK_DATA "data"
#define PKTFWDBR_JSON_TXPK_NCRC	"ncrc"
#define PKTFWDBR_JSON_TXPK_RSSI	"rssi"

struct pktfwdbr_rx_rfparams {
	// basic rf bits
	const gchar* modulation;
	gdouble frequency;
	gint16 rssi;
	// lora specifc stuff
	const gchar* datarate;
	const gchar* coderate;
	// misc
	guint32 rfchain;
#ifdef __JSONGEN
	void __jsongen_member_modulation_modu;
	void __jsongen_member_frequency_freq;
	void __jsongen_member_datarate_datr;
	void __jsongen_member_coderate_codr;
	void __jsongen_member_rfchain_rfch;
#endif
};

struct pktfwdbr_rx {
	struct pktfwdbr_rx_rfparams rfparams;
	const gchar* data;
	gsize size;
	guint32 timestamp;
#ifdef __JSONGEN
	void __jsongen_flags_rfparams_inline;
	void __jsongen_member_timestamp_tmst;
#endif
};
#ifdef __JSONGEN
typedef struct pktfwdbr_rx __jsongen_parser;
#endif
