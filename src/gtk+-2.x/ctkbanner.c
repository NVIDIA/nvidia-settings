/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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

#include <gtk/gtk.h>

#include "image.h"
#include "ctkbanner.h"

static void
ctk_banner_class_init    (CtkBannerClass *);

static void
ctk_banner_finalize      (GObject *);

static gboolean
ctk_banner_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_banner_size_request  (GtkWidget *, GtkRequisition *);

static gboolean
ctk_banner_configure_event  (GtkWidget *, GdkEventConfigure *);

static GObjectClass *parent_class;


GType ctk_banner_get_type(
    void
)
{
    static GType ctk_banner_type = 0;

    if (!ctk_banner_type) {
        static const GTypeInfo ctk_banner_info = {
            sizeof (CtkBannerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_banner_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkBanner),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_banner_type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                        "CtkBanner", &ctk_banner_info, 0);
    }

    return ctk_banner_type;
}

static void ctk_banner_class_init(
    CtkBannerClass *ctk_banner_class
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_banner_class;
    gobject_class = (GObjectClass *) ctk_banner_class;

    parent_class = g_type_class_peek_parent(ctk_banner_class);

    gobject_class->finalize = ctk_banner_finalize;

    widget_class->expose_event = ctk_banner_expose_event;
    widget_class->size_request = ctk_banner_size_request;
    widget_class->configure_event = ctk_banner_configure_event;
}

static void ctk_banner_finalize(
    GObject *object
)
{
    CtkBanner *ctk_banner = CTK_BANNER(object);

    g_object_unref(ctk_banner->gdk_img_pixbuf);
    g_object_unref(ctk_banner->gdk_img_fill_pixbuf);

    if (ctk_banner->gdk_pixbuf)
        g_object_unref(ctk_banner->gdk_pixbuf);

    free_decompressed_image(ctk_banner->image_data, NULL);
}

static gboolean ctk_banner_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);

    gdk_draw_pixbuf(widget->window,
                    widget->style->fg_gc[GTK_STATE_NORMAL],
                    ctk_banner->gdk_pixbuf,
                    0, 0, 0, 0, -1, -1,
                    GDK_RGB_DITHER_NORMAL, 0, 0);

    return FALSE;
}

static gboolean ctk_banner_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);
    gboolean has_alpha = FALSE;

    if ((event->width < ctk_banner->img->width) ||
            (event->height < ctk_banner->img->height))
        return FALSE;

    if (ctk_banner->gdk_pixbuf)
        g_object_unref(ctk_banner->gdk_pixbuf);

    /* RGBA */
    if (ctk_banner->img->bytes_per_pixel == 4)
        has_alpha = TRUE;

    ctk_banner->gdk_pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8,
                       MAX(event->width, ctk_banner->img->width),
                       MAX(event->height, ctk_banner->img->height));

    gdk_pixbuf_copy_area(ctk_banner->gdk_img_pixbuf,
                         0, 0, ctk_banner->img->fill_column_index,
                         ctk_banner->img->height,
                         ctk_banner->gdk_pixbuf,
                         0, 0);

    gdk_pixbuf_scale(ctk_banner->gdk_img_fill_pixbuf,
                     ctk_banner->gdk_pixbuf,
                     ctk_banner->img->fill_column_index, 0,
                     MAX((event->width - ctk_banner->img->width), 1),
                     ctk_banner->img->height, 0.0f, 0.0f,
                     (ctk_banner->img->fill_column_index +
                      MAX((event->width - ctk_banner->img->width), 1)),
                     1.0f, GDK_INTERP_NEAREST);

    gdk_pixbuf_copy_area(ctk_banner->gdk_img_pixbuf,
                         ctk_banner->img->fill_column_index, 0,
                         ctk_banner->img->fill_column_index,
                         ctk_banner->img->height,
                         ctk_banner->gdk_pixbuf,
                         (gdk_pixbuf_get_width(ctk_banner->gdk_pixbuf) -
                          ctk_banner->img->fill_column_index),
                         0);

    return FALSE;
}

static void ctk_banner_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);

    requisition->width  = ctk_banner->img->width;
    requisition->height = ctk_banner->img->height;
}

GtkWidget* ctk_banner_new(const nv_image_t *img)
{
    GObject *object;
    CtkBanner *ctk_banner;
    guint8 *image_data;
    gboolean has_alpha = FALSE;

    if (!img->fill_column_index) return NULL;

    image_data = decompress_image_data(img);
    if (!image_data) return NULL;

    object = g_object_new(CTK_TYPE_BANNER, NULL);

    ctk_banner = CTK_BANNER(object);

    ctk_banner->image_data = image_data;

    ctk_banner->img = img;
    ctk_banner->gdk_pixbuf = NULL;

    /* RGBA */
    if (ctk_banner->img->bytes_per_pixel == 4)
        has_alpha = TRUE;

    ctk_banner->gdk_img_pixbuf =
        gdk_pixbuf_new_from_data(image_data, GDK_COLORSPACE_RGB,
                                 has_alpha, 8, img->width, img->height,
                                 img->width * img->bytes_per_pixel,
                                 NULL, NULL);

    ctk_banner->gdk_img_fill_pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, has_alpha, 8,
                       1, ctk_banner->img->height);

    gdk_pixbuf_copy_area(ctk_banner->gdk_img_pixbuf, 
                         ctk_banner->img->fill_column_index, 0, 1,
                         ctk_banner->img->height,
                         ctk_banner->gdk_img_fill_pixbuf,
                         0, 0);

    return GTK_WIDGET(object);
}
