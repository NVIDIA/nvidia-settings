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

#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "NvCtrlAttributes.h"

#include "ctkbanner.h"

#include "ctkxvideo.h"
#include "ctkscale.h"

#include "ctkhelp.h"


static const char *__xv_sync_to_display_help =
"This controls which display device will be synched to when "
"XVideo Sync To VBlank is enabled.";

static void xv_sync_to_display_changed(GtkWidget *widget, gpointer user_data);

static GtkWidget *xv_sync_to_display_radio_button_add(CtkXVideo *ctk_xvideo,
                                                      GtkWidget *prev_radio,
                                                      char *label,
                                                      gint value,
                                                      int index);

static void
xv_sync_to_display_update_radio_buttons(CtkXVideo *ctk_xvideo, gint value,
                                        gboolean update_status);

static void xv_sync_to_display_changed(GtkWidget *widget, gpointer user_data);

static void xv_sync_to_display_update_received(GtkObject *object, gpointer arg1,
                                               gpointer user_data);

static void post_xv_sync_to_display_changed(CtkXVideo *ctk_xvideo,
                                            GtkWidget *widget);
static void nv_ctrl_enabled_displays(GtkObject *object, gpointer arg1,
                                     gpointer user_data);
static void xv_sync_to_display_radio_button_remove(CtkXVideo *ctk_xvideo,
                                                   gint value);
static void xv_sync_to_display_radio_button_enabled_add(CtkXVideo *ctk_xvideo,
                                                        gint add_device_mask);

#define FRAME_PADDING 5


#define __XV_SYNC_TO_DISPLAY 1


