#include <glib.h>
#include <string.h>

#include "utils.h"
#include "pktfwdbr.h"

gchar* utils_createtopic(const gchar* id, ...) {
	GString* topicstr = g_string_new(PKTFWDBR_TOPIC_ROOT"/");
	g_string_append(topicstr, id);

	va_list args;
	va_start(args, id);

	const gchar * part = va_arg(args, const gchar*);
	for (; part != NULL; part = va_arg(args, const gchar*)) {
		g_string_append(topicstr, "/");
		g_string_append(topicstr, part);
	}

	va_end(args);
	gchar* topic = g_string_free(topicstr, FALSE);
	return topic;
}

gchar* utils_bin2hex(const void* buff, gsize len) {
	const guint8* byte = buff;
	GString* gs = g_string_new(NULL);
	for (gsize i = 0; i < len; i++)
		g_string_append_printf(gs, "%02x", (unsigned) *byte++);
	return g_string_free(gs, FALSE);
}

void utils_hex2bin(const gchar* string, void* buff, gsize buffsz) {
	guint8* byte = buff;
	if (strlen(string) % 2 != 0) {
		g_message("hex string length should be a multiple of 2");
		return;
	}

	gchar slice[3];
	slice[2] = '\0';

	for (int i = 0; i < buffsz; i++) {
		guint64 b;
		memcpy(slice, string, 2);
		g_ascii_string_to_unsigned(slice, 16, 0, 0xff, &b, NULL);
		*byte++ = b;
		string += 2;
		if (*string == '\0')
			break;
	}
}

guint16 utils_hex2u16(const gchar* string) {
	guint16 result = 0;
	utils_hex2bin(string, &result, sizeof(result));
	result = GUINT16_FROM_BE(result);
	return result;
}

guint32 utils_hex2u24(const gchar* string) {
	guint32 result = 0;
	utils_hex2bin(string, ((guint8*) &result) + 1, 3);
	result = GUINT32_FROM_BE(result);
	return result;
}

guint32 utils_hex2u32(const gchar* string) {
	guint32 result = 0;
	utils_hex2bin(string, &result, sizeof(result));
	result = GUINT32_FROM_BE(result);
	return result;
}
