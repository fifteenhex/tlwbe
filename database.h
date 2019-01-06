#pragma once

#include "tlwbe.h"

#include "control.h"
#include "join.h"
#include "uplink.h"
#include "downlink.h"

struct keyparts {
	const gchar* key;
	const gchar* appnonce;
	const gchar* devnonce;
	// not really needed for the keys but useful
	const gchar* appeui;
	const gchar* deveui;
};

gboolean database_init(struct context* cntx, const gchar* databasepath);

// app
void database_app_add(context_readonly* cntx, const struct app* app);
void database_app_update(context_readonly* cntx, const struct app* app);
void database_app_get(context_readonly* cntx, const char* eui, const char* name,
		void (*callback)(const struct app* app, void* data), void* data);
void database_app_del(context_readonly* cntx, const char* eui);
void database_apps_list(context_readonly* cntx,
		void (*callback)(const char* eui, void* data), void* data);
void database_appflags_list(context_readonly* cntx, const char* appeui,
		void (*callback)(const char* flag, void* data), void* data);
// dev
void database_dev_add(context_readonly* cntx, const struct dev* dev);
void database_dev_update(context_readonly* cntx, const struct dev* dev);
void database_dev_get(context_readonly* cntx, const char* eui, const char* name,
		void (*callback)(const struct dev* dev, void* data), void* data);
void database_dev_del(context_readonly* cntx, const char* eui);
void database_devs_list(context_readonly* cntx,
		void (*callback)(const char* eui, void* data), void* data);
//session
gboolean database_session_add(context_readonly* cntx, const struct session* dev);
void database_session_get_deveui(context_readonly* cntx, const char* deveui,
		void (*callback)(const struct session* session, void* data), void* data);
void database_session_get_devaddr(context_readonly* cntx, const char* devaddr,
		void (*callback)(const struct session* session, void* data), void* data);
void database_session_del(context_readonly* cntx, const char* deveui);
int database_framecounter_down_getandinc(context_readonly* cntx,
		const char* devaddr);
void database_framecounter_up_set(context_readonly* cntx, const char* devaddr,
		int framecounter);
// dev + session
void database_keyparts_get(context_readonly*, const char* devaddr,
		void (*callback)(const struct keyparts* keyparts, void* data),
		void* data);
//uplinks
void database_uplink_add(context_readonly* cntx, struct uplink* uplink);
void database_uplinks_get(context_readonly* cntx, const char* deveui,
		void (*callback)(const struct uplink* uplink, void* data), void* data);
void database_uplinks_clean(context_readonly* cntx, guint64 timestamp);

//downlinks
void database_downlink_add(context_readonly* cntx, struct downlink* downlink);
void database_downlinks_clean(context_readonly* cntx, guint64 timestamp);
int database_downlinks_count(context_readonly* cntx, const char* appeui,
		const char* deveui);
gboolean database_downlinks_get_first(context_readonly* cntx,
		const char* appeui, const char* deveui, struct downlink* downlink);
gboolean database_downlinks_delete_by_token(context_readonly* cntx,
		const char* token);
