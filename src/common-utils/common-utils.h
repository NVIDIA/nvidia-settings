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

#include "msg.h"

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

char *fget_next_line(FILE *fp, int *eof);

int nv_open(const char *pathname, int flags, mode_t mode);
int nv_get_file_length(const char *filename);
void nv_set_file_length(const char *filename, int fd, int len);
void *nv_mmap(const char *filename, size_t len, int prot, int flags, int fd);
char *nv_basename(const char *path);
int nv_mkdir_recursive(const char *path, const mode_t mode,
                       char **error_str, char **log_str);

char *nv_trim_space(char *string);
char *nv_trim_char(char *string, char trim);
char *nv_trim_char_strict(char *string, char trim);
void remove_trailing_slashes(char *string);

int directory_exists(const char *dir);

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
