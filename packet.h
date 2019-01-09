#pragma once

#include <glib.h>
#include <lorawan/packet.h>
#include "uplink.h"
#include "database.h"
#include "regional_temp.h"

void packet_debug(guint8* data, gsize len);
