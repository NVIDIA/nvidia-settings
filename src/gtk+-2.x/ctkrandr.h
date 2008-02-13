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

#ifndef __CTK_RANDR_H__
#define __CTK_RANDR_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_RANDR (ctk_randr_get_type())

#define CTK_RANDR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_RANDR, CtkRandR))

#define CTK_RANDR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_RANDR, CtkRandRClass))

#define CTK_IS_RANDR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_RANDR))

#define CTK_IS_RANDR_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_RANDR))

#define CTK_RANDR_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_RANDR, CtkRandRClass))


/* Image pixbuf indices */
#define CTKRANDR_IMG_ROTATION_NORMAL   RR_Rotate_0
#define CTKRANDR_IMG_ROTATION_LEFT     RR_Rotate_90
#define CTKRANDR_IMG_ROTATION_INVERTED RR_Rotate_180
#define CTKRANDR_IMG_ROTATION_RIGHT    RR_Rotate_270

/* Button pixbuf indices */
#define CTKRANDR_BTN_ROTATE_LEFT_OFF  0
#define CTKRANDR_BTN_ROTATE_LEFT_ON   1
#define CTKRANDR_BTN_ROTATE_RIGHT_OFF 2
#define CTKRANDR_BTN_ROTATE_RIGHT_ON  3


typedef struct _CtkRandR
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkLabel *label;

    GtkImage  *orientation_image;
    GdkPixbuf *orientation_image_pixbufs[9];

    GtkImage  *rotate_left_button_image;
    Bool       rotate_left_button_pressed;
    GtkImage  *rotate_right_button_image;
    Bool       rotate_right_button_pressed;
    GdkPixbuf *button_pixbufs[4];

} CtkRandR;

typedef struct _CtkRandRClass
{
    GtkVBoxClass parent_class;
} CtkRandRClass;

GType       ctk_randr_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_randr_new       (NvCtrlAttributeHandle *, CtkConfig *,
                                 CtkEvent *);

GtkTextBuffer *ctk_randr_create_help(GtkTextTagTable *, CtkRandR *);

G_END_DECLS

#endif /* __CTK_RANDR_H__ */
