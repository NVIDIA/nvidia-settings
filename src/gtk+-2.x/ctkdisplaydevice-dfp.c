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

#include "ctkbanner.h"

#include "ctkdisplaydevice-dfp.h"

#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#include <stdio.h>

static void ctk_display_device_dfp_class_init(CtkDisplayDeviceDfpClass *);
static void ctk_display_device_dfp_finalize(GObject *);

static GtkWidget *make_scaling_radio_button(CtkDisplayDeviceDfp
                                            *ctk_display_device_dfp,
                                            GtkWidget *vbox,
                                            GtkWidget *prev_radio,
                                            char *label,
                                            gint value);

static void dfp_scaling_changed(GtkWidget *widget, gpointer user_data);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void
dfp_scaling_update_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                           gint value);


static void dfp_update_received(GtkObject *object, gpointer arg1,
                                gpointer user_data);

static void dfp_info_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp);

static void dfp_scaling_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp);

static void ctk_display_device_dfp_setup(CtkDisplayDeviceDfp
                                         *ctk_display_device_dfp);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data);


#define FRAME_PADDING 5

#define __SCALING (1<<0)


#define GET_SCALING_TARGET(V) ((V) >> 16)
#define GET_SCALING_METHOD(V) ((V) & 0xFFFF)
#define MAKE_SCALING_VALUE(T, M)    (((T) << 16) | ((M) & 0xFFFF))


static const char *__scaling_help =
"A flat panel usually has a single 'native' resolution.  If you are "
"using a resolution that is smaller than the flat panel's native "
"resolution, then Flat Panel Scaling can adjust how the image is "
"displayed on the flat panel.  This setting will only take effect "
"when GPU scaling is active (This occurs when the frontend and "
"backend resolutions of the current mode are different.)";

static const char *__info_help = 
"This section describes basic informations about the "
"DVI connection to the digital flat panel.";

static const char * __native_res_help =
"The Native Resolution is the width and height in pixels that the flat "
"panel uses to display the image.  All other resolutions must be scaled "
"to this resolution by the GPU and/or the DFP's built-in scaler.";

static const char * __best_fit_res_help =
"The Best Fit Resolution is a resolution supported by the DFP that "
"closely matches the frontend resolution.  The Best Fit Resolution "
"is used as the Backend Resolution when you want to let the DFP do "
"the scaling from the Frontend Resolution to the Native Resolution.";

static const char * __frontend_res_help =
"The Frontend Resolution is the current resolution of the image in pixels.";

static const char * __refresh_rate_help =
"The refresh rate displays the rate at which the screen is currently "
"refreshing the image.";

static const char * __backend_res_help =
"The Backend Resolution is the resolution that the GPU is driving to "
"the DFP.  If the Backend Resolution is different than the Frontend "
"Resolution, then the GPU will scale the image from the Frontend "
"Resolution to the Backend Resolution.  If the Backend Resolution "
"is different than the Native Resolution, then the DFP will scale "
"the image from the Backend Resolution to the Native Resolution.  "
"Backend Resolution is either the Native Resolution or the Best "
"Fit Resolution.";

static const char * __force_gpu_scaling_help =
"When set, the driver will make the GPU scale the "
"frontend (current) mode to the flat panel's native "
"resolution.  If disabled, the GPU will only scale (if "
"needed) to the best fitting resolution reported in the flat "
"panel's EDID; the flat panel will then scale the image to "
"its native resolution.";


