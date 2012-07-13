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

/*
 * query-assign.h - prototypes for querying and assigning
 * attribute values.
 */

#ifndef __QUERY_ASSIGN_H__
#define __QUERY_ASSIGN_H__

#include "NvCtrlAttributes.h"

#include "parse.h"
#include "command-line.h"

enum {
    NV_DPY_PROTO_NAME_TYPE_BASENAME = 0,
    NV_DPY_PROTO_NAME_TYPE_ID,
    NV_DPY_PROTO_NAME_DP_GUID,
    NV_DPY_PROTO_NAME_EDID_HASH,
    NV_DPY_PROTO_NAME_TARGET_INDEX,
    NV_DPY_PROTO_NAME_RANDR,
    NV_DPY_PROTO_NAME_MAX,
};

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
    char *name;               /* Name for this target */
    char *protoNames[NV_DPY_PROTO_NAME_MAX];  /* List of valid names for this target */
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
