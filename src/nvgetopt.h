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

#ifndef __NVGETOPT_H__
#define __NVGETOPT_H__

#define NVGETOPT_FALSE 0
#define NVGETOPT_TRUE 1
#define NVGETOPT_INVALID 2

#define NVGETOPT_HAS_ARGUMENT 0x1
#define NVGETOPT_IS_BOOLEAN   0x2

typedef struct {
    const char *name;
    int val;
    unsigned int flags;
    void (*print_description)(void); /* not used by nvgetopt() */
    char *description; /* not used by nvgetopt() */
} NVGetoptOption;

int nvgetopt(int argc, char *argv[], const NVGetoptOption *options,
             char **strval, int *boolval);

#endif /* __NVGETOPT_H__ */
