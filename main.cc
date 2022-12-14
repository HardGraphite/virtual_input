#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "argparse.h"
#include "desktops.h"
#include "prints.h"
#include "script.h"

using namespace vinput;

static void parse_args(int argc, char *argv[], Desktop *&desktop, Script &script);

int main(int argc, char *argv[]) {
	int exit_status = EXIT_SUCCESS;
	Desktop *desktop = nullptr;
	Script script;

	try {
		parse_args(argc, argv, desktop, script);
		script.play(*desktop);
	} catch (const std::exception &e) {
		print_error(e);
		exit_status = EXIT_FAILURE;
	}

	if (desktop)
		disconnect_desktop(desktop);

	return exit_status;
}

static void print_program_help() noexcept;

namespace {

struct ArgParseContext {
	Desktop *&desktop;
	Script &script;
};

}

static int oh_help(void *, const argparse_option_t *, const char *) noexcept {
	print_program_help();
	std::exit(EXIT_SUCCESS);
}

static int oh_help_script(void *, const argparse_option_t *, const char *) noexcept {
	Script::print_doc(std::cout);
	std::exit(EXIT_SUCCESS);
}

static int oh_test(void *data, const argparse_option_t *, const char *) noexcept {
	auto &desktop = static_cast<ArgParseContext *>(data)->desktop;
	if (desktop)
		disconnect_desktop(desktop);
	desktop = connect_test_desktop();
	return 0;
}

static int oh_trace_pointer(
		void *data, const argparse_option_t *, const char *) noexcept {
	auto &script = static_cast<ArgParseContext *>(data)->script;
	std::istringstream ss(R"(\{\[?!]\})");
	script.append(ss);
	return 0;
}

static int oh_no_rand_sleep(
		void *, const argparse_option_t *, const char *) noexcept {
	Script::random_sleep = false;
	return 0;
}

static int oh_no_ignore_space(
		void *, const argparse_option_t *, const char *) noexcept {
	Script::ignore_space = false;
	return 0;
}

static int oh_file(
		void *data, const argparse_option_t *, const char *arg) noexcept {
	auto &script = static_cast<ArgParseContext *>(data)->script;

	try {
		if (!std::strcmp(arg, "-")) {
			script.append(std::cin);
		} else {
			std::ifstream file(arg);
			if (file.is_open())
				script.append(file);
		}
	} catch (const ScriptSyntaxError &e) {
		print_error(e);
		std::exit(EXIT_FAILURE);
	}

	return 0;
}

#pragma pack(push, 1)

static const argparse_option_t options[] = {
	{'h', "help", nullptr, "print help message and exit", oh_help},
	{0, "help-script", nullptr, "print script syntax and exit", oh_help_script},
	{'t', "test", nullptr, "print instructions instead of executing them", oh_test},
	{'p', "trace-pointer", nullptr,
		"trace pointer position and print to stdout", oh_trace_pointer},
	{0, "no-rand-sleep", nullptr,
		"disable random sleep time difference", oh_no_rand_sleep},
	{'s', "no-ignore-space", nullptr,
		"recognize spaces (0x09, 0x0a, 0x0d, 0x20) as keys in script", oh_no_ignore_space},
	{0, nullptr, "FILE", nullptr, oh_file},
	{0, nullptr, nullptr, nullptr, nullptr},
};

static const argparse_program_t program = {
	.name = "vinput",
	.usage = "[OPTION...] [SCRIPT_FILE|-]*",
	.help = "virtual input, read script and send fake input events to the display server",
	.opts = options,
};

#pragma pack(pop)

static void print_program_help() noexcept {
	argparse_help(&program);
}

static void parse_args(
		int argc, char *argv[], Desktop *&desktop, Script &script) {
	desktop = nullptr;
	ArgParseContext ctx = {
		.desktop = desktop,
		.script = script,
	};
	const auto ap_status = argparse_parse(options, argc, argv, &ctx);
	if (!ap_status) {
		if (!desktop)
			desktop = connect_current_desktop();
		if (script.empty())
			script.append(std::cin);
		return;
	}

	const char *err_msg;
	switch (ARGPARSE_GETERROR(ap_status)) {
	case ARGPARSE_ERR_BADOPT: err_msg = "unrecognized option"; break;
	case ARGPARSE_ERR_NOARG: err_msg = "unexpected argument for this option"; break;
	case ARGPARSE_ERR_NEEDARG: err_msg = "the option takes one argument"; break;
	case ARGPARSE_ERR_TERM: err_msg = nullptr; break;
	default: err_msg = nullptr; break;
	}
	if (err_msg) {
		std::cerr << argv[0] << ": " << err_msg << ": "
			<< argv[ARGPARSE_GETINDEX(ap_status)] << std::endl;
	}
	std::exit(EXIT_FAILURE);
}
