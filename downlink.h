#pragma once

#if defined(__SQLITEGEN) || defined(__JSONGEN)
#include "codegen/fakeglib.h"
#else
#include <json-glib/json-glib.h>
#include "pktfwdbr.h"
#include "regional.h"
#endif

struct downlink {
#ifdef __SQLITEGEN
	guint64 id;
#endif
	guint64 timestamp;
	guint32 deadline;
#ifndef __JSONGEN
	const gchar* appeui;
	const gchar* deveui;
	guint8 port;
#endif
	const guint8* payload;
#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
	gsize payloadlen;
#endif
	gboolean confirm;
#ifndef __JSONGEN
	const gchar* token;
#endif
#ifdef __SQLITEGEN
	void __sqlitegen_flags_id_hidden;
	void __sqlitegen_constraints_id_notnull_primarykey_autoincrement_unique;
	void __sqlitegen_constraints_timestamp_notnull;
	void __sqlitegen_constraints_deadline_notnull;
	void __sqlitegen_constraints_appeui_notnull;
	void __sqlitegen_constraints_deveui_notnull;
	void __sqlitegen_constraints_port_notnull;
	void __sqlitegen_constraints_payload_notnull;
	void __sqlitegen_flags_token_searchable;
	void __sqlitegen_constraints_token_notnull_unique;
#endif
};
#ifdef __SQLITEGEN
	typedef struct downlink __sqlitegen_table_downlinks;
#endif
#ifdef __JSONGEN
	typedef struct downlink __jsongen_parser;
#endif

struct downlink_announce_sent {

};
#ifdef __JSONGEN
	typedef struct downlink_announce_sent __jsongen_builder;
#endif

struct downlink_schedule_result {

};
#ifdef __JSONGEN
	typedef struct downlink_schedule_result __jsongen_builder;
#endif

#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
#define DOWNLINK_SUBTOPIC "downlink"
#define DOWNLINK_SCHEDULE "schedule"

void downlink_dodownlink(struct context* cntx, const gchar* gateway,
	const gchar* token, guint8* pkt, gsize pktlen, const struct pktfwdpkt* rxpkt,
	enum RXWINDOW rxwindow);
void downlink_dorxwindowdownlink(struct context* cntx, const gchar* gateway,
	const gchar* token, guint8* pkt, gsize pktlen, const struct pktfwdpkt* rxpkt);

void downlink_onbrokerconnect(const struct context* cntx);
void downlink_onmsg(struct context* cntx, char** splittopic, int numtopicparts,
	const JsonObject* rootobj);
gboolean downlink_cleanup(gpointer data);
void downlink_announce_sent(struct context* cntx, const gchar* token);
void downlink_process_txack(const struct context* cntx, const gchar* token, enum pktfwdbr_txack_error error);
void downlink_init(struct context* cntx);
#endif
