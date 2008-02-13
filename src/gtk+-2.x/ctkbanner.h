/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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

#ifndef __CTK_BANNER_H__
#define __CTK_BANNER_H__

#include "image.h"

G_BEGIN_DECLS

#define CTK_TYPE_BANNER (ctk_banner_get_type())

#define CTK_BANNER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_BANNER, CtkBanner))

#define CTK_BANNER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_BANNER, CtkBannerClass))

#define CTK_IS_BANNER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_BANNER))

#define CTK_IS_BANNER_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_BANNER))

#define CTK_BANNER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_BANNER, CtkBannerClass))


typedef struct _CtkBanner       CtkBanner;
typedef struct _CtkBannerClass  CtkBannerClass;

typedef struct {
    int w, h;
    GdkPixbuf *pixbuf;
} PBuf;

struct _CtkBanner
{
    GtkDrawingArea parent;
    
    const nv_image_t *img;
    
    guint8 *image_data;

    PBuf back;
    PBuf artwork;

    PBuf *background;
    PBuf *logo;

    int logo_pad_x;
    int logo_pad_y;
    
    int artwork_pad_x;
};

struct _CtkBannerClass
{
    GtkDrawingAreaClass parent_class;
};

GType       ctk_banner_get_type     (void) G_GNUC_CONST;
GtkWidget*  ctk_banner_new          (BannerArtworkType);

G_END_DECLS

#endif /* __CTK_BANNER_H__ */

