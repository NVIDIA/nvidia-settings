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

#include <stdlib.h> /* malloc */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixdata.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkrandr.h"
#include "ctkbanner.h"

#include "rotation_orientation_horz_pixdata.h"
#include "rotation_orientation_vert_pixdata.h"

#include "rotate_left_on_pixdata.h"
#include "rotate_left_off_pixdata.h"
#include "rotate_right_on_pixdata.h"
#include "rotate_right_off_pixdata.h"


GType ctk_randr_get_type(void)
{
    static GType ctk_randr_type = 0;

    if (!ctk_randr_type) {
        static const GTypeInfo ctk_randr_info = {
            sizeof (CtkRandRClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkRandR),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_randr_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkRandR", &ctk_randr_info, 0);
    }

    return ctk_randr_type;
}



/*
 * Human readable rotation settings
 */
static char *get_rotation_string(int rotation)
{
    switch ( rotation ) {
    case RR_Rotate_0:
        return "Normal (0 degree rotation)";
        break;
    case RR_Rotate_90:
        return "Rotated left (90 degree rotation)";
        break;
    case RR_Rotate_180:
        return "Inverted (180 degree rotation)";
        break;
    case RR_Rotate_270:
        return "Rotated right (270 degree rotation)";
        break;
    default:
        return "Unknown rotation";
        break;
    }
}



/*
 * Helper function to flip the contents of a pixbuf
 * horizontally.  This is used to perform a 180 degree
 * rotation + vertical flip.
 */

static void horz_flip_pixbuf(GdkPixbuf *pixbuf)
{
    guchar *pixels;
    int width;
    int height;
    int rowstride;       // # bytes per row
    int bits_per_sample; // # bits per color sample
    int n_channels;      // # color samples per component

    guchar *row;
    guchar *head;
    guchar *tail;
    int x, y, c;
    int component_size;

    /* Get pixbuf information */
    pixels = gdk_pixbuf_get_pixels(pixbuf);
    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);
    rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    bits_per_sample = gdk_pixbuf_get_bits_per_sample(pixbuf);
    n_channels = gdk_pixbuf_get_n_channels(pixbuf);

    /* Compute the number of bytes per component (RGB/RGBA) */
    component_size = n_channels * (bits_per_sample / 8);

    /* Swap all components in all rows */
    for (y = 0; y < height; y++) {

        row = pixels + (y * rowstride);
        head = row;
        tail = row + (width - 1) * (component_size);

        for (x = 0; x < width/2; x++) {

            for (c = 0; c < component_size; c++) {
                guchar h = head[c];
                head[c] = tail[c];
                tail[c] = h;
            }
            head += component_size;
            tail -= component_size;
        }
    }
 
} /* horz_flip_pixbuf() */



/*
 * Helper function to load the correctly oriented pixbuf
 * orientation image from the nv_image dump repository
 */
static GdkPixbuf *load_orientation_image_pixbuf(Rotation rotation)
{
    const GdkPixdata *pixdata;
    GdkPixbuf *pixbuf;
    gboolean tweak_img; /* Rotate and flip image */


    /* Figure out which image and rotation to use */
    switch ( rotation ) {
    case RR_Rotate_0: /* Normal */
        pixdata = &rotation_orientation_horz_pixdata;
        tweak_img = FALSE;
        break;
    case RR_Rotate_90: /* Left */
        pixdata = &rotation_orientation_vert_pixdata;
        tweak_img = TRUE;
        break;
    case RR_Rotate_180: /* Inverted */
        pixdata = &rotation_orientation_horz_pixdata;
        tweak_img = TRUE;
        break;
    case RR_Rotate_270: /* Right */
        pixdata = &rotation_orientation_vert_pixdata;
        tweak_img = FALSE;
        break;
    default: /* Unknown */
        pixdata = &rotation_orientation_horz_pixdata;
        tweak_img = FALSE;
        break;
    }


    /* Generate the pixbuf from the pixdata */
    pixbuf = gdk_pixbuf_from_pixdata(pixdata, TRUE, NULL);
    if (!pixbuf) {
        return NULL;
    }


    /* Image requires 180 degree rotation + vertical flip,
     * this is the same as a horizontal flip.
     */
    if (tweak_img) {
        horz_flip_pixbuf(pixbuf);
    }

    return pixbuf;
 
} /* load_orientation_image_pixbuf() */



/*
 * Makes widgets associated with rotation reflect the given
 * rotation setting.
 */
