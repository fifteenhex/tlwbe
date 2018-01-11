#include <string.h>

#include "flags.h"
#include "database.h"

static void flags_forapp_rowcallback(const char* flag, void* data) {
	struct flags* flags = data;
	if (strcmp(flag, FLAG_UPLINK_RAW) == 0) {
		flags->uplink.raw = TRUE;
	} else if (strcmp(flag, FLAG_UPLINK_NOSTORE) == 0) {
		flags->uplink.nostore = TRUE;
	} else if (strcmp(flag, FLAG_UPLINK_NOREALTIME) == 0) {
		flags->uplink.norealtime = TRUE;
	}
}

void flags_forapp(struct context* cntx, const gchar* appeui,
		struct flags* flags) {
	memset(flags, 0, sizeof(*flags));
	database_appflags_list(cntx, appeui, flags_forapp_rowcallback, flags);
}
