/*
 * Copyright (C) 2010-2012 NVIDIA Corporation
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
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */


#include <stdio.h>
#include <stdarg.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#include "common-utils.h"


/****************************************************************************/
/* Memory allocation helper functions */
/****************************************************************************/

/*
 * nvalloc() - calloc wrapper that checks for errors; if an error
 * occurs, an error is printed to stderr and exit is called -- this
 * function will only return on success.
 */

void *nvalloc(size_t size)
{
    void *m = calloc(1, size);

    if (!m) {
        fprintf(stderr, "%s: memory allocation failure (%s)! \n",
                PROGRAM_NAME, strerror(errno));
        exit(1);
    }
    return m;

} /* nvalloc() */



/*
 * nvstrcat() - allocate a new string, copying all given strings
 * into it.  taken from glib
 */

char *nvstrcat(const char *str, ...)
{
    unsigned int l;
    va_list args;
    char *s;
    char *concat;

    l = 1 + strlen(str);
    va_start(args, str);
    s = va_arg(args, char *);

    while (s) {
        l += strlen(s);
        s = va_arg(args, char *);
    }
    va_end(args);

    concat = nvalloc(l);
    concat[0] = 0;

    strcat(concat, str);
    va_start(args, str);
    s = va_arg(args, char *);
    while (s) {
        strcat(concat, s);
        s = va_arg(args, char *);
    }
    va_end(args);

    return concat;

} /* nvstrcat() */



/*
 * nvrealloc() - realloc wrapper that checks for errors; if an error
 * occurs, an error is printed to stderr and exit is called -- this
 * function will only return on success.
 */

void *nvrealloc(void *ptr, size_t size)
{
    void *m;

    if (ptr == NULL) return nvalloc(size);

    m = realloc(ptr, size);
    if (!m) {
        fprintf(stderr, "%s: memory re-allocation failure (%s)! \n",
                PROGRAM_NAME, strerror(errno));
        exit(1);
    }
    return m;

} /* nvrealloc() */



/*
 * nvstrdup() - wrapper for strdup() that checks the return value; if
 * an error occurs, an error is printed to stderr and exit is called
 * -- this function will only return on success.
 */

char *nvstrdup(const char *s)
{
    char *m;

    if (!s) return NULL;

    m = strdup(s);

    if (!m) {
        fprintf(stderr, "%s: memory allocation failure during strdup (%s)! \n",
                PROGRAM_NAME, strerror(errno));
        exit(1);
    }
    return m;

} /* nvstrdup() */



/*
 * nvstrndup() - implementation of strndup() that checks return values; if
 * an error occurs, an error is printed to stderr and exit is called
 * -- this function will only return on success.
 */

char *nvstrndup(const char *s, size_t n)
{
    char *m;

    if (!s) return NULL;

    m = malloc(n + 1);

    if (!m) {
        fprintf(stderr, "%s: memory allocation failure during malloc (%s)! \n",
                PROGRAM_NAME, strerror(errno));
        exit(1);
    }

    strncpy (m, s, n);
    m[n] = '\0';

    return m;

} /* nvstrndup() */



/*
 * nvstrtolower() - convert the given string to lowercase.
 */

char *nvstrtolower(char *s)
{
    char *start = s;

    if (s == NULL) return NULL;

    while (*s) {
        *s = tolower(*s);
        s++;
    }

    return start;

} /* nvstrtolower() */



/*
 * nvstrtoupper() - convert the given string to uppercase.
 */

char *nvstrtoupper(char *s)
{
    char *start = s;

    if (s == NULL) return NULL;

    while (*s) {
        *s = toupper(*s);
        s++;
    }

    return start;

} /* nvstrtoupper() */



/*
 * nvasprintf() - implementation of asprintf() that checks return values; if an
 * error occurs, an error is printed to stderr and exit is called.
 * -- this function will only return on success.
 */
char *nvasprintf(const char *fmt, ...)
{
    char *str;

    NV_VSNPRINTF(str, fmt);

    return str;

} /* nvasprintf() */



/*
 * nvfree() - frees memory allocated with nvalloc(), provided
 * a non-NULL pointer is provided.
 */
void nvfree(void *s)
{
    if (s) free(s);

} /* nvfree() */



/****************************************************************************/
/* misc */
/****************************************************************************/

/*
 * tilde_expansion() - do tilde expansion on the given path name;
 * based loosely on code snippets found in the comp.unix.programmer
 * FAQ.  The tilde expansion rule is: if a tilde ('~') is alone or
 * followed by a '/', then substitute the current user's home
 * directory; if followed by the name of a user, then substitute that
 * user's home directory.
 *
 * Returns NULL if its argument is NULL; otherwise, returns a malloced
 * and tilde-expanded string.
 */

