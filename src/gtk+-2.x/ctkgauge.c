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

#include <gtk/gtk.h>
#include <string.h>

#include "ctkgauge.h"
#include "ctkutils.h"

#define REQUESTED_WIDTH  116
#define REQUESTED_HEIGHT 86

static void
ctk_gauge_class_init    (CtkGaugeClass *, gpointer);

static void
ctk_gauge_finalize      (GObject *);

#ifdef CTK_GTK3
static gboolean
ctk_gauge_draw_event  (GtkWidget *, cairo_t *);

static void
ctk_gauge_get_preferred_width(GtkWidget *, gint *, gint *);

static void
ctk_gauge_get_preferred_height(GtkWidget *, gint *, gint *);
#else

static gboolean
ctk_gauge_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_gauge_size_request  (GtkWidget *, GtkRequisition *);

#endif

static gboolean
ctk_gauge_configure_event  (GtkWidget *, GdkEventConfigure *);

static void draw        (CtkGauge *);

#ifdef CTK_GTK3
static void
set_foreground_color    (cairo_t *, gint);
#else
static GdkColor*
get_foreground_color    (CtkGauge *, gint);
#endif

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
            NULL  /* value_table */
        };

        ctk_gauge_type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                        "CtkGauge", &ctk_gauge_info, 0);
    }

    return ctk_gauge_type;
}

static void ctk_gauge_class_init(
    CtkGaugeClass *ctk_gauge_class,
    gpointer class_data
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_gauge_class;
    gobject_class = (GObjectClass *) ctk_gauge_class;

    parent_class = g_type_class_peek_parent(ctk_gauge_class);

    gobject_class->finalize = ctk_gauge_finalize;

#ifdef CTK_GTK3
    widget_class->draw = ctk_gauge_draw_event;
    widget_class->get_preferred_width  = ctk_gauge_get_preferred_width;
    widget_class->get_preferred_height = ctk_gauge_get_preferred_height;
#else
    widget_class->expose_event = ctk_gauge_expose_event;
    widget_class->size_request = ctk_gauge_size_request;
#endif
    widget_class->configure_event = ctk_gauge_configure_event;
}

static void ctk_gauge_finalize(
    GObject *object
)
{
#ifndef CTK_GTK3
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
#endif
}

#ifdef CTK_GTK3
static gboolean ctk_gauge_draw_event(
    GtkWidget *widget,
    cairo_t *cr
)
#else
static gboolean ctk_gauge_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
#endif
{
    CtkGauge *ctk_gauge;
    GtkAllocation allocation;

    ctk_gauge = CTK_GAUGE(widget);

    ctk_widget_get_allocation(widget, &allocation);

#ifdef CTK_GTK3
    gtk_render_frame(gtk_widget_get_style_context(widget),
                     cr, 0, 0, allocation.width, allocation.height);

    cairo_set_operator(ctk_gauge->c_context, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_surface(cr, ctk_gauge->c_surface, 0, 0);
    cairo_paint(cr);
#else
    {
        gint width  = allocation.width  - 2 * gtk_widget_get_style(widget)->xthickness;
        gint height = allocation.height - 2 * gtk_widget_get_style(widget)->ythickness;

        gtk_paint_shadow(widget->style, widget->window,
                         GTK_STATE_NORMAL, GTK_SHADOW_IN,
                         &event->area, widget, "ctk_gauge", 0, 0,
                         widget->allocation.width, widget->allocation.height);

        gdk_gc_set_function(ctk_gauge->gdk_gc, GDK_COPY);

        gdk_draw_drawable(widget->window, ctk_gauge->gdk_gc, ctk_gauge->gdk_pixmap,
                          0, 0, widget->style->xthickness,
                          widget->style->ythickness,
                          width, height);
    }
#endif
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

#ifdef CTK_GTK3
    if (ctk_gauge->c_context) cairo_destroy(ctk_gauge->c_context);
    if (ctk_gauge->c_surface) cairo_surface_destroy(ctk_gauge->c_surface);

    ctk_gauge->c_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                      ctk_gauge->width,
                                                      ctk_gauge->height);
    ctk_gauge->c_context = cairo_create(ctk_gauge->c_surface);
#else
    if (ctk_gauge->gdk_pixmap) g_object_unref(ctk_gauge->gdk_pixmap);
    if (ctk_gauge->gdk_gc) g_object_unref(ctk_gauge->gdk_gc);

    ctk_gauge->gdk_pixmap =
        gdk_pixmap_new(widget->window, ctk_gauge->width,
               ctk_gauge->height, -1);
    ctk_gauge->gdk_gc = gdk_gc_new(ctk_gauge->gdk_pixmap);
#endif

    draw(ctk_gauge);

    return FALSE;
}

