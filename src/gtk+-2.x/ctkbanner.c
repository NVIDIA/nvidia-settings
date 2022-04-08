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
 *
 *
 * This source file implements the banner widget.
 */

#include <gtk/gtk.h>
#include <stdio.h>

#include "ctkbanner.h"
#include "common-utils.h"
#include "ctkutils.h"

/* PNG headers */

#include "background.png.h"
#include "background_tall.png.h"
#include "logo.png.h"
#include "logo_tall.png.h"

#include "antialias.png.h"
#include "bsd.png.h"
#include "clock.png.h"
#include "color.png.h"
#include "config.png.h"
#include "crt.png.h"
#include "dfp.png.h"
#include "display_config.png.h"
#include "framelock.png.h"
#include "gpu.png.h"
#include "graphics.png.h"
#include "help.png.h"
#include "opengl.png.h"
#include "penguin.png.h"
#include "server_licensing.png.h"
#include "slimm.png.h"
#include "solaris.png.h"
#include "thermal.png.h"
#include "vdpau.png.h"
#include "x.png.h"
#include "xvideo.png.h"
#include "svp_3dvp.png.h"


static void
ctk_banner_class_init    (CtkBannerClass *, gpointer);

static void
ctk_banner_finalize      (GObject *);
#ifdef CTK_GTK3
static gboolean
ctk_banner_draw_event  (GtkWidget *, cairo_t *);

static void
ctk_banner_get_preferred_width (GtkWidget *, gint *, gint *);

static void
ctk_banner_get_preferred_height(GtkWidget *, gint *, gint *);
#else
static gboolean
ctk_banner_expose_event  (GtkWidget *, GdkEventExpose *);

static void
ctk_banner_size_request  (GtkWidget *, GtkRequisition *);
#endif

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
            NULL  /* value_table */
        };

        ctk_banner_type = g_type_register_static(GTK_TYPE_DRAWING_AREA,
                        "CtkBanner", &ctk_banner_info, 0);
    }

    return ctk_banner_type;
}

static void ctk_banner_class_init(
    CtkBannerClass *ctk_banner_class,
    gpointer class_data
)
{
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_banner_class;
    gobject_class = (GObjectClass *) ctk_banner_class;

    parent_class = g_type_class_peek_parent(ctk_banner_class);

    gobject_class->finalize = ctk_banner_finalize;

#ifdef CTK_GTK3
    widget_class->draw = ctk_banner_draw_event;
    widget_class->get_preferred_width  = ctk_banner_get_preferred_width;
    widget_class->get_preferred_height = ctk_banner_get_preferred_height;
#else
    widget_class->expose_event = ctk_banner_expose_event;
    widget_class->size_request = ctk_banner_size_request;
#endif
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

#ifdef CTK_GTK3
static gboolean ctk_banner_draw_event(
    GtkWidget *widget,
    cairo_t *cr_i
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));

    /* copy the backing pixbuf into the exposed portion of the window */

    gdk_cairo_set_source_pixbuf(cr, ctk_banner->back.pixbuf, 0, 0);
    cairo_paint(cr);

    cairo_destroy(cr);

    return FALSE;
}
#else
static gboolean ctk_banner_expose_event(
    GtkWidget *widget,
    GdkEventExpose *event
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);
    GdkRectangle *rects;
    int n_rects;
    int i;

    /* copy the backing pixbuf into the exposed portion of the window */

    gdk_region_get_rectangles(event->region, &rects, &n_rects);

    for (i = 0; i < n_rects; i++) {
        gdk_draw_pixbuf(widget->window,
                        widget->style->fg_gc[GTK_STATE_NORMAL],
                        ctk_banner->back.pixbuf,
                        rects[i].x, rects[i].y,
                        rects[i].x, rects[i].y,
                        rects[i].width, rects[i].height,
                        GDK_RGB_DITHER_NORMAL, 0, 0);
    }

    g_free(rects);

    return FALSE;
}
#endif



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

    w = NV_MIN(ctk_banner->background->w, ctk_banner->back.w);
    h = NV_MIN(ctk_banner->background->h, ctk_banner->back.h);


    gdk_pixbuf_copy_area(ctk_banner->background->pixbuf,  // src
                         0,                               // src_x
                         0,                               // src_y
                         w,                               // width
                         h,                               // height
                         ctk_banner->back.pixbuf,         // dest
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
        
        ctk_banner->logo_x = x - ctk_banner->logo_pad_x;
        ctk_banner->logo_y = y + ctk_banner->logo_pad_y;

        gdk_pixbuf_composite(ctk_banner->logo->pixbuf,  // src
                             ctk_banner->back.pixbuf,   // dest
                             ctk_banner->logo_x,        // dest_x
                             ctk_banner->logo_y,        // dest_y
                             w,                         // dest_width
                             h,                         // dest_height
                             ctk_banner->logo_x,        // offset_x
                             ctk_banner->logo_y,        // offset_y
                             1.0,                       // scale_x
                             1.0,                       // scale_y
                             GDK_INTERP_BILINEAR,       // interp_type
                             255);                      // overall_alpha
    }
    
    /*
     * composite the artwork into the lower left corner of the backing
     * pixbuf
     */
   
    needed_w = ctk_banner->artwork.w + ctk_banner->artwork_pad_x;
    needed_h = ctk_banner->artwork.h;

    if (ctk_banner->artwork.pixbuf &&
        (ctk_banner->back.w >= needed_w) &&
        (ctk_banner->back.h >= needed_h)) {
        
        w = ctk_banner->artwork.w;
        h = ctk_banner->artwork.h;
        
        x = 0;
        y = ctk_banner->back.h - h;

        ctk_banner->artwork_x = x + ctk_banner->artwork_pad_x;
        ctk_banner->artwork_y = y;

        gdk_pixbuf_composite(ctk_banner->artwork.pixbuf,    // src
                             ctk_banner->back.pixbuf,       // dest
                             ctk_banner->artwork_x,         // dest_x
                             ctk_banner->artwork_y,         // dest_y
                             w,                             // dest_width
                             h,                             // dest_height
                             ctk_banner->artwork_x,         // offset_x
                             ctk_banner->artwork_y,         // offset_y
                             1.0,                           // scale_x
                             1.0,                           // scale_y
                             GDK_INTERP_BILINEAR,           // interp_type
                             255);                          // overall_alpha

        /* Do any user-specific compositing */

        if (ctk_banner->callback_func) {
            ctk_banner->callback_func(ctk_banner, ctk_banner->callback_data);
        }
    }
    
    return FALSE;
}


