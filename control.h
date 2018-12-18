#pragma once

#if  defined(__SQLITEGEN) || defined(__JSONGEN)
#include "codegen/fakeglib.h"
#else
#include "tlwbe.h"
#endif

struct flag {
#ifdef __SQLITEGEN
	guint64 id;
#endif
	const gchar* eui;
	const gchar* flag;
#ifdef __SQLITEGEN
	void __sqlitegen_flags_id_hidden;
	void __sqlitegen_constraints_id_notnull_primarykey_autoincrement_unique;
	void __sqlitegen_constraints_eui_notnull;
	void __sqlitegen_constraints_flag_notnull;
#endif
};

struct app {
	const gchar* eui;
	const gchar* name;
	guint32 serial;
#ifdef __SQLITEGEN
	void __sqlitegen_flags_eui_searchable;
	void __sqlitegen_constraints_eui_notnull_primarykey_unique;
#endif
};
#ifdef __SQLITEGEN
typedef struct app __sqlitegen_table_apps;
typedef struct flag __sqlitegen_table_appflags;
#endif
#ifdef __JSONGEN
typedef struct app __jsongen_builder;
#endif

struct dev {
	const gchar* eui;
	const gchar* appeui;
	const gchar* key;
	const gchar* name;
	guint32 serial;
#ifdef __SQLITEGEN
	void __sqlitegen_flags_eui_searchable;
	void __sqlitegen_constraints_eui_notnull_primarykey_unique;
#endif
};
#ifdef __SQLITEGEN
typedef struct dev __sqlitegen_table_devs;
typedef struct flag __sqlitegen_table_devflags;
#endif
#ifdef __JSONGEN
typedef struct dev __jsongen_builder;
#endif


#if !(defined(__SQLITEGEN) || defined(__JSONGEN))
#define CONTROL_SUBTOPIC		"control"
#define CONTROL_ENTITY_APP		"app"
#define CONTROL_ENTITY_DEV		"dev"
#define CONTROL_ACTION_ADD		"add"
#define CONTROL_ACTION_UPDATE	"update"
#define CONTROL_ACTION_DEL		"del"
#define CONTROL_ACTION_GET		"get"
#define CONTROL_ACTION_LIST		"list"
#define CONTROL_RESULT			"result"

#define CONTROL_JSON_NAME		"name"
#define CONTROL_JSON_EUI		"eui"
#define CONTROL_JSON_APPEUI		"appeui"
#define CONTROL_JSON_KEY		"key"
#define CONTROL_JSON_SERIAL		"serial"

void control_onbrokerconnect(struct context* cntx);
void control_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
#endif
