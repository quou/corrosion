#pragma once

#include "common.h"
#include "core.h"
#include "maths.h"

struct dtable_key {
	char chars[64];
	usize len;
	u64 hash;
};

enum {
	dtable_parent = 0,
	dtable_number,
	dtable_bool,
	dtable_string,
	dtable_colour,
	dtable_v2,
	dtable_v3,
	dtable_v4
};

struct dtable_value {
	u32 type;

	union {
		f64 number;
		bool boolean;
		char* string;
		v4f colour;
		v2f v2;
		v3f v3;
		v4f v4;
	} as;
};

struct dtable {
	struct dtable_key key;

	struct dtable_value value;

	vector(struct dtable) children;
};

struct dtable new_dtable(const char* name);
struct dtable new_number_dtable(const char* name, f64 value);
struct dtable new_bool_dtable(const char* name, bool value);
struct dtable new_string_dtable(const char* name, const char* string);
struct dtable new_string_n_dtable(const char* name, const char* string, usize n);
struct dtable new_colour_dtable(const char* name, v4f value);

void dtable_add_child(struct dtable* dt, const struct dtable* child);

void write_dtable(const struct dtable* dt, const char* filename);

/* Adds a the parsed table as a child to `dt'. Returns zero on failure. */
bool parse_dtable(struct dtable* dt, const char* text);

void deinit_dtable(struct dtable* dt);

bool dtable_find_child(const struct dtable* dt, const char* key, struct dtable* dst);
