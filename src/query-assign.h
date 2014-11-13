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


int nv_process_assignments_and_queries(const Options *op,
                                       CtrlSystemList *systems);

int nv_process_parsed_attribute(const Options *op,
                                ParsedAttribute*, CtrlSystem *system,
                                int, int, char*, ...) NV_ATTRIBUTE_PRINTF(6, 7);



#endif /* __QUERY_ASSIGN_H__ */
