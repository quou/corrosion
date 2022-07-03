#pragma once

#include "common.h"
#include "core.h"

enum {
	dtable_int,
	dtable_real,
	dtable_string,
	dtable_parent
};

struct dtable {
	u32 type;
};

void write_dtable(const char* filename, struct dtable* dtable);
