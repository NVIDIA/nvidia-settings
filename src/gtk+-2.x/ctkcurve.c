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
#include "NvCtrlAttributes.h"

#include "ctkcurve.h"


#define REQUESTED_WIDTH 94
#define REQUESTED_HEIGHT 94

static void
ctk_curve_class_init    (CtkCurveClass *);

static void
ctk_curve_finalize      (GObject *);

#ifdef CTK_GTK3
static gboolean
ctk_curve_draw_event  (GtkWidget *, cairo_t *);

static void
ctk_curve_get_preferred_width(GtkWidget *, gint *, gint *);

static void
ctk_curve_get_preferred_height(GtkWidget *, gint *, gint *);
#else
static gboolean
ctk_curve_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_curve_size_request  (GtkWidget *, GtkRequisition *);
#endif

static gboolean
ctk_curve_configure_event(GtkWidget *, GdkEventConfigure *);

static void
#ifdef CTK_GTK3
plot_color_ramp (cairo_t *, gushort *, gint, gint, gint);
#else
plot_color_ramp         (GdkPixmap *, GdkGC *, gushort *, gint, gint, gint);

#endif


static void draw(CtkCurve *ctk_curve);

static GObjectClass *parent_class;


GType ctk_curve_get_type(
    void
)
{
    static GType ctk_curve_type = 0;

    if (!ctk_curve_type) {
        static const GTypeInfo ctk_curve_info = {
            sizeof (CtkCurveClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_curve_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkCurve),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_curve_type = g_type_register_static (GTK_TYPE_DRAWING_AREA,
                        "CtkCurve", &ctk_curve_info, 0);
    }

    return ctk_curve_type;
}

static void ctk_curve_class_init(
    CtkCurveClass *ctk_curve_class
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_curve_class;
    gobject_class = (GObjectClass *) ctk_curve_class;

    parent_class = g_type_class_peek_parent(ctk_curve_class);

    gobject_class->finalize = ctk_curve_finalize;

#ifdef CTK_GTK3
    widget_class->draw = ctk_curve_draw_event;
    widget_class->get_preferred_width = ctk_curve_get_preferred_width;
    widget_class->get_preferred_height = ctk_curve_get_preferred_height;
#else
    widget_class->expose_event = ctk_curve_expose_event;
    widget_class->size_request = ctk_curve_size_request;
#endif
    widget_class->configure_event = ctk_curve_configure_event;
}



static void ctk_curve_finalize(
    GObject *object
)
{
#ifndef CTK_GTK3
    CtkCurve *ctk_curve;
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;

    ctk_curve = CTK_CURVE(object);

    gdk_colormap = ctk_curve->gdk_colormap;

    gdk_color = &ctk_curve->gdk_color_red;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    gdk_color = &ctk_curve->gdk_color_green;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    gdk_color = &ctk_curve->gdk_color_blue;
    gdk_colormap_free_colors(gdk_colormap, gdk_color, 1);

    g_object_unref(gdk_colormap);
#endif
}

#ifdef CTK_GTK3
static gboolean ctk_curve_draw_event(
    GtkWidget *widget,
    cairo_t *cr
)
#else
static gboolean ctk_curve_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
#endif
{
    gint width, height;
    CtkCurve *ctk_curve;
    GtkAllocation allocation;

    ctk_curve = CTK_CURVE(widget);

    ctk_widget_get_allocation(widget, &allocation);

    width  = allocation.width  - 2 * gtk_widget_get_style(widget)->xthickness;
    height = allocation.height - 2 * gtk_widget_get_style(widget)->ythickness;

#ifdef CTK_GTK3
    gtk_render_frame(gtk_widget_get_style_context(widget),
                     cr, 0, 0, allocation.width, allocation.height);

    cairo_set_operator(ctk_curve->c_context, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, ctk_curve->c_surface, 0, 0);
    cairo_paint(cr);
#else
    gtk_paint_shadow(widget->style, widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_IN,
                     &event->area, widget, "ctk_curve", 0, 0,
                     allocation.width, allocation.height);

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_COPY);

    gdk_draw_drawable(widget->window, ctk_curve->gdk_gc, ctk_curve->gdk_pixmap,
                      0, 0, widget->style->xthickness,
                      widget->style->ythickness,
                      width, height);
#endif
    return FALSE;
}