static void ctk_banner_size_request(
    GtkWidget *widget,
    GtkRequisition *requisition
)
{
    CtkBanner *ctk_banner = CTK_BANNER(widget);

    requisition->width = NV_MAX(400,
                                ctk_banner->logo->w +
                                ctk_banner->artwork.w +
                                ctk_banner->logo_pad_x +
                                ctk_banner->artwork_pad_x);

    requisition->height = ctk_banner->background->h;
}

#ifdef CTK_GTK3
static void ctk_banner_get_preferred_width(
    GtkWidget *widget,
    gint *minimum_width,
    gint *natural_width
)
{
    GtkRequisition requisition;
    ctk_banner_size_request(widget, &requisition);
    *minimum_width = *natural_width = requisition.width;
}

static void ctk_banner_get_preferred_height(
    GtkWidget *widget,
    gint *minimum_height,
    gint *natural_height
)
{
    GtkRequisition requisition;
    ctk_banner_size_request(widget, &requisition);
    *minimum_height = *natural_height = requisition.height;
}
#endif



/*
 * select_artwork() - given a BannerArtworkType, lookup the pixbuf and other
 * related data
 */

static GdkPixbuf* select_artwork(BannerArtworkType artwork,
                                 gboolean *tall,
                                 int *pad_x)
{
    static const struct {
        BannerArtworkType artwork;
        gboolean tall;
        int pad_x;
        const char *png_start;
        const char *png_end;
    } ArtworkTable[] = {
#define PNG(s) _binary_##s##_png_start, _binary_##s##_png_end
        /* artwork                       tall  pad_x     png_start, png_end       }, */
        { BANNER_ARTWORK_ANTIALIAS,      FALSE, 16,      PNG(antialias)           },
        { BANNER_ARTWORK_BSD,            TRUE,  16,      PNG(bsd)                 },
        { BANNER_ARTWORK_CLOCK,          FALSE, 16,      PNG(clock)               },
        { BANNER_ARTWORK_COLOR,          FALSE, 16,      PNG(color)               },
        { BANNER_ARTWORK_CONFIG,         FALSE, 16,      PNG(config)              },
        { BANNER_ARTWORK_CRT,            FALSE, 16,      PNG(crt)                 },
        { BANNER_ARTWORK_DFP,            FALSE, 16,      PNG(dfp)                 },
        { BANNER_ARTWORK_DISPLAY_CONFIG, FALSE, 16,      PNG(display_config)      },
        { BANNER_ARTWORK_FRAMELOCK,      FALSE, 16,      PNG(framelock)           },
        { BANNER_ARTWORK_GPU,            FALSE, 16,      PNG(gpu)                 },
        { BANNER_ARTWORK_GRAPHICS,       FALSE, 16,      PNG(graphics)            },
        { BANNER_ARTWORK_HELP,           FALSE, 16,      PNG(help)                },
        { BANNER_ARTWORK_OPENGL,         FALSE, 16,      PNG(opengl)              },
        { BANNER_ARTWORK_PENGUIN,        TRUE,  16,      PNG(penguin)             },
        { BANNER_ARTWORK_SERVER_LICENSING, FALSE, 16,    PNG(server_licensing)    },
        { BANNER_ARTWORK_SLIMM,          FALSE, 16,      PNG(slimm)               },
        { BANNER_ARTWORK_SOLARIS,        TRUE,  16,      PNG(solaris)             },
        { BANNER_ARTWORK_THERMAL,        FALSE, 16,      PNG(thermal)             },
        { BANNER_ARTWORK_VDPAU,          FALSE, 16,      PNG(vdpau)               },
        { BANNER_ARTWORK_X,              FALSE, 16,      PNG(x)                   },
        { BANNER_ARTWORK_XVIDEO,         FALSE, 16,      PNG(xvideo)              },
        { BANNER_ARTWORK_SVP,            FALSE, 16,      PNG(svp_3dvp)            },
        { BANNER_ARTWORK_BLANK,          FALSE, 16,      NULL, NULL               },
#undef PNG
    };

    int i;

    for (i = 0; ArtworkTable[i].artwork <= BANNER_ARTWORK_BLANK; i++) {
        if (ArtworkTable[i].artwork == artwork) {
            *tall = ArtworkTable[i].tall;
            *pad_x = ArtworkTable[i].pad_x;

            return ctk_pixbuf_from_data(ArtworkTable[i].png_start,
                                        ArtworkTable[i].png_end);
        }
    }

    return NULL;
}



