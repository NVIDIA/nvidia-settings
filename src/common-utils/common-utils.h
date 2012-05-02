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

#ifndef __COMMON_UTILS_H__
#define __COMMON_UTILS_H__

#include <stdio.h>
#include <stdarg.h>

#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))

#define TAB "  "
#define BIGTAB "      "

typedef struct {
    char **t; /* the text rows */
    int n;    /* number of rows */
    int m;    /* maximum row length */
} TextRows;

void *nvalloc(size_t size);
char *nvstrcat(const char *str, ...);
void *nvrealloc(void *ptr, size_t size);
char *nvstrdup(const char *s);
char *nvstrndup(const char *s, size_t n);
char *nvstrtolower(char *s);
void nvfree(void *s);

char *tilde_expansion(const char *str);

TextRows *nv_format_text_rows(const char *prefix,
                              const char *str,
                              int width, int word_boundary);
void nv_text_rows_append(TextRows *t, const char *msg);
void nv_concat_text_rows(TextRows *t0, TextRows *t1);
void nv_free_text_rows(TextRows *t);

void reset_current_terminal_width(unsigned short new_val);

void silence_fmt(int val);
void fmtout(const char *fmt, ...);
void fmtoutp(const char *prefix, const char *fmt, ...);
void fmterr(const char *fmt, ...);
void fmtwarn(const char *fmt, ...);
void fmt(FILE *stream, const char *prefix, const char *fmt, ...);

/*
 * NV_VSNPRINTF(): macro that assigns buf using vsnprintf().  This is
 * correct for differing semantics of the vsnprintf() return value:
 *
 * -1 when the buffer is not long enough (glibc < 2.1)
 *
 *   or
 *
 * the length the string would have been if the buffer had been large
 * enough (glibc >= 2.1)
 *
 * This macro allocates memory for buf; the caller should free it when
 * done.
 */

#define NV_FMT_BUF_LEN 256

#define NV_VSNPRINTF(buf, fmt)                                  \
do {                                                            \
    if (!fmt) {                                                 \
        (buf) = NULL;                                           \
    } else {                                                    \
        va_list ap;                                             \
        int len, current_len = NV_FMT_BUF_LEN;                  \
                                                                \
        (buf) = malloc(current_len);                            \
                                                                \
        while (1) {                                             \
            va_start(ap, fmt);                                  \
            len = vsnprintf((buf), current_len, (fmt), ap);     \
            va_end(ap);                                         \
                                                                \
            if ((len > -1) && (len < current_len)) {            \
                break;                                          \
            } else if (len > -1) {                              \
                current_len = len + 1;                          \
            } else {                                            \
                current_len += NV_FMT_BUF_LEN;                  \
            }                                                   \
            free(buf);                                          \
            (buf) = malloc(current_len);                        \
        }                                                       \
    }                                                           \
} while (0)

#endif /* __COMMON_UTILS_H__ */
