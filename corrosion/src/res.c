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

	usize header_offset;

	struct pak_entry* entries;
	usize entry_count;
}* bound_pak;

void res_use_pak(const struct res_pak* pak) {
	bound_pak = pak;
}

const struct res_pak* res_bound_pack() {
	return bound_pak;
}

struct res_pak* pak_open(const char* path, usize header_offset) {
	FILE* handle = fopen(path, "rb");
	if (!handle) {
		error("Failed to open `%s'.", path);
		return null;
	}

	struct res_pak* pak = core_calloc(1, sizeof(struct res_pak));

	pak->header_offset = header_offset;

	fseek(handle, (long)pak->header_offset, SEEK_SET);

	pak->handle = handle;

	struct pak_header header;
	fread(header.id, 1, sizeof header.id, handle);
	fread(&header.offset, 1, sizeof header.offset, handle);
	fread(&header.size, 1, sizeof header.size, handle);

	pak->entry_count = (usize)header.size / sizeof(struct pak_entry);
	pak->entries = core_alloc((usize)header.size);

	fseek(handle, header.offset + (long)pak->header_offset, SEEK_SET);

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

usize write_pak(const char* outname, struct pak_write_file* files, usize file_count) {
	u8 buffer[2048];

	FILE* outfile = fopen(outname, "wb");
	if (!outfile) {
		error("Failed to open `%s' for writing.", outname);
		return 0;
	}

	struct pak_header header = { .id = "PACK" };

	fseek(outfile, sizeof header, SEEK_SET);

	vector(struct pak_entry) entries = null;

	usize written = 0;

	usize blob_size = 0;

	for (usize i = 0; i < file_count; i++) {
		FILE* infile = fopen(files[i].src, "rb");
		if (!infile) {
			error("Failed to open `%s'.", files[i].src);
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

		usize name_len = strlen(files[i].dst);
		memcpy(entry.name, files[i].dst, cr_max(name_len, 55));
		entry.name[name_len] = '\0';

		vector_push(entries, entry);

		blob_size += file_size;
		written += file_size;

		fclose(infile);
	}

	header.offset = (u32)(sizeof header + blob_size);
	header.size = (u32)(vector_count(entries) * sizeof(struct pak_entry));

	for (usize i = 0; i < vector_count(entries); i++) {
		fwrite(entries[i].name,    1, sizeof entries[i].name,   outfile);
		fwrite(&entries[i].offset, 1, sizeof entries[i].offset, outfile);
		fwrite(&entries[i].size,   1, sizeof entries[i].size,   outfile);
		written += sizeof entries[i].name + sizeof entries[i].offset + sizeof entries[i].size;
	}

	fseek(outfile, 0, SEEK_SET);

	fwrite(header.id, 1, sizeof header.id, outfile);
	fwrite(&header.offset, 1, sizeof header.offset, outfile);
	fwrite(&header.size, 1, sizeof header.size, outfile);

	written += sizeof header.id + sizeof header.offset + sizeof header.size;

	fclose(outfile);

	free_vector(entries);

	return written;
}

bool read_raw(const char* path, u8** buf, usize* size) {
	*buf = null;
	size ? *size = 0 : 0;

	if (bound_pak) {
		for (usize i = 0; i < bound_pak->entry_count; i++) {
			struct pak_entry* e = bound_pak->entries + i;

			if (strcmp(e->name, path) == 0) {
				fseek(bound_pak->handle, e->offset + (long)bound_pak->header_offset, SEEK_SET);

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
				fseek(bound_pak->handle, e->offset + (long)bound_pak->header_offset, SEEK_SET);

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

#ifdef __EMSCRIPTEN__
	/* I have no idea why this is necessary, but for some reason when loading images
	 * with Emscripten they are flipped...
	 *
	 * TODO: Figure out why. */
	flip_image_y(image);
#endif
}

void deinit_image(struct image* image) {
	stbi_image_free(image->colours);
}

void flip_image_y(struct image* image) {
	const usize stride = image->size.x * 4;
	u8* row  = core_alloc(stride);
	u8* low  = image->colours;
	u8* high = &image->colours[(image->size.y - 1) * stride];

	for (; low < high; low += stride, high -= stride) {
		memcpy(row, low, stride);
		memcpy(low, high, stride);
		memcpy(high, row, stride);
	}

	core_free(row);
}

struct res {
	u8* payload;
	const char* config_name;
	const char* name;
	usize payload_size;
	bool ok;
};

table(const char*, struct res_config) res_registry;
table(const char*, struct res)        res_cache;
const char* last_copied_cache_key = null;
const char* last_copied_type_key = null;

inline static u8* res_cache_table_copy_string(const u8* src) {
	char* r = copy_string((const char*)src);
	last_copied_cache_key = r;
	return (u8*)r;
}

inline static u8* res_registry_table_copy_string(const u8* src) {
	char* r = copy_string((const char*)src);
	last_copied_type_key = r;
	return (u8*)r;
}

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
		size = (f32)(*(i32*)udata);
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

	memset(&res_registry, 0, sizeof res_registry);
	memset(&res_cache, 0, sizeof res_cache);

	res_registry.hash = table_hash_string;
	res_registry.compare = table_compare_string;
	res_registry.free_key = table_free_string;
	res_registry.copy_key = res_registry_table_copy_string;

	res_cache.hash = table_hash_string;
	res_cache.compare = table_compare_string;
	res_cache.free_key = table_free_string;
	res_cache.copy_key = res_cache_table_copy_string;

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
		.udata_size = sizeof(i32),
		.free_raw_on_load = false,
		.terminate_raw = false,
		.alt_raw = bir_DejaVuSans_ttf,
		.alt_raw_size = bir_DejaVuSans_ttf_size,
		.on_load = ttf_on_load,
		.on_unload = ttf_on_unload
	});
}

void res_deinit() {
	for (const char** i = table_first(res_cache); i; i = table_next(res_cache, *i)) {
		struct res* res = table_get(res_cache, *i);

		struct res_config* config = table_get(res_registry, res->config_name);

		config->on_unload(res->payload, res->payload_size);

		core_free(res->payload);
	}

	free_table(res_cache);
	free_table(res_registry);
}

void reg_res_type(const char* type, const struct res_config* config) {
	table_set(res_registry, type, *config);
	struct res_config* c = table_get(res_registry, type);
	c->_name = last_copied_type_key;
}

static usize base64_size(usize size) {
	return 4 * ((size + 2) / 3);
}

static char encoding_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static i32 mod_table[] = {0, 2, 1};

/* From: https://stackoverflow.com/a/6782480/9549501 */
static void base64_encode(char* out, const u8* data, usize size) {
	usize os = base64_size(size);

	for (usize i = 0, j = 0; i < size;) {
		u32 octet_a = i < size ? (u8)data[i++] : 0;
		u32 octet_b = i < size ? (u8)data[i++] : 0;
		u32 octet_c = i < size ? (u8)data[i++] : 0;
		
		u32 triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
		
		out[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		out[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		out[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		out[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}
	
	for (int i = 0; i < mod_table[size % 3]; i++) {
		out[os - 1 - i] = '=';
	}
}

/* Get a unique ID buffer for each resource, based on the config type and the user data. This allows
 * the same filename to be used for different resource types and with different user data configs without
 * the system confusing them for each other in the resource cache. */
static char* get_resource_id_bytebuffer(const char* filename, const void* udata, const char* type, const struct res_config* config) {
	usize filename_len = strlen(filename);
	usize type_len = strlen(type);
	usize udata_len = base64_size(config->udata_size);

	char* buf = core_alloc(filename_len + type_len + udata_len + 1);
	memcpy(buf, filename, filename_len);
	memcpy(buf + filename_len, type, type_len);

	if (udata) {
		base64_encode(buf + filename_len + type_len, udata, config->udata_size);
	}

	buf[filename_len + udata_len + type_len] = '\0';

	return buf;
}

struct resource res_load(const char* type, const char* filename, void* udata) {
	struct res_config* config = table_get(res_registry, type);

	if (!config) {
		error("Loading `%s': No resource handler registered for this type of file (%s).", filename, type);
		return (struct resource) { null, null };
	}

	char* resource_id_buf = get_resource_id_bytebuffer(filename, udata, type, config);

	struct res* got = table_get(res_cache, resource_id_buf);
	if (got) {
		core_free(resource_id_buf);
		return (struct resource) { got->name, got->payload };
	}

	struct res new_res = {
		.payload = core_calloc(1, config->payload_size),
		.payload_size = config->payload_size,
		.config_name = config->_name
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
			raw = (u8*)config->alt_raw;
			raw_size = config->alt_raw_size;
		}
	}

	new_res.ok = !error;

	config->on_load(filename, raw, raw_size, new_res.payload, new_res.payload_size, udata);

	if (config->free_raw_on_load && !error) {
		core_free(raw);
	}

	table_set(res_cache, resource_id_buf, new_res);

	struct res* new_res_ptr = table_get(res_cache, resource_id_buf);
	new_res_ptr->name = last_copied_cache_key;

	core_free(resource_id_buf);

	return (struct resource) { new_res_ptr->name, new_res.payload };
}

void res_unload(const struct resource* r) {
	struct res* res = table_get(res_cache, r->key);
	if (!res) {
		error("Failed to unload resource.");
		return;
	}

	struct res_config* config = table_get(res_registry, res->config_name);

	config->on_unload(res->payload, res->payload_size);

	if (res->ok) {
		core_free(res->payload);
	}

	table_delete(res_cache, r->key);
}

struct texture* load_texture(const char* filename, u32 flags, struct resource* r) {
	struct resource temp;

	if (!r) {
		r = &temp;
	}

	*r = res_load("texture", filename, &flags);
	return r->payload;
}

struct font* load_font(const char* filename, i32 size, struct resource* r) {
	struct resource temp;

	if (!r) {
		r = &temp;
	}

	*r = res_load("true type font", filename, &size);
	return r->payload;
}

struct shader* load_shader(const char* filename, struct resource* r) {
	struct resource temp;

	if (!r) {
		r = &temp;
	}

	*r = res_load("shader", filename, null);
	return r->payload;
}
