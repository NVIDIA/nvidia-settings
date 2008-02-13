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
 *
 * This source file implements the banner widget.
 */

#include <gtk/gtk.h>
#include <stdio.h>

#include "image.h"
#include "ctkbanner.h"

#include <gdk-pixbuf/gdk-pixdata.h>

/* pixdata headers */

#include "background_pixdata.h"
#include "background_tall_pixdata.h"
#include "logo_pixdata.h"
#include "logo_tall_pixdata.h"

#include "antialias_pixdata.h"
#include "bsd_pixdata.h"
#include "clock_pixdata.h"
#include "color_pixdata.h"
#include "config_pixdata.h"
#include "crt_pixdata.h"
#include "cursor_shadow_pixdata.h"
#include "dfp_pixdata.h"
#include "display_config_pixdata.h"
#include "framelock_pixdata.h"
#include "glx_pixdata.h"
#include "gpu_pixdata.h"
#include "help_pixdata.h"
#include "opengl_pixdata.h"
#include "penguin_pixdata.h"
#include "rotation_pixdata.h"
#include "sdi_pixdata.h"
#include "solaris_pixdata.h"
#include "thermal_pixdata.h"
#include "tv_pixdata.h"
#include "vcsc_pixdata.h"
#include "x_pixdata.h"
#include "xvideo_pixdata.h"




static void
ctk_banner_class_init    (CtkBannerClass *);

static void
ctk_banner_finalize      (GObject *);

static gboolean
ctk_banner_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_banner_size_request  (GtkWidget *, GtkRequisition *);

static gboolean
ctk_banner_configure_event  (GtkWidget *, GdkEventConfigure *);

static GObjectClass *parent_class;


/* global shared copy of background and logo images */

static PBuf Background = { 0, 0, NULL };
static PBuf TallBackground = { 0, 0, NULL };
static PBuf Logo = { 0, 0, NULL };
static PBuf TallLogo = { 0, 0, NULL };


GType ctk_banner_get_type(
    void
)
{
    static GType ctk_banner_type = 0;

    if (!ctk_banner_type) {
        static const GTypeInfo ctk_banner_info = {
            sizeof (CtkBannerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_banner_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkBanner),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_banner_type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                        "CtkBanner", &ctk_banner_info, 0);
    }

    return ctk_banner_type;
}

static void ctk_banner_class_init(
    CtkBannerClass *ctk_banner_class
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_banner_class;
    gobject_class = (GObjectClass *) ctk_banner_class;

    parent_class = g_type_class_peek_parent(ctk_banner_class);

    gobject_class->finalize = ctk_banner_finalize;

    widget_class->expose_event = ctk_banner_expose_event;
    widget_class->size_request = ctk_banner_size_request;
    widget_class->configure_event = ctk_banner_configure_event;
}

static void ctk_banner_finalize(
    GObject *object
)
{
    CtkBanner *ctk_banner = CTK_BANNER(object);

    if (ctk_banner->back.pixbuf)
        g_object_unref(ctk_banner->back.pixbuf);

    if (ctk_banner->artwork.pixbuf)
        g_object_unref(ctk_banner->artwork.pixbuf);

    if (ctk_banner->logo->pixbuf)
        g_object_unref(ctk_banner->logo->pixbuf);

    if (ctk_banner->background->pixbuf)
        g_object_unref(ctk_banner->background->pixbuf);
}

static gboolean ctk_banner_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);

    /* copy the backing pixbuf into the exposed portion of the window */

    gdk_draw_pixbuf(widget->window,
                    widget->style->fg_gc[GTK_STATE_NORMAL],
                    ctk_banner->back.pixbuf,
                    0, 0, // start x,y from src
                    0, 0, // dest x,y
                    ctk_banner->back.w, // width,height of copy region
                    ctk_banner->back.h,
                    GDK_RGB_DITHER_NORMAL, 0, 0);
    
    return FALSE;
}



/*
 * ctk_banner_configure_event() - the banner was configured; composite
 * the backing pixbuf image
 */

