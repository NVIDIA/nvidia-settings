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

#include <gtk/gtk.h>
#include <string.h>

#include "ctkgauge.h"

#define REQUESTED_WIDTH  116
#define REQUESTED_HEIGHT 86

static void
ctk_gauge_class_init    (CtkGaugeClass *);

static void
ctk_gauge_finalize      (GObject *);

static gboolean
ctk_gauge_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_gauge_size_request  (GtkWidget *, GtkRequisition *);

static gboolean
ctk_gauge_configure_event  (GtkWidget *, GdkEventConfigure *);

static void draw        (CtkGauge *);

static GdkColor*
get_foreground_color    (CtkGauge *, gint);

static GObjectClass *parent_class;


GType ctk_gauge_get_type(
    void
)
{
    static GType ctk_gauge_type = 0;

    if (!ctk_gauge_type) {
        static const GTypeInfo ctk_gauge_info = {
            sizeof (CtkGaugeClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_gauge_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGauge),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_gauge_type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                        "CtkGauge", &ctk_gauge_info, 0);
    }

    return ctk_gauge_type;
}

static void ctk_gauge_class_init(
    CtkGaugeClass *ctk_gauge_class
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_gauge_class;
    gobject_class = (GObjectClass *) ctk_gauge_class;

    parent_class = g_type_class_peek_parent(ctk_gauge_class);

    gobject_class->finalize = ctk_gauge_finalize;

    widget_class->expose_event = ctk_gauge_expose_event;
    widget_class->size_request = ctk_gauge_size_request;
    widget_class->configure_event = ctk_gauge_configure_event;
}

static void ctk_gauge_finalize(
    GObject *object
)
{
    CtkGauge *ctk_gauge;
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;

    ctk_gauge = CTK_GAUGE(object);

    gdk_colormap = ctk_gauge->gdk_colormap;

    gdk_color = &ctk_gauge->gdk_color_gray;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    gdk_color = &ctk_gauge->gdk_color_red;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    gdk_color = &ctk_gauge->gdk_color_yellow;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    gdk_color = &ctk_gauge->gdk_color_green;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    g_object_unref(gdk_colormap);
}

static gboolean ctk_gauge_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
{
    gint width, height;
    CtkGauge *ctk_gauge;

    ctk_gauge = CTK_GAUGE(widget);

    width  = widget->allocation.width  - 2 * widget->style->xthickness;
    height = widget->allocation.height - 2 * widget->style->ythickness;

    gtk_paint_shadow(widget->style, widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_IN,
                     &event->area, widget, "ctk_gauge", 0, 0,
                     widget->allocation.width, widget->allocation.height);

    gdk_gc_set_function(ctk_gauge->gdk_gc, GDK_COPY);
    
    gdk_draw_drawable(widget->window, ctk_gauge->gdk_gc, ctk_gauge->gdk_pixmap,
                      0, 0, widget->style->xthickness,
                      widget->style->ythickness,
                      width, height);
    return FALSE;
}

static gboolean ctk_gauge_configure_event
(
 GtkWidget *widget,
 GdkEventConfigure *event
 )
{
    CtkGauge *ctk_gauge = CTK_GAUGE(widget);

    ctk_gauge->width = event->width;
    ctk_gauge->height = event->height;
    
    if (ctk_gauge->gdk_pixmap) g_object_unref(ctk_gauge->gdk_pixmap);
    if (ctk_gauge->gdk_gc) g_object_unref(ctk_gauge->gdk_gc);

    ctk_gauge->gdk_pixmap =
        gdk_pixmap_new(widget->window, ctk_gauge->width,
               ctk_gauge->height, -1);
    ctk_gauge->gdk_gc = gdk_gc_new(ctk_gauge->gdk_pixmap);

    draw(ctk_gauge);

    return FALSE;
}

static void ctk_gauge_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    requisition->width  = REQUESTED_WIDTH;
    requisition->height = REQUESTED_HEIGHT;
}