GType ctk_display_device_dfp_get_type(void)
{
    static GType ctk_display_device_dfp_type = 0;
    
    if (!ctk_display_device_dfp_type) {
        static const GTypeInfo ctk_display_device_dfp_info = {
            sizeof (CtkDisplayDeviceDfpClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_display_device_dfp_class_init,
            NULL, /* class_finalize, */
            NULL, /* class_data */
            sizeof (CtkDisplayDeviceDfp),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_display_device_dfp_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkDisplayDeviceDfp", &ctk_display_device_dfp_info, 0);
    }

    return ctk_display_device_dfp_type;
}

static void ctk_display_device_dfp_class_init(
    CtkDisplayDeviceDfpClass *ctk_display_device_dfp_class
)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_display_device_dfp_class;
    gobject_class->finalize = ctk_display_device_dfp_finalize;
}

static void ctk_display_device_dfp_finalize(
    GObject *object
)
{
    CtkDisplayDeviceDfp *ctk_display_device_dfp = CTK_DISPLAY_DEVICE_DFP(object);
    g_free(ctk_display_device_dfp->name);
    g_signal_handlers_disconnect_matched(ctk_display_device_dfp->ctk_event,
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_display_device_dfp);
}


/*
 * ctk_display_device_dfp_new() - constructor for the DFP display
 * device page.
 */

GtkWidget* ctk_display_device_dfp_new(NvCtrlAttributeHandle *handle,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      unsigned int display_device_mask,
                                      char *name)
{
    GObject *object;
    CtkDisplayDeviceDfp *ctk_display_device_dfp;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *hbox, *vbox, *tmpbox;
    GtkWidget *eventbox;

    GtkWidget *button;
    GtkWidget *radio0;
    GtkWidget *radio1;
    GtkWidget *radio2;
    GtkWidget *alignment;
    
    GtkWidget *table;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE_DFP, NULL);
    if (!object) return NULL;

    ctk_display_device_dfp = CTK_DISPLAY_DEVICE_DFP(object);
    ctk_display_device_dfp->handle = handle;
    ctk_display_device_dfp->ctk_event = ctk_event;
    ctk_display_device_dfp->ctk_config = ctk_config;
    ctk_display_device_dfp->display_device_mask = display_device_mask;
    ctk_display_device_dfp->name = g_strdup(name);

    gtk_box_set_spacing(GTK_BOX(object), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_DFP);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);
    
    /*
     * create the reset button (which we need while creating the
     * controls in this page so that we can set the button's
     * sensitivity), though we pack it at the bottom of the page
     */

    ctk_display_device_dfp->reset_button =
        gtk_button_new_with_label("Reset Hardware Defaults");

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_display_device_dfp->reset_button);
    gtk_box_pack_end(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(ctk_display_device_dfp->reset_button),
                     "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_display_device_dfp);
    
    ctk_config_set_tooltip(ctk_config, ctk_display_device_dfp->reset_button,
                           "The Reset Hardware Defaults button restores "
                           "the DFP settings to their default values.");

    /* create the hbox to store dfp info, scaling */

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);

    /* DFP info */

    frame = gtk_frame_new("Flat Panel Information");
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
    
    /*
     * insert a vbox between the frame and the widgets, so that the
     * widgets don't expand to fill all of the space within the
     * frame
     */
    
    tmpbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), tmpbox);
    
    /* Make the txt widgets that will get updated */
    ctk_display_device_dfp->txt_chip_location = gtk_label_new("");
    ctk_display_device_dfp->txt_link = gtk_label_new("");
    ctk_display_device_dfp->txt_signal = gtk_label_new("");
    ctk_display_device_dfp->txt_native_resolution = gtk_label_new("");
    ctk_display_device_dfp->txt_best_fit_resolution = gtk_label_new("");
    ctk_display_device_dfp->txt_frontend_resolution = gtk_label_new("");
    ctk_display_device_dfp->txt_backend_resolution = gtk_label_new("");
    ctk_display_device_dfp->txt_refresh_rate = gtk_label_new("");

    /* Add information widget lines */
    {
        typedef struct {
            GtkWidget *label;
            GtkWidget *txt;
            const gchar *tooltip;
        } TextLineInfo;

        TextLineInfo lines[] = {
            {
                gtk_label_new("Chip location:"),
                ctk_display_device_dfp->txt_chip_location,
                NULL
            },
            {
                gtk_label_new("Connection link:"),
                ctk_display_device_dfp->txt_link,
                NULL
            },
            {
                gtk_label_new("Signal:"),
                ctk_display_device_dfp->txt_signal,
                NULL
            },
            {
                gtk_label_new("Native Resolution:"),
                ctk_display_device_dfp->txt_native_resolution,
                __native_res_help,
            },
            {
                gtk_label_new("Best Fit Resolution:"),
                ctk_display_device_dfp->txt_best_fit_resolution,
                __best_fit_res_help,
            },
            {
                gtk_label_new("Frontend Resolution:"),
                ctk_display_device_dfp->txt_frontend_resolution,
                __frontend_res_help,
            },
            {
                gtk_label_new("Backend Resolution:"),
                ctk_display_device_dfp->txt_backend_resolution,
                __backend_res_help,
            },
            {
                gtk_label_new("Refresh Rate:"),
                ctk_display_device_dfp->txt_refresh_rate,
                __refresh_rate_help,
            },
            { NULL, NULL, NULL }
        };
        int i;

        GtkRequisition req;
        int max_width;

        /* Compute max width of lables and setup text alignments */
        max_width = 0;
        for (i = 0; lines[i].label; i++) {
            gtk_misc_set_alignment(GTK_MISC(lines[i].label), 0.0f, 0.5f);
            gtk_misc_set_alignment(GTK_MISC(lines[i].txt), 0.0f, 0.5f);

            gtk_widget_size_request(lines[i].label, &req);
            if (max_width < req.width) {
                max_width = req.width;
            }
        }

        /* Pack labels */
        for (i = 0; lines[i].label; i++) {
            GtkWidget *tmphbox;

            /* Add separators */
            if (i == 3 || i == 5 || i == 7) {
                GtkWidget *separator = gtk_hseparator_new();
                gtk_box_pack_start(GTK_BOX(tmpbox), separator,
                                   FALSE, FALSE, 0);
            }

            /* Set the label's width */
            gtk_widget_set_size_request(lines[i].label, max_width, -1);

            /* add the widgets for this line */
            tmphbox = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(tmphbox), lines[i].label,
                               FALSE, TRUE, 5);
            gtk_box_pack_start(GTK_BOX(tmphbox), lines[i].txt,
                               FALSE, TRUE, 5);

            /* Include tooltips */
            if (!lines[i].tooltip) {
                gtk_box_pack_start(GTK_BOX(tmpbox), tmphbox, FALSE, FALSE, 0);
            } else {
                eventbox = gtk_event_box_new();
                gtk_container_add(GTK_CONTAINER(eventbox), tmphbox);
                ctk_config_set_tooltip(ctk_config, eventbox, lines[i].tooltip);
                gtk_box_pack_start(GTK_BOX(tmpbox), eventbox, FALSE, FALSE, 0);
            }
        }
    }

    
    /* Flat Panel Scaling */
    
    frame = gtk_frame_new("Flat Panel Scaling");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), frame);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_display_device_dfp->scaling_frame = frame;
    
    ctk_config_set_tooltip(ctk_config, eventbox, __scaling_help);

    vbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    button = gtk_check_button_new_with_label("Force Full GPU Scaling");
    ctk_display_device_dfp->scaling_gpu_button = button;
    ctk_config_set_tooltip(ctk_config, button, __force_gpu_scaling_help);

    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

    table = gtk_table_new(1, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
    ctk_display_device_dfp->txt_scaling = 
        add_table_row(table, 0,
                      0, 0.5, "Scaling:",
                      0, 0.5,  "");

    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

    frame = gtk_frame_new("GPU Scaling Method");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
    vbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(dfp_scaling_changed),
                     (gpointer) ctk_display_device_dfp);
    
    radio0 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, NULL, "Stretched",
         NV_CTRL_GPU_SCALING_METHOD_STRETCHED);
    
    radio1 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, radio0, "Centered",
         NV_CTRL_GPU_SCALING_METHOD_CENTERED);
    
    radio2 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, radio1, "Aspect Ratio Scaled",
         NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_SCALING),
                     G_CALLBACK(dfp_update_received),
                     (gpointer) ctk_display_device_dfp);

    /* pack the image sliders */
    
    ctk_display_device_dfp->image_sliders =
        ctk_image_sliders_new(handle, ctk_config, ctk_event,
                              ctk_display_device_dfp->reset_button,
                              display_device_mask, name);
    if (ctk_display_device_dfp->image_sliders) {
        gtk_box_pack_start(GTK_BOX(object),
                           ctk_display_device_dfp->image_sliders,
                           FALSE, FALSE, 0);
    }

    /* pack the EDID button */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);
    ctk_display_device_dfp->edid_box = hbox;
    
    /* show the page */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* Update the GUI */

    ctk_display_device_dfp_setup(ctk_display_device_dfp);
    
    /* handle enable/disable events on the display device */

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_received),
                     (gpointer) ctk_display_device_dfp);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_SCALING_ACTIVE),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DFP_SCALING_ACTIVE),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FRONTEND_RESOLUTION),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_BACKEND_RESOLUTION),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_REFRESH_RATE),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_display_device_dfp);

    return GTK_WIDGET(object);

} /* ctk_display_device_dfp_new() */



