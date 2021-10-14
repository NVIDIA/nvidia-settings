/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2017 NVIDIA Corporation.
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

#include "ctkutils.h"
#include "ctkglwidget.h"

#include <assert.h>
#include <gtk/gtk.h>
#include <GL/glx.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include "opengl_loading.h"

GtkWidget *ctk_glwidget_new(int glx_attributes[],
                            void *app_data,
                            int (*app_setup_callback) (void *app_data),
                            void (*draw_frame_callback) (void *app_data) )
{
#ifdef CTK_GTK3
    GObject *object;
    CtkGLWidget *ctk_glwidget;
    GLXFBConfig *fb_configs;
    int n_elements;
    int x_visual_id;

    GdkDisplay *gdk_display;
    Display *display;
    GLXContext glx_context;
    GdkVisual *gdk_visual;

    gdk_display = gdk_display_get_default();


    display = gdk_x11_display_get_xdisplay(gdk_display);

    if (!loadGL()) {
        return NULL;
    }

    fb_configs = dGL.glXChooseFBConfig(display,
                                       DefaultScreen(display),
                                       glx_attributes, &n_elements);
    if (n_elements == 0) {
        return NULL;
    }

    dGL.glXGetFBConfigAttrib(display,
                             fb_configs[0],
                             GLX_VISUAL_ID,
                             &x_visual_id);

    gdk_visual = gdk_x11_screen_lookup_visual(gdk_screen_get_default(),
                                              x_visual_id);

    if (gdk_visual == NULL) {
        XFree(fb_configs);
        return NULL;
    }

    gdk_x11_display_error_trap_push(gdk_display);
    glx_context = dGL.glXCreateNewContext(display,
                                          fb_configs[0],
                                          GLX_RGBA_TYPE,
                                          NULL,
                                          GL_TRUE);

    if (gdk_x11_display_error_trap_pop(gdk_display) != 0) {
        glx_context = NULL;
    }

    XFree(fb_configs);

    if (glx_context == NULL) {
        return NULL;
    }

    object = g_object_new(CTK_TYPE_GLWIDGET, NULL);
    ctk_glwidget = CTK_GLWIDGET(object);

    ctk_glwidget->app_data = app_data;
    ctk_glwidget->app_setup_callback = app_setup_callback;
    ctk_glwidget->draw_frame_callback = draw_frame_callback;

    ctk_glwidget->gdk_display = gdk_display;
    ctk_glwidget->gdk_window = NULL;
    ctk_glwidget->display = display;

    ctk_glwidget->is_error = FALSE;
    ctk_glwidget->timer_interval = 100;

    ctk_glwidget->glx_context = glx_context;
    ctk_glwidget->gdk_visual = gdk_visual;

    return GTK_WIDGET(ctk_glwidget);

#else
    return NULL;
#endif
}

#ifdef CTK_GTK3
static void on_error(CtkGLWidget *ctk_glwidget);
static gboolean draw_frame_in_glwidget(gpointer p);

static void ctk_glwidget_realize(GtkWidget *gtk_widget)
{
    CtkGLWidget *ctk_glwidget;
    GdkWindowAttr gdk_attributes;
    gint attributes_mask;
    GtkAllocation allocation;

    ctk_glwidget = CTK_GLWIDGET(gtk_widget);

    gtk_widget_set_realized(gtk_widget, TRUE);
    gtk_widget_set_has_window(gtk_widget, TRUE);

    if (ctk_glwidget->is_error) {
        return;
    }

    if (ctk_glwidget->gdk_window) {
        gdk_window_set_user_data(ctk_glwidget->gdk_window,
                                 gtk_widget);
        return;
    }

    gtk_widget_get_allocation(gtk_widget, &allocation);

    gdk_attributes.x = allocation.x;
    gdk_attributes.y = allocation.y;
    gdk_attributes.width = allocation.width;
    gdk_attributes.height = allocation.height;
    gdk_attributes.wclass = GDK_INPUT_OUTPUT;
    gdk_attributes.window_type = GDK_WINDOW_CHILD;
    gdk_attributes.event_mask = gtk_widget_get_events(gtk_widget);
    gdk_attributes.visual = ctk_glwidget->gdk_visual;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    ctk_glwidget->gdk_window =
        gdk_window_new(gtk_widget_get_parent_window(gtk_widget),
                       &gdk_attributes, attributes_mask);

    if (ctk_glwidget->gdk_window == NULL) {
        on_error(ctk_glwidget);
        return;
    }

    ctk_glwidget->window = gdk_x11_window_get_xid(ctk_glwidget->gdk_window);
    gdk_window_set_user_data(ctk_glwidget->gdk_window, gtk_widget);
    gtk_widget_set_window(gtk_widget, ctk_glwidget->gdk_window);

    dGL.glXMakeContextCurrent(ctk_glwidget->display, ctk_glwidget->window,
                              ctk_glwidget->window, ctk_glwidget->glx_context);

    if (ctk_glwidget->app_setup_callback(ctk_glwidget->app_data) != 0) {
        on_error(ctk_glwidget);
        return;
    }

    g_timeout_add_full(G_PRIORITY_LOW,
                       ctk_glwidget->timer_interval,
                       draw_frame_in_glwidget,
                       ctk_glwidget,
                       0);

}

