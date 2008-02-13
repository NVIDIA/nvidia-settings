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

#include "dfp_banner.h"
#include "ctkimage.h"

#include "ctkdisplaydevice-dfp.h"

#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"


static void ctk_display_device_dfp_class_init(CtkDisplayDeviceDfpClass *);
static void ctk_display_device_dfp_finalize(GObject *);

static GtkWidget *make_scaling_radio_button(CtkDisplayDeviceDfp
                                            *ctk_display_device_dfp,
                                            GtkWidget *vbox,
                                            GtkWidget *prev_radio,
                                            char *label,
                                            gint value);

static GtkWidget *make_dithering_radio_button(CtkDisplayDeviceDfp
                                              *ctk_display_device_dfp,
                                              GtkWidget *vbox,
                                              GtkWidget *prev_radio,
                                              char *label,
                                              gint value);

static void dfp_scaling_changed(GtkWidget *widget, gpointer user_data);

static void dfp_dithering_changed(GtkWidget *widget, gpointer user_data);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void
dfp_scaling_update_radio_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                                 gint value);


static void
dfp_dithering_update_radio_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                                   gint value);

static void dfp_update_received(GtkObject *object, gpointer arg1,
                                gpointer user_data);

static void dfp_info_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp);

static void dfp_scaling_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp);

static void dfp_dithering_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp);

static void ctk_display_device_dfp_setup(CtkDisplayDeviceDfp
                                         *ctk_display_device_dfp);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);


#define FRAME_PADDING 5

#define __SCALING (1<<0)
#define __DITHERING (1<<1)


static const char *__scaling_help =
"A FlatPanel usually has a single 'native' "
"resolution.  If you are using a resolution that is "
"smaller than the FlatPanel's native resolution, then "
"FlatPanel Scaling can adjust how the image is "
"displayed on the FlatPanel.";

static const char *__dithering_help =
"Some GeForce2 GPUs required dithering to "
"properly display on a flatpanel; this option allows "
"you to control the dithering behavior.";

static const char *__info_help = 
"This section describes basic informations about the "
"DVI connection to the digital flat panel.";


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

    GtkWidget *radio0;
    GtkWidget *radio1;
    GtkWidget *radio2;
    GtkWidget *radio3;
    GtkWidget *alignment;
    
    GtkWidget *table;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE_DFP, NULL);

    ctk_display_device_dfp = CTK_DISPLAY_DEVICE_DFP(object);
    ctk_display_device_dfp->handle = handle;
    ctk_display_device_dfp->ctk_config = ctk_config;
    ctk_display_device_dfp->display_device_mask = display_device_mask;
    ctk_display_device_dfp->name = g_strdup(name);

    gtk_box_set_spacing(GTK_BOX(object), 10);

    /* banner */

    banner = ctk_banner_image_new(&dfp_banner_image);
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

    /* create the hbox to store dfp info, scaling and dithering */

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);

    /* DFP info */

    frame = gtk_frame_new("Flat Panel Information");
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(3, 2, FALSE);
    
    /*
     * insert a vbox between the frame and the table, so that the
     * table doesn't expand to fill all of the space within the
     * frame
     */
    
    tmpbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tmpbox), table, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(frame), tmpbox);
    
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    
    ctk_display_device_dfp->txt_chip_location = 
        add_table_row(table, 0,
                      0, 0.5, "Chip location:",
                      0, 0.5,  "");

    ctk_display_device_dfp->txt_link = 
        add_table_row(table, 1,
                      0, 0.5, "DVI connection link:",
                      0, 0.5,  "");

    ctk_display_device_dfp->txt_signal = 
        add_table_row(table, 2,
                      0, 0.5, "Signal:",
                      0, 0.5,  "");
    
    /* FlatPanel Scaling */
    
    frame = gtk_frame_new("FlatPanel Scaling");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), frame);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_display_device_dfp->scaling_frame = frame;
    
    ctk_config_set_tooltip(ctk_config, eventbox, __scaling_help);
    
    vbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    radio0 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, NULL, "Default",
         NV_CTRL_FLATPANEL_SCALING_DEFAULT);
    
    radio1 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, radio0, "Scaled",
         NV_CTRL_FLATPANEL_SCALING_SCALED);
    
    radio2 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, radio1, "Centered",
         NV_CTRL_FLATPANEL_SCALING_CENTERED);
    
    radio3 = make_scaling_radio_button
        (ctk_display_device_dfp, vbox, radio2, "Fixed Aspect Ratio Scaled",
         NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED);
    
    /*
     * XXX TODO: determine when we should advertise Monitor
     * Scaling (aka "Native" scaling)
     */
    
    ctk_display_device_dfp->scaling_buttons
        [NV_CTRL_FLATPANEL_SCALING_NATIVE] = NULL;
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_SCALING),
                     G_CALLBACK(dfp_update_received),
                     (gpointer) ctk_display_device_dfp);

    /* FlatPanel Dithering */

    frame = gtk_frame_new("FlatPanel Dithering");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), frame);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, TRUE, TRUE, 0);
    ctk_display_device_dfp->dithering_frame = frame;
    
    ctk_config_set_tooltip(ctk_config, eventbox, __dithering_help);
    
    vbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    
    radio0 = make_dithering_radio_button
        (ctk_display_device_dfp, vbox, NULL, "Default",
         NV_CTRL_FLATPANEL_DITHERING_DEFAULT);
    
    radio1 = make_dithering_radio_button
        (ctk_display_device_dfp, vbox, radio0, "Enabled",
         NV_CTRL_FLATPANEL_DITHERING_ENABLED);
    
    radio2 = make_dithering_radio_button
        (ctk_display_device_dfp, vbox, radio1, "Disabled",
         NV_CTRL_FLATPANEL_DITHERING_DISABLED);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_DITHERING),
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

    ctk_display_device_dfp->scaling_buttons[value] = radio;

    return radio;
    
} /* make_scaling_radio_button() */



