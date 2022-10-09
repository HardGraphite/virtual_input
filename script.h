#pragma once

#include <exception>
#include <iosfwd>

namespace vinput {

// Input action script.
class Script final {
public:
	static bool random_sleep; // Default: true
	static bool ignore_space; // Default: true

	static void print_doc(std::ostream &out) noexcept;

	Script() noexcept;
	Script(std::istream &source);
	Script(Script &&other) noexcept;
	Script(const Script &) = delete;
	~Script();

	Script &operator=(Script &&other) noexcept;
	Script &operator=(const Script &) = delete;

	bool empty() const noexcept;

	void append(std::istream &source);
	void clear() noexcept;

	void play(class Desktop &desktop) const;

private:
	struct Impl;

	Impl *_impl;
};

class ScriptSyntaxError : public std::exception {
public:
	enum Error {
		UNKNOWN_KEY,
		UNKNOWN_COMMAND,
		ILLEGAL_ARGUMENT,
	};

	ScriptSyntaxError(Error e) noexcept : error(e) { }

	virtual const char *what() const noexcept override;

private:
	Error error;
};

}
