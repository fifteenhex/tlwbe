#pragma once

#include <glib.h>
#include "database_context.h"

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	struct mosquitto* mosq;
	const gchar* mqtthost;
	gint mqttport;
	GIOChannel* mosqchan;
	guint mosqsource;
	struct database_context dbcntx;
};
