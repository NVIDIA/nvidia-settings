/*
 * Copyright (C) 2004-2010 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *      Free Software Foundation, Inc.
 *      59 Temple Place - Suite 330
 *      Boston, MA 02111-1307, USA
 *
 *
 * nvgetopt.h
 */

#ifndef __NVGETOPT_H__
#define __NVGETOPT_H__

#define NVGETOPT_FALSE 0
#define NVGETOPT_TRUE  1


/*
 * indicates that the option is a boolean value; the presence of the
 * option will be interpretted as a TRUE value; if the option is
 * prepended with '--no-', the option will be interpretted as a FALSE
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


#endif /* __NVGETOPT_H__ */
