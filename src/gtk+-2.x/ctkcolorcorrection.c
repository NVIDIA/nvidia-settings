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
#include "ctkutils.h"

#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#define _(STRING) gettext(STRING)
#define N_(STRING) STRING

static const char *__active_color_help = N_("The Active Color Channel drop-down "
"menu allows you to select the color channel controlled by the Brightness, "
"Contrast and Gamma sliders.  You can adjust the red, green or blue channels "
"individually or all three channels at once.");

static const char *__resest_button_help = N_("The Reset Hardware Defaults "
"button restores the color correction settings to their default values.");

static const char *__confirm_button_help = N_("Some color correction settings "
"can yield an unusable display "
"(e.g., making the display unreadably dark or light).  When you "
"change the color correction values, the '10 Seconds to Confirm' "
"button will count down to zero.  If you have not clicked the "
"button by then to accept the changes, it will restore your previous values.");

static const char *__color_curve_help = N_("The color curve graph changes to "
"reflect your adjustments made with the Brightness, Contrast, and Gamma "
"sliders.");

static void
color_channel_changed         (GtkComboBox *, gpointer);

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

static void ctk_color_correction_finalize(GObject *);

static void
apply_parsed_attribute_list(CtkColorCorrection *, ParsedAttribute *);

static gboolean
do_confirm_countdown (gpointer);

static void callback_palette_update(GObject *object, CtrlEvent *event,
                                    gpointer user_data);

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

