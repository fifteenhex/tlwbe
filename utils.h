#pragma once

#include <json-glib/json-glib.h>

struct pair {
	void* first;
	void* second;
};

gchar* utils_createtopic(const gchar* id, ...);
gchar* utils_bin2hex(const void* buff, gsize len);
void utils_hex2bin(const gchar* string, void* buff, gsize buffsz);
gchar* utils_jsonbuildertostring(JsonBuilder* jsonbuilder, gsize* jsonlen);
