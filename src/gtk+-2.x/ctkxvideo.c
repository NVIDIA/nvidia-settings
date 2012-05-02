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
#include <gtk/gtk.h>
#include "NvCtrlAttributes.h"

#include "ctkbanner.h"

#include "ctkxvideo.h"
#include "ctkscale.h"

#include "ctkhelp.h"


static const char *__xv_overlay_saturation_help =
"The Video Overlay Saturation slider controls "
"the saturation level for the Overlay Xv Adaptor.";

static const char *__xv_overlay_contrast_help =
"The Video Overlay Contrast slider controls "
"the contrast level for the Overlay Xv Adaptor.";

static const char *__xv_overlay_brightness_help =
"The Video Overlay Brightness slider controls "
"the brightness level for the Overlay Xv Adaptor.";

static const char *__xv_overlay_hue_help =
"The Video Overlay Hue slider controls "
"the hue level for the Overlay Xv Adaptor.";

static const char *__xv_texture_sync_to_vblank_help =
"The Video Texture Sync To VBlank checkbox "
"toggles syncing XvPutVideo(3X) and XvPutStill(3X) to "
"the vertical retrace of your display device "
"for the Texture Xv Adaptor.";

static const char *__xv_texture_contrast_help =
"The Video Texture Contrast slider controls "
"the contrast level for the Texture Xv Adaptor.";

static const char *__xv_texture_brightness_help =
"The Video Texture Brightness slider controls "
"the brightness level for the Texture Xv Adaptor.";

static const char *__xv_texture_hue_help =
"The Video Texture Hue slider controls "
"the hue level for the Texture Xv Adaptor.";

static const char *__xv_texture_saturation_help =
"The Video Texture Saturation slider controls "
"the saturation level for the Texture Xv Adaptor.";

static const char *__xv_blitter_sync_to_vblank_help =
"The Video Blitter Sync To VBlank checkbox "
"toggles syncing XvPutVideo(3X) and XvPutStill(3X) to "
"the vertical retrace of your display device "
"for the Blitter Xv Adaptor.";

static const char *__xv_sync_to_display_help =
"This controls which display device will be synched to when "
"XVideo Sync To VBlank is enabled.";
 
static const char *__reset_button_help =
"The Reset Hardware Defaults button restores "
"the XVideo settings to their default values.";


static void xv_sync_to_display_changed(GtkWidget *widget, gpointer user_data);

static GtkWidget *xv_sync_to_display_radio_button_add(CtkXVideo *ctk_xvideo,
                                                      GtkWidget *prev_radio,
                                                      char *label,
                                                      gint value,
                                                      int index);

static void
xv_sync_to_display_update_radio_buttons(CtkXVideo *ctk_xvideo, gint value);

static void xv_sync_to_display_changed(GtkWidget *widget, gpointer user_data);

static void xv_sync_to_display_update_received(GtkObject *object, gpointer arg1,
                                               gpointer user_data);

static GtkWidget *create_slider(CtkXVideo *ctk_xvideo,
                                GtkWidget *vbox,
                                GtkWidget *button,
                                const gchar *name,
                                const char *help,
                                gint attribute,
                                unsigned int bit);

static void reset_slider(CtkXVideo *ctk_xvideo,
                         GtkWidget *slider,
                         gint attribute);

static void slider_changed(GtkAdjustment *adjustment, gpointer user_data);

static GtkWidget *create_check_button(CtkXVideo *ctk_xvideo,
                                      GtkWidget *vbox,
                                      GtkWidget *button,
                                      const gchar *name,
                                      const char *help,
                                      gint attribute,
                                      unsigned int bit);

static void check_button_toggled(GtkWidget *widget, gpointer user_data);


static void post_check_button_toggled(CtkXVideo *ctk_xvideo,
                                      gchar *str,
                                      gboolean enabled);

static void sensitize_radio_buttons(CtkXVideo *ctk_xvideo);

static void reset_check_button(CtkXVideo *ctk_xvideo,
                               GtkWidget *check_button,
                               gint attribute);

