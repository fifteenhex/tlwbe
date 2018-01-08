#pragma once

#include <glib.h>
#include "tlwbe.h"
#include "pktfwdbr.h"
#include "lorawan.h"

#define UPLINK_SUBTOPIC_UPLINKS			"uplinks"
#define UPLINK_SUBTOPIC_UPLINKS_QUERY	"query"
#define UPLINK_SUBTOPIC_UPLINKS_RESULT	"result"

struct sessionkeys {
	gboolean found;
	uint8_t nwksk[SESSIONKEYLEN];
	uint8_t appsk[SESSIONKEYLEN];
	char* appeui;
	char* deveui;
};

void uplink_process(struct context* cntx, const gchar* gateway, guchar* data,
		int datalen, const struct pktfwdpkt* rxpkt);

gboolean uplink_havequeueduplink(void);
gboolean uplink_cleanup(gpointer data);
void uplink_onbrokerconnect(struct context* cntx);
void uplink_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
