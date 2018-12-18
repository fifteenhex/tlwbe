#include <json-glib/json-glib.h>

#include "json-glib-macros/jsonparserutils.h"

#include "regional.h"

gboolean regional_init(struct regional* regional, const gchar* region) {
	if (region == NULL) {
		g_message("region unspecified, defaulting to \"default\"");
		region = "default";
	}

	JsonParser* parser = json_parser_new_immutable();
	if (json_parser_load_from_file(parser, "../regionalparameters.json",
	NULL)) {
		JsonNode* root = json_parser_get_root(parser);
		JsonObject* rootobject = JSON_NODE_GET_OBJECT(root);
		if (rootobject != NULL) {
			JsonObject* parameters = JSON_OBJECT_GET_MEMBER_OBJECT(rootobject,
					region);
			if (parameters != NULL) {
				JsonArray* cflist = JSON_OBJECT_GET_MEMBER_ARRAY(parameters,
						"cflist");
				int cflisttype = JSON_OBJECT_GET_MEMBER_INT(parameters,
						"cflist_type");

				if (cflist != NULL && cflisttype == 0) {
					g_message("processing cflist...");
					for (int i = 0; i < json_array_get_length(cflist) && i < 5;
							i++) {
						guint32 freq = json_array_get_int_element(cflist, i);
						guint32 cflistfreq = freq / 100;
						g_message("adding frequency %d", freq);
						regional->extrachannels[i] = cflistfreq;
					}
					regional->sendcflist = TRUE;
				}
			} else
				g_message("didn't find parameters for region %s", region);
		} else
			g_message("regional parameters root should have been an object");

	} else
		g_message("failed to parse regional parameters json");
	g_object_unref(parser);
	return TRUE;
}

guint64 regional_getwindowdelay(enum RXWINDOW rxwindow) {
	switch (rxwindow) {
	case RXW_R1:
		return 1000000;
	case RXW_R2:
		return 2000000;
	case RXW_J1:
		return 5000000;
	case RXW_J2:
		return 6000000;
	default:
		return 0;
	}
}

gdouble regional_getfrequency(enum RXWINDOW rxwindow,
		const struct pktfwdpkt* rxpkt) {
	switch (rxwindow) {
	case RXW_R2:
	case RXW_J2:
		return 923.2;
	default:
		return rxpkt->rfparams.frequency;
	}

}
