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
#include "NVCtrlLib.h"

#include "msg.h"

#include "ctkbanner.h"
#include "ctkxvideo.h"
#include "ctkutils.h"
#include "ctkhelp.h"


static const char *__xv_sync_to_display_help =
"This controls which display device will be synched to when "
"XVideo Sync To VBlank is enabled.";


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
 * Updates the status bar for when a change occured.
 */
static void post_xv_sync_to_display_update(CtkXVideo *ctk_xvideo,
                                           GtkWidget *active_button)
{
    const gchar *label;

    label = gtk_button_get_label(GTK_BUTTON(active_button));

    ctk_config_statusbar_message(ctk_xvideo->ctk_config,
                                     "XVideo application syncing to %s.",
                                     label);
}



/*
 * xv_sync_to_display_id_toggled() - callback function for changes to the
 * sync_to_display radio button group; if the specified radio button is
 * active, send xv_sync_to_display state to the server
 */
static void xv_sync_to_display_id_toggled(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    gboolean enabled;
    gint device_id;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    if (!enabled) {
        /* Ignore 'disable' events. */
        return;
    }

    user_data = g_object_get_data(G_OBJECT(widget), "display_id");

    device_id = GPOINTER_TO_INT(user_data);

    NvCtrlSetAttribute(ctk_xvideo->handle, NV_CTRL_XV_SYNC_TO_DISPLAY_ID,
                       device_id);

    post_xv_sync_to_display_update(ctk_xvideo, widget);
}



/*
 * Sets the radio button at the given index as enabled.
 */
static void xv_sync_to_display_set_enabled(CtkXVideo *ctk_xvideo,
                                           GtkWidget *button,
                                           gboolean update_status)
{
    /* turn off signal handling.  Note that we only disable events
     * for the button being enabled since we ignore disable events.
     */
    g_signal_handlers_block_by_func
        (G_OBJECT(button), G_CALLBACK(xv_sync_to_display_id_toggled),
         (gpointer) ctk_xvideo);

    /* set the button as active */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

    /* turn on signal handling */

    g_signal_handlers_unblock_by_func
        (G_OBJECT(button), G_CALLBACK(xv_sync_to_display_id_toggled),
         (gpointer) ctk_xvideo);

    if (update_status) {
        post_xv_sync_to_display_update(ctk_xvideo, button);
    }
}



/*
 * xv_sync_to_display_radio_button_add() - create a radio button and plug it
 * into the xv_sync_display_buttons radio group.
 */
static GtkWidget *xv_sync_to_display_radio_button_add(CtkXVideo *ctk_xvideo,
                                                      GtkWidget *last_button,
                                                      gint display_id)
{
    Bool valid;
    char *name;
    char *randr;
    gchar *label;
    GtkWidget *button;
    GSList *slist;


    valid =
        XNVCTRLQueryTargetStringAttribute(NvCtrlGetDisplayPtr(ctk_xvideo->handle),
                                          NV_CTRL_TARGET_TYPE_DISPLAY,
                                          display_id,
                                          0,
                                          NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                          &name);
    if (!valid) {
        name = NULL;
    }

    valid =
        XNVCTRLQueryTargetStringAttribute(NvCtrlGetDisplayPtr(ctk_xvideo->handle),
                                          NV_CTRL_TARGET_TYPE_DISPLAY,
                                          display_id,
                                          0,
                                          NV_CTRL_STRING_DISPLAY_NAME_RANDR,
                                          &randr);
    if (!valid) {
        randr = NULL;
    }

    if (name && randr) {
        label = g_strdup_printf("%s (%s)", name, randr);
    } else {
        label = g_strdup_printf("%s",
                                name ? name : (randr ? randr : "Unknown"));
    }

    XFree(name);
    XFree(randr);

    if (last_button) {
        slist = gtk_radio_button_get_group(GTK_RADIO_BUTTON(last_button));
    } else {
        slist = NULL;
    }
    button = gtk_radio_button_new_with_label(slist, label);
    g_free(label);

    gtk_box_pack_start(GTK_BOX(ctk_xvideo->xv_sync_to_display_button_box),
                       button, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(button), "display_id",
                      GINT_TO_POINTER(display_id));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(xv_sync_to_display_id_toggled),
                     (gpointer) ctk_xvideo);

    ctk_config_set_tooltip(ctk_xvideo->ctk_config, button,
                           __xv_sync_to_display_help);

    return button;
}



/*
 * Rebuilds the list of display devices available for syncing.
 */