/*
 * make_dithering_radio_button() - create a radio button and plug it
 * into the dithering radio group.
 */

static GtkWidget *make_dithering_radio_button(CtkDisplayDeviceDfp
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
    
    g_object_set_data(G_OBJECT(radio), "dithering_value",
                      GINT_TO_POINTER(value));
   
    g_signal_connect(G_OBJECT(radio), "toggled",
                     G_CALLBACK(dfp_dithering_changed),
                     (gpointer) ctk_display_device_dfp);

    ctk_display_device_dfp->dithering_buttons[value] = radio;

    return radio;
    
} /* make_dithering_radio_button() */



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
    static const char *scaling_string_table[] = {
        "Default",        /* NV_CTRL_FLATPANEL_SCALING_DEFAULT */
        "Monitor Scaled", /* NV_CTRL_FLATPANEL_SCALING_NATIVE */
        "Scaled",         /* NV_CTRL_FLATPANEL_SCALING_SCALED */
        "Centered",       /* NV_CTRL_FLATPANEL_SCALING_CENTERED */
        "Aspect Scaled"   /* NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED */
    };
        
    if (value > NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED) return;
    
    ctk_config_statusbar_message(ctk_display_device_dfp->ctk_config,
                                 "Set FlatPanel Scaling for %s to %s.",
                                 ctk_display_device_dfp->name,
                                 scaling_string_table[value]);
    
} /* post_dfp_scaling_update() */



/*
 * dfp_scaling_changed() - callback function for changes to the
 * scaling radio button group; if the specified radio button is
 * active, send updated state to the server
 */

static void dfp_scaling_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_display_device_dfp =
        CTK_DISPLAY_DEVICE_DFP(user_data);
    gboolean enabled;
    gint value;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {

        user_data = g_object_get_data(G_OBJECT(widget), "scaling_value");
        value = GPOINTER_TO_INT(user_data);
        
        NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_SCALING, value);
        
        post_dfp_scaling_update(ctk_display_device_dfp, value);
    }

} /* dfp_scaling_changed() */



/*
 * post_dfp_dithering_update() - helper function for
 * dfp_dithering_changed() and dfp_update_received(); this does
 * whatever work is necessary after dithering has been updated --
 * currently, this just means posting a statusbar message.
 */

static void
post_dfp_dithering_update(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                          gint value)
{
    static const char *dithering_string_table[] = {
        "Default", /* NV_CTRL_FLATPANEL_DITHERING_DEFAULT */
        "Enabled", /* NV_CTRL_FLATPANEL_DITHERING_ENABLED */
        "Disabled" /* NV_CTRL_FLATPANEL_DITHERING_DISABLED */
    };

    if (value > NV_CTRL_FLATPANEL_DITHERING_DISABLED) return;
    
    ctk_config_statusbar_message(ctk_display_device_dfp->ctk_config,
                                 "Set FlatPanel Dithering for %s to %s.",
                                 ctk_display_device_dfp->name,
                                 dithering_string_table[value]);
    
} /* post_dfp_dithering_update() */



/*
 * dfp_dithering_changed() - callback function for changes to the
 * dithering radio button group; if the specified radio button is
 * active, send updated state to the server
 */

