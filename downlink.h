#pragma once

#include <glib.h>

#include "pktfwdbr.h"

#define DOWNLINK_SUBTOPIC "downlink"

enum DOWNLINK_RXWINDOW {
	RXW_R1, RXW_R2, RXW_J1, RXW_J2
};

void downlink_dodownlink(struct context* cntx, const gchar* gateway,
		guint8* pkt, gsize pktlen, const struct pktfwdpkt* rxpkt,
		enum DOWNLINK_RXWINDOW rxwindow);

void downlink_onbrokerconnect(struct context* cntx);
void downlink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
