#pragma once

#include <glib.h>
#include <mosquittomainloop.h>
#include "database_context.h"

#define TLWBE_TOPICROOT "tlwbe"

struct context {
	MosquittoClient* mosqclient;
	struct database_context dbcntx;
};