static void xv_sync_to_display_rebuild_buttons(CtkXVideo *ctk_xvideo,
                                               gboolean update_status)
{
    ReturnStatus ret;
    int enabled_display_id;
    int *pData;
    int len;
    int i;

    GtkWidget *last_button;


    /* Remove all buttons */

    ctk_empty_container(ctk_xvideo->xv_sync_to_display_button_box);


    /* Rebuild the list based on the curren configuration */

    ret = NvCtrlGetAttribute(ctk_xvideo->handle,
                             NV_CTRL_XV_SYNC_TO_DISPLAY_ID,
                             &enabled_display_id);
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query XV Sync display ID on X screen %d.",
                       NvCtrlGetTargetId(ctk_xvideo->handle));
        return;
    }

    ret = NvCtrlGetBinaryAttribute(ctk_xvideo->handle, 0,
                                   NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN,
                                   (unsigned char **)(&pData), &len);
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query list of displays assigned to X screen "
                       " %d.",
                       NvCtrlGetTargetId(ctk_xvideo->handle));
        return;
    }


    /* Add a button for each display device */

    last_button = NULL;
    for (i = 0; i < pData[0]; i++) {
        GtkWidget *button;
        int display_id = pData[1+i];

        button = xv_sync_to_display_radio_button_add(ctk_xvideo,
                                                     last_button,
                                                     display_id);
        if (!button) {
            continue;
        }

        /* Make sure the enabled display is marked as so */
        if (display_id == enabled_display_id) {
            xv_sync_to_display_set_enabled(ctk_xvideo, button,
                                           update_status);
        }

        /* Track the first button */
        if (!last_button) {
            ctk_xvideo->xv_sync_to_display_buttons = button;
        }

        last_button = button;
    }

    gtk_widget_show_all(ctk_xvideo->xv_sync_to_display_button_box);
}



/*
 * Handles NV_CTRL_ENABLED_DISPLAYS events and updates
 * the list of displays in the UI.
 */
static void  enabled_displays_handler(GObject *object, gpointer arg1,
                                      gpointer user_data)
{
     CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);

     xv_sync_to_display_rebuild_buttons(ctk_xvideo, TRUE);
}



/*
 * Handler for NV_CTRL_XV_SYNC_TO_DISPLAY_ID events.
 */
static void xv_sync_to_display_id_handler(GObject *object,
                                          gpointer arg1,
                                          gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    GSList *slist;

    /* Find the right button and enable it */

    slist =
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(ctk_xvideo->xv_sync_to_display_buttons));

    while (slist) {
        GtkWidget *button = GTK_WIDGET(slist->data);

        user_data = g_object_get_data(G_OBJECT(button), "display_id");
        if (GPOINTER_TO_INT(user_data) == event_struct->value) {
            xv_sync_to_display_set_enabled(ctk_xvideo, button, TRUE);
            break;
        }

        slist = g_slist_next(slist);
    }
}



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
    int xv_overlay_present, xv_texture_present, xv_blitter_present;
    gboolean show_page;
    ReturnStatus ret;

    /*
     * before we do anything else, determine if any of the Xv adapters
     * are present
     */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_OVERLAY_PRESENT,
                             &xv_overlay_present);
    if (ret != NvCtrlSuccess) {
        xv_overlay_present = FALSE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT,
                             &xv_texture_present);
    if (ret != NvCtrlSuccess) {
        xv_texture_present = FALSE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT,
                             &xv_blitter_present);
    if (ret != NvCtrlSuccess) {
        xv_blitter_present = FALSE;
    }

    if (!xv_overlay_present && !xv_texture_present && !xv_blitter_present) {
        return NULL;
    }

    /* If nothing to show, bail */

    show_page = FALSE;
    if (xv_texture_present || xv_blitter_present) {
        int display_id;
        ret = NvCtrlGetAttribute(handle,
                                 NV_CTRL_XV_SYNC_TO_DISPLAY_ID,
                                 &display_id);
        if (ret == NvCtrlSuccess) {
            show_page = TRUE;
        }
    }

    if (!show_page) {
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

    frame = gtk_frame_new("Sync to this display device");
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    ctk_xvideo->xv_sync_to_display_button_box = vbox;

    xv_sync_to_display_rebuild_buttons(ctk_xvideo, FALSE);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_XV_SYNC_TO_DISPLAY_ID),
                     G_CALLBACK(xv_sync_to_display_id_handler),
                     (gpointer) ctk_xvideo);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_handler),
                     (gpointer) ctk_xvideo);


    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_xvideo));

    return GTK_WIDGET(ctk_xvideo);
}



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
        ctk_help_para(b, &i, "%s", __xv_sync_to_display_help);
    }

    ctk_help_finish(b);

    return b;
}