static void set_button_sensitive(GtkButton *button);

static void reset_defaults(GtkButton *button, gpointer user_data);

static void post_xv_sync_to_display_changed(CtkXVideo *ctk_xvideo,
                                            gboolean enabled,
                                            gchar *label);
static void nv_ctrl_enabled_displays(GtkObject *object, gpointer arg1,
                                     gpointer user_data);
static void xv_sync_to_display_radio_button_remove(CtkXVideo *ctk_xvideo,
                                                   gint value);
static void xv_sync_to_display_radio_button_enabled_add(CtkXVideo *ctk_xvideo,
                                                        gint add_device_mask);

#define FRAME_PADDING 5


#define __XV_OVERLAY_SATURATION     (1 << 1)
#define __XV_OVERLAY_CONTRAST       (1 << 2)
#define __XV_OVERLAY_BRIGHTNESS     (1 << 3)
#define __XV_OVERLAY_HUE            (1 << 4)
#define __XV_TEXTURE_SYNC_TO_VBLANK (1 << 5)
#define __XV_TEXTURE_CONTRAST       (1 << 6)
#define __XV_TEXTURE_BRIGHTNESS     (1 << 7)
#define __XV_TEXTURE_SATURATION     (1 << 8)
#define __XV_TEXTURE_HUE            (1 << 9)
#define __XV_BLITTER_SYNC_TO_VBLANK (1 << 10)
#define __XV_SYNC_TO_DISPLAY        (1 << 11)



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
xv_sync_to_display_update_radio_buttons(CtkXVideo *ctk_xvideo, gint value)
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
    gint value;
    gchar *label;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {

        user_data = g_object_get_data(G_OBJECT(widget), "xv_sync_to_display");
        
        value = GPOINTER_TO_INT(user_data);
        
        NvCtrlSetAttribute(ctk_xvideo->handle,
                           NV_CTRL_XV_SYNC_TO_DISPLAY, value);
                           
        gtk_label_get(GTK_LABEL(GTK_BIN(widget)->child), &label);

        post_xv_sync_to_display_changed(ctk_xvideo, enabled, label);
     }
}/* xv_sync_to_display_changed() */


