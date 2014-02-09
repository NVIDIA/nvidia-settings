/*
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#if defined(__sun)
#include <sys/termios.h>
#endif

#include "msg.h"
#include "common-utils.h"


/*
 * verbosity, controls output of errors, warnings and other
 * information.
 */

static NvVerbosity __verbosity = NV_VERBOSITY_DEFAULT;

NvVerbosity nv_get_verbosity()
{
    return __verbosity;
}

void nv_set_verbosity(NvVerbosity level)
{
    __verbosity = level;
}


/****************************************************************************/
/* Formatted I/O functions */
/****************************************************************************/

#define DEFAULT_WIDTH 75

static unsigned short __terminal_width = 0;

/*
 * reset_current_terminal_width() - if new_val is zero, then use the
 * TIOCGWINSZ ioctl to get the current width of the terminal, and
 * assign it the value to __terminal_width.  If the ioctl fails, use a
 * hardcoded constant.  If new_val is non-zero, then use new_val.
 */

void reset_current_terminal_width(unsigned short new_val)
{
    struct winsize ws;

    if (new_val) {
        __terminal_width = new_val;
        return;
    }

    if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        __terminal_width = DEFAULT_WIDTH;
    } else {
        __terminal_width = ws.ws_col - 1;
    }
}


static void format(FILE *stream, const char *prefix, const char *buf,
                   const int whitespace)
{
    int i;
    TextRows *t;

    if (!__terminal_width) reset_current_terminal_width(0);

    t = nv_format_text_rows(prefix, buf, __terminal_width, whitespace);

    for (i = 0; i < t->n; i++) fprintf(stream, "%s\n", t->t[i]);

    nv_free_text_rows(t);
}


#define NV_FORMAT(stream, prefix, fmt, whitespace) \
do {                                               \
    char *buf;                                     \
    NV_VSNPRINTF(buf, fmt);                        \
    format(stream, prefix, buf, whitespace);       \
    free (buf);                                    \
} while(0)


/*
 * nv_error_msg() - print an error message, nicely formatted using the
 * format() function.
 *
 * This function should be used for all errors.
 */

void nv_error_msg(const char *fmt, ...)
{
    if (__verbosity < NV_VERBOSITY_ERROR) return;

    format(stderr, NULL, "", TRUE);
    NV_FORMAT(stderr, "ERROR: ", fmt, TRUE);
    format(stderr, NULL, "", TRUE);
} /* nv_error_msg() */


/*
 * nv_deprecated_msg() - print a deprecation message, nicely formatted using
 * the format() function.
 *
 * This function should be used for all deprecation messages.
 */

void nv_deprecated_msg(const char *fmt, ...)
{
    if (__verbosity < NV_VERBOSITY_DEPRECATED) return;

    format(stderr, NULL, "", TRUE);
    NV_FORMAT(stderr, "DEPRECATED: ", fmt, TRUE);
    format(stderr, NULL, "", TRUE);
}


/*
 * nv_warning_msg() - print a warning message, nicely formatted using
 * the format() function.
 *
 * This function should be used for all warnings.
 */

void nv_warning_msg(const char *fmt, ...)
{
    if (__verbosity < NV_VERBOSITY_WARNING) return;

    format(stderr, NULL, "", TRUE);
    NV_FORMAT(stderr, "WARNING: ", fmt, TRUE);
    format(stderr, NULL, "", TRUE);
} /* nv_warning_msg() */


/*
 * nv_info_msg() - print an info message, nicely formatted using
 * the format() function.
 *
 * This function should be used to display verbose information.
 */

void nv_info_msg(const char *prefix, const char *fmt, ...)
{
    if (__verbosity < NV_VERBOSITY_ALL) return;

    NV_FORMAT(stdout, prefix, fmt, TRUE);
} /* nv_info_msg() */


/*
 * nv_info_msg_to_file() - Prints the message, just like nv_info_msg()
 * using format() the difference is, it prints to any stream defined by
 * the corresponding argument.
 */

void nv_info_msg_to_file(FILE *stream, const char *prefix, const char *fmt, ...)
{
    if (__verbosity < NV_VERBOSITY_ALL) return;

    NV_FORMAT(stream, prefix, fmt, TRUE);
} /* nv_info_msg_to_file() */


/*
 * nv_msg() - print a message, nicely formatted using the format()
 * function.
 *
 * This function should be used to display messages independent
 * of the verbosity level.
 */

void nv_msg(const char *prefix, const char *fmt, ...)
{
    NV_FORMAT(stdout, prefix, fmt, TRUE);
} /* nv_msg() */


/*
 * nv_msg_preserve_whitespace() - Prints the message, just like nv_msg()
 * using format(), the difference is, whitespace characters are not
 * skipped during the text processing.
 */

void nv_msg_preserve_whitespace(const char *prefix, const char *fmt, ...)
{
    NV_FORMAT(stdout, prefix, fmt, FALSE);
} /* nv_msg_preserve_whitespace() */


