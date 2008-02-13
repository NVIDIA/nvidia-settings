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

#ifndef __CTK_GAUGE_H__
#define __CTK_GAUGE_H__

G_BEGIN_DECLS

#define CTK_TYPE_GAUGE (ctk_gauge_get_type())

#define CTK_GAUGE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GAUGE, CtkGauge))

#define CTK_GAUGE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GAUGE, CtkGaugeClass))

#define CTK_IS_GAUGE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GAUGE))

#define CTK_IS_GAUGE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GAUGE))

#define CTK_GAUGE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GAUGE, CtkGaugeClass))


typedef struct _CtkGauge       CtkGauge;
typedef struct _CtkGaugeClass  CtkGaugeClass;

struct _CtkGauge
{
    GtkDrawingArea parent;

    gint lower, upper;
    gint current;

    GdkColormap *gdk_colormap;

    GdkColor gdk_color_gray;
    GdkColor gdk_color_red;
    GdkColor gdk_color_yellow;
    GdkColor gdk_color_green;

    GdkPixmap *gdk_pixmap;
    GdkGC *gdk_gc;

    PangoLayout *pango_layout;

    gint width, height;
};

struct _CtkGaugeClass
{
    GtkDrawingAreaClass parent_class;
};

GType       ctk_gauge_get_type     (void) G_GNUC_CONST;
GtkWidget*  ctk_gauge_new          (gint, gint);
void        ctk_gauge_set_current  (CtkGauge *, gint);
void        ctk_gauge_draw         (CtkGauge *);

G_END_DECLS

#endif /* __CTK_GAUGE_H__ */

