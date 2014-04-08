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

static gboolean
ctk_curve_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_curve_size_request  (GtkWidget *, GtkRequisition *);

static gboolean
ctk_curve_configure_event(GtkWidget *, GdkEventConfigure *);

static void
plot_color_ramp         (GdkPixmap *, GdkGC *, gushort *, gint, gint, gint);


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

    widget_class->expose_event = ctk_curve_expose_event;
    widget_class->size_request = ctk_curve_size_request;
    widget_class->configure_event = ctk_curve_configure_event;
}



static void ctk_curve_finalize(
    GObject *object
)
{
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
}

static gboolean ctk_curve_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
{
    gint width, height;
    CtkCurve *ctk_curve;

    ctk_curve = CTK_CURVE(widget);

    width  = widget->allocation.width  - 2 * widget->style->xthickness;
    height = widget->allocation.height - 2 * widget->style->ythickness;

    gtk_paint_shadow(widget->style, widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_IN,
                     &event->area, widget, "ctk_curve", 0, 0,
                     widget->allocation.width, widget->allocation.height);

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_COPY);
    
    gdk_draw_drawable(widget->window, ctk_curve->gdk_gc, ctk_curve->gdk_pixmap,
                      0, 0, widget->style->xthickness,
                      widget->style->ythickness,
                      width, height);
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
    
    if (ctk_curve->gdk_pixmap) g_object_unref(ctk_curve->gdk_pixmap);
    if (ctk_curve->gdk_gc) g_object_unref(ctk_curve->gdk_gc);

    ctk_curve->gdk_pixmap = gdk_pixmap_new(widget->window, ctk_curve->width,
                                           ctk_curve->height, -1);
    ctk_curve->gdk_gc = gdk_gc_new(ctk_curve->gdk_pixmap);

    draw(ctk_curve);

    return FALSE;
}


static void plot_color_ramp(
    GdkPixmap *gdk_pixmap,
    GdkGC *gdk_gc,
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

    gdk_draw_lines(gdk_pixmap, gdk_gc, gdk_points, width);

    g_free(gdk_points);
}

static void ctk_curve_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    requisition->width  = REQUESTED_WIDTH;
    requisition->height = REQUESTED_HEIGHT;
}

void ctk_curve_color_changed(GtkWidget *widget)
{
    GdkRectangle rectangle;

    rectangle.x = widget->style->xthickness;
    rectangle.y = widget->style->ythickness;

    rectangle.width  = widget->allocation.width  - 2 * rectangle.x;
    rectangle.height = widget->allocation.height - 2 * rectangle.y;

    if (GTK_WIDGET_DRAWABLE(widget)) {
        draw(CTK_CURVE(widget)); /* only draw when visible */
        gdk_window_invalidate_rect(widget->window, &rectangle, FALSE);
    }
}

GtkWidget* ctk_curve_new(NvCtrlAttributeHandle *handle, GtkWidget *color)
{
    GObject *object;
    CtkCurve *ctk_curve;
    GdkColormap *gdk_colormap;
    GdkColor *gdk_color;

    object = g_object_new(CTK_TYPE_CURVE, NULL);

    ctk_curve = CTK_CURVE(object);

    ctk_curve->handle = handle;
    ctk_curve->color = color;

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


    g_signal_connect_swapped(G_OBJECT(ctk_curve->color), "changed",
                             G_CALLBACK(ctk_curve_color_changed),
                             (gpointer) ctk_curve);

    return GTK_WIDGET(object);
}



static void draw(CtkCurve *ctk_curve)
{
    GtkWidget *widget = GTK_WIDGET(ctk_curve);

    gushort *lut;
    gint n_lut_entries;

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_COPY);
    
    gdk_draw_rectangle(ctk_curve->gdk_pixmap, widget->style->black_gc,
                       TRUE, 0, 0, ctk_curve->width, ctk_curve->height);

    gdk_gc_set_function(ctk_curve->gdk_gc, GDK_XOR);

    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_red);
    NvCtrlGetColorRamp(ctk_curve->handle, RED_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);
    
    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_green);
    NvCtrlGetColorRamp(ctk_curve->handle, GREEN_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);

    gdk_gc_set_foreground(ctk_curve->gdk_gc, &ctk_curve->gdk_color_blue);
    NvCtrlGetColorRamp(ctk_curve->handle, BLUE_CHANNEL, &lut, &n_lut_entries);
    plot_color_ramp(ctk_curve->gdk_pixmap, ctk_curve->gdk_gc,
                    lut, n_lut_entries, ctk_curve->width, ctk_curve->height);
    
}
