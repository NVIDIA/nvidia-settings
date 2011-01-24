/*
 * Copyright (C) 2010 NVIDIA Corporation
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
