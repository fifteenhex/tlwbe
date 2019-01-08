#pragma once

#include <json-glib/json-glib.h>
#include <stdint.h>

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

static int __attribute__((unused)) utils_gbytearray_writer(uint8_t* data,
		size_t len, void* userdata) {
	GByteArray* bytearray = userdata;
	g_byte_array_append(bytearray, data, len);
	g_message("appended %d bytes to buffer", len);
	return 0;
}
