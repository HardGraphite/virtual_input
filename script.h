#pragma once

#include <exception>
#include <iosfwd>

namespace vinput {

// Input action script.
class Script {
public:
	static bool random_sleep; // Default: true

	static void print_doc(std::ostream &out) noexcept;

	Script() noexcept;
	Script(std::istream &source);
	Script(Script &&) = delete;
	~Script();

	Script(const Script &) = delete;
	Script &operator=(Script &&) = delete;
	Script &operator=(const Script &) = delete;

	bool empty() const noexcept;

	void append(std::istream &source);
	void clear() noexcept;

	void play(class Desktop &desktop) const;

private:
	struct Impl;

	Impl *_impl;
};

class ScriptSyntaxError : std::exception {
};

}
