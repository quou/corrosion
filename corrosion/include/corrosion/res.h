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

/* Helper macro to iterate a directory. Usage example:
 *     for_dir("res", i, {
 *         info("%s", i->name);
 *     });
 * Using break to exit the loop will cause a memory leak. */
#define for_dir(n_, v_, c_) \
	do { \
		struct dir_iter* cat(v_, __LINE__) = new_dir_iter(n_); \
		while (dir_iter_next(cat(v_, __LINE__))) { \
			struct dir_entry* v_ = dir_iter_cur(cat(v_, __LINE__)); \
			c_ \
		} \
		free_dir_iter(cat(v_, __LINE__)); \
	} while (0)

bool get_file_info(const char* path, struct file_info* info);

/* If a PAK archive is currently
 * bound, read_raw and read_raw_text will read from the currently
 * bound PAK archive. Otherwise, they read from a file.
 */
bool read_raw(const char* path, u8** buf, usize* size);
bool write_raw(const char* path, const u8* buf, usize size);
bool read_raw_text(const char* path, char** buf);
bool write_raw_text(const char* path, const char* buf);

void res_init(const char* argv0);
void res_deinit();

/* PAK archive read and writer. Use the res_use_pak function to
 * bind a PAK archive once opened. If a PAK archive is currently
 * bound, read_raw and read_raw_text will read from the currently
 * bound PAK archive. Otherwise, they read from a file.
 */
struct res_pak;

void res_use_pak(const struct res_pak* pak);

struct res_pak* pak_open(const char* path);
void pak_close(struct res_pak* pak);

struct pak_write_file {
	const char* src;
	const char* dst;
};

bool write_pak(const char* outname, struct pak_write_file* files, usize file_count);

struct res_config {
	usize payload_size;

	/* The user data is included in the ID of a resource so that
	 * resources loaded with different user data are not confused
	 * in the cache. This behaviour can be effectively disabled by
	 * setting this parameter to zero. */
	usize udata_size;

	/* If true, frees the raw data immediately after on_load
	 * has executed. Otherwise, the raw data is kept alive for
	 * the entire lifetime of the resource. If false, the user
	 * is responsible for freeing the raw data when she sees
	 * fit. */
	bool free_raw_on_load;

	/* If true, adds a null terminator to the end of the
	 * data when loading from a file. Useful when loading
	 * textual data. */
	bool terminate_raw;

	/* alt_raw and alt_raw_size are used if loading the file
	 * failed. This is useful for adding error resources to
	 * avoid simply crashing if a file is deleted by accident.
	 */
	const void* alt_raw;
	usize alt_raw_size;

	void (*on_load)(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata);
	void (*on_unload)(void* payload, usize payload_size);
};

struct resource {
	u64 id;
	void* payload;
};

void reg_res_type(const char* type, const struct res_config* config);

/* Returns the resource payload. */
struct resource res_load(const char* type, const char* filename, void* udata);
void res_unload(const struct resource* r);

struct texture* load_texture(const char* filename, u32 flags, struct resource* r);
struct font*    load_font(const char* filename, i32 size, struct resource* r);
struct shader*  load_shader(const char* filename, struct resource* r);

/* Primitive resources. */

/* Represents an image loaded from disk. 32 bits per-pixel, RGBA. */
struct image {
	v2i size;
	u8* colours;
};

void init_image_from_raw(struct image* image, const u8* raw, usize raw_size);
void deinit_image(struct image* image);

void flip_image_y(struct image* image);
