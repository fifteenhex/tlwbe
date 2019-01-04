#pragma once

#include <glib.h>

#include "regional_temp.h"
#include "pktfwdbr_rx.h"

enum RXWINDOW {
	RXW_R1, RXW_R2, RXW_J1, RXW_J2
};

gboolean regional_init(struct regional* regional, const gchar* path,
		const gchar* region);
guint64 regional_getwindowdelay(enum RXWINDOW rxwindow);
const gchar* regional_getdatarate(struct regional* regional,
		enum RXWINDOW rxwindow, const struct pktfwdbr_rx* rxpkt);
gdouble regional_getfrequency(struct regional* regional, enum RXWINDOW rxwindow,
		const struct pktfwdbr_rx* rxpkt);
