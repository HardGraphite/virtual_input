#include "desktops.h"

#include "desktop.h"
#include "desktops_def.h"

using namespace vinput;

#if VINPUT_DESKTOP_WINDOWS
	VINPUT_DESKTOP_CONNECTER(windows);
#endif // VINPUT_DESKTOP_WINDOWS
#if VINPUT_DESKTOP_X11
	VINPUT_DESKTOP_CONNECTER(x11);
#endif // VINPUT_DESKTOP_X11

VINPUT_DESKTOP_CONNECTER(test);

static Desktop *(*const available_desktops[])() = {
#if VINPUT_DESKTOP_WINDOWS
	VINPUT_DESKTOP_CONNECTER_NAME(windows),
#endif // VINPUT_DESKTOP_WINDOWS
#if VINPUT_DESKTOP_X11
	VINPUT_DESKTOP_CONNECTER_NAME(x11),
#endif // VINPUT_DESKTOP_X11
};

[[nodiscard]] Desktop *vinput::connect_current_desktop() {
	Desktop *desktop;
	for (auto func : available_desktops) {
		try {
			desktop = func();
		} catch (const DesktopUnavailabeError &) {
			continue;
		}
		return desktop;
	}
	throw DesktopBaseError("vinput", "cannot find available desktop");
}

[[nodiscard]] Desktop *vinput::connect_test_desktop() {
	const auto desktop = VINPUT_DESKTOP_CONNECTER_NAME(test)();
	return desktop;
}

void vinput::disconnect_desktop(Desktop *desktop) noexcept {
	delete desktop;
}
