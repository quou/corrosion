#pragma once

#include "common.h"
#include "maths.h"

struct window_config {
	const char* title;
	v2i size;
	bool resizable;
};

struct app_config {
	const char* name;
	u32 api;
	bool enable_validation;
	struct window_config window_config;
};