static void post_xv_sync_to_display_changed(CtkXVideo *ctk_xvideo,
                                            gboolean enabled,
                                            gchar *label)
{   
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
     CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
     int i, enabled, prev_enabled = 0;
     int remove_devices_mask, add_devices_mask;
     unsigned int mask;
     gpointer udata;
     GtkWidget *b;

     enabled = event_struct->value;
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
     sensitize_radio_buttons(ctk_xvideo);
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
                xv_sync_to_display_update_radio_buttons(ctk_xvideo, i);
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
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *button;
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
    
    
    /* Create button, but don't pack it, yet */

    label = gtk_label_new("Reset Hardware Defaults");
    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new();

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(reset_defaults), (gpointer) ctk_xvideo);
    
    ctk_config_set_tooltip(ctk_config, button, __reset_button_help);
    
    /* Video film banner */
    
    banner = ctk_banner_image_new(BANNER_ARTWORK_XVIDEO);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    
    /* XVideo Overlay sliders */
    
    if (xv_overlay_present) {
        
        frame = gtk_frame_new("Video Overlay Adaptor");
        gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
        
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        ctk_xvideo->overlay_saturation =
            create_slider(ctk_xvideo, vbox, button, "Saturation",
                          __xv_overlay_saturation_help,
                          NV_CTRL_ATTR_XV_OVERLAY_SATURATION,
                          __XV_OVERLAY_SATURATION);
        
        ctk_xvideo->overlay_contrast =
            create_slider(ctk_xvideo, vbox, button, "Contrast",
                          __xv_overlay_contrast_help,
                          NV_CTRL_ATTR_XV_OVERLAY_CONTRAST,
                          __XV_OVERLAY_CONTRAST);
        
        ctk_xvideo->overlay_brightness =
            create_slider(ctk_xvideo, vbox, button, "Brightness",
                          __xv_overlay_brightness_help,
                          NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS,
                          __XV_OVERLAY_BRIGHTNESS);
        
        ctk_xvideo->overlay_hue =
            create_slider(ctk_xvideo, vbox, button, "Hue",
                          __xv_overlay_hue_help,
                          NV_CTRL_ATTR_XV_OVERLAY_HUE,
                          __XV_OVERLAY_HUE);
    }

    /* XVideo Texture */

    if (xv_texture_present) {
        
        frame = gtk_frame_new("Video Texture Adaptor");
        gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
        
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        ctk_xvideo->texture_sync_to_blank =
            create_check_button(ctk_xvideo, vbox, button, "Sync to VBlank",
                                __xv_texture_sync_to_vblank_help,
                                NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK,
                                __XV_TEXTURE_SYNC_TO_VBLANK);
        
        ctk_xvideo->texture_brightness =
            create_slider(ctk_xvideo, vbox, button, "Brightness",
                          __xv_texture_brightness_help,
                          NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS,
                          __XV_TEXTURE_BRIGHTNESS);
        
        ctk_xvideo->texture_contrast =
            create_slider(ctk_xvideo, vbox, button, "Contrast",
                          __xv_texture_contrast_help,
                          NV_CTRL_ATTR_XV_TEXTURE_CONTRAST,
                          __XV_TEXTURE_CONTRAST);

        ctk_xvideo->texture_hue =
            create_slider(ctk_xvideo, vbox, button, "Hue",
                          __xv_texture_hue_help,
                          NV_CTRL_ATTR_XV_TEXTURE_HUE,
                          __XV_TEXTURE_HUE);
        
        ctk_xvideo->texture_saturation =
            create_slider(ctk_xvideo, vbox, button, "Saturation",
                          __xv_texture_saturation_help,
                          NV_CTRL_ATTR_XV_TEXTURE_SATURATION,
                          __XV_TEXTURE_SATURATION);
    
    }
    
    /* XVideo Blitter */

    if (xv_blitter_present) {

        frame = gtk_frame_new("Video Blitter Adaptor Settings");
        gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
        
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        ctk_xvideo->blitter_sync_to_blank =
            create_check_button(ctk_xvideo,
                                vbox,
                                button,
                                "Sync to VBlank",
                                __xv_blitter_sync_to_vblank_help,
                                NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK,
                                __XV_BLITTER_SYNC_TO_VBLANK);

    }

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
                sensitize_radio_buttons(ctk_xvideo);

                if (current != -1)
                    xv_sync_to_display_update_radio_buttons(ctk_xvideo, current);

            }
        }
    }
    
    /* Reset button */

    sensitize_radio_buttons(ctk_xvideo);
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);
    
    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_xvideo));

    return GTK_WIDGET(ctk_xvideo);
    
} /* ctk_xvideo_new() */



/*
 * create_slider() - 
 */

