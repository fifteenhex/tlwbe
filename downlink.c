#include <json-glib/json-glib.h>
#include <string.h>

#include "downlink.h"
#include "pktfwdbr.h"
#include "utils.h"

gchar* downlink_createtxjson(guchar* data, gsize datalen, gsize* length,
		const struct pktfwdpkt* rxpkt) {
	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);

	// add in the rf stuff
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_MODU);
	json_builder_add_string_value(jsonbuilder, rxpkt->modulation);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_FREQ);
	json_builder_add_double_value(jsonbuilder, rxpkt->frequency);
	//

	guchar* txdata = "yo yo";
	gchar* b64txdata = g_base64_encode(txdata, 5);

	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_DATA);
	json_builder_add_string_value(jsonbuilder, b64txdata);
	json_builder_set_member_name(jsonbuilder, PKTFWDBR_JSON_TXPK_SIZE);
	json_builder_add_int_value(jsonbuilder, strlen(b64txdata));

	json_builder_end_object(jsonbuilder);

	char* json = utils_jsonbuildertostring(jsonbuilder, length);

	g_object_unref(jsonbuilder);
	g_free(b64txdata);

	return json;
}
