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

#ifndef __CTK_OPENGL_H__
#define __CTK_OPENGL_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_OPENGL (ctk_opengl_get_type())

#define CTK_OPENGL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_OPENGL, CtkOpenGL))

#define CTK_OPENGL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_OPENGL, CtkOpenGLClass))

#define CTK_IS_OPENGL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_OPENGL))

#define CTK_IS_OPENGL_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_OPENGL))

#define CTK_OPENGL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_OPENGL, CtkOpenGLClass))


typedef struct _CtkOpenGL       CtkOpenGL;
typedef struct _CtkOpenGLClass  CtkOpenGLClass;

struct _CtkOpenGL
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;
    CtrlEvent *ctk_event;

    GtkWidget *sync_to_vblank_button;
    GtkWidget *allow_flipping_button;
    GtkWidget *allow_gsync_button;
    GtkWidget *show_gsync_visual_indicator_button;
    GtkWidget *use_conformant_clamping_button;
    GtkWidget *force_stereo_button;
    GtkWidget *xinerama_stereo_button;
    GtkWidget *stereo_eyes_exchange_button;
    GtkWidget *stereo_swap_mode_menu;
    GtkWidget *image_settings_scale;
    GtkWidget *aa_line_gamma_button;
    GtkWidget *aa_line_gamma_scale;
    GtkWidget *show_sli_visual_indicator_button;
    GtkWidget *show_multigpu_visual_indicator_button;
    GtkWidget *show_graphics_visual_indicator_button;

    gint *stereo_swap_mode_table;
    gint stereo_swap_mode_table_size;

    unsigned int active_attributes;
};

struct _CtkOpenGLClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_opengl_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_opengl_new       (CtrlTarget *, CtkConfig *, CtkEvent *);

GtkTextBuffer *ctk_opengl_create_help(GtkTextTagTable *, CtkOpenGL *);

G_END_DECLS

#endif /* __CTK_OPENGL_H__ */

