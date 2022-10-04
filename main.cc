#include <iostream>

#include "script.h"
#include "desktops.h"

int main() {
	vinput::Script script(std::cin);
	auto &desktop = vinput::connect_current_desktop();
	script.play(desktop);
	vinput::disconnect_desktop(desktop);
}
