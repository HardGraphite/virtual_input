#include "script.h"

#include <cassert>
#include <chrono>
#include <cctype>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <istream>
#include <ostream>
#include <random>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "desktop.h"

using namespace vinput;

struct Script::Impl {
	enum class Opcode : unsigned char {
		SLEEP_MS,
		SLEEP_SEC,
		KEY_UP,
		KEY_DOWN,
		KEY_CLICK,
		BUTTON_UP,
		BUTTON_DOWN,
		BUTTON_CLICK,
		POINTER_GOTO,
		POINTER_WHERE,
		LOOP_BEGIN,
		LOOP_END,
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
	std::vector<std::pair<unsigned int, unsigned int>> positions;
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
	void command_enter(const std::vector<const char *> &args, Script::Impl &script);
	void command_tab(const std::vector<const char *> &args, Script::Impl &script);
	void command_space(const std::vector<const char *> &args, Script::Impl &script);
	void command_sleep(const std::vector<const char *> &args, Script::Impl &script);
	void command_click_left(const std::vector<const char *> &args, Script::Impl &script);
	void command_click_middle(const std::vector<const char *> &args, Script::Impl &script);
	void command_click_right(const std::vector<const char *> &args, Script::Impl &script);
	void command_move_pointer(const std::vector<const char *> &args, Script::Impl &script);
	void command_find_pointer(const std::vector<const char *> &args, Script::Impl &script);
	void command_begin_loop(const std::vector<const char *> &args, Script::Impl &script);
	void command_end_loop(const std::vector<const char *> &args, Script::Impl &script);
	void command_send_key(const std::vector<const char *> &args, Script::Impl &script);
	void command_send_button(const std::vector<const char *> &args, Script::Impl &script);
};

class Script::Impl::Player {
public:
	Player() noexcept;
	Player(const Player &) = delete;
	Player(Player &&) = delete;
	~Player();
	Player &operator=(const Player &) = delete;
	Player &operator=(Player &&) = delete;

	void random_sleep(bool status) noexcept;

	void operator()(const Script::Impl &script, Desktop &desktop);

private:
	class StopToken {
	public:
		void set() noexcept { state = true; }
		void clear() noexcept { state = false; }
		bool test() const noexcept { return state; }

	private:
		bool state = false;
	};

	struct Random {
		std::mt19937 rand_gen;
		std::normal_distribution<double> norm_dist;
	};

	struct LoopBlock {
		const Instruction *begin;
		std::size_t times;
	};

	static StopToken stop_token;

	Random *random;
	std::vector<LoopBlock> loops;

	void sleep_ms(unsigned int time_ms) noexcept;
	void print_pointer(const Desktop &desktop, unsigned int flags) noexcept;
};

Script::Impl::Instruction::Instruction(Opcode opcode, unsigned int operand) noexcept {
	static_assert(std::size_t(Opcode::_COUNT) <= 16);
	this->data = std::uint16_t(static_cast<unsigned char>(opcode) | (operand << 4));
	assert(this->opcode() == opcode);
	assert(this->operand() == operand);
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
	const auto doc = R"%%((* vinput script *)
script = { key | command } ;
key    = ALPHA | DIGIT | PUNCT ;
command
	= "\\"  (* backslash key *)
	| "\n" | "\r"  (* enter key *)
	| "\t"  (* tab key *)
	| "\s" (* space key *)
	| "\#" | ("\[#" FLOAT "]")  (* sleep for 1 or FLOAT seconds *)
	| "\<"  (* left click *)
	| "\|" | "\[|^]" | "\[|v]"  (* middle click / scroll up / scroll down *)
	| "\>"  (* right click *)
	| "\[@" INT "," INT "]"  (* move pointer to the coordinate *)
	| "\?" | "\[?!]"  (* get pointer coordinate and print / print without LF *)
	| "\{" | "\[{" INT "]"  (* begin loop forever / INT times (INT <= 0 means forever) *)
	| "\}"  (* end loop *)
	| "\[$" KEY_NAME [ "," "v" | "^" ] "]"  (* click / press / release key *)
	| "\[%" BUTTON_NAME [ "," "v" | "^" ] "]"  (* click / press / release button *)
	;
)%%"sv;
	out.write(doc.data(), doc.length());
	out.write("KEY_NAME\n", 9);
	for (std::size_t i = 0; i < std::size_t(Desktop::Key::_COUNT); i++) {
		const auto name = Desktop::key_to_name(static_cast<Desktop::Key>(i));
		out.write(i ? "\t|" : "\t=", 2);
		out.write(" \"", 2);
		out.write(name.data(), name.size());
		out.write("\"\n", 2);
	}
	out.write("\t;\n", 3);
	out.write("BUTTON_NAME\n", 12);
	for (std::size_t i = 0; i < std::size_t(Desktop::Button::_COUNT); i++) {
		const auto name = Desktop::button_to_name(static_cast<Desktop::Button>(i));
		out.write(i ? "\t|" : "\t=", 2);
		out.write(" \"", 2);
		out.write(name.data(), name.size());
		out.write("\"\n", 2);
	}
	out.write("\t;\n", 3);
}

