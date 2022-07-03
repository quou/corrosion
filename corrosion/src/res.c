#include <string.h>
#include <stdio.h>

#include "core.h"
#include "font.h"
#include "res.h"
#include "stb.h"

bool read_raw(const char* path, u8** buf, usize* size) {
	*buf = null;
	size ? *size = 0 : 0;

	FILE* file = fopen(path, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open file %s.\n", path);
		return false;
	}

	fseek(file, 0, SEEK_END);
	const usize file_size = ftell(file);
	rewind(file);

	*buf = core_alloc(file_size);
	const usize bytes_read = fread(*buf, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		warning("Failed to read all of file `%s'.\n", path);
	}

	if (size) {
		*size = file_size;
	}

	fclose(file);

	return true;
}

bool write_raw(const char* path, const u8* buf, usize size) {
	FILE* file = fopen(path, "wb");

	if (!file) {
		error("Failed to open file `%s' for reading.\n", path);
		return false;
	}

	fwrite(buf, 1, size, file);

	fclose(file);

	return true;
}

bool read_raw_text(const char* path, char** buf) {
	*buf = null;

	FILE* file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "Failed to fopen file `%s'.\n", path);
		return false;
	}

	fseek(file, 0, SEEK_END);
	const usize file_size = ftell(file);
	rewind(file);

	*buf = core_alloc(file_size + 1);
	const usize bytes_read = fread(*buf, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		warning("Failed to read all of file: `%s'\n", path);
	}

	buf[file_size] = '\0';

	fclose(file);

	return true;
}

bool write_raw_text(const char* path, const char* buf) {
	FILE* file = fopen(path, "w");

	if (!file) {
		error("Failed to open file `%s' for reading.\n", path);
		return false;
	}

	usize size = strlen(buf);

	fwrite(buf, 1, size, file);

	fclose(file);

	return true;
}

void init_image_from_raw(struct image* image, u8* raw, usize raw_size) {
	i32 c;

	u8* data = stbi_load_from_memory(raw, (i32)raw_size, &image->size.x, &image->size.y, &c, 4);
	if (!data) {
		error("Failed to parse image: %s", stbi_failure_reason());
	}

	image->colours = data;
}

void deinit_image(struct image* image) {
	stbi_image_free(image->colours);
}

struct res {
	u8* payload;
	usize payload_size;
	u64 config_id;
};

table(u64, struct res_config) res_registry = { 0 };
table(u64, struct res)        res_cache    = { 0 };

static void image_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct image* image = payload;

	init_image_from_raw(image, raw, raw_size);
}

static void image_on_unload(void* payload, usize payload_size) {
	struct image* image = payload;

	stbi_image_free(image->colours);
}

static void ttf_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	f32 size = 14.0f;

	if (udata) {
		size = *(f32*)udata;
	}

	init_font(payload, raw, raw_size, size);
}

static void ttf_on_unload(void* payload, usize payload_size) {
	void* data = get_font_data(payload);
	deinit_font(payload);
	core_free(data);
}

void res_init(const char* argv0) {
	reg_res_type("image", &(struct res_config) {
		.payload_size = sizeof(struct image),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.on_load = image_on_load,
		.on_unload = image_on_unload
	});

	reg_res_type("true type font", &(struct res_config) {
		.payload_size = font_struct_size(),
		.free_raw_on_load = false,
		.terminate_raw = false,
		.on_load = ttf_on_load,
		.on_unload = ttf_on_unload
	});
}

void res_deinit() {
	for (u64* i = table_first(res_cache); i; i = table_next(res_cache, *i)) {
		struct res* res = table_get(res_cache, *i);

		struct res_config* config = table_get(res_registry, res->config_id);

		config->on_unload(res->payload, res->payload_size);

		core_free(res->payload);
	}

	free_table(res_cache);
	free_table(res_registry);
}

void reg_res_type(const char* type, struct res_config* config) {
	table_set(res_registry, hash_string(type), *config);
}

void* res_load_p(const char* type, const char* filename, void* udata) {
	struct file_info info;
	if (!get_file_info(filename, &info)) {
		error("Failed to stat `%s'.", filename);
		return null;
	}

	if (info.type != file_normal) {
		error("`%s' is a directory, symlink or other non-normal file. Do not try to load it as a resource.", filename);
		return null;
	}

	u64 config_id = hash_string(type);

	struct res* got = table_get(res_cache, hash_string(filename) + config_id);
	if (got) { return got->payload; }

	struct res_config* config = table_get(res_registry, config_id);

	if (!config) {
		error("Loading `%s': No resource handler registered for this type of file (%s).", filename, type);
		return null;
	}

	struct res new_res = {
		.payload = core_calloc(1, config->payload_size),
		.payload_size = config->payload_size,
		.config_id = config_id
	};

	u8* raw;
	usize raw_size;

	if (config->terminate_raw) {
		read_raw_text(filename, (char**)&raw);
		raw_size = strlen((char*)raw) + 1;
	} else {
		read_raw(filename, &raw, &raw_size);
	}

	config->on_load(filename, raw, raw_size, new_res.payload, new_res.payload_size, udata);

	if (config->free_raw_on_load) {
		core_free(raw);
	}

	u64 cache_key = hash_string(filename) + config_id;

	table_set(res_cache, cache_key, new_res);

	return new_res.payload;
}

void res_unload(const char* filename) {
	struct res* res = table_get(res_cache, hash_string(filename));
	if (!res) { return; }

	struct res_config* config = table_get(res_registry, res->config_id);

	config->on_unload(res->payload, res->payload_size);

	core_free(res->payload);

	table_delete(res_cache, hash_string(filename) + res->config_id);
}

struct texture* load_texture(const char* filename, u32 flags) {
	return res_load("texture", filename, flags);
}

struct font* load_font(const char* filename, f32 size) {
	return res_load("true type font", filename, size);
}

struct shader* load_shader(const char* filename) {
	return res_load("shader", filename, 0);
}
