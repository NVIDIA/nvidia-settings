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

#include "NvCtrlAttributes.h"

#include "rgb_xpm.h"
#include "red_xpm.h"
#include "green_xpm.h"
#include "blue_xpm.h"

#include "ctkcurve.h"
#include "ctkscale.h"
#include "ctkcolorcorrection.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

#include <string.h>
#include <stdlib.h>


static const char *__active_color_help = "The Active Color Channel drop-down "
"menu allows you to select the color channel controlled by the Brightness, "
"Contrast and Gamma sliders.  You can adjust the red, green or blue channels "
"individually or all three channels at once.";

static const char *__resest_button_help = "The Reset Hardware Defaults "
"button restores the color correction settings to their default values.";

static const char *__confirm_button_help = "Some color correction settings "
"can yield an unusable display "
"(e.g., making the display unreadably dark or light).  When you "
"change the color correction values, the '10 Seconds to Confirm' "
"button will count down to zero.  If you have not clicked the "
"button by then to accept the changes, it will restore your previous values.";

static const char *__color_curve_help = "The color curve graph changes to "
"reflect your adjustments made with the Brightness, Constrast, and Gamma "
"sliders.";

static void
option_menu_changed         (GtkOptionMenu *, gpointer);

static void
set_button_sensitive        (GtkButton *);

static void
reset_button_clicked        (GtkButton *, gpointer);

static void
set_color_state           (CtkColorCorrection *, gint, gint, gfloat, gboolean);

static void
confirm_button_clicked      (GtkButton *, gpointer);

static void
adjustment_value_changed    (GtkAdjustment *, gpointer);

static gfloat
get_attribute_channel_value (CtkColorCorrection *, gint, gint);

static void
flush_attribute_channel_values (CtkColorCorrection *, gint, gint);

static void
ctk_color_correction_class_init(CtkColorCorrectionClass *);

static void
apply_parsed_attribute_list(CtkColorCorrection *, ParsedAttribute *);

static gboolean
do_confirm_countdown (gpointer);

static void
update_confirm_text  (CtkColorCorrection *);