void Script::Impl::Compiler::operator()(std::istream &source, Script::Impl &script) {
	while (this->next_instr(source, script));
}

bool Script::Impl::Compiler::next_instr(std::istream &source, Script::Impl &script) {
	auto &code = script.code;
	Desktop::Key key_code;

	switch (const auto ch = source.get(); static_cast<char>(ch)) {
	case '\t': if (ignore_space) return true; key_code = Desktop::Key::TAB; break;
	case '\n': if (ignore_space) return true; key_code = Desktop::Key::RETURN; break;
	case '\r': if (ignore_space) return true; key_code = Desktop::Key::RETURN; break;
	case ' ': if (ignore_space) return true; key_code = Desktop::Key::SPACE; break;
	case '!': key_code = Desktop::Key::EXCLAM; break;
	case '"': key_code = Desktop::Key::QUOTATION; break;
	case '#': key_code = Desktop::Key::NUMBERSIGN; break;
	case '$': key_code = Desktop::Key::DOLLAR; break;
	case '%': key_code = Desktop::Key::PERCENT; break;
	case '&': key_code = Desktop::Key::AMPERSAND; break;
	case '\'': key_code = Desktop::Key::APOSTROPHE; break;
	case '(': key_code = Desktop::Key::PARENLEFT; break;
	case ')': key_code = Desktop::Key::PARENRIGHT; break;
	case '*': key_code = Desktop::Key::ASTERISK; break;
	case '+': key_code = Desktop::Key::PLUS; break;
	case ',': key_code = Desktop::Key::COMMA; break;
	case '-': key_code = Desktop::Key::MINUS; break;
	case '.': key_code = Desktop::Key::PERIOD; break;
	case '/': key_code = Desktop::Key::SLASH; break;
	case '0': key_code = Desktop::Key::_0; break;
	case '1': key_code = Desktop::Key::_1; break;
	case '2': key_code = Desktop::Key::_2; break;
	case '3': key_code = Desktop::Key::_3; break;
	case '4': key_code = Desktop::Key::_4; break;
	case '5': key_code = Desktop::Key::_5; break;
	case '6': key_code = Desktop::Key::_6; break;
	case '7': key_code = Desktop::Key::_7; break;
	case '8': key_code = Desktop::Key::_8; break;
	case '9': key_code = Desktop::Key::_9; break;
	case ':': key_code = Desktop::Key::COLON; break;
	case ';': key_code = Desktop::Key::SEMICOLON; break;
	case '<': key_code = Desktop::Key::LESS; break;
	case '=': key_code = Desktop::Key::EQUAL; break;
	case '>': key_code = Desktop::Key::GREATER; break;
	case '?': key_code = Desktop::Key::QUESTION; break;
	case '@': key_code = Desktop::Key::AT; break;
	case 'A': key_code = Desktop::Key::A; break;
	case 'B': key_code = Desktop::Key::B; break;
	case 'C': key_code = Desktop::Key::C; break;
	case 'D': key_code = Desktop::Key::D; break;
	case 'E': key_code = Desktop::Key::E; break;
	case 'F': key_code = Desktop::Key::F; break;
	case 'G': key_code = Desktop::Key::G; break;
	case 'H': key_code = Desktop::Key::H; break;
	case 'I': key_code = Desktop::Key::I; break;
	case 'J': key_code = Desktop::Key::J; break;
	case 'K': key_code = Desktop::Key::K; break;
	case 'L': key_code = Desktop::Key::L; break;
	case 'M': key_code = Desktop::Key::M; break;
	case 'N': key_code = Desktop::Key::N; break;
	case 'O': key_code = Desktop::Key::O; break;
	case 'P': key_code = Desktop::Key::P; break;
	case 'Q': key_code = Desktop::Key::Q; break;
	case 'R': key_code = Desktop::Key::R; break;
	case 'S': key_code = Desktop::Key::S; break;
	case 'T': key_code = Desktop::Key::T; break;
	case 'U': key_code = Desktop::Key::U; break;
	case 'V': key_code = Desktop::Key::V; break;
	case 'W': key_code = Desktop::Key::W; break;
	case 'X': key_code = Desktop::Key::X; break;
	case 'Y': key_code = Desktop::Key::Y; break;
	case 'Z': key_code = Desktop::Key::Z; break;
	case '[': key_code = Desktop::Key::BRACKETLEFT; break;
	case ']': key_code = Desktop::Key::BRACKETRIGHT; break;
	case '^': key_code = Desktop::Key::ASCIICIRCUM; break;
	case '_': key_code = Desktop::Key::UNDERSCORE; break;
	case '`': key_code = Desktop::Key::GRAVE; break;
	case 'a': key_code = Desktop::Key::a; break;
	case 'b': key_code = Desktop::Key::b; break;
	case 'c': key_code = Desktop::Key::c; break;
	case 'd': key_code = Desktop::Key::d; break;
	case 'e': key_code = Desktop::Key::e; break;
	case 'f': key_code = Desktop::Key::f; break;
	case 'g': key_code = Desktop::Key::g; break;
	case 'h': key_code = Desktop::Key::h; break;
	case 'i': key_code = Desktop::Key::i; break;
	case 'j': key_code = Desktop::Key::j; break;
	case 'k': key_code = Desktop::Key::k; break;
	case 'l': key_code = Desktop::Key::l; break;
	case 'm': key_code = Desktop::Key::m; break;
	case 'n': key_code = Desktop::Key::n; break;
	case 'o': key_code = Desktop::Key::o; break;
	case 'p': key_code = Desktop::Key::p; break;
	case 'q': key_code = Desktop::Key::q; break;
	case 'r': key_code = Desktop::Key::r; break;
	case 's': key_code = Desktop::Key::s; break;
	case 't': key_code = Desktop::Key::t; break;
	case 'u': key_code = Desktop::Key::u; break;
	case 'v': key_code = Desktop::Key::v; break;
	case 'w': key_code = Desktop::Key::w; break;
	case 'x': key_code = Desktop::Key::x; break;
	case 'y': key_code = Desktop::Key::y; break;
	case 'z': key_code = Desktop::Key::z; break;
	case '{': key_code = Desktop::Key::BRACELEFT; break;
	case '|': key_code = Desktop::Key::BAR; break;
	case '}': key_code = Desktop::Key::BRACERIGHT; break;
	case '~': key_code = Desktop::Key::ASCIITILDE; break;
	case 0x7f: key_code = Desktop::Key::BACKSPACE; break;

	case '\\':
		this->parse_command(source, script);
		return true;

	[[unlikely]] default:
		if (ch == std::istream::traits_type::eof())
			return false;
		throw ScriptSyntaxError();
	}

	code.emplace_back(Impl::Opcode::KEY_CLICK, unsigned(key_code));
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
	case '\\':command_func = &Compiler::command_backslash; break;
	case 'n': case 'r': command_func = &Compiler::command_enter; break;
	case 't': command_func = &Compiler::command_tab; break;
	case 's': command_func = &Compiler::command_space; break;
	case '#': command_func = &Compiler::command_sleep; break;
	case '<': command_func = &Compiler::command_click_left; break;
	case '|': command_func = &Compiler::command_click_middle; break;
	case '>': command_func = &Compiler::command_click_right; break;
	case '@': command_func = &Compiler::command_move_pointer; break;
	case '?': command_func = &Compiler::command_find_pointer; break;
	case '{': command_func = &Compiler::command_begin_loop; break;
	case '}': command_func = &Compiler::command_end_loop; break;
	case '$': command_func = &Compiler::command_send_key; break;
	case '%': command_func = &Compiler::command_send_button; break;
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
	script.code.emplace_back(Opcode::KEY_CLICK, unsigned(Desktop::Key::BACKSLASH));
}

