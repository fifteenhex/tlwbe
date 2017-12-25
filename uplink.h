#pragma once

#include <glib.h>
#include "tlwbe.h"
#include "pktfwdbr.h"

void uplink_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdpkt* rxpkt);
