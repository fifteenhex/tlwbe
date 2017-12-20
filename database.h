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
	const gchar* key;
	const gchar* name;
	const guint32 serial;
};

void database_init(struct context* cntx, const gchar* databasepath);

void database_app_add(struct context* cntx, const struct app* app);
void database_app_update(struct context* cntx, const struct app* app);
void database_app_get(struct context* cntx, const char* eui,
		void (*callback)(struct app* app, void* data), void* data);
void database_app_del(struct context* cntx, const char* eui);
void database_apps_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data);
void database_dev_add(struct context* cntx, const struct dev* dev);
void database_dev_update(struct context* cntx, const struct dev* dev);
void database_dev_get(struct context* cntx, const char* eui,
		void (*callback)(struct dev* app, void* data), void* data);
void database_dev_del(struct context* cntx, const char* eui);
void database_devs_list(struct context* cntx,
		void (*callback)(const char* eui, void* data), void* data);
