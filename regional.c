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
				JsonArray* datarates = JSON_OBJECT_GET_MEMBER_ARRAY(parameters,
						"datarates");
				if (datarates != NULL) {
					int numdrs = json_array_get_length(datarates);
					int maxdrs = G_N_ELEMENTS(regional->datarates);
					if (numdrs > maxdrs)
						g_message("have %d datarates, only using the first %d",
								numdrs, maxdrs);
					for (int i = 0; i < numdrs; i++) {
						const gchar* dr = json_array_get_string_element(
								datarates, i);
						g_message("adding datarate %s", dr);
						regional->datarates[i] = g_strdup(dr);
					}
				} else
					g_message("didn't find data rates");

				JsonArray* extrachannels = JSON_OBJECT_GET_MEMBER_ARRAY(
						parameters, "extra_channels");
				if (extrachannels != NULL) {
					g_message("processing extra channels...");
					int numchans = json_array_get_length(extrachannels);
					int maxchans = G_N_ELEMENTS(regional->extrachannels);
					if (numchans < maxchans)
						g_message(
								"have %d extra channels, only using the first %d",
								numchans, maxchans);
					for (int i = 0; i < numchans && i < maxchans; i++) {
						guint32 freq = json_array_get_int_element(extrachannels,
								i);
						guint32 cflistfreq = freq / 100;
						g_message("adding frequency %d", freq);
						regional->extrachannels[i] = cflistfreq;
					}
					regional->sendcflist = TRUE;
				} else
					g_message("didn't find any extra channels");

				JsonObject* rx2 = JSON_OBJECT_GET_MEMBER_OBJECT(parameters,
						"rx2");
				if (rx2 != NULL) {
					g_message("processing rx2 window parameters...");
					regional->rx2.frequency = json_object_get_int_member(rx2,
							"frequency");
					regional->rx2.datarate = json_object_get_int_member(rx2,
							"datarate");
					g_message(
							"rx2 window will use frequency %d with data rate %s",
							regional->rx2.frequency,
							regional->datarates[regional->rx2.datarate]);
				} else
					g_message("didn't find rx2 window parameters");

			} else
				g_message("didn't find parameters for region %s", region);
		} else
			g_message("regional parameters root should have been an object");

	} else
		g_message("failed to parse regional parameters json");
	g_object_unref(parser);
	return TRUE;
}

/* in the future these might need to come from
 * the region config but at the moment these seem to be
 * the same for all regions
 */
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

gdouble regional_getfrequency(struct regional* regional, enum RXWINDOW rxwindow,
		const struct pktfwdpkt* rxpkt) {
	switch (rxwindow) {
	case RXW_R2:
	case RXW_J2:
		return (gdouble) regional->rx2.frequency / 1000000.0;
	default:
		return rxpkt->rfparams.frequency;
	}
}

const gchar* regional_getdatarate(struct regional* regional,
		enum RXWINDOW rxwindow, const struct pktfwdpkt* rxpkt) {
	switch (rxwindow) {
	case RXW_R2:
	case RXW_J2:
		return regional->datarates[regional->rx2.datarate];
	default:
		return rxpkt->rfparams.datarate;
	}
}
