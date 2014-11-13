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

#ifndef __CTK_CURVE_H__
#define __CTK_CURVE_H__

#include "ctkutils.h"

G_BEGIN_DECLS

#define CTK_TYPE_CURVE (ctk_curve_get_type())

#define CTK_CURVE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CURVE, CtkCurve))

#define CTK_CURVE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CURVE, CtkCurveClass))

#define CTK_IS_CURVE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CURVE))

#define CTK_IS_CURVE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CURVE))

#define CTK_CURVE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CURVE, CtkCurveClass))


typedef struct _CtkCurve       CtkCurve;
typedef struct _CtkCurveClass  CtkCurveClass;

struct _CtkCurve
{
    GtkDrawingArea parent;

    CtrlTarget *ctrl_target;
    GtkWidget *color;

#ifdef CTK_GTK3
    cairo_surface_t *c_surface;
    cairo_t *c_context;
#else
    GdkColor gdk_color_red;
    GdkColor gdk_color_green;
    GdkColor gdk_color_blue;

    GdkColormap *gdk_colormap;

    GdkPixmap *gdk_pixmap;
    GdkGC *gdk_gc;
#endif
    gint width;
    gint height;
};

struct _CtkCurveClass
{
    GtkDrawingAreaClass parent_class;
};

GType       ctk_curve_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_curve_new       (CtrlTarget *, GtkWidget *);
void        ctk_curve_color_changed(GtkWidget *);

G_END_DECLS

#endif /* __CTK_CURVE_H__ */

