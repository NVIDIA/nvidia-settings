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

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkimagesliders.h"

#include "ctkscale.h"
#include "ctkconfig.h"
#include "ctkhelp.h"


#define FRAME_PADDING 5

static const char *__digital_vibrance_help = "The Digital Vibrance slider "
"alters the level of Digital Vibrance for this display device.";

static const char *__overscan_compensation_help = "The Overscan Compensation "
"slider adjusts the amount of overscan compensation applied to this display "
"device, in raster pixels.";

static const char *__image_sharpening_help = "The Image Sharpening slider "
"alters the level of Image Sharpening for this display device.";


static GtkWidget * add_scale(CtkConfig *ctk_config,
                             int attribute,
                             char *name,
                             const char *help,
                             gint value_type,
                             gpointer callback_data);

static void setup_scale(CtkImageSliders *ctk_image_sliders,
                        int attribute, GtkWidget *scale);

static void scale_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data);

static void scale_value_received(GtkObject *, gpointer arg1, gpointer);


GType ctk_image_sliders_get_type(void)
{
    static GType ctk_image_sliders_type = 0;
    
    if (!ctk_image_sliders_type) {
        static const GTypeInfo ctk_image_sliders_info = {
            sizeof (CtkImageSlidersClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkImageSliders),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_image_sliders_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkImageSliders", &ctk_image_sliders_info, 0);
    }

    return ctk_image_sliders_type;
}


GtkWidget* ctk_image_sliders_new(NvCtrlAttributeHandle *handle,
                                 CtkConfig *ctk_config, CtkEvent *ctk_event,
                                 GtkWidget *reset_button,
                                 unsigned int display_device_mask,
                                 char *name)
{
    CtkImageSliders *ctk_image_sliders;
 
    GObject *object;
    
    GtkWidget *frame;
    GtkWidget *vbox;
    ReturnStatus status;
    gint val;
    
    /*
     * now that we know that we will have atleast one attribute,
     * create the object
     */

    object = g_object_new(CTK_TYPE_IMAGE_SLIDERS, NULL);

    ctk_image_sliders = CTK_IMAGE_SLIDERS(object);
    ctk_image_sliders->handle = handle;
    ctk_image_sliders->ctk_config = ctk_config;
    ctk_image_sliders->ctk_event = ctk_event;
    ctk_image_sliders->reset_button = reset_button;
    ctk_image_sliders->display_device_mask = display_device_mask;
    ctk_image_sliders->name = name;
    
    /* cache image sharpening default value */

    status = NvCtrlGetDisplayAttribute(ctk_image_sliders->handle,
                                       ctk_image_sliders->display_device_mask,
                                       NV_CTRL_IMAGE_SHARPENING_DEFAULT,
                                       &val);
    if (status != NvCtrlSuccess) {
        val = 0;
    }
    ctk_image_sliders->default_val = val;
    
    /* create the frame and vbox */
    
    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    ctk_image_sliders->frame = frame;
    
    /* Digital Vibrance */
    
    ctk_image_sliders->digital_vibrance =
        add_scale(ctk_config,
                  NV_CTRL_DIGITAL_VIBRANCE, "Digital Vibrance",
                  __digital_vibrance_help, G_TYPE_DOUBLE, ctk_image_sliders);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DIGITAL_VIBRANCE),
                     G_CALLBACK(scale_value_received),
                     (gpointer) ctk_image_sliders);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_image_sliders->digital_vibrance,
                       TRUE, TRUE, 0);

    /* Overscan Compensation */

    ctk_image_sliders->overscan_compensation =
        add_scale(ctk_config,
                  NV_CTRL_OVERSCAN_COMPENSATION, "Overscan Compensation",
                  __overscan_compensation_help, G_TYPE_INT,
                  ctk_image_sliders);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_OVERSCAN_COMPENSATION),
                     G_CALLBACK(scale_value_received),
                     (gpointer) ctk_image_sliders);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_image_sliders->overscan_compensation,
                       TRUE, TRUE, 0);

    /* Image Sharpening */
    
    ctk_image_sliders->image_sharpening =
        add_scale(ctk_config,
                  NV_CTRL_IMAGE_SHARPENING, "Image Sharpening",
                  __image_sharpening_help, G_TYPE_DOUBLE, ctk_image_sliders);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_IMAGE_SHARPENING),
                     G_CALLBACK(scale_value_received),
                     (gpointer) ctk_image_sliders);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_image_sliders->image_sharpening,
                       TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    /* update the GUI */

    ctk_image_sliders_setup(ctk_image_sliders);

    return GTK_WIDGET(object);
    
} /* ctk_image_sliders_new() */



