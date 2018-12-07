#include <sqlite3.h>

#include "database.h"
#include "utils.h"

#include "control.sqlite.h"
#include "join.sqlite.h"
#include "uplink.sqlite.h"
#include "downlink.sqlite.h"

//sql for apps
#define LIST_APPS		"SELECT eui FROM apps;"
#define LIST_APPFLAGS	"SELECT flag FROM appflags WHERE eui = ?;"

//sql for devs
#define LIST_DEVS		"SELECT eui FROM devs;"
#define LIST_DEVFLAGS	"SELECT flag FROM devflags WHERE eui = ?;"

//sql for sessions
#define GET_SESSIONDEVADDR	"SELECT * FROM sessions WHERE devaddr = ?;"
#define GET_KEYPARTS		"SELECT key,appnonce,devnonce,appeui,deveui "\
								"FROM sessions INNER JOIN devs on devs.eui = sessions.deveui WHERE devaddr = ?"
#define GET_FRAMECOUNTER_UP			"SELECT upcounter FROM sessions WHERE devaddr = ?"
#define GET_FRAMECOUNTER_DOWN		"SELECT downcounter FROM sessions WHERE devaddr = ?"
#define SET_FRAMECOUNTER_UP		"UPDATE sessions SET upcounter = ? WHERE devaddr = ?"
#define INC_FRAMECOUNTER_DOWN		"UPDATE sessions SET downcounter = downcounter +  1 WHERE devaddr = ?"

//sql for uplinks
#define CLEAN_UPLINKS	"DELETE FROM uplinks WHERE (timestamp < ?);"

//sql for downlinks
#define CLEAN_DOWNLINKS "DELETE FROM downlinks WHERE ((? - timestamp)/1000000) > deadline"
#define COUNT_DOWNLINKS	"SELECT count(*) FROM downlinks WHERE appeui = ? AND deveui = ?"

static int database_stepuntilcomplete(sqlite3_stmt* stmt,
		void (*rowcallback)(sqlite3_stmt* stmt, void* data), void* data) {
	int ret;
	while (1) {
		ret = sqlite3_step(stmt);
		if (ret == SQLITE_DONE)
			break;
		else if (ret == SQLITE_ROW) {
			if (rowcallback != NULL)
				rowcallback(stmt, data);
		} else if ((ret & SQLITE_ERROR) == SQLITE_ERROR) {
			g_message("sqlite error; %s", sqlite3_errstr(ret));
			break;
		}
	}
	return ret;
}

#define INITSTMT(SQL, STMT) if (sqlite3_prepare_v2(cntx->dbcntx.db, SQL,\
								-1, &STMT, NULL) != SQLITE_OK) {\
									g_message("failed to prepare sql; %s -> %s",\
										SQL, sqlite3_errmsg(cntx->dbcntx.db));\
									goto out;\
								}

#define LOOKUPBYSTRING(stmt, string, convertor)\
		const struct pair callbackanddata = { .first = callback, .second = data };\
			sqlite3_bind_text(stmt, 1, string, -1, SQLITE_STATIC);\
			database_stepuntilcomplete(stmt, convertor,\
					(void*) &callbackanddata);\
			sqlite3_reset(stmt)

