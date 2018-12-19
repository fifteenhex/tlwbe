#pragma once

#include <glib.h>

struct regional_rx2 {
	guint32 frequency;
	guint8 datarate;
};

struct regional {
	gchar* datarates[15];
	guint32 extrachannels[5];
	gboolean sendcflist;
	struct regional_rx2 rx2;
};
