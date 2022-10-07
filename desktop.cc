#include "desktop.h"

#include <cstdlib>
#include <cstring>
#include <unordered_map>

using namespace vinput;

static Desktop *desktop_instance = nullptr;

#pragma pack(push, 1)

static const char *key_names[] = {
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",

	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",

	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",

	"SPACE",
	"EXCLAM",
	"QUOTATION",
	"NUMBERSIGN",
	"DOLLAR",
	"PERCENT",
	"AMPERSAND",
	"APOSTROPHE",
	"PARENLEFT",
	"PARENRIGHT",
	"ASTERISK",
	"PLUS",
	"COMMA",
	"MINUS",
	"PERIOD",
	"SLASH",
	"COLON",
	"SEMICOLON",
	"LESS",
	"EQUAL",
	"GREATER",
	"QUESTION",
	"AT",
	"BRACKETLEFT",
	"BACKSLASH",
	"BRACKETRIGHT",
	"ASCIICIRCUM",
	"UNDERSCORE",
	"GRAVE",
	"BRACELEFT",
	"BAR",
	"BRACERIGHT",
	"ASCIITILDE",

	"BACKSPACE",
	"TAB",
	"RETURN",
	"ESCAPE",
	"DELETE",
	"CONTROL_L",
	"SHIFT_L",
	"ALT_L",
	"META_L",
	"SUPER_L",
	"CONTROL_R",
	"SHIFT_R",
	"ALT_R",
	"META_R",
	"SUPER_R",
};

static const char *button_names[] = {
	"LEFT",
	"MIDDLE",
	"RIGHT",
	"SCROLL_UP",
	"SCROLL_DOWN",
};

#pragma pack(pop)

static std::unordered_map<std::string_view, Desktop::Key> key_name_map;
static std::unordered_map<std::string_view, Desktop::Button> button_name_map;

std::pair<Desktop::Key, bool>
Desktop::key_from_name(std::string_view name) noexcept {
	if (key_name_map.empty()) [[unlikely]] {
		key_name_map.reserve(std::size(key_names));
		for (std::size_t i = 0; i < std::size(key_names); i++)
			key_name_map.emplace(key_names[i], static_cast<Key>(i));
	}
	const auto iter = key_name_map.find(name);
	if (iter == key_name_map.end()) [[unlikely]]
		return {Key::_COUNT, false};
	return {iter->second, true};
}

std::string_view Desktop::key_to_name(Key key) noexcept {
	const auto index = static_cast<std::size_t>(key);
	if (index >= std::size(key_names)) [[unlikely]]
		return { };
	return key_names[index];
}

std::pair<Desktop::Button, bool>
Desktop::button_from_name(std::string_view name) noexcept {
	if (button_name_map.empty()) [[unlikely]] {
		button_name_map.reserve(std::size(button_names));
		for (std::size_t i = 0; i < std::size(button_names); i++)
			button_name_map.emplace(button_names[i], static_cast<Button>(i));
	}
	const auto iter = button_name_map.find(name);
	if (iter == button_name_map.end()) [[unlikely]]
		return {Button::_COUNT, false};
	return {iter->second, true};
}

std::string_view Desktop::button_to_name(Button button) noexcept {
	const auto index = static_cast<std::size_t>(button);
	if (index >= std::size(button_names)) [[unlikely]]
		return { };
	return button_names[index];
}

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
