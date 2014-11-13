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

/*
 * The CtkGvoBanner widget is in charge of displaying the GVO Banner image.
 * The LEDs are drawn based on the state currently set by the 
 * ctk_gvo_banner_update_video_output() and
 * ctk_gvo_banner_update_video_input() functions.  It is the caller's
 * job to set the appropriate state so that the banner can be drawn correctly.
 */

#include <gtk/gtk.h>
#include <string.h>

#include "NvCtrlAttributes.h"

#include "ctkhelp.h"
#include "ctkgvo-banner.h"
#include "ctkutils.h"
#include "ctkbanner.h"

#include "msg.h"



/* values for controlling LED state */

#define GVO_LED_VID_OUT_NOT_IN_USE 0
#define GVO_LED_VID_OUT_HD_MODE    1
#define GVO_LED_VID_OUT_SD_MODE    2

#define GVO_LED_SDI_SYNC_NONE      0
#define GVO_LED_SDI_SYNC_HD        1
#define GVO_LED_SDI_SYNC_SD        2
#define GVO_LED_SDI_SYNC_ERROR     3

#define GVO_LED_COMP_SYNC_NONE     0
#define GVO_LED_COMP_SYNC_SYNC     1

/* LED colors */

#define LED_GREY    0
#define LED_GREEN   1
#define LED_YELLOW  2
#define LED_RED     3

/* How often the LEDs in the banner should be updated */

#define UPDATE_GVO_BANNER_TIME_INTERVAL  200
#define DEFAULT_GVO_PROBE_TIME_INTERVAL 1000




/* Position of LEDs relative to the SDI image, used for drawing LEDs */

static int __led_pos_x[] = { 74, 101, 128, 156 }; // From sdi.png
static int __led_pos_y   = 36;                    // From sdi.png



/* local prototypes */

static void composite_callback(CtkBanner *ctk_banner, void *data);

static gboolean update_gvo_banner_led_images(gpointer data);
static gboolean update_gvo_banner_led_images_shared_sync_bnc(gpointer data);

static void update_gvo_banner_led_state(CtkGvoBanner *ctk_gvo_banner);

static void gvo_event_received(GObject *object,
                               CtrlEvent *event,
                               gpointer user_data);

static void ctk_gvo_banner_class_init(CtkGvoBannerClass *ctk_object_class);
static void ctk_gvo_banner_finalize(GObject *object);



/*
 * ctk_gvo_banner_get_type() - Returns the GType for a CtkGvoBanner object
 */

GType ctk_gvo_banner_get_type(void)
{
    static GType ctk_gvo_banner_type = 0;

    if (!ctk_gvo_banner_type) {
        static const GTypeInfo ctk_gvo_banner_info = {
            sizeof (CtkGvoBannerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_gvo_banner_class_init, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGvoBanner),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_gvo_banner_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkGvoBanner",
                                   &ctk_gvo_banner_info, 0);
    }
    
    return ctk_gvo_banner_type;
    
} /* ctk_gvo_banner_get_type() */



static void ctk_gvo_banner_class_init(CtkGvoBannerClass *ctk_object_class)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_gvo_banner_finalize;
}