/*
 * make_scaling_radio_button() - create a radio button and plug it
 * into the scaling radio group.
 */

static GtkWidget *make_scaling_radio_button(CtkDisplayDeviceDfp
                                            *ctk_display_device_dfp,
                                            GtkWidget *vbox,
                                            GtkWidget *prev_radio,
                                            char *label,
                                            gint value)
{
    GtkWidget *radio;
    
    if (prev_radio) {
        radio = gtk_radio_button_new_with_label_from_widget
            (GTK_RADIO_BUTTON(prev_radio), label);
    } else {
        radio = gtk_radio_button_new_with_label(NULL, label);
    }
    
    gtk_box_pack_start(GTK_BOX(vbox), radio, FALSE, FALSE, 0);
 
    g_object_set_data(G_OBJECT(radio), "scaling_value",
                      GINT_TO_POINTER(value));
   
    g_signal_connect(G_OBJECT(radio), "toggled",
                     G_CALLBACK(dfp_scaling_changed),
                     (gpointer) ctk_display_device_dfp);

    ctk_display_device_dfp->scaling_method_buttons[value -1] = radio;

    return radio;
    
} /* make_scaling_radio_button() */



/*
 * post_dfp_scaling_update() - helper function for
 * dfp_scaling_changed() and dfp_update_received(); this does whatever
 * work is necessary after scaling has been updated -- currently, this
 * just means posting a statusbar message.
 */