/*
 * XXX gcc's '-ansi' option causes vsnprintf to not be defined, so
 * declare the prototype here.
 */

#if defined(__STRICT_ANSI__)
int vsnprintf(char *str, size_t size, const char  *format,
              va_list ap);
#endif


/****************************************************************************/
/* TextRows helper functions */
/****************************************************************************/

/*
 * nv_format_text_rows() - this function breaks the given string str
 * into some number of rows, where each row is not longer than the
 * specified width.
 *
 * If prefix is non-NULL, the first line is prepended with the prefix,
 * and subsequent lines are indented to line up with the prefix.
 *
 * If word_boundary is TRUE, then attempt to only break lines on
 * boundaries between words.
 */

TextRows *nv_format_text_rows(const char *prefix, const char *str, int width,
                              int word_boundary)
{
    int len, prefix_len, z, w, i;
    char *line, *buf, *local_prefix, *a, *b, *c;
    TextRows *t;

    /* initialize the TextRows structure */

    t = (TextRows *) malloc(sizeof(TextRows));

    if (!t) return NULL;

    t->t = NULL;
    t->n = 0;
    t->m = 0;

    if (!str) return t;

    buf = strdup(str);

    if (!buf) return t;

    z = strlen(buf); /* length of entire string */
    a = buf;         /* pointer to the start of the string */

    /* initialize the prefix fields */

    if (prefix) {
        prefix_len = strlen(prefix);
        local_prefix = strdup(prefix);
    } else {
        prefix_len = 0;
        local_prefix = NULL;
    }

    /* adjust the max width for any prefix */

    w = width - prefix_len;

    do {
        /*
         * if the string will fit on one line, point b to the end of the
         * string
         */

        if (z < w) b = a + z;

        /*
         * if the string won't fit on one line, move b to where the
         * end of the line should be, and then move b back until we
         * find a space; if we don't find a space before we back b all
         * the way up to a, just assign b to where the line should end.
         */

        else {
            b = a + w;

            if (word_boundary) {
                while ((b >= a) && (!isspace(*b))) b--;
                if (b <= a) b = a + w;
            }
        }

        /* look for any newline between a and b, and move b to it */

        for (c = a; c < b; c++) if (*c == '\n') { b = c; break; }

        /*
         * copy the string that starts at a and ends at b, prepending
         * with a prefix, if present
         */

        len = b-a;
        len += prefix_len;
        line = (char *) malloc(len+1);
        if (local_prefix) strncpy(line, local_prefix, prefix_len);
        strncpy(line + prefix_len, a, len - prefix_len);
        line[len] = '\0';

        /* append the new line to the array of text rows */

        t->t = (char **) realloc(t->t, sizeof(char *) * (t->n + 1));
        t->t[t->n] = line;
        t->n++;

        if (t->m < len) t->m = len;

        /*
         * adjust the length of the string and move the pointer to the
         * beginning of the new line
         */

        z -= (b - a + 1);
        a = b + 1;

        /* move to the first non whitespace character (excluding newlines) */

        if (word_boundary && isspace(*b)) {
            while ((z) && (isspace(*a)) && (*a != '\n')) a++, z--;
        } else {
            if (!isspace(*b)) z++, a--;
        }

        if (local_prefix) {
            for (i = 0; i < prefix_len; i++) local_prefix[i] = ' ';
        }

    } while (z > 0);

    if (local_prefix) free(local_prefix);
    free(buf);

    return t;
}


/*
 * nv_text_rows_append() - append the given msg to the existing TextRows
 */

void nv_text_rows_append(TextRows *t, const char *msg)
{
    int len;

    t->t = realloc(t->t, sizeof(char *) * (t->n + 1));

    if (msg) {
        t->t[t->n] = strdup(msg);
        len = strlen(msg);
        if (t->m < len) t->m = len;
    } else {
        t->t[t->n] = NULL;
    }

    t->n++;
}

/*
 * nv_concat_text_rows() - concatenate two text rows, storing the
 * result in t0
 */

void nv_concat_text_rows(TextRows *t0, TextRows *t1)
{
    int n, i;

    n = t0->n + t1->n;

    t0->t = realloc(t0->t, sizeof(char *) * n);

    for (i = 0; i < t1->n; i++) {
        t0->t[i + t0->n] = strdup(t1->t[i]);
    }

    t0->m = NV_MAX(t0->m, t1->m);
    t0->n = n;

} /* nv_concat_text_rows() */


/*
 * nv_free_text_rows() - free the TextRows data structure allocated by
 * nv_format_text_rows()
 */

void nv_free_text_rows(TextRows *t)
{
    int i;

    if (!t) return;
    for (i = 0; i < t->n; i++) free(t->t[i]);
    if (t->t) free(t->t);
    free(t);

} /* nv_free_text_rows() */

