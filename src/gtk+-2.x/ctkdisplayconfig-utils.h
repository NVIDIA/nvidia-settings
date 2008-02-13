/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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
 
#ifndef __CTK_DISPLAYCONFIG_UTILS_H__
#define __CTK_DISPLAYCONFIG_UTILS_H__

#include <gtk/gtk.h>

#include "ctkdisplaylayout.h"




G_BEGIN_DECLS

/* Token parsing handlers */

typedef struct _ScreenInfo {
    int x;
    int y;
    int width;
    int height;
} ScreenInfo;

void apply_modeline_token(char *token, char *value, void *data);
void apply_metamode_token(char *token, char *value, void *data);
void apply_monitor_token(char *token, char *value, void *data);
void apply_screen_info_token(char *token, char *value, void *data);


/* Mode functions */

nvModePtr mode_parse(nvDisplayPtr display, const char *mode_str);


/* Display functions */

gchar * display_get_type_str(unsigned int device_mask, int be_generic);
int display_find_closest_mode_matching_modeline(nvDisplayPtr display,
                                                nvModeLinePtr modeline);
Bool display_add_modelines_from_server(nvDisplayPtr display, gchar **err_str);
void display_remove_modes(nvDisplayPtr display);


/* Metamode functions */


/* Screen functions */

void screen_remove_display(nvDisplayPtr display);
gchar * screen_get_metamode_str(nvScreenPtr screen, int metamode_idx,
                                int be_generic);


/* GPU functions */

nvDisplayPtr gpu_get_display(nvGpuPtr gpu, unsigned int device_mask);
void gpu_remove_and_free_display(nvDisplayPtr display);
nvDisplayPtr gpu_add_display_from_server(nvGpuPtr gpu,
                                         unsigned int device_mask,
                                         gchar **err_str);

void gpu_remove_and_free_screen(nvScreenPtr screen);
Bool gpu_add_screenless_modes_to_displays(nvGpuPtr gpu);


/* Layout functions */

void layout_free(nvLayoutPtr layout);
nvLayoutPtr layout_load_from_server(NvCtrlAttributeHandle *handle,
                                    gchar **err_str);
nvScreenPtr layout_get_a_screen(nvLayoutPtr layout, nvGpuPtr preferred_gpu);



                   
G_END_DECLS

#endif /* __CTK_DISPLAYCONFIG_UTILS_H__ */