/*
 * Returns whether or not the scale is active
 */

static gint get_scale_active(CtkScale *scale)
{
    GtkAdjustment *adj = scale->gtk_adjustment;

    return
        GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adj), "attribute active"));

} /* get_scale_active() */



/*
 * add_scale() - if the specified attribute exists and we can
 * query its valid values, create a new scale widget
 */

static GtkWidget * add_scale(CtkConfig *ctk_config,
                             int attribute,
                             char *name,
                             const char *help,
                             gint value_type,
                             gpointer callback_data)
{
    GtkObject *adj;
    GtkWidget *scale;

   
    adj = gtk_adjustment_new(0, 0, 10, 1, 1, 0);
        
    g_object_set_data(G_OBJECT(adj), "attribute",
                      GINT_TO_POINTER(attribute));

    g_object_set_data(G_OBJECT(adj), "attribute name", name);
    
    g_object_set_data(G_OBJECT(adj), "attribute active",
                      GINT_TO_POINTER(0));

    g_signal_connect(G_OBJECT(adj), "value_changed",
                     G_CALLBACK(scale_value_changed),
                     (gpointer) callback_data);
        
    scale = ctk_scale_new(GTK_ADJUSTMENT(adj), name, ctk_config, value_type);
        
    if (help) {
        ctk_config_set_tooltip(ctk_config, CTK_SCALE_TOOLTIP_WIDGET(scale),
                               help);
    }
    
    return scale;

} /* add_scale() */



/*
 * post_scale_value_changed() - helper function for
 * scale_value_changed() and value_changed(); this does whatever
 * work is necessary after the adjustment has been updated --
 * currently, this just means posting a statusbar message.
 */

static void post_scale_value_changed(GtkAdjustment *adjustment,
                                     CtkImageSliders *ctk_image_sliders,
                                     gint value)
{
    char *name = g_object_get_data(G_OBJECT(adjustment), "attribute name");
    
    gtk_widget_set_sensitive(ctk_image_sliders->reset_button, TRUE);

    ctk_config_statusbar_message(ctk_image_sliders->ctk_config,
                                 "%s set to %d.", name, value);
    
} /* post_scale_value_changed() */



/*
 * scale_value_changed() - callback when any of the adjustments
 * in the CtkImageSliders are changed: get the new value from the
 * adjustment, send it to the X server, and do any post-adjustment
 * work.
 */

static void scale_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data)
{
    CtkImageSliders *ctk_image_sliders =
        CTK_IMAGE_SLIDERS(user_data);
    
    gint value;
    gint attribute;
    
    value = (gint) gtk_adjustment_get_value(adjustment);
    
    user_data = g_object_get_data(G_OBJECT(adjustment), "attribute");
    attribute = GPOINTER_TO_INT(user_data);
    
    NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                              ctk_image_sliders->display_device_mask,
                              attribute, (int) value);
    
    post_scale_value_changed(adjustment, ctk_image_sliders, value);
    
} /* scale_value_changed() */



/*
 * ctk_image_sliders_reset() - 
 */

