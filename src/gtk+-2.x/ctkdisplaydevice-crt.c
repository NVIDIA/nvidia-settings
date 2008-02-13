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

#include "crt_banner.h"

#include "ctkdisplaydevice-crt.h"

#include "ctkimagesliders.h"
#include "ctkconfig.h"
#include "ctkhelp.h"


static void reset_button_clicked(GtkButton *button, gpointer user_data);


GType ctk_display_device_crt_get_type(void)
{
    static GType ctk_display_device_crt_type = 0;
    
    if (!ctk_display_device_crt_type) {
        static const GTypeInfo ctk_display_device_crt_info = {
            sizeof (CtkDisplayDeviceCrtClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDisplayDeviceCrt),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_display_device_crt_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkDisplayDeviceCrt", &ctk_display_device_crt_info, 0);
    }

    return ctk_display_device_crt_type;
}


GtkWidget* ctk_display_device_crt_new(NvCtrlAttributeHandle *handle,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      unsigned int display_device_mask,
                                      char *name)
{
    GObject *object;
    CtkDisplayDeviceCrt *ctk_display_device_crt;
    GtkWidget *image;
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *alignment;
    
    guint8 *image_buffer = NULL;
    const nv_image_t *img;
    char *s;
    
    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE_CRT, NULL);

    ctk_display_device_crt = CTK_DISPLAY_DEVICE_CRT(object);
    ctk_display_device_crt->handle = handle;
    ctk_display_device_crt->ctk_config = ctk_config;
    ctk_display_device_crt->display_device_mask = display_device_mask;
    ctk_display_device_crt->name = name;
    
    gtk_box_set_spacing(GTK_BOX(object), 10);
    
    /* banner */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    
    img = &crt_banner_image;
    
    image_buffer = decompress_image_data(img);
    
    image = gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));

    gtk_container_add(GTK_CONTAINER(frame), image);


    /*
     * create the reset button (which we need while creating the
     * controls in this page so that we can set the button's
     * sensitivity), though we pack it at the bottom of the page
     */

    label = gtk_label_new("Reset Hardware Defaults");
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_display_device_crt->reset_button = gtk_button_new();
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_display_device_crt->reset_button),
                      hbox);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_display_device_crt->reset_button);
    gtk_box_pack_end(GTK_BOX(object), alignment, TRUE, TRUE, 0);
    
    g_signal_connect(G_OBJECT(ctk_display_device_crt->reset_button),
                     "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_display_device_crt);

    ctk_config_set_tooltip(ctk_config, ctk_display_device_crt->reset_button,
                           "The Reset Hardware Defaults button restores "
                           "the CRT settings to their default values.");
    
    /* pack the image sliders */
    
    ctk_display_device_crt->image_sliders =
        ctk_image_sliders_new(handle, ctk_config, ctk_event,
                              ctk_display_device_crt->reset_button,
                              display_device_mask, name);
    if (ctk_display_device_crt->image_sliders) {
        gtk_box_pack_start(GTK_BOX(object),
                           ctk_display_device_crt->image_sliders,
                           FALSE, FALSE, 0);
    } else {
        s = g_strconcat("There are no configurable options available for ",
                        ctk_display_device_crt->name, ".", NULL);
        
        label = gtk_label_new(s);
        
        g_free(s);

        gtk_box_pack_start(GTK_BOX(object), label, FALSE, FALSE, 0);
    }

    /* show the page */

    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
}



GtkTextBuffer *ctk_display_device_crt_create_help(GtkTextTagTable *table,
                                                  CtkDisplayDeviceCrt
                                                  *ctk_display_device_crt)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    gboolean ret;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "%s Help", ctk_display_device_crt->name);
    
    if (ctk_display_device_crt->image_sliders) {
        ret = add_image_sharpening_help
            (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders), b, &i);
    } else {
        ret = FALSE;
    }
    
    if (!ret) {
        ctk_help_para(b, &i, "There are no configurable options available "
                      "for %s.", ctk_display_device_crt->name);
    }
    
    ctk_help_finish(b);

    return b;
}



/*
 * reset_button_clicked() -
 */

static void reset_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkDisplayDeviceCrt *ctk_display_device_crt =
        CTK_DISPLAY_DEVICE_CRT(user_data);
    
    if (ctk_display_device_crt->image_sliders) {
        ctk_image_sliders_reset
            (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders));
    }
    
    ctk_config_statusbar_message(ctk_display_device_crt->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_display_device_crt->name);

} /* reset_button_clicked() */
