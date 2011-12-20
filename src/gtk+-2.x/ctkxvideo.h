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

#ifndef __CTK_XVIDEO_H__
#define __CTK_XVIDEO_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS

#define CTK_TYPE_XVIDEO (ctk_xvideo_get_type())

#define CTK_XVIDEO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_XVIDEO, CtkXVideo))

#define CTK_XVIDEO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_XVIDEO, CtkXVideoClass))

#define CTK_IS_XVIDEO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_XVIDEO))

#define CTK_IS_XVIDEO_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_XVIDEO))

#define CTK_XVIDEO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_XVIDEO, CtkXVideoClass))


typedef struct _CtkXVideo       CtkXVideo;
typedef struct _CtkXVideoClass  CtkXVideoClass;

struct _CtkXVideo
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *overlay_saturation;
    GtkWidget *overlay_contrast;
    GtkWidget *overlay_brightness;
    GtkWidget *overlay_hue;
    GtkWidget *texture_contrast;
    GtkWidget *texture_brightness;
    GtkWidget *texture_hue;
    GtkWidget *texture_saturation;
    GtkWidget *texture_sync_to_blank;
    GtkWidget *blitter_sync_to_blank;
    GtkWidget *xv_sync_to_display_buttons[24];
    GtkWidget *xv_sync_to_display_button_box;
    unsigned int active_attributes;
};

struct _CtkXVideoClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_xvideo_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_xvideo_new       (NvCtrlAttributeHandle *, CtkConfig *, 
                                  CtkEvent *ctk_event);

GtkTextBuffer *ctk_xvideo_create_help(GtkTextTagTable *, CtkXVideo *);

G_END_DECLS

#endif /* __CTK_XVIDEO_H__ */