void ctk_image_sliders_reset(CtkImageSliders *ctk_image_sliders)
{
    if (!ctk_image_sliders) return;

    if (get_scale_active(CTK_SCALE(ctk_image_sliders->digital_vibrance))) {
        NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                                  ctk_image_sliders->display_device_mask,
                                  NV_CTRL_DIGITAL_VIBRANCE,
                                  0);
    }

    /*
     * Reset IMAGE_SHARPENING before OVERSCAN_COMPENSATION, even though they're
     * in the other order in the page, because setting OVERSCAN_COMPENSATION to
     * 0 may cause IMAGE_SHARPENING to become unavailable and trying to reset it
     * here would cause a BadValue error.
     */
    if (get_scale_active(CTK_SCALE(ctk_image_sliders->image_sharpening))) {
        NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                                  ctk_image_sliders->display_device_mask,
                                  NV_CTRL_IMAGE_SHARPENING,
                                  ctk_image_sliders->default_val);
    }

    if (get_scale_active(CTK_SCALE(ctk_image_sliders->overscan_compensation))) {
        NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                                  ctk_image_sliders->display_device_mask,
                                  NV_CTRL_OVERSCAN_COMPENSATION,
                                  0);
    }

    /*
     * The above may have triggered events (e.g., changing
     * NV_CTRL_OVERSCAN_COMPENSATION may trigger an
     * NV_CTRL_IMAGE_SHARPENING value change).  Such an event will
     * cause scale_value_changed() and post_scale_value_changed() to
     * be called when control returns to the gtk_main loop.
     * post_scale_value_changed() will write a status message to the
     * statusbar.
     *
     * However, the caller of ctk_image_sliders_reset() (e.g.,
     * ctkdisplaydevice-dfp.c:reset_button_clicked()) may also want to
     * write a status message to the statusbar.  To ensure that the
     * caller's statusbar message takes precedence (i.e., is the last
     * thing written to the statusbar), process any generated events
     * now, before returning to the caller.
     */

    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }

    ctk_image_sliders_setup(ctk_image_sliders);
    
} /* ctk_image_sliders_reset() */



/*
 * scale_value_received() - callback function for changed image settings; this
 * is called when we receive an event indicating that another
 * NV-CONTROL client changed any of the settings that we care about.
 */

static void scale_value_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkImageSliders *ctk_image_sliders =
        CTK_IMAGE_SLIDERS(user_data);
    
    GtkAdjustment *adj;
    GtkWidget *scale;
    gint val;
    

    event_struct = (CtkEventStruct *) arg1;

    /* if the event is not for this display device, return */

    if (!(event_struct->display_mask &
          ctk_image_sliders->display_device_mask)) {
        return;
    }
    
    switch (event_struct->attribute) {
    case NV_CTRL_DIGITAL_VIBRANCE:
        scale = ctk_image_sliders->digital_vibrance;
        break;
    case NV_CTRL_OVERSCAN_COMPENSATION:
        scale = ctk_image_sliders->overscan_compensation;
        break;
    case NV_CTRL_IMAGE_SHARPENING:
        scale = ctk_image_sliders->image_sharpening;
        if (event_struct->availability == FALSE) {
            gtk_widget_set_sensitive(scale, FALSE);
            gtk_widget_hide(scale);
            g_object_set_data(G_OBJECT(CTK_SCALE(scale)->gtk_adjustment),
                              "attribute active",
                              GINT_TO_POINTER(0));
        } else if (event_struct->availability == TRUE) {
            setup_scale(ctk_image_sliders, NV_CTRL_IMAGE_SHARPENING,
                        ctk_image_sliders->image_sharpening);
            gtk_widget_set_sensitive(scale, TRUE);
            gtk_widget_show(scale);
            g_object_set_data(G_OBJECT(CTK_SCALE(scale)->gtk_adjustment),
                              "attribute active",
                              GINT_TO_POINTER(1));
            /* In case of image sharpening slider here we are syncing to the 
             * recent image sharpening value, so updating status bar message */
            post_scale_value_changed(CTK_SCALE(scale)->gtk_adjustment,
                                     ctk_image_sliders,
                                     gtk_adjustment_get_value(
                                             CTK_SCALE(scale)->gtk_adjustment));
        }
        break;
    default:
        return;
    }
    
    adj = CTK_SCALE(scale)->gtk_adjustment;
    val = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

    if (val != event_struct->value) {
        
        val = event_struct->value;

        g_signal_handlers_block_by_func(adj, scale_value_changed,
                                        ctk_image_sliders);
        
        gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), val);
        
        post_scale_value_changed(GTK_ADJUSTMENT(adj),
                                 ctk_image_sliders, val);
        
        g_signal_handlers_unblock_by_func(adj, scale_value_changed,
                                          ctk_image_sliders);
    }

} /* scale_value_received() */



/*
 * add_image_sliders_help() - 
 */