void Script::Impl::Compiler::command_enter(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::KEY_CLICK, unsigned(Desktop::Key::RETURN));
}

void Script::Impl::Compiler::command_tab(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::KEY_CLICK, unsigned(Desktop::Key::TAB));
}

void Script::Impl::Compiler::command_space(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::KEY_CLICK, unsigned(Desktop::Key::SPACE));
}

void Script::Impl::Compiler::command_sleep(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (args.empty()) {
		script.code.emplace_back(Opcode::SLEEP_SEC, 1);
		return;
	}

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
		script.code.emplace_back(Opcode::SLEEP_MS, unsigned(f * 1e3));
	}
}

void Script::Impl::Compiler::command_click_left(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::BUTTON_CLICK, unsigned(Desktop::Button::LEFT));
}

void Script::Impl::Compiler::command_click_middle(
		const std::vector<const char *> &args, Script::Impl &script) {
	Desktop::Button button;
	if (args.empty()) {
		button = Desktop::Button::MIDDLE;
	} else if (args.size() == 1 && (args[0][0] && !args[0][1])) {
		const auto dir = args[0][0];
		if (dir == '^')
			button = Desktop::Button::SCROLL_UP;
		else if (dir == 'v' || dir == 'V' )
			button = Desktop::Button::SCROLL_DOWN;
		else
			throw ScriptSyntaxError();
	} else {
		throw ScriptSyntaxError();
	}
	script.code.emplace_back(Opcode::BUTTON_CLICK, unsigned(button));
}

