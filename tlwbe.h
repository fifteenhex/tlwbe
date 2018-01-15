#pragma once

#include <glib.h>
#include "mosquittomainloop/mosquittomainloop.h"
#include "database_context.h"

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	struct mosquitto_context mosqcntx;
	struct database_context dbcntx;
};
