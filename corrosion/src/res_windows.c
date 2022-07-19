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
	u64 time = file_mod_time(path);

	if (time == 0) {
		info->type = file_other;
		return false;
	}

	if (!info) { return true; }

	info->mod_time = time;

	DWORD attribs = GetFileAttributesA(path);

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
