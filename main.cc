#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "argparse.h"
#include "script.h"
#include "desktops.h"

static void parse_args(int argc, char *argv[], vinput::Script &script);

int main(int argc, char *argv[]) {
	vinput::Script script;
	parse_args(argc, argv, script);
	auto &desktop = vinput::connect_current_desktop();
	script.play(desktop);
	vinput::disconnect_desktop(desktop);
}

static void print_program_help() noexcept;

static int oh_help(void *, const argparse_option_t *, const char *) noexcept {
	print_program_help();
	std::exit(EXIT_SUCCESS);
}

static int oh_help_script(void *, const argparse_option_t *, const char *) noexcept {
	vinput::Script::print_doc(std::cout);
	std::exit(EXIT_SUCCESS);
}

static int oh_no_rand_sleep(
		void *, const argparse_option_t *, const char *) noexcept {
	vinput::Script::random_sleep = false;
	return 0;
}

static int oh_ignore_space(
		void *, const argparse_option_t *, const char *) noexcept {
	vinput::Script::ignore_space = true;
	return 0;
}

static int oh_file(
		void *data, const argparse_option_t *, const char *arg) noexcept {
	auto &script = *static_cast<vinput::Script *>(data);
	if (!std::strcmp(arg, "-")) {
		script.append(std::cin);
	} else {
		std::ifstream file(arg);
		if (file.is_open())
			script.append(file);
	}
	return 0;
}

#pragma pack(push, 1)

static const argparse_option_t options[] = {
	{'h', "help", nullptr, "print help message and exit", oh_help},
	{0, "help-script", nullptr, "print script syntax and exit", oh_help_script},
	{0, "no-rand-sleep", nullptr,
		"disable random sleep time difference", oh_no_rand_sleep},
	{0, "ignore-space", nullptr,
		"ignore spaces (0x09, 0x0a, 0x0d, 0x20) in script", oh_ignore_space},
	{0, nullptr, "FILE", nullptr, oh_file},
	{0, nullptr, nullptr, nullptr, nullptr},
};

static const argparse_program_t program = {
	.name = "vinput",
	.usage = "[OPTION...] [SCRIPT_FILE|-]*",
	.help = "virtual input, sending fake input events to the display server",
	.opts = options,
};

#pragma pack(pop)

static void print_program_help() noexcept {
	argparse_help(&program);
}

static void parse_args(int argc, char *argv[], vinput::Script &script) {
	const auto ap_status = argparse_parse(options, argc, argv, &script);
	if (!ap_status) {
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
