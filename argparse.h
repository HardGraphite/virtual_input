#ifndef ARGPARSE_H
#define ARGPARSE_H

#ifdef __cplusplus
#	define ARGPARSE_INLINE    inline
#	define ARGPARSE_NOEXCEPT  noexcept
#else /* !__cplusplus */
#	define ARGPARSE_INLINE    static inline
#	define ARGPARSE_NOEXCEPT
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct argparse_option;

/* Callback function for option handling.
Paramter `opt_arg` will be NULL if there is no argument for this option.
If this function returns a non-zero value, argparse_parse() will return. */
typedef int (*argparse_optionhandler_t)
	(void *data, const struct argparse_option *opt, const char *opt_arg) ARGPARSE_NOEXCEPT;

/* Description of an option. */
typedef struct argparse_option {
	char short_name; /* Short option name to be used like `-o [arg]` or `-o[arg]`. Assign 0 to disable. */
	const char *long_name; /* Long option name to be used like `--opt [arg]` or `--opt=[arg]`. Assign NULL to disable. */
	const char *argument; /* Name of argument which will be printed by argparse_help(). Assign NULL to disable. */
	const char *help; /* Help message which will be used by argparse_help(). Nullable. */
	argparse_optionhandler_t handler; /* The option handler. */
} argparse_option_t;

/* An array of options. The last element must be filled with 0. */
typedef const argparse_option_t *argparse_options_t;

/* Description of the program. */
typedef struct argparse_program {
	const char *name; /* Program name. */
	const char *usage; /* Arguments usage message. Nullable. */
	const char *help; /* Help message. Nullable. */
	argparse_options_t opts; /* Valid options. */
} argparse_program_t;

#define ARGPARSE_OK          0 /* No error. */
#define ARGPARSE_ERR_BADARG  1 /* Error: unexpected positional argument. */
#define ARGPARSE_ERR_BADOPT  2 /* Error: unrecognized option. */
#define ARGPARSE_ERR_NOARG   3 /* Error: unexpected argument for this option. */
#define ARGPARSE_ERR_NEEDARG 4 /* Error: this option takes an argument. */
#define ARGPARSE_ERR_TERM    7 /* Error: terminate. */

/* Get error code (ARGPARSE_ERR_XXX or ARGPARSE_OK) from return value of argparse_parse(). */
#define ARGPARSE_GETERROR(STATUS) ((STATUS) & 0xf)
/* Get error index (argv[index]) from return value of argparse_parse(). */
#define ARGPARSE_GETINDEX(STATUS) ((STATUS) >> 4)

/* Parse command-line arguments. On error, it will return a non-zero value
representing the error type and position, which can be interpreted using
ARGPARSE_GETERROR() and ARGPARSE_GETINDEX(). */
ARGPARSE_INLINE int argparse_parse(
	argparse_options_t opts, int argc, char *argv[], void *data) ARGPARSE_NOEXCEPT;

