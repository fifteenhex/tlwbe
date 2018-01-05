#pragma once

#include <sqlite3.h>

struct database_context {
	sqlite3* db;
	// statements for apps
	sqlite3_stmt* insertappstmt;
	sqlite3_stmt* getappsstmt;
	sqlite3_stmt* listappsstmt;
	// statements for devs
	sqlite3_stmt* insertdevstmt;
	sqlite3_stmt* getdevstmt;
	sqlite3_stmt* listdevsstmt;
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
	// statements for downlinks
	sqlite3_stmt* insertdownlink;
	sqlite3_stmt* cleandownlinks;
	sqlite3_stmt* countdownlinks;
};