#ifdef CTK_GTK3
static void ctk_gauge_get_preferred_height(
    GtkWidget *widget,
    gint *minimum_height,
    gint *natural_height
)
{
    *minimum_height = *natural_height = REQUESTED_HEIGHT;
}

static void ctk_gauge_get_preferred_width(
    GtkWidget *widget,
    gint *minimum_width,
    gint *natural_width
)
{
    *minimum_width = *natural_width = REQUESTED_WIDTH;
}
#else
static void ctk_gauge_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    requisition->width  = REQUESTED_WIDTH;
    requisition->height = REQUESTED_HEIGHT;
}
#endif

GtkWidget* ctk_gauge_new(gint lower, gint upper)
{
    GObject *object;
    CtkGauge *ctk_gauge;
#ifndef CTK_GTK3
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;
#endif

    object = g_object_new(CTK_TYPE_GAUGE, NULL);

    ctk_gauge = CTK_GAUGE(object);

    ctk_gauge->lower = lower;
    ctk_gauge->upper = upper;

#ifdef CTK_GTK3
    ctk_gauge->c_surface = NULL;
    ctk_gauge->c_context = NULL;
#else
    ctk_gauge->gdk_pixmap = NULL;
    ctk_gauge->gdk_gc = NULL;
#endif

    ctk_gauge->pango_layout =
        gtk_widget_create_pango_layout(GTK_WIDGET(ctk_gauge), NULL);

#ifndef CTK_GTK3
    ctk_gauge->gdk_colormap = gdk_colormap = gdk_colormap_get_system();

    g_object_ref(gdk_colormap);

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
#endif
    return GTK_WIDGET(object);
}


void ctk_gauge_set_current(CtkGauge *ctk_gauge, gint current)
{
    gchar *ts;

    g_return_if_fail(CTK_IS_GAUGE(ctk_gauge));
    ctk_gauge->current = current;

    ts = g_strdup_printf("%d\xc2\xb0" /* split for g_utf8_validate() */ "C",
                         current);
    pango_layout_set_text(ctk_gauge->pango_layout, ts, -1);

    g_free(ts);
}

#ifdef CTK_GTK3
static void set_foreground_color(cairo_t *cr, gint i)
{
    if (i >= 7) {
        cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    } else if (i > 3) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
    } else {
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
    }
}
#else
static GdkColor *get_foreground_color(CtkGauge *ctk_gauge, gint i)
{
    if (i >= 7) {
        return &ctk_gauge->gdk_color_red;
    } else if (i > 3) {
        return &ctk_gauge->gdk_color_yellow;
    } else {
        return &ctk_gauge->gdk_color_green;
    }
}
#endif


