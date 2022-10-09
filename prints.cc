#include <cstdarg>
#include <cstdio>
#include <iostream>

#ifdef _WIN32
#	include <Windows.h>
#endif // _WIN32

#include "prints.h"

using namespace vinput;

std::ostream &vinput::cout() noexcept {
	return std::cout;
}

std::ostream &vinput::cerr() noexcept {
	return std::cerr;
}

void vinput::print_error(const std::exception &e) noexcept {
	auto &out = cerr();
	out << "vinput: error: " << e.what() << std::endl;
}

void vinput::print_warning(const char *fmt, ...) noexcept {
	char buffer[128];
	std::va_list ap;
	va_start(ap, fmt);
	const int n = std::vsnprintf(buffer, sizeof buffer, fmt, ap);
	va_end(ap);
	if (n <= 0) [[unlikely]]
		return;
	buffer[n] = '\n';

	auto &out = cerr();
	out << "vinput: warning: ";
	out.write(buffer, n + 1);
}

#ifdef _WIN32

void win32_enable_ansi_esc() noexcept {
	static bool initialized = false;
	if (initialized) [[likely]]
		return;
	initialized = true;

	const auto h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	if (!GetConsoleMode(h_stdout, &mode))
		return;
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(h_stdout, mode);
}

#endif // _WIN32