static void update_rotation(CtkRandR *ctk_randr, Rotation rotation)
{
    /* Update screen image */
    gtk_image_set_from_pixbuf(ctk_randr->orientation_image,
                              ctk_randr->orientation_image_pixbufs[rotation]);

    /* Update label */
    gtk_label_set_text(ctk_randr->label,
                       get_rotation_string(rotation));


    /* Update the status bar */
    ctk_config_statusbar_message(ctk_randr->ctk_config,
                                 "Screen rotation set to %s.",
                                 get_rotation_string(rotation));

} /* update_rotation() */



/*
 * When XRandR events happens outside of the control panel,
 * they are trapped by this function so the gui can be updated
 * with the new rotation setting.
 */
void ctk_randr_event_handler(GtkWidget *widget,
                             XRRScreenChangeNotifyEvent *ev,
                             gpointer data)
{
    update_rotation((CtkRandR *)data, ev->rotation);
} /* ctk_randr_event_handler() */



/*
 * Rotate left button event handlers
 */
static void do_button_rotate_left_clicked(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;
    int orig_rotation;
    int rotation;
    int rotations;
    ReturnStatus status;


    /* Get current rotation */
    NvCtrlGetAttribute(ctk_randr->handle, NV_CTRL_ATTR_XRANDR_ROTATION,
                       &orig_rotation);

    /* Get available rotations */
    NvCtrlGetAttribute(ctk_randr->handle, NV_CTRL_ATTR_XRANDR_ROTATIONS,
                       &rotations);

    /* Find next available rotation to the left */
    rotation = orig_rotation;
    do {
        rotation <<= 1;
        if ( rotation > 8 ) {
            rotation = 1;
        }
    } while ( !(rotation & rotations) && (rotation != orig_rotation) );

    /* Set rotation */
    status = NvCtrlSetAttribute(ctk_randr->handle,
                                NV_CTRL_ATTR_XRANDR_ROTATION,
                                rotation);

    /* Update widgets */
    update_rotation(ctk_randr, rotation);
}

static void do_button_rotate_left_press(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    ctk_randr->rotate_left_button_pressed = True;
    gtk_image_set_from_pixbuf(ctk_randr->rotate_left_button_image,
                              ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_ON]);
}

static void do_button_rotate_left_release(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    ctk_randr->rotate_left_button_pressed = False;
    gtk_image_set_from_pixbuf(ctk_randr->rotate_left_button_image,
                              ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_OFF]);
}

static void do_button_rotate_left_enter(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    if ( ctk_randr->rotate_left_button_pressed ) {
        gtk_image_set_from_pixbuf(ctk_randr->rotate_left_button_image,
                                  ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_ON]);
    }
}

static void do_button_rotate_left_leave(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    if ( ctk_randr->rotate_left_button_pressed ) {
        gtk_image_set_from_pixbuf(ctk_randr->rotate_left_button_image,
                                  ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_OFF]);
    }
}



/*
 * Rotate right button event handlers
 */
static void do_button_rotate_right_clicked(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;
    int orig_rotation;
    int rotation;
    int rotations;
    ReturnStatus status;


    /* Get current rotation */
    NvCtrlGetAttribute(ctk_randr->handle, NV_CTRL_ATTR_XRANDR_ROTATION,
                       &orig_rotation);

    /* Get available rotations */
    NvCtrlGetAttribute(ctk_randr->handle, NV_CTRL_ATTR_XRANDR_ROTATIONS,
                       &rotations);

    /* Find next available rotation to the left */
    rotation = orig_rotation;
    do {
        rotation >>= 1;
        if ( rotation == 0 ) {
            rotation = 8;
        }
    } while ( !(rotation & rotations) && (rotation != orig_rotation) );


    /* Set rotation */
    status = NvCtrlSetAttribute(ctk_randr->handle,
                                NV_CTRL_ATTR_XRANDR_ROTATION,
                                rotation);

    /* Update widgets */
    update_rotation(ctk_randr, rotation);
}

static void do_button_rotate_right_press(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    ctk_randr->rotate_right_button_pressed = True;
    gtk_image_set_from_pixbuf(ctk_randr->rotate_right_button_image,
                              ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_ON]);
}


static void do_button_rotate_right_release(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    ctk_randr->rotate_right_button_pressed = False;
    gtk_image_set_from_pixbuf(ctk_randr->rotate_right_button_image,
                              ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_OFF]);
}