static void draw(CtkGauge *ctk_gauge)
{
    gint x1, x2, y, width, i, percent, pos;
    gint upper, lower, range, current;

    lower = ctk_gauge->lower;
    upper = ctk_gauge->upper;
    range = upper - lower;
    current = ctk_gauge->current;

#ifdef CTK_GTK3
    /* Fill Curve surface with black background */
    cairo_set_operator(ctk_gauge->c_context, CAIRO_OPERATOR_SOURCE);

    cairo_set_source_rgba(ctk_gauge->c_context, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle(ctk_gauge->c_context, 0, 0,
                    ctk_gauge->width, ctk_gauge->height);
    cairo_fill(ctk_gauge->c_context);
#else
    {
        GtkWidget *widget = GTK_WIDGET(ctk_gauge);;

        gdk_gc_set_function(ctk_gauge->gdk_gc, GDK_COPY);

        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, widget->style->black_gc,
           TRUE, 0, 0, ctk_gauge->width, ctk_gauge->height);
    }
#endif

    width = ctk_gauge->width / 5;
    y = ctk_gauge->height / 5;
    percent = (range > 0) ? (((current - lower) * 100) / range) : 0;
    pos = (percent >= 95) ? 10 : (percent / 10);

    x1 = (ctk_gauge->width / 2) - width - 4;
    x2 = x1 + width + 2;

#ifdef CTK_GTK3
    cairo_set_source_rgba(ctk_gauge->c_context, 0.5, 0.5, 0.5, 1.0);
#else
    gdk_gc_set_foreground(ctk_gauge->gdk_gc, &ctk_gauge->gdk_color_gray);
#endif

    for (i = 10; i > pos; i--) {
#ifdef CTK_GTK3
        cairo_rectangle(ctk_gauge->c_context, x1, y, width, 2);
        cairo_fill(ctk_gauge->c_context);
        cairo_rectangle(ctk_gauge->c_context, x2, y, width, 2);
        cairo_fill(ctk_gauge->c_context);
#else
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x1, y, width, 2);
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x2, y, width, 2);
#endif
        y += 2 * 2;
    }

    for (; i > 0; i--) {
#ifdef CTK_GTK3
        set_foreground_color(ctk_gauge->c_context, i);
        cairo_rectangle(ctk_gauge->c_context, x1, y, width, 2);
        cairo_fill(ctk_gauge->c_context);
        cairo_rectangle(ctk_gauge->c_context, x2, y, width, 2);
        cairo_fill(ctk_gauge->c_context);
#else
        gdk_gc_set_foreground(ctk_gauge->gdk_gc,
            get_foreground_color(ctk_gauge, i));
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x1, y, width, 2);
        gdk_draw_rectangle(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
            TRUE, x2, y, width, 2);
#endif
        y += 2 * 2;
    }

#ifdef CTK_GTK3
    cairo_set_source_rgba(ctk_gauge->c_context, 0.5, 0.5, 0.5, 1.0);
    cairo_move_to(ctk_gauge->c_context, x1, y);
    pango_cairo_show_layout(ctk_gauge->c_context, ctk_gauge->pango_layout);
#else
    gdk_gc_set_foreground(ctk_gauge->gdk_gc, &ctk_gauge->gdk_color_gray);

    gdk_draw_layout(ctk_gauge->gdk_pixmap, ctk_gauge->gdk_gc,
        x1, y, ctk_gauge->pango_layout);
#endif
}

void ctk_gauge_draw(CtkGauge *ctk_gauge)
{
    GtkWidget *widget;
    GdkRectangle rectangle;
    GtkAllocation allocation;

    g_return_if_fail(CTK_IS_GAUGE(ctk_gauge));
    widget = GTK_WIDGET(ctk_gauge);

    ctk_widget_get_allocation(widget, &allocation);

    rectangle.x = gtk_widget_get_style(widget)->xthickness;
    rectangle.y = gtk_widget_get_style(widget)->ythickness;

    rectangle.width  = allocation.width  - 2 * rectangle.x;
    rectangle.height = allocation.height - 2 * rectangle.y;

    if (ctk_widget_is_drawable(widget)) {
        draw(ctk_gauge); /* only draw when visible */
        gdk_window_invalidate_rect(ctk_widget_get_window(widget),
                                   &rectangle, FALSE);
    }
}