static void ctk_gvo_banner_finalize(GObject *object)
{
    CtkGvoBanner *ctk_object = CTK_GVO_BANNER(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}



/*
 * ctk_gvo_banner_new() - constructor for the CtkGvoBanner widget
 */

GtkWidget* ctk_gvo_banner_new(CtrlTarget *ctrl_target,
                              CtkConfig *ctk_config,
                              CtkEvent *ctk_event)
{
    GObject *object;
    CtkGvoBanner *ctk_gvo_banner;
    ReturnStatus ret;
    gint val;
    gint caps;


    /* make sure we have a valid target */

    g_return_val_if_fail((ctrl_target != NULL) &&
                         (ctrl_target->h != NULL), NULL);

    /* check if this screen supports GVO */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_SUPPORTED, &val);
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_GVO_SUPPORTED_TRUE)) {
        /* GVO not available */
        return NULL;
    }

    /* get the GVO capabilities */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_CAPABILITIES, &caps);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* create the CtkGvoBanner object */
    
    object = g_object_new(CTK_TYPE_GVO_BANNER, NULL);
    
    /* initialize fields in the CtkGvoBanner object */
    
    ctk_gvo_banner = CTK_GVO_BANNER(object);
    ctk_gvo_banner->ctrl_target = ctrl_target;
    ctk_gvo_banner->ctk_config = ctk_config;
    ctk_gvo_banner->ctk_event = ctk_event;
    ctk_gvo_banner->parent_box = NULL;

    /* handle GVO devices that share the sync input differently */

    ctk_gvo_banner->shared_sync_bnc =
        !!(caps & NV_CTRL_GVO_CAPABILITIES_SHARED_SYNC_BNC);

    /* create the banner image */

    if (ctk_gvo_banner->shared_sync_bnc) {
        ctk_gvo_banner->image =
            ctk_banner_image_new_with_callback
            (BANNER_ARTWORK_SDI_SHARED_SYNC_BNC,
             composite_callback,
             ctk_gvo_banner);
    } else {
        ctk_gvo_banner->image =
            ctk_banner_image_new_with_callback(BANNER_ARTWORK_SDI,
                                               composite_callback,
                                               ctk_gvo_banner);
    }

    g_object_ref(ctk_gvo_banner->image);

    gtk_box_pack_start(GTK_BOX(ctk_gvo_banner), ctk_gvo_banner->image,
                       FALSE, FALSE, 0);

    ctk_gvo_banner->ctk_banner = NULL;

    /* initialize LED state */

    ctk_gvo_banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_NOT_IN_USE;
    ctk_gvo_banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    ctk_gvo_banner->state[GVO_BANNER_SDI]  = GVO_LED_SDI_SYNC_NONE;
    ctk_gvo_banner->state[GVO_BANNER_COMP] = GVO_LED_COMP_SYNC_NONE;

    ctk_gvo_banner->img[GVO_BANNER_VID1] = LED_GREY;
    ctk_gvo_banner->img[GVO_BANNER_VID2] = LED_GREY;
    ctk_gvo_banner->img[GVO_BANNER_SDI]  = LED_GREY;
    ctk_gvo_banner->img[GVO_BANNER_COMP] = LED_GREY;

    /* Get the current GVO state */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_LOCK_OWNER, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_LOCK_OWNER_NONE;
    }
    ctk_gvo_banner->gvo_lock_owner = val;

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_SYNC_MODE, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING;
    }
    ctk_gvo_banner->sync_mode = val;

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_SYNC_SOURCE, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE;
    }
    ctk_gvo_banner->sync_source = val;

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVIO_VIDEO_FORMAT_NONE;
    }
    ctk_gvo_banner->output_video_format = val;

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_DATA_FORMAT, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444;
    }
    ctk_gvo_banner->output_data_format = val;

    /* Update the current LED state */

    update_gvo_banner_led_state(ctk_gvo_banner);

    /*
     * register a timeout function (directly with glib, not through
     * ctk_config) to update the LEDs
     */

    if (ctk_gvo_banner->shared_sync_bnc) {
        g_timeout_add(UPDATE_GVO_BANNER_TIME_INTERVAL,
                      update_gvo_banner_led_images_shared_sync_bnc,
                      ctk_gvo_banner);
    } else {
        g_timeout_add(UPDATE_GVO_BANNER_TIME_INTERVAL,
                      update_gvo_banner_led_images,
                      ctk_gvo_banner);
    }
        
    /* Add a timer so we can probe the hardware */

    ctk_config_add_timer(ctk_gvo_banner->ctk_config,
                         DEFAULT_GVO_PROBE_TIME_INTERVAL,
                         "Graphics To Video Probe",
                         (GSourceFunc) ctk_gvo_banner_probe,
                         (gpointer) ctk_gvo_banner);

    /* Listen for events */

    g_signal_connect(G_OBJECT(ctk_gvo_banner->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_LOCK_OWNER),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo_banner);

    g_signal_connect(G_OBJECT(ctk_gvo_banner->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_MODE),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo_banner);

    g_signal_connect(G_OBJECT(ctk_gvo_banner->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_SOURCE),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo_banner);

    g_signal_connect(G_OBJECT(ctk_gvo_banner->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo_banner);

    g_signal_connect(G_OBJECT(ctk_gvo_banner->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_DATA_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo_banner);

    /* show the GVO banner widget */
    
    gtk_widget_show_all(GTK_WIDGET(ctk_gvo_banner));

    return GTK_WIDGET(ctk_gvo_banner);

} /* ctk_gvo_banner_new() */



/*
 * draw_led() - Updates the LED to the given color in the banner's
 * backing pixbuf.
 */

static void draw_led(CtkBanner *ctk_banner, int led, int color) {
    
    /* Which LED to draw */
    int dst_x = ctk_banner->artwork_x +__led_pos_x[led];
    int dst_y = ctk_banner->artwork_y +__led_pos_y;

    /* Offset LED color into LED position */
    int offset_x = ctk_banner->artwork_x +__led_pos_x[led] -__led_pos_x[color];
    int offset_y = ctk_banner->artwork_y;

    gdk_pixbuf_composite(ctk_banner->artwork.pixbuf,    // src
                         ctk_banner->back.pixbuf,       // dest
                         dst_x,                         // dest_x
                         dst_y,                         // dest_y
                         10,                            // dest_width
                         10,                            // dest_height
                         offset_x,                      // offset_x
                         offset_y,                      // offset_y
                         1.0,                           // scale_x
                         1.0,                           // scale_y
                         GDK_INTERP_BILINEAR,           // interp_type
                         255);                          // overall_alpha

} /* draw_led() */



/*
 * composite_callback() - Draws all the LEDs to the banner.
 */

static void composite_callback(CtkBanner *ctk_banner, void *data)
{
    CtkGvoBanner *ctk_gvo_banner = (CtkGvoBanner *) data;
    int i;
    int last_led;

    /* Grab the latest banner widget */
    ctk_gvo_banner->ctk_banner = GTK_WIDGET(ctk_banner);

    /* Draw the current state */
    last_led = ctk_gvo_banner->shared_sync_bnc ? 3 : 4;

    for (i = 0; i < last_led; i++) {
        draw_led(ctk_banner, i, ctk_gvo_banner->img[i]);
    }

} /* composite_callback() */



/*
 * update_led_image() - Updates the state of an LED and causes and
 * expose event.
 */

static void update_led_image(CtkGvoBanner *banner, int led, int color)
{
    GtkWidget *ctk_banner = banner->ctk_banner;
    GdkRectangle rec = {0, __led_pos_y,  10, 10};

    /* Update the state of the LED */
    banner->img[led] = color;

    /* Draw the LED and tell gdk to draw it to the window */
    if (ctk_banner && ctk_widget_get_window(ctk_banner)) {

        draw_led(CTK_BANNER(ctk_banner), led, color);

        rec.x = CTK_BANNER(ctk_banner)->artwork_x + __led_pos_x[led];
        rec.y = CTK_BANNER(ctk_banner)->artwork_y + __led_pos_y;
        gdk_window_invalidate_rect(ctk_widget_get_window(ctk_banner), &rec, TRUE);
    }

} /* update_led_image() */



/*
 * update_gvo_banner_led_images() - called by a timer to update the LED images
 * based on current state
 */

static gboolean update_gvo_banner_led_images(gpointer data)
{
    guint8 old, new;
    CtkGvoBanner *banner = (CtkGvoBanner *) data;

    /*
     * we store the flashing state here:
     *
     * 0 == no LED is currently flashing
     * 1 == some LED is flashing; currently "on" (lit)
     * 2 == some LED is flashing; currently "off" (grey)
     *
     * this is used to track the current state, so that we can make
     * all LEDs flash at the same time.
     */

    gint flashing = 0;


    /* Vid 1 out */

    old = banner->img[GVO_BANNER_VID1];
    
    if (banner->state[GVO_BANNER_VID1] == GVO_LED_VID_OUT_HD_MODE) {
        new = (old == LED_GREY) ? LED_GREEN: LED_GREY;
        flashing = (new == LED_GREY) ? 2 : 1;

    } else if (banner->state[GVO_BANNER_VID1] == GVO_LED_VID_OUT_SD_MODE) {
        new = (old == LED_GREY) ? LED_YELLOW: LED_GREY;
        flashing = (new == LED_GREY) ? 2 : 1;

    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_VID1, new);
    }

    /* Vid 2 out */

    old = banner->img[GVO_BANNER_VID2];
    
    if (banner->state[GVO_BANNER_VID2] == GVO_LED_VID_OUT_HD_MODE) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN: LED_GREY;
        } else {
            new = (old == LED_GREY) ? LED_GREEN: LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_VID2] == GVO_LED_VID_OUT_SD_MODE) {
        if (flashing) {
            new = (flashing == 1) ? LED_YELLOW: LED_GREY;
        } else {
            new = (old == LED_GREY) ? LED_YELLOW: LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_VID2, new);
    }
    
    /* SDI sync */
    
    old = banner->img[GVO_BANNER_SDI];
    
    if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_HD) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_SDI] == LED_GREY) ?
                LED_GREEN : LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_SD) {
        if (flashing) {
            new = (flashing == 1) ? LED_YELLOW : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_SDI] == LED_GREY) ?
                LED_YELLOW : LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_ERROR) {
        new = LED_YELLOW;
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_SDI, new);
    }

    /* COMP sync */
    
    old = banner->img[GVO_BANNER_COMP];
    
    if (banner->state[GVO_BANNER_COMP] == GVO_LED_COMP_SYNC_SYNC) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_COMP] == LED_GREY) ?
                LED_GREEN : LED_GREY;
        }
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_COMP, new);
    }

    return TRUE;

} /* update_gvo_banner_led_images() */



