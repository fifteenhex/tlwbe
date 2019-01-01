#pragma once

#if defined(__SQLITEGEN) || defined(__JSONGEN)
#include "codegen/fakeglib.h"
#else
#include <glib.h>
#endif

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
};

struct pktfwdbr_rx {
	struct pktfwdbr_rx_rfparams rfparams;
	const gchar* data;
	gsize size;
	guint32 timestamp;
#ifdef __JSONGEN
	void __jsongen_flags_rfparams_inline;
#endif
};
#ifdef __JSONGEN
typedef struct pktfwdbr_rx __jsongen_parser;
#endif
