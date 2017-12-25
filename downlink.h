#pragma once

#include <glib.h>

#include "pktfwdbr.h"

gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		guint64 delay, const struct pktfwdpkt* rxpkt);
