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
#include "NvCtrlAttributes.h"

#include "xvideo_banner.h"

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

static const char *__xv_blitter_sync_to_vblank_help =
"The Video Blitter Sync To VBlank checkbox "
"toggles syncing XvPutVideo(3X) and XvPutStill(3X) to "
"the vertical retrace of your display device "
"for the Blitter Xv Adaptor.";

static const char *__reset_button_help =
"The Reset Hardware Defaults button restores "
"the XVideo settings to their default values.";



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

static void reset_check_button(CtkXVideo *ctk_xvideo,
                               GtkWidget *check_button,
                               gint attribute);

static void set_button_sensitive(GtkButton *button);

static void reset_defaults(GtkButton *button, gpointer user_data);


#define FRAME_PADDING 5


#define __XV_OVERLAY_SATURATION     (1 << 1)
#define __XV_OVERLAY_CONTRAST       (1 << 2)
#define __XV_OVERLAY_BRIGHTNESS     (1 << 3)
#define __XV_OVERLAY_HUE            (1 << 4)
#define __XV_TEXTURE_SYNC_TO_VBLANK (1 << 5)
#define __XV_BLITTER_SYNC_TO_VBLANK (1 << 6)



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
        };

        ctk_xvideo_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkXVideo", &ctk_xvideo_info, 0);
    }

    return ctk_xvideo_type;
}



/*
 * ctk_xvideo_new() - constructor for the XVideo widget
 */

GtkWidget* ctk_xvideo_new(NvCtrlAttributeHandle *handle,
                          CtkConfig *ctk_config)
{
    GObject *object;
    CtkXVideo *ctk_xvideo;
    GtkWidget *image;
    GtkWidget *frame;
    GtkWidget *alignment;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *button;
    
    guint8 *image_buffer = NULL;
    const nv_image_t *img;
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
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
    
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    img = &xvideo_banner_image;

    image_buffer = decompress_image_data(img);

    image = gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));

    gtk_container_add(GTK_CONTAINER(frame), image);
    
    
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
            create_check_button(ctk_xvideo, vbox, button, "Sync To VBlank",
                                __xv_texture_sync_to_vblank_help,
                                NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK,
                                __XV_TEXTURE_SYNC_TO_VBLANK);
    }
    
    /* XVideo Blitter */

    if (xv_blitter_present) {

        
        frame = gtk_frame_new("Video Blitter Adaptor");
        gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
        
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        ctk_xvideo->blitter_sync_to_blank =
            create_check_button(ctk_xvideo, vbox, button, "Sync To VBlank",
                                __xv_blitter_sync_to_vblank_help,
                                NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK,
                                __XV_BLITTER_SYNC_TO_VBLANK);
    }
    
    /* Reset button */

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
    
    /* get the attribute value */

    NvCtrlGetAttribute(ctk_xvideo->handle, attribute, &val);

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
    
    ctk_config_statusbar_message(ctk_xvideo->ctk_config, "%s XVideo %s.",
                                 enabled ? "Enabled" : "Disabled", str);
} /* check_button_toggled() */



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

    if (ctk_xvideo->active_attributes & __XV_BLITTER_SYNC_TO_VBLANK) {
        ctk_help_heading(b, &i, "Video Blitter Sync To VBlank");
        ctk_help_para(b, &i, __xv_blitter_sync_to_vblank_help);
    }
    
    ctk_help_heading(b, &i, "Reset Hardware Defaults");
    ctk_help_para(b, &i, __reset_button_help);

    ctk_help_finish(b);

    return b;
}
