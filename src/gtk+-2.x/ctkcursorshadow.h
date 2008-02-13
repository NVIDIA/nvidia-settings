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

#ifndef __CTK_CURSOR_SHADOW_H__
#define __CTK_CURSOR_SHADOW_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS

#define CTK_TYPE_CURSOR_SHADOW (ctk_cursor_shadow_get_type())

#define CTK_CURSOR_SHADOW(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CURSOR_SHADOW, \
                                 CtkCursorShadow))

#define CTK_CURSOR_SHADOW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CURSOR_SHADOW, \
                              CtkCursorShadowClass))

#define CTK_IS_CURSOR_SHADOW(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CURSOR_SHADOW))

#define CTK_IS_CURSOR_SHADOW_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CURSOR_SHADOW))

#define CTK_CURSOR_SHADOW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CURSOR_SHADOW, \
                                CtkCursorShadowClass))


typedef struct _CtkCursorShadow       CtkCursorShadow;
typedef struct _CtkCursorShadowClass  CtkCursorShadowClass;

struct _CtkCursorShadow
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    GtkWidget *scales[3];
    GtkWidget *reset_button;
    GtkWidget *color_selector_check_button;
    GtkWidget *cursor_shadow_check_button;
    GtkWidget *color_selector;
    GtkWidget *color_selector_window;
    gboolean reset_button_sensitivity;
    NVCTRLAttributeValidValuesRec red_range;
    NVCTRLAttributeValidValuesRec green_range;
    NVCTRLAttributeValidValuesRec blue_range;
};

struct _CtkCursorShadowClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_cursor_shadow_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_cursor_shadow_new       (NvCtrlAttributeHandle *handle,
                                         CtkConfig *ctk_config,
                                         CtkEvent *ctk_event);

GtkTextBuffer *ctk_cursor_shadow_create_help(GtkTextTagTable *,
                                             CtkCursorShadow *);

G_END_DECLS

#endif /* __CTK_CURSOR_SHADOW_H__ */

