#include <sys/stat.h>
#include <dirent.h>

#include "core.h"
#include "res.h"

bool get_file_info(const char* path, struct file_info* info) {
	struct stat s;
	if (stat(path, &s) != 0) {
		if (info) { info->type = file_other; }
		return false;
	}

	if (!info) { return true; }

	info->mod_time = s.st_mtim.tv_sec;

	if (S_ISDIR(s.st_mode)) {
		info->type = file_directory;
	} else if (S_ISREG(s.st_mode)) {
		info->type = file_normal;
	} else if (S_ISLNK(s.st_mode)) {
		info->type = file_symlink;
	} else {
		info->type = file_other;
	};

	return true;
}

struct dir_iter {
	char root[1024];

	DIR* dir;

	struct dir_entry entry;
};

struct dir_iter* new_dir_iter(const char* dir_name) {
	DIR* dir = opendir(dir_name);

	if (!dir) {
		error("Failed to open directory: `%s'", dir_name);
		return null;
	}

	struct dir_iter* it = core_calloc(1, sizeof(struct dir_iter));

	strcpy(it->root, dir_name);
	it->dir = dir;

	dir_iter_next(it);

	return it;
}

void free_dir_iter(struct dir_iter* it) {
	closedir(it->dir);
	core_free(it);
}

struct dir_entry* dir_iter_cur(struct dir_iter* it) {
	return &it->entry;
}

bool dir_iter_next(struct dir_iter* it) {
	struct dirent* e = readdir(it->dir);

	if (!e) { return false; }

	if (strcmp(e->d_name, ".")  == 0) { return dir_iter_next(it); }
	if (strcmp(e->d_name, "..") == 0) { return dir_iter_next(it); }

	strcpy(it->entry.name, it->root);

	if (it->entry.name[strlen(it->entry.name) - 1] != '/') {
		strcat(it->entry.name, "/");
	}

	strcat(it->entry.name, e->d_name);

	it->entry.iter = it;

	if (!get_file_info(it->entry.name, &it->entry.info)) {
		error("Failed to stat: %s", it->entry.name);
	}

	return true;
}

bool create_dir(const char* name) {
	i32 r = mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	return r == 0;
}
