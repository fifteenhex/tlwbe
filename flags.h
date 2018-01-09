#pragma once

#include <glib.h>

#include "tlwbe.h"

#define FLAG_UPLINK_RAW			"UPLINK_RAW"
#define FLAG_UPLINK_NOSTORE		"UPLINK_NOSTORE"
#define FLAG_UPLINK_NOREALTIME	"UPLINK_NOREALTIME"

struct flags_uplink {
	gboolean raw;
	gboolean nostore;
	gboolean norealtime;
};

struct flags {
	struct flags_uplink uplink;
};

void flags_forapp(struct context* cntx, const gchar* appeui,
		struct flags* flags);