static GtkWidget *create_slider(CtkXVideo *ctk_xvideo,
                                GtkWidget *vbox,
                                GtkWidget *button,
                                const gchar *name,
                                const char *help,
                                gint attribute,
                                unsigned int bit)
{
    GtkObject *adjustment;
    GtkWidget *scale, *widget;
    gint min, max, val, step_incr, page_incr;
    NVCTRLAttributeValidValuesRec range;
    ReturnStatus ret;
    
    /* get the attribute value */

    ret = NvCtrlGetAttribute(ctk_xvideo->handle, attribute, &val);
    
    if (ret != NvCtrlSuccess) return NULL;
    
    /* get the range for the attribute */

    NvCtrlGetValidAttributeValues(ctk_xvideo->handle, attribute, &range);

    if (range.type != ATTRIBUTE_TYPE_RANGE) return NULL;
    
    min = range.u.range.min;
    max = range.u.range.max;

    step_incr = ((max) - (min))/250;
    if (step_incr <= 0) step_incr = 1;
    
    page_incr = ((max) - (min))/25;
    if (page_incr <= 0) page_incr = 1;

    /* create the slider */
    
    adjustment = gtk_adjustment_new(val, min, max,
                                    step_incr, page_incr, 0.0);

    g_object_set_data(G_OBJECT(adjustment), "xvideo_attribute",
                      GINT_TO_POINTER(attribute));

    g_signal_connect(G_OBJECT(adjustment), "value_changed",
                     G_CALLBACK(slider_changed),
                     (gpointer) ctk_xvideo);

    g_signal_connect_swapped(G_OBJECT(adjustment), "value_changed",
                             G_CALLBACK(set_button_sensitive),
                             (gpointer) button);

    scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), name,
                          ctk_xvideo->ctk_config, G_TYPE_INT);

    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    ctk_xvideo->active_attributes |= bit;

    widget = CTK_SCALE(scale)->gtk_scale;
    
    ctk_config_set_tooltip(ctk_xvideo->ctk_config, widget, help);

    return scale;
    
} /* create_slider() */



/*
 * reset_slider() - 
 */

static void reset_slider(CtkXVideo *ctk_xvideo,
                         GtkWidget *slider,
                         gint attribute)
{
    GtkAdjustment *adjustment;
    gint val;
    
    if (!slider) return;

    adjustment = CTK_SCALE(slider)->gtk_adjustment;

    NvCtrlGetAttribute(ctk_xvideo->handle, attribute, &val);
    
    g_signal_handlers_block_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(slider_changed), NULL);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), val);
    
    g_signal_handlers_unblock_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(slider_changed), NULL);

} /* reset_slider() */



/*
 * slider_changed() -
 */

static void slider_changed(GtkAdjustment *adjustment, gpointer user_data)
{
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    gint attribute, value;
    gchar *str;
    
    user_data = g_object_get_data(G_OBJECT(adjustment), "xvideo_attribute");
    attribute = GPOINTER_TO_INT(user_data);
    value = (gint) adjustment->value;
    
    NvCtrlSetAttribute(ctk_xvideo->handle, attribute, value);

    switch (attribute) {
    case NV_CTRL_ATTR_XV_OVERLAY_SATURATION: str = "Overlay Saturation"; break;
    case NV_CTRL_ATTR_XV_OVERLAY_CONTRAST:   str = "Overlay Contrast";   break;
    case NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS: str = "Overlay Brightness"; break;
    case NV_CTRL_ATTR_XV_OVERLAY_HUE:        str = "Overlay Hue";        break;
    case NV_CTRL_ATTR_XV_TEXTURE_CONTRAST:   str = "Texture Contrast";   break;
    case NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS: str = "Texture Brightness"; break;
    case NV_CTRL_ATTR_XV_TEXTURE_HUE:        str = "Texture Hue";        break;
    case NV_CTRL_ATTR_XV_TEXTURE_SATURATION: str = "Texture Saturation"; break;

    default:
        return;
    }
    
    ctk_config_statusbar_message(ctk_xvideo->ctk_config,
                                 "Set XVideo %s to %d.", str, value);
    
} /* slider_changed() */



/*
 * create_check_button() - 
 */

static GtkWidget *create_check_button(CtkXVideo *ctk_xvideo,
                                      GtkWidget *vbox,
                                      GtkWidget *button,
                                      const gchar *name,
                                      const char *help,
                                      gint attribute,
                                      unsigned int bit)
{
    GtkWidget *check_button;
    ReturnStatus ret;
    int val;
    
    /* get the attribute value */
    
    ret = NvCtrlGetAttribute(ctk_xvideo->handle, attribute, &val);
    
    if (ret != NvCtrlSuccess) return NULL;
    
    check_button = gtk_check_button_new_with_label(name);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);
            
    g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(check_button_toggled),
                     (gpointer) ctk_xvideo);

    g_signal_connect_swapped(G_OBJECT(check_button), "toggled",
                             G_CALLBACK(set_button_sensitive),
                             (gpointer) button);

    g_object_set_data(G_OBJECT(check_button), "xvideo_attribute",
                      GINT_TO_POINTER(attribute));

    ctk_config_set_tooltip(ctk_xvideo->ctk_config, check_button, name);

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);
    
    ctk_xvideo->active_attributes |= bit;

    ctk_config_set_tooltip(ctk_xvideo->ctk_config, check_button, help);

    return check_button;
    
} /* create_check_button() */



