#include <array>
#include <cassert>

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
	::Display *display;
	::Window root_window;

	void send_key(Event event);
	void flush() noexcept;
};

}

VINPUT_DESKTOP_CONNECTER(x11) { return new X11Desktop; }

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
	if (flush)
		this->flush();
}

static const ::KeySym keysym_map[] = {
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

static constexpr std::size_t event_key_count = std::size_t(Event::Key::_COUNT);
static_assert(event_key_count == std::size(keysym_map));

static std::pair<KeyCode, unsigned int> x11_key_sym_to_code_and_mods(
		Display *display, KeySym key_sym) noexcept {
	const auto key_code = XKeysymToKeycode(display, key_sym);
	const auto key_mods =
		XkbKeycodeToKeysym(display, key_code, 0, 0) == key_sym ? 0 : ShiftMask;
	return std::make_pair(key_code, key_mods);
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
	if (std::size_t(event.key) >= event_key_count) [[unlikely]]
		return;
	const auto key_sym = keysym_map[std::size_t(event.key)];
	const auto [key_code, key_mods] =
		x11_key_sym_to_code_and_mods(this->display, key_sym);
	const bool key_down = event.type == Event::KEY_DOWN ? True : False;
	x11_send_key_event(this->display, this->root_window, key_down, key_code, key_mods);
}

void X11Desktop::flush() noexcept {
	XFlush(this->display);
}
