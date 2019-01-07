#include <json-glib/json-glib.h>
#include <mosquitto.h>
#include <string.h>
#include <stdio.h>
#include <lorawan/lorawan.h>
#include <lorawan/crypto.h>

#include "control.h"
#include "database.h"
#include "utils.h"

#include "json-glib-macros/jsonbuilderutils.h"

#include "control.json.h"

enum entity {
	ENTITY_APP, ENTITY_DEV, ENTITY_INVALID
};

enum action {
	ACTION_ADD,
	ACTION_UPDATE,
	ACTION_DEL,
	ACTION_GET,
	ACTION_LIST,
	ACTION_INVALID
};

static const char* entities[] = { CONTROL_ENTITY_APP, CONTROL_ENTITY_DEV };

static const char* actions[] = { CONTROL_ACTION_ADD, CONTROL_ACTION_UPDATE,
CONTROL_ACTION_DEL, CONTROL_ACTION_GET, CONTROL_ACTION_LIST };

static char* control_generateeui64(char* buff) {
	guint64 now = g_get_real_time() / 1000000;
	guint32 rand = g_random_int();
	guint64 eui = (now << 4) | rand;
	sprintf(buff, "%"G_GINT64_MODIFIER"x", eui);
	return buff;
}

static char* control_generatekey() {
	uint8_t key[KEYLEN];
	crypto_randbytes(key, sizeof(key));
	return utils_bin2hex(key, sizeof(key));
}

