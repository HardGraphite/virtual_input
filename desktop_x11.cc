#include <array>
#include <cassert>
#include <cstdint>

#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>

#include "desktop.h"
#include "desktops_def.h"

using namespace vinput;

namespace {

class X11Desktop : public Desktop {
public:
	X11Desktop();
	~X11Desktop();

	virtual bool ready() const noexcept override;
	virtual void send(Event event, bool flush) override;

private:
	class KeyRepInfo {
	public:
		constexpr KeyRepInfo() noexcept : data(0) { }
		KeyRepInfo(KeyCode c, unsigned int m) noexcept { assign(c, m); }
		operator bool() const noexcept { return data; }
		void assign(KeyCode key_code, unsigned int modifiers) noexcept;
		KeyCode key_code() const noexcept;
		unsigned int modifiers_mask() const noexcept;

	private:
		std::uint16_t data;
	};

	static constexpr auto KEY_COUNT = std::size_t(Event::Key::_COUNT);
	static constexpr auto BUTTON_COUNT = std::size_t(Event::Button::_COUNT);
	static const KeySym keysym_map[KEY_COUNT];
	static const int button_map[BUTTON_COUNT];

	Display *display;
	Window root_window;
	KeyRepInfo keyrep_cache[KEY_COUNT];

	KeyRepInfo get_keyrep(Event::Key key) noexcept;
	void send_key(Event event);
	void send_button(Event event);
	void send_pointer_position(Event event);
	void flush() noexcept;
};

}

VINPUT_DESKTOP_CONNECTER(x11) { return new X11Desktop; }

void X11Desktop::KeyRepInfo::assign(
		KeyCode key_code, unsigned int modifiers) noexcept {
	static_assert((ShiftMask | LockMask | ControlMask) <= 0b111);
	this->data = (std::uint16_t(key_code) << 3) | (modifiers & 0b111);
	assert(this->key_code() == key_code);
}

KeyCode X11Desktop::KeyRepInfo::key_code() const noexcept {
	return static_cast<KeyCode>(this->data >> 3);
}

unsigned int X11Desktop::KeyRepInfo::modifiers_mask() const noexcept {
	return this->data & 0b111;
}

X11Desktop::X11Desktop() {
	this->display = XOpenDisplay(nullptr);
	if (!this->display)
		throw DesktopBaseError("x11", "cannot connect to X11 server");

	this->root_window = XRootWindow(this->display, 0);
}

X11Desktop::~X11Desktop() {
	if (!this->display)
		return;
	XCloseDisplay(this->display);
	this->display = nullptr;
}

bool X11Desktop::ready() const noexcept {
	return this->display;
}

void X11Desktop::send(Event event, bool flush) {
	if (event.type == Event::KEY_UP || event.type == Event::KEY_DOWN)
		this->send_key(event);
	else if (event.type == Event::BUTTON_UP || event.type == Event::BUTTON_DOWN)
		this->send_button(event);
	else if (event.type == Event::POINTER_GOTO)
		this->send_pointer_position(event);
	if (flush)
		this->flush();
}

const KeySym X11Desktop::keysym_map[KEY_COUNT] = {
	XK_0,
	XK_1,
	XK_2,
	XK_3,
	XK_4,
	XK_5,
	XK_6,
	XK_7,
	XK_8,
	XK_9,

	XK_A,
	XK_B,
	XK_C,
	XK_D,
	XK_E,
	XK_F,
	XK_G,
	XK_H,
	XK_I,
	XK_J,
	XK_K,
	XK_L,
	XK_M,
	XK_N,
	XK_O,
	XK_P,
	XK_Q,
	XK_R,
	XK_S,
	XK_T,
	XK_U,
	XK_V,
	XK_W,
	XK_X,
	XK_Y,
	XK_Z,

	XK_a,
	XK_b,
	XK_c,
	XK_d,
	XK_e,
	XK_f,
	XK_g,
	XK_h,
	XK_i,
	XK_j,
	XK_k,
	XK_l,
	XK_m,
	XK_n,
	XK_o,
	XK_p,
	XK_q,
	XK_r,
	XK_s,
	XK_t,
	XK_u,
	XK_v,
	XK_w,
	XK_x,
	XK_y,
	XK_z,

	XK_space,
	XK_exclam,
	XK_quotedbl,
	XK_numbersign,
	XK_dollar,
	XK_percent,
	XK_ampersand,
	XK_apostrophe,
	XK_parenleft,
	XK_parenright,
	XK_asterisk,
	XK_plus,
	XK_comma,
	XK_minus,
	XK_period,
	XK_slash,
	XK_colon,
	XK_semicolon,
	XK_less,
	XK_equal,
	XK_greater,
	XK_question,
	XK_at,
	XK_bracketleft,
	XK_backslash,
	XK_bracketright,
	XK_asciicircum,
	XK_underscore,
	XK_grave,
	XK_braceleft,
	XK_bar,
	XK_braceright,
	XK_asciitilde,

	XK_BackSpace,
	XK_Tab,
	XK_Return,
	XK_Escape,
	XK_Delete,

	XK_Control_L,
	XK_Shift_L,
	XK_Alt_L,
	XK_Meta_L,
	XK_Super_L,

	XK_Control_R,
	XK_Shift_R,
	XK_Alt_R,
	XK_Meta_R,
	XK_Super_R,
};

