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
 * nv_append_sprintf() - similar to glib's g_string_append_printf(), except
 * instead of operating on a GString it operates on a (char **). Appends a
 * formatted string to the end of the dynamically-allocated string pointed to by
 * *buf (or the empty string if *buf is NULL), potentially reallocating the
 * string in the process.  This function only returns on succcess.
 */
void nv_append_sprintf(char **buf, const char *fmt, ...)
{
    char *prefix, *suffix;

    prefix = *buf;
    NV_VSNPRINTF(suffix, fmt);

    if (!prefix) {
        *buf = suffix;
    } else {
        *buf = nvstrcat(prefix, suffix, NULL);
        free(prefix);
        free(suffix);
    }
}


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


/*
 * nv_prepend_to_string_list() - add a new string to a string list, delimited
 * by the given string delimiter. The original list is freed.
 */

char *nv_prepend_to_string_list(char *list, const char *item, const char *delim)
{
    char *new_list = nvstrcat(item, list ? delim : NULL, list, NULL);
    nvfree(list);
    return new_list;
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

/****************************************************************************/
/* file helper functions */
/****************************************************************************/

/*
 * nv_open() - open(2) wrapper; prints an error message if open(2)
 * fails and calls exit().  This function only returns on success.
 */

int nv_open(const char *pathname, int flags, mode_t mode)
{
    int fd;
    fd = open(pathname, flags, mode);
    if (fd == -1) {
        fprintf(stderr, "Failure opening %s (%s).\n",
                pathname, strerror(errno));
        exit(1);
    }
    return fd;

} /* nv_name() */



/*
 * nv_get_file_length() - stat(2) wrapper; prints an error message if
 * the system call fails and calls exit().  This function only returns
 * on success.
 */

int nv_get_file_length(const char *filename)
{
    struct stat stat_buf;
    int ret;
 
    ret = stat(filename, &stat_buf);
    if (ret == -1) {
        fprintf(stderr, "Unable to determine '%s' file length (%s).\n",
                filename, strerror(errno));
        exit(1);
    }
    return stat_buf.st_size;

} /* nv_get_file_length() */



/*
 * nv_set_file_length() - wrapper for lseek() and write(); prints an
 * error message if the system calls fail and calls exit().  This
 * function only returns on success.
 */

void nv_set_file_length(const char *filename, int fd, int len)
{
    if ((lseek(fd, len - 1, SEEK_SET) == -1) ||
        (write(fd, "", 1) == -1)) {
        fprintf(stderr, "Unable to set file '%s' length %d (%s).\n",
                filename, fd, strerror(errno));
        exit(1);
    }
} /* nv_set_file_length() */



/*
 * nv_mmap() - mmap(2) wrapper; prints an error message if mmap(2)
 * fails and calls exit().  This function only returns on success.
 */

void *nv_mmap(const char *filename, size_t len, int prot, int flags, int fd)
{
    void *ret;

    ret = mmap(0, len, prot, flags, fd, 0);
    if (ret == (void *) -1) {
        fprintf(stderr, "Unable to mmap file %s (%s).\n",
                filename, strerror(errno));
        exit(1);
    }
    return ret;

} /* nv_mmap() */


/*
 * nv_basename() - alternative to basename(3) which avoids differences in
 * behavior from different implementations: this implementation never modifies
 * the original string, and the return value can always be passed to free(3).
 */

char *nv_basename(const char *path)
{
    char *last_slash = strrchr(path, '/');
    if (last_slash) {
        return strdup(last_slash+1);
    } else {
        return strdup(path);
    }
}


/****************************************************************************/
/* string helper functions */
/****************************************************************************/

/*
 * nv_trim_space() - remove any leading and trailing whitespace from a string
 * and return a pointer to the modified string. The original string may be
 * modified; the returned value should *NOT* be deallocated with free(), since
 * it may point somewhere other than the beginning of the original string. If
 * the original string was a malloc()ed buffer, that string should be stored
 * separately from the returned value of nv_strip_space, and freed.
 */

char *nv_trim_space(char *string) {
    char *ret, *end;

    for (ret = string; *ret && isspace(*ret); ret++);
    for (end = ret + strlen(ret) - 1; end >= ret && isspace(*end); end--) {
        *end = '\0';
    }

    return ret;
}

/*
 * trim_char() - helper function to remove a character from the initial and
 * final positions of a string, and optionally report how many replacements
 * were made. The returned value should not be free()d (see nv_trim_space()).
 */

static char *trim_char(char *string, char trim, int *count) {
    int len, replaced = 0;

    if (count) {
        *count = 0;
    }

    if (string == NULL || trim == '\0') {
        return string;
    }

    if (string[0] == trim) {
        string++;
        replaced++;
    }

    len = strlen(string);

    if (string[len - 1] == trim) {
        string[len - 1] = '\0';
        replaced++;
    }

    if (count) {
        *count = replaced;
    }

    return string;
}

/*
 * nv_trim_char() - remove a character from the initial and final positions of
 * a string. The returned value should not be free()d (see nv_trim_space()).
 */

char *nv_trim_char(char *string, char trim) {
    return trim_char(string, trim, NULL);
}

/*
 * nv_trim_char_strict() - remove a character from the initial and final
 * positions of a string. If no replacements were made, or if replacements were
 * made at both positions, return the modified string. Otherwise, return NULL.
 * The returned value should not be free()d (see nv_trim_space()).
 */

char *nv_trim_char_strict(char *string, char trim) {
    int count;
    char *trimmed;

    trimmed = trim_char(string, trim, &count);

    if (count == 0 || count == 2) {
        return trimmed;
    }

    return NULL;
}

