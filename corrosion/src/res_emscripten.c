#include "core.h"
#include "res.h"

bool get_file_info(const char* path, struct file_info* info) {
	abort_with("Not implemented.");
}

struct dir_iter* new_dir_iter(const char* dir_name) {
	abort_with("Not implemented.");
}

void free_dir_iter(struct dir_iter* it) {
	abort_with("Not implemented.");
}

struct dir_entry* dir_iter_cur(struct dir_iter* it) {
	abort_with("Not implemented.");
}

bool dir_iter_next(struct dir_iter* it) {
	abort_with("Not implemented.");
}
