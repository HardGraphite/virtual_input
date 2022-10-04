#pragma once

#define VINPUT_DESKTOP_CONNECTER_NAME(NAME) \
	__vinput_connect_ ##NAME ##_desktop

#define VINPUT_DESKTOP_CONNECTER(NAME) \
	extern "C" Desktop * VINPUT_DESKTOP_CONNECTER_NAME(NAME) ()
