#include <string.h>
#include <stdio.h>

#include "core.h"
#include "bir.h"
#include "font.h"
#include "maths.h"
#include "res.h"
#include "stb.h"

struct pak_header {
	char id[4];
	u32 offset;
	u32 size;
};

struct pak_entry {
	char name[56];
	u32 offset;
	u32 size;
};

const struct res_pak {
	FILE* handle;

	struct pak_entry* entries;
	usize entry_count;
}* bound_pak;

void res_use_pak(const struct res_pak* pak) {
	bound_pak = pak;
}

struct res_pak* pak_open(const char* path) {
	FILE* handle = fopen(path, "rb");
	if (!handle) {
		error("Failed to open `%s'.", path);
		return null;
	}

	struct res_pak* pak = core_calloc(1, sizeof(struct res_pak));

	pak->handle = handle;

	struct pak_header header;
	fread(header.id, 1, sizeof header.id, handle);
	fread(&header.offset, 1, sizeof header.offset, handle);
	fread(&header.size, 1, sizeof header.size, handle);

	pak->entry_count = (usize)header.size / sizeof(struct pak_entry);
	pak->entries = core_alloc((usize)header.size);

	fseek(handle, header.offset, SEEK_SET);

	for (usize i = 0; i < pak->entry_count; i++) {
		struct pak_entry* e = pak->entries + i;
		fread(&e->name, 1, sizeof e->name, handle);
		fread(&e->offset, 1, sizeof e->offset, handle);
		fread(&e->size, 1, sizeof e->size, handle);
	}

	return pak;
}

void pak_close(struct res_pak* pak) {
	fclose(pak->handle);
	core_free(pak->entries);
	core_free(pak);
}

bool write_pak(const char* outname, const char** files, usize file_count) {
	u8 buffer[2048];

	FILE* outfile = fopen(outname, "wb");
	if (!outfile) {
		error("Failed to open `%s' for writing.", outname);
		return false;
	}

	struct pak_header header = { .id = "PACK" };

	fseek(outfile, sizeof header, SEEK_SET);

	vector(struct pak_entry) entries = null;

	bool ok = true;

	usize blob_size = 0;

	for (usize i = 0; i < file_count; i++) {
		FILE* infile = fopen(files[i], "rb");
		if (!infile) {
			error("Failed to open `%s'.", files[i]);
			ok = false;
			continue;
		}

		fseek(infile, 0, SEEK_END);
		usize file_size = ftell(infile);
		rewind(infile);

		for (usize j = 0; j < file_size;) {
			usize read = fread(buffer, 1, sizeof buffer, infile);
			fwrite(buffer, 1, read, outfile);

			j += read;
		}

		struct pak_entry entry = {
			.offset = (u32)(sizeof header + blob_size),
			.size = (u32)file_size
		};

		usize name_len = strlen(files[i]);
		memcpy(entry.name, files[i], cr_max(name_len, 55));
		entry.name[name_len] = '\0';

		vector_push(entries, entry);

		blob_size += file_size;

		fclose(infile);
	}

	header.offset = (u32)(sizeof header + blob_size);
	header.size = (u32)(vector_count(entries) * sizeof(struct pak_entry));

	for (usize i = 0; i < vector_count(entries); i++) {
		fwrite(entries[i].name,    1, sizeof entries[i].name,   outfile);
		fwrite(&entries[i].offset, 1, sizeof entries[i].offset, outfile);
		fwrite(&entries[i].size,   1, sizeof entries[i].size,   outfile);
	}

	fseek(outfile, 0, SEEK_SET);

	fwrite(header.id, 1, sizeof header.id, outfile);
	fwrite(&header.offset, 1, sizeof header.offset, outfile);
	fwrite(&header.size, 1, sizeof header.size, outfile);

	fclose(outfile);

	free_vector(entries);

	return ok;
}