enum {
    CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define RED        RED_CHANNEL_INDEX
#define GREEN      GREEN_CHANNEL_INDEX
#define BLUE       BLUE_CHANNEL_INDEX

#define ALL_CHANNELS_INDEX 3

#define CONTRAST   (CONTRAST_INDEX - CONTRAST_INDEX)
#define BRIGHTNESS (BRIGHTNESS_INDEX - CONTRAST_INDEX)
#define GAMMA      (GAMMA_INDEX - CONTRAST_INDEX)

#define DEFAULT_CONFIRM_COLORCORRECTION_TIMEOUT 10

#define CREATE_COLOR_ADJUSTMENT(adj, attr, min, max)          \
{                                                             \
    gdouble _step_incr, _page_incr, _def;                     \
                                                              \
    _step_incr = ((gdouble)((max) - (min)))/250.0;            \
    _page_incr = ((gdouble)((max) - (min)))/25.0;             \
                                                              \
    _def = get_attribute_channel_value(ctk_color_correction,  \
                                       (attr), ALL_CHANNELS); \
                                                              \
    (adj) = gtk_adjustment_new(_def, (min), (max),            \
                               _step_incr, _page_incr, 0.0);  \
}

GType ctk_color_correction_get_type(
    void
)
{
    static GType ctk_color_correction_type = 0;

    if (!ctk_color_correction_type) {
        static const GTypeInfo ctk_color_correction_info = {
            sizeof (CtkColorCorrectionClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_color_correction_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkColorCorrection),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_color_correction_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkColorCorrection",
                                   &ctk_color_correction_info, 0);
    }

    return ctk_color_correction_type;
}


static void
ctk_color_correction_class_init(CtkColorCorrectionClass
                                *ctk_color_correction_class)
{
    signals[CHANGED] =
        g_signal_new("changed",
                     G_OBJECT_CLASS_TYPE(ctk_color_correction_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(CtkColorCorrectionClass, changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
}


GtkWidget* ctk_color_correction_new(NvCtrlAttributeHandle *handle,
                                    CtkConfig *ctk_config, ParsedAttribute *p,
                                    CtkEvent *ctk_event)
{
    GObject *object;
    CtkColorCorrection *ctk_color_correction;
    GtkRequisition requisition;

    GtkWidget *menu;
    GtkWidget *image;
    GtkWidget *label;
    GtkWidget *scale;
    GtkWidget *curve;
    GtkWidget *menu_item;
    GtkWidget *alignment;
    GtkWidget *mainhbox;
    GtkWidget *leftvbox;
    GtkWidget *rightvbox;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *button, *confirm_button, *confirm_label;
    GtkWidget *widget;
    GtkWidget *hsep;
    GtkWidget *eventbox;

    object = g_object_new(CTK_TYPE_COLOR_CORRECTION, NULL);

    ctk_color_correction = CTK_COLOR_CORRECTION(object);
    ctk_color_correction->handle = handle;
    ctk_color_correction->ctk_config = ctk_config;
    ctk_color_correction->confirm_timer = 0;
    ctk_color_correction->confirm_countdown =
        DEFAULT_CONFIRM_COLORCORRECTION_TIMEOUT;
    apply_parsed_attribute_list(ctk_color_correction, p);

    gtk_box_set_spacing(GTK_BOX(ctk_color_correction), 10);

    /* create the main hbox and the two main vboxes*/

    mainhbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(ctk_color_correction), mainhbox,
                       FALSE, FALSE, 0);

    leftvbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(mainhbox), leftvbox, FALSE, FALSE, 0);
   
    rightvbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(mainhbox), rightvbox, TRUE, TRUE, 0);
    
    
    /*
     * Option menu: MIDDLE - LEFT
     *
     * This option menu allows the users to select which color_box
     * channel to apply contrast, brightness or gamma settings to.
     */

    
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(leftvbox), alignment, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    label = gtk_label_new("Active Color Channel:");
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox);

