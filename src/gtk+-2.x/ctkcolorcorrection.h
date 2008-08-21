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

#ifndef __CTK_COLOR_CORRECTION_H__
#define __CTK_COLOR_CORRECTION_H__

#include "parse.h"
#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_CORRECTION (ctk_color_correction_get_type())

#define CTK_COLOR_CORRECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_COLOR_CORRECTION, \
                                CtkColorCorrection))

#define CTK_COLOR_CORRECTION_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_COLOR_CORRECTION, \
                             CtkColorCorrectionClass))

#define CTK_IS_COLOR_CORRECTION(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_COLOR_CORRECTION))

#define CTK_IS_COLOR_CORRECTION_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_COLOR_CORRECTION))

#define CTK_COLOR_CORRECTION_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_COLOR_CORRECTION, \
                               CtkColorCorrectionClass))


typedef struct _CtkColorCorrection       CtkColorCorrection;
typedef struct _CtkColorCorrectionClass  CtkColorCorrectionClass;

struct _CtkColorCorrection
{
    GtkVBox parent;
    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    GtkWidget *option_menu;
    GtkObject *brightness_adjustment;
    GtkObject *contrast_adjustment;
    GtkObject *gamma_adjustment;
    GtkWidget *confirm_button;
    gint confirm_countdown;
    guint confirm_timer;
    gfloat cur_val[3][3];  // as [attribute][channel]
    gfloat prev_val[3][3]; // as [attribute][channel]
    guint enabled_display_devices;
};

struct _CtkColorCorrectionClass
{
    GtkVBoxClass parent_class;

    void (*changed) (CtkColorCorrection *);
};

GType       ctk_color_correction_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_color_correction_new       (NvCtrlAttributeHandle *,
                                            CtkConfig *, ParsedAttribute *,
                                            CtkEvent *);
GtkTextBuffer *ctk_color_correction_create_help(GtkTextTagTable *);

G_END_DECLS

#endif /* __CTK_COLOR_CORRECTION_H__ */

