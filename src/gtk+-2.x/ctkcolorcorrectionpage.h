/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2012 NVIDIA Corporation.
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

#ifndef __CTK_COLOR_CORRECTION_PAGE_H__
#define __CTK_COLOR_CORRECTION_PAGE_H__

#include "parse.h"
#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_COLOR_CORRECTION_PAGE (ctk_color_correction_page_get_type())

#define CTK_COLOR_CORRECTION_PAGE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_COLOR_CORRECTION_PAGE, \
                                CtkColorCorrectionPage))

#define CTK_COLOR_CORRECTION_PAGE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_COLOR_CORRECTION_PAGE, \
                             CtkColorCorrectionPageClass))

#define CTK_IS_COLOR_CORRECTION_PAGE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_COLOR_CORRECTION_PAGE))

#define CTK_IS_COLOR_CORRECTION_PAGE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_COLOR_CORRECTION_PAGE))

#define CTK_COLOR_CORRECTION_PAGE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_COLOR_CORRECTION_PAGE, \
                               CtkColorCorrectionPageClass))


typedef struct _CtkColorCorrectionPage       CtkColorCorrectionPage;
typedef struct _CtkColorCorrectionPageClass  CtkColorCorrectionPageClass;

struct _CtkColorCorrectionPage
{
    GtkVBox parent;
    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    GtkWidget *option_menu;
    GtkAdjustment *brightness_adjustment;
    GtkAdjustment *contrast_adjustment;
    GtkAdjustment *gamma_adjustment;
    GtkWidget *confirm_button;
    GtkWidget *confirm_label;
    gint confirm_countdown;
    guint confirm_timer;
    gfloat cur_slider_val[3][4];  // as [attribute][channel]
    gfloat prev_slider_val[3][4]; // as [attribute][channel]
    guint enabled_display_devices;
};

struct _CtkColorCorrectionPageClass
{
    GtkVBoxClass parent_class;

    void (*changed) (CtkColorCorrectionPage *);
};

GType       ctk_color_correction_page_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_color_correction_page_new       (NvCtrlAttributeHandle *,
                                                 CtkConfig *, ParsedAttribute *,
                                                 CtkEvent *);
GtkTextBuffer *ctk_color_correction_page_create_help(GtkTextTagTable *);

G_END_DECLS

#endif /* __CTK_COLOR_CORRECTION_PAGE_H__ */

