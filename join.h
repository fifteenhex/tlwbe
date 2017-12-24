#pragma once

#include "tlwbe.h"
#include "pktfwdbr.h"

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen, const struct pktfwdpkt* rxpkt);