static gboolean ctk_curve_configure_event
(
 GtkWidget *widget,
 GdkEventConfigure *event
 )
{
    CtkCurve *ctk_curve = CTK_CURVE(widget);

    ctk_curve->width = event->width;
    ctk_curve->height = event->height;
    
#ifdef CTK_GTK3
    if (ctk_curve->c_context) {
        cairo_destroy(ctk_curve->c_context);
    }
    if (ctk_curve->c_surface) {
        cairo_surface_destroy(ctk_curve->c_surface);
    }

    ctk_curve->c_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                      ctk_curve->width,
                                                      ctk_curve->height);
    ctk_curve->c_context = cairo_create(ctk_curve->c_surface);

#else
    if (ctk_curve->gdk_pixmap) g_object_unref(ctk_curve->gdk_pixmap);
    if (ctk_curve->gdk_gc) g_object_unref(ctk_curve->gdk_gc);

    ctk_curve->gdk_pixmap = gdk_pixmap_new(widget->window, ctk_curve->width,
                                           ctk_curve->height, -1);
    ctk_curve->gdk_gc = gdk_gc_new(ctk_curve->gdk_pixmap);
#endif

    draw(ctk_curve);

    return FALSE;
}


static void plot_color_ramp(
#ifdef CTK_GTK3
    cairo_t *cr,
#else
    GdkPixmap *gdk_pixmap,
    GdkGC *gdk_gc,
#endif
    gushort *color_ramp,
    gint n_color_ramp_entries,
    gint width,
    gint height
)
{
    gfloat x, dx, y;
    GdkPoint *gdk_points;
    gint i;

    gdk_points = g_malloc(width * sizeof(GdkPoint));

    x = 0;
    dx = (n_color_ramp_entries - 1.0) / (width - 1.0);

    for (i = 0; i < width; i++, x += dx) {
        y = (gfloat) color_ramp[(int) (x + 0.5)];
        gdk_points[i].x = i;
        gdk_points[i].y = height - ((height - 1) * (y / 65535) + 0.5);
    }

#ifdef CTK_GTK3
    cairo_set_line_width(cr, 1.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);

    cairo_move_to(cr, gdk_points[0].x, gdk_points[0].y);
    for (i = 1; i < width; i++) {
        cairo_line_to(cr, gdk_points[i].x, gdk_points[i].y);
    }
    cairo_stroke(cr);
#else
    gdk_draw_lines(gdk_pixmap, gdk_gc, gdk_points, width);
#endif

    g_free(gdk_points);
}

#ifdef CTK_GTK3
static void ctk_curve_get_preferred_height(
    GtkWidget *widget,
    gint *minimum_height,
    gint *natural_height
)
{
    *minimum_height = *natural_height = REQUESTED_WIDTH;
}
static void ctk_curve_get_preferred_width(
    GtkWidget *widget,
    gint *minimum_width,
    gint *natural_width
)
{
    *minimum_width = *natural_width = REQUESTED_WIDTH;
}
#else
static void ctk_curve_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    requisition->width  = REQUESTED_WIDTH;
    requisition->height = REQUESTED_HEIGHT;
}
#endif

void ctk_curve_color_changed(GtkWidget *widget)
{
    GdkRectangle rectangle;
    GtkAllocation allocation;

    ctk_widget_get_allocation(widget, &allocation);

    rectangle.x = gtk_widget_get_style(widget)->xthickness;
    rectangle.y = gtk_widget_get_style(widget)->ythickness;

    rectangle.width  = allocation.width  - 2 * rectangle.x;
    rectangle.height = allocation.height - 2 * rectangle.y;

    if (ctk_widget_is_drawable(widget)) {
        draw(CTK_CURVE(widget)); /* only draw when visible */
        gdk_window_invalidate_rect(ctk_widget_get_window(widget),
                                   &rectangle, FALSE);
    }
}

