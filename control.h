#pragma once

#include "tlwbe.h"

#define CONTROL_SUBTOPIC		"control"
#define CONTROL_ENTITY_APP		"app"
#define CONTROL_ENTITY_DEV		"dev"
#define CONTROL_ACTION_ADD		"add"
#define CONTROL_ACTION_UPDATE	"update"
#define CONTROL_ACTION_DEL		"del"
#define CONTROL_ACTION_GET		"get"
#define CONTROL_ACTION_LIST		"list"
#define CONTROL_RESULT			"result"

#define CONTROL_JSON_TOKEN		"token"

void control_onbrokerconnect(struct context* cntx);
void control_onmsg(struct context* cntx, const struct mosquitto_message* msg,
		char** splittopic, int numtopicparts);