static int control_app_add(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {

	struct control_app_add appadd = { 0 };
	if (!__jsongen_control_app_add_from_json(&appadd, rootobj)) {
		return -1;
	}

	gchar euibuff[EUIASCIILEN];
	if (appadd.eui == NULL) {
		g_message("no eui provided for app, generating one");
		appadd.eui = control_generateeui64(euibuff);
	}

	struct app a = { .name = appadd.name, .eui = appadd.eui };

	database_app_add(cntx, &a);

	json_builder_set_member_name(jsonbuilder, "app");
	__jsongen_app_to_json(&a, jsonbuilder);

	return 0;
}

static void control_app_get_callback(const struct app* app, void* data) {
	JsonBuilder* jsonbuilder = data;
	json_builder_set_member_name(jsonbuilder, "app");
	__jsongen_app_to_json(app, jsonbuilder);
}

static int control_app_get(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	int ret = -1;
	struct control_app_dev_get appdevget = { 0 };
	if (__jsongen_control_app_dev_get_from_json(&appdevget, rootobj)) {
		if (appdevget.eui != NULL || appdevget.name != NULL) {
			database_app_get(cntx, appdevget.eui, appdevget.name,
					control_app_get_callback, jsonbuilder);
			ret = 0;
		}
	}
	return 0;
}

static int control_app_update(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	database_app_update(cntx, NULL);
	return 0;
}

static int control_app_del(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	int ret = -1;
	struct control_app_dev_del request = { 0 };
	if (__jsongen_control_app_dev_del_from_json(&request, rootobj)) {
		database_app_del(cntx, request.eui);
		ret = 0;
	}
	return ret;
}

static void control_apps_list_euicallback(const char* eui, void* data) {
	JsonBuilder* jsonbuilder = data;
	json_builder_add_string_value(jsonbuilder, eui);
}

static int control_apps_list(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	json_builder_set_member_name(jsonbuilder, CONTROL_JSON_EUI_LIST);
	json_builder_begin_array(jsonbuilder);
	database_apps_list(cntx, control_apps_list_euicallback, jsonbuilder);
	json_builder_end_array(jsonbuilder);
	return 0;
}

static int control_dev_add(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	int ret = -1;
	struct control_dev_add devadd = { 0 };
	gchar euibuff[EUIASCIILEN];
	gchar keybuff[KEYASCIILEN];
	gboolean freekey = FALSE;
	if (__jsongen_control_dev_add_from_json(&devadd, rootobj)) {
		if (devadd.eui == NULL) {
			g_message("no eui provided for dev, generating one");
			devadd.eui = control_generateeui64(euibuff);
		}

		if (devadd.key == NULL) {
			g_message("no key provided for dev, generating one");
			devadd.key = control_generatekey();
			freekey = TRUE;
		}

		struct dev d = { .eui = devadd.eui, .appeui = devadd.appeui, .key =
				devadd.key, .name = devadd.name };
		database_dev_add(cntx, &d);

		json_builder_set_member_name(jsonbuilder, "dev");
		__jsongen_dev_to_json(&d, jsonbuilder);

		if (freekey)
			g_free((void*) devadd.key);

		ret = 0;
	}

	return ret;
}

static int control_dev_update(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	database_dev_update(cntx, NULL);
	return 0;
}

static void control_dev_get_callback(const struct dev* dev, void* data) {
	JsonBuilder* jsonbuilder = data;
	json_builder_set_member_name(jsonbuilder, "dev");
	__jsongen_dev_to_json(dev, jsonbuilder);
}

static int control_dev_get(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	int ret = -1;

	struct control_app_dev_get appdevget = { 0 };
	if (__jsongen_control_app_dev_get_from_json(&appdevget, rootobj)) {
		if (appdevget.eui != NULL || appdevget.name != NULL) {
			database_dev_get(cntx, appdevget.eui, appdevget.name,
					control_dev_get_callback, jsonbuilder);
			ret = 0;
		}
	}
	return ret;
}

static int control_dev_del(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	int ret = -1;
	struct control_app_dev_del request = { 0 };
	if (__jsongen_control_app_dev_del_from_json(&request, rootobj)) {
		database_dev_del(cntx, request.eui);
		ret = 0;
	}
	return ret;
}

static int control_devs_list(struct context* cntx, const JsonObject* rootobj,
		JsonBuilder* jsonbuilder) {
	json_builder_set_member_name(jsonbuilder, CONTROL_JSON_EUI_LIST);
	json_builder_begin_array(jsonbuilder);
	database_devs_list(cntx, control_apps_list_euicallback, jsonbuilder);
	json_builder_end_array(jsonbuilder);
	return 0;
}

void control_onmsg(struct context* cntx, char** splittopic, int numtopicparts,
		const JsonObject* rootobj) {

	if (numtopicparts < 3) {
		g_message("need 3 or more topic parts, have %d", numtopicparts);
		goto out;
	}

	char* entity = splittopic[0];
	char* action = splittopic[1];
	char* token = splittopic[2];

	enum entity e = ENTITY_INVALID;
	for (int i = 0; i < G_N_ELEMENTS(entities); i++) {
		if (strcmp(entity, entities[i]) == 0)
			e = i;
	}
	if (e == ENTITY_INVALID) {
		g_message("invalid entity %s", entity);
		goto out;
	}

	enum action a = ACTION_INVALID;
	for (int i = 0; i < G_N_ELEMENTS(actions); i++) {
		if (strcmp(action, actions[i]) == 0)
			a = i;
	}
	if (a == ACTION_INVALID) {
		g_message("invalid action %s", action);
		goto out;
	}

	JsonBuilder* jsonbuilder = json_builder_new();
	json_builder_begin_object(jsonbuilder);

	int code = -1;
	switch (e) {
	case ENTITY_APP:
		switch (a) {
		case ACTION_ADD:
			code = control_app_add(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_UPDATE:
			code = control_app_update(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_GET:
			code = control_app_get(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_DEL:
			code = control_app_del(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_LIST:
			code = control_apps_list(cntx, rootobj, jsonbuilder);
			break;
		default:
			g_assert(false);
		}

		break;
	case ENTITY_DEV:
		switch (a) {
		case ACTION_ADD:
			code = control_dev_add(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_UPDATE:
			code = control_dev_update(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_GET:
			code = control_dev_get(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_DEL:
			code = control_dev_del(cntx, rootobj, jsonbuilder);
			break;
		case ACTION_LIST:
			code = control_devs_list(cntx, rootobj, jsonbuilder);
			break;
		default:
			g_assert(false);
		}
		break;
	default:
		g_assert(false);
	}

	JSONBUILDER_ADD_INT(jsonbuilder, "code", code);

	json_builder_end_object(jsonbuilder);

	gchar* topic = mosquitto_client_createtopic(TLWBE_TOPICROOT,
	CONTROL_SUBTOPIC, CONTROL_RESULT, token, NULL);
	mosquitto_client_publish_json_builder(cntx->mosqclient, jsonbuilder, topic,
	TRUE);
	g_free(topic);

	out: return;
}

void control_onbrokerconnect(const struct context* cntx) {
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, TLWBE_TOPICROOT"/"CONTROL_SUBTOPIC"/"CONTROL_ENTITY_APP"/#", 0);
	mosquitto_subscribe(mosquitto_client_getmosquittoinstance(cntx->mosqclient),
	NULL, TLWBE_TOPICROOT"/"CONTROL_SUBTOPIC"/"CONTROL_ENTITY_DEV"/#", 0);
}

