#include "database.h"

void database_init(struct context* cntx, const gchar* databasepath) {
	int ret = sqlite3_open(databasepath, &cntx->db);
	if (ret)
		sqlite3_close(cntx->db);

}

