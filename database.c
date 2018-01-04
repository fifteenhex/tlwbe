#include "database.h"
#include "utils.h"

#define CREATETABLE_APPS	"CREATE TABLE IF NOT EXISTS apps ("\
								"eui	TEXT NOT NULL UNIQUE,"\
								"name	TEXT,"\
								"serial	INTEGER,"\
								"PRIMARY KEY(eui)"\
							");"

#define CREATETABLE_DEVS	"CREATE TABLE IF NOT EXISTS devs ("\
								"eui	TEXT NOT NULL UNIQUE,"\
								"appeui	TEXT NOT NULL,"\
								"key	TEXT NOT NULL,"\
								"name	TEXT,"\
								"serial	INTEGER NOT NULL,"\
								"PRIMARY KEY(eui)"\
							");"

#define CREATETABLE_DEVFLAGS	"CREATE TABLE IF NOT EXISTS devflags ("\
								"id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"\
								"deveui	TEXT NOT NULL,"\
								"flag	TEXT NOT NULL"\
							");"

#define CREATETABLE_SESSIONS	"CREATE TABLE IF NOT EXISTS sessions ("\
								"deveui			TEXT NOT NULL UNIQUE,"\
								"devnonce		TEXT NOT NULL,"\
								"appnonce		TEXT NOT NULL,"\
								"devaddr		TEXT NOT NULL UNIQUE,"\
								"upcounter		INTEGER DEFAULT 0,"\
								"downcounter	INTEGER DEFAULT 0,"\
								"PRIMARY KEY(deveui)"\
							");"

#define CREATETABLE_DOWNLINKS	"CREATE TABLE IF NOT EXISTS downlinks ("\
								"id			INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"\
								"timestamp	INTEGER NOT NULL,"\
								"deadline	INTEGER NOT NULL,"\
								"appeui		TEXT NOT NULL,"\
								"deveui		TEXT NOT NULL,"\
								"port		INTEGER NOT NULL,"\
								"payload	BLOB NOT NULL,"\
								"token		TEXT NOT NULL UNIQUE"\
");"

//sql for apps
#define INSERT_APP	"INSERT INTO apps (eui,name,serial) VALUES (?,?,?);"
#define GET_APP		"SELECT * FROM apps WHERE eui = ?;"
#define LIST_APPS	"SELECT eui FROM apps;"

//sql for devs
#define INSERT_DEV	"INSERT INTO devs (eui, appeui, key, name,serial) VALUES (?,?,?,?,?);"
#define GET_DEV		"SELECT * FROM devs WHERE eui = ?;"
#define LIST_DEVS	"SELECT eui FROM devs;"

//sql for sessions
#define INSERT_SESSION		"INSERT INTO sessions (deveui, devnonce, appnonce, devaddr) VALUES (?,?,?,?);"
#define GET_SESSIONDEVEUI	"SELECT * FROM sessions WHERE deveui = ?;"
#define GET_SESSIONDEVADDR	"SELECT * FROM sessions WHERE devaddr = ?;"
#define DELETE_SESSION		"DELETE FROM sessions WHERE deveui = ?;"
#define GET_KEYPARTS		"SELECT key,appnonce,devnonce,appeui,deveui "\
								"FROM sessions INNER JOIN devs on devs.eui = sessions.deveui WHERE devaddr = ?"
#define GET_FRAMECOUNTER_UP			"SELECT upcounter FROM sessions WHERE devaddr = ?"
#define GET_FRAMECOUNTER_DOWN		"SELECT downcounter FROM sessions WHERE devaddr = ?"
#define SET_FRAMECOUNTER_UP		"UPDATE sessions SET upcounter = ? WHERE devaddr = ?"
#define INC_FRAMECOUNTER_DOWN		"UPDATE sessions SET downcounter = downcounter +  1 WHERE devaddr = ?"

