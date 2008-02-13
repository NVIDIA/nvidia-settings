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
 * The CtrlHandles struct contains an array of target types for an X
 * server.  For each target type, we store the number of those targets
 * on this X server.  Per target, we store a NvCtrlAttributeHandle, a
 * bitmask of what display devices are enabled on that target, and a
 * string description of that target.
 */

typedef struct {
    NvCtrlAttributeHandle *h; /* handle for this target */
    uint32 d;                 /* display device mask for this target */
    uint32 c;                 /* Connected display device mask for target */
    char *name;               /* name for this target */
} CtrlHandleTarget;

typedef struct {
    int n;                    /* number of targets */
    CtrlHandleTarget *t;      /* dynamically allocated array of targets */
} CtrlHandleTargets;

typedef struct {
    char *display;            /* string for XOpenDisplay */
    Display *dpy;             /* X display connection */
    CtrlHandleTargets targets[MAX_TARGET_TYPES];
} CtrlHandles;


int nv_process_assignments_and_queries(Options *op);

CtrlHandles *nv_alloc_ctrl_handles(const char *display);
void nv_free_ctrl_handles(CtrlHandles *h);

int nv_process_parsed_attribute(ParsedAttribute*, CtrlHandles *h,
                                int, int, char*, ...);

#endif /* __QUERY_ASSIGN_H__ */
