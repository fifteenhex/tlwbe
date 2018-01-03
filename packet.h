#pragma once

#include <glib.h>
#include "uplink.h"
#include "database.h"

struct packet_unpacked_data {
	guint32 devaddr;
	gboolean adr, adrackreq, ack, pending;
	guint8 foptscount;
	guint8 fopts[16];
	guint16 framecount;
	guint8 port;
	guint8* payload;
	gsize payloadlen;
};

struct packet_unpacked_joinreq {
	uint64_t appeui;
	uint64_t deveui;
	uint16_t devnonce;
};

struct packet_unpacked {
	guint8 type;
	struct packet_unpacked_data data;
	struct packet_unpacked_joinreq joinreq;
	guint32 mic;
};

guint8* packet_build_joinresponse(const struct session* s, const char* appkey,
		gsize* pktlen);
guint8* packet_build_data(guint8 type, guint32 devaddr, gboolean adr,
		gboolean ack, guint32 framecounter, guint8 port, guint8* payload,
		gsize payloadlen, struct sessionkeys* keys, gsize* pktlen);
guint8* packet_build_dataack(guint8 type, guint32 devaddr, guint32 framecounter,
		struct sessionkeys* keys, gsize* pktlen);
void packet_unpack(guint8* data, gsize len, struct packet_unpacked* result);
void packet_pack(struct packet_unpacked* unpacked, struct sessionkeys* keys);
void packet_debug(guint8* data, gsize len);