void Script::Impl::Compiler::command_click_right(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::BUTTON_CLICK, unsigned(Desktop::Button::RIGHT));
}

void Script::Impl::Compiler::command_move_pointer(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (args.size() != 2)
		throw ScriptSyntaxError();
	const auto x = atoi(args[0]), y = atoi(args[1]);
	const auto index = script.positions.size();
	script.positions.emplace_back(x >= 0 ? unsigned(x) : 0u, y >= 0 ? unsigned(y) : 0u);
	script.code.emplace_back(Opcode::POINTER_GOTO, unsigned(index));
}

void Script::Impl::Compiler::command_find_pointer(
		const std::vector<const char *> &args, Script::Impl &script) {
	unsigned int flags = 0;
	for (const char *arg : args) {
		if (arg[0] == '!' && !arg[1])
			flags |= 0b0001;
		else
			throw ScriptSyntaxError();
	}
	script.code.emplace_back(Opcode::POINTER_WHERE, flags);
}

void Script::Impl::Compiler::command_begin_loop(
		const std::vector<const char *> &args, Script::Impl &script) {
	int loops;
	if (args.empty())
		loops = 0;
	else if (args.size() == 1)
		loops = atoi(args[0]);
	else
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::LOOP_BEGIN, loops > 0 ? unsigned(loops) : 0);
}

void Script::Impl::Compiler::command_end_loop(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (!args.empty())
		throw ScriptSyntaxError();
	script.code.emplace_back(Opcode::LOOP_END, 0);
}

void Script::Impl::Compiler::command_send_key(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (args.empty() || args.size() > 2)
		throw ScriptSyntaxError();
	const auto [key, ok] = Desktop::key_from_name(args[0]);
	if (!ok)
		throw ScriptSyntaxError();
	Opcode op;
	if (args.size() == 1) {
		op = Opcode::KEY_CLICK;
	} else if (args[1][0] && !args[1][1]) {
		const auto dir = args[1][0];
		if (dir == '^')
			op = Opcode::KEY_UP;
		else if (dir == 'v' || dir == 'V' )
			op = Opcode::KEY_DOWN;
		else
			throw ScriptSyntaxError();
	} else {
		throw ScriptSyntaxError();
	}
	script.code.emplace_back(op, static_cast<unsigned int>(key));
}

void Script::Impl::Compiler::command_send_button(
		const std::vector<const char *> &args, Script::Impl &script) {
	if (args.empty() || args.size() > 2)
		throw ScriptSyntaxError();
	const auto [button, ok] = Desktop::button_from_name(args[0]);
	if (!ok)
		throw ScriptSyntaxError();
	Opcode op;
	if (args.size() == 1) {
		op = Opcode::BUTTON_CLICK;
	} else if (args[1][0] && !args[1][1]) {
		const auto dir = args[1][0];
		if (dir == '^')
			op = Opcode::BUTTON_UP;
		else if (dir == 'v' || dir == 'V' )
			op = Opcode::BUTTON_DOWN;
		else
			throw ScriptSyntaxError();
	} else {
		throw ScriptSyntaxError();
	}
	script.code.emplace_back(op, static_cast<unsigned int>(button));
}

