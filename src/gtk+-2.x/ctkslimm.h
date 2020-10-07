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

#ifndef __CTK_SLIMM_H__
#define __CTK_SLIMM_H__

#include <gtk/gtk.h>

#include "ctkevent.h"
#include "ctkdisplaylayout.h"
#include "ctkconfig.h"
#include "ctkdisplayconfig-utils.h"

#include "XF86Config-parser/xf86Parser.h"

typedef struct GridConfigRec {
    int rows;
    int columns;
} GridConfig;

typedef struct nvModeLineItemRec {
    nvModeLinePtr modeline;
    struct nvModeLineItemRec *next;
} nvModeLineItem, *nvModeLineItemPtr;

typedef struct _CtkMMDialog
{
    GtkWidget *parent;
    nvLayoutPtr layout;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;

    GtkWidget *dialog;
    gboolean is_active;

    GtkWidget *mnu_display_config;
    GtkWidget *mnu_display_resolution;
    GtkWidget *mnu_display_refresh;
    GtkWidget *spbtn_hedge_overlap;
    GtkWidget *spbtn_vedge_overlap;
    GtkWidget *lbl_total_size;
    GtkWidget *box_total_size;
    GtkWidget *chk_all_displays;
    nvModeLinePtr *resolution_table;
    nvModeLinePtr *refresh_table;
    int resolution_table_len;
    int refresh_table_len;
    int cur_resolution_table_idx;
    gint h_overlap_parsed, v_overlap_parsed;
    gboolean mnu_refresh_disabled;
    nvModeLineItemPtr modelines;
    nvModeLinePtr cur_modeline;
    gint num_modelines;
    int num_displays;
    int parsed_rows;
    int parsed_cols;

    int max_screen_width;
    int max_screen_height;

    gint x_displays, y_displays;
    gint resolution_idx, refresh_idx;
    gint h_overlap, v_overlap;

    /**
     * The grid_configs array enumerates the display grid configurations
     * that are presently supported.
     **/
    GridConfig *grid_configs;
    int num_grid_configs;

} CtkMMDialog;

CtkMMDialog *create_mosaic_dialog(GtkWidget *parent, CtrlTarget *ctrl_target,
                                  CtkConfig *ctk_config, nvLayoutPtr layout);
int run_mosaic_dialog(CtkMMDialog *dialog, GtkWidget *parent,
                      nvLayoutPtr layout);

void ctk_mmdialog_insert_help(GtkTextBuffer *b, GtkTextIter *i);
void update_mosaic_dialog_ui(CtkMMDialog *ctk_mmdialog, nvLayoutPtr layout);

#endif /* __CTK_SLIMM_H__ */