#define CREATE_COLOR_ADJUSTMENT(adj, attr, min, max)                         \
{                                                                            \
    gdouble _step_incr, _page_incr, _def;                                    \
                                                                             \
    _step_incr = ((gdouble)((max) - (min)))/250.0;                           \
    _page_incr = ((gdouble)((max) - (min)))/25.0;                            \
                                                                             \
    _def = get_attribute_channel_value(ctk_color_correction,                 \
                                       (attr), ALL_CHANNELS);                \
                                                                             \
    (adj) = GTK_ADJUSTMENT(gtk_adjustment_new(_def, (min), (max),            \
                                              _step_incr, _page_incr, 0.0)); \
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
    GObjectClass *gobject_class = (GObjectClass *)ctk_color_correction_class;

    signals[CHANGED] =
        g_signal_new("changed",
                     G_OBJECT_CLASS_TYPE(ctk_color_correction_class),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(CtkColorCorrectionClass, changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
    gobject_class->finalize = ctk_color_correction_finalize;
}



static void ctk_color_correction_finalize(GObject *object)
{
    CtkColorCorrection *ctk_color_correction = CTK_COLOR_CORRECTION(object);
    CtrlTarget *ctrl_target = ctk_color_correction->ctrl_target;

    if (ctk_color_correction->confirm_timer) {
        /*
         * This situation comes, if user perform VT-switching
         * without confirmation of color-correction settings.
         */
        gint attr, ch;
        unsigned int channels = 0;
        unsigned int attributes = 0;

        /* kill the timer */
        g_source_remove(ctk_color_correction->confirm_timer);
        ctk_color_correction->confirm_timer = 0;

        /*
         * Reset color settings to previous state,
         * since user did not confirm settings yet.
         */
        for (attr = CONTRAST_INDEX; attr <= GAMMA_INDEX; attr++) {
            for (ch = RED; ch <= ALL_CHANNELS_INDEX; ch++) {
                    /* Check for attribute channel value change. */
                    int index = attr - CONTRAST_INDEX;

                    ctk_color_correction->cur_slider_val[index][ch] =
                        ctk_color_correction->prev_slider_val[index][ch];

                    attributes |= (1 << attr);
                    channels |= (1 << ch);
            }
        }

        NvCtrlSetColorAttributes(ctrl_target,
                                 ctk_color_correction->cur_slider_val[CONTRAST],
                                 ctk_color_correction->cur_slider_val[BRIGHTNESS],
                                 ctk_color_correction->cur_slider_val[GAMMA],
                                 attributes | channels);
    }

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_color_correction->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_color_correction);


}



GtkWidget* ctk_color_correction_new(CtrlTarget *ctrl_target,
                                    CtkConfig *ctk_config,
                                    ParsedAttribute *p,
                                    CtkEvent *ctk_event)
{
    GObject *object;
    CtkColorCorrection *ctk_color_correction;
    GtkRequisition requisition;

    GtkWidget *image;
    GtkWidget *label;
    GtkWidget *scale;
    GtkWidget *curve;
    GtkWidget *alignment;
    GtkWidget *mainhbox;
    GtkWidget *leftvbox;
    GtkWidget *rightvbox;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *button, *confirm_button, *confirm_label;
    GtkWidget *widget;
    GtkWidget *hsep;
    GtkWidget *center_alignment;
    GtkWidget *eventbox;
    GtkWidget *combo_box;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;

    object = g_object_new(CTK_TYPE_COLOR_CORRECTION, NULL);

    ctk_color_correction = CTK_COLOR_CORRECTION(object);
    ctk_color_correction->ctrl_target = ctrl_target;
    ctk_color_correction->ctk_config = ctk_config;
    ctk_color_correction->ctk_event = ctk_event;
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
    label = gtk_label_new(_("Active Color Channel:"));
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), vbox);

    store = gtk_list_store_new(2, GDK_TYPE_PIXBUF, G_TYPE_STRING);

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, gdk_pixbuf_new_from_xpm_data(rgb_xpm),
                       1, _("All Channels"), -1);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, gdk_pixbuf_new_from_xpm_data(red_xpm),
                       1, _("Red"), -1);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, gdk_pixbuf_new_from_xpm_data(green_xpm),
                       1, _("Green"), -1);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,
                       0, gdk_pixbuf_new_from_xpm_data(blue_xpm),
                       1, _("Blue"), -1);

    combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
                                   "pixbuf", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), renderer,
                                   "text", 1, NULL);

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);

    ctk_color_correction->color_channel = combo_box;

    gtk_box_pack_start(GTK_BOX(vbox),
                       ctk_color_correction->color_channel,
                       FALSE, FALSE, 0); 

    g_object_set_data(G_OBJECT(ctk_color_correction->color_channel),
                      "color_channel", GINT_TO_POINTER(ALL_CHANNELS));

    g_signal_connect(G_OBJECT(ctk_color_correction->color_channel), "changed",
                     G_CALLBACK(color_channel_changed),
                     (gpointer) ctk_color_correction);

    ctk_config_set_tooltip(ctk_config, ctk_color_correction->color_channel,
                           _(__active_color_help));
    /*
     * Gamma curve: BOTTOM - LEFT
     *
     * This gamma curve plots the current color_box ramps in response
     * to user changes to contrast, brightness and gamma.
     */

    alignment = gtk_alignment_new(0, 0, 1.0, 1.0);
    gtk_box_pack_start(GTK_BOX(leftvbox), alignment, TRUE, TRUE, 0);

    curve = ctk_curve_new(ctrl_target, GTK_WIDGET(ctk_color_correction));
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), curve);
    gtk_container_add(GTK_CONTAINER(alignment), eventbox);

    ctk_config_set_tooltip(ctk_config, eventbox, _(__color_curve_help));
    ctk_color_correction->curve = curve;

    /*
     * Reset button: BOTTOM - RIGHT (see below)
     *
     * This button will reset the contrast, brightness and gamma
     * settings to their respective default values (for all channels).
     */

    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new_with_label(_("Reset Hardware Defaults"));
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    
    confirm_button = gtk_button_new();
    confirm_label = gtk_label_new(_("Confirm Current Changes"));
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
    ctk_color_correction->reset_button = button;
    ctk_config_set_tooltip(ctk_config, eventbox, _(__confirm_button_help));
    ctk_config_set_tooltip(ctk_config, button, _(__resest_button_help));

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

    ctk_config_set_tooltip(ctk_config, widget, _("The Brightness slider alters "
                           "the amount of brightness for the selected color "
                           "channel(s)."));

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

    ctk_config_set_tooltip(ctk_config, widget, _("The Contrast slider alters "
                           "the amount of contrast for the selected color "
                           "channel(s)."));

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
         _("Gamma"), ctk_config, G_TYPE_DOUBLE);

    gtk_box_pack_start(GTK_BOX(rightvbox), scale, TRUE, TRUE, 0);

    widget = CTK_SCALE(scale)->gtk_scale;
 
    ctk_config_set_tooltip(ctk_config, widget, _("The Gamma slider alters "
                           "the amount of gamma for the selected color "
                           "channel(s)."));

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

    /* external update notification label */

    center_alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_box_pack_start(GTK_BOX(vbox), center_alignment, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(center_alignment), hbox);

    label = gtk_label_new(_("Warning: The color settings have been changed "
                          "outside of nvidia-settings so the current slider "
                          "values may be incorrect."));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

    image = ctk_image_new_from_str(CTK_STOCK_DIALOG_WARNING,
                                   GTK_ICON_SIZE_BUTTON);

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    ctk_color_correction->warning_container = hbox;


    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* except the external color change update warning */

    gtk_widget_hide(ctk_color_correction->warning_container);

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

    ctk_widget_get_preferred_size(ctk_color_correction->confirm_label,
                                  &requisition);
    gtk_widget_set_size_request(ctk_color_correction->confirm_label,
                                requisition.width, -1);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_PALETTE_UPDATE_EVENT),
                     G_CALLBACK(callback_palette_update),
                     (gpointer) ctk_color_correction);

    return GTK_WIDGET(object);
}


