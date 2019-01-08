#pragma once

#include <glib.h>
#include <lorawan/packet.h>
#include "uplink.h"
#include "database.h"
#include "regional_temp.h"

guint8* packet_build_data(guint8 type, guint32 devaddr, gboolean adr,
		gboolean ack, guint32 framecounter, guint8 port, const guint8* payload,
		gsize payloadlen, struct sessionkeys* keys, gsize* pktlen);
void packet_debug(guint8* data, gsize len);
