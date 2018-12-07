#pragma once

#ifdef __SQLITEGEN
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
	guint32	upcounter;
	guint32 downcounter;
	void __sqlitegen_flags_deveui_searchable;
	void __sqlitegen_constraints_deveui_notnull_primarykey_unique;
	void __sqlitegen_constraints_devnonce_notnull;
	void __sqlitegen_constraints_appnonce_notnull;
	void __sqlitegen_flags_upcounter_hidden;
//	void __sqlitegen_constraints_upcounter_notnull;
	void __sqlitegen_flags_downcounter_hidden;
//	void __sqlitegen_constraints_downcounter_notnull;
#endif
};
#ifdef __SQLITEGEN
typedef struct session __sqlitegen_table_sessions;
#endif

#ifndef __SQLITEGEN
void join_processjoinrequest(struct context* cntx, const gchar* gateway,
	guchar* data, int datalen, const struct pktfwdpkt* rxpkt);
#endif