static gboolean database_init_createtables(struct context* cntx) {
	sqlite3_stmt* createappsstmt = NULL;
	sqlite3_stmt* createappflagsstmt = NULL;
	sqlite3_stmt* createdevsstmt = NULL;
	sqlite3_stmt* createdevflagsstmt = NULL;
	sqlite3_stmt* createsessionsstmt = NULL;
	sqlite3_stmt* createuplinksstmt = NULL;
	sqlite3_stmt* createdownlinksstmt = NULL;

	INITSTMT(__SQLITEGEN_APPS_TABLE_CREATE, createappsstmt);
	INITSTMT(__SQLITEGEN_APPFLAGS_TABLE_CREATE, createappflagsstmt);
	INITSTMT(__SQLITEGEN_DEVS_TABLE_CREATE, createdevsstmt);
	INITSTMT(__SQLITEGEN_DEVFLAGS_TABLE_CREATE, createdevflagsstmt);
	INITSTMT(__SQLITEGEN_SESSIONS_TABLE_CREATE, createsessionsstmt);
	INITSTMT(__SQLITEGEN_UPLINKS_TABLE_CREATE, createuplinksstmt);
	INITSTMT(__SQLITEGEN_DOWNLINKS_TABLE_CREATE, createdownlinksstmt);

	sqlite3_stmt* createstmts[] = { createappsstmt, createappflagsstmt,
			createdevsstmt, createdevflagsstmt, createsessionsstmt,
			createuplinksstmt, createdownlinksstmt };

	for (int i = 0; i < G_N_ELEMENTS(createstmts); i++) {
		database_stepuntilcomplete(createstmts[i], NULL, NULL);
		sqlite3_finalize(createstmts[i]);
	}

	return TRUE;

	out: return FALSE;
}

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->dbcntx.db);
	if (ret)
		sqlite3_close(cntx->dbcntx.db);

	g_assert(database_init_createtables(cntx));

	INITSTMT(__SQLITEGEN_APPS_INSERT, cntx->dbcntx.insertappstmt);
	INITSTMT(__SQLITEGEN_APPS_GETBY_EUI, cntx->dbcntx.getappsstmt);
	INITSTMT(LIST_APPS, cntx->dbcntx.listappsstmt);
	INITSTMT(LIST_APPFLAGS, cntx->dbcntx.listappflagsstmt);

	INITSTMT(__SQLITEGEN_DEVS_INSERT, cntx->dbcntx.insertdevstmt);
	INITSTMT(__SQLITEGEN_DEVS_GETBY_EUI, cntx->dbcntx.getdevstmt);
	INITSTMT(LIST_DEVS, cntx->dbcntx.listdevsstmt);

	INITSTMT(__SQLITEGEN_SESSIONS_INSERT, cntx->dbcntx.insertsessionstmt);
	INITSTMT(__SQLITEGEN_SESSIONS_GETBY_DEVEUI,
			cntx->dbcntx.getsessionbydeveuistmt);
	INITSTMT(GET_SESSIONDEVADDR, cntx->dbcntx.getsessionbydevaddrstmt);
	INITSTMT(__SQLITEGEN_SESSIONS_DELETEBY_DEVEUI,
			cntx->dbcntx.deletesessionstmt);

	INITSTMT(GET_KEYPARTS, cntx->dbcntx.getkeyparts);

	INITSTMT(GET_FRAMECOUNTER_UP, cntx->dbcntx.getframecounterup);
	INITSTMT(GET_FRAMECOUNTER_DOWN, cntx->dbcntx.getframecounterdown);
	INITSTMT(SET_FRAMECOUNTER_UP, cntx->dbcntx.setframecounterup);
	INITSTMT(INC_FRAMECOUNTER_DOWN, cntx->dbcntx.incframecounterdown);

	INITSTMT(__SQLITEGEN_UPLINKS_INSERT, cntx->dbcntx.insertuplink);
	INITSTMT(__SQLITEGEN_UPLINKS_GETBY_DEVEUI, cntx->dbcntx.getuplinks_dev);
	INITSTMT(CLEAN_UPLINKS, cntx->dbcntx.cleanuplinks);

	INITSTMT(__SQLITEGEN_DOWNLINKS_INSERT, cntx->dbcntx.insertdownlink);
	INITSTMT(CLEAN_DOWNLINKS, cntx->dbcntx.cleandownlinks);
	INITSTMT(COUNT_DOWNLINKS, cntx->dbcntx.countdownlinks);

	goto noerr;

	out: {
		sqlite3_stmt* stmts[] = { cntx->dbcntx.insertappstmt,
				cntx->dbcntx.getappsstmt, cntx->dbcntx.listappsstmt,
				cntx->dbcntx.listappflagsstmt, cntx->dbcntx.insertdevstmt,
				cntx->dbcntx.getdevstmt, cntx->dbcntx.listdevsstmt,
				cntx->dbcntx.insertsessionstmt,
				cntx->dbcntx.getsessionbydeveuistmt,
				cntx->dbcntx.getsessionbydevaddrstmt,
				cntx->dbcntx.deletesessionstmt, cntx->dbcntx.getkeyparts,
				cntx->dbcntx.getframecounterup,
				cntx->dbcntx.getframecounterdown,
				cntx->dbcntx.setframecounterup,
				cntx->dbcntx.incframecounterdown, cntx->dbcntx.insertuplink,
				cntx->dbcntx.getuplinks_dev, cntx->dbcntx.cleanuplinks,
				cntx->dbcntx.insertdownlink, cntx->dbcntx.cleandownlinks,
				cntx->dbcntx.countdownlinks };

		for (int i = 0; i < G_N_ELEMENTS(stmts); i++) {
			sqlite3_finalize(stmts[i]);
		}
	}

	noerr: {

	}

	return;
}

