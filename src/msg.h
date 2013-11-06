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

#ifndef __MSG_H__
#define __MSG_H__

#include <stdarg.h>
#include <stdio.h>

#include "common-utils.h"

void nv_error_msg(const char*, ...)                            NV_ATTRIBUTE_PRINTF(1, 2);
void nv_deprecated_msg(const char*, ...)                       NV_ATTRIBUTE_PRINTF(1, 2);
void nv_warning_msg(const char*, ...)                          NV_ATTRIBUTE_PRINTF(1, 2);
void nv_info_msg(const char*, const char*, ...)                NV_ATTRIBUTE_PRINTF(2, 3);
void nv_msg(const char*, const char*, ...)                     NV_ATTRIBUTE_PRINTF(2, 3);
void nv_msg_preserve_whitespace(const char*, const char*, ...) NV_ATTRIBUTE_PRINTF(2, 3);

#endif /* __MSG_H__ */
