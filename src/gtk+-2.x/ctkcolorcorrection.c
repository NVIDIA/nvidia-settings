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

#include "rgb_xpm.h"
#include "red_xpm.h"
#include "green_xpm.h"
#include "blue_xpm.h"
#include "color_correction_banner.h"
#include "ctkimage.h"

#include "ctkcurve.h"
#include "ctkscale.h"
#include "ctkcolorcorrection.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

#include <string.h>
#include <stdlib.h>


static const char *__active_color_help = "The Active Color Channel drop-down "
"menu allows you to select the color channel controlled by the Brightness, "
"Contrast and Gamma sliders. You can adjust the red, green or blue channels "
"individually or all three channels at once.";

static const char *__resest_button_help = "The Reset Hardware Defaults "
"button restores the color correction settings to their default values.";

static const char *__color_curve_help = "The color curve graph changes to "
"reflect your adjustments made with the Brightness, Constrast, and Gamma "
"sliders.";

static void
option_menu_changed         (GtkOptionMenu *, gpointer);

static void
set_button_sensitive        (GtkButton *);

static void
button_clicked              (GtkButton *, gpointer);

static void
adjustment_value_changed    (GtkAdjustment *, gpointer);


static gfloat
get_attribute_channel_value (CtkColorCorrection *, gint, gint);

static void
set_attribute_channel_value (CtkColorCorrection *, gint, gint, gfloat);

static void
ctk_color_correction_class_init(CtkColorCorrectionClass *);

static void
apply_parsed_attribute_list(CtkColorCorrection *, ParsedAttribute *);

enum {
    CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


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
    GObjectClass *gobject_class;

    gobject_class = (GObjectClass *) ctk_color_correction_class;

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

    GtkWidget *menu;
    GtkWidget *image;
    GtkWidget *banner;
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
    GtkWidget *button;
    GtkWidget *widget;
    GtkWidget *hsep;
    GtkWidget *eventbox;
    ReturnStatus ret;
    gint val;

    /* check if the VidMode extension is present */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_VM_PRESENT, &val);
    if ((ret != NvCtrlSuccess) || (val == FALSE)) return NULL;
    
    object = g_object_new(CTK_TYPE_COLOR_CORRECTION, NULL);

    ctk_color_correction = CTK_COLOR_CORRECTION(object);
    ctk_color_correction->handle = handle;
    ctk_color_correction->ctk_config = ctk_config;

    apply_parsed_attribute_list(ctk_color_correction, p);

    gtk_box_set_spacing(GTK_BOX(ctk_color_correction), 10);

    /*
     * Banner: TOP - LEFT -> RIGHT
     *
     * This image serves as a visual reference for basic color_box correction
     * purposes.
     */

    banner = ctk_banner_image_new(&color_correction_banner_image);
    gtk_box_pack_start(GTK_BOX(ctk_color_correction), banner, FALSE, FALSE, 0);

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

    label = gtk_label_new("Reset Hardware Defaults");
    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new();
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(button_clicked),
                     (gpointer) ctk_color_correction);

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
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(object));
    
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
        
        g_object_set_data(G_OBJECT(option_menu), "color_channel",
                          GINT_TO_POINTER(channel));
        
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
    
        if (channel == ALL_CHANNELS)
            gtk_adjustment_value_changed(GTK_ADJUSTMENT(adjustment));
    }
}

static void set_button_sensitive(
    GtkButton *button
)
{
    gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
}

static void button_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction;
    gfloat c[3] = {CONTRAST_DEFAULT,   CONTRAST_DEFAULT,   CONTRAST_DEFAULT};
    gfloat b[3] = {BRIGHTNESS_DEFAULT, BRIGHTNESS_DEFAULT, BRIGHTNESS_DEFAULT};
    gfloat g[3] = {GAMMA_DEFAULT,      GAMMA_DEFAULT,      GAMMA_DEFAULT};
    GtkOptionMenu *option_menu;
        
    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);

    NvCtrlSetColorAttributes(ctk_color_correction->handle, c, b, g,
                             ALL_CHANNELS | ALL_VALUES);
    
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

    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

