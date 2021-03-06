#pragma once

#if (defined(__SQLITEGEN) || defined(__JSONGEN))
#include <codegen/fakeglib.h>
#else
#include <json-glib/json-glib.h>
#include <lorawan/lorawan.h>
#include "tlwbe.h"
#endif
#include "pktfwdbr_rx.h"

struct uplink {
#ifdef __SQLITEGEN
	guint64 id;
#endif
	guint64 timestamp;
	const gchar* appeui;
	const gchar* deveui;
	guint8 port;
	const guint8* payload;
#ifndef __SQLITEGEN
	gsize payloadlen;
#endif
	struct pktfwdbr_rx_rfparams rfparams;
#ifdef __SQLITEGEN
	void __sqlitegen_flags_id_hidden;
	void __sqlitegen_constraints_id_notnull_primarykey_autoincrement_unique;
	void __sqlitegen_flags_appeui_searchable;
	void __sqlitegen_constraints_appeui_notnull;
	void __sqlitegen_flags_deveui_searchable;
	void __sqlitegen_constraints_deveui_notnull;
#endif
};
#ifdef __SQLITEGEN
typedef struct uplink __sqlitegen_table_uplinks;
#endif
#ifdef __JSONGEN
typedef struct uplink __jsongen_parser;
typedef struct uplink __jsongen_builder;
#endif

#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
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
	int datalen, const struct pktfwdbr_rx* rxpkt);

gboolean uplink_havequeueduplink(void);
gboolean uplink_cleanup(gpointer data);
void uplink_onbrokerconnect(const struct context* cntx);
void uplink_onmsg(struct context* cntx, char** splittopic, int numtopicparts,
	const JsonObject* rootobj);
#endif