/*
 * update_gvo_banner_led_images_shared_sync_bnc() - called by a timer to
 * update the LED images based on current state for GVO devices that have
 * a shared input sync signal BNC connector.
 */

static gboolean update_gvo_banner_led_images_shared_sync_bnc(gpointer data)
{
    guint8 old, new;
    CtkGvoBanner *banner = (CtkGvoBanner *) data;

    /* Flash is used to make all the LEDs flash at the same time. */

    banner->flash = !banner->flash;


    /* Vid 1 out */

    old = banner->img[GVO_BANNER_VID1];
    
    if (banner->state[GVO_BANNER_VID1] != GVO_LED_VID_OUT_NOT_IN_USE) {
        new = banner->flash ? LED_GREEN : LED_GREY;
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_VID1, new);
    }

    /* Vid 2 out */

    old = banner->img[GVO_BANNER_VID2];
    
    if (banner->state[GVO_BANNER_VID2] != GVO_LED_VID_OUT_NOT_IN_USE) {
        new = banner->flash ? LED_GREEN : LED_GREY;
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led_image(banner, GVO_BANNER_VID2, new);
    }
    
    /* Sync */

    /* For this GVO device both the SDI and Composite sync signals
     * share the same LED.  This LED doesn't care about the lock
     * status of the input signal/output video.
     */

    old = banner->img[GVO_BANNER_SDI];

    if ((banner->sync_mode != NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) &&
        (((banner->sync_source == NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE) &&
          banner->state[GVO_BANNER_COMP] != GVO_LED_COMP_SYNC_NONE) ||
         ((banner->sync_source == NV_CTRL_GVO_SYNC_SOURCE_SDI) &&
          banner->state[GVO_BANNER_SDI] != GVO_LED_SDI_SYNC_NONE))) {

        if (banner->input_video_format != NV_CTRL_GVIO_VIDEO_FORMAT_NONE) {
            /* LED blinks if video format is detected */
            new = banner->flash ? LED_GREEN : LED_GREY;
        } else {
            /* LED is solid green if the input video format is not detected. */
            new = LED_GREEN;
        }

    } else {
        new = LED_GREY;
    }

    if (old != new) {
        update_led_image(banner, GVO_BANNER_SDI, new);
    }

    return TRUE;

} /* update_gvo_banner_led_images_shared_sync_bnc() */



