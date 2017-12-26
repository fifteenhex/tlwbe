#pragma once

#include <glib.h>

#include "pktfwdbr.h"

#define DOWNLINK_SUBTOPIC "downlink"

gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		guint64 delay, const struct pktfwdpkt* rxpkt);

void downlink_onbrokerconnect(struct context* cntx);
void downlink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
