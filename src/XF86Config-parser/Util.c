/*
 * nvidia-xconfig: A tool for manipulating X config files,
 * specifically for use by the NVIDIA Linux graphics driver.
 *
 * Copyright (C) 2005 NVIDIA Corporation
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
 *
 *
 * Util.c
 */

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "xf86Parser.h"
#include "Configint.h"

void *xconfigAlloc(size_t size)
{
    void *m = malloc(size);

    if (!m) {
        fprintf(stderr, "memory allocation failure (%s)! \n", strerror(errno));
        exit(1);
    }
    memset((char *) m, 0, size);
    return m;

} /* xconfigAlloc() */


/*
 * xconfigStrdup() - wrapper for strdup() that checks the return
 * value; if an error occurs, an error is printed to stderr and exit
 * is called -- this function will only return on success.
 */

char *xconfigStrdup(const char *s)
{
    char *m;

    if (!s) return NULL;

    m = strdup(s);

    if (!m) {
        fprintf(stderr, "memory allocation failure during strdup (%s)! \n",
                strerror(errno));
        exit(1);
    }
    return m;

} /* xconfigStrdup() */


/*
 * xconfigStrcat() - allocate a new string, copying all given strings
 * into it.  taken from glib
 */

char *xconfigStrcat(const char *str, ...)
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

    concat = xconfigAlloc(l);
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

} /* xconfigStrcat() */






#define NV_FMT_BUF_LEN 64

extern int configLineNo;
extern char *configSection;
extern char *configPath;

__attribute__((__format__(__printf__, 2, 0)))
void xconfigErrorMsg(MsgType t, char *fmt, ...)
{
    va_list ap;
    int len, current_len = NV_FMT_BUF_LEN;
    char *b, *pre = NULL, *msg;
    char scratch[64];

    b = xconfigAlloc(current_len);

    while (1) {
        va_start(ap, fmt);
        len = vsnprintf(b, current_len, fmt, ap);
        va_end(ap);

        if ((len > -1) && (len < current_len)) {
            break;
        } else if (len > -1) {
            current_len = len + 1;
        } else {
            current_len += NV_FMT_BUF_LEN;
        }

        free(b);
        b = xconfigAlloc(current_len);
    }

    switch (t) {
    case ParseErrorMsg:
        sprintf(scratch, "%d", configLineNo);
        pre = xconfigStrcat("Parse error on line ", scratch, " of section ",
                         configSection, " in file ", configPath, ".\n", NULL);
        break;
    case ParseWarningMsg:
        sprintf(scratch, "%d", configLineNo);
        pre = xconfigStrcat("Parse warning on line ", scratch, " of section ",
                         configSection, " in file ", configPath, ".\n", NULL);
        break;
    case ValidationErrorMsg:
        pre = xconfigStrcat("Data incomplete in file ", configPath, ".\n", NULL);
        break;
    case InternalErrorMsg: break;
    case WriteErrorMsg: break;
    case WarnMsg: break;
    case ErrorMsg: break;
    case DebugMsg: break;
    case UnknownMsg: break;
    }

    if (pre) {
        msg = xconfigStrcat(pre, b, NULL);
    } else {
        msg = strdup(b);
    }

    /* call back into the host to print the message */

    xconfigPrint(t, msg);


    free(b);
    free(msg);
    if (pre) free(pre);
}
