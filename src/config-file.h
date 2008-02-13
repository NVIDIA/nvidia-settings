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

#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#include "query-assign.h"


/*
 * The ConfigProperties structure contains additional configuration
 * data to be written to the rc file; these are configuration
 * properties of the nvidia-settings utilities itself, rather than
 * properties of the X screen(s) that nvidia-settings is configuring.
 */

#define CONFIG_PROPERTIES_TOOLTIPS                            (1<<0)
#define CONFIG_PROPERTIES_DISPLAY_STATUS_BAR                  (1<<1)
#define CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES                 (1<<2)
#define CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE (1<<3)
#define CONFIG_PROPERTIES_SHOW_QUIT_DIALOG                    (1<<4)

typedef struct {
    unsigned int booleans;
} ConfigProperties;


int nv_read_config_file(const char *, const char *,
                        ParsedAttribute *, ConfigProperties *);

int nv_write_config_file(const char *, CtrlHandles *,
                         ParsedAttribute *, ConfigProperties *);

#endif /* __CONFIG_FILE_H__ */
