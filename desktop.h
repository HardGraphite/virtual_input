#pragma once

#include <exception>

#include "event.h"

namespace vinput {

// Abstract interface of OS desktop input device operations.
class Desktop {
public:
	// Keyboard keys.
	enum class Key {
		_0, _1, _2, _3, _4, _5, _6, _7, _8, _9,
		A, B, C, D, E, F, G, H, I, J, K, L, M,
		N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
		a, b, c, d, e, f, g, h, i, j, k, l, m,
		n, o, p, q, r, s, t, u, v, w, x, y, z,
		SPACE, EXCLAM, QUOTATION, NUMBERSIGN, DOLLAR, PERCENT, AMPERSAND,
		APOSTROPHE, PARENLEFT, PARENRIGHT, ASTERISK, PLUS, COMMA, MINUS, PERIOD,
		SLASH, COLON, SEMICOLON, LESS, EQUAL, GREATER, QUESTION, AT,
		BRACKETLEFT, BACKSLASH, BRACKETRIGHT, ASCIICIRCUM, UNDERSCORE, GRAVE,
		BRACELEFT, BAR, BRACERIGHT, ASCIITILDE,
		BACKSPACE, TAB, RETURN, ESCAPE, DELETE,
		CONTROL_L, SHIFT_L, ALT_L, META_L, SUPER_L,
		CONTROL_R, SHIFT_R, ALT_R, META_R, SUPER_R,

		_COUNT
	};

	// Mouse buttons and wheel actions.
	enum class Button {
		LEFT, MIDDLE, RIGHT,
		SCROLL_UP, SCROLL_DOWN,

		_COUNT
	};

	// Press or release.
	enum class PressAction {
		Press,
		Release,
	};

	Desktop() noexcept;
	virtual ~Desktop();

	Desktop(const Desktop &) = delete;
	Desktop(Desktop &&) = delete;
	Desktop &operator=(const Desktop &) = delete;
	Desktop &operator=(Desktop &&) = delete;

	// Check whether the desktop is connected and ready for event handling.
	virtual bool ready() const noexcept = 0;
	// Send key event.
	virtual void key(Key k, PressAction a) = 0;
	// Send button event.
	virtual void button(Button b, PressAction a) = 0;
	// Send pointer movement event.
	virtual void pointer(unsigned int x, unsigned int y) = 0;
	// Immediately handle the events in the queue.
	virtual void flush() = 0;

	operator bool() const noexcept { return ready(); }
};

// Base desktop error.
class DesktopBaseError : std::exception {
public:
	DesktopBaseError(const char *desktop_name, const char *message) noexcept;
	DesktopBaseError(DesktopBaseError &&) noexcept;
	DesktopBaseError(const DesktopBaseError &) noexcept;
	virtual ~DesktopBaseError();

	DesktopBaseError &operator=(DesktopBaseError &&) noexcept;
	DesktopBaseError &operator=(const DesktopBaseError &) noexcept;

	virtual const char *what() const noexcept override;

protected:
	char *message;
};

// Error: desktop not available.
class DesktopUnavailabeError : DesktopBaseError {
public:
	DesktopUnavailabeError(const char *desktop_name) noexcept;
};

}
