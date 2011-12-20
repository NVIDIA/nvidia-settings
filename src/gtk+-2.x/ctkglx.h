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

#ifndef __CTK_GLX_H__
#define __CTK_GLX_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_GLX (ctk_glx_get_type())

#define CTK_GLX(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GLX, CtkGLX))

#define CTK_GLX_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GLX, CtkGLXClass))

#define CTK_IS_GLX(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GLX))

#define CTK_IS_GLX_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GLX))

#define CTK_GLX_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GLX, CtkGLXClass))


typedef struct _CtkGLX       CtkGLX;
typedef struct _CtkGLXClass  CtkGLXClass;

struct _CtkGLX
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *glxinfo_vpane;
    Bool       glxinfo_initialized;
    GtkWidget *show_fbc_button;
    GtkWidget *fbc_window;
};

struct _CtkGLXClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_glx_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_glx_new       (NvCtrlAttributeHandle *,
                               CtkConfig *, CtkEvent *);

GtkTextBuffer *ctk_glx_create_help(GtkTextTagTable *, CtkGLX *);

void ctk_glx_probe_info(GtkWidget *widget);


G_END_DECLS

#endif /* __CTK_GLX_H__ */
