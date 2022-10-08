#include <cassert>

#include "desktop.h"
#include "desktops_def.h"

#include <Windows.h>

using namespace vinput;

namespace {

class WindowsDesktop : public Desktop {
public:
	WindowsDesktop() noexcept;

	virtual bool ready() const noexcept override;
	virtual void key(Key k, PressAction a) override;
	virtual void button(Button b, PressAction a) override;
	virtual void pointer(PointerPosition pos) override;
	virtual PointerPosition pointer() const override;
	virtual void flush() override;

private:
	static constexpr auto KEY_COUNT = std::size_t(Key::_COUNT);
	static constexpr WORD WINVK_IS_CHAR = 0x8000;
	static constexpr WORD WINVK_WITH_SHIFT = 0x1000;
	static constexpr WORD WINVK_NOT_AVAILABLE = 0x0000;
	static constexpr DWORD WINSCAN_UNKNOWN = (DWORD)-1;
	static const WORD vk_map[KEY_COUNT];
	static const DWORD mb_flag_map[3][2];

	DWORD scancode_map[KEY_COUNT];

	void send_keyboard_input(Key k, bool down) noexcept;
	void send_mouse_button_input(Button b, bool down) noexcept;
	void send_mouse_wheel_input(Button b) noexcept;
	void send_mouse_move_input(PointerPosition pos) noexcept;
	PointerPosition get_cursor_pos() const noexcept;
};

}

VINPUT_DESKTOP_CONNECTER(windows) { return new WindowsDesktop; }

WindowsDesktop::WindowsDesktop() noexcept {
	for (auto& scan : this->scancode_map)
		scan = WINSCAN_UNKNOWN;
}

bool WindowsDesktop::ready() const noexcept {
	return true;
}

void WindowsDesktop::key(Key k, PressAction a) {
	this->send_keyboard_input(k, a == PressAction::Press);
}

void WindowsDesktop::button(Button b, PressAction a) {
	if (std::size_t(b) <= std::size_t(Button::RIGHT))
		this->send_mouse_button_input(b, a == PressAction::Press);
	else if (a == PressAction::Press)
		this->send_mouse_wheel_input(b);
}

void WindowsDesktop::pointer(PointerPosition pos) {
	this->send_mouse_move_input(pos);
}

WindowsDesktop::PointerPosition WindowsDesktop::pointer() const {
	return this->get_cursor_pos();
}

void WindowsDesktop::flush() {
}

const WORD WindowsDesktop::vk_map[KEY_COUNT] = {
	// 0 ~ 9
	0x30,
	0x31,
	0x32,
	0x33,
	0x34,
	0x35,
	0x36,
	0x37,
	0x38,
	0x39,

	// A ~ Z
	WINVK_WITH_SHIFT | 0x41,
	WINVK_WITH_SHIFT | 0x42,
	WINVK_WITH_SHIFT | 0x43,
	WINVK_WITH_SHIFT | 0x44,
	WINVK_WITH_SHIFT | 0x45,
	WINVK_WITH_SHIFT | 0x46,
	WINVK_WITH_SHIFT | 0x47,
	WINVK_WITH_SHIFT | 0x48,
	WINVK_WITH_SHIFT | 0x49,
	WINVK_WITH_SHIFT | 0x4a,
	WINVK_WITH_SHIFT | 0x4b,
	WINVK_WITH_SHIFT | 0x4c,
	WINVK_WITH_SHIFT | 0x4d,
	WINVK_WITH_SHIFT | 0x4e,
	WINVK_WITH_SHIFT | 0x4f,
	WINVK_WITH_SHIFT | 0x50,
	WINVK_WITH_SHIFT | 0x51,
	WINVK_WITH_SHIFT | 0x52,
	WINVK_WITH_SHIFT | 0x53,
	WINVK_WITH_SHIFT | 0x54,
	WINVK_WITH_SHIFT | 0x55,
	WINVK_WITH_SHIFT | 0x56,
	WINVK_WITH_SHIFT | 0x57,
	WINVK_WITH_SHIFT | 0x58,
	WINVK_WITH_SHIFT | 0x59,
	WINVK_WITH_SHIFT | 0x5a,

	// a ~ z
	0x41,
	0x42,
	0x43,
	0x44,
	0x45,
	0x46,
	0x47,
	0x48,
	0x49,
	0x4a,
	0x4b,
	0x4c,
	0x4d,
	0x4e,
	0x4f,
	0x50,
	0x51,
	0x52,
	0x53,
	0x54,
	0x55,
	0x56,
	0x57,
	0x58,
	0x59,
	0x5a,

	VK_SPACE,
	WINVK_IS_CHAR | '!',
	WINVK_IS_CHAR | '"',
	WINVK_IS_CHAR | '#',
	WINVK_IS_CHAR | '$',
	WINVK_IS_CHAR | '%',
	WINVK_IS_CHAR | '&',
	WINVK_IS_CHAR | '\'',
	WINVK_IS_CHAR | '(',
	WINVK_IS_CHAR | ')',
	VK_MULTIPLY,
	VK_ADD,
	WINVK_IS_CHAR | ',',
	VK_SUBTRACT,
	VK_DECIMAL,
	VK_DIVIDE,
	WINVK_IS_CHAR | ':',
	WINVK_IS_CHAR | ';',
	WINVK_IS_CHAR | '<',
	WINVK_IS_CHAR | '=',
	WINVK_IS_CHAR | '>',
	WINVK_IS_CHAR | '?',
	WINVK_IS_CHAR | '@',
	WINVK_IS_CHAR | '[',
	WINVK_IS_CHAR | '\\',
	WINVK_IS_CHAR | ']',
	WINVK_IS_CHAR | '^',
	WINVK_IS_CHAR | '_',
	WINVK_IS_CHAR | '`',
	WINVK_IS_CHAR | '{',
	WINVK_IS_CHAR | '|',
	WINVK_IS_CHAR | '}',
	WINVK_IS_CHAR | '~',

	VK_BACK,
	VK_TAB,
	VK_RETURN,
	VK_ESCAPE,
	VK_DELETE,

	VK_LCONTROL,
	VK_LSHIFT,
	WINVK_NOT_AVAILABLE,
	WINVK_NOT_AVAILABLE,
	VK_LWIN,

	VK_RCONTROL,
	VK_RSHIFT,
	WINVK_NOT_AVAILABLE,
	WINVK_NOT_AVAILABLE,
	VK_RWIN,
};