bool read_raw(const char* path, u8** buf, usize* size) {
	*buf = null;
	size ? *size = 0 : 0;

	if (bound_pak) {
		for (usize i = 0; i < bound_pak->entry_count; i++) {
			struct pak_entry* e = bound_pak->entries + i;

			if (strcmp(e->name, path) == 0) {
				fseek(bound_pak->handle, e->offset, SEEK_SET);

				*buf = core_alloc((usize)e->size);
				
				if (size) { *size = (usize)e->size; }

				fread(*buf, 1, (usize)e->size, bound_pak->handle);

				return true;
			}
		}

		error("Failed to find `%s' in package.", path);

		return false;
	} else {
		FILE* file = fopen(path, "rb");
		if (!file) {
			error("Failed to open file %s.", path);
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
	}

	return true;
}

bool write_raw(const char* path, const u8* buf, usize size) {
	FILE* file = fopen(path, "wb");

	if (!file) {
		error("Failed to open file `%s' for writing.\n", path);
		return false;
	}

	fwrite(buf, 1, size, file);

	fclose(file);

	return true;
}

bool read_raw_text(const char* path, char** buf) {
	*buf = null;

	if (bound_pak) {
		for (usize i = 0; i < bound_pak->entry_count; i++) {
			struct pak_entry* e = bound_pak->entries + i;

			if (strcmp(e->name, path) == 0) {
				fseek(bound_pak->handle, e->offset, SEEK_SET);

				*buf = core_alloc((usize)e->size + 1);

				fread(*buf, 1, (usize)e->size, bound_pak->handle);
				(*buf)[e->size] = '\0';

				return true;
			}
		}

		error("Failed to find `%s' in package.", path);

		return false;
	} else {
		FILE* file = fopen(path, "r");
		if (!file) {
			error("Failed to fopen file `%s'.", path);
			return false;
		}

		fseek(file, 0, SEEK_END);
		const usize file_size = ftell(file);
		rewind(file);

		*buf = core_alloc(file_size + 1);
		const usize bytes_read = fread(*buf, sizeof(char), file_size, file);

		(*buf)[bytes_read] = '\0';

		fclose(file);
	}

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

void init_image_from_raw(struct image* image, const u8* raw, usize raw_size) {
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
	bool ok;
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
	bound_pak = null;

	reg_res_type("image", &(struct res_config) {
		.payload_size = sizeof(struct image),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_png,
		.alt_raw_size = bir_error_png_size,
		.on_load = image_on_load,
		.on_unload = image_on_unload
	});

	reg_res_type("true type font", &(struct res_config) {
		.payload_size = font_struct_size(),
		.free_raw_on_load = false,
		.terminate_raw = false,
		.alt_raw = bir_DejaVuSans_ttf,
		.alt_raw_size = bir_DejaVuSans_ttf_size,
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

void* res_load(const char* type, const char* filename, void* udata) {
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

	bool error;

	if (config->terminate_raw) {
		error = !read_raw_text(filename, (char**)&raw);

		if (!error) { raw_size = strlen((char*)raw) + 1; }
	} else {
		error = !read_raw(filename, &raw, &raw_size);
	}

	if (error) {
		core_free(raw);

		if (config->alt_raw && config->alt_raw_size) {
			raw = config->alt_raw;
			raw_size = config->alt_raw_size;
		}
	}

	new_res.ok = !error;

	config->on_load(filename, raw, raw_size, new_res.payload, new_res.payload_size, udata);

	if (config->free_raw_on_load && !error) {
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

	if (res->ok) {
		core_free(res->payload);
	}

	table_delete(res_cache, hash_string(filename) + res->config_id);
}

struct texture* load_texture(const char* filename, u32 flags) {
	return res_load("texture", filename, &flags);
}

struct font* load_font(const char* filename, f32 size) {
	return res_load("true type font", filename, &size);
}

struct shader* load_shader(const char* filename) {
	return res_load("shader", filename, null);
}
