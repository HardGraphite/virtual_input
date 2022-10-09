#include <climits>
#include <cstring>

#include "desktop.h"
#include "desktops_def.h"
#include "prints.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <unistd.h>

using namespace vinput;

namespace {

class LinuxUinputDesktop : public Desktop {
public:
	LinuxUinputDesktop();
	~LinuxUinputDesktop();

	LinuxUinputDesktop(LinuxUinputDesktop &&) = delete;
	LinuxUinputDesktop(const LinuxUinputDesktop &) = delete;
	LinuxUinputDesktop &operator=(LinuxUinputDesktop &&) = delete;
	LinuxUinputDesktop &operator=(const LinuxUinputDesktop &) = delete;

	virtual bool ready() const noexcept override;
	virtual void key(Key k, PressAction a) override;
	virtual void button(Button b, PressAction a) override;
	virtual void pointer(PointerPosition pos) override;
	virtual PointerPosition pointer() const override;
	virtual void flush() override;

private:
	static constexpr auto KEY_COUNT = std::size_t(Key::_COUNT);
	static constexpr auto BUTTON_COUNT = std::size_t(Button::_COUNT);
	static constexpr int KEY_NEED_SHIFT = 0x4000'0000;
	static const int key_code_map[KEY_COUNT];
	static const int btn_code_map[BUTTON_COUNT];

	static void create_uinput_keyboard_dev(int fd) noexcept;
	static void create_uinput_mouse_dev(int fd) noexcept;
	static void destroy_uinput_dev(int fd) noexcept;

	static void emit(int fd, int type, int code, int value) noexcept;
	static void event_syn_report(int fd) noexcept;

	int fd_keyboard, fd_mouse;

	void keyboard_key(Key k, bool press) noexcept;
	void mouse_button(Button b, bool press) noexcept;
	void mouse_wheel(Button b) noexcept;
	void mouse_goto(PointerPosition pos) noexcept;
};

}

VINPUT_DESKTOP_CONNECTER(linux) { return new LinuxUinputDesktop; }

LinuxUinputDesktop::LinuxUinputDesktop()
		: fd_keyboard(open("/dev/uinput", O_WRONLY | O_NONBLOCK))
		, fd_mouse(open("/dev/uinput", O_WRONLY | O_NONBLOCK)) {
	if (this->fd_keyboard == -1 || this->fd_mouse == -1) {
		if (this->fd_keyboard != -1)
			close(this->fd_keyboard);
		if (this->fd_mouse != -1)
			close(this->fd_mouse);
		throw DesktopUnavailabeError("linux");
	}

	create_uinput_keyboard_dev(this->fd_keyboard);
	create_uinput_mouse_dev(this->fd_mouse);
	usleep(500'000);
}

LinuxUinputDesktop::~LinuxUinputDesktop() {
	usleep(500'000);
	if (this->fd_keyboard >= 0) {
		destroy_uinput_dev(this->fd_keyboard);
		close(this->fd_keyboard);
	}
	if (this->fd_mouse >= 0) {
		destroy_uinput_dev(this->fd_mouse);
		close(this->fd_mouse);
	}
}

bool LinuxUinputDesktop::ready() const noexcept {
	return true;
}

void LinuxUinputDesktop::key(Key k, PressAction a) {
	this->keyboard_key(k, a == PressAction::Press);
}

void LinuxUinputDesktop::button(Button b, PressAction a) {
	if (static_cast<std::size_t>(b) <= static_cast<std::size_t>(Button::RIGHT))
		this->mouse_button(b, a == PressAction::Press);
	else
		this->mouse_wheel(b);
}

void LinuxUinputDesktop::pointer(PointerPosition pos) {
	this->mouse_goto(pos);
}

LinuxUinputDesktop::PointerPosition LinuxUinputDesktop::pointer() const {
	return { 0, 0 };
}

void LinuxUinputDesktop::flush() {
}

const int LinuxUinputDesktop::key_code_map[KEY_COUNT] = {
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,

	KEY_NEED_SHIFT | KEY_A,
	KEY_NEED_SHIFT | KEY_B,
	KEY_NEED_SHIFT | KEY_C,
	KEY_NEED_SHIFT | KEY_D,
	KEY_NEED_SHIFT | KEY_E,
	KEY_NEED_SHIFT | KEY_F,
	KEY_NEED_SHIFT | KEY_G,
	KEY_NEED_SHIFT | KEY_H,
	KEY_NEED_SHIFT | KEY_I,
	KEY_NEED_SHIFT | KEY_J,
	KEY_NEED_SHIFT | KEY_K,
	KEY_NEED_SHIFT | KEY_L,
	KEY_NEED_SHIFT | KEY_M,
	KEY_NEED_SHIFT | KEY_N,
	KEY_NEED_SHIFT | KEY_O,
	KEY_NEED_SHIFT | KEY_P,
	KEY_NEED_SHIFT | KEY_Q,
	KEY_NEED_SHIFT | KEY_R,
	KEY_NEED_SHIFT | KEY_S,
	KEY_NEED_SHIFT | KEY_T,
	KEY_NEED_SHIFT | KEY_U,
	KEY_NEED_SHIFT | KEY_V,
	KEY_NEED_SHIFT | KEY_W,
	KEY_NEED_SHIFT | KEY_X,
	KEY_NEED_SHIFT | KEY_Y,
	KEY_NEED_SHIFT | KEY_Z,

	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	KEY_SPACE,
	0 /* exclam */,
	0 /* quotedbl */,
	0 /* numbersign */,
	0 /* dollar */,
	0 /* percent */,
	0 /* ampersand */,
	KEY_APOSTROPHE,
	0 /* parenleft */,
	0 /* parenright */,
	0 /* asterisk */,
	0 /* plus */,
	KEY_COMMA,
	KEY_MINUS,
	KEY_DOT,
	KEY_SLASH,
	0 /* colon */,
	KEY_SEMICOLON,
	0 /* less */,
	KEY_EQUAL,
	0 /* greater */,
	0 /* question */,
	0 /* at */,
	0 /* bracketleft */,
	KEY_BACKSLASH,
	0 /* bracketright */,
	0 /* asciicircum */,
	0 /* underscore */,
	KEY_GRAVE,
	KEY_LEFTBRACE,
	0 /* bar */,
	KEY_RIGHTBRACE,
	0 /* asciitilde */,

	KEY_BACKSPACE,
	KEY_TAB,
	KEY_ENTER,
	KEY_ESC,
	KEY_DELETE,

	KEY_LEFTCTRL,
	KEY_LEFTSHIFT,
	KEY_LEFTALT,
	0 /* Meta_L */,
	0 /* Super_L */,

	KEY_RIGHTCTRL,
	KEY_RIGHTSHIFT,
	KEY_RIGHTALT,
	0 /* Meta_R */,
	0 /* Super_R */,
};

const int LinuxUinputDesktop::btn_code_map[BUTTON_COUNT] = {
	BTN_LEFT,
	BTN_MIDDLE,
	BTN_RIGHT,
	0,
	0,
};

void LinuxUinputDesktop::create_uinput_keyboard_dev(int fd) noexcept {
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	for (auto key_code : key_code_map) {
		if (key_code == 0)
			continue;
		ioctl(fd, UI_SET_KEYBIT, key_code);
	}

	struct uinput_setup usetup;
	bzero(&usetup, sizeof usetup);
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x4867;
	usetup.id.product = 0x5669;
	std::strcpy(usetup.name, "vinput-keyboard");
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);
}

void LinuxUinputDesktop::create_uinput_mouse_dev(int fd) noexcept {
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);

	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);
	ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

	struct uinput_setup usetup;
	bzero(&usetup, sizeof usetup);
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x4867;
	usetup.id.product = 0x566a;
	std::strcpy(usetup.name, "vinput-mouse");
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);
}