const int X11Desktop::button_map[BUTTON_COUNT] = {
	Button1,
	Button2,
	Button3,
	Button4,
	Button5,
};

X11Desktop::KeyRepInfo X11Desktop::get_keyrep(Event::Key key) noexcept {
	const auto index = std::size_t(key);
	if (index >= X11Desktop::KEY_COUNT) [[unlikely]]
		return { };
	if (const auto rep = this->keyrep_cache[index]; rep)
		return rep;
	const auto key_sym  = keysym_map[index];
	const auto key_code = XKeysymToKeycode(this->display, key_sym);
	const auto key_mods =
		XkbKeycodeToKeysym(this->display, key_code, 0, 0) == key_sym ? 0 : ShiftMask;
	const KeyRepInfo rep(key_code, key_mods);
	this->keyrep_cache[index] = rep;
	return rep;
}

static void x11_send_key_event(
		Display *display, Window root_window,
		bool press, KeyCode key_code, unsigned int key_mods) noexcept {
	Window focused_window;
	int focused_revert;
	XGetInputFocus(display, &focused_window, &focused_revert);
	XKeyEvent key_event;
	key_event.display = display;
	key_event.window = focused_window;
	key_event.root = root_window;
	key_event.subwindow = None;
	key_event.time = CurrentTime;
	key_event.x = 0;
	key_event.y = 0;
	key_event.x_root = 0;
	key_event.y_root = 0;
	key_event.same_screen = True;
	key_event.type = press ? KeyPress : KeyRelease;
	key_event.keycode = key_code;
	key_event.state = key_mods;
	key_event.send_event = False;
	key_event.serial = 0;
	XSendEvent(
		display, focused_window,
		True, KeyPressMask | KeyReleaseMask,
		reinterpret_cast<XEvent *>(&key_event)
	);
}

void X11Desktop::send_key(Event event) {
	assert(event.type == Event::KEY_UP || event.type == Event::KEY_DOWN);
	const auto key_rep = this->get_keyrep(event.key);
	assert(key_rep);
	const bool key_down = event.type == Event::KEY_DOWN ? True : False;
	x11_send_key_event(this->display, this->root_window,
		key_down, key_rep.key_code(), key_rep.modifiers_mask());
}

static void x11_send_button_event(
		Display *display, bool press, int button) noexcept {
	XTestFakeButtonEvent(display, unsigned(button), int(press), CurrentTime);
}

void X11Desktop::send_button(Event event) {
	assert(event.type == Event::BUTTON_UP || event.type == Event::BUTTON_DOWN);
	const bool pressed = event.type == Event::BUTTON_DOWN;
	assert(std::size_t(event.button) < X11Desktop::BUTTON_COUNT);
	const auto button = X11Desktop::button_map[std::size_t(event.button)];
	x11_send_button_event(this->display, pressed, button);
}

static void x11_send_motion_event(
		Display *display, int screen, int x, int y) noexcept {
	XTestFakeMotionEvent(display, screen, x, y, CurrentTime);
}

void X11Desktop::send_pointer_position(Event event) {
	assert(event.type == Event::POINTER_GOTO);
	x11_send_motion_event(
		this->display, CurrentTime,
		event.pointer_position.x, event.pointer_position.y
	);
}

void X11Desktop::flush() noexcept {
	XFlush(this->display);
}
