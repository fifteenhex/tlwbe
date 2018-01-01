#pragma once

#include <glib.h>
#include "uplink.h"

guint8* packet_build(guint8 type, guint32 devaddr, struct sessionkeys* keys,
		gsize* pktlen);
