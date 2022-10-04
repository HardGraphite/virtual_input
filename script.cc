#include "script.h"

#include <cassert>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "desktop.h"
#include "event.h"

using namespace vinput;

struct Script::Impl {
	enum class Opcode : unsigned char {
		NOP,
		SLEEP_MS,
		SLEEP_SEC,
		KEY_UP,
		KEY_DOWN,
		KEY_PRESS,
		_COUNT
	};

	class Instruction final {
	public:
		Instruction(Opcode opcode, unsigned int operand) noexcept;
		Instruction &operator=(std::pair<Opcode, unsigned int> x) noexcept;
		Opcode opcode() const noexcept;
		unsigned int operand() const noexcept;

	private:
		std::uint16_t data;
	};

	class Compiler;
	class Player;

	std::vector<Instruction> code;
};

class Script::Impl::Compiler {
public:
	static void print_doc(std::ostream &out) noexcept;
	void operator()(std::istream &source, Script::Impl &script);

private:
	std::string string_buffer;
	std::vector<const char *> strarr_buffer;

	bool next_instr(std::istream &source, Script::Impl &script);
	void parse_command(std::istream &source, Script::Impl &script);

	void command_backslash(const std::vector<const char *> &args, Script::Impl &script);
	void command_sleep(const std::vector<const char *> &args, Script::Impl &script);
};

class Script::Impl::Player {
public:
	void operator()(const Script::Impl &script, Desktop &desktop);

private:
	static void sleep_ms(unsigned int time_ms) noexcept;
};

Script::Impl::Instruction::Instruction(Opcode opcode, unsigned int operand) noexcept {
	static_assert(std::size_t(Opcode::_COUNT) <= 16);
	this->data = std::uint16_t(static_cast<unsigned char>(opcode) | (operand << 4));
	assert(this->opcode() == opcode && this->operand() == operand);
}

Script::Impl::Instruction &
Script::Impl::Instruction::operator=(std::pair<Opcode, unsigned int> x) noexcept {
	new (this) Instruction(x.first, x.second);
	return *this;
}

Script::Impl::Opcode Script::Impl::Instruction::opcode() const noexcept {
	return static_cast<Opcode>(this->data & 0b1111);
}

unsigned int Script::Impl::Instruction::operand() const noexcept {
	return static_cast<unsigned int>(this->data >> 4);
}

void Script::Impl::Compiler::print_doc(std::ostream &out) noexcept {
	using namespace std::string_view_literals;
	const auto doc = R"%%(
script  = key | command ;
key     = ALPHA | DIGIT | PUNCT ;
command = "\" ( CMD_PUNCT | ( "[" CMD_PUNCT ":" ARG1 "," ... "]" ) ) ;
)%%"sv;
	out.write(doc.data(), doc.length());
}

void Script::Impl::Compiler::operator()(std::istream &source, Script::Impl &script) {
	while (this->next_instr(source, script));
}

