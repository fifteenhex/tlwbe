#pragma once

#include <glib.h>

#include "pktfwdbr.h"
#include "regional.h"

#define DOWNLINK_SUBTOPIC "downlink"
#define DOWNLINK_QUEUE	"queue"

void downlink_dodownlink(struct context* cntx, const gchar* gateway,
		guint8* pkt, gsize pktlen, const struct pktfwdpkt* rxpkt,
		enum RXWINDOW rxwindow);

void downlink_onbrokerconnect(struct context* cntx);
void downlink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
gboolean downlink_cleanup(gpointer data);
