#pragma once

#include <sqlite3.h>

struct database_context {
	sqlite3* db;
	// statements for apps
	sqlite3_stmt* insertappstmt;
	sqlite3_stmt* app_get_by_eui;
	sqlite3_stmt* app_get_by_name;
	sqlite3_stmt* listappsstmt;
	sqlite3_stmt* listappflagsstmt;
	sqlite3_stmt* app_delete_by_eui;
	// statements for devs
	sqlite3_stmt* insertdevstmt;
	sqlite3_stmt* dev_get_by_eui;
	sqlite3_stmt* dev_get_by_name;
	sqlite3_stmt* listdevsstmt;
	sqlite3_stmt* dev_delete_by_eui;
	// statements for sessions
	sqlite3_stmt* insertsessionstmt;
	sqlite3_stmt* getsessionbydeveuistmt;
	sqlite3_stmt* getsessionbydevaddrstmt;
	sqlite3_stmt* deletesessionstmt;
	sqlite3_stmt* getkeyparts;
	sqlite3_stmt* getframecounterup;
	sqlite3_stmt* getframecounterdown;
	sqlite3_stmt* incframecounterdown;
	sqlite3_stmt* setframecounterup;
	//statements for uplinks
	sqlite3_stmt* insertuplink;
	sqlite3_stmt* cleanuplinks;
	sqlite3_stmt* getuplinks_dev;
	// statements for downlinks
	sqlite3_stmt* insertdownlink;
	sqlite3_stmt* cleandownlinks;
	sqlite3_stmt* countdownlinks;
	sqlite3_stmt* downlinks_get_first;
	sqlite3_stmt* downlinks_delete_by_token;
};