/*
 * ctk_banner_new() - allocate new banner object; open and read in
 * pixbufs that we will need later.
 */

GtkWidget* ctk_banner_new(BannerArtworkType artwork)
{
    GObject *object;
    CtkBanner *ctk_banner;

    int tall = 0, pad_x = 0;
    GdkPixbuf *pixbuf = select_artwork(artwork, &tall, &pad_x);

    object = g_object_new(CTK_TYPE_BANNER, NULL);
    
    ctk_banner = CTK_BANNER(object);
    
    ctk_banner->back.pixbuf = NULL;
    ctk_banner->artwork.pixbuf = NULL;
    
    ctk_banner->artwork_pad_x = pad_x;
    
    /* load the global images */

    if (!Background.pixbuf) {
        Background.pixbuf = CTK_LOAD_PIXBUF(background);
        Background.w = gdk_pixbuf_get_width(Background.pixbuf);
        Background.h = gdk_pixbuf_get_height(Background.pixbuf);
    }
    g_object_ref(Background.pixbuf);

    if (!TallBackground.pixbuf) {
        TallBackground.pixbuf = CTK_LOAD_PIXBUF(background_tall);
        TallBackground.w = gdk_pixbuf_get_width(TallBackground.pixbuf);
        TallBackground.h = gdk_pixbuf_get_height(TallBackground.pixbuf);
    }
    g_object_ref(TallBackground.pixbuf);
    
    if (!Logo.pixbuf) {
        Logo.pixbuf = CTK_LOAD_PIXBUF(logo);
        Logo.w = gdk_pixbuf_get_width(Logo.pixbuf);
        Logo.h = gdk_pixbuf_get_height(Logo.pixbuf);
    }
    g_object_ref(Logo.pixbuf);
    
    if (!TallLogo.pixbuf) {
        TallLogo.pixbuf = CTK_LOAD_PIXBUF(logo_tall);
        TallLogo.w = gdk_pixbuf_get_width(TallLogo.pixbuf);
        TallLogo.h = gdk_pixbuf_get_height(TallLogo.pixbuf);
    }
    g_object_ref(TallLogo.pixbuf);

    /*
     * assign fields based on whether the artwork is tall; XXX these
     * may need to be tweaked
     */
    
    if (tall) {
        ctk_banner->logo_pad_x = 11;
        ctk_banner->logo_pad_y = 0;
        ctk_banner->background = &TallBackground;
        ctk_banner->logo = &TallLogo;
    } else {
        ctk_banner->logo_pad_x = 10;
        ctk_banner->logo_pad_y = 10;
        ctk_banner->background = &Background;
        ctk_banner->logo = &Logo;
    }
    
    
    /* load the artwork pixbuf */
    if (pixbuf) {
        ctk_banner->artwork.pixbuf = pixbuf;
        ctk_banner->artwork.w = gdk_pixbuf_get_width(pixbuf);
        ctk_banner->artwork.h = gdk_pixbuf_get_height(pixbuf);
    }

    return GTK_WIDGET(object);
}



void ctk_banner_set_composite_callback (CtkBanner *ctk_banner,
                                        ctk_banner_composite_callback func,
                                        void *data)
{
    ctk_banner->callback_func = func;
    ctk_banner->callback_data = data;
}



/*
 * CTK composited banner image widget creation
 *
 */
GtkWidget* ctk_banner_image_new_with_callback(BannerArtworkType artwork,
                                              ctk_banner_composite_callback callback,
                                              void *data)
{
    GtkWidget *image;
    GtkWidget *hbox;
    GtkWidget *frame;


    image = ctk_banner_new(artwork);

    if (!image) return NULL;

    ctk_banner_set_composite_callback(CTK_BANNER(image), callback, data);

    hbox = gtk_hbox_new(FALSE, 0);
    frame = gtk_frame_new(NULL);

    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(frame), image);


    return hbox;
}



GtkWidget* ctk_banner_image_new(BannerArtworkType artwork)
{
    return ctk_banner_image_new_with_callback(artwork, NULL, NULL);
}
