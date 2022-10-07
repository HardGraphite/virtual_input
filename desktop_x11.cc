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
	virtual void key(Key k, PressAction a) override;
	virtual void button(Button b, PressAction a) override;
	virtual void pointer(PointerPosition pos) override;
	virtual PointerPosition pointer() const override;
	virtual void flush() override;

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

	static constexpr auto KEY_COUNT = std::size_t(Key::_COUNT);
	static constexpr auto BUTTON_COUNT = std::size_t(Button::_COUNT);
	static const KeySym keysym_map[KEY_COUNT];
	static const unsigned int button_map[BUTTON_COUNT];

	Display *display;
	Window root_window;
	KeyRepInfo keyrep_cache[KEY_COUNT];

	KeyRepInfo get_keyrep(Key key) noexcept;
	void send_fake_key_event(Key key, bool press);
	void send_fake_button_event(Button button, bool press);
	void send_fake_motion_event(int x, int y);
	PointerPosition query_pointer() const noexcept;
	void do_flush() noexcept;
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

void X11Desktop::key(Key k, PressAction a) {
	this->send_fake_key_event(k, a == PressAction::Press);
}

void X11Desktop::button(Button b, PressAction a) {
	this->send_fake_button_event(b, a == PressAction::Press);
}

void X11Desktop::pointer(PointerPosition pos) {
	this->send_fake_motion_event(int(pos.x), int(pos.y));
}

X11Desktop::PointerPosition X11Desktop::pointer() const {
	return this->query_pointer();
}

void X11Desktop::flush() {
	this->do_flush();
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

const unsigned int X11Desktop::button_map[BUTTON_COUNT] = {
	Button1,
	Button2,
	Button3,
	Button4,
	Button5,
};

X11Desktop::KeyRepInfo X11Desktop::get_keyrep(Key key) noexcept {
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

void X11Desktop::send_fake_key_event(Key key, bool press) {
	const auto key_rep = this->get_keyrep(key);
	assert(key_rep);

	Window focused_window;
	int focused_revert;
	XGetInputFocus(display, &focused_window, &focused_revert);

	XKeyEvent key_event;
	key_event.display = this->display;
	key_event.window = focused_window;
	key_event.root = this->root_window;
	key_event.subwindow = None;
	key_event.time = CurrentTime;
	key_event.x = 0;
	key_event.y = 0;
	key_event.x_root = 0;
	key_event.y_root = 0;
	key_event.same_screen = True;
	key_event.type = press ? KeyPress : KeyRelease;
	key_event.keycode = key_rep.key_code();
	key_event.state = key_rep.modifiers_mask();
	key_event.send_event = False;
	key_event.serial = 0;
	XSendEvent(
		this->display, focused_window,
		True, KeyPressMask | KeyReleaseMask,
		reinterpret_cast<XEvent *>(&key_event)
	);
}

void X11Desktop::send_fake_button_event(Button button, bool press) {
	assert(std::size_t(button) < X11Desktop::BUTTON_COUNT);
	const auto x_button = X11Desktop::button_map[std::size_t(button)];
	const int x_press = press ? True : False;
	XTestFakeButtonEvent(this->display, x_button, x_press, CurrentTime);
}

void X11Desktop::send_fake_motion_event(int x, int y) {
	XTestFakeMotionEvent(this->display, 0, x, y, CurrentTime);
}

X11Desktop::PointerPosition X11Desktop::query_pointer() const noexcept {
	Window root_win, child_win;
	int root_x, root_y, win_x, win_y;
	unsigned int mask;
	const auto ok = XQueryPointer(
		this->display, this->root_window,
		&root_win, &child_win, &root_x, &root_y, &win_x, &win_y, &mask
	);
	if (!ok)
		return {0, 0};
	return {static_cast<unsigned int>(root_x), static_cast<unsigned int>(root_y)};
}

void X11Desktop::do_flush() noexcept {
	XFlush(this->display);
}
