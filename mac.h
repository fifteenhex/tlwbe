#pragma once

#include "packet.h"

void mac_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdpkt* rxpkt);
