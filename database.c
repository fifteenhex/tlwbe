#include <sqlite3.h>

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
								"deveui	TEXT NOT NULL,"\
								"flag	TEXT NOT NULL,"\
								"PRIMARY KEY(deveui)"\
							");"

#define CREATETABLE_SESSIONS	"CREATE TABLE IF NOT EXISTS sessions ("\
								"deveui		TEXT NOT NULL UNIQUE,"\
								"devnonce	TEXT NOT NULL,"\
								"appnonce	TEXT NOT NULL,"\
								"devaddr	TEXT NOT NULL UNIQUE,"\
								"PRIMARY KEY(deveui)"\
							");"

#define INSERT_APP	"INSERT INTO apps (eui,name,serial) VALUES (?,?,?);"
#define GET_APP		"SELECT * FROM apps WHERE eui = ?;"
#define LIST_APPS	"SELECT eui FROM apps;"

#define INSERT_DEV	"INSERT INTO devs (eui, appeui, key, name,serial) VALUES (?,?,?,?,?);"
#define GET_DEV		"SELECT * FROM devs WHERE eui = ?;"
#define LIST_DEVS	"SELECT eui FROM devs;"

#define INSERT_SESSION		"INSERT INTO sessions (deveui, devnonce, appnonce, devaddr) VALUES (?,?,?,?);"
#define GET_SESSIONDEVEUI	"SELECT * FROM sessions WHERE deveui = ?;"
#define GET_SESSIONDEVADDR	"SELECT * FROM sessions WHERE devaddr = ?;"
#define DELETE_SESSION		"DELETE FROM sessions WHERE deveui = ?;"
#define GET_KEYPARTS		"SELECT key,appnonce,devnonce,appeui,deveui "\
								"FROM sessions INNER JOIN devs on devs.eui = sessions.deveui WHERE devaddr = ?"

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

#define INITSTMT(SQL, STMT) STMT = NULL;\
							if (sqlite3_prepare_v2(cntx->db, SQL,\
								-1, &STMT, NULL) != SQLITE_OK) {\
									g_message("failed to prepare sql; %s -> %s",\
										sqlite3_errmsg(cntx->db));\
									goto out;\
								}

#define LOOKUPBYSTRING(stmt, string, convertor)\
		const struct pair callbackanddata = { .first = callback, .second = data };\
			sqlite3_bind_text(stmt, 1, string, -1, SQLITE_STATIC);\
			database_stepuntilcomplete(stmt, convertor,\
					(void*) &callbackanddata);\
			sqlite3_reset(stmt)

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->db);
	if (ret)
		sqlite3_close(cntx->db);

	sqlite3_stmt *createappsstmt;
	sqlite3_stmt *createdevsstmt;
	sqlite3_stmt *createdevflagsstmt;
	sqlite3_stmt *createsessionsstmt;

	INITSTMT(CREATETABLE_APPS, createappsstmt);
	INITSTMT(CREATETABLE_DEVS, createdevsstmt);
	INITSTMT(CREATETABLE_DEVFLAGS, createdevflagsstmt);
	INITSTMT(CREATETABLE_SESSIONS, createsessionsstmt);

	database_stepuntilcomplete(createappsstmt, NULL, NULL);
	database_stepuntilcomplete(createdevsstmt, NULL, NULL);
	database_stepuntilcomplete(createdevflagsstmt, NULL, NULL);
	database_stepuntilcomplete(createsessionsstmt, NULL, NULL);

	INITSTMT(INSERT_APP, cntx->insertappstmt);
	INITSTMT(GET_APP, cntx->getappsstmt);
	INITSTMT(LIST_APPS, cntx->listappsstmt);

	INITSTMT(INSERT_DEV, cntx->insertdevstmt);
	INITSTMT(GET_DEV, cntx->getdevstmt);
	INITSTMT(LIST_DEVS, cntx->listdevsstmt);

	INITSTMT(INSERT_SESSION, cntx->insertsessionstmt);
	INITSTMT(GET_SESSIONDEVEUI, cntx->getsessionbydeveuistmt);
	INITSTMT(GET_SESSIONDEVADDR, cntx->getsessionbydevaddrstmt);
	INITSTMT(DELETE_SESSION, cntx->deletesessionstmt);

	INITSTMT(GET_KEYPARTS, cntx->getkeyparts);

	goto noerr;

	out: {
		sqlite3_stmt* stmts[] = { cntx->insertappstmt, cntx->getappsstmt,
				cntx->listappsstmt, cntx->insertdevstmt, cntx->getdevstmt,
				cntx->listdevsstmt, cntx->insertsessionstmt,
				cntx->getsessionbydeveuistmt, cntx->getsessionbydevaddrstmt,
				cntx->deletesessionstmt, cntx->getkeyparts };

		for (int i = 0; i < G_N_ELEMENTS(stmts); i++) {
			sqlite3_finalize(stmts[i]);
		}
	}

	noerr: {
		sqlite3_stmt* createstmts[] = { createappsstmt, createdevsstmt,
				createdevflagsstmt, createsessionsstmt };
		for (int i = 0; i < G_N_ELEMENTS(createstmts); i++) {
			if (createstmts[i] != NULL)
				sqlite3_finalize(createstmts[i]);
		}
	}

	return;
}

void database_app_add(struct context* cntx, const struct app* app) {
	sqlite3_stmt* stmt = cntx->insertappstmt;
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
	LOOKUPBYSTRING(cntx->getappsstmt, eui, database_app_get_rowcallback);
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
	sqlite3_stmt* stmt = cntx->listappsstmt;
	database_stepuntilcomplete(stmt, database_apps_list_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_dev_add(struct context* cntx, const struct dev* dev) {
	sqlite3_stmt* stmt = cntx->insertdevstmt;
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
	LOOKUPBYSTRING(cntx->getdevstmt, eui, database_dev_get_rowcallback);
}

void database_dev_del(struct context* cntx, const char* eui) {

}

void database_devs_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->listdevsstmt;
	database_stepuntilcomplete(stmt, database_apps_list_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_session_add(struct context* cntx, const struct session* session) {
	sqlite3_stmt* stmt = cntx->insertsessionstmt;
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
	sqlite3_stmt* stmt = cntx->getsessionbydeveuistmt;
	sqlite3_bind_text(stmt, 1, deveui, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_session_get_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_session_get_devaddr(struct context* cntx, const char* devaddr,
		void (*callback)(const struct session* session, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->getsessionbydevaddrstmt;
	sqlite3_bind_text(stmt, 1, devaddr, -1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_session_get_rowcallback,
			(void*) &callbackanddata);
	sqlite3_reset(stmt);
}

void database_session_del(struct context* cntx, const char* deveui) {
	sqlite3_stmt* stmt = cntx->deletesessionstmt;
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
	LOOKUPBYSTRING(cntx->getkeyparts, devaddr,
			database_keyparts_get_rowcallback);
}
