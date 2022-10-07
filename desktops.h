#pragma once

namespace vinput {

class Desktop;

// Try to connect the desktop which is in use.
[[nodiscard]] Desktop *connect_current_desktop();

// Connect the testing desktop.
[[nodiscard]] Desktop *connect_test_desktop();

// Close the connection.
void disconnect_desktop(Desktop *desktop) noexcept;

}
