#pragma once

struct pair {
	void* first;
	void* second;
};

gchar* utils_createtopic(const gchar* id, ...);