/*
 * check_button_toggled() - 
 */

static void check_button_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    gint attribute, enabled;
    gchar *str;
    
    user_data = g_object_get_data(G_OBJECT(widget), "xvideo_attribute");
    attribute = GPOINTER_TO_INT(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    NvCtrlSetAttribute(ctk_xvideo->handle, attribute, enabled);
    
    switch (attribute) {
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
        str = "Texture Sync To VBlank"; break;
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
        str = "Blitter Sync To VBlank"; break;

    default:
        return;
    }
    sensitize_radio_buttons(ctk_xvideo);
    post_check_button_toggled(ctk_xvideo, str, enabled);
} /* check_button_toggled() */

/*
 * post_check_button_toggled()
 */

static void post_check_button_toggled(CtkXVideo *ctk_xvideo, gchar *str, gboolean enabled)
{
    ctk_config_statusbar_message(ctk_xvideo->ctk_config, "%s XVideo %s.",
                                 enabled ? "Enabled" : "Disabled", str);
} /* post_check_button_toggled() */

/*
 * sensitize_radio_buttons()
 */
static void sensitize_radio_buttons(CtkXVideo *ctk_xvideo)
{
    int i;
    gboolean enabled;

    if (!ctk_xvideo->blitter_sync_to_blank &&
        !ctk_xvideo->texture_sync_to_blank) return;

    enabled = FALSE;
    if (ctk_xvideo->blitter_sync_to_blank) {
        enabled |= gtk_toggle_button_get_active
            (GTK_TOGGLE_BUTTON(ctk_xvideo->blitter_sync_to_blank));
    }
    if (ctk_xvideo->texture_sync_to_blank) {
        enabled |= gtk_toggle_button_get_active
            (GTK_TOGGLE_BUTTON(ctk_xvideo->texture_sync_to_blank));
    }
    for (i = 0; i < 24; i++) {
        GtkWidget *b = ctk_xvideo->xv_sync_to_display_buttons[i];
        if (!b) continue;
        gtk_widget_set_sensitive(b, enabled);
    }
} /* sensitize_radio_buttons() */


/*
 * reset_check_button() - 
 */