    menu = gtk_menu_new();
    
    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menu_item), hbox);

    label = gtk_label_new("All Channels");
    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(rgb_xpm));

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);


    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menu_item), hbox);

    label = gtk_label_new ("Red");
    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(red_xpm));

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);


    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menu_item), hbox);

    label = gtk_label_new("Green");
    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(green_xpm));

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);


    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(menu_item), hbox);

    label = gtk_label_new("Blue");
    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(blue_xpm));

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);


    ctk_color_correction->option_menu = gtk_option_menu_new ();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_color_correction->option_menu), menu);

    gtk_box_pack_start(GTK_BOX(vbox),
                       ctk_color_correction->option_menu,
                       FALSE, FALSE, 0); 

    g_object_set_data(G_OBJECT(ctk_color_correction->option_menu),
                      "color_channel", GINT_TO_POINTER(ALL_CHANNELS));
    
    g_signal_connect(G_OBJECT(ctk_color_correction->option_menu), "changed",
                     G_CALLBACK(option_menu_changed),
                     (gpointer) ctk_color_correction);

    ctk_config_set_tooltip(ctk_config, ctk_color_correction->option_menu,
                           __active_color_help);
    /*
     * Gamma curve: BOTTOM - LEFT
     *
     * This gamma curve plots the current color_box ramps in response
     * to user changes to contrast, brightness and gamma.
     */

    alignment = gtk_alignment_new(0, 0, 1.0, 1.0);
    gtk_box_pack_start(GTK_BOX(leftvbox), alignment, TRUE, TRUE, 0);

    curve = ctk_curve_new(handle, GTK_WIDGET(ctk_color_correction));
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), curve);
    gtk_container_add(GTK_CONTAINER(alignment), eventbox);

    ctk_config_set_tooltip(ctk_config, eventbox, __color_curve_help);

    /*
     * Reset button: BOTTOM - RIGHT (see below)
     *
     * This button will reset the contrast, brightness and gamma
     * settings to their respective default values (for all channels).
     */

    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new_with_label("Reset Hardware Defaults");
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    
    confirm_button = gtk_button_new();
    confirm_label = gtk_label_new("Confirm Current Changes");
    gtk_container_add(GTK_CONTAINER(confirm_button), confirm_label);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), confirm_button);
    gtk_box_pack_end(GTK_BOX(hbox), eventbox, FALSE, FALSE, 5);
    gtk_widget_set_sensitive(confirm_button, FALSE);
    
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_color_correction);

    g_signal_connect(G_OBJECT(confirm_button), "clicked",
                     G_CALLBACK(confirm_button_clicked),
                     (gpointer) ctk_color_correction);

    ctk_color_correction->confirm_label = confirm_label;
    ctk_color_correction->confirm_button = confirm_button;
    ctk_config_set_tooltip(ctk_config, eventbox, __confirm_button_help);
    ctk_config_set_tooltip(ctk_config, button, __resest_button_help);

    /*
     * Control sliders: MIDDLE - CENTER->RIGHT
     *
     * The user controls brightness, contrast and gamma values for
     * either or all of the possible color_box channels using these
     * sliders.
     */

    /* brightness slider */

    CREATE_COLOR_ADJUSTMENT(ctk_color_correction->brightness_adjustment,
                            BRIGHTNESS_VALUE, BRIGHTNESS_MIN, BRIGHTNESS_MAX);

    g_object_set_data(G_OBJECT(ctk_color_correction->brightness_adjustment),
                      "color_attribute", GINT_TO_POINTER(BRIGHTNESS_VALUE));

    g_signal_connect(G_OBJECT(ctk_color_correction->brightness_adjustment),
                     "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_color_correction);

    g_signal_connect_swapped
        (G_OBJECT(ctk_color_correction->brightness_adjustment),
         "value_changed", G_CALLBACK(set_button_sensitive), (gpointer) button);

    scale = ctk_scale_new
        (GTK_ADJUSTMENT(ctk_color_correction->brightness_adjustment),
         "Brightness", ctk_config, G_TYPE_DOUBLE);

    gtk_box_pack_start(GTK_BOX(rightvbox), scale, TRUE, TRUE, 0);
    
    widget = CTK_SCALE(scale)->gtk_scale;

    ctk_config_set_tooltip(ctk_config, widget, "The Brightness slider alters "
                           "the amount of brightness for the selected color "
                           "channel(s).");

    /* contrast slider */

    CREATE_COLOR_ADJUSTMENT(ctk_color_correction->contrast_adjustment,
                            CONTRAST_VALUE, CONTRAST_MIN, CONTRAST_MAX);

    g_object_set_data(G_OBJECT(ctk_color_correction->contrast_adjustment),
                      "color_attribute", GINT_TO_POINTER(CONTRAST_VALUE));
    
    g_signal_connect(G_OBJECT(ctk_color_correction->contrast_adjustment),
                     "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_color_correction);

    g_signal_connect_swapped
        (G_OBJECT(ctk_color_correction->contrast_adjustment), "value_changed",
         G_CALLBACK(set_button_sensitive), (gpointer) button);

    scale = ctk_scale_new
        (GTK_ADJUSTMENT(ctk_color_correction->contrast_adjustment),
         "Contrast", ctk_config, G_TYPE_DOUBLE);

    gtk_box_pack_start(GTK_BOX(rightvbox), scale, TRUE, TRUE, 0);
    
    widget = CTK_SCALE(scale)->gtk_scale;

    ctk_config_set_tooltip(ctk_config, widget, "The Contrast slider alters "
                           "the amount of contrast for the selected color "
                           "channel(s).");

    /* gamma slider */
    
    CREATE_COLOR_ADJUSTMENT(ctk_color_correction->gamma_adjustment,
                            GAMMA_VALUE, GAMMA_MIN, GAMMA_MAX);
    
    g_object_set_data(G_OBJECT(ctk_color_correction->gamma_adjustment),
                      "color_attribute", GINT_TO_POINTER(GAMMA_VALUE));
    
    g_signal_connect(G_OBJECT(ctk_color_correction->gamma_adjustment),
                     "value_changed", G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_color_correction);

    g_signal_connect_swapped
        (G_OBJECT(ctk_color_correction->gamma_adjustment), "value_changed",
         G_CALLBACK(set_button_sensitive), (gpointer) button);

    scale = ctk_scale_new
        (GTK_ADJUSTMENT(ctk_color_correction->gamma_adjustment),
         "Gamma", ctk_config, G_TYPE_DOUBLE);

    gtk_box_pack_start(GTK_BOX(rightvbox), scale, TRUE, TRUE, 0);

    widget = CTK_SCALE(scale)->gtk_scale;
 
    ctk_config_set_tooltip(ctk_config, widget, "The Gamma slider alters "
                           "the amount of gamma for the selected color "
                           "channel(s).");

    /* horizontal separator */
    
    vbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(ctk_color_correction), vbox, FALSE, FALSE, 0);
    
    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hsep, TRUE, TRUE, 0);


    /*
     * Reset button: BOTTOM - RIGHT (see above)
     *
     * The button was created earlier to make it accessible as event data
     * and still needs to be packed.
     */

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(object));

    /*
     * lock the size of the confirm button, so that it is not resized
     * when we change the button text later.
     *
     * Note: this assumes that the initial size of the button is the
     * largest size needed for any text placed in the button.  In the
     * case of the confirm button, this works out:
     *
     *  "Confirm Current Changes" <-- initial value
     *  "%d Seconds to Confirm"
     */

    gtk_widget_size_request(ctk_color_correction->confirm_label,
                            &requisition);
    gtk_widget_set_size_request(ctk_color_correction->confirm_label,
                                requisition.width, -1);

    return GTK_WIDGET(object);
}


