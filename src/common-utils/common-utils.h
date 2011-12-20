/*
 * Copyright (C) 2010 NVIDIA Corporation
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

#if !defined(TRUE)
#define TRUE 1
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

void *nvalloc(size_t size);
char *nvstrcat(const char *str, ...);
void *nvrealloc(void *ptr, size_t size);
char *nvstrdup(const char *s);
char *nvstrtolower(char *s);
void nvfree(void *s);

char *tilde_expansion(const char *str);

#endif /* __COMMON_UTILS_H__ */
