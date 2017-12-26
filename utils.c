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

gchar* utils_jsonbuildertostring(JsonBuilder* jsonbuilder, gsize* jsonlen) {
	JsonNode* responseroot = json_builder_get_root(jsonbuilder);
	JsonGenerator* generator = json_generator_new();
	json_generator_set_root(generator, responseroot);

	gchar* json = json_generator_to_data(generator, jsonlen);
	json_node_free(responseroot);
	g_object_unref(generator);

	return json;
}