static void option_menu_changed(
    GtkOptionMenu *option_menu,
    gpointer user_data
)
{
    GtkObject *adjustment;
    gint history, attribute, channel;
    CtkColorCorrection *ctk_color_correction;
    gfloat value;
    gint i;

    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);

    history = gtk_option_menu_get_history(option_menu);

    switch (history) {
        default:
        case 0:
            channel = ALL_CHANNELS;
            break;
        case 1:
            channel = RED_CHANNEL;
            break;
        case 2:
            channel = GREEN_CHANNEL;
            break;
        case 3:
            channel = BLUE_CHANNEL;
            break;
    }

    /*
     * store the currently selected color channel, so that
     * adjustment_value_changed() can update the correct channel(s) in
     * response to slider changes
     */

    g_object_set_data(G_OBJECT(option_menu), "color_channel",
                      GINT_TO_POINTER(channel));

    for (i = 0; i < 3; i++) {

        if (i == 0) {
            adjustment = ctk_color_correction->brightness_adjustment;
            attribute = BRIGHTNESS_VALUE;
        } else if (i == 1) {
            adjustment = ctk_color_correction->contrast_adjustment;
            attribute = CONTRAST_VALUE;
        } else {
            adjustment = ctk_color_correction->gamma_adjustment;
            attribute = GAMMA_VALUE;
        }
        
        value = get_attribute_channel_value(ctk_color_correction,
                                            attribute, channel);

        g_signal_handlers_block_matched
            (G_OBJECT(adjustment),
             G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(adjustment_value_changed), NULL);

        gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), value);
    
        g_signal_handlers_unblock_matched
            (G_OBJECT(adjustment),
             G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(adjustment_value_changed), NULL);
    }
}

static void set_button_sensitive(
    GtkButton *button
)
{
    gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
}



/** set_color_state() *******************
 *
 * Stores color state to cur_slider_val[attribute][channel]
 * and prev_slider_val[attribute][channel].
 *
 **/

