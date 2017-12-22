#pragma once

struct pair {
	void* first;
	void* second;
};

gchar* utils_createtopic(const gchar* id, ...);
gchar* utils_bin2hex(guint8* buff, gsize len);
