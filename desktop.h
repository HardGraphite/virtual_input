#pragma once

#include <exception>

#include "event.h"

namespace vinput {

// Abstract interface of OS desktop input device operations.
class Desktop {
public:
	Desktop() noexcept;
	virtual ~Desktop();

	Desktop(const Desktop &) = delete;
	Desktop(Desktop &&) = delete;
	Desktop &operator=(const Desktop &) = delete;
	Desktop &operator=(Desktop &&) = delete;

	// Check whether the desktop is connected and ready for event handling.
	virtual bool ready() const noexcept = 0;
	// Send event to the desktop.
	virtual void send(Event event, bool flush = true) = 0;

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
