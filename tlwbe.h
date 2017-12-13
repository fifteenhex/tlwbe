#pragma once

struct context {
	struct mosquitto* mosq;
	const gchar* mqtthost;
	gint mqttport;
	GIOChannel* mosqchan;
	guint mosqsource;
};
