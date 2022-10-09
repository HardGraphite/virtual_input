#pragma once

#include <exception>
#include <iosfwd>

namespace vinput {

std::ostream &cout() noexcept;
std::ostream &cerr() noexcept;

void print_error(const std::exception &e) noexcept;
void print_warning(const char *fmt, ...) noexcept;

#ifdef _WIN32

void win32_enable_ansi_esc() noexcept;

#endif // _WIN32

}
