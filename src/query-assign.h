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

/*
 * query-assign.h - prototypes for querying and assigning
 * attribute values.
 */

#ifndef __QUERY_ASSIGN_H__
#define __QUERY_ASSIGN_H__

#include "NvCtrlAttributes.h"

#include "parse.h"
#include "command-line.h"


/*
 * The CtrlHandles struct contains an array of NvCtrlAttributeHandle's
 * (one per X screen on this X server), as well as the number of X
 * screens, an array of enabled display devices for each screen, and a
 * string description of each screen.
 */

typedef struct {
    char *display;
    Display *dpy;
    int num_screens;
    NvCtrlAttributeHandle **h;
    uint32 *d;
    char **screen_names;
} CtrlHandles;


int nv_process_assignments_and_queries(Options *op);

uint32 *nv_get_enabled_display_devices(int, NvCtrlAttributeHandle**);
CtrlHandles *nv_alloc_ctrl_handles(const char *display);
void nv_free_ctrl_handles(CtrlHandles *h);

int nv_process_parsed_attribute(ParsedAttribute*, CtrlHandles *h,
                                int, int, char*, ...);

#endif /* __QUERY_ASSIGN_H__ */
