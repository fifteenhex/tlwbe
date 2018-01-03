#include "regional.h"

guint64 regional_getwindowdelay(enum RXWINDOW rxwindow) {
	switch (rxwindow) {
	case RXW_R1:
		return 1000000;
	case RXW_R2:
		return 2000000;
	case RXW_J1:
		return 5000000;
	case RXW_J2:
		return 6000000;
	default:
		return 0;
	}
}