/* Print help message to stdout. */
ARGPARSE_INLINE void argparse_help(const argparse_program_t *prog) ARGPARSE_NOEXCEPT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ARGPARSE_INLINE int argparse_parse(
		argparse_options_t opts, int argc, char *argv[], void *data) ARGPARSE_NOEXCEPT {
	int err_code = ARGPARSE_OK;
	int err_index = 0;

#define RETURN_ERR(ERR_CODE, ERR_INDEX) \
	do { \
		err_code = ERR_CODE; \
		err_index = ERR_INDEX; \
		goto return_err; \
	} while (0)

	for (int arg_index = 1; arg_index < argc; arg_index++) {
		char *const arg_str = argv[arg_index];
		const argparse_option_t *opt = NULL;

		if (arg_str[0] != '-' || !arg_str[1]) {
			for (const argparse_option_t *p = opts; ; p++) {
				if (!p->short_name && !p->long_name) {
					if (p->argument)
						opt = p;
					break;
				}
			}
			if (!opt)
				RETURN_ERR(ARGPARSE_ERR_BADARG, arg_index);
			if (opt->handler(data, opt, arg_str))
				RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
			continue;
		}

		if (arg_str[1] == '-') {
			char *const long_name = arg_str + 2;
			char *const equal_pos = strchr(long_name, '=');
			if (equal_pos)
				*equal_pos = '\0';
			for (const argparse_option_t *p = opts; ; p++) {
				const char *const p_long_name = p->long_name;
				if (p_long_name) {
					if (!strcmp(p_long_name, long_name)) {
						opt = p;
						break;
					}
				} else if (!p->short_name && !p->argument) {
					break;
				}
			}
			if (equal_pos)
				*equal_pos = '=';
			if (!opt)
				RETURN_ERR(ARGPARSE_ERR_BADOPT, arg_index);
			if (opt->argument) {
				if (equal_pos) {
					if (opt->handler(data, opt, equal_pos + 1))
						RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
				} else {
					const char *const opt_arg = argv[arg_index + 1];
					if (arg_index + 1 >= argc || opt_arg[0] == '-')
						RETURN_ERR(ARGPARSE_ERR_NEEDARG, arg_index);
					if (opt->handler(data, opt, opt_arg))
						RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
					arg_index++;
				}
			} else {
				if (equal_pos)
					RETURN_ERR(ARGPARSE_ERR_NOARG, arg_index);
				if (opt->handler(data, opt, NULL))
					RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
			}
		} else {
			const char short_name = arg_str[1];
			for (const argparse_option_t *p = opts; ; p++) {
				const char p_short_name = p->short_name;
				if (p_short_name) {
					if (p_short_name == short_name) {
						opt = p;
						break;
					}
				} else if (!p->long_name && !p->argument) {
					break;
				}
			}
			if (!opt)
				RETURN_ERR(ARGPARSE_ERR_BADOPT, arg_index);
			if (opt->argument) {
				if (arg_str[2]) {
					if (opt->handler(data, opt, arg_str + 2))
						RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
				} else {
					const char *const opt_arg = argv[arg_index + 1];
					if (arg_index + 1 >= argc || opt_arg[0] == '-')
						RETURN_ERR(ARGPARSE_ERR_NEEDARG, arg_index);
					if (opt->handler(data, opt, opt_arg))
						RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
					arg_index++;
				}
			} else {
				if (arg_str[2])
					RETURN_ERR(ARGPARSE_ERR_NOARG, arg_index);
				if (opt->handler(data, opt, NULL))
					RETURN_ERR(ARGPARSE_ERR_TERM, arg_index);
			}
		}
	}

	return 0;

#undef RETURN_ERR

return_err:
	assert(err_code != ARGPARSE_OK);
	const int status = (err_index << 4) | err_code;
	assert(ARGPARSE_GETERROR(status) == err_code);
	assert(ARGPARSE_GETINDEX(status) == err_index);
	return status;
}

ARGPARSE_INLINE void argparse_help(const argparse_program_t *prog) ARGPARSE_NOEXCEPT {
	printf("Usage: %s %s\n", prog->name, prog->usage ? prog->usage : "...");
	if (prog->help) {
		fwrite("  ", 1, 2, stdout);
		puts(prog->help);
		fputc('\n', stdout);
	}

	puts("Options:");
	const size_t left_width = 30, right_width = 80 - left_width;
	for (const argparse_option_t *opt = prog->opts; ; opt++) {
		if (!opt->short_name && !opt->long_name) {
			if (opt->argument)
				continue;
			else
				break;
		}

		size_t char_count = 0;
		fwrite("  ", 1, 2, stdout);
		char_count += 2;
		if (opt->short_name) {
			fputc('-', stdout);
			fputc(opt->short_name, stdout);
			char_count += 2;
			if (opt->long_name) {
				fwrite(", ", 1, 2, stdout);
				char_count += 2;
			}
		}
		if (opt->long_name) {
			const size_t n = strlen(opt->long_name);
			fwrite("--", 1, 2, stdout);
			fwrite(opt->long_name, 1, n, stdout);
			char_count += 2 + n;
		}
		if (opt->argument) {
			const size_t n = strlen(opt->argument);
			fputc(' ', stdout);
			fwrite(opt->argument, 1, n, stdout);
			char_count += 1 + n;
		}
		fputc(' ', stdout);
		char_count += 1;

		if (opt->help) {
			size_t pad_n;
			if (char_count <= left_width) {
				pad_n = left_width - char_count;
			} else {
				fputc('\n', stdout);
				pad_n = left_width;
			}
			for (size_t i = 0; i < pad_n; i++)
				fputc(' ', stdout);

			const char *s = opt->help;
			while (1) {
				if (strlen(s) <= right_width) {
					fputs(s, stdout);
					break;
				}

				const char *end = s + right_width;
				while (1) {
					if (*end == ' ')
						break;
					if (end == s) {
						end = s + right_width;
						break;
					}
					end--;
				}

				fwrite(s, 1, (size_t)(end - s), stdout);
				s = *end == ' ' ? end + 1 : end;
				fputc('\n', stdout);
				for (size_t i = 0; i < left_width; i++)
					fputc(' ', stdout);
			}
		}

		fputc('\n', stdout);
	}
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#undef ARGPARSE_INLINE
#undef ARGPARSE_NOEXCEPT

#endif /* ARGPARSE_H */