static void ctk_glwidget_unrealize(GtkWidget *gtk_widget)
{

    if (CTK_GLWIDGET(gtk_widget)->is_error) {
        return;
    }

    if (gtk_widget_get_has_window(gtk_widget)) {
        gdk_window_set_user_data(CTK_GLWIDGET(gtk_widget)->gdk_window, NULL);
    }

    gtk_selection_remove_all(gtk_widget);
    gtk_widget_set_realized(gtk_widget, FALSE);
}

static void ctk_glwidget_finalize(GObject *gobject)
{
    assert(!"unimplemented");
}

static void ctk_glwidget_class_init(CtkGLWidgetClass *klass,
                                    gpointer class_data)
{
    GtkWidgetClass *widget_class;
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    widget_class = (GtkWidgetClass *) klass;

    widget_class->realize = ctk_glwidget_realize;
    widget_class->unrealize = ctk_glwidget_unrealize;
    gobject_class->finalize = ctk_glwidget_finalize;
}

GType ctk_glwidget_get_type(void)
{
    static GType ctk_glwidget_type = 0;

    if (!ctk_glwidget_type) {
        static const GTypeInfo info_ctk_glwidget = {
            sizeof (CtkGLWidgetClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_glwidget_class_init, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGLWidget),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };
        ctk_glwidget_type = g_type_register_static(GTK_TYPE_WIDGET,
                                "CtkGLWidget", &info_ctk_glwidget, 0);
    }
    return ctk_glwidget_type;
}

static gboolean draw_frame_in_glwidget(gpointer p)
{
    CtkGLWidget *ctk_glwidget = CTK_GLWIDGET(p);

    if (ctk_glwidget->is_error) {
        return FALSE;
    }

    if (!gtk_widget_get_realized(GTK_WIDGET(p))) {
        return TRUE;
    }

    if (ctk_widget_is_drawable(GTK_WIDGET(ctk_glwidget))) {
        ctk_glwidget_make_current(ctk_glwidget);
        ctk_glwidget->draw_frame_callback(ctk_glwidget->app_data);
        ctk_glwidget_swap(ctk_glwidget);
    }
    return TRUE;
}

static void on_error(CtkGLWidget *ctk_glwidget)
{
    ctk_glwidget->is_error = TRUE;
    gtk_widget_set_size_request(GTK_WIDGET(ctk_glwidget), 0, 0);

    if (ctk_glwidget->gdk_window) {
        gdk_window_set_user_data(ctk_glwidget->gdk_window, NULL);
    }
}

void ctk_glwidget_make_current(CtkGLWidget *ctk_glwidget)
{
    dGL.glXMakeContextCurrent(ctk_glwidget->display,
                              ctk_glwidget->window,
                              ctk_glwidget->window,
                              ctk_glwidget->glx_context);
}

void ctk_glwidget_swap(CtkGLWidget *ctk_glwidget)
{
    dGL.glXSwapBuffers(ctk_glwidget->display, ctk_glwidget->window);
}
#endif

