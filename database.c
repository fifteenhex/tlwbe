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

#define INSERT_APP	"INSERT INTO apps (eui,name,serial) VALUES (?,?,?);"
#define GET_APP		"SELECT * FROM apps WHERE eui = ?;"
#define LIST_APPS	"SELECT eui FROM apps;"
#define INSERT_DEV	"INSERT INTO devs (eui, appeui, key, name,serial) VALUES (?,?,?,?,?);"
#define GET_DEV		"SELECT * FROM devs WHERE eui = ?;"
#define LIST_DEVS	"SELECT eui FROM devs;"

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

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->db);
	if (ret)
		sqlite3_close(cntx->db);

	sqlite3_stmt *createappsstmt = NULL;
	sqlite3_stmt *createdevsstmt = NULL;
	cntx->insertappstmt = NULL;
	cntx->getappsstmt = NULL;
	cntx->listappsstmt = NULL;
	cntx->insertdevstmt = NULL;

	if (sqlite3_prepare_v2(cntx->db, CREATETABLE_APPS, -1, &createappsstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to create apps table; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, CREATETABLE_DEVS, -1, &createdevsstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to create devs table; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	database_stepuntilcomplete(createappsstmt, NULL, NULL);
	database_stepuntilcomplete(createdevsstmt, NULL, NULL);

	if (sqlite3_prepare_v2(cntx->db, INSERT_APP, -1, &cntx->insertappstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to insert app; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, GET_APP, -1, &cntx->getappsstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to get app; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, LIST_APPS, -1, &cntx->listappsstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to list apps; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, INSERT_DEV, -1, &cntx->insertdevstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to insert dev; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, GET_DEV, -1, &cntx->getdevstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to get dev; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	if (sqlite3_prepare_v2(cntx->db, LIST_DEVS, -1, &cntx->listdevsstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to lise devs; %s",
				sqlite3_errmsg(cntx->db));
		goto out;
	}

	goto noerr;

	out: if (cntx->insertappstmt != NULL)
		sqlite3_finalize(cntx->insertappstmt);
	if (cntx->getappsstmt != NULL)
		sqlite3_finalize(cntx->getappsstmt);
	if (cntx->listappsstmt != NULL)
		sqlite3_finalize(cntx->listappsstmt);
	if (cntx->insertdevstmt != NULL)
		sqlite3_finalize(cntx->insertdevstmt);
	if (cntx->getdevstmt != NULL)
		sqlite3_finalize(cntx->getdevstmt);
	if (cntx->listdevsstmt != NULL)
		sqlite3_finalize(cntx->listdevsstmt);
	noerr: if (createappsstmt != NULL)
		sqlite3_finalize(createappsstmt);
	if (createdevsstmt != NULL)
		sqlite3_finalize(createdevsstmt);

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
	struct app a;
	((void (*)(struct app*, void*)) callbackanddata->first)(&a,
			callbackanddata->second);
}

void database_app_get(struct context* cntx, const char* eui,
		void (*callback)(struct app* app, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->getappsstmt;
	sqlite3_bind_text(stmt, 1, eui, 1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_app_get_rowcallback,
			&callbackanddata);
	sqlite3_reset(stmt);
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
			&callbackanddata);
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
	struct dev d;
	((void (*)(struct dev*, void*)) callbackanddata->first)(&d,
			callbackanddata->second);
}

void database_dev_get(struct context* cntx, const char* eui,
		void (*callback)(struct dev* app, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->getdevstmt;
	sqlite3_bind_text(stmt, 1, eui, 1, SQLITE_STATIC);
	database_stepuntilcomplete(stmt, database_dev_get_rowcallback,
			&callbackanddata);
	sqlite3_reset(stmt);
}

void database_dev_del(struct context* cntx, const char* eui) {

}

void database_devs_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data) {
	const struct pair callbackanddata = { .first = callback, .second = data };
	sqlite3_stmt* stmt = cntx->listdevsstmt;
	database_stepuntilcomplete(stmt, database_apps_list_rowcallback,
			&callbackanddata);
	sqlite3_reset(stmt);
}