void database_app_add(struct context* cntx, const struct app* app) {
	__sqlitegen_apps_add(cntx->dbcntx.insertappstmt, app);
	database_stepuntilcomplete(cntx->dbcntx.insertappstmt, NULL, NULL);
	sqlite3_reset(cntx->dbcntx.insertappstmt);
}

void database_app_update(struct context* cntx, const struct app* app) {

}

static void database_app_get_rowcallback(sqlite3_stmt* stmt, void* data) {
	struct pair* callbackanddata = data;

	const unsigned char* eui = sqlite3_column_text(stmt, 0);
	const unsigned char* name = sqlite3_column_text(stmt, 1);
	int serial = sqlite3_column_int(stmt, 2);

	const struct app a = { .eui = eui, .name = name, .serial = serial };

	((void (*)(const struct app*, void*)) callbackanddata->first)(&a,
			callbackanddata->second);
}

void database_app_get(struct context* cntx, const char* eui,
		void (*callback)(const struct app* app, void* data), void* data) {
	LOOKUPBYSTRING(cntx->dbcntx.getappsstmt, eui, database_app_get_rowcallback);
}

void database_app_del(struct context* cntx, const char* eui) {

}

static void database_apps_list_rowcallback(sqlite3_stmt* stmt, void* data) {
	const struct pair* callbackanddata = data;
	const unsigned char* eui = sqlite3_column_text(stmt, 0);
	((void (*)(const char*, void*)) callbackanddata->first)(eui,
			callbackanddata->second);
}

