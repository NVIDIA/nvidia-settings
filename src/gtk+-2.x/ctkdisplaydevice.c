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

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkimage.h"

#include <gdk-pixbuf/gdk-pixdata.h>

#include "tv_pixdata.h"
#include "dfp_pixdata.h"
#include "crt_pixdata.h"

#include "ctkdisplaydevice.h"

#include "ctkconfig.h"
#include "ctkhelp.h"


GType ctk_display_device_get_type(void)
{
    static GType ctk_display_device_type = 0;
    
    if (!ctk_display_device_type) {
        static const GTypeInfo ctk_display_device_info = {
            sizeof (CtkDisplayDeviceClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDisplayDevice),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_display_device_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkDisplayDevice", &ctk_display_device_info, 0);
    }

    return ctk_display_device_type;
}


GtkWidget* ctk_display_device_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config, CtkEvent *ctk_event)
{
    GObject *object;
    CtkDisplayDevice *ctk_display_device;
    GtkWidget *image;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *alignment;
    ReturnStatus ret;
    
    const GdkPixdata *img;
    int enabled, i, mask, n;
    char *name;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE, NULL);

    ctk_display_device = CTK_DISPLAY_DEVICE(object);
    ctk_display_device->handle = handle;
    ctk_display_device->ctk_config = ctk_config;
    ctk_display_device->num_display_devices = 0;

    gtk_box_set_spacing(GTK_BOX(object), 10);
    
    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_DISPLAY_CONFIG);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /*
     * In the future: this page will be where things like TwinView
     * will be configured.  In the meantime, just put place holders
     * for each display device present on this X screen.
     */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ENABLED_DISPLAYS, &enabled);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }
    
    ctk_display_device->enabled_display_devices = enabled;
    
    /* create an alignment to center the placeholder */
    
    alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, FALSE, 0);
    
    /* create a frame to hold the whole placeholder */
    
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(alignment), frame);
    
    /* create an hbox to hold each display device */
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    /* create a vbox with image and label for each display device */

    for (n = 0, i = 0; i < 24; i++) {

        mask = 1 << i;
        if (!(enabled & mask)) continue;
        
        /* get the name of the display device */
        
        ret =
            NvCtrlGetStringDisplayAttribute(handle, mask,
                                            NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                            &name);
        
        if ((ret != NvCtrlSuccess) || (!name)) {
            name = g_strdup("Unknown");
        }
        
        /* get the correct image for each display device type */
        
        if (mask & CTK_DISPLAY_DEVICE_CRT_MASK) {
            img = &crt_pixdata;
        } else if (mask & CTK_DISPLAY_DEVICE_TV_MASK) {
            img = &tv_pixdata;
        } else if (mask & CTK_DISPLAY_DEVICE_DFP_MASK) {
            img = &dfp_pixdata;
        } else {
            continue;
        }
        
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);
        
        frame = gtk_frame_new(NULL);
        gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);
        
        image = gtk_image_new_from_pixbuf(gdk_pixbuf_from_pixdata(img,
                                                                  TRUE, NULL));
                                  
        gtk_container_add(GTK_CONTAINER(frame), image);
        
        label = gtk_label_new(name);
        
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

        /* save the display device name */

        ctk_display_device->display_device_names[n] = name;
        
        /* increment the display device count */
        
        n++;
    }
    
    ctk_display_device->num_display_devices = n;

    /* show the page */

    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);

} /* ctk_display_device_new() */



/*
 * ctk_display_device_create_help() - construct help page
 */

GtkTextBuffer *ctk_display_device_create_help(GtkTextTagTable *table,
                                              CtkDisplayDevice
                                              *ctk_display_device)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    char *title, *page, *s, *tmp, *name;
    int n, num;

    num = ctk_display_device->num_display_devices;

    if (num == 1) {
        title = "Display Device Help";
        page = "page";
    } else {
        title = "Display Devices Help";
        page = "pages";
    }

    /* ugliness to build list of display device names */

    s = NULL;
    for (n = 0; n < num; n++) {
        
        name = ctk_display_device->display_device_names[n];

        if (s) {
            tmp = s;
            s = g_strconcat(s, " ", NULL);
            g_free(tmp);
        } else {
            s = g_strdup(" ");
        }
        
        tmp = s;

        if (n == (num - 1)) {
            s = g_strconcat(s, name, NULL);
        } else if (n == (num - 2)) {
            s = g_strconcat(s, name, " and", NULL);
        } else {
            s = g_strconcat(s, name, ",", NULL);
        }
        
        g_free(tmp);
    }


    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, title);
    
    ctk_help_para(b, &i, "The %s page is a place holder until support "
                  "for configuring TwinView is added.", title);
    
    ctk_help_para(b, &i, "Please see the %s for%s for per-display device "
                  "configuration.", page, s);
    
    ctk_help_finish(b);

    return b;
    
} /* ctk_display_device_create_help() */
