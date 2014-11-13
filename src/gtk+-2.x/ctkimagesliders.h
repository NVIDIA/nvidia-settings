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

#ifndef __CTK_IMAGE_SLIDERS_H__
#define __CTK_IMAGE_SLIDERS_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_IMAGE_SLIDERS (ctk_image_sliders_get_type())

#define CTK_IMAGE_SLIDERS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_IMAGE_SLIDERS, \
                                 CtkImageSliders))

#define CTK_IMAGE_SLIDERS_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_IMAGE_SLIDERS, \
                              CtkImageSlidersClass))

#define CTK_IS_IMAGE_SLIDERS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_IMAGE_SLIDERS))

#define CTK_IS_IMAGE_SLIDERS_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_IMAGE_SLIDERS))

#define CTK_IMAGE_SLIDERS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_IMAGE_SLIDERS, \
                                CtkImageSlidersClass))

typedef struct _CtkImageSliders       CtkImageSliders;
typedef struct _CtkImageSlidersClass  CtkImageSlidersClass;

struct _CtkImageSliders
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    char *name;

    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    GtkWidget *reset_button;

    GtkWidget *frame;

    GtkWidget *digital_vibrance;
    GtkWidget *image_sharpening;
};

struct _CtkImageSlidersClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_image_sliders_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_image_sliders_new       (CtrlTarget *,
                                         CtkConfig *, CtkEvent *,
                                         GtkWidget *reset_button,
                                         char *name);

void ctk_image_sliders_reset(CtkImageSliders *);

void ctk_image_sliders_setup(CtkImageSliders *ctk_image_sliders);

void add_image_sliders_help(CtkImageSliders *ctk_image_sliders,
                            GtkTextBuffer *b,
                            GtkTextIter *i);

G_END_DECLS

#endif /* __CTK_IMAGE_SLIDERS_H__ */