static void
post_dfp_scaling_update(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                        gint value)
{
    int scaling_target = GET_SCALING_TARGET(value);
    int scaling_method = GET_SCALING_METHOD(value);
    
    static const char *scaling_target_string_table[] = {
        "Best Fit", /* NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_BEST_FIT */
        "Native",   /* NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_NATIVE */
    };

    static const char *scaling_method_string_table[] = {
        "Stretched",           /* NV_CTRL_GPU_SCALING_METHOD_STRETCHED */
        "Centered",            /* NV_CTRL_GPU_SCALING_METHOD_CENTERED */
        "Aspect Ratio Scaled"  /* NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED */
    };

    if ((scaling_target < NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_BEST_FIT) ||
        (scaling_target > NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_NATIVE)) return;

    if ((scaling_method < NV_CTRL_GPU_SCALING_METHOD_STRETCHED) ||
        (scaling_method > NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED)) return;
    
    ctk_config_statusbar_message(ctk_display_device_dfp->ctk_config,
                                 "Set Flat Panel Scaling for %s to %s %s.",
                                 ctk_display_device_dfp->name,
                                 scaling_method_string_table[scaling_method -1],
                                 scaling_target_string_table[scaling_target -1]);
    
} /* post_dfp_scaling_update() */



/*
 * dfp_scaling_changed() - callback function for changes to the
 * scaling target and method buttons.
 */

static void dfp_scaling_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_display_device_dfp =
        CTK_DISPLAY_DEVICE_DFP(user_data);
    gboolean enabled;
    int scaling_target;
    int scaling_method;
    gint value;
    int i;
    GtkWidget *radio;

    /* Get the scaling target */

    enabled = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_display_device_dfp->scaling_gpu_button));

    if (enabled) {
        scaling_target = NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_NATIVE;
    } else {
        scaling_target = NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_BEST_FIT;
    }

    /* Get the scaling method */

    scaling_method = NV_CTRL_GPU_SCALING_METHOD_INVALID;

    for (i = 0; i < NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED; i++) {
        radio = ctk_display_device_dfp->scaling_method_buttons[i];
        
        if (!radio) continue;

        enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio));

        if (enabled) {
            user_data = g_object_get_data(G_OBJECT(radio), "scaling_value");
            scaling_method = GPOINTER_TO_INT(user_data);
            break;
        }
    }

    if (scaling_method == NV_CTRL_GPU_SCALING_METHOD_INVALID) {
        return;
    }
    
    value = MAKE_SCALING_VALUE(scaling_target, scaling_method);
        
    NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                              ctk_display_device_dfp->display_device_mask,
                              NV_CTRL_GPU_SCALING, value);
    
    gtk_widget_set_sensitive(ctk_display_device_dfp->reset_button, TRUE);

    post_dfp_scaling_update(ctk_display_device_dfp, value);

} /* dfp_scaling_changed() */



