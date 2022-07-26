#pragma once

#include "common.h"
#include "maths.h"

/* Resource mananger. Maintains exactly one copy
 * of each resource. res_use_pak causes read_raw and
 * read_raw_text to read from the current package.
 * Packages only support reading and batch writing.
 * Packages are in the Quake PAK format.
 *
 * The resource manager is not thread safe. */

enum {
	file_normal = 0,
	file_directory,
	file_symlink,
	file_other
};

struct file_info {
	u64 mod_time;
	u8 type;
};

struct dir_iter;

struct dir_entry {
	char name[256];

	struct dir_iter* iter;

	struct file_info info;
};

struct dir_iter* new_dir_iter(const char* dir_name);
void free_dir_iter(struct dir_iter* it);
struct dir_entry* dir_iter_cur(struct dir_iter* it);
bool dir_iter_next(struct dir_iter* it);

bool get_file_info(const char* path, struct file_info* info);

bool read_raw(const char* path, u8** buf, usize* size);
bool write_raw(const char* path, const u8* buf, usize size);
bool read_raw_text(const char* path, char** buf);
bool write_raw_text(const char* path, const char* buf);

void res_init(const char* argv0);
void res_deinit();

struct res_pak;

void res_use_pak(const struct res_pak* pak);

struct res_pak* pak_open(const char* path);
void pak_close(struct res_pak* pak);

bool write_pak(const char* outname, const char** files, usize file_count);

struct res_config {
	usize payload_size;
	bool free_raw_on_load;
	bool terminate_raw;

	void (*on_load)(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata);
	void (*on_unload)(void* payload, usize payload_size);
};

void reg_res_type(const char* type, struct res_config* config);

/* Returns the resource payload. */
void* res_load(const char* type, const char* filename, void* udata);
void res_unload(const char* filename);

struct texture* load_texture(const char* filename, u32 flags);
struct font*    load_font(const char* filename, f32 size);
struct shader*  load_shader(const char* filename);

/* Primitive resources. */

/* Represents an image loaded from disk. 32 bits per-pixel, RGBA. */
struct image {
	v2i size;
	u8* colours;
};

void init_image_from_raw(struct image* image, const u8* raw, usize raw_size);
void deinit_image(struct image* image);
