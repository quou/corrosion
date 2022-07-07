#include <stdio.h>
#include <string.h>

#include "dtable.h"
#include "maths.h"

struct dtable new_dtable(const char* name) {
	struct dtable r = { 0 };

	r.key.len = strlen(name);
	r.key.hash = elf_hash((const u8*)name, r.key.len);
	memcpy(r.key.chars, name, min(r.key.len, sizeof r.key.chars));

	r.value.type = dtable_empty;

	return r;
}

struct dtable new_int_dtable(const char* name, i64 value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_int;
	r.value.as.integer = value;

	return r;
}

struct dtable new_float_dtable(const char* name, f64 value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_float;
	r.value.as.real = value;

	return r;
}

struct dtable new_bool_dtable(const char* name, bool value) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_bool;
	r.value.as.boolean = value;

	return r;
}

struct dtable new_string_dtable(const char* name, const char* string) {
	struct dtable r = new_dtable(name);

	usize len = strlen(string);

	r.value.type = dtable_string;

	r.value.as.string = core_alloc(len + 1);
	memcpy(r.value.as.string, string, len);
	r.value.as.string[len] = '\0';

	return r;
}

struct dtable new_string_n_dtable(const char* name, const char* string, usize n) {
	struct dtable r = new_dtable(name);

	r.value.type = dtable_string;

	r.value.as.string = core_alloc(n + 1);
	memcpy(r.value.as.string, string, n);
	r.value.as.string[n] = '\0';

	return r;
}

void dtable_add_child(struct dtable* dt, const struct dtable* child) {
	vector_push(dt->children, *child);
}

static void write_dtable_value(const struct dtable_value* val, FILE* file) {
	switch (val->type) {
		case dtable_int:
			fprintf(file, "%ld", val->as.integer);
			break;
		case dtable_float:
			fprintf(file, "%g", val->as.real);
			break;
		case dtable_bool:
			fprintf(file, val->as.boolean ? "true" : "false");
			break;
		case dtable_string:
			fprintf(file, "\"%s\"", val->as.string);
			break;
		default: break;
	}

	fwrite(";", 1, 1, file);
}

static void write_indent(u32 count, FILE* file) {
	for (u32 i = 0; i < count; i++) {
		fwrite("\t", 1, 1, file);
	}
}

static void impl_write_dtable(const struct dtable* dt, FILE* file, u32 indent) {
	write_indent(indent, file);

	fwrite(dt->key.chars, 1, dt->key.len, file);
	fwrite(" = ", 1, 3, file);

	if (dt->value.type == dtable_empty) {
		fwrite("{\n", 1, 2, file);

		for (struct dtable* ch = vector_start(dt->children); ch < vector_end(dt->children); ch++) {
			impl_write_dtable(ch, file, indent + 1);
		}

		write_indent(indent, file);
		fwrite("}\n", 1, 2, file);
	} else {
		write_dtable_value(&dt->value, file);
		fwrite("\n", 1, 1, file);
	}
}

void write_dtable(const struct dtable* dt, const char* filename) {
	FILE* file = fopen(filename, "w");
	if (!file) {
		error("Failed to fopen `%s' for writing.", filename);
	}

	impl_write_dtable(dt, file, 0);

	fclose(file);
}