GType ctk_xvideo_get_type(
    void
)
{
    static GType ctk_xvideo_type = 0;

    if (!ctk_xvideo_type) {
        static const GTypeInfo ctk_xvideo_info = {
            sizeof (CtkXVideoClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkXVideo),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_xvideo_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkXVideo", &ctk_xvideo_info, 0);
    }

    return ctk_xvideo_type;
}

/*
 * xv_sync_to_display_radio_button_add() - create a radio button and plug it
 * into the xv_sync_display radio group.
 */

static GtkWidget *xv_sync_to_display_radio_button_add(CtkXVideo *ctk_xvideo,
                                                       GtkWidget *prev_radio,
                                                       char *label,
                                                       gint value,
                                                       int index)
{
    GtkWidget *radio;
    
    if (prev_radio) {
        radio = gtk_radio_button_new_with_label_from_widget
            (GTK_RADIO_BUTTON(prev_radio), label);
    } else {
        radio = gtk_radio_button_new_with_label(NULL, label);
    }
    
    gtk_box_pack_start(GTK_BOX(ctk_xvideo->xv_sync_to_display_button_box),
                       radio, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(radio), "xv_sync_to_display",
                      GINT_TO_POINTER(value));
   
    g_signal_connect(G_OBJECT(radio), "toggled",
                     G_CALLBACK(xv_sync_to_display_changed),
                     (gpointer) ctk_xvideo);

    ctk_xvideo->xv_sync_to_display_buttons[index] = radio;

    return radio;
    
} /* xv_sync_to_display_radio_button_add() */


static void
xv_sync_to_display_update_radio_buttons(CtkXVideo *ctk_xvideo,
                                        gint value, gboolean update_status)
{
    GtkWidget *b, *button = NULL;
    int i;

    button = ctk_xvideo->xv_sync_to_display_buttons[value];
    
    if (!button) return;

    /* turn off signal handling for all the sync buttons */

    for (i = 0; i < 24; i++) {
        b = ctk_xvideo->xv_sync_to_display_buttons[i];
        if (!b) continue;

        g_signal_handlers_block_by_func
            (G_OBJECT(b), G_CALLBACK(xv_sync_to_display_changed),
             (gpointer) ctk_xvideo);
    }
    
    /* set the appropriate button active */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    
    /* turn on signal handling for all the sync buttons */

    for (i = 0; i < 24; i++) {
        b = ctk_xvideo->xv_sync_to_display_buttons[i];
        if (!b) continue;

        g_signal_handlers_unblock_by_func
            (G_OBJECT(b), G_CALLBACK(xv_sync_to_display_changed),
             (gpointer) ctk_xvideo);
    }

    if (update_status) {
        post_xv_sync_to_display_changed(ctk_xvideo, button);
    }

} /* xv_sync_to_display_update_radio_buttons() */


/*
 * xv_sync_to_display_changed() - callback function for changes to the
 * sync_to_display radio button group; if the specified radio button is
 * active, send xv_sync_to_display state to the server
 */

static void xv_sync_to_display_changed(GtkWidget *widget, gpointer user_data)
{
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    gboolean enabled;
    gint device_mask;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (!enabled) {
        /* Ignore 'disable' events. */
        return;
    }

    user_data = g_object_get_data(G_OBJECT(widget), "xv_sync_to_display");

    device_mask = GPOINTER_TO_INT(user_data);

    NvCtrlSetAttribute(ctk_xvideo->handle,
                       NV_CTRL_XV_SYNC_TO_DISPLAY, device_mask);

    post_xv_sync_to_display_changed(ctk_xvideo, widget);

}/* xv_sync_to_display_changed() */


static void post_xv_sync_to_display_changed(CtkXVideo *ctk_xvideo,
                                            GtkWidget *active_button)
{
    const gchar *label;

    label = gtk_button_get_label(GTK_BUTTON(active_button));

    ctk_config_statusbar_message(ctk_xvideo->ctk_config,
                                     "XVideo application syncing to %s.",
                                     label);
}

/*
 * xv_sync_to_display_radio_button_remove()
 */
static void xv_sync_to_display_radio_button_remove(CtkXVideo *ctk_xvideo,
                                                   gint value)
{
    int j;
    GtkWidget *b;
    gpointer user_data;
    for (j = 0; j < 24; j++) {
        b = ctk_xvideo->xv_sync_to_display_buttons[j];
        if (b != NULL) {
            user_data = g_object_get_data(G_OBJECT(b), "xv_sync_to_display");
            if (GPOINTER_TO_INT(user_data) == value) {
                g_object_set_data(G_OBJECT(b), "xv_sync_to_display",
                                  GINT_TO_POINTER(0));
                gtk_container_remove(GTK_CONTAINER(ctk_xvideo->xv_sync_to_display_button_box), b);
                ctk_xvideo->xv_sync_to_display_buttons[j] = NULL;
                break;
            }
        }
    }
} /* xv_sync_to_display_radio_button_remove() */

/*
 * nv_ctrl_enabled_displays()
 */
static void  nv_ctrl_enabled_displays(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
     CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
     int i, enabled, prev_enabled = 0;
     int remove_devices_mask, add_devices_mask;
     unsigned int mask;
     gpointer udata;
     GtkWidget *b;
     ReturnStatus ret;

     /*
      * The event data passed in gives us the enabled displays mask
      * for all displays on all X Screens on this GPU. Since we can
      * only sync to a display on this X Screen, we need to have the
      * correct enabled displays.
      */
     ret = NvCtrlGetAttribute(ctk_xvideo->handle,
                              NV_CTRL_ENABLED_DISPLAYS,
                              &enabled);
     if (ret != NvCtrlSuccess) {
         enabled = 0;
     }

     /*  Extract the previous value. */
     for ( i = 0; i < 24; i++) {
        b = ctk_xvideo->xv_sync_to_display_buttons[i];
        if (b == NULL) continue;
        udata = g_object_get_data(G_OBJECT(b),
                                  "xv_sync_to_display");
        
        prev_enabled |= GPOINTER_TO_INT(udata);
     }
     
     /* Remove devices that were previously enabled but are no
      * longer enabled. */
     remove_devices_mask = (prev_enabled & (~enabled));

     /* Add devices that were not previously enabled */
     add_devices_mask = (enabled & (~prev_enabled));
     prev_enabled =  enabled;
     for (mask = 1; mask; mask <<= 1) {
        if (mask & add_devices_mask)
            xv_sync_to_display_radio_button_enabled_add(ctk_xvideo, mask);
        
        if (mask & remove_devices_mask)
            xv_sync_to_display_radio_button_remove(ctk_xvideo, mask);
     }
     gtk_widget_show_all(ctk_xvideo->xv_sync_to_display_button_box);
} /* nv_ctrl_enabled_displays() */

/*
 * xv_sync_to_display_radio_button_enabled_add()
 */
static void xv_sync_to_display_radio_button_enabled_add(CtkXVideo *ctk_xvideo,
                                                        gint add_device_mask)
{
    GtkWidget *radio[24], *prev_radio = NULL, *b1, *b2;
    int n;
    ReturnStatus ret;
    char *name, *type;
    gchar *name_str;

    /* Get the previous radio button. */
    for (n = 0; n < 24; n++) {
        b1 = ctk_xvideo->xv_sync_to_display_buttons[n];
        if (b1 != NULL) {
            prev_radio = b1;
            break;
        }
    }
    /* Get the next index where to add button. */
    for (n = 0; n < 24; n++) {
        b2 = ctk_xvideo->xv_sync_to_display_buttons[n];
        if (b2 == NULL) break;
    }
    ret = NvCtrlGetStringDisplayAttribute(ctk_xvideo->handle, add_device_mask,
                                          NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                          &name);

    if ((ret != NvCtrlSuccess) || (!name)) {
        name = g_strdup("Unknown");
    }
    /* get the display device type */
    type = display_device_mask_to_display_device_name(add_device_mask);
    name_str = g_strdup_printf("%s (%s)", name, type);
    XFree(name);
    free(type);

    radio[n] = xv_sync_to_display_radio_button_add(ctk_xvideo,
                                                   prev_radio, name_str,
                                                   add_device_mask, n);
    g_free(name_str);
    ctk_config_set_tooltip(ctk_xvideo->ctk_config, radio[n],
                                            __xv_sync_to_display_help);
    

} /* xv_sync_to_display_radio_button_enabled_add() */

/*
 * xv_sync_to_display_update_received() - callback function for changed sync
 * to display settings; this is called when we receive an event indicating that
 * another NV-CONTROL client changed any of the settings that we care
 * about.
 */

static void xv_sync_to_display_update_received(GtkObject *object, 
                                               gpointer arg1,
                                               gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    gint i;
    GtkWidget *b;
    
    switch (event_struct->attribute) {
    case NV_CTRL_XV_SYNC_TO_DISPLAY:
        for (i = 0; i < 24; i++) {
            b = ctk_xvideo->xv_sync_to_display_buttons[i];
            if (!b) continue;
            user_data = g_object_get_data(G_OBJECT(b), "xv_sync_to_display");

            if (GPOINTER_TO_INT(user_data) == event_struct->value) {
                xv_sync_to_display_update_radio_buttons(ctk_xvideo, i,
                                                        TRUE);
                break;
            }
        }
        break;
        
    default:
        break;
    }
    
} /* xv_sync_to_display_update_received() */


/*
 * ctk_xvideo_new() - constructor for the XVideo widget
 */

GtkWidget* ctk_xvideo_new(NvCtrlAttributeHandle *handle,
                          CtkConfig *ctk_config,
                          CtkEvent *ctk_event)
{
    GObject *object;
    CtkXVideo *ctk_xvideo;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *alignment;
    GtkWidget *vbox;
    int sync_mask;
    int xv_overlay_present, xv_texture_present, xv_blitter_present;
    ReturnStatus ret;

    /*
     * before we do anything else, determine if any of the Xv adapters
     * are present
     */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_OVERLAY_PRESENT,
                             &xv_overlay_present);
    
    if (ret != NvCtrlSuccess) xv_overlay_present = FALSE;
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT,
                             &xv_texture_present);

    if (ret != NvCtrlSuccess) xv_texture_present = FALSE;
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT,
                             &xv_blitter_present);
    
    if (ret != NvCtrlSuccess) xv_blitter_present = FALSE;
    
    if (!xv_overlay_present && !xv_texture_present && !xv_blitter_present) {
        return NULL;
    }
    
    
    /* create the XVideo widget */
    
    object = g_object_new(CTK_TYPE_XVIDEO, NULL);
    ctk_xvideo = CTK_XVIDEO(object);
    
    ctk_xvideo->handle = handle;
    ctk_xvideo->ctk_config = ctk_config;
    ctk_xvideo->active_attributes = 0;
    
    gtk_box_set_spacing(GTK_BOX(ctk_xvideo), 10);
    
    
    /* Video film banner */
    
    banner = ctk_banner_image_new(BANNER_ARTWORK_XVIDEO);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /* Sync to display selection */
    if (xv_texture_present || xv_blitter_present) {
        ret = NvCtrlGetAttribute(handle,
                                 NV_CTRL_XV_SYNC_TO_DISPLAY,
                                 &sync_mask);
        if (ret == NvCtrlSuccess) {
            int enabled;
            ret = NvCtrlGetAttribute(handle, 
                                     NV_CTRL_ENABLED_DISPLAYS,
                                     &enabled);
            if (ret == NvCtrlSuccess) {

                GtkWidget *radio[24], *prev_radio;
                int i, n, current = -1, mask;
                char *name, *type;
                gchar *name_str;
                frame = gtk_frame_new("Sync to this display device");
                gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

                vbox = gtk_vbox_new(FALSE, 5);
                gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
                gtk_container_add(GTK_CONTAINER(frame), vbox);
                ctk_xvideo->xv_sync_to_display_button_box = vbox;

                for (n=0, i = 0; i < 24; i++) {

                    mask = 1 << i;
                    if (!(enabled & mask)) continue;

                    /* get the name of the display device */

                    ret = NvCtrlGetStringDisplayAttribute(handle, mask,
                                              NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                              &name);

                    if ((ret != NvCtrlSuccess) || (!name)) {
                        name = g_strdup("Unknown");
                    }

                    /* get the display device type */

                    type = display_device_mask_to_display_device_name(mask);

                    name_str = g_strdup_printf("%s (%s)", name, type);
                    XFree(name);
                    free(type);

                    if (n==0) {
                        prev_radio = NULL;
                    } else {
                        prev_radio = radio[n-1];
                    } 
                    radio[n] = xv_sync_to_display_radio_button_add(ctk_xvideo, 
                                                                   prev_radio,
                                                                   name_str,
                                                                   mask, n);
                    g_free(name_str);
                    ctk_config_set_tooltip(ctk_config, radio[n],
                                           __xv_sync_to_display_help);    

                    if (mask == sync_mask) {
                        current = n;
                    }

                    n++;
                    ctk_xvideo->active_attributes |= __XV_SYNC_TO_DISPLAY;
                }

                g_signal_connect(G_OBJECT(ctk_event),
                                 CTK_EVENT_NAME(NV_CTRL_XV_SYNC_TO_DISPLAY),
                                 G_CALLBACK(xv_sync_to_display_update_received),
                                 (gpointer) ctk_xvideo);
                g_signal_connect(G_OBJECT(ctk_event),
                                 CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                                 G_CALLBACK(nv_ctrl_enabled_displays),
                                 (gpointer) ctk_xvideo);

                if (current != -1) {
                    xv_sync_to_display_update_radio_buttons(ctk_xvideo,
                                                            current, FALSE);
                }
            }
        }
    }
    
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);
    
    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_xvideo));

    return GTK_WIDGET(ctk_xvideo);
    
} /* ctk_xvideo_new() */

GtkTextBuffer *ctk_xvideo_create_help(GtkTextTagTable *table,
                                      CtkXVideo *ctk_xvideo)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "X Server XVideo Settings Help");

    ctk_help_para(b, &i, "The X Server XVideo Settings page uses the XVideo "
                  "X extension.");
    
    if (ctk_xvideo->active_attributes & __XV_SYNC_TO_DISPLAY) {
        ctk_help_heading(b, &i, "Sync to this display device");
        ctk_help_para(b, &i, __xv_sync_to_display_help);
    }
    
    ctk_help_finish(b);

    return b;
}
