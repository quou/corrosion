#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "core.h"

cpp_compat void info(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vinfo(fmt, args);
	va_end(args);
}

cpp_compat void error(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	verror(fmt, args);
	va_end(args);
}

cpp_compat void warning(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vwarning(fmt, args);
	va_end(args);
}

/* Some Windows consoles don't support ANSI escape codes, so we use the WIN32 API
 * instead to colour the console output.
 *
 * Most terminal emulators on Linux do, however, so we don't bother checking if
 * it does. We don't do coloured output on Emscripten. */

cpp_compat void vinfo(const char* fmt, va_list args) {
#if defined(_WIN32)
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, FOREGROUND_GREEN);
		printf("info ");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(__EMSCRIPTEN__)
		printf("info: ");
#else
		printf("\033[31;32minfo \033[0m");
#endif
		vprintf(fmt, args);
		printf("\n");

}

cpp_compat void verror(const char* fmt, va_list args) {
#if defined(_WIN32)
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, FOREGROUND_RED);
		printf("error ");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(__EMSCRIPTEN__)
		printf("error: ");
#else
		printf("\033[31;31merror \033[0m");
#endif
		vprintf(fmt, args);
		printf("\n");

}

cpp_compat void vwarning(const char* fmt, va_list args) {
#if defined(_WIN32)
		HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_BLUE);
		printf("warning ");
		SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#elif defined(__EMSCRIPTEN__)
		printf("warning: ");
#else
		printf("\033[31;35mwarning \033[0m");
#endif
		vprintf(fmt, args);
		printf("\n");

}

cpp_compat void abort_with(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	verror(fmt, args);
	va_end(args);

	abort();
}

cpp_compat void vabort_with(const char* fmt, va_list args) {
	verror(fmt, args);

	abort();
}
