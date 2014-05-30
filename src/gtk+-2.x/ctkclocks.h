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

#ifndef __CTK_CLOCKS_H__
#define __CTK_CLOCKS_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_CLOCKS (ctk_clocks_get_type())

#define CTK_CLOCKS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CLOCKS, CtkClocks))

#define CTK_CLOCKS_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CLOCKS, CtkClocksClass))

#define CTK_IS_CLOCKS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CLOCKS))

#define CTK_IS_CLOCKS_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CLOCKS))

#define CTK_CLOCKS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CLOCKS, CtkClocksClass))

#define CLOCKS_NONE 0
#define CLOCKS_2D   2
#define CLOCKS_3D   3


typedef struct _CtkClocks       CtkClocks;
typedef struct _CtkClocksClass  CtkClocksClass;

struct _CtkClocks
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *detect_dialog;

    GtkWidget *enable_checkbox;  /* Overclocking available */
    GtkWidget *clock_menu;       /* 2D/3D dropdown selector */

    GtkWidget *gpu_clk_scale;    /* Current 2D or 3D clock sliders */
    GtkWidget *mem_clk_scale;

    GtkWidget *apply_button;     /* Apply target clock frequencies */
    GtkWidget *detect_button;    /* Auto detects best 3D clock frequencies */
    GtkWidget *reset_button;     /* Reset hardware default frequencies */

    int clocks_being_modified;   /* Wether we're editing the 2D or 3D clocks */
    Bool clocks_modified;        /* The clocks were modified by the user */
    Bool clocks_moved;           /* The clock sliders were moved by the user */

    Bool overclocking_enabled;   /* Overclocking is enabled */
    Bool auto_detection_available;  /* Optimal clock detection is available */
    Bool probing_optimal;        /* Optimal clocks being probed */
};

struct _CtkClocksClass
{
    GtkVBoxClass parent_class;
};

GType      ctk_clocks_get_type (void) G_GNUC_CONST;
GtkWidget *ctk_clocks_new      (NvCtrlAttributeHandle *, CtkConfig *,
                               CtkEvent *);

GtkTextBuffer *ctk_clocks_create_help (GtkTextTagTable *, CtkClocks *);

void ctk_clocks_select (GtkWidget *widget);

G_END_DECLS

#endif /* __CTK_CLOCKS_H__ */