//sql for downlinks
#define INSERT_DOWNLINK	"INSERT INTO downlinks (timestamp, deadline, appeui, deveui, port, payload, token) VALUES (?,?,?,?,?,?,?);"
#define CLEAN_DOWNLINKS "DELETE FROM downlinks where ((? - timestamp)/1000000) > deadline"

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

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->dbcntx.db);
	if (ret)
		sqlite3_close(cntx->dbcntx.db);

	sqlite3_stmt *createappsstmt = NULL;
	sqlite3_stmt *createdevsstmt = NULL;
	sqlite3_stmt *createdevflagsstmt = NULL;
	sqlite3_stmt *createsessionsstmt = NULL;
	sqlite3_stmt *createdownlinksstmt = NULL;

	INITSTMT(CREATETABLE_APPS, createappsstmt);
	INITSTMT(CREATETABLE_DEVS, createdevsstmt);
	INITSTMT(CREATETABLE_DEVFLAGS, createdevflagsstmt);
	INITSTMT(CREATETABLE_SESSIONS, createsessionsstmt);
	INITSTMT(CREATETABLE_DOWNLINKS, createdownlinksstmt);

	database_stepuntilcomplete(createappsstmt, NULL, NULL);
	database_stepuntilcomplete(createdevsstmt, NULL, NULL);
	database_stepuntilcomplete(createdevflagsstmt, NULL, NULL);
	database_stepuntilcomplete(createsessionsstmt, NULL, NULL);
	database_stepuntilcomplete(createdownlinksstmt, NULL, NULL);

	INITSTMT(INSERT_APP, cntx->dbcntx.insertappstmt);
	INITSTMT(GET_APP, cntx->dbcntx.getappsstmt);
	INITSTMT(LIST_APPS, cntx->dbcntx.listappsstmt);

	INITSTMT(INSERT_DEV, cntx->dbcntx.insertdevstmt);
	INITSTMT(GET_DEV, cntx->dbcntx.getdevstmt);
	INITSTMT(LIST_DEVS, cntx->dbcntx.listdevsstmt);

	INITSTMT(INSERT_SESSION, cntx->dbcntx.insertsessionstmt);
	INITSTMT(GET_SESSIONDEVEUI, cntx->dbcntx.getsessionbydeveuistmt);
	INITSTMT(GET_SESSIONDEVADDR, cntx->dbcntx.getsessionbydevaddrstmt);
	INITSTMT(DELETE_SESSION, cntx->dbcntx.deletesessionstmt);

	INITSTMT(GET_KEYPARTS, cntx->dbcntx.getkeyparts);

	INITSTMT(GET_FRAMECOUNTER_UP, cntx->dbcntx.getframecounterup);
	INITSTMT(GET_FRAMECOUNTER_DOWN, cntx->dbcntx.getframecounterdown);
	INITSTMT(SET_FRAMECOUNTER_UP, cntx->dbcntx.setframecounterup);
	INITSTMT(INC_FRAMECOUNTER_DOWN, cntx->dbcntx.incframecounterdown);

	INITSTMT(INSERT_DOWNLINK, cntx->dbcntx.insertdownlink);
	INITSTMT(CLEAN_DOWNLINKS, cntx->dbcntx.cleandownlinks);

	goto noerr;

	out: {
		sqlite3_stmt* stmts[] = { cntx->dbcntx.insertappstmt,
				cntx->dbcntx.getappsstmt, cntx->dbcntx.listappsstmt,
				cntx->dbcntx.insertdevstmt, cntx->dbcntx.getdevstmt,
				cntx->dbcntx.listdevsstmt, cntx->dbcntx.insertsessionstmt,
				cntx->dbcntx.getsessionbydeveuistmt,
				cntx->dbcntx.getsessionbydevaddrstmt,
				cntx->dbcntx.deletesessionstmt, cntx->dbcntx.getkeyparts,
				cntx->dbcntx.getframecounterup,
				cntx->dbcntx.getframecounterdown,
				cntx->dbcntx.setframecounterup,
				cntx->dbcntx.incframecounterdown, cntx->dbcntx.insertdownlink,
				cntx->dbcntx.cleandownlinks };

		for (int i = 0; i < G_N_ELEMENTS(stmts); i++) {
			sqlite3_finalize(stmts[i]);
		}
	}

	noerr: {
		sqlite3_stmt* createstmts[] = { createappsstmt, createdevsstmt,
				createdevflagsstmt, createsessionsstmt, createdownlinksstmt };
		for (int i = 0; i < G_N_ELEMENTS(createstmts); i++) {
			if (createstmts[i] != NULL)
				sqlite3_finalize(createstmts[i]);
		}
	}

	return;
}

void database_app_add(struct context* cntx, const struct app* app) {
	sqlite3_stmt* stmt = cntx->dbcntx.insertappstmt;
	sqlite3_bind_text(stmt, 1, app->eui, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, app->name, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, app->serial);
	database_stepuntilcomplete(stmt, NULL, NULL);
	sqlite3_reset(stmt);
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

void database_dev_add(struct context* cntx, const struct dev* dev) {
	sqlite3_stmt* stmt = cntx->dbcntx.insertdevstmt;
	sqlite3_bind_text(stmt, 1, dev->eui, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, dev->appeui, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, dev->key, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, dev->name, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 5, dev->serial);
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

void database_session_add(struct context* cntx, const struct session* session) {
	sqlite3_stmt* stmt = cntx->dbcntx.insertsessionstmt;
	sqlite3_bind_text(stmt, 1, session->deveui, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, session->devnonce, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, session->appnonce, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 4, session->devaddr, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, NULL, NULL);
	sqlite3_reset(stmt);
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

void database_downlink_add(struct context* cntx, struct downlink* downlink) {
	sqlite3_bind_int64(cntx->dbcntx.insertdownlink, 1, downlink->timestamp);
	sqlite3_bind_int(cntx->dbcntx.insertdownlink, 2, downlink->deadline);
	sqlite3_bind_text(cntx->dbcntx.insertdownlink, 3, downlink->appeui, -1,
	SQLITE_STATIC);
	sqlite3_bind_text(cntx->dbcntx.insertdownlink, 4, downlink->deveui, -1,
	SQLITE_STATIC);
	sqlite3_bind_int(cntx->dbcntx.insertdownlink, 5, downlink->port);
	sqlite3_bind_blob(cntx->dbcntx.insertdownlink, 6, downlink->payload,
			downlink->payloadlen, NULL);
	sqlite3_bind_text(cntx->dbcntx.insertdownlink, 7, downlink->token, -1,
	SQLITE_STATIC);

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