const DWORD WindowsDesktop::mb_flag_map[3][2] = {
	{ MOUSEEVENTF_LEFTDOWN,   MOUSEEVENTF_LEFTUP   },
	{ MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP },
	{ MOUSEEVENTF_RIGHTDOWN,  MOUSEEVENTF_RIGHTUP  },
};

void WindowsDesktop::send_keyboard_input(Key k, bool down) noexcept {
	if (static_cast<std::size_t>(k) >= KEY_COUNT) [[unlikely]]
		return;
	const auto vk_code = vk_map[static_cast<std::size_t>(k)];
	if (vk_code == WINVK_NOT_AVAILABLE) [[unlikely]]
		return;

	INPUT inputs[2];
	int inputs_n;
	ZeroMemory(inputs, sizeof inputs);

	if (vk_code & WINVK_IS_CHAR) {
		DWORD scancode_info = this->scancode_map[static_cast<std::size_t>(k)];
		if (scancode_info == WINSCAN_UNKNOWN) [[unlikely]] {
			scancode_info = OemKeyScan(vk_code & 0xff);
			if (scancode_info == WINSCAN_UNKNOWN)
				return;
			this->scancode_map[static_cast<std::size_t>(k)] = scancode_info;
		}
		const WORD scan = static_cast<WORD>(scancode_info & 0xff);
		const bool shift = scancode_info & (1 << 16);

		if (shift) {
			const DWORD flags = down ? 0 : KEYEVENTF_KEYUP;
			inputs_n = 2;
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = VK_LSHIFT;
			inputs[0].ki.dwFlags = flags;
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wScan = scan;
			inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | flags;
			if (!down)
				std::swap(inputs[0], inputs[1]);
		} else {
			inputs_n = 1;
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wScan = scan;
			inputs[0].ki.dwFlags =
				KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
		}
	} else {
		if (vk_code & WINVK_WITH_SHIFT) {
			const DWORD flags = down ? 0 : KEYEVENTF_KEYUP;
			inputs_n = 2;
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = VK_LSHIFT;
			inputs[0].ki.dwFlags = flags;
			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = vk_code & ~WINVK_WITH_SHIFT;
			inputs[1].ki.dwFlags = flags;
			if (!down)
				std::swap(inputs[0].ki.wVk, inputs[1].ki.wVk);
		}
		else {
			inputs_n = 1;
			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = vk_code;
			inputs[0].ki.dwFlags = (down ? 0 : KEYEVENTF_KEYUP);
		}
	}

	assert(inputs_n <= 2);
	SendInput(inputs_n, inputs, static_cast<int>(sizeof inputs[0]));
}

void WindowsDesktop::send_mouse_button_input(Button b, bool down) noexcept {
	assert(b == Button::LEFT || b == Button::MIDDLE || b == Button::RIGHT);
	INPUT input;
	ZeroMemory(&input, sizeof input);
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = mb_flag_map[std::size_t(b)][down ? 0 : 1];
	SendInput(1, &input, static_cast<int>(sizeof input));
}

void WindowsDesktop::send_mouse_wheel_input(Button b) noexcept {
	assert(b == Button::SCROLL_UP || b == Button::SCROLL_DOWN);
	INPUT input;
	ZeroMemory(&input, sizeof input);
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_WHEEL;
	input.mi.mouseData = b == Button::SCROLL_UP ? WHEEL_DELTA : -WHEEL_DELTA;
	SendInput(1, &input, static_cast<int>(sizeof input));
}

void WindowsDesktop::send_mouse_move_input(PointerPosition pos) noexcept {
	INPUT input;
	ZeroMemory(&input, sizeof input);
	input.type = INPUT_MOUSE;
	input.mi.dwFlags =
		MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK | MOUSEEVENTF_ABSOLUTE;
	input.mi.dx =
		static_cast<LONG>(pos.x * (65535.0 / GetSystemMetrics(SM_CXSCREEN)));
	input.mi.dy =
		static_cast<LONG>(pos.y * (65535.0 / GetSystemMetrics(SM_CYSCREEN)));
	SendInput(1, &input, static_cast<int>(sizeof input));
}

Desktop::PointerPosition WindowsDesktop::get_cursor_pos() const noexcept {
	POINT p;
	if (!GetCursorPos(&p)) [[unlikely]]
		return { 0, 0 };
	return { static_cast<unsigned int>(p.x), static_cast<unsigned int>(p.y) };
}