/*
 * ctk_gvo_banner_update_video_output() - update banner state of the
 * GVO video output LEDs accordingly, based on the current
 * output_video_format and output_data_format.
 */

static void update_video_output_state(CtkGvoBanner *banner,
                                      gint output_video_format,
                                      gint output_data_format)
{
    if (output_video_format == NV_CTRL_GVIO_VIDEO_FORMAT_NONE) {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_NOT_IN_USE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    } else if ((output_video_format ==
                NV_CTRL_GVIO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC) ||
               (output_video_format ==
                NV_CTRL_GVIO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL)) {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_SD_MODE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_SD_MODE;
    } else {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_HD_MODE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_HD_MODE;
    }
    
    if (output_data_format == NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422) {
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    }

} /* update_video_output_state() */



/*
 * ctk_gvo_banner_update_video_input() - update banner state of the
 * video input GVO banner LEDs accordingly, based on the current sdi
 * and composite input.
 */

static void update_video_input_state(CtkGvoBanner *banner,
                                     gint sdi, gint comp)
{
    if (sdi == NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_HD) {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_HD;
    } else if (sdi == NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_SD) {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_SD;
    } else {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_NONE;
    }
    
    banner->state[GVO_BANNER_COMP] = comp ?
        GVO_LED_COMP_SYNC_SYNC : GVO_LED_COMP_SYNC_NONE;
    
} /* update_video_input_state() */



