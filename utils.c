#include <glib.h>

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

gchar* utils_bin2hex(guint8* buff, gsize len) {
	GString* gs = g_string_new(NULL);
	for (gsize i = 0; i < len; i++)
		g_string_append_printf(gs, "%02x", (unsigned) *buff++);
	return g_string_free(gs, FALSE);
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