/*
 * reset_button_clicked() - callback when the reset button is clicked
 */

static void reset_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_display_device_dfp =
        CTK_DISPLAY_DEVICE_DFP(user_data);

    gint value;
    
    ctk_image_sliders_reset
        (CTK_IMAGE_SLIDERS(ctk_display_device_dfp->image_sliders));
 
    /*
     * if scaling is active, send the default scaling value to the
     * server and update the radio button group
     */
   
    if (ctk_display_device_dfp->active_attributes & __SCALING) {
        
        value =
            MAKE_SCALING_VALUE(NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_BEST_FIT,
                               NV_CTRL_GPU_SCALING_METHOD_STRETCHED);

        NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_GPU_SCALING, value);

        dfp_scaling_update_buttons(ctk_display_device_dfp, value);
    }
    
    /* Update the reset button */

    gtk_widget_set_sensitive(ctk_display_device_dfp->reset_button, FALSE);

    /* status bar message */

    ctk_config_statusbar_message(ctk_display_device_dfp->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_display_device_dfp->name);
    
} /* reset_button_clicked() */



/*
 * dfp_scaling_update_buttons() - update the GUI state of the scaling button
 * group, making the specified scaling value active.
 */

static void
dfp_scaling_update_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                           gint value)
{
    GtkWidget *b, *button = NULL;
    int scaling_target = GET_SCALING_TARGET(value);
    int scaling_method = GET_SCALING_METHOD(value);
    gboolean enabled;
    int i;

    if ((scaling_target < NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_BEST_FIT) ||
        (scaling_target > NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_NATIVE))
        return;

    if ((scaling_method < NV_CTRL_GPU_SCALING_METHOD_STRETCHED) ||
        (scaling_method > NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED))
        return;

    if (scaling_target == NV_CTRL_GPU_SCALING_TARGET_FLATPANEL_NATIVE) {
        enabled = TRUE;
    } else {
        enabled = FALSE;
    }

    button = ctk_display_device_dfp->scaling_method_buttons[scaling_method -1];
    
    if (!button) return;

    /* turn off signal handling for all the scaling buttons */

    for (i = 0; i < NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED; i++) {
        b = ctk_display_device_dfp->scaling_method_buttons[i];
        if (!b) continue;

        g_signal_handlers_block_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_scaling_changed),
             (gpointer) ctk_display_device_dfp);
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_display_device_dfp->scaling_gpu_button),
         G_CALLBACK(dfp_scaling_changed),
         (gpointer) ctk_display_device_dfp);

    /* set the appropriate button active */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_display_device_dfp->scaling_gpu_button),
         enabled);
    
    /* turn on signal handling for all the scaling buttons */

    for (i = 0; i < NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED; i++) {
        b = ctk_display_device_dfp->scaling_method_buttons[i];
        if (!b) continue;

        g_signal_handlers_unblock_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_scaling_changed),
             (gpointer) ctk_display_device_dfp);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_display_device_dfp->scaling_gpu_button),
         G_CALLBACK(dfp_scaling_changed),
         (gpointer) ctk_display_device_dfp);

} /* dfp_scaling_update_buttons() */



/*
 * dfp_update_received() - callback function for changed DFP
 * settings; this is called when we receive an event indicating that
 * another NV-CONTROL client changed any of the settings that we care
 * about.
 */

static void dfp_update_received(GtkObject *object, gpointer arg1,
                                gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkDisplayDeviceDfp *ctk_display_device_dfp =
        CTK_DISPLAY_DEVICE_DFP(user_data);
    
    /* if the event is not for this display device, return */
    
    if (!(event_struct->display_mask &
          ctk_display_device_dfp->display_device_mask)) {
        return;
    }
    
    switch (event_struct->attribute) {
    case NV_CTRL_GPU_SCALING:
        dfp_scaling_update_buttons(ctk_display_device_dfp,
                                   event_struct->value);
        post_dfp_scaling_update(ctk_display_device_dfp, event_struct->value);
        break;
        
    default:
        break;
    }
    
} /* dfp_update_received() */



