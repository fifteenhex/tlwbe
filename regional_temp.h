#pragma once

#include <glib.h>

struct regional_rx2 {
	guint32 frequency;
};

struct regional {
	guint32 extrachannels[5];
	gboolean sendcflist;
	struct regional_rx2 rx2;
};