static void color_channel_changed(
    GtkComboBox *combo_box,
    gpointer user_data
)
{
    GtkAdjustment *adjustment;
    gint history, attribute, channel;
    CtkColorCorrection *ctk_color_correction;
    gfloat value;
    gint i;

    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);

    history = gtk_combo_box_get_active(combo_box);

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

    g_object_set_data(G_OBJECT(combo_box), "color_channel",
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
                       _("Confirm Current Changes"));
    
    gtk_widget_set_sensitive(ctk_color_correction->confirm_button, FALSE);
} /* confirm_button_clicked() */
        


static void reset_button_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction;
    GtkComboBox *combo_box;
    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);
    /* Set default values */
    set_color_state(ctk_color_correction, CONTRAST, ALL_CHANNELS,
                    CONTRAST_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, BRIGHTNESS, ALL_CHANNELS,
                    BRIGHTNESS_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, GAMMA, ALL_CHANNELS,
                    GAMMA_DEFAULT, TRUE);

    ctk_color_correction->num_expected_updates++;
    
    flush_attribute_channel_values(ctk_color_correction,
                                   ALL_VALUES, ALL_CHANNELS);

    combo_box = GTK_COMBO_BOX(ctk_color_correction->color_channel);
    
    if (gtk_combo_box_get_active(combo_box) == 0) {
        /*
         * We use the color_channel_changed function to reload color information
         * from the server. If we are already on the correct channel, we cannot
         * rely on the "changed" signal to be triggered so we will just call it
         * directly here.
         */
        color_channel_changed(combo_box, user_data);
    } else {
        gtk_combo_box_set_active(combo_box, 0);
    }

    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 _("Reset color correction hardware defaults."));

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
                       _("Confirm Current Changes"));
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
        (G_OBJECT(ctk_color_correction->color_channel), "color_channel");
    channel = GPOINTER_TO_INT(user_data);

    value = gtk_adjustment_get_value(adjustment);
    
    ctk_color_correction->num_expected_updates++;

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
                                 _("Set %s%s to %f."),
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
    CtrlTarget *ctrl_target = ctk_color_correction->ctrl_target;

    NvCtrlSetColorAttributes(ctrl_target,
                             ctk_color_correction->cur_slider_val[CONTRAST],
                             ctk_color_correction->cur_slider_val[BRIGHTNESS],
                             ctk_color_correction->cur_slider_val[GAMMA],
                             attribute | channel);

    gtk_widget_hide(ctk_color_correction->warning_container);

    g_signal_emit(ctk_color_correction, signals[CHANGED], 0);
}