static void dfp_dithering_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayDeviceDfp *ctk_display_device_dfp =
        CTK_DISPLAY_DEVICE_DFP(user_data);
    
    gboolean enabled;
    gint value;
    
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    if (enabled) {
        
        user_data = g_object_get_data(G_OBJECT(widget), "dithering_value");
        value = GPOINTER_TO_INT(user_data);
        
        NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING, value);
        
        post_dfp_dithering_update(ctk_display_device_dfp, value);
    }
    
} /* dfp_dithering_changed() */



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
        
        value = NV_CTRL_FLATPANEL_SCALING_DEFAULT;

        NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_SCALING, value);

        dfp_scaling_update_radio_buttons(ctk_display_device_dfp, value);
    }
    
    /*
     * if dithering is active, send the default dithering value to the
     * server and update the radio button group
     */
    
    if (ctk_display_device_dfp->active_attributes & __DITHERING) {
    
        value = NV_CTRL_FLATPANEL_DITHERING_DEFAULT;

        NvCtrlSetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING, value);

        dfp_dithering_update_radio_buttons(ctk_display_device_dfp, value);
    }
    
    /* Update the reset button */

    gtk_widget_set_sensitive(ctk_display_device_dfp->reset_button, FALSE);

    /* status bar message */

    ctk_config_statusbar_message(ctk_display_device_dfp->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_display_device_dfp->name);
    
} /* reset_button_clicked() */



/*
 * dfp_scaling_update_radio_buttons() - update the scaling radio
 * button group, making the specified scaling value active.
 */

static void
dfp_scaling_update_radio_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                                 gint value)
{
    GtkWidget *b, *button = NULL;
    int i;

    if ((value < NV_CTRL_FLATPANEL_SCALING_DEFAULT) ||
        (value > NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED)) return;

    button = ctk_display_device_dfp->scaling_buttons[value];
    
    if (!button) return;

    /* turn off signal handling for all the scaling buttons */

    for (i = 0; i < 5; i++) {
        b = ctk_display_device_dfp->scaling_buttons[i];
        if (!b) continue;

        g_signal_handlers_block_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_scaling_changed),
             (gpointer) ctk_display_device_dfp);
    }
    
    /* set the appropriate button active */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
    
    /* turn on signal handling for all the scaling buttons */

    for (i = 0; i < 5; i++) {
        b = ctk_display_device_dfp->scaling_buttons[i];
        if (!b) continue;

        g_signal_handlers_unblock_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_scaling_changed),
             (gpointer) ctk_display_device_dfp);
    }
    
} /* dfp_scaling_update_radio_buttons() */



/*
 * dfp_dithering_update_radio_buttons() - update the dithering radio
 * button group, making the specified dithering value active.
 */