/*
 * update_gvo_banner_led_state() - Modifies the LED state based on the
 * current GVO state.
 */

static void update_gvo_banner_led_state(CtkGvoBanner *ctk_gvo_banner)
{
    /* Update input state */

    update_video_input_state(ctk_gvo_banner,
                             ctk_gvo_banner->sdi_sync_input_detected,
                             ctk_gvo_banner->composite_sync_input_detected);

    /* Update output state */

    if (ctk_gvo_banner->gvo_lock_owner !=
        NV_CTRL_GVO_LOCK_OWNER_NONE) {
        update_video_output_state(ctk_gvo_banner,
                                  ctk_gvo_banner->output_video_format,
                                  ctk_gvo_banner->output_data_format);
    } else {
        update_video_output_state(ctk_gvo_banner,
                                  NV_CTRL_GVIO_VIDEO_FORMAT_NONE,
                                  ctk_gvo_banner->output_data_format);
    }

} /* update_gvo_banner_led_state() */



/*
 * ctk_gvo_banner_probe() - query the incoming signal and state of
 * the GVO board.
 */

gint ctk_gvo_banner_probe(gpointer data)
{
    ReturnStatus ret;
    gint val;
    CtkGvoBanner *ctk_gvo_banner = CTK_GVO_BANNER(data);
    CtrlTarget *ctrl_target = ctk_gvo_banner->ctrl_target;


    // XXX We could get notified of these (sync source/mode) and
    //     not have to probe - i.e., it could be the job of the
    //     caller/user of the ctk_gvo_banner widget to notify the
    //     banner when these change.  We don't however since doing
    //     that could be prone to bitrot.

    /* query NV_CTRL_GVO_SYNC_MODE */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_SYNC_MODE, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING;
    }

    ctk_gvo_banner->sync_mode = val;


    /* query NV_CTRL_GVO_SYNC_SOURCE */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GVO_SYNC_SOURCE, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE;
    }

    ctk_gvo_banner->sync_source = val;


    /* query NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVIO_VIDEO_FORMAT_NONE;
    }

    ctk_gvo_banner->input_video_format = val;


    /* query COMPOSITE_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_FALSE;
    }

    ctk_gvo_banner->composite_sync_input_detected = val;


    /* query SDI_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE;
    }

    ctk_gvo_banner->sdi_sync_input_detected = val;


    /* query SYNC_LOCK_STATUS */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GVO_SYNC_LOCK_STATUS, &val);

    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_LOCK_STATUS_UNLOCKED;
    }

    ctk_gvo_banner->sync_lock_status = val;


    /* Update the banner state */

    update_gvo_banner_led_state(ctk_gvo_banner);

    
    /* Update the banner's parent */
    
    if (ctk_gvo_banner->probe_callback) {
        ctk_gvo_banner->probe_callback(ctk_gvo_banner->probe_callback_data);
    }
    
    return TRUE;
    
} /* ctk_gvo_banner_probe() */