GtkWidget* ctk_gauge_new(gint lower, gint upper)
{
    GObject *object;
    CtkGauge *ctk_gauge;
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;

    object = g_object_new(CTK_TYPE_GAUGE, NULL);

    ctk_gauge = CTK_GAUGE(object);

    ctk_gauge->lower = lower;
    ctk_gauge->upper = upper;

    ctk_gauge->gdk_pixmap = NULL;
    ctk_gauge->gdk_gc = NULL;
    
    ctk_gauge->pango_layout = 
        gtk_widget_create_pango_layout(GTK_WIDGET(ctk_gauge), NULL);

    ctk_gauge->gdk_colormap = gdk_colormap = gdk_colormap_get_system();

    gdk_color = &ctk_gauge->gdk_color_gray;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->red   = 32768;
    gdk_color->green = 32768;
    gdk_color->blue  = 32768;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    gdk_color = &ctk_gauge->gdk_color_red;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->red   = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    gdk_color = &ctk_gauge->gdk_color_yellow;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->red   = 65535;
    gdk_color->green = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    gdk_color = &ctk_gauge->gdk_color_green;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->green = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    return GTK_WIDGET(object);
}


void ctk_gauge_set_current(CtkGauge *ctk_gauge, gint current)
{
    gchar *ts;

    g_return_if_fail(CTK_IS_GAUGE(ctk_gauge));
    ctk_gauge->current = current;

    ts = g_strdup_printf("%d\xc2\xb0", current);
    pango_layout_set_text(ctk_gauge->pango_layout, ts, -1);

    g_free(ts);
}

static GdkColor* get_foreground_color(CtkGauge *ctk_gauge, gint i)
{
    if (i >= 7)
        return &ctk_gauge->gdk_color_red;
    else if (i > 3)
        return &ctk_gauge->gdk_color_yellow;
    else
        return &ctk_gauge->gdk_color_green;
}

static void draw(CtkGauge *ctk_gauge)
{
    GtkWidget *widget;
    gint x1, x2, y, width, i, percent, pos;
    gint upper, lower, current;
    
    lower = ctk_gauge->lower;
    upper = ctk_gauge->upper;
    current = ctk_gauge->current;

    gdk_gc_set_function(ctk_gauge->gdk_gc, GDK_COPY);
    
    widget = GTK_WIDGET(ctk_gauge);
    gdk_draw_rectangle(ctk_gauge->gdk_pixmap, widget->style->black_gc,
       TRUE, 0, 0, ctk_gauge->width, ctk_gauge->height);

    width = ctk_gauge->width / 5;
    y = ctk_gauge->height / 5;
    percent = ((current - lower) * 100) / (upper - lower);
    pos = (percent >= 95) ? 10 : (percent / 10);

    x1 = (ctk_gauge->width / 2) - width - 4;
    x2 = x1 + width + 2;

    gdk_gc_set_foreground(ctk_gauge->gdk_gc, &ctk_gauge->gdk_color_gray);

    for (i = 10; i > pos; i--) {
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x1, y, width, 2);
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x2, y, width, 2);
        y += 2 * 2;
    }

    for (i = i; i > 0; i--) {
        gdk_gc_set_foreground(ctk_gauge->gdk_gc,
            get_foreground_color(ctk_gauge, i));
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x1, y, width, 2);
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x2, y, width, 2);
        y += 2 * 2;
    }

    gdk_gc_set_foreground(ctk_gauge->gdk_gc, &ctk_gauge->gdk_color_gray);

    gdk_draw_layout(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
        x1, y, ctk_gauge->pango_layout);
}

void ctk_gauge_draw(CtkGauge *ctk_gauge)
{
    GtkWidget *widget;
    GdkRectangle rectangle;
    
    g_return_if_fail(CTK_IS_GAUGE(ctk_gauge));
    widget = GTK_WIDGET(ctk_gauge);

    rectangle.x = widget->style->xthickness;
    rectangle.y = widget->style->ythickness;

    rectangle.width  = widget->allocation.width  - 2 * rectangle.x;
    rectangle.height = widget->allocation.height - 2 * rectangle.y;

    if (GTK_WIDGET_DRAWABLE(widget)) {
        draw(ctk_gauge); /* only draw when visible */
        gdk_window_invalidate_rect(widget->window, &rectangle, FALSE);
    }
}
