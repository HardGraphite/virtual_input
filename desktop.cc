#include "desktop.h"

#include <cstdlib>
#include <cstring>

using namespace vinput;

static Desktop *desktop_instance = nullptr;

Desktop::Desktop() noexcept {
	if (desktop_instance)
		std::abort();
	desktop_instance = this;
}

Desktop::~Desktop() {
	if (desktop_instance != this)
		std::abort();
	desktop_instance = nullptr;
}

DesktopBaseError::DesktopBaseError(const char *name, const char *msg) noexcept {
	const auto name_len = std::strlen(name);
	const auto msg_len = std::strlen(msg);
	this->message = new char[name_len + msg_len + 3];
	char *p = this->message;
	std::memcpy(p, name, name_len);
	p += name_len;
	std::memcpy(p, ": ", 2);
	p += 2;
	std::memcpy(p, msg, msg_len);
	p += msg_len;
	*p = '\0';
}

DesktopBaseError::DesktopBaseError(DesktopBaseError &&other) noexcept
		: message(other.message) {
	other.message = nullptr;
}

DesktopBaseError::DesktopBaseError(const DesktopBaseError &other) noexcept {
	const auto size = std::strlen(other.message) + 1;
	this->message = new char[size];
	std::memcpy(this->message, other.message, size);
}

DesktopBaseError::~DesktopBaseError() {
	if (this->message)
		delete [] this->message;
}

DesktopBaseError &DesktopBaseError::operator=(
		DesktopBaseError &&other) noexcept {
	DesktopBaseError::~DesktopBaseError();
	new (this) DesktopBaseError(std::move(other));
	return *this;
}

DesktopBaseError &DesktopBaseError::operator=(
		const DesktopBaseError &other) noexcept {
	DesktopBaseError::~DesktopBaseError();
	new (this) DesktopBaseError(other);
	return *this;
}

const char *DesktopBaseError::what() const noexcept {
	return this->message;
}

DesktopUnavailabeError::DesktopUnavailabeError(const char *desktop_name) noexcept
		: DesktopBaseError(desktop_name, "not available") {
}