GtkWidget* ctk_curve_new(CtrlTarget *ctrl_target, GtkWidget *color)
{
    GObject *object;
    CtkCurve *ctk_curve;
#ifndef CTK_GTK3
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;
#endif

    object = g_object_new(CTK_TYPE_CURVE, NULL);

    ctk_curve = CTK_CURVE(object);

    ctk_curve->ctrl_target = ctrl_target;
    ctk_curve->color = color;

#ifdef CTK_GTK3
    ctk_curve->c_context = NULL;
    ctk_curve->c_surface = NULL;
#else
    ctk_curve->gdk_pixmap = NULL;
    ctk_curve->gdk_gc = NULL;
    
    ctk_curve->gdk_colormap = gdk_colormap = gdk_colormap_get_system();

    g_object_ref(gdk_colormap);

    gdk_color = &ctk_curve->gdk_color_red;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->red = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    gdk_color = &ctk_curve->gdk_color_green;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->green = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);

    gdk_color = &ctk_curve->gdk_color_blue;
    memset(gdk_color, 0, sizeof(GdkColor));
    gdk_color->blue = 65535;
    gdk_colormap_alloc_color(gdk_colormap, gdk_color, FALSE, TRUE);
#endif

    g_signal_connect_swapped(G_OBJECT(ctk_curve->color), "changed",
                             G_CALLBACK(ctk_curve_color_changed),
                             (gpointer) ctk_curve);

    return GTK_WIDGET(object);
}



static void draw(CtkCurve *ctk_curve)
{
    CtrlTarget *ctrl_target = ctk_curve->ctrl_target;
    gushort *lut;
    gint n_lut_entries;

#ifdef CTK_GTK3
    /* Fill Curve surface with black background */
    cairo_set_operator(ctk_curve->c_context, CAIRO_OPERATOR_SOURCE);
    
    cairo_set_source_rgba(ctk_curve->c_context, 0.0, 0.0, 0.0, 1.0);
    cairo_rectangle(ctk_curve->c_context, 0, 0,
                    ctk_curve->width, ctk_curve->height);
    cairo_fill(ctk_curve->c_context);

    /* Add the Color LUT ramp lines */
    cairo_set_operator(ctk_curve->c_context, CAIRO_OPERATOR_ADD);

    cairo_set_source_rgba(ctk_curve->c_context, 1.0, 0.0, 0.0, 1.0);
    NvCtrlGetColorRamp(ctrl_target, RED_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->c_context, lut, n_lut_entries,
                    ctk_curve->width, ctk_curve->height);

    cairo_set_source_rgba(ctk_curve->c_context, 0.0, 1.0, 0.0, 1.0);
    NvCtrlGetColorRamp(ctrl_target, GREEN_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->c_context, lut, n_lut_entries,
                    ctk_curve->width, ctk_curve->height);

    cairo_set_source_rgba(ctk_curve->c_context, 0.0, 0.0, 1.0, 1.0);
    NvCtrlGetColorRamp(ctrl_target, BLUE_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->c_context, lut, n_lut_entries,
                    ctk_curve->width, ctk_curve->height);
#else
    GtkWidget *widget = GTK_WIDGET(ctk_curve);

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_COPY);
    
    gdk_draw_rectangle(ctk_curve->gdk_pixmap, widget->style->black_gc,
                       TRUE, 0, 0, ctk_curve->width, ctk_curve->height);

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_XOR);

    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_red);
    NvCtrlGetColorRamp(ctrl_target, RED_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);
    
    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_green);
    NvCtrlGetColorRamp(ctrl_target, GREEN_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);

    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_blue);
    NvCtrlGetColorRamp(ctrl_target, BLUE_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);
#endif
}
