#pragma once

#include <glib.h>
#include <mosquitto_client.h>

#include "database_context.h"
#include "regional_temp.h"
#include "stats.h"

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	MosquittoClient* mosqclient;
	struct database_context dbcntx;
	struct regional regional;
	struct stats stats;
};