/*
 * ctk_display_device_dfp_create_help() - construct the DFP display
 * device help page
 */

GtkTextBuffer *ctk_display_device_dfp_create_help(GtkTextTagTable *table,
                                                  CtkDisplayDeviceDfp
                                                  *ctk_display_device_dfp)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);
    
    ctk_help_title(b, &i, "%s Help", ctk_display_device_dfp->name);
    
    ctk_help_heading(b, &i, "Flat Panel Information");
    ctk_help_para(b, &i, __info_help);
        
    ctk_help_term(b, &i, "Chip Location");
    ctk_help_para(b, &i, "Report whether the flat panel is driven by "
                  "the on-chip controller (internal), or a "
                  " separate controller chip elsewhere on the "
                  "graphics board (external)");
                      
    ctk_help_term(b, &i, "Link");
    ctk_help_para(b, &i, "Report whether the specified display device "
                  "is driven by a single link or dual link DVI "
                  "connection.");
    
    ctk_help_term(b, &i, "Signal");
    ctk_help_para(b, &i, "Report whether the flat panel is driven by "
                  "an LVDS, TMDS, or DisplayPort signal");

    ctk_help_term(b, &i, "Native Resolution");
    ctk_help_para(b, &i, __native_res_help);

    ctk_help_term(b, &i, "Best Fit Resolution");
    ctk_help_para(b, &i, __best_fit_res_help);

    ctk_help_term(b, &i, "Frontend Resolution");
    ctk_help_para(b, &i, __frontend_res_help);

    ctk_help_term(b, &i, "Backend Resolution");
    ctk_help_para(b, &i, __backend_res_help);
    
    ctk_help_term(b, &i, "Refresh Rate");
    ctk_help_para(b, &i, __refresh_rate_help);

    ctk_help_heading(b, &i, "Flat Panel Scaling");
    ctk_help_para(b, &i, __scaling_help);
    
    ctk_help_term(b, &i, "Force Full GPU Scaling");
    ctk_help_para(b, &i, __force_gpu_scaling_help);
    
    ctk_help_term(b, &i, "Scaling");
    ctk_help_para(b, &i, "Reports whether the GPU and/or DFP are actively "
                  "scaling the current resolution.");
    
    ctk_help_term(b, &i, "Stretched");
    ctk_help_para(b, &i, "The image will be expanded to fit the entire "
                  "flat panel.");
    
    ctk_help_term(b, &i, "Centered");
    ctk_help_para(b, &i, "The image will only occupy the number of pixels "
                  "needed and be centered on the flat panel.");
    
    ctk_help_term(b, &i, "Aspect Ratio Scaled");
    ctk_help_para(b, &i, "The image will be scaled (retaining the original "
                  "aspect ratio) to expand and fit as much of the entire "
                  "flat panel as possible.");

    add_image_sliders_help
        (CTK_IMAGE_SLIDERS(ctk_display_device_dfp->image_sliders), b, &i);
    
    if (ctk_display_device_dfp->edid) {
        add_acquire_edid_help(b, &i);
    }

    ctk_help_finish(b);
    
    return b;
    
} /* ctk_display_device_dfp_create_help() */



/*
 * dfp_info_setup() -
 *
 *
 */