static void
dfp_dithering_update_radio_buttons(CtkDisplayDeviceDfp *ctk_display_device_dfp,
                                   gint value)
{
    GtkWidget *b, *button = NULL;
    int i;

    if ((value < NV_CTRL_FLATPANEL_DITHERING_DEFAULT) ||
        (value > NV_CTRL_FLATPANEL_DITHERING_DISABLED)) return;

    button = ctk_display_device_dfp->dithering_buttons[value];
    
    if (!button) return;
    
    /* turn off signal handling for all the dithering buttons */

    for (i = 0; i < 3; i++) {
        b = ctk_display_device_dfp->dithering_buttons[i];
        if (!b) continue;
        
        g_signal_handlers_block_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_dithering_changed),
             (gpointer) ctk_display_device_dfp);
    }
    
    /* set the appropriate button active */

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

    /* turn on signal handling for all the dithering buttons */

    for (i = 0; i < 3; i++) {
        b = ctk_display_device_dfp->dithering_buttons[i];
        if (!b) continue;
        
        g_signal_handlers_unblock_by_func
            (G_OBJECT(b), G_CALLBACK(dfp_dithering_changed),
             (gpointer) ctk_display_device_dfp);
    }
} /* dfp_dithering_update_radio_buttons() */



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
    case NV_CTRL_FLATPANEL_SCALING:
        dfp_scaling_update_radio_buttons(ctk_display_device_dfp,
                                         event_struct->value);
        post_dfp_scaling_update(ctk_display_device_dfp, event_struct->value);
        break;
        
    case NV_CTRL_FLATPANEL_DITHERING:
        dfp_dithering_update_radio_buttons(ctk_display_device_dfp,
                                           event_struct->value);
        post_dfp_dithering_update(ctk_display_device_dfp, event_struct->value);
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
    
    ctk_help_heading(b, &i, "FlatPanel Information");
    ctk_help_para(b, &i, __info_help);
        
    ctk_help_term(b, &i, "Chip Location");
    ctk_help_para(b, &i, "Report whether the flatpanel is driven by "
                  "the on-chip controller (internal), or a "
                  " separate controller chip elsewhere on the "
                  "graphics board (external)");
                      
    ctk_help_term(b, &i, "Link");
    ctk_help_para(b, &i, "Report whether the specified display device "
                  "is driven by a single link or dual link DVI "
                  "connection.");
    
    ctk_help_term(b, &i, "Signal");
    ctk_help_para(b, &i, "Report whether the flatpanel is driven by "
                  "an LVDS or TMDS signal");
    
    ctk_help_heading(b, &i, "FlatPanel Scaling");
    ctk_help_para(b, &i, __scaling_help);
    
    ctk_help_term(b, &i, "Default");
    ctk_help_para(b, &i, "The driver will choose what scaling state is "
                  "best.");
    
    ctk_help_term(b, &i, "Scaled");
    ctk_help_para(b, &i, "The image will be expanded to fit the entire "
                  "FlatPanel.");
    
    ctk_help_term(b, &i, "Centered");
    ctk_help_para(b, &i, "The image will only occupy the number of pixels "
                  "needed and be centered on the FlatPanel.");
    
    ctk_help_term(b, &i, "Fixed Aspect Ratio Scaled");
    ctk_help_para(b, &i, "The image will be expanded (like when Scaled), "
                  "but the image will retain the original aspect ratio.");
    
    ctk_help_heading(b, &i, "FlatPanel Dithering");
    ctk_help_para(b, &i, __dithering_help);
    
    ctk_help_term(b, &i, "Default");
    ctk_help_para(b, &i, "The driver will choose when to dither.");
    
    ctk_help_term(b, &i, "Enabled");
    ctk_help_para(b, &i, "Force dithering on.");
    
    ctk_help_term(b, &i, "Disabled");
    ctk_help_para(b, &i, "Force dithering off.");

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
    gint val;
    char *chip_location, *link, *signal;

    chip_location = link = signal = "Unknown";

    /* Chip location */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_CHIP_LOCATION, &val);
    if (ret == NvCtrlSuccess) {
        if (val == NV_CTRL_FLATPANEL_CHIP_LOCATION_INTERNAL)
            chip_location = "Internal";
        if (val == NV_CTRL_FLATPANEL_CHIP_LOCATION_EXTERNAL)
            chip_location = "External";
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_chip_location), chip_location);

    /* Link */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_LINK, &val);
    if (ret == NvCtrlSuccess) {
        if (val == NV_CTRL_FLATPANEL_LINK_SINGLE) link = "Single";
        if (val == NV_CTRL_FLATPANEL_LINK_DUAL) link = "Dual";
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_link), link);

    /* Signal */

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_SIGNAL, &val);
    if (ret == NvCtrlSuccess) {
        if (val == NV_CTRL_FLATPANEL_SIGNAL_LVDS) signal = "LVDS";
        if (val == NV_CTRL_FLATPANEL_SIGNAL_TMDS) signal = "TMDS";
    }
    gtk_label_set_text
        (GTK_LABEL(ctk_display_device_dfp->txt_signal), signal);

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
                                  NV_CTRL_FLATPANEL_SCALING, &val);
    if (ret != NvCtrlSuccess) {
        gtk_widget_set_sensitive(ctk_display_device_dfp->scaling_frame, FALSE);
        gtk_widget_hide(ctk_display_device_dfp->scaling_frame);
        ctk_display_device_dfp->active_attributes &= ~__SCALING;
        return;
    }

    gtk_widget_show(ctk_display_device_dfp->scaling_frame);
    ctk_display_device_dfp->active_attributes |= __SCALING;

    gtk_widget_set_sensitive(ctk_display_device_dfp->scaling_frame, TRUE);

    dfp_scaling_update_radio_buttons(ctk_display_device_dfp, val);

} /* dfp_scaling_setup() */



/*
 * dfp_dithering_setup() - Update GUI to reflect X server settings
 * of DFP Dithering.
 */
static void dfp_dithering_setup(CtkDisplayDeviceDfp *ctk_display_device_dfp)
{
    ReturnStatus ret;
    int val;

    ret =
        NvCtrlGetDisplayAttribute(ctk_display_device_dfp->handle,
                                  ctk_display_device_dfp->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING, &val);
    if (ret != NvCtrlSuccess) {
        gtk_widget_set_sensitive(ctk_display_device_dfp->dithering_frame,
                                 FALSE);
        gtk_widget_hide(ctk_display_device_dfp->dithering_frame);
        ctk_display_device_dfp->active_attributes &= ~__DITHERING;
        return;
    }

    gtk_widget_show(ctk_display_device_dfp->dithering_frame);
    ctk_display_device_dfp->active_attributes |= __DITHERING;

    gtk_widget_set_sensitive(ctk_display_device_dfp->dithering_frame, TRUE);

    dfp_dithering_update_radio_buttons(ctk_display_device_dfp, val);

} /* dfp_dithering_setup() */



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

    dfp_dithering_setup(ctk_display_device_dfp);


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
