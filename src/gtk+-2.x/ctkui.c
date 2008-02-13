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

#include "ctkui.h"
#include "ctkwindow.h"

#include <gtk/gtk.h>

/*
 * This source file provides thin wrappers over the gtk routines, so
 * that nvidia-settings.c doesn't need to include gtk+
 */

int ctk_init_check(int *argc, char **argv[])
{
    return gtk_init_check(argc, argv);
}

char *ctk_get_display(void)
{
    return gdk_get_display();
}

void ctk_main(NvCtrlAttributeHandle **attribute_handles,
              int num_screens, ParsedAttribute *p, ConfigProperties *conf)
{
    ctk_window_new(attribute_handles, num_screens, p, conf);
    gtk_main();
}