static void set_color_state(CtkColorCorrection *ctk_color_correction,
    gint attribute_idx, gint channel_mask,
    gfloat value, gboolean all
)
{
    if (channel_mask & RED_CHANNEL) {
        ctk_color_correction->cur_slider_val[attribute_idx][RED] = value;
        if (all) {
            ctk_color_correction->prev_slider_val[attribute_idx][RED] = value;
        }
    }

    if (channel_mask & GREEN_CHANNEL) {
        ctk_color_correction->cur_slider_val[attribute_idx][GREEN] = value;
        if (all) {
            ctk_color_correction->prev_slider_val[attribute_idx][GREEN] = value;
        }
    }

    if (channel_mask & BLUE_CHANNEL) {
        ctk_color_correction->cur_slider_val[attribute_idx][BLUE] = value;
        if (all) {
            ctk_color_correction->prev_slider_val[attribute_idx][BLUE] = value;
        }
    }

    if (channel_mask == ALL_CHANNELS) {
        ctk_color_correction->cur_slider_val
            [attribute_idx][ALL_CHANNELS_INDEX] = value;
        if (all) {
            ctk_color_correction->prev_slider_val
                [attribute_idx][ALL_CHANNELS_INDEX] = value;
        }
    }
} /* set_color_state() */



/** confirm_button_clicked() *************
 *
 * Callback function which stores current values to previous value when user
 * clicks Confirm button.
 *
 **/

static void confirm_button_clicked(
   GtkButton *button,
   gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction = CTK_COLOR_CORRECTION(user_data);

    /* Store cur_slider_val[attribute][channel] to
       prev_slider_val[attribute][channel]. */
    memcpy (ctk_color_correction->prev_slider_val,
            ctk_color_correction->cur_slider_val,
            sizeof(ctk_color_correction->cur_slider_val));
    
    /* kill the timer */
    g_source_remove(ctk_color_correction->confirm_timer);
    ctk_color_correction->confirm_timer = 0;
    
    /* Reset confirm button text */
    gtk_label_set_text(GTK_LABEL(ctk_color_correction->confirm_label),
                       "Confirm Current Changes");
    
    gtk_widget_set_sensitive(ctk_color_correction->confirm_button, FALSE);
} /* confirm_button_clicked() */
        


static void reset_button_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction;
    GtkOptionMenu *option_menu;
    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);
    /* Set default values */
    set_color_state(ctk_color_correction, CONTRAST, ALL_CHANNELS,
                    CONTRAST_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, BRIGHTNESS, ALL_CHANNELS,
                    BRIGHTNESS_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, GAMMA, ALL_CHANNELS,
                    GAMMA_DEFAULT, TRUE);

    flush_attribute_channel_values(ctk_color_correction,
                                   ALL_VALUES, ALL_CHANNELS);

    option_menu = GTK_OPTION_MENU(ctk_color_correction->option_menu);
    
    if (gtk_option_menu_get_history(option_menu) == 0) {
        /*
         * gtk_option_menu_set_history will not emit the "changed" signal
         * unless the new index differs from the old one; reasonable, but
         * we need to cope with it here.
         */
        gtk_option_menu_set_history(option_menu, 1);
        gtk_option_menu_set_history(option_menu, 0);
    } else {
        gtk_option_menu_set_history(option_menu, 0);
    }

    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 "Reset color correction hardware defaults.");

    gtk_widget_set_sensitive(GTK_WIDGET(ctk_color_correction->confirm_button),
                             FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
    /* kill the timer */
    if (ctk_color_correction->confirm_timer) {
        g_source_remove(ctk_color_correction->confirm_timer);
        ctk_color_correction->confirm_timer = 0;
    }
    
    /* Reset confirm button text */
    gtk_label_set_text(GTK_LABEL(ctk_color_correction->confirm_label),
                       "Confirm Current Changes");
}

