/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
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

#include "msg.h"
#include "command-line.h"
#include "common-utils.h"

#include <NvCtrlAttributes.h>

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

extern int __verbosity;

static void format(FILE*, const char*, char *, int);
static int get_terminal_width(void);

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
    if (__verbosity < VERBOSITY_ERROR) return;

    fprintf(stderr, "\n");
 
    NV_FORMAT(stderr, "ERROR: ", fmt, False);

    fprintf(stderr, "\n");

} /* nv_error_msg() */



/*
 * nv_deprecated_msg() - print a deprecation message, nicely formatted using
 * the format() function.
 *
 * This function should be used for all deprecation messages.
 */

void nv_deprecated_msg(const char *fmt, ...)
{
    if (__verbosity < VERBOSITY_DEPRECATED) return;

    fprintf(stderr, "\n");

    NV_FORMAT(stderr, "DEPRECATED: ", fmt, False);

    fprintf(stderr, "\n");
}



/*
 * nv_warning_msg() - print a warning message, nicely formatted using
 * the format() function.
 *
 * This function should be used for all warnings.
 */

void nv_warning_msg(const char *fmt, ...)
{
    if (__verbosity < VERBOSITY_WARNING) return;

    fprintf(stdout, "\n");

    NV_FORMAT(stdout, "WARNING: ", fmt, False);

    fprintf(stdout, "\n");
    
} /* nv_warning_msg() */



/*
 * nv_info_msg() - print an info message, nicely formatted using
 * the format() function.
 *
 * This function should be used to display verbose information.
 */

void nv_info_msg(const char *prefix, const char *fmt, ...)
{
    if (__verbosity < VERBOSITY_ALL) return;

    NV_FORMAT(stdout, prefix, fmt, False);
    
} /* nv_info_msg() */



/*
 * nv_msg() - print a message, nicely formatted using the format()
 * function.
 *
 * This function should be used to display messages independent
 * of the verbosity level.
 */

void nv_msg(const char *prefix, const char *fmt, ...)
{
    NV_FORMAT(stdout, prefix, fmt, False);

} /* nv_msg() */


/*
 * nv_msg_preserve_whitespace() - Prints the message, just like nv_msg()
 * using format(), the difference is, whitespace characters are not
 * skipped during the text processing.
 */

void nv_msg_preserve_whitespace(const char *prefix, const char *fmt, ...)
{
    NV_FORMAT(stdout, prefix, fmt, True);

} /* nv_msg_preserve_whitespace() */


/*
 * XXX gcc's '-ansi' option causes vsnprintf to not be defined, so
 * declare the prototype here.
 */

#if defined(__STRICT_ANSI__)
int vsnprintf(char *str, size_t size, const char  *format,
              va_list ap);
#endif



/*
 * format() - formats and prints the string buf so that no more than
 * 80 characters are printed across.
 */

static void format(FILE *stream, const char *prefix,
                   char *buf, int preserveWhitespace)
{
    int len, prefix_len, z, w, i, max_width;
    char *line, *local_prefix, *a, *b, *c;
        
    max_width = get_terminal_width();

    /* loop until we've printed the entire string */
    
    z = strlen(buf);
    a = buf;

    if (prefix) {
        prefix_len = strlen(prefix);
        local_prefix = nvalloc(prefix_len+1);
        strcpy(local_prefix, prefix);
    } else {
        prefix_len = 0;
        local_prefix = NULL;
    }
    
    do {
        w = max_width;
        
        /* adjust the max width for any prefix */

        w -= prefix_len;
        
        /*
         * if the string will fit on one line, point b to the end of the
         * string
         */
        
        if (z < w) b = a + z;

        /* 
         * if the string won't fit on one line, move b to where the end of
         * the line should be, and then move b back until we find a space;
         * if we don't find a space before we back b all the way up to a, start
         * b at a, and move forward until we do find a space
         */
        
        else {
            b = a + w;
            while ((b >= a) && (!isspace(*b))) b--;
            
            if (b <= a) {
                b = a;
                while (*b && !isspace(*b)) b++;
            }
        }

        /* look for any newline in the line, and move b to it */
        
        for (c = a; c < b; c++) if (*c=='\n') { b = c; break; }
        
        /* print the string that starts at a and ends at b */

        /*
         * XXX this could be done just by temporarily overwriting b
         * with '\0'
         */

        len = b-a;
        line = nvalloc(len+1);
        strncpy(line, a, len);
        line[len] = '\0';
        if (local_prefix) {
            fprintf(stream, "%s%s\n", local_prefix, line); 
        } else {
            fprintf(stream, "%s\n", line);
        }
        free(line);
        
        /*
         * adjust the length of the string and move the pointer to the
         * beginning of the new line
         */
        
        z -= (b - a + 1);
        a = b + 1;

        if (!preserveWhitespace) {
            /* move to the first non whitespace character */
            while ((z > 0) && (isspace(*a))) a++, z--;
        }

        if (local_prefix) {
            for (i = 0; i < prefix_len; i++) local_prefix[i] = ' ';
        }
        
    } while (z > 0);
    
    if (local_prefix) free (local_prefix);

} /* format() */


#define DEFAULT_MAX_WIDTH 75

static int get_terminal_width(void)
{
    struct winsize ws;
    
    if (ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return DEFAULT_MAX_WIDTH;
    } else {
        return (ws.ws_col - 1);
    }
}

