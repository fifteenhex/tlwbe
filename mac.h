#pragma once

#include <glib.h>

#include "tlwbe.h"
#include "pktfwdbr_rx.h"

void mac_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdbr_rx* rxpkt);
