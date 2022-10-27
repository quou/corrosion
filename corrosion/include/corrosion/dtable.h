#pragma once

#include "common.h"
#include "core.h"
#include "maths.h"

/* Text-based serialisation format.
 *
 * Syntax example:
 *     a_key = {
 *         some_string = "Hello, world";
 *         some_colour = colour(#ff00ff, 150);
 *         some_vector2 = v2(40, 2);
 *         some_number = 40.5;
 *         some_child = {
 *             some_boolean = true;
 *             some_vector3 = v3(40, 50); 
 *         }
 *     }
 * 
 * The above table could be accessed from C like so:
 *     struct dtable t;
 *     parse_dtable(&t, source_text);
 *     
 *     struct dtable a_key;
 *     if (dtable_find_child(&t, "a_key", &a_key)) {
 *         // Find "some_string" and print the value.
 *         struct dtable some_string;
 *         if (dtable_find_child(&a_key, "some_string", &some_string)) {
 *             if (some_string.value.type == dtable_string) {
 *                 const char* str = some_string.as.string;
 *                 info("a_key:some_string: %s", str);
 *             }
 *         }
 *     }
 * 
 * A table like this:
 *     a_key = {
 *         some_string = "Hello, world";
 *     }
 * ...Could be generated and written to a file by the following C code:
 *     struct dtable dt = new_dtable("my_table");
 *     
 *     struct dtable a_key = new_dtable("a_key");
 *     struct dtable some_string = new_string_dtable("some_string", "Hello, world");
 *     dtable_add_child(&a_key, &some_string);
 *     
 *     dtable_add_child(&dt, &a_key);
 *     
 *     write_dtable(&dt, "my_dtable.dt");
 */

struct dtable_key {
	char chars[64];
	usize len;
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
struct dtable new_v2_dtable(const char* name, v2f value);
struct dtable new_v3_dtable(const char* name, v3f value);
struct dtable new_v4_dtable(const char* name, v4f value);

void dtable_add_child(struct dtable* dt, const struct dtable* child);

void write_dtable(const struct dtable* dt, const char* filename);

/* Adds a the parsed table as a child to `dt'. Returns zero on failure. */
bool parse_dtable(struct dtable* dt, const char* text);

void deinit_dtable(struct dtable* dt);

bool dtable_find_child(const struct dtable* dt, const char* key, struct dtable* dst);
