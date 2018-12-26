#pragma once

#if defined(__SQLITEGEN) || defined(__JSONGEN)
#include "codegen/fakeglib.h"
#else
#include "tlwbe.h"
#include "pktfwdbr.h"
#endif

struct session {
	const gchar* deveui;
	const gchar* devnonce;
	const gchar* appnonce;
	const gchar* devaddr;
#ifdef __SQLITEGEN
	// the frame counters exist in table but shouldn't be directly manipulated
	// so they are hidden from code
	guint32	upcounter;
	guint32 downcounter;
	void __sqlitegen_flags_deveui_searchable;
	void __sqlitegen_constraints_deveui_notnull_primarykey_unique;
	void __sqlitegen_constraints_devnonce_notnull;
	void __sqlitegen_constraints_appnonce_notnull;
	void __sqlitegen_flags_upcounter_hidden;
	void __sqlitegen_constraints_upcounter_notnull;
	void __sqlitegen_default_upcounter_0;
	void __sqlitegen_flags_downcounter_hidden;
	void __sqlitegen_constraints_downcounter_notnull;
	void __sqlitegen_default_downcounter_0;
#endif
};
#ifdef __SQLITEGEN
typedef struct session __sqlitegen_table_sessions;
#endif


struct joinannounce {
	guint64 timestamp;
};
#ifdef __JSONGEN
typedef struct joinannounce __jsongen_builder;
#endif

#if !defined(__SQLITEGEN) && !defined(__JSONGEN)
void join_processjoinrequest(struct context* cntx, const gchar* gateway,
	guchar* data, int datalen, const struct pktfwdpkt* rxpkt);
#endif
