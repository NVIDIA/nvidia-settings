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

#ifndef __CTK_GLWIDGET_H__
#define __CTK_GLWIDGET_H__

#include <gtk/gtk.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <gdk/gdkx.h>

#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_GLWIDGET (ctk_glwidget_get_type())

#define CTK_GLWIDGET(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GLWIDGET, CtkGLWidget))

#define CTK_GLWIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GLWIDGET, CtkGLWidgetClass))

#define CTK_IS_GLWIDGET(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GLWIDGET))

#define CTK_IS_GLWIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GLWIDGET))

#define CTK_GLWIDGET_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GLWIDGET, CtkGLWidgetClass))


typedef struct _CtkGLWidget      CtkGLWidget;
typedef struct _CtkGLWidgetClass CtkGLWidgetClass;

struct _CtkGLWidget
{
    GtkWidget parent;

    GdkDisplay *gdk_display;
    GdkWindow *gdk_window;
    Display *display;
    Window window;
    GLXContext glx_context;
    GdkVisual *gdk_visual;

    gboolean is_error;
    int timer_interval;
    void *app_data;

    int (*app_setup_callback) (void *app_data);
    void (*draw_frame_callback) (void *app_data);
};

struct _CtkGLWidgetClass
{
    GtkWidgetClass parent_class;
};

GType ctk_glwidget_get_type (void);

GtkWidget *ctk_glwidget_new(int glx_attributes[],
                            void *app_data,
                            int (*app_setup_callback) (void *app_data),
                            void (*draw_frame_callback) (void *app_data));

#ifdef CTK_GTK3
void ctk_glwidget_make_current(CtkGLWidget *ctk_glwidget);
void ctk_glwidget_swap(CtkGLWidget *ctk_glwidget);
#endif

G_END_DECLS

#endif