static void reset_check_button(CtkXVideo *ctk_xvideo,
                               GtkWidget *check_button,
                               gint attribute)
{
    gint val;

    if (!check_button) return;
    
    NvCtrlGetAttribute(ctk_xvideo->handle, attribute, &val);
    
    g_signal_handlers_block_matched
        (G_OBJECT(check_button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(check_button_toggled), NULL);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

    g_signal_handlers_unblock_matched
        (G_OBJECT(check_button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(check_button_toggled), NULL);
    sensitize_radio_buttons(ctk_xvideo);

} /* reset_check_button() */



static void set_button_sensitive(GtkButton *button)
{
    gtk_widget_set_sensitive(GTK_WIDGET(button), TRUE);
}




static void reset_defaults(GtkButton *button, gpointer user_data)
{
    CtkXVideo *ctk_xvideo = CTK_XVIDEO(user_data);
    
    /*
     * XXX should we expose separate buttons for each adaptor's
     * defaults?
     */
    
    NvCtrlSetAttribute(ctk_xvideo->handle,
                       NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS, 1);

    NvCtrlSetAttribute(ctk_xvideo->handle,
                       NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS, 1);
    
    NvCtrlSetAttribute(ctk_xvideo->handle,
                       NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS, 1);
    

    reset_slider(ctk_xvideo, ctk_xvideo->overlay_saturation,
                 NV_CTRL_ATTR_XV_OVERLAY_SATURATION);
    
    reset_slider(ctk_xvideo, ctk_xvideo->overlay_contrast,
                 NV_CTRL_ATTR_XV_OVERLAY_CONTRAST);
    
    reset_slider(ctk_xvideo, ctk_xvideo->overlay_brightness,
                 NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS);
    
    reset_slider(ctk_xvideo, ctk_xvideo->overlay_hue,
                 NV_CTRL_ATTR_XV_OVERLAY_HUE);

    reset_check_button(ctk_xvideo, ctk_xvideo->texture_sync_to_blank,
                       NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK);
    
    reset_slider(ctk_xvideo, ctk_xvideo->texture_contrast,
                 NV_CTRL_ATTR_XV_TEXTURE_CONTRAST);
    
    reset_slider(ctk_xvideo, ctk_xvideo->texture_brightness,
                 NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS);

    reset_slider(ctk_xvideo, ctk_xvideo->texture_hue,
                 NV_CTRL_ATTR_XV_TEXTURE_HUE);

    reset_slider(ctk_xvideo, ctk_xvideo->texture_saturation,
                 NV_CTRL_ATTR_XV_TEXTURE_SATURATION);


    reset_check_button(ctk_xvideo, ctk_xvideo->blitter_sync_to_blank,
                       NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK);
    

    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    ctk_config_statusbar_message
        (CTK_CONFIG(ctk_xvideo->ctk_config),
         "Reset XVideo hardware defaults.");
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

    if (ctk_xvideo->active_attributes & __XV_OVERLAY_SATURATION) {
        ctk_help_heading(b, &i, "Video Overlay Saturation");
        ctk_help_para(b, &i, __xv_overlay_saturation_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_OVERLAY_CONTRAST) {
        ctk_help_heading(b, &i, "Video Overlay Contrast");
        ctk_help_para(b, &i, __xv_overlay_contrast_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_OVERLAY_BRIGHTNESS) {   
        ctk_help_heading(b, &i, "Video Overlay Brightness");
        ctk_help_para(b, &i, __xv_overlay_brightness_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_OVERLAY_HUE) {
        ctk_help_heading(b, &i, "Video Overlay Hue");
        ctk_help_para(b, &i, __xv_overlay_hue_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_TEXTURE_SYNC_TO_VBLANK) {
        ctk_help_heading(b, &i, "Video Texture Sync To VBlank");
        ctk_help_para(b, &i, __xv_texture_sync_to_vblank_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_TEXTURE_BRIGHTNESS) {   
        ctk_help_heading(b, &i, "Video Texture Brightness");
        ctk_help_para(b, &i, __xv_texture_brightness_help);
    }

    if (ctk_xvideo->active_attributes & __XV_TEXTURE_CONTRAST) {
        ctk_help_heading(b, &i, "Video Texture Contrast");
        ctk_help_para(b, &i, __xv_texture_contrast_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_TEXTURE_HUE) {
        ctk_help_heading(b, &i, "Video Texture Hue");
        ctk_help_para(b, &i, __xv_texture_hue_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_TEXTURE_SATURATION) {
        ctk_help_heading(b, &i, "Video Texture Saturation");
        ctk_help_para(b, &i, __xv_texture_saturation_help);
    }

    if (ctk_xvideo->active_attributes & __XV_BLITTER_SYNC_TO_VBLANK) {
        ctk_help_heading(b, &i, "Video Blitter Sync To VBlank");
        ctk_help_para(b, &i, __xv_blitter_sync_to_vblank_help);
    }
    
    if (ctk_xvideo->active_attributes & __XV_SYNC_TO_DISPLAY) {
        ctk_help_heading(b, &i, "Sync to this display device");
        ctk_help_para(b, &i, __xv_sync_to_display_help);
    }
    
    ctk_help_heading(b, &i, "Reset Hardware Defaults");
    ctk_help_para(b, &i, __reset_button_help);

    ctk_help_finish(b);

    return b;
}
