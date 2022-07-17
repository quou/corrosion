#pragma once

#include "common.h"
#include "maths.h"

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

struct res_config {
	usize payload_size;
	bool free_raw_on_load;
	bool terminate_raw;

	void (*on_load)(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata);
	void (*on_unload)(void* payload, usize payload_size);
};

void reg_res_type(const char* type, struct res_config* config);

/* Returns the resource payload. */
void* res_load_p(const char* type, const char* filename, void* udata);
void res_unload(const char* filename);

#define res_load(t_, f_, v_) \
	res_load_p(t_, f_, lit_ptr(v_))

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
