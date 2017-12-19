#pragma once

#include "tlwbe.h"

struct app {
	const gchar* eui;
	const gchar* name;
	const guint32 serial;
};

struct dev {
	const gchar* eui;
	const gchar* appeui;
	const gchar* name;
	const guint32 serial;
};

void database_init(struct context* cntx, const gchar* databasepath);

void database_app_add(struct context* cntx, const struct app* app);
