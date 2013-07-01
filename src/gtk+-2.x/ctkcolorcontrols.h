/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#ifndef __CTK_COLOR_CONTROLS_H__
#define __CTK_COLOR_CONTROLS_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_CONTROLS (ctk_color_controls_get_type())

#define CTK_COLOR_CONTROLS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_COLOR_CONTROLS, \
                                 CtkColorControls))

#define CTK_COLOR_CONTROLS_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_COLOR_CONTROLS, \
                              CtkColorControlsClass))

#define CTK_IS_COLOR_CONTROLS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_COLOR_CONTROLS))

#define CTK_IS_COLOR_CONTROLS_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_COLOR_CONTROLS))

#define CTK_COLOR_CONTROLS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_COLOR_CONTROLS, \
                                CtkCOLORControlsClass))

typedef struct _CtkColorControls       CtkColorControls;
typedef struct _CtkColorControlsClass  CtkColorControlsClass;

struct _CtkColorControls
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    GtkWidget *reset_button;
    GtkWidget *color_controls_box;

    GtkWidget *color_range_menu;
    GtkWidget *color_space_menu;

    gint *color_space_table;
    gint color_space_table_size;
    gint *color_range_table;
    gint color_range_table_size;
    gint default_color_config;
    gint default_color_space;
    char *name;
};

struct _CtkColorControlsClass
{
    GtkVBoxClass parent_class;
};

GType ctk_color_controls_get_type (void) G_GNUC_CONST;
GtkWidget* ctk_color_controls_new (NvCtrlAttributeHandle *,
                                   CtkConfig *, CtkEvent *,
                                   GtkWidget *,
                                   char *);

void ctk_color_controls_reset (CtkColorControls*);
void ctk_color_controls_setup (CtkColorControls*);
void add_color_controls_help  (CtkColorControls*, GtkTextBuffer *b,
                                   GtkTextIter *i);

G_END_DECLS

#endif /* __CTK_COLOR_CONTROLS_H__ */
