#pragma once

#include "common.h"
#include "core.h"

struct dtable_key {
	char chars[64];
	usize len;
	u64 hash;
};

enum {
	dtable_empty = 0,
	dtable_int,
	dtable_float,
	dtable_bool,
	dtable_string
};

struct dtable_value {
	u32 type;

	union {
		i64 integer;
		f64 real;
		bool boolean;
		char* string;
	} as;
};

struct dtable {
	struct dtable_key key;

	struct dtable_value value;

	vector(struct dtable) children;
};

struct dtable new_dtable(const char* name);
struct dtable new_int_dtable(const char* name, i64 value);
struct dtable new_float_dtable(const char* name, f64 value);
struct dtable new_bool_dtable(const char* name, bool value);
struct dtable new_string_dtable(const char* name, const char* string);
struct dtable new_string_n_dtable(const char* name, const char* string, usize n);

void dtable_add_child(struct dtable* dt, const struct dtable* child);

void write_dtable(const struct dtable* dt, const char* filename);