bool Script::Impl::Compiler::next_instr(std::istream &source, Script::Impl &script) {
	auto &code = script.code;
	Event::Key key_code;

	switch (const auto ch = source.get(); static_cast<char>(ch)) {
	case '\t': key_code = Event::Key::TAB; break;
	case '\n': key_code = Event::Key::RETURN; break;
	case '\r': key_code = Event::Key::RETURN; break;
	case ' ': key_code = Event::Key::SPACE; break;
	case '!': key_code = Event::Key::EXCLAM; break;
	case '"': key_code = Event::Key::QUOTATION; break;
	case '#': key_code = Event::Key::NUMBERSIGN; break;
	case '$': key_code = Event::Key::DOLLAR; break;
	case '%': key_code = Event::Key::PERCENT; break;
	case '&': key_code = Event::Key::AMPERSAND; break;
	case '\'': key_code = Event::Key::APOSTROPHE; break;
	case '(': key_code = Event::Key::PARENLEFT; break;
	case ')': key_code = Event::Key::PARENRIGHT; break;
	case '*': key_code = Event::Key::ASTERISK; break;
	case '+': key_code = Event::Key::PLUS; break;
	case ',': key_code = Event::Key::COMMA; break;
	case '-': key_code = Event::Key::MINUS; break;
	case '.': key_code = Event::Key::PERIOD; break;
	case '/': key_code = Event::Key::SLASH; break;
	case '0': key_code = Event::Key::_0; break;
	case '1': key_code = Event::Key::_1; break;
	case '2': key_code = Event::Key::_2; break;
	case '3': key_code = Event::Key::_3; break;
	case '4': key_code = Event::Key::_4; break;
	case '5': key_code = Event::Key::_5; break;
	case '6': key_code = Event::Key::_6; break;
	case '7': key_code = Event::Key::_7; break;
	case '8': key_code = Event::Key::_8; break;
	case '9': key_code = Event::Key::_9; break;
	case ':': key_code = Event::Key::COLON; break;
	case ';': key_code = Event::Key::SEMICOLON; break;
	case '<': key_code = Event::Key::LESS; break;
	case '=': key_code = Event::Key::EQUAL; break;
	case '>': key_code = Event::Key::GREATER; break;
	case '?': key_code = Event::Key::QUESTION; break;
	case '@': key_code = Event::Key::AT; break;
	case 'A': key_code = Event::Key::A; break;
	case 'B': key_code = Event::Key::B; break;
	case 'C': key_code = Event::Key::C; break;
	case 'D': key_code = Event::Key::D; break;
	case 'E': key_code = Event::Key::E; break;
	case 'F': key_code = Event::Key::F; break;
	case 'G': key_code = Event::Key::G; break;
	case 'H': key_code = Event::Key::H; break;
	case 'I': key_code = Event::Key::I; break;
	case 'J': key_code = Event::Key::J; break;
	case 'K': key_code = Event::Key::K; break;
	case 'L': key_code = Event::Key::L; break;
	case 'M': key_code = Event::Key::M; break;
	case 'N': key_code = Event::Key::N; break;
	case 'O': key_code = Event::Key::O; break;
	case 'P': key_code = Event::Key::P; break;
	case 'Q': key_code = Event::Key::Q; break;
	case 'R': key_code = Event::Key::R; break;
	case 'S': key_code = Event::Key::S; break;
	case 'T': key_code = Event::Key::T; break;
	case 'U': key_code = Event::Key::U; break;
	case 'V': key_code = Event::Key::V; break;
	case 'W': key_code = Event::Key::W; break;
	case 'X': key_code = Event::Key::X; break;
	case 'Y': key_code = Event::Key::Y; break;
	case 'Z': key_code = Event::Key::Z; break;
	case '[': key_code = Event::Key::BRACKETLEFT; break;
	case ']': key_code = Event::Key::BRACKETRIGHT; break;
	case '^': key_code = Event::Key::ASCIICIRCUM; break;
	case '_': key_code = Event::Key::UNDERSCORE; break;
	case '`': key_code = Event::Key::GRAVE; break;
	case 'a': key_code = Event::Key::a; break;
	case 'b': key_code = Event::Key::b; break;
	case 'c': key_code = Event::Key::c; break;
	case 'd': key_code = Event::Key::d; break;
	case 'e': key_code = Event::Key::e; break;
	case 'f': key_code = Event::Key::f; break;
	case 'g': key_code = Event::Key::g; break;
	case 'h': key_code = Event::Key::h; break;
	case 'i': key_code = Event::Key::i; break;
	case 'j': key_code = Event::Key::j; break;
	case 'k': key_code = Event::Key::k; break;
	case 'l': key_code = Event::Key::l; break;
	case 'm': key_code = Event::Key::m; break;
	case 'n': key_code = Event::Key::n; break;
	case 'o': key_code = Event::Key::o; break;
	case 'p': key_code = Event::Key::p; break;
	case 'q': key_code = Event::Key::q; break;
	case 'r': key_code = Event::Key::r; break;
	case 's': key_code = Event::Key::s; break;
	case 't': key_code = Event::Key::t; break;
	case 'u': key_code = Event::Key::u; break;
	case 'v': key_code = Event::Key::v; break;
	case 'w': key_code = Event::Key::w; break;
	case 'x': key_code = Event::Key::x; break;
	case 'y': key_code = Event::Key::y; break;
	case 'z': key_code = Event::Key::z; break;
	case '{': key_code = Event::Key::BRACELEFT; break;
	case '|': key_code = Event::Key::BAR; break;
	case '}': key_code = Event::Key::BRACERIGHT; break;
	case '~': key_code = Event::Key::ASCIITILDE; break;
	case 0x7f: key_code = Event::Key::BACKSPACE; break;

	case '\\':
		this->parse_command(source, script);
		return true;

	[[unlikely]] default:
		if (ch == std::istream::traits_type::eof())
			return false;
		throw ScriptSyntaxError();
	}

	code.emplace_back(Impl::Opcode::KEY_PRESS, unsigned(key_code));
	return true;
}

