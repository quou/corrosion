#pragma once

#include "common.h"
#include "maths.h"

struct window_config {
	const char* title;
	v2i size;
	bool resizable;
};

struct video_config {
	u32 api;
	bool enable_validation;
	v4f clear_colour;
};

struct app_config {
	const char* name;
	struct video_config video_config;
	struct window_config window_config;
};