static void apply_parsed_attribute_list(
    CtkColorCorrection *ctk_color_correction,
    ParsedAttribute *p
)
{
    CtrlTarget *ctrl_target = ctk_color_correction->ctrl_target;
    int target_type, target_id;
    unsigned int attr = 0;

    ctk_color_correction->num_expected_updates = 0;

    set_color_state(ctk_color_correction, CONTRAST, ALL_CHANNELS,
                    CONTRAST_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, BRIGHTNESS, ALL_CHANNELS,
                    BRIGHTNESS_DEFAULT, TRUE);
    set_color_state(ctk_color_correction, GAMMA, ALL_CHANNELS,
                    GAMMA_DEFAULT, TRUE);

    target_type = NvCtrlGetTargetType(ctrl_target);
    target_id = NvCtrlGetTargetId(ctrl_target);

    while (p) {
        CtrlTargetNode *node;
        const AttributeTableEntry *a = p->attr_entry;

        if (!p->next) goto next_attribute;

        if (a->type != CTRL_ATTRIBUTE_TYPE_COLOR) {
            if (a->attr == NV_CTRL_COLOR_SPACE ||
                a->attr == NV_CTRL_COLOR_RANGE) {
                for (node = p->targets; node ; node = node->next) {
                    int attr_target_type = NvCtrlGetTargetType(node->t);
                    int attr_target_id = NvCtrlGetTargetId(node->t);

                    if ((attr_target_type == target_type) &&
                        (attr_target_id == target_id)) {
                        ctk_color_correction->num_expected_updates++;
                    }
                }
            }
            goto next_attribute;
        }

        /*
         * Apply the parsed attribute's settings only if the color 
         * correction's target matches one of the (parse attribute's)
         * specification targets.
         */

        for (node = p->targets; node; node = node->next) {

            int attr_target_type = NvCtrlGetTargetType(node->t);
            int attr_target_id = NvCtrlGetTargetId(node->t);

            if ((attr_target_type != target_type) ||
                (attr_target_id != target_id)) {
                continue;
            }

            switch (a->attr & (ALL_VALUES | ALL_CHANNELS)) {
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
                continue;
            }

            ctk_color_correction->num_expected_updates++;

            attr |= (a->attr & (ALL_VALUES | ALL_CHANNELS));

        }

    next_attribute:

        p = p->next;
    }

    if (attr) {

        int i;

        /*
         * if all the separate color channels are the same for an
         * attribute, propagate the value to ALL_CHANNELS for that
         * attribute
         */
        for (i = CONTRAST; i <= GAMMA; i++) {
            float val = ctk_color_correction->cur_slider_val[i][RED];

            if ((ctk_color_correction->cur_slider_val[i][GREEN] == val) &&
                (ctk_color_correction->cur_slider_val[i][BLUE] == val)) {
                set_color_state(ctk_color_correction, i,
                                ALL_CHANNELS, val, TRUE);
                attr |= ALL_CHANNELS;
            }
        }

        ctk_color_correction->num_expected_updates++;

        NvCtrlSetColorAttributes(ctrl_target,
                                 ctk_color_correction->cur_slider_val[CONTRAST],
                                 ctk_color_correction->cur_slider_val[BRIGHTNESS],
                                 ctk_color_correction->cur_slider_val[GAMMA],
                                 attr);
    }
    
} /* apply_parsed_attribute_list() */

/** update_confirm_text() ************************************
 *
 * Generates the text used to label confirmation button.
 *
 **/