void Script::Impl::Compiler::parse_command(
		std::istream &source, Script::Impl &script) {
	const auto c0 = source.get();
	const bool has_args = c0 == '[';
	const char command = has_args ? source.get() : c0;

	if (command == std::istream::traits_type::eof())
		throw ScriptSyntaxError();

	using command_func_t =
		void (Compiler::*)(const std::vector<const char *> &, Script::Impl &);
	command_func_t command_func;
	switch (command) {
	case '#': command_func = &Compiler::command_sleep; break;
	default: throw ScriptSyntaxError();
	}

	auto &args = this->strarr_buffer;
	args.clear();
	if (has_args) {
		auto &argstr = this->string_buffer;
		std::getline(source, argstr, ']');
		for (std::size_t off = 0; ; ) {
			const auto pos = argstr.find(',', off);
			args.emplace_back(argstr.c_str() + off);
			if (pos == argstr.npos)
				break;
			argstr.data()[pos] = '\0';
			off = pos + 1;
		}
	}

	(this->*command_func)(args, script);
}

void Script::Impl::Compiler::command_backslash(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::KEY_PRESS, unsigned(Event::Key::BACKSLASH));
}

void Script::Impl::Compiler::command_sleep(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (args.size() != 1)
		throw ScriptSyntaxError();
	const auto time = std::atof(args[0]);
	if (time < 0.001)
		return;
	double i, f;
	f = std::modf(time, &i);
	while (true) {
		if (i > 4096) {
			script.code.emplace_back(Opcode::SLEEP_SEC, 4096);
			i -= 4096;
		} else {
			if (i)
				script.code.emplace_back(Opcode::SLEEP_SEC, unsigned(i));
			break;
		}
	}
	if (f) {
		script.code.emplace_back(Opcode::SLEEP_SEC, unsigned(f * 1e3));
	}
}

void Script::Impl::Player::operator()(const Script::Impl &script, Desktop &desktop) {
	const auto *code_pointer = script.code.data();
	const auto *const code_end = code_pointer + script.code.size();
	while (code_pointer < code_end) {
		const auto instruction = *code_pointer++;
		const auto operand = instruction.operand();

		switch (instruction.opcode()) {
			using enum Impl::Opcode;

		case NOP:
			break;

		case SLEEP_MS:
			this->sleep_ms(operand);
			break;

		case SLEEP_SEC:
			this->sleep_ms(operand * 1000);
			break;

		case KEY_UP:
			desktop.send({
				.type = Event::KEY_UP,
				.key = static_cast<Event::Key>(operand),
			});
			break;

		case KEY_DOWN:
			desktop.send({
				.type = Event::KEY_DOWN,
				.key = static_cast<Event::Key>(operand),
			});
			break;

		case KEY_PRESS:
			desktop.send({
				.type = Event::KEY_DOWN,
				.key = static_cast<Event::Key>(operand),
			}, false);
			desktop.send({
				.type = Event::KEY_UP,
				.key = static_cast<Event::Key>(operand),
			}, true);
			break;

		[[unlikely]] default:
			break;
		}

		this->sleep_ms(100);
	}
}

void Script::Impl::Player::sleep_ms(unsigned int time_ms) noexcept {
	std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
}

Script::Script() noexcept : _impl(new Impl) {
}

Script::Script(std::istream &source) : Script() {
	this->append(source);
}

Script::~Script() {
	delete this->_impl;
}

void Script::print_doc(std::ostream &out) noexcept {
	Impl::Compiler::print_doc(out);
}

void Script::append(std::istream &source) {
	Impl::Compiler compiler;
	compiler(source, *this->_impl);
}

void Script::clear() noexcept {
	auto &code = this->_impl->code;
	code.clear();
}

void Script::play(Desktop &desktop) const {
	Impl::Player player;
	player(*this->_impl, desktop);
}