static gboolean ctk_banner_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);
    
    int x, y, w, h, needed_w, needed_h;
    
    /* free the pixbuf we already have one */

    if (ctk_banner->back.pixbuf)
        g_object_unref(ctk_banner->back.pixbuf);
    
    /* allocate a backing pixbuf the size of the new window */
    
    ctk_banner->back.pixbuf =
        gdk_pixbuf_new(GDK_COLORSPACE_RGB, // colorSpace
                       FALSE, // has_alpha (no alpha needed for backing pixbuf)
                       gdk_pixbuf_get_bits_per_sample
                       (ctk_banner->background->pixbuf),
                       event->width,
                       event->height);  
    
    ctk_banner->back.w = gdk_pixbuf_get_width(ctk_banner->back.pixbuf);
    ctk_banner->back.h = gdk_pixbuf_get_height(ctk_banner->back.pixbuf);
    
    /* clear the backing pixbuf to black */

    gdk_pixbuf_fill(ctk_banner->back.pixbuf, 0x00000000);

    /* copy the base image into the backing pixbuf */

    w = MIN(ctk_banner->background->w, ctk_banner->back.w);
    h = MIN(ctk_banner->background->h, ctk_banner->back.h);
    

    gdk_pixbuf_copy_area(ctk_banner->background->pixbuf, // src
                         0,                               // src_x
                         0,                               // src_y
                         w,                               // width
                         h,                               // height
                         ctk_banner->back.pixbuf,        // dest
                         0,                               // dest_x
                         0);                              // dest_y

    /*
     * composite the logo into the backing pixbuf; positioned in the
     * upper right corner of the backing pixbuf.  We should only do
     * this, though, if the backing pixbuf is large enough to contain
     * the logo
     */
    
    needed_w = ctk_banner->logo->w + ctk_banner->logo_pad_x;
    needed_h = ctk_banner->logo->h + ctk_banner->logo_pad_y;
    
    if ((ctk_banner->back.w >= needed_w) &&
        (ctk_banner->back.h >= needed_h)) {
        
        w = ctk_banner->logo->w;
        h = ctk_banner->logo->h;
        
        x = ctk_banner->back.w - w;
        y = 0;
        
        gdk_pixbuf_composite(ctk_banner->logo->pixbuf,   // src
                             ctk_banner->back.pixbuf,    // dest
                             x - ctk_banner->logo_pad_x, // dest_x
                             y + ctk_banner->logo_pad_y, // dest_y
                             w,                           // dest_width
                             h,                           // dest_height
                             x - ctk_banner->logo_pad_x, // offset_x
                             y + ctk_banner->logo_pad_y, // offset_y
                             1.0,                         // scale_x
                             1.0,                         // scale_y
                             GDK_INTERP_BILINEAR,         // interp_type
                             255);                        // overall_alpha
    }
    
    /*
     * composite the artwork into the lower left corner of the backing
     * pixbuf
     */
   
    needed_w = ctk_banner->artwork.w + ctk_banner->artwork_pad_x;
    needed_h = ctk_banner->artwork.h;

    if ((ctk_banner->back.w >= needed_w) &&
        (ctk_banner->back.h >= needed_h)) {
        
        w = ctk_banner->artwork.w;
        h = ctk_banner->artwork.h;
        
        x = 0;
        y = ctk_banner->back.h - h;

        gdk_pixbuf_composite(ctk_banner->artwork.pixbuf,    // src
                             ctk_banner->back.pixbuf,       // dest
                             x + ctk_banner->artwork_pad_x, // dest_x
                             y,                              // dest_y
                             w,                              // dest_width
                             h,                              // dest_height
                             x + ctk_banner->artwork_pad_x, // offset_x
                             y,                              // offset_y
                             1.0,                            // scale_x
                             1.0,                            // scale_y
                             GDK_INTERP_BILINEAR,            // interp_type
                             255);                           // overall_alpha
    }
    
    return FALSE;
}


static void ctk_banner_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);

    requisition->width = MAX(400,
                             ctk_banner->logo->w +
                             ctk_banner->artwork.w +
                             ctk_banner->logo_pad_x +
                             ctk_banner->artwork_pad_x);
    
    requisition->height = ctk_banner->background->h;
}



/*
 * select_artwork() - given a BannerArtworkType, lookup the pixdata
 * and other related data
 */

