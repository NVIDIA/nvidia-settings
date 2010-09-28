/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#ifndef __CTK_DITHERING_CONTROLS_H__
#define __CTK_DITHERING_CONTROLS_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_DITHERING_CONTROLS (ctk_dithering_controls_get_type())

#define CTK_DITHERING_CONTROLS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DITHERING_CONTROLS, \
                                 CtkDitheringControls))

#define CTK_DITHERING_CONTROLS_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DITHERING_CONTROLS, \
                              CtkDitheringControlsClass))

#define CTK_IS_DITHERING_CONTROLS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DITHERING_CONTROLS))

#define CTK_IS_DITHERING_CONTROLS_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DITHERING_CONTROLS))

#define CTK_DITHERING_CONTROLS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DITHERING_CONTROLS, \
                                CtkDitheringControlsClass))

typedef struct _CtkDitheringControls       CtkDitheringControls;
typedef struct _CtkDitheringControlsClass  CtkDitheringControlsClass;

struct _CtkDitheringControls
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    GtkWidget *reset_button;
    GtkWidget *dithering_controls_main;
    GtkWidget *dithering_mode_box;

    GtkWidget *dithering_config_menu;
    GtkWidget *dithering_mode_menu;
    GtkWidget *dithering_current_config;
    GtkWidget *dithering_current_mode;

    gint display_device_mask;
    gint *dithering_mode_table;
    gint dithering_mode_table_size;
    gint default_dithering_config;
    gint default_dithering_mode;
    char *name;
};

struct _CtkDitheringControlsClass
{
    GtkVBoxClass parent_class;
};

GType ctk_dithering_controls_get_type (void) G_GNUC_CONST;
GtkWidget* ctk_dithering_controls_new (NvCtrlAttributeHandle *,
                                       CtkConfig *, CtkEvent *,
                                       GtkWidget *,
                                       unsigned int display_device_mask,
                                       char *);

void ctk_dithering_controls_reset (CtkDitheringControls*);
void ctk_dithering_controls_setup (CtkDitheringControls*);
void add_dithering_controls_help  (CtkDitheringControls*, GtkTextBuffer *b,
                                   GtkTextIter *i);

G_END_DECLS

#endif /* __CTK_DITHERING_CONTROLS_H__ */
