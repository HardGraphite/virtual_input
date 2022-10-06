#pragma once

#include <cstdint>

namespace vinput {

// Input event.
struct Event {
	enum Type : unsigned char {
		NOP,
		KEY_UP,
		KEY_DOWN,
		BUTTON_UP,
		BUTTON_DOWN,
		POINTER_GOTO,
	};

	enum class Key : unsigned char {
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

	enum class Button : unsigned char {
		LEFT, MIDDLE, RIGHT,
		SCROLL_UP, SCROLL_DOWN,

		_COUNT
	};

	struct PointerPosition {
		std::uint16_t x, y;
	};

	Type type;
	union {
		Key key;
		Button button;
		PointerPosition pointer_position;
	};
};

}