static void do_button_rotate_right_enter(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    if ( ctk_randr->rotate_right_button_pressed ) {
        gtk_image_set_from_pixbuf(ctk_randr->rotate_right_button_image,
                                  ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_ON]);
    }
}

static void do_button_rotate_right_leave(GtkWidget *widget, gpointer data)
{
    CtkRandR *ctk_randr = (CtkRandR *)data;

    if ( ctk_randr->rotate_right_button_pressed ) {
        gtk_image_set_from_pixbuf(ctk_randr->rotate_right_button_image,
                                  ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_OFF]);
    }
}



/*
 * CTK RandR widget creation
 *
 */
GtkWidget* ctk_randr_new(NvCtrlAttributeHandle *handle,
                         CtkConfig *ctk_config,
                         CtkEvent *ctk_event)
{
    GObject *object;
    CtkRandR *ctk_randr;

    Bool ret;  /* NvCtrlxxx function return value */
    int  rotation_supported;
    int  rotation;



    /* Make sure we have a handle */
    g_return_val_if_fail(handle != NULL, NULL);
    

    /* Check if this screen supports rotation */    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_XRANDR_ROTATION_SUPPORTED,
                             &rotation_supported);
    if ((ret != NvCtrlSuccess) || (!rotation_supported)) {
        /* Rotation not available */
        return NULL;
    }


    /* Get the initial state of rotation */
    NvCtrlGetAttribute(handle, NV_CTRL_ATTR_XRANDR_ROTATION, &rotation);


    /* Create the ctk object */
    object = g_object_new(CTK_TYPE_RANDR, NULL);
    ctk_randr = CTK_RANDR(object);


    /* Cache the attribute handle */
    ctk_randr->handle = handle;


    /* Set container properties of the object */
    ctk_randr->ctk_config = ctk_config;
    gtk_box_set_spacing(GTK_BOX(ctk_randr), 10);


    /* Preload orientated screens pixbufs & image */
    ctk_randr->orientation_image_pixbufs[CTKRANDR_IMG_ROTATION_NORMAL] =
        load_orientation_image_pixbuf(RR_Rotate_0);

    ctk_randr->orientation_image_pixbufs[CTKRANDR_IMG_ROTATION_LEFT] =
        load_orientation_image_pixbuf(RR_Rotate_90);

    ctk_randr->orientation_image_pixbufs[CTKRANDR_IMG_ROTATION_INVERTED] =
        load_orientation_image_pixbuf(RR_Rotate_180);

    ctk_randr->orientation_image_pixbufs[CTKRANDR_IMG_ROTATION_RIGHT] =
        load_orientation_image_pixbuf(RR_Rotate_270);
    
    ctk_randr->orientation_image =
        GTK_IMAGE(gtk_image_new_from_pixbuf(ctk_randr->orientation_image_pixbufs[rotation]) );


    /* Preload button pixbufs & images */
    ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_OFF] =
        gdk_pixbuf_from_pixdata(&rotate_left_off_pixdata,
                                TRUE, NULL);

    ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_ON] =
        gdk_pixbuf_from_pixdata(&rotate_left_on_pixdata,
                                TRUE, NULL);

    ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_OFF] =
        gdk_pixbuf_from_pixdata(&rotate_right_off_pixdata,
                                TRUE, NULL);

    ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_ON] =
        gdk_pixbuf_from_pixdata(&rotate_right_on_pixdata,
                                TRUE, NULL);

    ctk_randr->rotate_left_button_image =
        GTK_IMAGE(gtk_image_new_from_pixbuf(ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_LEFT_OFF]) );

    ctk_randr->rotate_right_button_image =
        GTK_IMAGE(gtk_image_new_from_pixbuf(ctk_randr->button_pixbufs[CTKRANDR_BTN_ROTATE_RIGHT_OFF]) );



    { /* Banner image */
        GtkWidget *banner = ctk_banner_image_new(BANNER_ARTWORK_ROTATION);
        gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);
    }


    { /* Rotation control pane */
        GtkWidget *vRotationPane       = gtk_vbox_new(FALSE, 0);
        GtkWidget *hStrechedControlBox = gtk_hbox_new(TRUE, 0);
        GtkWidget *hControlBox         = gtk_hbox_new(FALSE, 10);

        gtk_box_pack_start(GTK_BOX(object), vRotationPane, TRUE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(vRotationPane), hStrechedControlBox,
                           FALSE, FALSE,
                           0);
        
        gtk_box_pack_start(GTK_BOX(hStrechedControlBox), hControlBox,
                           FALSE, FALSE, 0);


        { /* Rotate left button */
            GtkWidget *vbox    = gtk_vbox_new(FALSE, 0);
            GtkWidget *btn_box = gtk_hbox_new(FALSE, 0);
            GtkWidget *button  = gtk_button_new();

            gtk_box_pack_start(GTK_BOX(hControlBox), vbox, TRUE, FALSE, 0);

            gtk_widget_set_size_request(button, 26, 26);
            ctk_config_set_tooltip(ctk_config, button, "Rotate left");                        
            g_signal_connect(G_OBJECT(button), "pressed",
                             G_CALLBACK(do_button_rotate_left_press),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "released",
                             G_CALLBACK(do_button_rotate_left_release),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "enter",
                             G_CALLBACK(do_button_rotate_left_enter),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "leave",
                             G_CALLBACK(do_button_rotate_left_leave),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "clicked",
                             G_CALLBACK(do_button_rotate_left_clicked),
                             (gpointer) (ctk_randr));
            
            gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 0);

            gtk_container_add(GTK_CONTAINER(button), btn_box);

            gtk_container_add(GTK_CONTAINER(btn_box),
                              GTK_WIDGET(ctk_randr->rotate_left_button_image));
        }

        { /* Rotation orientation image */
            GtkWidget *frame = gtk_frame_new(NULL);
            GtkWidget *img_box = gtk_hbox_new(TRUE, 0);
            GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
            GtkWidget *vbox = gtk_vbox_new(TRUE, 0);

            gtk_box_pack_start(GTK_BOX(img_box),
                               GTK_WIDGET(ctk_randr->orientation_image),
                               FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(frame), img_box);
            gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
            gtk_widget_set_size_request(hbox, 130, 130);
            gtk_box_pack_start(GTK_BOX(hControlBox), hbox, FALSE, FALSE, 0);
        }

        { /* Rotate right button */
            GtkWidget *vbox    = gtk_vbox_new(FALSE, 0);
            GtkWidget *btn_box = gtk_hbox_new(FALSE, 0);
            GtkWidget *button  = gtk_button_new();

            gtk_box_pack_start(GTK_BOX(hControlBox), vbox, TRUE, FALSE, 0);

            gtk_widget_set_size_request(button, 26, 26);
            ctk_config_set_tooltip(ctk_config, button, "Rotate right");
            g_signal_connect(G_OBJECT(button), "pressed",
                             G_CALLBACK(do_button_rotate_right_press),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "released",
                             G_CALLBACK(do_button_rotate_right_release),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "enter",
                             G_CALLBACK(do_button_rotate_right_enter),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "leave",
                             G_CALLBACK(do_button_rotate_right_leave),
                             (gpointer) ctk_randr);
            g_signal_connect(G_OBJECT(button), "clicked",
                             G_CALLBACK(do_button_rotate_right_clicked),
                             (gpointer) (ctk_randr));
            
            gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, FALSE, 0);

            gtk_container_add(GTK_CONTAINER(button), btn_box);

            gtk_container_add(GTK_CONTAINER(btn_box),
                              GTK_WIDGET(ctk_randr->rotate_right_button_image));
        }
    
        { /* Rotation label */
            ctk_randr->label =
                GTK_LABEL( gtk_label_new(get_rotation_string(rotation)));
            gtk_box_pack_start(GTK_BOX(vRotationPane),
                               GTK_WIDGET(ctk_randr->label),
                               TRUE, TRUE, 10);
        }

    } /* Rotation control pane */


 
    /* Show the widget */
    gtk_widget_show_all(GTK_WIDGET(ctk_randr));


    /* Link widget to XRandR events */
    g_signal_connect(ctk_event, "CTK_EVENT_RRScreenChangeNotify",
                     G_CALLBACK(ctk_randr_event_handler),
                     (gpointer) ctk_randr);


    return GTK_WIDGET(ctk_randr);
}



/*
 * RandR help screen
 */
GtkTextBuffer *ctk_randr_create_help(GtkTextTagTable *table,
                                     CtkRandR *ctk_randr)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Rotation Help");
    ctk_help_para(b, &i,
                  "This page in the NVIDIA X Server Control Panel allows "
                  "you to select the desired screen orientation through "
                  "the XRandR extension."
                  );

    ctk_help_finish(b);

    return b;
}