static void adjustment_value_changed(
    GtkAdjustment *adjustment,
    gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction;
    gint attribute, channel;
    gint attribute_idx;
    gfloat value;
    gchar *channel_str, *attribute_str;

    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);
    
    user_data = g_object_get_data(G_OBJECT(adjustment), "color_attribute");
    attribute = GPOINTER_TO_INT(user_data);

    user_data = g_object_get_data
        (G_OBJECT(ctk_color_correction->option_menu), "color_channel");
    channel = GPOINTER_TO_INT(user_data);

    value = gtk_adjustment_get_value(adjustment);
    
    /* start timer for confirming changes */
    ctk_color_correction->confirm_countdown =
        DEFAULT_CONFIRM_COLORCORRECTION_TIMEOUT;
    update_confirm_text(ctk_color_correction);

    if (ctk_color_correction->confirm_timer == 0) {
        ctk_color_correction->confirm_timer =
            g_timeout_add(1000,
                          (GSourceFunc)do_confirm_countdown,
                          (gpointer) (ctk_color_correction));
    }

    switch (attribute) {
    case CONTRAST_VALUE:
        attribute_idx = CONTRAST;
        attribute_str = "contrast";
        break;
    case BRIGHTNESS_VALUE:
        attribute_idx = BRIGHTNESS;
        attribute_str = "brightness";
        break;
    case GAMMA_VALUE:
        attribute_idx = GAMMA;
        attribute_str = "gamma";
        break;
    default:
        return;
    }

    switch (channel) {
    case RED_CHANNEL:
        channel_str = "red ";
        break;
    case GREEN_CHANNEL:
        channel_str = "green ";
        break;
    case BLUE_CHANNEL:
        channel_str = "blue ";
        break;
    case ALL_CHANNELS:
        channel_str = "";
        break;
    default:
        return;
    }

    set_color_state(ctk_color_correction, attribute_idx, channel,
                    value, FALSE);
    
    flush_attribute_channel_values(ctk_color_correction, attribute, channel);
    
    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 "Set %s%s to %f.",
                                 channel_str, attribute_str, value);
    gtk_widget_set_sensitive(ctk_color_correction->confirm_button, TRUE);
}


static gfloat get_attribute_channel_value(CtkColorCorrection
                                          *ctk_color_correction,
                                          gint attribute, gint channel)
{
    int attribute_idx, channel_idx;

    switch (attribute) {
        case CONTRAST_VALUE:
            attribute_idx = CONTRAST;
            break;
        case BRIGHTNESS_VALUE:
            attribute_idx = BRIGHTNESS;
            break;
        case GAMMA_VALUE:
            attribute_idx = GAMMA;
            break;
        default:
            return 0.0;
    }

    switch (channel) {
        case ALL_CHANNELS:
            channel_idx = ALL_CHANNELS_INDEX;
            break;
        case RED_CHANNEL:
            channel_idx = RED_CHANNEL_INDEX;
            break;
        case GREEN_CHANNEL:
            channel_idx = GREEN_CHANNEL_INDEX;
            break;
        case BLUE_CHANNEL:
            channel_idx = BLUE_CHANNEL_INDEX;
            break;
        default:
            return 0.0;
    }

    return ctk_color_correction->cur_slider_val[attribute_idx][channel_idx];
}

static void flush_attribute_channel_values(
    CtkColorCorrection *ctk_color_correction,
    gint attribute,
    gint channel
)
{
    NvCtrlAttributeHandle *handle = ctk_color_correction->handle;
    
    NvCtrlSetColorAttributes(handle,
                             ctk_color_correction->cur_slider_val[CONTRAST],
                             ctk_color_correction->cur_slider_val[BRIGHTNESS],
                             ctk_color_correction->cur_slider_val[GAMMA],
                             attribute | channel);
    
    g_signal_emit(ctk_color_correction, signals[CHANGED], 0);
}