static void dfp_info_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp)
{
    ReturnStatus ret;
    gint val, signal_type, gpu_scaling, dfp_scaling;
    char *chip_location, *link, *signal;
    char *scaling;
    char tmp[32];

    chip_location = link = signal = "Unknown";
    scaling = "Unknown";

    /* Chip location */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_CHIP_LOCATION, &val);
    if (ret == NvCtrlSuccess) {
        switch (val) {
        case NV_CTRL_FLATPANEL_CHIP_LOCATION_INTERNAL:
            chip_location = "Internal";
            break;
        case NV_CTRL_FLATPANEL_CHIP_LOCATION_EXTERNAL:
            chip_location = "External";
            break;
        }
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_chip_location), chip_location);

    /* Signal */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_SIGNAL, &val);
    if (ret == NvCtrlSuccess) {
        switch (val) {
        case NV_CTRL_FLATPANEL_SIGNAL_LVDS:
            signal = "LVDS";
            break;
        case NV_CTRL_FLATPANEL_SIGNAL_TMDS:
            signal = "TMDS";
            break;
        case NV_CTRL_FLATPANEL_SIGNAL_DISPLAYPORT:
            signal = "DisplayPort";
            break;
        }
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_signal), signal);
    signal_type = val;

    /* Link */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_LINK, &val);
    if (ret == NvCtrlSuccess) {
        if (signal_type == NV_CTRL_FLATPANEL_SIGNAL_DISPLAYPORT) {
            int lanes;

            lanes = val + 1;

            ret =
                NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                          ctk_display_device_dfp->display_device_mask,
                                          NV_CTRL_DISPLAYPORT_LINK_RATE, &val);
            if (ret == NvCtrlSuccess && val == NV_CTRL_DISPLAYPORT_LINK_RATE_DISABLED) {
                link = "Disabled";
            } else {
                char *bw = "unknown bandwidth";

                if (ret == NvCtrlSuccess) {
                    switch (val) {
                    case NV_CTRL_DISPLAYPORT_LINK_RATE_1_62GBPS:
                        bw = "1.62 Gbps";
                        break;
                    case NV_CTRL_DISPLAYPORT_LINK_RATE_2_70GBPS:
                        bw = "2.70 Gbps";
                        break;
                    }
                }

                snprintf(tmp, 32, "%d lane%s @ %s", lanes, lanes == 1 ? "" : "s",
                         bw);
                link = tmp;
            }
        } else {
            // LVDS or TMDS
            switch(val) {
            case NV_CTRL_FLATPANEL_LINK_SINGLE:
                link = "Single";
                break;
            case NV_CTRL_FLATPANEL_LINK_DUAL:
                link = "Dual";
                break;
            }
        }
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_link), link);


    /* Native Resolution */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_NATIVE_RESOLUTION, &val);
    if (ret == NvCtrlSuccess) {
        gchar *resolution =
            g_strdup_printf("%dx%d", (val >> 16), (val & 0xFFFF));
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_native_resolution),
             resolution);
        g_free(resolution);
    } else {
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_native_resolution),
             "Unknown");
    }

    /* Frontend Resolution */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FRONTEND_RESOLUTION, &val);
    if (ret == NvCtrlSuccess) {
        gchar *resolution =
            g_strdup_printf("%dx%d", (val >> 16), (val & 0xFFFF));
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_frontend_resolution),
             resolution);
        g_free(resolution);
    } else {
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_frontend_resolution),
             "Unknown");
    }

    /* Best Fit Resolution */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION, &val);
    if (ret == NvCtrlSuccess) {
        gchar *resolution =
            g_strdup_printf("%dx%d", (val >> 16), (val & 0xFFFF));
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_best_fit_resolution),
             resolution);
        g_free(resolution);
    } else {
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_best_fit_resolution),
             "Unknown");
    }

    /* Backend Resolution */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_BACKEND_RESOLUTION, &val);
    if (ret == NvCtrlSuccess) {
        gchar *resolution =
            g_strdup_printf("%dx%d", (val >> 16), (val & 0xFFFF));
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_backend_resolution),
             resolution);
        g_free(resolution);
    } else {
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_backend_resolution),
             "Unknown");
    }
    /* Refresh Rate */
     
    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_REFRESH_RATE, &val);
    if (ret == NvCtrlSuccess) {
        char str[32];
        float fvalue = ((float)(val)) / 100.0f;
        snprintf(str, 32, "%.2f Hz", fvalue);
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_refresh_rate),
             str);
    } else {
        gtk_label_set_text
            (GTK_LABEL(ctk_display_device_dfp->txt_refresh_rate),
             "Unknown");
   
    }

    /* GPU/DFP Scaling */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_GPU_SCALING_ACTIVE,
                                  &gpu_scaling);
    if (ret == NvCtrlSuccess) {
        ret = NvCtrlGetDisplayAttribute
            (ctk_display_device_dfp->handle,
             ctk_display_device_dfp->display_device_mask,
             NV_CTRL_DFP_SCALING_ACTIVE, &dfp_scaling);
    }
    if (ret != NvCtrlSuccess) {
        scaling = "Unknown";
    } else {
        if (gpu_scaling && dfp_scaling) {
            scaling = "GPU & DFP";
        } else if (gpu_scaling) {
            scaling = "GPU";
        } else if (dfp_scaling) {
            scaling = "DFP";
        } else {
            scaling = "None";
        }
    }

    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_scaling), scaling);

} /* dfp_info_setup() */



