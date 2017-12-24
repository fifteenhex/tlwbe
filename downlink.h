#pragma once

#include <glib.h>

#include "pktfwdbr.h"

gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		const struct pktfwdpkt* rxpkt);