void database_apps_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->dbcntx.listappsstmt;
	database_stepuntilcomplete(stmt, database_apps_list_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

static void data_appflags_list_rowcallback(sqlite3_stmt* stmt, void* data) {

}

void database_appflags_list(struct context* cntx, const char* appeui,
		void (*callback)(const char* flag, void* data), void* data) {
	LOOKUPBYSTRING(cntx->dbcntx.listappflagsstmt, appeui,
			data_appflags_list_rowcallback);
}

void database_dev_add(struct context* cntx, const struct dev* dev) {
	sqlite3_stmt* stmt = cntx->dbcntx.insertdevstmt;
	__sqlitegen_devs_add(stmt, dev);
	database_stepuntilcomplete(stmt, NULL, NULL);
	sqlite3_reset(stmt);
}

void database_dev_update(struct context* cntx, const struct dev* dev) {

}

static void database_dev_get_rowcallback(sqlite3_stmt* stmt, void* data) {
	struct pair* callbackanddata = data;

	const unsigned char* eui = sqlite3_column_text(stmt, 0);
	const unsigned char* appeui = sqlite3_column_text(stmt, 1);
	const unsigned char* key = sqlite3_column_text(stmt, 2);
	const unsigned char* name = sqlite3_column_text(stmt, 3);
	int serial = sqlite3_column_int(stmt, 4);

	const struct dev d = { .eui = eui, .appeui = appeui, .key = key, .name =
			name, .serial = serial };

	((void (*)(const struct dev*, void*)) callbackanddata->first)(&d,
			callbackanddata->second);
}

void database_dev_get(struct context* cntx, const char* eui,
		void (*callback)(const struct dev* app, void* data), void* data) {
	LOOKUPBYSTRING(cntx->dbcntx.getdevstmt, eui, database_dev_get_rowcallback);
}

void database_dev_del(struct context* cntx, const char* eui) {

}

void database_devs_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->dbcntx.listdevsstmt;
	database_stepuntilcomplete(stmt, database_apps_list_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

gboolean database_session_add(struct context* cntx,
		const struct session* session) {
	sqlite3_stmt* stmt = cntx->dbcntx.insertsessionstmt;
	__sqlitegen_sessions_add(stmt, session);
	database_stepuntilcomplete(stmt, NULL, NULL);
	sqlite3_reset(stmt);
	return true;
}

static void database_session_get_rowcallback(sqlite3_stmt* stmt, void* data) {
	struct pair* callbackanddata = data;

	const char* deveui = sqlite3_column_text(stmt, 0);
	const char* devnonce = sqlite3_column_text(stmt, 1);
	const char* appnonce = sqlite3_column_text(stmt, 2);
	const char* devaddr = sqlite3_column_text(stmt, 3);

	const struct session s = { .deveui = deveui, .devnonce = devnonce,
			.appnonce = appnonce, .devaddr = devaddr };

	((void (*)(const struct session*, void*)) callbackanddata->first)(&s,
			callbackanddata->second);
}

void database_session_get_deveui(struct context* cntx, const char* deveui,
		void (*callback)(const struct session* session, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->dbcntx.getsessionbydeveuistmt;
	sqlite3_bind_text(stmt, 1, deveui, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_session_get_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_session_get_devaddr(struct context* cntx, const char* devaddr,
		void (*callback)(const struct session* session, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->dbcntx.getsessionbydevaddrstmt;
	sqlite3_bind_text(stmt, 1, devaddr, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_session_get_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_session_del(struct context* cntx, const char* deveui) {
	sqlite3_stmt* stmt = cntx->dbcntx.deletesessionstmt;
	sqlite3_bind_text(stmt, 1, deveui, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, NULL, NULL);
	sqlite3_reset(stmt);
}

static void database_keyparts_get_rowcallback(sqlite3_stmt* stmt, void* data) {
	struct pair* callbackanddata = data;

	const char* key = sqlite3_column_text(stmt, 0);
	const char* appnonce = sqlite3_column_text(stmt, 1);
	const char* devnonce = sqlite3_column_text(stmt, 2);
	const char* appeui = sqlite3_column_text(stmt, 3);
	const char* deveui = sqlite3_column_text(stmt, 4);

	const struct keyparts kp = { .key = key, .appnonce = appnonce, .devnonce =
			devnonce, .appeui = appeui, .deveui = deveui };

	((void (*)(const struct keyparts*, void*)) callbackanddata->first)(&kp,
			callbackanddata->second);
}

void database_keyparts_get(struct context* cntx, const char* devaddr,
		void (*callback)(const struct keyparts* keyparts, void* data),
		void* data) {
	LOOKUPBYSTRING(cntx->dbcntx.getkeyparts, devaddr,
			database_keyparts_get_rowcallback);
}

static void database_framecounter_get_rowcallback(sqlite3_stmt* stmt,
		void* data) {
	int* framecounter = data;
	*framecounter = sqlite3_column_int(stmt, 0);
}

int database_framecounter_down_getandinc(struct context* cntx,
		const char* devaddr) {
	int framecounter = -1;
	sqlite3_bind_text(cntx->dbcntx.getframecounterdown, 1, devaddr, -1,
	SQLITE_STATIC);
	database_stepuntilcomplete(cntx->dbcntx.getframecounterdown,
			database_framecounter_get_rowcallback, &framecounter);
	sqlite3_reset(cntx->dbcntx.getframecounterdown);
	sqlite3_bind_text(cntx->dbcntx.incframecounterdown, 1, devaddr, -1,
	SQLITE_STATIC);
	database_stepuntilcomplete(cntx->dbcntx.incframecounterdown,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.incframecounterdown);
	return framecounter;
}

void database_framecounter_up_set(struct context* cntx, const char* devaddr,
		int framecounter) {
	sqlite3_bind_int(cntx->dbcntx.setframecounterup, 1, framecounter);
	sqlite3_bind_text(cntx->dbcntx.setframecounterup, 2, devaddr, -1,
	SQLITE_STATIC);
	database_stepuntilcomplete(cntx->dbcntx.setframecounterup,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.setframecounterup);
}

void database_uplink_add(struct context* cntx, struct uplink* uplink) {
	__sqlitegen_uplinks_add(cntx->dbcntx.insertuplink, uplink);
	database_stepuntilcomplete(cntx->dbcntx.insertuplink,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.insertuplink);
}

static void database_uplinks_get_rowcallback(sqlite3_stmt* stmt, void* data) {
	struct pair* callbackanddata = data;

	guint64 timestamp = sqlite3_column_int64(stmt, 1);
	const char* appeui = sqlite3_column_text(stmt, 2);
	const char* deveui = sqlite3_column_text(stmt, 3);
	guint8 port = sqlite3_column_int(stmt, 4);
	const guint8* payload = sqlite3_column_blob(stmt, 5);
	gsize payloadlen = sqlite3_column_bytes(stmt, 5);

	struct uplink up = { .timestamp = timestamp, .appeui = appeui, .deveui =
			deveui, .port = port, .payload = payload, .payloadlen = payloadlen };

	((void (*)(const struct uplink*, void*)) callbackanddata->first)(&up,
			callbackanddata->second);
}

void database_uplinks_get(struct context* cntx, const char* deveui,
		void (*callback)(const struct uplink* uplink, void* data), void* data) {
	LOOKUPBYSTRING(cntx->dbcntx.getuplinks_dev, deveui,
			database_uplinks_get_rowcallback);
}

void database_uplinks_clean(struct context* cntx, guint64 timestamp) {
	sqlite3_bind_int64(cntx->dbcntx.cleanuplinks, 1, timestamp);
	database_stepuntilcomplete(cntx->dbcntx.cleanuplinks,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.cleanuplinks);
}

void database_downlink_add(struct context* cntx, struct downlink* downlink) {
	__sqlitegen_downlinks_add(cntx->dbcntx.insertdownlink, downlink);
	database_stepuntilcomplete(cntx->dbcntx.insertdownlink,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.insertdownlink);
}

void database_downlinks_clean(struct context* cntx, guint64 timestamp) {
	sqlite3_bind_int64(cntx->dbcntx.cleandownlinks, 1, timestamp);
	database_stepuntilcomplete(cntx->dbcntx.cleandownlinks,
	NULL, NULL);
	sqlite3_reset(cntx->dbcntx.cleandownlinks);
}

static void database_downlinks_count_rowcallback(sqlite3_stmt* stmt, void* data) {
	int* rows = data;
	*rows = sqlite3_column_int(stmt, 0);
}

int database_downlinks_count(struct context* cntx, const char* appeui,
		const char* deveui) {
	int rows = 0;
	sqlite3_bind_text(cntx->dbcntx.countdownlinks, 1, appeui, -1,
	SQLITE_STATIC);
	sqlite3_bind_text(cntx->dbcntx.countdownlinks, 1, deveui, -1,
	SQLITE_STATIC);
	database_stepuntilcomplete(cntx->dbcntx.countdownlinks,
			database_downlinks_count_rowcallback, &rows);
	sqlite3_reset(cntx->dbcntx.countdownlinks);
	return rows;
}