static void adjustment_value_changed(
    GtkAdjustment *adjustment,
    gpointer user_data
)
{
    CtkColorCorrection *ctk_color_correction;
    gint attribute, channel;
    gfloat value;
    gchar *channel_str, *attribute_str;

    ctk_color_correction = CTK_COLOR_CORRECTION(user_data);
    
    user_data = g_object_get_data(G_OBJECT(adjustment), "color_attribute");
    attribute = GPOINTER_TO_INT(user_data);

    user_data = g_object_get_data
        (G_OBJECT(ctk_color_correction->option_menu), "color_channel");
    channel = GPOINTER_TO_INT(user_data);

    value = gtk_adjustment_get_value(adjustment);

    set_attribute_channel_value(ctk_color_correction,
                                attribute, channel, value);

    switch (attribute) {
      case CONTRAST_VALUE:   attribute_str = "contrast";   break;
      case BRIGHTNESS_VALUE: attribute_str = "brightness"; break;
      case GAMMA_VALUE:      attribute_str = "gamma";      break;
      default:               attribute_str = "unknown";    break;
    }

    switch (channel) {
      case RED_CHANNEL:   channel_str = "red ";   break;
      case GREEN_CHANNEL: channel_str = "green "; break;
      case BLUE_CHANNEL:  channel_str = "blue ";  break;
      case ALL_CHANNELS:  /* fall through */
      default:            channel_str = "";       break;
    }

    ctk_config_statusbar_message(ctk_color_correction->ctk_config,
                                 "Set %s%s to %f.",
                                 channel_str, attribute_str, value);
}


static gfloat get_attribute_channel_value(CtkColorCorrection
                                          *ctk_color_correction,
                                          gint attribute, gint channel)
{
    gfloat values[3] = { 0.0, 0.0, 0.0 };
    gfloat ignore[3] = { 0.0, 0.0, 0.0 };

    NvCtrlAttributeHandle *handle = ctk_color_correction->handle;

    switch (attribute) {
        case CONTRAST_VALUE:
            NvCtrlGetColorAttributes(handle, values, ignore, ignore);
            break;
        case BRIGHTNESS_VALUE:
            NvCtrlGetColorAttributes(handle, ignore, values, ignore);
            break;
        case GAMMA_VALUE:
            NvCtrlGetColorAttributes(handle, ignore, ignore, values);
            break;
        default:
            return 0.0;
    }

    switch (channel) {
        case ALL_CHANNELS: /* XXX what to do for all channels? */
        case RED_CHANNEL:
            return values[0];
        case GREEN_CHANNEL:
            return values[1];
        case BLUE_CHANNEL:
            return values[2];
        default:
            return 0;
    }
}

static void set_attribute_channel_value(
    CtkColorCorrection *ctk_color_correction,
    gint attribute,
    gint channel,
    gfloat value
)
{
    NvCtrlAttributeHandle *handle = ctk_color_correction->handle;
    gfloat v[3];
    
    v[0] = v[1] = v[2] = value;
    
    NvCtrlSetColorAttributes(handle, v, v, v, attribute | channel);
    
    g_signal_emit(ctk_color_correction, signals[CHANGED], 0);
}


#define RED   RED_CHANNEL_INDEX
#define GREEN GREEN_CHANNEL_INDEX
#define BLUE  BLUE_CHANNEL_INDEX

