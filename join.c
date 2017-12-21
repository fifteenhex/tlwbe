#include <glib.h>
#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>

#include "join.h"
#include "tlwbe.h"
#include "lorawan.h"
#include "pktfwdbr.h"
#include "crypto.h"
#include "utils.h"
#include "database.h"

gchar* createtxjson(guchar* data, gsize datalen, gsize* length) {
	JsonGenerator* generator = json_generator_new();
	JsonNode* rootnode = json_node_new(JSON_NODE_OBJECT);
	JsonObject* rootobj = json_object_new();
	json_node_set_object(rootnode, rootobj);
	json_generator_set_root(generator, rootnode);

	guchar* txdata = "yo yo";
	gchar* b64txdata = g_base64_encode(txdata, 5);
	json_object_set_string_member(rootobj, PKTFWDBR_JSON_TXPK_DATA, b64txdata);
	json_object_set_int_member(rootobj, PKTFWDBR_JSON_TXPK_SIZE,
			strlen(b64txdata));

	return json_generator_to_data(generator, length);
}

static void join_processjoinrequest_rowcallback(struct dev* d, void* data) {
	uint8_t* key = data;
	for (int i = 0; i < KEYLEN; i++) {
		unsigned b;
		sscanf(d->key + (i * 2), "%02x", &b);
		key[i] = b;
	}
}

void join_processjoinrequest(struct context* cntx, const gchar* gateway,
		guchar* data, int datalen) {
	guchar* pkt = data + 1;
	int joinreqlen = 1 + sizeof(struct lorawan_joinreq) + 4;
	if (datalen == joinreqlen) {
		struct lorawan_joinreq* joinreq = (struct lorawan_joinreq*) pkt;
		guint64 appeui = GUINT64_FROM_LE(joinreq->appeui);
		guint64 deveui = GUINT64_FROM_LE(joinreq->deveui);
		guint16 devnonce = GUINT16_FROM_LE(joinreq->devnonce);
		g_message("handling join request for app %"G_GINT64_MODIFIER
				"x from %"G_GINT64_MODIFIER"x "
				"nonce %"G_GINT16_MODIFIER"x", appeui, deveui, devnonce);

		pkt += sizeof(*joinreq);
		guint32 inmic;
		memcpy(&inmic, pkt, sizeof(inmic));

		char asciieui[EUIASCIILEN];
		sprintf(asciieui, "%016"G_GINT64_MODIFIER"x", deveui);
		uint8_t key[KEYLEN];
		database_dev_get(cntx, asciieui, join_processjoinrequest_rowcallback,
				key);

		guint32 calcedmic = crypto_mic(key, sizeof(key), data,
				sizeof(*joinreq) + 1);

	if (calcedmic == inmic) {
		gchar* topic = utils_createtopic(gateway, PKTFWDBR_TOPIC_TX, NULL);
		gsize payloadlen;
		gchar* payload = createtxjson(NULL, 0, &payloadlen);
		mosquitto_publish(cntx->mosq,NULL, topic, payloadlen, payload, 0, false);
		g_free(topic);
	}
	else
	g_message("mic should be %"G_GINT32_MODIFIER"x, calculated %"G_GINT32_MODIFIER"x", inmic, calcedmic);

} else
	g_message("join request should be %d bytes long, have %d", joinreqlen,
			datalen);
}

