#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>

#include "control.h"

enum action {
	ACTION_ADD,
	ACTION_UPDATE,
	ACTION_DEL,
	ACTION_GET,
	ACTION_LIST,
	ACTION_INVALID
};

static const char* actions[] = { CONTROL_ACTION_ADD, CONTROL_ACTION_UPDATE,
CONTROL_ACTION_DEL, CONTROL_ACTION_GET, CONTROL_ACTION_LIST };

void control_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts) {

	gchar* payload = NULL;
	char* entity = splittopic[2];
	char* action = splittopic[3];

	enum action a = ACTION_INVALID;
	for (int i = 0; i < G_N_ELEMENTS(actions); i++) {
		if (strcmp(action, actions[i]) == 0)
			a = i;
	}
	if (a == ACTION_INVALID) {
		g_message("invalid action %d", action);
		goto out;
	}

	JsonParser* jsonparser = json_parser_new_immutable();
	if (!json_parser_load_from_data(jsonparser, msg->payload, msg->payloadlen,
	NULL)) {
		g_message("failed to parse control message json");
		goto out;
	}

	const gchar* token = NULL;
	{
		JsonNode* rootnode = json_parser_get_root(jsonparser);
		if (json_node_get_node_type(rootnode) != JSON_NODE_OBJECT) {
			g_message("control message root should be an object");
			goto out;
		}
		JsonObject* rootobj = json_node_get_object(rootnode);
		if (!json_object_has_member(rootobj, CONTROL_JSON_TOKEN)) {
			g_message("control message does not contain token");
			goto out;
		}
		token = json_object_get_string_member(rootobj, CONTROL_JSON_TOKEN);
	}

	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);
	json_builder_set_member_name(jsonbuilder, CONTROL_JSON_TOKEN);
	json_builder_add_string_value(jsonbuilder, token);

	if (strcmp(entity, CONTROL_ENTITY_APP) == 0) {

	} else if (strcmp(entity, CONTROL_ENTITY_DEV) == 0) {

	} else {
		g_message("unexpected entity %s", entity);
	}

	json_builder_end_object(jsonbuilder);

	JsonNode* root = json_builder_get_root(jsonbuilder);
	JsonGenerator* generator = json_generator_new();
	json_generator_set_root(generator, root);

	gsize payloadlen;
	payload = json_generator_to_data(generator, &payloadlen);

	json_node_free(root);
	g_object_unref(generator);
	g_object_unref(jsonbuilder);

	mosquitto_publish(cntx->mosq, NULL, TLWBE_TOPICROOT"/"CONTROL_SUBTOPIC"/"CONTROL_RESULT, payloadlen, payload, 0, false);

	out: if (payload != NULL)
		g_free(payload);
}

void control_onbrokerconnect(struct context* cntx) {
mosquitto_subscribe(cntx->mosq, NULL, TLWBE_TOPICROOT"/"CONTROL_SUBTOPIC"/"CONTROL_ENTITY_APP"/#",
		0);
mosquitto_subscribe(cntx->mosq, NULL, TLWBE_TOPICROOT"/"CONTROL_SUBTOPIC"/"CONTROL_ENTITY_DEV"/#",
		0);
}

