#pragma once

#if defined(__JSONGEN)
#include "codegen/fakeglib.h"
#else
#include <glib.h>
#endif

#define PKTFWDBR_JSON_TXACK_ROOT "tx_ack"
#define PKTFWDBR_JSON_ERROR						"error"

enum pktfwdbr_txack_error {
	PKTFWDBR_TXACK_ERROR_NONE,
	PKTFWDBR_TXACK_ERROR_TOO_LATE,
	PKTFWDBR_TXACK_ERROR_TOO_EARLY,
	PKTFWDBR_TXACK_ERROR_COLLISION_PACKET,
	PKTFWDBR_TXACK_ERROR_COLLISION_BEACON,
	PKTFWDBR_TXACK_ERROR_TX_FREQ,
	PKTFWDBR_TXACK_ERROR_TX_POWER,
	PKTFWDBR_TXACK_ERROR_GPS_UNLOCKED
};

struct pktfwdbr_txack {
	enum pktfwdbr_txack_error error;
};

struct pktfwdbr_txack_wrapper {
	struct pktfwdbr_txack tx_ack;
};
#ifdef __JSONGEN
typedef struct pktfwdbr_txack_wrapper __jsongen_parser;
#endif
