#include <sqlite3.h>

#include "database.h"

#define CREATETABLE_APPS	"CREATE TABLE IF NOT EXISTS `apps` ("\
								"`eui`	TEXT NOT NULL UNIQUE,"\
								"`name`	TEXT,"\
								"`serial`	INTEGER,"\
								"PRIMARY KEY(`eui`)"\
							");"

#define CREATETABLE_DEVS	"CREATE TABLE IF NOT EXISTS `devs` ("\
								"`eui` TEXT NOT NULL UNIQUE,"\
								"`appeui` TEXT NOT NULL,"\
								"`name` TEXT,"\
								"`serial` INTEGER NOT NULL,"\
								"PRIMARY KEY(`eui`)"\
							");"

#define INSERT_APP "INSERT INTO `apps` (`eui`,`name`,`serial`) VALUES (?,?,?);"

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->db);
	if (ret)
		sqlite3_close(cntx->db);

	sqlite3_stmt *createappsstmt = NULL;
	sqlite3_stmt *createdevsstmt = NULL;
	cntx->insertappstmt = NULL;

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

	if (sqlite3_prepare_v2(cntx->db, INSERT_APP, -1, &cntx->insertappstmt,
	NULL) != SQLITE_OK) {
		g_message("failed to prepare sql to insert app; %s",
				sqlite3_errmsg(cntx->db));
	}

	while (sqlite3_step(createappsstmt) != SQLITE_DONE) {

	}

	while (sqlite3_step(createdevsstmt) != SQLITE_DONE) {

	}

	goto noerr;

	out: if (cntx->insertappstmt != NULL)
		sqlite3_finalize(cntx->insertappstmt);
	noerr: if (createappsstmt != NULL)
		sqlite3_finalize(createappsstmt);
	if (createdevsstmt != NULL)
		sqlite3_finalize(createdevsstmt);

	return;
}

static int database_stepuntilcomplete(sqlite3_stmt* stmt) {
	int ret;
	while (1) {
		ret = sqlite3_step(stmt);
		if (ret == SQLITE_DONE)
			break;
		if ((ret & SQLITE_ERROR) == SQLITE_ERROR) {
			g_message("sqlite error; %s", sqlite3_errstr(ret));
			break;
		}
	}
	return ret;
}

void database_app_add(struct context* cntx, const struct app* app) {
	sqlite3_stmt* stmt = cntx->insertappstmt;
	sqlite3_bind_text(stmt, 1, app->eui, -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, app->name, -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, app->serial);
	database_stepuntilcomplete(stmt);
	sqlite3_reset(stmt);
}

void database_dev_add() {

}
