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

#ifndef __CTK_SCALE_H__
#define __CTK_SCALE_H__

#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_SCALE (ctk_scale_get_type())

#define CTK_SCALE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SCALE, CtkScale))

#define CTK_SCALE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SCALE, CtkScaleClass))

#define CTK_IS_SCALE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SCALE))

#define CTK_IS_SCALE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SCALE))

#define CTK_SCALE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SCALE, CtkScaleClass))


/* the widget within the CtkScale that to which tooltips should be attached */

#define CTK_SCALE_TOOLTIP_WIDGET(obj) \
    ((CTK_SCALE(obj))->tooltip_widget)

typedef struct _CtkScale       CtkScale;
typedef struct _CtkScaleClass  CtkScaleClass;

struct _CtkScale
{
    GtkVBox parent;

    GtkAdjustment *gtk_adjustment;
    const gchar   *label;
    GtkWidget     *gtk_scale;
    GtkWidget     *text_entry;
    gboolean       text_entry_packed;
    GtkWidget     *text_entry_container;
    GtkWidget     *tooltip_widget;
    CtkConfig     *ctk_config;
    gint           value_type;
};

struct _CtkScaleClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_scale_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_scale_new       (GtkAdjustment *, const gchar *,
                                 CtkConfig *, gint);

G_END_DECLS

#endif /* __CTK_SCALE_H__ */

