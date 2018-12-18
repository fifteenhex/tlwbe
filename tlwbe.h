#pragma once

#include <glib.h>
#include <mosquittomainloop.h>

#include "database_context.h"
#include "regional_temp.h"

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	MosquittoClient* mosqclient;
	struct database_context dbcntx;
	struct regional regional;
};
