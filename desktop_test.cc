#include <cassert>
#include <cstdio>
#include <ostream>

#include "desktop.h"
#include "desktops_def.h"
#include "prints.h"

using namespace vinput;

namespace {

class TestDesktop : public Desktop {
public:
	virtual bool ready() const noexcept override;
	virtual void key(Key k, PressAction a) override;
	virtual void button(Button b, PressAction a) override;
	virtual void pointer(PointerPosition pos) override;
	virtual PointerPosition pointer() const override;
	virtual void flush() override;

private:
	PointerPosition pointer_position = { 0, 0 };
	std::ostream *out_stream = &cout();
};

}

VINPUT_DESKTOP_CONNECTER(test) { return new TestDesktop; }

bool TestDesktop::ready() const noexcept {
	return true;
}

void TestDesktop::key(Key k, PressAction a) {
	char buffer[128];
	const auto action = a == PressAction::Press ? "press" : "release";
	const auto name = Desktop::key_to_name(k);
	const auto n = std::snprintf(
		buffer, sizeof buffer, "* %-8s %6s <%.*s>\n",
		action, "key", int(name.size()), name.data()
	);
	assert(n > 0);
	this->out_stream->write(buffer, std::size_t(n));
}

void TestDesktop::button(Button b, PressAction a) {
	char buffer[128];
	const auto action = a == PressAction::Press ? "press" : "release";
	const auto name = Desktop::button_to_name(b);
	const auto n = std::snprintf(
		buffer, sizeof buffer, "* %-8s %6s <%.*s>\n",
		action, "button", int(name.size()), name.data()
	);
	assert(n > 0);
	this->out_stream->write(buffer, std::size_t(n));
}

void TestDesktop::pointer(PointerPosition pos) {
	this->pointer_position = pos;

	char buffer[128];
	const auto n = std::snprintf(
		buffer, sizeof buffer, "* move pointer to (%u,%u)\n",
		pos.x, pos.y
	);
	assert(n > 0);
	this->out_stream->write(buffer, std::size_t(n));
}

TestDesktop::PointerPosition TestDesktop::pointer() const {
	return this->pointer_position;
}

void TestDesktop::flush() {
}