Script::Impl::Player::StopToken Script::Impl::Player::stop_token;

Script::Impl::Player::Player() noexcept : random(nullptr) {
}

Script::Impl::Player::~Player () {
	if (this->random)
		delete this->random;
}

void Script::Impl::Player::random_sleep(bool status) noexcept {
	if (status) {
		if (!this->random)
			this->random = new Random;
	} else {
		if (this->random) {
			delete this->random;
			this->random = nullptr;
		}
	}
}

void Script::Impl::Player::operator()(const Script::Impl &script, Desktop &desktop) {
	Player::stop_token.clear();
	std::signal(SIGINT, [](int) { Player::stop_token.set(); });
	this->loops.clear();

	const auto *code_pointer = script.code.data();
	const auto *const code_end = code_pointer + script.code.size();
	while (code_pointer < code_end && !Player::stop_token.test()) {
		const auto instruction = *code_pointer++;
		const auto operand = instruction.operand();

		switch (instruction.opcode()) {
			using enum Impl::Opcode;

		case SLEEP_MS:
			this->sleep_ms(operand);
			continue;

		case SLEEP_SEC:
			this->sleep_ms(operand * 1000);
			continue;

		case KEY_UP:
			desktop.key(
				static_cast<Desktop::Key>(operand),
				Desktop::PressAction::Release
			);
			break;

		case KEY_DOWN:
			desktop.key(
				static_cast<Desktop::Key>(operand),
				Desktop::PressAction::Press
			);
			break;

		case KEY_CLICK:
			desktop.key(
				static_cast<Desktop::Key>(operand),
				Desktop::PressAction::Press
			);
			desktop.key(
				static_cast<Desktop::Key>(operand),
				Desktop::PressAction::Release
			);
			break;

		case BUTTON_UP:
			desktop.button(
				static_cast<Desktop::Button>(operand),
				Desktop::PressAction::Release
			);
			break;

		case BUTTON_DOWN:
			desktop.button(
				static_cast<Desktop::Button>(operand),
				Desktop::PressAction::Press
			);
			break;

		case BUTTON_CLICK:
			desktop.button(
				static_cast<Desktop::Button>(operand),
				Desktop::PressAction::Press
			);
			desktop.button(
				static_cast<Desktop::Button>(operand),
				Desktop::PressAction::Release
			);
			break;

		case POINTER_GOTO:
			desktop.pointer({
				script.positions[operand].first,
				script.positions[operand].second
			});
			break;

		case POINTER_WHERE:
			this->print_pointer(desktop, operand);
			break;

		case LOOP_BEGIN:
			this->loops.emplace_back(code_pointer, operand);
			break;

		case LOOP_END:
			if (this->loops.empty())
				break;
			if (auto &n = this->loops.back().times; n) {
				if (n == 1) {
					this->loops.pop_back();
					break;
				}
				n--;
			}
			code_pointer = this->loops.back().begin;
			break;

		[[unlikely]] default:
			continue;
		}

		desktop.flush();
		this->sleep_ms(50);
	}

	std::signal(SIGINT, SIG_DFL);
}

void Script::Impl::Player::sleep_ms(unsigned int time_ms) noexcept {
	if (this->random) {
		auto &rand = *this->random;
		auto off = rand.norm_dist(rand.rand_gen) * 0.125 * time_ms;
		if (-off >= time_ms) [[unlikely]]
			off = 0;
		time_ms += static_cast<int>(off);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
}

void Script::Impl::Player::print_pointer(
		const Desktop &desktop, unsigned int flags) noexcept {
	const auto pos = desktop.pointer();
	std::cout << '(' << pos.x << ',' << pos.y << ')';
	if (flags & 0b0001)
		std::cout << "\x1b[K\r" << std::flush;
	else
		std::cout << std::endl;
}

bool Script::random_sleep = true;
bool Script::ignore_space = false;

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

bool Script::empty() const noexcept {
	return this->_impl->code.empty();
}

void Script::append(std::istream &source) {
	Impl::Compiler compiler;
	compiler(source, *this->_impl);
}

void Script::clear() noexcept {
	auto &impl = *this->_impl;
	impl.code.clear();
	impl.positions.clear();
}

void Script::play(Desktop &desktop) const {
	Impl::Player player;
	player.random_sleep(Script::random_sleep);
	player(*this->_impl, desktop);
}
