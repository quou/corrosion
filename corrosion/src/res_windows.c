#include <Windows.h>

#include "core.h"
#include "res.h"

static u64 file_mod_time(const char* name) {
	HANDLE file = CreateFileA(name, GENERIC_READ, 0, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);

	if (file == INVALID_HANDLE_VALUE) {
		return 0;
	}

	FILETIME ft;

	if (!GetFileTime(file, null, null, &ft)) {
		return 0;
	}

	CloseHandle(file);

	LARGE_INTEGER date, adjust;
	date.HighPart = ft.dwHighDateTime;
	date.LowPart = ft.dwLowDateTime;

	adjust.QuadPart = 11644473600000 * 10000;

	date.QuadPart -= adjust.QuadPart;

	return date.QuadPart / 10000000;
}

bool get_file_info(const char* path, struct file_info* info) {
	DWORD attribs = GetFileAttributesA(path);
	if (attribs == INVALID_FILE_ATTRIBUTES) {
		info->type = file_other;
		return false;
	}

	u64 time = file_mod_time(path);

	if (!info) { return true; }

	info->mod_time = time;

	if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
		info->type = file_directory;
	} else if (attribs & FILE_ATTRIBUTE_REPARSE_POINT) {
		info->type = file_symlink;
	} else {
		/* TODO: Fix this. */
		info->type = file_normal;
	}

	return true;
}

struct dir_iter {
	char root[1024];

	HANDLE handle;

	WIN32_FIND_DATA ffd;

	struct dir_entry entry;
};

struct dir_iter* new_dir_iter(const char* dir_name) {
	struct dir_iter* iter = core_calloc(1, sizeof * iter);

	iter->entry.iter = iter;

	usize len = strlen(dir_name);

	strcat(iter->root, dir_name);

	if (iter->root[len - 1] != '/') {
		strcat(iter->root, "/");
	}

	return iter;
}

void free_dir_iter(struct dir_iter* it) {
	FindClose(it->handle);
	core_free(it);
}

struct dir_entry* dir_iter_cur(struct dir_iter* it) {
	return &it->entry;
}

static void dir_iter_info(struct dir_iter* it) {
	strcpy(it->entry.name, it->root);
	strcat(it->entry.name, it->ffd.cFileName);

	memset(&it->entry.info, 0, sizeof it->entry.info);
	get_file_info(it->entry.name, &it->entry.info);

	if (it->entry.info.type == file_directory) {
		strcat(it->entry.name, "/");
	}
}

bool dir_iter_next(struct dir_iter* it) {
	if (!it) { return false; }

	if (it->handle == 0) {
		strcat(it->root, "*");
		it->handle = FindFirstFileA(it->root, &it->ffd);
		if (!it->handle) { return false; }

		it->root[strlen(it->root) - 1] = '\0';

		if (strcmp(it->ffd.cFileName, ".") == 0) { return dir_iter_next(it); }
		if (strcmp(it->ffd.cFileName, "..") == 0) { return dir_iter_next(it); }

		dir_iter_info(it);
	} else {
		if (FindNextFileA(it->handle, &it->ffd)) {
			if (strcmp(it->ffd.cFileName, ".") == 0) { return dir_iter_next(it); }
			if (strcmp(it->ffd.cFileName, "..") == 0) { return dir_iter_next(it); }

			dir_iter_info(it);
			return true;
		}

		return false;
	}

	return it->handle != 0;
}