/*
 * gvo_event_received() - Handles updating the state of the GVO banner
 * for event-driven NV-CONTROL attributes.
 */

static void gvo_event_received(GObject *object,
                               CtrlEvent *event,
                               gpointer user_data)
{
    CtkGvoBanner *ctk_gvo_banner = CTK_GVO_BANNER(user_data);
    gint value;

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }
   
    value = event->int_attr.value;

    switch (event->int_attr.attribute) {
    case NV_CTRL_GVO_SYNC_MODE:
        ctk_gvo_banner->sync_mode = value;
        break;

    case NV_CTRL_GVO_SYNC_SOURCE:
        ctk_gvo_banner->sync_source = value;
        break;

    case NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT:
        ctk_gvo_banner->output_video_format = value;
        break;
        
    case NV_CTRL_GVO_DATA_FORMAT:
        ctk_gvo_banner->output_data_format = value;
        break;
        
    case NV_CTRL_GVO_LOCK_OWNER:
        ctk_gvo_banner->gvo_lock_owner = value;
        break;

    default:
        return;
    }

    update_gvo_banner_led_state(ctk_gvo_banner);

} /* gvo_event_recieved() */



/* ctk_gvo_banner_set_parent() - Sets which parent page owns 
 * (is currently displaying) the gvo banner widget.
 */

void ctk_gvo_banner_set_parent(CtkGvoBanner *ctk_gvo_banner,
                               GtkWidget *new_parent_box,
                               ctk_gvo_banner_probe_callback probe_callback,
                               gpointer probe_callback_data)
{
    /* Repack the banner into the new parent */

    if (ctk_gvo_banner->parent_box != new_parent_box) {
        
        if (ctk_gvo_banner->parent_box) {
            gtk_container_remove(GTK_CONTAINER(ctk_gvo_banner->parent_box),
                                 GTK_WIDGET(ctk_gvo_banner));
        }

        if (new_parent_box) {
            gtk_container_add(GTK_CONTAINER(new_parent_box),
                              GTK_WIDGET(ctk_gvo_banner));
        }
    }

    /* Start/stop the GVO probe */

    if (!ctk_gvo_banner->parent_box && new_parent_box) {

        ctk_config_start_timer(ctk_gvo_banner->ctk_config,
                               (GSourceFunc) ctk_gvo_banner_probe,
                               (gpointer) ctk_gvo_banner);

    } else if (ctk_gvo_banner->parent_box && !new_parent_box) {

        ctk_config_stop_timer(ctk_gvo_banner->ctk_config,
                              (GSourceFunc) ctk_gvo_banner_probe,
                              (gpointer) ctk_gvo_banner);
    }

    /* Keep track of the current banner owner */

    ctk_gvo_banner->parent_box = new_parent_box;

    ctk_gvo_banner->probe_callback = probe_callback;
    ctk_gvo_banner->probe_callback_data = probe_callback_data;

    /* If we are programming a callback, do an initial probe */

    if (probe_callback) {
        ctk_gvo_banner_probe((gpointer)(ctk_gvo_banner));
    }

} /* ctk_gvo_banner_set_parent() */
