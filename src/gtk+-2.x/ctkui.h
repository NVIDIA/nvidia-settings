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

#ifndef __CTK_UI_H__
#define __CTK_UI_H__

#include "NvCtrlAttributes.h"
#include "parse.h"
#include "config-file.h"

int ctk_init_check(int *argc, char **argv[]);

char *ctk_get_display(void);

void ctk_main(ParsedAttribute*,
              ConfigProperties*,
              CtrlSystem*,
              const char *page);


#endif /* __CTK_UI_H__ */
