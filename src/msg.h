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

#ifndef __MSG_H__
#define __MSG_H__

#include <stdarg.h>
#include <stdio.h>

void  nv_error_msg(const char*, ...);
void  nv_warning_msg(const char*, ...);
void  nv_info_msg(const char*, const char*, ...);
void  nv_msg(const char*, const char*, ...);
void  nv_msg_preserve_whitespace(const char*, const char*, ...);

/*
 * NV_VSNPRINTF(): macro that assigns buf using vsnprintf().  This is
 * correct for differing semantics of vsnprintf() in different
 * versions of glibc:
 *
 * different semantics of the return value from (v)snprintf:
 *
 * -1 when the buffer is not long enough (glibc < 2.1)
 *
 *   or
 *
 * the length the string would have been if the buffer had been large
 * enough (glibc >= 2.1)
 *
 * This macro allocates memory for buf; the caller should use free()
 * when done.
 */
#define NV_FMT_BUF_LEN 256
#define NV_VSNPRINTF(buf, fmt)                          \
do {                                                    \
    va_list ap;                                         \
    int len, current_len = NV_FMT_BUF_LEN;              \
                                                        \
    (buf) = malloc(current_len);                        \
                                                        \
    while (1) {                                         \
        va_start(ap, fmt);                              \
        len = vsnprintf((buf), current_len, (fmt), ap); \
        va_end(ap);                                     \
                                                        \
        if ((len > -1) && (len < current_len)) {        \
            break;                                      \
        } else if (len > -1) {                          \
            current_len = len + 1;                      \
        } else {                                        \
            current_len += NV_FMT_BUF_LEN;              \
        }                                               \
                                                        \
        (buf) = realloc(buf, current_len);              \
    }                                                   \
} while (0)

#endif /* __MSG_H__ */
