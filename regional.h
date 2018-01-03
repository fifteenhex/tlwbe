#pragma once

#include <glib.h>

enum RXWINDOW {
	RXW_R1, RXW_R2, RXW_J1, RXW_J2
};

guint64 regional_getwindowdelay(enum RXWINDOW rxwindow);