void add_image_sliders_help(CtkImageSliders *ctk_image_sliders,
                            GtkTextBuffer *b,
                            GtkTextIter *i)
{
    ctk_help_heading(b, i, "Digital Vibrance");
    ctk_help_para(b, i, "Digital Vibrance, a mechanism for "
                  "controlling color separation and intensity, boosts "
                  "the color saturation of an image so that all images "
                  "including 2D, 3D, and video appear brighter and "
                  "crisper (even on flat panels) in your applications.");

    ctk_help_heading(b, i, "Overscan Compensation");
    ctk_help_para(b, i, "Use the Overscan Compensation slider to adjust the "
                  "size of the display, to adjust for the overscan behavior of "
                  "certain display devices.");

    ctk_help_heading(b, i, "Image Sharpening");
    ctk_help_para(b, i, "Use the Image Sharpening slider to adjust the "
                  "sharpness of the image quality by amplifying high "
                  "frequency content.");
    
} /* add_image_sliders_help() */



/* Update GUI state of the scale to reflect current settings
 * on the X Driver.
 */

static void setup_scale(CtkImageSliders *ctk_image_sliders,
                        int attribute,
                        GtkWidget *scale)
{
    ReturnStatus ret0, ret1;
    NVCTRLAttributeValidValuesRec valid;
    NvCtrlAttributeHandle *handle = ctk_image_sliders->handle;
    unsigned int mask = ctk_image_sliders->display_device_mask;
    int val;
    GtkAdjustment *adj = CTK_SCALE(scale)->gtk_adjustment;
    

    /* Read settings from X server */
    ret0 = NvCtrlGetValidDisplayAttributeValues(handle, mask,
                                                attribute, &valid);
    
    ret1 = NvCtrlGetDisplayAttribute(handle, mask, attribute, &val);
    
    if ((ret0 == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        (valid.type == ATTRIBUTE_TYPE_RANGE)) {

        g_signal_handlers_block_by_func(adj, scale_value_changed,
                                        ctk_image_sliders);

        adj->lower = valid.u.range.min;
        adj->upper = valid.u.range.max;
        gtk_adjustment_changed(GTK_ADJUSTMENT(adj));

        gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), val);

        g_signal_handlers_unblock_by_func(adj, scale_value_changed,
                                          ctk_image_sliders);

        g_object_set_data(G_OBJECT(adj), "attribute active",
                      GINT_TO_POINTER(1));

        gtk_widget_set_sensitive(scale, TRUE);
        gtk_widget_show(scale);
    } else {
        g_object_set_data(G_OBJECT(adj), "attribute active",
                      GINT_TO_POINTER(0));

        gtk_widget_set_sensitive(scale, FALSE);
        gtk_widget_hide(scale);
    }

} /* setup_scale() */



/*
 * Updates the page to reflect the current configuration of
 * the display device.
 */
void ctk_image_sliders_setup(CtkImageSliders *ctk_image_sliders)
{
    int active;


    if (!ctk_image_sliders) return;

    /* Update sliders */
    
    /* NV_CTRL_DIGITAL_VIBRANCE */
    
    setup_scale(ctk_image_sliders, NV_CTRL_DIGITAL_VIBRANCE,
                ctk_image_sliders->digital_vibrance);

    /* NV_CTRL_OVERSCAN_COMPENSATION */

    setup_scale(ctk_image_sliders, NV_CTRL_OVERSCAN_COMPENSATION,
                ctk_image_sliders->overscan_compensation);

    /* NV_CTRL_IMAGE_SHARPENING */
    
    setup_scale(ctk_image_sliders, NV_CTRL_IMAGE_SHARPENING,
                ctk_image_sliders->image_sharpening);

    active =
        get_scale_active(CTK_SCALE(ctk_image_sliders->digital_vibrance)) ||
        get_scale_active(CTK_SCALE(ctk_image_sliders->overscan_compensation)) ||
        get_scale_active(CTK_SCALE(ctk_image_sliders->image_sharpening));

    if (!active) {
        gtk_widget_hide(ctk_image_sliders->frame);
    } else {
        gtk_widget_show(ctk_image_sliders->frame);
    }

} /* ctk_image_sliders_setup() */
