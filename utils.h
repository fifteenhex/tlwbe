#pragma once

#include <json-glib/json-glib.h>

struct pair {
	void* first;
	void* second;
};

gchar* utils_createtopic(const gchar* id, ...);
gchar* utils_bin2hex(const void* buff, gsize len);
void utils_hex2bin(const gchar* string, void* buff, gsize buffsz);
guint16 utils_hex2u16(const gchar* string);
guint32 utils_hex2u24(const gchar* string);
guint32 utils_hex2u32(const gchar* string);