static gboolean select_artwork(BannerArtworkType artwork,
                               gboolean *tall,
                               int *pad_x,
                               const GdkPixdata **pixdata)
{
    static const struct {
        BannerArtworkType artwork;
        gboolean tall;
        int pad_x;
        const GdkPixdata *pixdata;
    } ArtworkTable[] = {
        /* artwork                       tall  pad_x pixdata */
        { BANNER_ARTWORK_ANTIALIAS,      FALSE, 16, &antialias_pixdata      },
        { BANNER_ARTWORK_BSD,            TRUE,  16, &bsd_pixdata            },
        { BANNER_ARTWORK_CLOCK,          FALSE, 16, &clock_pixdata          },
        { BANNER_ARTWORK_COLOR,          FALSE, 16, &color_pixdata          },
        { BANNER_ARTWORK_CONFIG,         FALSE, 16, &config_pixdata         },
        { BANNER_ARTWORK_CRT,            FALSE, 16, &crt_pixdata            },
        { BANNER_ARTWORK_CURSOR_SHADOW,  FALSE, 16, &cursor_shadow_pixdata  },
        { BANNER_ARTWORK_DFP,            FALSE, 16, &dfp_pixdata            },
        { BANNER_ARTWORK_DISPLAY_CONFIG, FALSE, 16, &display_config_pixdata },
        { BANNER_ARTWORK_FRAMELOCK,      FALSE, 16, &framelock_pixdata      },
        { BANNER_ARTWORK_GLX,            FALSE, 16, &glx_pixdata            },
        { BANNER_ARTWORK_GPU,            FALSE, 16, &gpu_pixdata            },
        { BANNER_ARTWORK_HELP,           FALSE, 16, &help_pixdata           },
        { BANNER_ARTWORK_OPENGL,         FALSE, 16, &opengl_pixdata         },
        { BANNER_ARTWORK_PENGUIN,        TRUE,  16, &penguin_pixdata        },
        { BANNER_ARTWORK_ROTATION,       FALSE, 16, &rotation_pixdata       },
        { BANNER_ARTWORK_SDI,            FALSE, 16, &sdi_pixdata            },
        { BANNER_ARTWORK_SOLARIS,        TRUE,  16, &solaris_pixdata        },
        { BANNER_ARTWORK_THERMAL,        FALSE, 16, &thermal_pixdata        },
        { BANNER_ARTWORK_TV,             FALSE, 16, &tv_pixdata             },
        { BANNER_ARTWORK_VCSC,           FALSE, 16, &vcsc_pixdata           },
        { BANNER_ARTWORK_X,              FALSE, 16, &x_pixdata              },
        { BANNER_ARTWORK_XVIDEO,         FALSE, 16, &xvideo_pixdata         },
        { BANNER_ARTWORK_INVALID,        FALSE, 16, NULL                    },
    };

    int i;

    for (i = 0; ArtworkTable[i].artwork != BANNER_ARTWORK_INVALID; i++) {
        if (ArtworkTable[i].artwork == artwork) {
            *tall = ArtworkTable[i].tall;
            *pad_x = ArtworkTable[i].pad_x;
            *pixdata = ArtworkTable[i].pixdata;
            return TRUE;
        }
    }
    
    return FALSE;
}



/*
 * ctk_banner_new() - allocate new banner object; open and read in
 * pixbufs that we will need later.
 */

GtkWidget* ctk_banner_new(BannerArtworkType artwork)
{
    GObject *object;
    CtkBanner *ctk_banner;
    const GdkPixdata *pixdata;
    int tall, pad_x;

    if (!select_artwork(artwork, &tall, &pad_x, &pixdata)) {
        return NULL;
    }
    
    object = g_object_new(CTK_TYPE_BANNER, NULL);
    
    ctk_banner = CTK_BANNER(object);
    
    ctk_banner->back.pixbuf = NULL;
    ctk_banner->artwork.pixbuf = NULL;
    
    ctk_banner->artwork_pad_x = pad_x;
    
    /* load the global images */

    if (!Background.pixbuf) {
        Background.pixbuf =
            gdk_pixbuf_from_pixdata(&background_pixdata, TRUE, NULL);
        Background.w = gdk_pixbuf_get_width(Background.pixbuf);
        Background.h = gdk_pixbuf_get_height(Background.pixbuf);
    }

    if (!TallBackground.pixbuf) {
        TallBackground.pixbuf =
            gdk_pixbuf_from_pixdata(&background_tall_pixdata, TRUE, NULL);
        TallBackground.w = gdk_pixbuf_get_width(TallBackground.pixbuf);
        TallBackground.h = gdk_pixbuf_get_height(TallBackground.pixbuf);
    }
    
    if (!Logo.pixbuf) {
        Logo.pixbuf = gdk_pixbuf_from_pixdata(&logo_pixdata, TRUE, NULL);
        Logo.w = gdk_pixbuf_get_width(Logo.pixbuf);
        Logo.h = gdk_pixbuf_get_height(Logo.pixbuf);
    }
    
    if (!TallLogo.pixbuf) {
        TallLogo.pixbuf =
            gdk_pixbuf_from_pixdata(&logo_tall_pixdata, TRUE, NULL);
        TallLogo.w = gdk_pixbuf_get_width(TallLogo.pixbuf);
        TallLogo.h = gdk_pixbuf_get_height(TallLogo.pixbuf);
    }
    
    
    /*
     * assign fields based on whether the artwork is tall; XXX these
     * may need to be tweaked
     */
    
    if (tall) {
        ctk_banner->logo_pad_x = 11;
        ctk_banner->logo_pad_y = 11;
        ctk_banner->background = &TallBackground;
        ctk_banner->logo = &TallLogo;
    } else {
        ctk_banner->logo_pad_x = 10;
        ctk_banner->logo_pad_y = 10;
        ctk_banner->background = &Background;
        ctk_banner->logo = &Logo;
    }
    
    
    /* load the artwork pixbuf */
    
    ctk_banner->artwork.pixbuf = gdk_pixbuf_from_pixdata(pixdata, TRUE, NULL);
    ctk_banner->artwork.w =
        gdk_pixbuf_get_width(ctk_banner->artwork.pixbuf);
    ctk_banner->artwork.h =
        gdk_pixbuf_get_height(ctk_banner->artwork.pixbuf);
    
    return GTK_WIDGET(object);
}