static void apply_parsed_attribute_list(
    CtkColorCorrection *ctk_color_correction,
    ParsedAttribute *p
)
{
    int target_type, target_id;
    unsigned int attr = 0;
    
    set_color_state(ctk_color_correction, CONTRAST, ALL_CHANNELS,
                    CONTRAST_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, BRIGHTNESS, ALL_CHANNELS,
                    BRIGHTNESS_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, GAMMA, ALL_CHANNELS,
                    GAMMA_DEFAULT, TRUE);

    target_type = NvCtrlGetTargetType(ctk_color_correction->handle);
    target_id = NvCtrlGetTargetId(ctk_color_correction->handle);

    while (p) {

        if (!p->next) goto next_attribute;
        
        if (!(p->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE)) goto next_attribute;
        
        /*
         * if this parsed attribute's target_type, target_id does not match the
         * current target_type and target_id then ignore
         */
        
        if ((p->target_type != target_type) ||
            (p->target_id != target_id)) goto next_attribute;
        
        switch (p->attr & (ALL_VALUES | ALL_CHANNELS)) {
        case (CONTRAST_VALUE | RED_CHANNEL):
            set_color_state(ctk_color_correction, CONTRAST,
                            RED_CHANNEL, p->val.f, TRUE); break;
        case (CONTRAST_VALUE | GREEN_CHANNEL):
            set_color_state(ctk_color_correction, CONTRAST,
                            GREEN_CHANNEL, p->val.f, TRUE); break;
        case (CONTRAST_VALUE | BLUE_CHANNEL):
            set_color_state(ctk_color_correction, CONTRAST,
                            BLUE_CHANNEL, p->val.f, TRUE); break;
        case (CONTRAST_VALUE | ALL_CHANNELS):
            set_color_state(ctk_color_correction, CONTRAST,
                            ALL_CHANNELS, p->val.f, TRUE); break;

        case (BRIGHTNESS_VALUE | RED_CHANNEL):
            set_color_state(ctk_color_correction, BRIGHTNESS,
                            RED_CHANNEL, p->val.f, TRUE); break;
        case (BRIGHTNESS_VALUE | GREEN_CHANNEL):
            set_color_state(ctk_color_correction, BRIGHTNESS,
                            GREEN_CHANNEL, p->val.f, TRUE); break;
        case (BRIGHTNESS_VALUE | BLUE_CHANNEL):
            set_color_state(ctk_color_correction, BRIGHTNESS,
                            BLUE_CHANNEL, p->val.f, TRUE); break;
        case (BRIGHTNESS_VALUE | ALL_CHANNELS):
            set_color_state(ctk_color_correction, BRIGHTNESS,
                            ALL_CHANNELS, p->val.f, TRUE); break;

        case (GAMMA_VALUE | RED_CHANNEL):
            set_color_state(ctk_color_correction, GAMMA,
                            RED_CHANNEL, p->val.f, TRUE); break;
        case (GAMMA_VALUE | GREEN_CHANNEL):
            set_color_state(ctk_color_correction, GAMMA,
                            GREEN_CHANNEL, p->val.f, TRUE); break;
        case (GAMMA_VALUE | BLUE_CHANNEL):
            set_color_state(ctk_color_correction, GAMMA,
                            BLUE_CHANNEL, p->val.f, TRUE); break;
        case (GAMMA_VALUE | ALL_CHANNELS):
            set_color_state(ctk_color_correction, GAMMA,
                            ALL_CHANNELS, p->val.f, TRUE); break;

        default:
            goto next_attribute;
        }
        
        attr |= (p->attr & (ALL_VALUES | ALL_CHANNELS));
        
    next_attribute:
        
        p = p->next;
    }

    if (attr) {
        NvCtrlSetColorAttributes(ctk_color_correction->handle,
                                 ctk_color_correction->cur_slider_val[CONTRAST],
                                 ctk_color_correction->cur_slider_val[BRIGHTNESS],
                                 ctk_color_correction->cur_slider_val[GAMMA],
                                 attr);
    }
    
} /* apply_parsed_attribute_list() */

/** update_confirm_text() ************************************
 *
 * Generates the text used to lable confirmation button.
 *
 **/

static void update_confirm_text(CtkColorCorrection *ctk_color_correction)
{
    gchar *str;
    str = g_strdup_printf("%d Seconds to Confirm",
                          ctk_color_correction->confirm_countdown);
    gtk_label_set_text(GTK_LABEL(ctk_color_correction->confirm_label),
                       str);
    g_free(str);
} /* update_confirm_text() */

/** do_confirm_countdown() ***********************************
 *
 * timeout callback for reverting color correction slider changes if user not
 * confirm changes.
 *
 **/

