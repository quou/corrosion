#include <stdio.h>
#include <string.h>

#include "common.h"

static const char* out_file = "corrosion/src/bir.h";
static const char* dir = "corrosion/bir";
static const char* files[] = {
	"DejaVuSans.ttf",
	"checkerboard.png",
	"error.png",
	"error.csh",
	"line.csh",
	"simple.csh",
	"ui.csh",
};

i32 main() {
	char buffer[2048];

	FILE* out = fopen(out_file, "w");
	if (!out) {
		fprintf(stderr, "Failed to fopen `%s'\n", out_file);
		return 1;
	}

	fprintf(out, "#pragma once\n\n");
	fprintf(out, "#include \"core.h\"\n\n");

	fprintf(out, "#ifndef bir_impl\n");

	for (usize i = 0; i < sizeof files / sizeof * files; i++) {
		char var_name[256];
		strcpy(var_name, "bir_");
		strcat(var_name, files[i]);

		for (char* c = var_name; *c; c++) {
			if (*c == '/' || *c == '.') {
				*c = '_';
			}
		}

		fprintf(out, "extern const char %s[];\n", var_name);
		strcat(var_name, "_size");
		fprintf(out, "extern const usize %s;\n", var_name);
	}

	fprintf(out, "#else\n");

	for (usize i = 0; i < sizeof files / sizeof *files; i++) {
		char var_name[256];
		strcpy(var_name, "bir_");
		strcat(var_name, files[i]);

		for (char* c = var_name; *c; c++) {
			if (*c == '/' || *c == '.') {
				*c = '_';
			}
		}

		fprintf(out, "const char %s[] = {\n", var_name);

		char file_name[256];
		strcpy(file_name, dir);
		strcat(file_name, "/");
		strcat(file_name, files[i]);

		FILE* infile = fopen(file_name, "rb");
		if (!infile) {
			fprintf(stderr, "Failed to fopen `%s'\n", file_name);
			return 1;
		}

		fseek(infile, 0, SEEK_END);
		usize file_size = ftell(infile);
		rewind(infile);

		for (usize j = 0; j < file_size;) {
			usize read = fread(buffer, 1, sizeof buffer, infile);

			for (usize x = 0; x < read; x++) {
				fprintf(out, "0x%02x,", (u8)buffer[x]);
			}

			j += read;
		}
		
		fprintf(out, "\n};\n");

		strcat(var_name, "_size");
		fprintf(out, "const usize %s = %llu;\n", var_name, file_size);

		fclose(infile);
	}

	fprintf(out, "#endif\n");

	fclose(out);
}
