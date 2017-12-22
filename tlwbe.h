#pragma once

#include <glib.h>
#include <sqlite3.h>

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	struct mosquitto* mosq;
	const gchar* mqtthost;
	gint mqttport;
	GIOChannel* mosqchan;
	guint mosqsource;
	//sqlite stuff
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
	sqlite3_stmt* getsessionstmt;
	sqlite3_stmt* deletesessionstmt;
};
