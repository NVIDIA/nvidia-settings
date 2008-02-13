/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

#include "msg.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

static void format(FILE*, const char*, const char *, va_list);
static int get_terminal_width(void);

/*
 * nv_error_msg() - print an error message, nicely formatted using the
 * format() function.
 */

void nv_error_msg(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "\n");
 
    va_start(ap, fmt);
    format(stderr, "ERROR: ", fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

} /* nv_error_msg() */



/*
 * nv_warning_msg() - print a warning message, nicely formatted using
 * the format() function.
 */

void nv_warning_msg(const char *fmt, ...)
{
    va_list ap;
    
    fprintf(stdout, "\n");

    va_start(ap, fmt);
    format(stdout, "WARNING: ", fmt, ap);
    va_end(ap);

    fprintf(stdout, "\n");
    
} /* nv_warning_msg() */



/*
 * nv_msg() - print a message, nicely formatted using the format()
 * function.
 */

void nv_msg(const char *prefix, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    format(stdout, prefix, fmt, ap);
    va_end(ap);

} /* nv_msg() */



/*
 * XXX gcc's '-ansi' option causes vsnprintf to not be defined, so
 * declare the prototype here.
 */

#if defined(__STRICT_ANSI__)
int vsnprintf(char *str, size_t size, const char  *format,
              va_list ap);
#endif



#define FMT_BUF_LEN 256

/*
 * nv_build_vararg_string() - return an alloced string, assembled
 * using vsnprintf().
 */

char *nv_build_vararg_string(const char *fmt, va_list ap)
{
    int len, finished, current_len;
    char *buf;

    finished = 0;
    current_len = FMT_BUF_LEN;
    buf = malloc(current_len);
    do {
        len = vsnprintf(buf, current_len, fmt, ap);
        if ((len == -1) || len > current_len) {
            
            /*
             * if we get in here we know that vsnprintf had to truncate the
             * string to make it fit in the buffer... we need to extend the
             * buffer to encompass the string.  Unfortunately, we have to deal
             * with two different semantics of the return value from
             * (v)snprintf:
             *
             * -1 when the buffer is not long enough (glibc < 2.1)
             * 
             * or
             *
             * the length the string would have been if the buffer had been
             * large enough (glibc >= 2.1)
             */
            
            if (len == -1) current_len += FMT_BUF_LEN;
            else current_len = len+1;
            buf = realloc(buf, current_len);
        }
        else finished = 1;
        
    } while (!finished);

    return buf;

} /* nv_build_vararg_string() */



/*
 * format() - this takes a printf-style format string and a variable
 * list of args.  We use sprintf to generate the desired string, and
 * then format the string so that not more than 80 characters are
 * printed across.
 */

static void format(FILE *stream, const char *prefix,
                   const char *fmt, va_list ap)
{
    int len, prefix_len, z, w, i, max_width;
    char *buf, *line, *local_prefix, *a, *b, *c;
        
    buf = nv_build_vararg_string(fmt, ap);
    
    max_width = get_terminal_width();

    /* loop until we've printed the entire string */
    
    z = strlen(buf);
    a = buf;

    if (prefix) {
        prefix_len = strlen(prefix);
        local_prefix = malloc(prefix_len+1);
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
                while (!isspace(*b)) b++;
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
        line = malloc(len+1);
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

        /* move to the first non whitespace character */
        
        while ((z > 0) && (isspace(*a))) a++, z--;

        if (local_prefix) {
            for (i = 0; i < prefix_len; i++) local_prefix[i] = ' ';
        }
        
    } while (z > 0);
    
    free (buf);
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
