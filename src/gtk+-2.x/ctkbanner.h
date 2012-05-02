/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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

#ifndef __CTK_BANNER_H__
#define __CTK_BANNER_H__

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

/*
 * enum for the banner artwork
 */

typedef enum {
    BANNER_ARTWORK_ANTIALIAS,
    BANNER_ARTWORK_BSD,
    BANNER_ARTWORK_CLOCK,
    BANNER_ARTWORK_COLOR,
    BANNER_ARTWORK_CONFIG,
    BANNER_ARTWORK_CRT,
    BANNER_ARTWORK_CURSOR_SHADOW,
    BANNER_ARTWORK_DFP,
    BANNER_ARTWORK_DISPLAY_CONFIG,
    BANNER_ARTWORK_FRAMELOCK,
    BANNER_ARTWORK_GLX,
    BANNER_ARTWORK_GPU,
    BANNER_ARTWORK_GVI,
    BANNER_ARTWORK_HELP,
    BANNER_ARTWORK_OPENGL,
    BANNER_ARTWORK_PENGUIN,
    BANNER_ARTWORK_SDI,
    BANNER_ARTWORK_SDI_SHARED_SYNC_BNC,
    BANNER_ARTWORK_SLIMM,
    BANNER_ARTWORK_SOLARIS,
    BANNER_ARTWORK_THERMAL,
    BANNER_ARTWORK_TV,
    BANNER_ARTWORK_VCS,
    BANNER_ARTWORK_X,
    BANNER_ARTWORK_XVIDEO,
    BANNER_ARTWORK_SVP,
    BANNER_ARTWORK_INVALID
} BannerArtworkType;


typedef struct _CtkBanner       CtkBanner;
typedef struct _CtkBannerClass  CtkBannerClass;

typedef void (* ctk_banner_composite_callback) (CtkBanner *, void *);

typedef struct {
    int w, h;
    GdkPixbuf *pixbuf;
} PBuf;

struct _CtkBanner
{
    GtkDrawingArea parent;
    
    guint8 *image_data;

    PBuf back;
    PBuf artwork;
    int artwork_x; /* Position within banner where artwork is drawn */
    int artwork_y;

    ctk_banner_composite_callback callback_func;
    void * callback_data;

    PBuf *background;
    PBuf *logo;
    int logo_x; /* Position within banner where logo is drawn */
    int logo_y;

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

void        ctk_banner_set_composite_callback (CtkBanner *,
                                               ctk_banner_composite_callback,
                                               void *);

GtkWidget*  ctk_banner_image_new   (BannerArtworkType artwork);

GtkWidget*  ctk_banner_image_new_with_callback (BannerArtworkType artwork,
                                                ctk_banner_composite_callback,
                                                void *);


G_END_DECLS

#endif /* __CTK_BANNER_H__ */