static void apply_parsed_attribute_list(
    CtkColorCorrection *ctk_color_correction,
    ParsedAttribute *p
)
{
    float c[3], b[3], g[3];
    char *this_display_name, *display_name;
    unsigned int attr = 0;
    
    this_display_name = NvCtrlGetDisplayName(ctk_color_correction->handle);
    
    c[0] = c[1] = c[2] = CONTRAST_DEFAULT;
    b[0] = b[1] = b[2] = BRIGHTNESS_DEFAULT;
    g[0] = g[1] = g[2] = GAMMA_DEFAULT;
    
    while (p) {

        display_name = NULL;

        if (!p->next) goto next_attribute;
        
        if (!(p->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE)) goto next_attribute;
        
        /*
         * if this parsed attribute's display name does not match the
         * current display name, then ignore
         */
        
        display_name = nv_standardize_screen_name(p->display, -1);
        
        if (strcmp(display_name, this_display_name) != 0) goto next_attribute;
        
        switch (p->attr & (ALL_VALUES | ALL_CHANNELS)) {
        case (CONTRAST_VALUE | RED_CHANNEL):     c[RED]   = p->fval; break;
        case (CONTRAST_VALUE | GREEN_CHANNEL):   c[GREEN] = p->fval; break;
        case (CONTRAST_VALUE | BLUE_CHANNEL):    c[BLUE]  = p->fval; break;
        case (CONTRAST_VALUE | ALL_CHANNELS):
            c[RED] = c[GREEN] = c[BLUE] = p->fval; break;
                                  
        case (BRIGHTNESS_VALUE | RED_CHANNEL):   b[RED]   = p->fval; break;
        case (BRIGHTNESS_VALUE | GREEN_CHANNEL): b[GREEN] = p->fval; break;
        case (BRIGHTNESS_VALUE | BLUE_CHANNEL):  b[BLUE]  = p->fval; break;
        case (BRIGHTNESS_VALUE | ALL_CHANNELS):
            b[RED] = b[GREEN] = b[BLUE] = p->fval; break;
            
        case (GAMMA_VALUE | RED_CHANNEL):        g[RED]   = p->fval; break;
        case (GAMMA_VALUE | GREEN_CHANNEL):      g[GREEN] = p->fval; break;
        case (GAMMA_VALUE | BLUE_CHANNEL):       g[BLUE]  = p->fval; break;
        case (GAMMA_VALUE | ALL_CHANNELS):
            g[RED] = g[GREEN] = g[BLUE] = p->fval; break;

        default:
            goto next_attribute;
        }
        
        attr |= (p->attr & (ALL_VALUES | ALL_CHANNELS));
        
    next_attribute:
        
        if (display_name) free(display_name);
        
        p = p->next;
    }

    free(this_display_name);

    if (attr) {
        NvCtrlSetColorAttributes(ctk_color_correction->handle, c, b, g, attr);
    }
    
} /* apply_parsed_attribute_list() */



GtkTextBuffer *ctk_color_correction_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);
    
    ctk_help_title(b, &i, "X Server Color Correction Help");

    ctk_help_heading(b, &i, "Active Color Channel");
    ctk_help_para(b, &i, __active_color_help);

    ctk_help_heading(b, &i, "Brightness, Contrast and Gamma");
    ctk_help_para(b, &i, "The Brightness, Contrast and Gamma sliders "
                  "allow you to adjust the brightness, contrast, "
                  "or gamma values for the selected color channel(s).  This "
                  "helps you to compensate "
                  "for variations in luminance between a source image and "
                  "its output on a display device. This is useful when "
                  "working with image processing applications to help "
                  "provide more accurate color reproduction of images (such "
                  "as photographs) when they are displayed on your "
                  "monitor.");

    ctk_help_para(b, &i, "Also, many 3D-accelerated games may appear too "
                  "dark to play.  Increasing the brightness and/or gamma "
                  "value equally across all channels will make these games "
                  "appear brighter, making them more playable.");
    
    ctk_help_para(b, &i, __color_curve_help);

    ctk_help_para(b, &i, "Note that the X Server Color Correction page uses "
                  "the XF86VidMode extension to manipulate the X screen's "
                  "color ramps.");
    
    ctk_help_heading(b, &i, "Reset Hardware Defaults");
    ctk_help_para(b, &i, __resest_button_help);

    ctk_help_finish(b);

    return b;
}