static void update_confirm_text(CtkColorCorrection *ctk_color_correction)
{
    gchar *str;
    str = g_strdup_printf(ngettext("%d Second to Confirm",
                                   "%d Seconds to Confirm",
                                   ctk_color_correction->confirm_countdown),
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
    GtkComboBox *combo_box =
        GTK_COMBO_BOX(ctk_color_correction->color_channel);

    ctk_color_correction->confirm_countdown--;
    if (ctk_color_correction->confirm_countdown > 0) {
        update_confirm_text(ctk_color_correction);
        return True;
    }

    ctk_color_correction->num_expected_updates++;

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
    color_channel_changed(combo_box, (gpointer)(ctk_color_correction));
    
    /* Reset confirm button text */
    gtk_label_set_text(GTK_LABEL(ctk_color_correction->confirm_label),
                       _("Confirm Current Changes"));
    
    /* Update status bar message */
    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 _("Reverted color correction changes, due to "
                                 "confirmation timeout."));
                                
    
    ctk_color_correction->confirm_timer = 0;
    gtk_widget_set_sensitive(ctk_color_correction->confirm_button, FALSE);
    return False;

} /* do_confirm_countdown() */


GtkTextBuffer *ctk_color_correction_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    const gchar *title = _("X Server Color Correction");
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);
    
    ctk_help_title(b, &i, _("%s Help"), title);

    ctk_color_correction_tab_help(b, &i, title, FALSE /* randr */);
    
    ctk_help_heading(b, &i, _("Reset Hardware Defaults"));
    ctk_help_para(b, &i, "%s", _(__resest_button_help));

    ctk_help_finish(b);

    return b;
}


void ctk_color_correction_tab_help(GtkTextBuffer *b, GtkTextIter *i,
                                   const gchar *title,
                                   gboolean randr)
{
    ctk_help_heading(b, i, _("Color Correction"));

    ctk_help_term(b, i, _("Active Color Channel"));
    ctk_help_para(b, i, "%s", _(__active_color_help));

    ctk_help_term(b, i, _("Brightness, Contrast and Gamma"));
    ctk_help_para(b, i, _("The Brightness, Contrast and Gamma sliders "
                  "allow you to adjust the brightness, contrast, "
                  "or gamma values for the selected color channel(s).  This "
                  "helps you to compensate "
                  "for variations in luminance between a source image and "
                  "its output on a display device.  This is useful when "
                  "working with image processing applications to help "
                  "provide more accurate color reproduction of images (such "
                  "as photographs) when they are displayed on your "
                  "monitor."));

    ctk_help_para(b, i, _("Also, many 3D-accelerated games may appear too "
                  "dark to play.  Increasing the brightness and/or gamma "
                  "value equally across all channels will make these games "
                  "appear brighter, making them more playable."));

    ctk_help_para(b, i, "%s", _(__color_curve_help));

    if (randr) {
        ctk_help_para(b, i, _("The %s tab uses the RandR extension to "
                      "manipulate an RandR CRTC's gamma ramp."), title);
    } else {
        ctk_help_para(b, i, _("The %s page uses the XF86VidMode extension "
                      "to manipulate the X screen's gamma ramps"), title);
    }

    ctk_help_term(b, i, _("Confirm Current Changes"));
    ctk_help_para(b, i, "%s", _(__confirm_button_help));
}


static void callback_palette_update(GObject *object,
                                    CtrlEvent *event,
                                    gpointer user_data)
{
    gboolean reload_needed;
    CtkColorCorrection *ctk_color_correction = (CtkColorCorrection *)user_data;
    CtrlTarget *ctrl_target = ctk_color_correction->ctrl_target;

    reload_needed = (ctk_color_correction->num_expected_updates <= 0);

    if (ctk_color_correction->num_expected_updates > 0) {
        ctk_color_correction->num_expected_updates--;
    }

    if (reload_needed) {
        NvCtrlReloadColorRamp(ctrl_target);

        ctk_curve_color_changed(ctk_color_correction->curve);
        gtk_widget_set_sensitive(ctk_color_correction->reset_button, TRUE);

        gtk_widget_show(ctk_color_correction->warning_container);
    } else {
        gtk_widget_hide(ctk_color_correction->warning_container);
    }
}

