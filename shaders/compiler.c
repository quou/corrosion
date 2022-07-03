#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <corrosion/common.h>

#define buffer_size 1024 * 1024

#define check_alloc(ptr_, s_) \
	do { \
		(ptr_) = malloc(s_); \
		if (!(ptr_)) { \
			fprintf(stderr, "Out of memory.\n"); \
			return 2; \
		} \
	} while (0)

#define fail_with(c_, ...) \
	do { \
		fprintf(stderr, ##__VA_ARGS__); \
		error_code = c_; \
		goto end; \
	} while (0)

#define execute(...) \
	do { \
		sprintf(buffer, ##__VA_ARGS__); \
		system(buffer); \
	} while (0)

void replace(char* str, char s, char d) {
	for (char* c = str; *c; c++) {
		if (*c == s) { *c = d; }
	}
}

char* copy_string(const char* str) {
	usize len = strlen(str);

	char* r = malloc(len + 1);

	r[len] = '\0';
	memcpy(r, str, len);

	return r;
}

#pragma pack(push, 1)
struct shader_header {
	char header[2];
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
};
#pragma pack(pop)

i32 main(i32 argc, const char** argv) {
	if (argc == 0) { return 1; }

	if (argc == 1) {
		fprintf(stderr, "Usage: %s file intdir outfile.\n", argv[0]);
		return 1;
	}

	const char* file_path    = argv[1];
	const char* int_dir_path = argv[2];
	const char* out_path     = argv[3];

	char* file_not_path = copy_string(file_path);
	replace(file_not_path, '/', '_');
	replace(file_not_path, '.', '_');

	FILE* file = fopen(file_path, "r");
	if (!file) {
		fprintf(stderr, "Failed to fopen %s.\n", file_path);
		return 3;
	}

	char int_v_file_path[1024] = "";
	strcat(int_v_file_path, int_dir_path);
	if (int_v_file_path[strlen(int_v_file_path) - 1] != '/') {
		strcat(int_v_file_path, "/");
	}
	strcat(int_v_file_path, file_not_path);
	strcat(int_v_file_path, ".vert");

	FILE* int_v_file = fopen(int_v_file_path, "w");
	if (!int_v_file) {
		fprintf(stderr, "Failed to fopen %s.\n", int_v_file_path);
		return 3;
	}

	char int_f_file_path[1024] = "";
	strcat(int_f_file_path, int_dir_path);
	if (int_f_file_path[strlen(int_f_file_path) - 1] != '/') {
		strcat(int_f_file_path, "/");
	}
	strcat(int_f_file_path, file_not_path);
	strcat(int_f_file_path, ".frag");

	FILE* int_f_file = fopen(int_f_file_path, "w");
	if (!int_f_file) {
		fprintf(stderr, "Failed to fopen %s.\n", int_f_file_path);
		return 3;
	}

	char* buffer;
	char* version_string;

	check_alloc(buffer, buffer_size);
	check_alloc(version_string, 256);
	version_string[0] = '\0';

	i32 error_code = 0;

	u32 line_number = 1;

	i32 adding_to = -1;

	while (fgets(buffer, buffer_size, file)) {
		usize line_len = strlen(buffer);
		if (buffer[line_len - 1] == '\n') {
			buffer[line_len - 1] = '\0';
		}

		const char* line = buffer;

		if (line_number == 1) {
			if (memcmp(line, "#version", 8) == 0) {
				memcpy(version_string, line, line_len);
			} else {
				fail_with(4, "error: First line must be #version\n");
			}
		} else {
			if (memcmp(line, "#begin", 6) == 0) {
				const char* token = line + 7;

				if (memcmp(token, "VERTEX", 6) == 0) {
					adding_to = 0;
					fprintf(int_v_file, "%s\n", version_string);
					fprintf(int_v_file, "#line %d \"%s\"\n", line_number + 1, file_path);
				} else if (memcmp(token, "FRAGMENT", 8) == 0) {
					adding_to = 1;
					fprintf(int_f_file, "%s\n", version_string);
					fprintf(int_f_file, "#line %d \"%s\"\n", line_number + 1, file_path);
				} else {
					fail_with(4, "error: line %d: Invalid #begin token.", line_number);
				}
			} else if (memcmp(line, "#end", 4) == 0) {
				adding_to = -1;
			} else {
				if (adding_to == 0) {
					fprintf(int_v_file, "%s\n", line);
				} else if (adding_to == 1) {
					fprintf(int_f_file, "%s\n", line);
				}
			}
		}

		line_number++;
	}

	fclose(int_v_file);
	fclose(int_f_file);

	char out_v_file_path[1024] = "";
	strcat(out_v_file_path, int_v_file_path);
	strcat(out_v_file_path, ".spv");
	char out_f_file_path[1024] = "";
	strcat(out_f_file_path, int_f_file_path);
	strcat(out_f_file_path, ".spv");

	execute("glslc --target-env=vulkan %s -o %s", int_v_file_path, out_v_file_path);
	execute("glslc --target-env=vulkan %s -o %s", int_f_file_path, out_f_file_path);

	FILE* out_v_file = fopen(out_v_file_path, "rb");
	if (!out_v_file) {
		fail_with(3, "Failed to fopen %s.\n", out_v_file_path);
	}

	FILE* out_f_file = fopen(out_f_file_path, "rb");
	if (!out_f_file) {
		fail_with(3, "Failed to fopen %s.\n", out_f_file_path);
	}

	fseek(out_v_file, 0, SEEK_END);
	u64 v_size = ftell(out_v_file);
	fseek(out_v_file, 0, SEEK_SET);

	fseek(out_f_file, 0, SEEK_END);
	u64 f_size = ftell(out_f_file);
	fseek(out_f_file, 0, SEEK_SET);

	FILE* out_file = fopen(out_path, "wb");
	if (!out_file) {
		fail_with(3, "Failed to fopen %s.\n", out_path);
	}

	struct shader_header header = {
		.header = "CS",
		.v_offset = sizeof(struct shader_header),
		.f_offset = sizeof(struct shader_header) + v_size,
		.v_size = v_size,
		.f_size = f_size
	};

	fwrite(&header, 1, sizeof(struct shader_header), out_file);

	for (usize i = 0; i < v_size; i += buffer_size) {
		usize read = fread(buffer, 1, v_size, out_v_file);
		fwrite(buffer, 1, read, out_file);
	}

	for (usize i = 0; i < f_size; i += buffer_size) {
		usize read = fread(buffer, 1, f_size, out_f_file);
		fwrite(buffer, 1, read, out_file);
	}

	fclose(out_v_file);
	fclose(out_f_file);
	fclose(out_file);

end:
	fclose(file);

	free(buffer);
	free(version_string);

	return error_code;
}
