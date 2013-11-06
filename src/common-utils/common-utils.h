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
#include <sys/types.h>
#include <stdint.h>

#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))

#define NV_MIN(x,y) ((x) < (y) ? (x) : (y))
#define NV_MAX(x,y) ((x) > (y) ? (x) : (y))

#define TAB "  "
#define BIGTAB "      "

#define VERBOSITY_NONE     0 /* nothing */
#define VERBOSITY_ERROR    1 /* errors only */
#define VERBOSITY_DEPRECATED 2 /* errors, deprecation messages and warnings */
#define VERBOSITY_WARNING  3 /* errors and warnings */
#define VERBOSITY_ALL      4 /* errors, warnings and other info */

#define VERBOSITY_DEFAULT  VERBOSITY_ERROR

/*
 * Define a printf format attribute macro.  This definition is based on the one
 * from Xfuncproto.h, available in the 'xproto' package at
 * http://xorg.freedesktop.org/releases/individual/proto/
 */

#if defined(__GNUC__) && ((__GNUC__ * 100 + __GNUC_MINOR__) >= 203)
# define NV_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y)))
#else /* not gcc >= 2.3 */
# define NV_ATTRIBUTE_PRINTF(x,y)
#endif

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
char *nvstrtoupper(char *s);
char *nvstrchrnul(char *s, int c);
char *nvasprintf(const char *fmt, ...) NV_ATTRIBUTE_PRINTF(1, 2);
void nv_append_sprintf(char **buf, const char *fmt, ...) NV_ATTRIBUTE_PRINTF(2, 3);
void nvfree(void *s);

char *tilde_expansion(const char *str);
char *nv_prepend_to_string_list(char *list, const char *item, const char *delim);

TextRows *nv_format_text_rows(const char *prefix,
                              const char *str,
                              int width, int word_boundary);
void nv_text_rows_append(TextRows *t, const char *msg);
void nv_concat_text_rows(TextRows *t0, TextRows *t1);
void nv_free_text_rows(TextRows *t);

void reset_current_terminal_width(unsigned short new_val);

void silence_fmt(int val);
void fmtout(const char *fmt, ...)                                NV_ATTRIBUTE_PRINTF(1, 2);
void fmtoutp(const char *prefix, const char *fmt, ...)           NV_ATTRIBUTE_PRINTF(2, 3);
void fmterr(const char *fmt, ...)                                NV_ATTRIBUTE_PRINTF(1, 2);
void fmtwarn(const char *fmt, ...)                               NV_ATTRIBUTE_PRINTF(1, 2);
void fmt(FILE *stream, const char *prefix, const char *fmt, ...) NV_ATTRIBUTE_PRINTF(3, 4);

char *fget_next_line(FILE *fp, int *eof);

int nv_open(const char *pathname, int flags, mode_t mode);
int nv_get_file_length(const char *filename);
void nv_set_file_length(const char *filename, int fd, int len);
void *nv_mmap(const char *filename, size_t len, int prot, int flags, int fd);
char *nv_basename(const char *path);

char *nv_trim_space(char *string);
char *nv_trim_char(char *string, char trim);
char *nv_trim_char_strict(char *string, char trim);

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
        while (1) {                                             \
            (buf) = nvalloc(current_len);                       \
                                                                \
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
                                                                \
            nvfree(buf);                                        \
        }                                                       \
    }                                                           \
} while (0)

#if defined(__GNUC__)
# define NV_INLINE __inline__
#else
# define NV_INLINE
#endif

/*
 * Simple function which encodes a version number, given as major, minor, micro,
 * and nano, as a 64-bit unsigned integer. This is defined in an inline function
 * rather than as a macro for convenience so it can be examined by the debugger.
 * Encoded version numbers can be compared directly in version checks.
 */
static NV_INLINE uint64_t nv_encode_version(unsigned int major,
                                            unsigned int minor,
                                            unsigned int micro,
                                            unsigned int nano)
{
    return (((uint64_t)(nano  & 0xFFFF)) |
           (((uint64_t)(micro & 0xFFFF)) << 16) |
           (((uint64_t)(minor & 0xFFFF)) << 32) |
           (((uint64_t)(major & 0xFFFF)) << 48));
}

/*
 * Wrapper macros for nv_encode_version(). For K in {2,3,4}, NV_VERSIONK() takes
 * a K-part version number.
 */
#define NV_VERSION2(major, minor)              \
    nv_encode_version(major, minor, 0, 0)
#define NV_VERSION3(major, minor, micro)       \
    nv_encode_version(major, minor, micro, 0)
#define NV_VERSION4(major, minor, micro, nano) \
    nv_encode_version(major, minor, micro, nano)

/*
 * Helper enum that can be used for boolean values that might or might not be
 * set. Care should be taken to avoid simple boolean testing, as a value of
 * NV_OPTIONAL_BOOL_DEFAULT would evaluate as true.
 *
 * The user is responsible for unconditionally initializing the default value of
 * any such booleans to NV_OPTIONAL_BOOL_DEFAULT, before any code path that
 * might optionally set their values is executed.
 */

typedef enum {
    NV_OPTIONAL_BOOL_DEFAULT = -1,
    NV_OPTIONAL_BOOL_FALSE   = FALSE,
    NV_OPTIONAL_BOOL_TRUE    = TRUE
} NVOptionalBool;

#endif /* __COMMON_UTILS_H__ */
