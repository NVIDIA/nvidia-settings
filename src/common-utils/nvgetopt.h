/*
 * Copyright (C) 2004-2010 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __NVGETOPT_H__
#define __NVGETOPT_H__

#define NVGETOPT_FALSE 0
#define NVGETOPT_TRUE  1


/*
 * mask of bits not used by nvgetopt in NVGetoptOption::flags;
 * these bits are available for use within specific users of
 * nvgetopt
 */

#define NVGETOPT_UNUSED_FLAG_RANGE 0xffff0000

/*
 * indicates that the option is a boolean value; the presence of the
 * option will be interpreted as a TRUE value; if the option is
 * prepended with '--no-', the option will be interpreted as a FALSE
 * value.  On success, nvgetopt will return the parsed boolean value
 * through 'boolval'.
 */

#define NVGETOPT_IS_BOOLEAN             0x01


/*
 * indicates that the option takes an argument to be interpreted as a
 * string; on success, nvgetopt will return the parsed string argument
 * through 'strval'.
 */

#define NVGETOPT_STRING_ARGUMENT        0x02


/*
 * indicates that the option takes an argument to be interpreted as an
 * integer; on success, nvgetopt will return the parsed integer
 * argument through 'intval'.
 */

#define NVGETOPT_INTEGER_ARGUMENT       0x04


/*
 * indicates that the option takes an argument to be interpreted as
 * an double; on success, nvgetopt will return the parsed double
 * argument through 'doubleval'.
 */

#define NVGETOPT_DOUBLE_ARGUMENT        0x08


/* helper macro */

#define NVGETOPT_HAS_ARGUMENT (NVGETOPT_STRING_ARGUMENT | \
                               NVGETOPT_INTEGER_ARGUMENT | \
                               NVGETOPT_DOUBLE_ARGUMENT)

/*
 * indicates that the option, which normally takes an argument, can be
 * disabled if the option is prepended with '--no-', in which case,
 * the option does not take an argument.  If the option is disabled,
 * nvgetopt will return TRUE through 'disable_val'.
 *
 * Note that NVGETOPT_ALLOW_DISABLE can only be used with options that
 * take arguments.
 */

#define NVGETOPT_ALLOW_DISABLE          0x10


/*
 * indicates that the argument for this option is optional; if no
 * argument is present (either the option is already at the end of the
 * argv array, or the next option in argv starts with '-'), then the
 * option is returned without an argument.
 */

#define NVGETOPT_ARGUMENT_IS_OPTIONAL   0x20


/*
 * The NVGETOPT_HELP_ALWAYS flag is not used by nvgetopt() itself, but
 * is often used by other users of NVGetoptOption tables, who print
 * out basic and advanced help.  In such cases, OPTION_HELP_ALWAYS is
 * used to indicate that the help for the option should always be
 * printed.
 */

#define NVGETOPT_HELP_ALWAYS            0x40


typedef struct {
    const char *name;
    int val;
    unsigned int flags;
    char *arg_name;     /* not used by nvgetopt() */
    char *description;  /* not used by nvgetopt() */
} NVGetoptOption;


/*
 * nvgetopt() - see the glibc getopt_long(3) manpage for usage
 * description.  Options can be prepended with "--", "-", or "--no-".
 *
 * A global variable stores the current index into the argv array, so
 * subsequent calls to nvgetopt() will advance through argv[].
 *
 * On success, the matching NVGetoptOption.val is returned.
 *
 * If the NVGETOPT_IS_BOOLEAN flag is set, boolval will be set to TRUE
 * (or FALSE, if the option string was prepended with "--no-").
 *
 * disable_val will be assigned TRUE if the option string was
 * prepended with "--no-", otherwise it will be assigned FALSE.
 *
 * If an argument is successfully parsed, one of strval, intval, or
 * doubleval will be assigned, based on which of
 * NVGETOPT_STRING_ARGUMENT, NVGETOPT_INTEGER_ARGUMENT, or
 * NVGETOPT_DOUBLE_ARGUMENT is set in the option's flags.  If strval
 * is assigned to a non-NULL value by nvgetopt, then it is the
 * caller's responsibility to free the string when done with it.
 *
 * On failure, an error is printed to stderr, and 0 is returned.
 *
 * When there are no more options to parse, -1 is returned.
 */

int nvgetopt(int argc,
             char *argv[],
             const NVGetoptOption *options,
             char **strval,
             int *boolval,
             int *intval,
             double *doubleval,
             int *disable_val);

/*
 * nvgetopt_print_help() - print a help message for each option in the
 * provided NVGetoptOption array.  This is useful for a utility's
 * "--help" output.
 *
 * Options will only be printed if they have every bit set that
 * include_mask includes.
 *
 * For each option, the provided callback function wil be called with
 * two strings: a name string that lists the option's name, and a
 * description string for the option.  The callback function is
 * responsible for actually printing these strings.  Examples:
 *
 * name = "-v, --version";
 * description = "Print usage information for the common commandline "
 *     "options and exit.";
 */

typedef void nvgetopt_print_help_callback_ptr(const char *name,
                                              const char *description);

void nvgetopt_print_help(const NVGetoptOption *options,
                         unsigned int include_mask,
                         nvgetopt_print_help_callback_ptr callback);

#endif /* __NVGETOPT_H__ */