static gboolean do_confirm_countdown(gpointer data)
{
    gint attr, ch;
    CtkColorCorrection *ctk_color_correction = (CtkColorCorrection *)(data);
    unsigned int channels = 0;
    unsigned int attributes = 0;
    GtkOptionMenu *option_menu =
        GTK_OPTION_MENU(ctk_color_correction->option_menu);

    ctk_color_correction->confirm_countdown--;
    if (ctk_color_correction->confirm_countdown > 0) {
        update_confirm_text(ctk_color_correction);
        return True;
    }

    /* Countdown timed out, reset color settings to previous state */
    for (attr = CONTRAST_INDEX; attr <= GAMMA_INDEX; attr++) {
        for (ch = RED; ch <= ALL_CHANNELS_INDEX; ch++) {
            /* Check for attribute channel value change. */
            int index = attr - CONTRAST_INDEX;
            if (ctk_color_correction->cur_slider_val[index][ch] !=
                ctk_color_correction->prev_slider_val[index][ch]) {
                ctk_color_correction->cur_slider_val[index][ch] =
                    ctk_color_correction->prev_slider_val[index][ch];
                attributes |= (1 << attr);
                channels |= (1 << ch);
            }
        }
    }
    if (attributes | channels) {
        flush_attribute_channel_values(ctk_color_correction,
                                       attributes, channels);
    }
    
    /* Refresh color correction page for current selected channel. */
    option_menu_changed(option_menu, (gpointer)(ctk_color_correction));
    
    /* Reset confirm button text */
    gtk_label_set_text(GTK_LABEL(ctk_color_correction->confirm_label),
                       "Confirm Current Changes");
    
    /* Update status bar message */
    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 "Reverted color correction changes, due to "
                                 "confirmation timeout.");
                                
    
    ctk_color_correction->confirm_timer = 0;
    gtk_widget_set_sensitive(ctk_color_correction->confirm_button, FALSE);
    return False;

} /* do_confirm_countdown() */


GtkTextBuffer *ctk_color_correction_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    const gchar *title = "X Server Color Correction";
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);
    
    ctk_help_title(b, &i, "%s Help", title);

    ctk_color_correction_tab_help(b, &i, title, FALSE /* randr */);
    
    ctk_help_heading(b, &i, "Reset Hardware Defaults");
    ctk_help_para(b, &i, __resest_button_help);

    ctk_help_finish(b);

    return b;
}


void ctk_color_correction_tab_help(GtkTextBuffer *b, GtkTextIter *i,
                                   const gchar *title,
                                   gboolean randr)
{
    ctk_help_heading(b, i, "Color Correction");

    ctk_help_term(b, i, "Active Color Channel");
    ctk_help_para(b, i, __active_color_help);

    ctk_help_term(b, i, "Brightness, Contrast and Gamma");
    ctk_help_para(b, i, "The Brightness, Contrast and Gamma sliders "
                  "allow you to adjust the brightness, contrast, "
                  "or gamma values for the selected color channel(s).  This "
                  "helps you to compensate "
                  "for variations in luminance between a source image and "
                  "its output on a display device.  This is useful when "
                  "working with image processing applications to help "
                  "provide more accurate color reproduction of images (such "
                  "as photographs) when they are displayed on your "
                  "monitor.");

    ctk_help_para(b, i, "Also, many 3D-accelerated games may appear too "
                  "dark to play.  Increasing the brightness and/or gamma "
                  "value equally across all channels will make these games "
                  "appear brighter, making them more playable.");

    ctk_help_para(b, i, __color_curve_help);

    if (randr) {
        ctk_help_para(b, i, "The %s tab uses the RandR extension to "
                      "manipulate an RandR CRTC's gamma ramp.", title);
    } else {
        ctk_help_para(b, i, "The %s page uses the XF86VidMode extension "
                      "to manipulate the X screen's gamma ramps", title);
    }

    ctk_help_term(b, i, "Confirm Current Changes");
    ctk_help_para(b, i, __confirm_button_help);
}