void LinuxUinputDesktop::destroy_uinput_dev(int fd) noexcept {
	ioctl(fd, UI_DEV_DESTROY);
}

void LinuxUinputDesktop::emit(int fd, int type, int code, int value) noexcept {
	struct input_event ie;
	ie.type = static_cast<decltype(ie.type)>(type);
	ie.code = static_cast<decltype(ie.code)>(code);
	ie.value = static_cast<decltype(ie.value)>(value);
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;
	write(fd, &ie, sizeof ie);
}

void LinuxUinputDesktop::event_syn_report(int fd) noexcept {
	emit(fd, EV_SYN, SYN_REPORT, 0);
}

void LinuxUinputDesktop::keyboard_key(Key k, bool press) noexcept {
	if (static_cast<std::size_t>(k) >= KEY_COUNT) [[unlikely]]
		return;
	const int key_code = key_code_map[static_cast<std::size_t>(k)];
	if (key_code == 0) [[unlikely]] {
		const auto key_name = key_to_name(k);
		print_warning("key <%.*s> not available", int(key_name.size()), key_name.data());
		return;
	}

	const auto fd = this->fd_keyboard;
	const int key_status = press ? 1 : 0;
	if (key_code & KEY_NEED_SHIFT) {
		const auto actual_key_code = key_code & ~KEY_NEED_SHIFT;
		emit(fd, EV_KEY, KEY_LEFTSHIFT, key_status);
		event_syn_report(fd);
		emit(fd, EV_KEY, actual_key_code, key_status);
		event_syn_report(fd);
	} else {
		emit(fd, EV_KEY, key_code, key_status);
		event_syn_report(fd);
	}
}

void LinuxUinputDesktop::mouse_button(Button b, bool press) noexcept {
	if (static_cast<std::size_t>(b) >= BUTTON_COUNT) [[unlikely]]
		return;
	const int btn_code = btn_code_map[static_cast<std::size_t>(b)];

	const auto fd = this->fd_keyboard;
	emit(fd, EV_KEY, btn_code, press ? 1 : 0);
	event_syn_report(fd);
}

void LinuxUinputDesktop::mouse_wheel(Button b) noexcept {
	const int distance = b == Button::SCROLL_UP ? 1 : -1;
	const auto fd = this->fd_mouse;
	emit(fd, EV_REL, REL_WHEEL, distance);
	event_syn_report(fd);
}

void LinuxUinputDesktop::mouse_goto(PointerPosition pos) noexcept {
	const auto fd = this->fd_mouse;
	emit(fd, EV_REL, REL_X, INT_MIN);
	emit(fd, EV_REL, REL_Y, INT_MIN);
	event_syn_report(fd);
	usleep(1'0000);
	emit(fd, EV_REL, REL_X, static_cast<int>(pos.x));
	emit(fd, EV_REL, REL_Y, static_cast<int>(pos.y));
	event_syn_report(fd);
}
