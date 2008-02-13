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

#include "ctkdisplaydevice-crt.h"

#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkconfig.h"
#include "ctkhelp.h"


static void ctk_display_device_crt_class_init(CtkDisplayDeviceCrtClass *);
static void ctk_display_device_crt_finalize(GObject *);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void ctk_display_device_crt_setup(CtkDisplayDeviceCrt
                                         *ctk_display_device_crt);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

GType ctk_display_device_crt_get_type(void)
{
    static GType ctk_display_device_crt_type = 0;
    
    if (!ctk_display_device_crt_type) {
        static const GTypeInfo ctk_display_device_crt_info = {
            sizeof (CtkDisplayDeviceCrtClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_display_device_crt_class_init,
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

static void ctk_display_device_crt_class_init(
    CtkDisplayDeviceCrtClass *ctk_display_device_crt_class
)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_display_device_crt_class;
    gobject_class->finalize = ctk_display_device_crt_finalize;
}

static void ctk_display_device_crt_finalize(
    GObject *object
)
{
    CtkDisplayDeviceCrt *ctk_display_device_crt = CTK_DISPLAY_DEVICE_CRT(object);
    g_free(ctk_display_device_crt->name);
}

/*
 * ctk_display_device_crt_new() - constructor for the CRT display
 * device page.
 */

GtkWidget* ctk_display_device_crt_new(NvCtrlAttributeHandle *handle,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      unsigned int display_device_mask,
                                      char *name)
{
    GObject *object;
    CtkDisplayDeviceCrt *ctk_display_device_crt;
    GtkWidget *banner;
    GtkWidget *hbox;
    GtkWidget *alignment;
        
    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE_CRT, NULL);

    ctk_display_device_crt = CTK_DISPLAY_DEVICE_CRT(object);
    ctk_display_device_crt->handle = handle;
    ctk_display_device_crt->ctk_config = ctk_config;
    ctk_display_device_crt->ctk_event = ctk_event;
    ctk_display_device_crt->display_device_mask = display_device_mask;
    ctk_display_device_crt->name = g_strdup(name);
    
    gtk_box_set_spacing(GTK_BOX(object), 10);
    
    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_CRT);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /*
     * create the reset button (which we need while creating the
     * controls in this page so that we can set the button's
     * sensitivity), though we pack it at the bottom of the page
     */

    ctk_display_device_crt->reset_button =
        gtk_button_new_with_label("Reset Hardware Defaults");

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
    }

    /* pack the EDID button */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);
    ctk_display_device_crt->edid_box = hbox;
    
    /* show the page */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* Update the GUI */

    ctk_display_device_crt_setup(ctk_display_device_crt);
    
    /* handle enable/disable events on the display device */

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_received),
                     (gpointer) ctk_display_device_crt);

    return GTK_WIDGET(object);
}



GtkTextBuffer *ctk_display_device_crt_create_help(GtkTextTagTable *table,
                                                  CtkDisplayDeviceCrt
                                                  *ctk_display_device_crt)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "%s Help", ctk_display_device_crt->name);
    
    add_image_sliders_help
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders), b, &i);

    if (ctk_display_device_crt->edid) {
        add_acquire_edid_help(b, &i);
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
    
    ctk_image_sliders_reset
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders));

    gtk_widget_set_sensitive(ctk_display_device_crt->reset_button, FALSE);
    
    ctk_config_statusbar_message(ctk_display_device_crt->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_display_device_crt->name);

} /* reset_button_clicked() */



/*
 * Updates the display device page to reflect the current
 * configuration of the display device.
 */
static void ctk_display_device_crt_setup(CtkDisplayDeviceCrt
                                         *ctk_display_device_crt)
{
    ReturnStatus ret;
    unsigned int enabled_displays;


    /* Is display enabled? */

    ret = NvCtrlGetAttribute(ctk_display_device_crt->handle,
                             NV_CTRL_ENABLED_DISPLAYS,
                             (int *)&enabled_displays);

    ctk_display_device_crt->display_enabled =
        (ret == NvCtrlSuccess &&
         (enabled_displays & (ctk_display_device_crt->display_device_mask)));


    /* Update the image sliders */

    ctk_image_sliders_setup
        (CTK_IMAGE_SLIDERS(ctk_display_device_crt->image_sliders));


    /* update acquire EDID button */
    
    if (ctk_display_device_crt->edid) {
            GList *list;
            
            list = gtk_container_get_children
                (GTK_CONTAINER(ctk_display_device_crt->edid_box));
            if (list) {
                gtk_container_remove
                    (GTK_CONTAINER(ctk_display_device_crt->edid_box),
                     (GtkWidget *)(list->data));
                g_list_free(list);
            }
    }

    ctk_display_device_crt->edid =
        ctk_edid_new(ctk_display_device_crt->handle,
                     ctk_display_device_crt->ctk_config,
                     ctk_display_device_crt->ctk_event,
                     ctk_display_device_crt->reset_button,
                     ctk_display_device_crt->display_device_mask,
                     ctk_display_device_crt->name);

    if (ctk_display_device_crt->edid) {
        gtk_box_pack_start(GTK_BOX(ctk_display_device_crt->edid_box),
                           ctk_display_device_crt->edid, TRUE, TRUE, 0);
    }


    /* update the reset button */

    gtk_widget_set_sensitive(ctk_display_device_crt->reset_button, FALSE);

} /* ctk_display_device_crt_setup() */



/*
 * When the list of enabled displays on the GPU changes,
 * this page should disable/enable access based on whether
 * or not the display device is enabled.
 */
static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
    CtkDisplayDeviceCrt *ctk_object = CTK_DISPLAY_DEVICE_CRT(user_data);

    ctk_display_device_crt_setup(ctk_object);

} /* enabled_displays_received() */
