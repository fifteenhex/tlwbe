#include <lorawan/packet.h>

#include "mac.h"

void mac_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdbr_rx* rxpkt) {

	struct packet_unpacked unpacked;
	packet_unpack(data, datalen, &unpacked);

	if (unpacked.data.adr)
		g_message("%x is asking for adr, we don't do adr",
				(unsigned )unpacked.data.devaddr);

}