/*
 * dfp_scaling_setup() - Update GUI to reflect X server settings of
 * DFP Scaling.
 */
static void dfp_scaling_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp)
{
    ReturnStatus ret;
    int val;

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_GPU_SCALING, &val);
    if (ret != NvCtrlSuccess) {
        gtk_widget_set_sensitive(ctk_display_device_dfp->scaling_frame, FALSE);
        gtk_widget_hide(ctk_display_device_dfp->scaling_frame);
        ctk_display_device_dfp->active_attributes &= ~__SCALING;
        return;
    }

    gtk_widget_show(ctk_display_device_dfp->scaling_frame);
    ctk_display_device_dfp->active_attributes |= __SCALING;

    gtk_widget_set_sensitive(ctk_display_device_dfp->scaling_frame, TRUE);

    dfp_scaling_update_buttons(ctk_display_device_dfp, val);

} /* dfp_scaling_setup() */



/*
 * Updates the display device page to reflect the current
 * configuration of the display device.
 */
static void ctk_display_device_dfp_setup(CtkDisplayDeviceDfp
                                         *ctk_display_device_dfp)
{
    ReturnStatus ret;
    unsigned int enabled_displays;


    /* Is display enabled? */

    ret = NvCtrlGetAttribute(ctk_display_device_dfp->handle,
                             NV_CTRL_ENABLED_DISPLAYS,
                             (int *)&enabled_displays);

    ctk_display_device_dfp->display_enabled =
        (ret == NvCtrlSuccess &&
         (enabled_displays & (ctk_display_device_dfp->display_device_mask)));


    /* Update DFP-specific settings */

    dfp_info_setup(ctk_display_device_dfp);

    dfp_scaling_setup(ctk_display_device_dfp);


    /* Update the image sliders */

    ctk_image_sliders_setup
        (CTK_IMAGE_SLIDERS(ctk_display_device_dfp->image_sliders));


    /* update acquire EDID button */
    
    if (ctk_display_device_dfp->edid) {
            GList *list;
            
            list = gtk_container_get_children
                (GTK_CONTAINER(ctk_display_device_dfp->edid_box));
            if (list) {
                gtk_container_remove
                    (GTK_CONTAINER(ctk_display_device_dfp->edid_box),
                     (GtkWidget *)(list->data));
                g_list_free(list);
            }
    }

    ctk_display_device_dfp->edid =
        ctk_edid_new(ctk_display_device_dfp->handle,
                     ctk_display_device_dfp->ctk_config,
                     ctk_display_device_dfp->ctk_event,
                     ctk_display_device_dfp->reset_button,
                     ctk_display_device_dfp->display_device_mask,
                     ctk_display_device_dfp->name);

    if (ctk_display_device_dfp->edid) {
        gtk_box_pack_start(GTK_BOX(ctk_display_device_dfp->edid_box),
                           ctk_display_device_dfp->edid, TRUE, TRUE, 0);
    }


    /* update the reset button */

    gtk_widget_set_sensitive(ctk_display_device_dfp->reset_button, FALSE);

} /* ctk_display_device_dfp_setup() */



/*
 * When the list of enabled displays on the GPU changes,
 * this page should disable/enable access based on whether
 * or not the display device is enabled.
 */
static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_object = CTK_DISPLAY_DEVICE_DFP(user_data);

    ctk_display_device_dfp_setup(ctk_object);

} /* enabled_displays_received() */


/*
 * When DFP/GPU scaling activation and/or resolution changes occur,
 * we should update the GUI to reflect the current state.
 */
static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_object = CTK_DISPLAY_DEVICE_DFP(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;

    /* if the event is not for this display device, return */

    if (!(event_struct->display_mask & ctk_object->display_device_mask)) {
        return;
    }

    dfp_info_setup(ctk_object);
}