char *tilde_expansion(const char *str)
{
    char *prefix = NULL;
    const char *replace;
    char *user, *ret;
    struct passwd *pw;
    int len;

    if (!str) return NULL;

    if (str[0] != '~') return strdup(str);

    if ((str[1] == '/') || (str[1] == '\0')) {

        /* expand to the current user's home directory */

        prefix = getenv("HOME");
        if (!prefix) {

            /* $HOME isn't set; get the home directory from /etc/passwd */

            pw = getpwuid(getuid());
            if (pw) prefix = pw->pw_dir;
        }

        replace = str + 1;

    } else {

        /* expand to the specified user's home directory */

        replace = strchr(str, '/');
        if (!replace) replace = str + strlen(str);

        len = replace - str;
        user = malloc(len + 1);
        strncpy(user, str+1, len-1);
        user[len] = '\0';
        pw = getpwnam(user);
        if (pw) prefix = pw->pw_dir;
        free (user);
    }

    if (!prefix) return strdup(str);

    ret = malloc(strlen(prefix) + strlen(replace) + 1);
    strcpy(ret, prefix);
    strcat(ret, replace);

    return ret;

} /* tilde_expansion() */


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

TextRows *nv_format_text_rows(const char *prefix,
                              const char *str,
                              int width, int word_boundary)
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

        /* look for any newline inbetween a and b, and move b to it */

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

#define NV_MAX(x,y) ((x) > (y) ? (x) : (y))

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


/****************************************************************************/
/* printing helper functions */
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

/*
 * Call silence_fmt(1) to turn fmtout(), fmtoutp() and format() into noops.
 */
static int __silent = 0;

void silence_fmt(int val)
{
    __silent = val;
}


static void vformat(FILE *stream, const int wb,
                    const char *prefix, const char *buf)
{
    int i;
    TextRows *t;

    if (!__terminal_width) reset_current_terminal_width(0);

    t = nv_format_text_rows(prefix, buf, __terminal_width, wb);

    for (i = 0; i < t->n; i++) fprintf(stream, "%s\n", t->t[i]);

    nv_free_text_rows(t);
}


#define NV_VFORMAT(stream, wb, prefix, fmt)     \
do {                                            \
    char *buf;                                  \
    NV_VSNPRINTF(buf, fmt);                     \
    vformat(stream, wb, prefix, buf);           \
    free (buf);                                 \
} while(0)


void fmtout(const char *fmt, ...)
{
    if (__silent > 0) {
        return;
    }
    NV_VFORMAT(stdout, TRUE, NULL, fmt);
}


void fmtoutp(const char *prefix, const char *fmt, ...)
{
    if (__silent > 0) {
        return;
    }
    NV_VFORMAT(stdout, TRUE, prefix, fmt);
}


void fmterr(const char *fmt, ...)
{
    vformat(stderr, 0, NULL, "");
    NV_VFORMAT(stderr, TRUE, "ERROR: ", fmt);
    vformat(stderr, 0, NULL, "");
}


void fmtwarn(const char *fmt, ...)
{
    vformat(stderr, 0, NULL, "");
    NV_VFORMAT(stderr, TRUE, "WARNING: ", fmt);
    vformat(stderr, 0, NULL, "");
}


void fmt(FILE *stream, const char *prefix, const char *fmt, ...)
{
    if (__silent > 0) {
        return;
    }
    NV_VFORMAT(stream, TRUE, prefix, fmt);
}


/*
 * Read from the given FILE stream until a newline, EOF, or nul
 * terminator is encountered, writing data into a growable buffer.
 * The eof parameter is set to TRUE when EOF is encountered.  In all
 * cases, the returned string is null-terminated.
 *
 * XXX this function will be rather slow because it uses fgetc() to
 * pull each character off the stream one at a time; this is done so
 * that each character can be examined as it's read so that we can
 * appropriately deal with EOFs and newlines.  A better implementation
 * would use fgets(), but that would still require us to parse each
 * read line, checking for newlines or guessing if we hit an EOF.
 */
char *fget_next_line(FILE *fp, int *eof)
{
    char *buf = NULL, *tmpbuf;
    char *c = NULL;
    int len = 0, buflen = 0;
    int ret;

    const int __fget_next_line_len = 32;

    if (eof) {
        *eof = FALSE;
    }

    while (1) {
        if (buflen == len) { /* buffer isn't big enough -- grow it */
            buflen += __fget_next_line_len;
            tmpbuf = nvalloc(buflen);
            if (buf) {
                memcpy(tmpbuf, buf, len);
                nvfree(buf);
            }
            buf = tmpbuf;
            c = buf + len;
        }

        ret = fgetc(fp);

        if ((ret == EOF) && (eof)) {
            *eof = TRUE;
        }

        if ((ret == EOF) || (ret == '\n') || (ret == '\0')) {
            *c = '\0';
            return buf;
        }

        *c = (char) ret;

        len++;
        c++;

    } /* while (1) */

    return NULL; /* should never get here */
}

char *nvstrchrnul(char *s, int c)
{
    char *result = strchr(s, c);
    if (!result) {
        return (s + strlen(s));
    }
    return result;
}
