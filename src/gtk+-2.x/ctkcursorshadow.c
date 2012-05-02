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

/*
 * The CursorShadow widget provides a way to enable and tweak the
 * parameters of the cursor shadow.  With the advent of the Xcursor
 * library and ARGB cursors, this is less interesting.
 *
 * Note that the cursor shadow and ARGB cursors cannot be used at the
 * same time, so if the user enables the cursor shadow but ARGB
 * cursors are currently in use, print a warning dialog box.
 *
 * TODO:
 *
 * - provide mechanism for configuring ARGB cursor themes, etc...
 */


#include <stdio.h>  /* sprintf */
#include <gtk/gtk.h>

#include "NvCtrlAttributes.h"

#include "ctkbanner.h"

#include "ctkcursorshadow.h"
#include "ctkscale.h"
#include "ctkhelp.h"
#include "ctkconstants.h"


static const char *__enable_cursor_shadow_help =
"The Enable Cursor Shadow checkbox enables cursor shadow functionality.  "
"Note that this functionality cannot be applied to ARGB cursors.";

static const char *__x_offset_help =
"The cursor shadow's X offset is the offset, in pixels, that the shadow "
"image will be shifted to the right from the real cursor image.";

static const char *__y_offset_help =
"The cursor shadow's Y offset is the offset, in pixels, that the shadow "
"image will be shifted down from the real cursor image.";

static const char *__alpha_help =
"The cursor shadow's alpha affects how transparent or opaque the cursor "
"shadow is.";

static const char *__color_selector_help =
"The Cursor Shadow Color Selector button toggles "
"the Cursor Shadow Color Selector window, which allows "
"you to select the color for the cursor shadow.";

static const char *__reset_button_help =
"The Reset Hardware Defaults button restores "
"the Cursor Shadow settings to their default values.";


/*
 * Define a table of defaults for each slider
 */

#define __CURSOR_SHADOW_X_OFFSET_DEFAULT 4
#define __CURSOR_SHADOW_Y_OFFSET_DEFAULT 2
#define __CURSOR_SHADOW_ALPHA_DEFAULT    64
#define __CURSOR_SHADOW_RED_DEFAULT      0
#define __CURSOR_SHADOW_GREEN_DEFAULT    0
#define __CURSOR_SHADOW_BLUE_DEFAULT     0

typedef struct {
    gint attribute;
    gint value;
} CursorShadowDefault;

#define X_OFFSET_INDEX 0
#define Y_OFFSET_INDEX 1
#define ALPHA_INDEX    2
#define RED_INDEX      3
#define GREEN_INDEX    4
#define BLUE_INDEX     5

static const CursorShadowDefault cursorShadowSliderDefaultsTable[] = {
    { NV_CTRL_CURSOR_SHADOW_X_OFFSET,
      __CURSOR_SHADOW_X_OFFSET_DEFAULT }, // X_OFFSET_INDEX
    { NV_CTRL_CURSOR_SHADOW_Y_OFFSET,
      __CURSOR_SHADOW_Y_OFFSET_DEFAULT }, // Y_OFFSET_INDEX
    { NV_CTRL_CURSOR_SHADOW_ALPHA,
      __CURSOR_SHADOW_ALPHA_DEFAULT    }, // ALPHA_INDEX
    { NV_CTRL_CURSOR_SHADOW_RED,
      __CURSOR_SHADOW_RED_DEFAULT      }, // RED_INDEX
    { NV_CTRL_CURSOR_SHADOW_GREEN,
      __CURSOR_SHADOW_GREEN_DEFAULT    }, // GREEN_INDEX
    { NV_CTRL_CURSOR_SHADOW_BLUE,
      __CURSOR_SHADOW_BLUE_DEFAULT     }, // BLUE_INDEX
};



/* local prototypes */

static void color_toggled(GtkWidget *widget, gpointer user_data);

static void shadow_toggled(GtkWidget *widget, gpointer user_data);

static GtkWidget *create_slider(CtkCursorShadow *ctk_cursor_shadow,
                                GtkWidget *vbox, const gchar *name,
                                const char *help, gint attribute);

static void reset_defaults(GtkButton *button, gpointer user_data);

static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data);

static void set_cursor_shadow_sensitivity(CtkCursorShadow *ctk_cursor_shadow,
                                          gboolean enabled);
static gboolean
get_initial_reset_button_sensitivity(CtkCursorShadow *ctk_cursor_shadow);

static void init_color_selector(CtkCursorShadow *ctk_cursor_shadow);

static void 
color_selector_close_button_clicked(GtkButton *button, gpointer user_data);

static gboolean
color_selector_window_destroy(GtkWidget *widget, GdkEvent *event,
                              gpointer user_data);

static void post_color_selector_changed(CtkCursorShadow *ctk_cursor_shadow,
                                        gint red, gint green, gint blue);

static void color_selector_changed(GtkColorSelection *colorselection,
                                   gpointer user_data);
static guint16 nvctrl2gtk_color(NVCTRLAttributeValidValuesRec *range, int val);
static int gtk2nvctrl_color(NVCTRLAttributeValidValuesRec *range,
                            guint16 value);
static int get_value_and_range(CtkCursorShadow *ctk_cursor_shadow,
                               gint attribute, guint16 *value,
                               NVCTRLAttributeValidValuesRec *range);

static void cursor_shadow_update_received(GtkObject *object,
                                          gpointer arg1, gpointer user_data);

static void adjustment_update_received(GtkObject *object,
                                       gpointer arg1, gpointer user_data);

static void color_update_received(GtkObject *object,
                                  gpointer arg1, gpointer user_data);

GType ctk_cursor_shadow_get_type(void)
{
    static GType ctk_cursor_shadow_type = 0;
    
    if (!ctk_cursor_shadow_type) {
        static const GTypeInfo ctk_cursor_shadow_info = {
            sizeof (CtkCursorShadowClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkCursorShadow),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };
        
        ctk_cursor_shadow_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkCursorShadow", &ctk_cursor_shadow_info, 0);
    }
    
    return ctk_cursor_shadow_type;
}


/*
 * ctk_cursor_shadow_new() - constructor for the CursorShadow widget
 */

GtkWidget* ctk_cursor_shadow_new(NvCtrlAttributeHandle *handle,
                                 CtkConfig *ctk_config,
                                 CtkEvent *ctk_event)
{
    GObject *object;
    CtkCursorShadow *ctk_cursor_shadow;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *alignment;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *check_button;
    GtkWidget *label;
    GdkColor   color;
    ReturnStatus ret;
    gint enabled;
    gint red, green, blue;
    char str[16];

    /* check to see if we can support cursor shadow */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_CURSOR_SHADOW, &enabled);
    
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* create the cursor shadow object */

    object = g_object_new(CTK_TYPE_CURSOR_SHADOW, NULL);
    ctk_cursor_shadow = CTK_CURSOR_SHADOW(object);
    
    ctk_cursor_shadow->handle = handle;
    ctk_cursor_shadow->ctk_config = ctk_config;
    ctk_cursor_shadow->ctk_event = ctk_event;

    gtk_box_set_spacing(GTK_BOX(ctk_cursor_shadow), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_CURSOR_SHADOW);
    gtk_box_pack_start(GTK_BOX(ctk_cursor_shadow), banner, FALSE, FALSE, 0);

    /* vbox */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);

    /* enable cursor checkbox */

    check_button =
        gtk_check_button_new_with_label("Enable Cursor Shadow");
    
    ctk_cursor_shadow->cursor_shadow_check_button = check_button;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enabled);

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(shadow_toggled),
                     (gpointer) ctk_cursor_shadow);

    /* receive the event when another NV-CONTROL client changes this */
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW),
                     G_CALLBACK(cursor_shadow_update_received),
                     (gpointer) ctk_cursor_shadow);
    
    ctk_config_set_tooltip(ctk_config, check_button,
                           __enable_cursor_shadow_help);
    
    /* sliders */

    ctk_cursor_shadow->scales[X_OFFSET_INDEX] =
        create_slider(ctk_cursor_shadow, vbox, "X Offset", __x_offset_help,
                      NV_CTRL_CURSOR_SHADOW_X_OFFSET);
    
    ctk_cursor_shadow->scales[Y_OFFSET_INDEX] =
        create_slider(ctk_cursor_shadow, vbox, "Y Offset", __y_offset_help,
                      NV_CTRL_CURSOR_SHADOW_Y_OFFSET);
    
    ctk_cursor_shadow->scales[ALPHA_INDEX] =
        create_slider(ctk_cursor_shadow, vbox, "Alpha", __alpha_help,
                      NV_CTRL_CURSOR_SHADOW_ALPHA);
    
    if (!ctk_cursor_shadow->scales[X_OFFSET_INDEX] ||
        !ctk_cursor_shadow->scales[Y_OFFSET_INDEX] ||
        !ctk_cursor_shadow->scales[ALPHA_INDEX]) return NULL;
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_X_OFFSET),
                     G_CALLBACK(adjustment_update_received),
                     (gpointer) ctk_cursor_shadow);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_Y_OFFSET),
                     G_CALLBACK(adjustment_update_received),
                     (gpointer) ctk_cursor_shadow);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_ALPHA),
                     G_CALLBACK(adjustment_update_received),
                     (gpointer) ctk_cursor_shadow);
    

    /* "Color Shadow Selector" button */
    
    ctk_cursor_shadow->color_selector_button =
        gtk_toggle_button_new();

    /* Cursor Shadow Color Box */
    
    frame = gtk_aspect_frame_new(NULL, 0, 0, 1, FALSE);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

    gtk_container_set_border_width(GTK_CONTAINER(frame), 1);


    ctk_cursor_shadow->cursor_shadow_bg = gtk_event_box_new();

    gtk_widget_set_size_request(ctk_cursor_shadow->cursor_shadow_bg, 10, 10);

    /* Grab current cursor shadow color */

    NvCtrlGetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_RED, &red);
    
    NvCtrlGetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_GREEN, &green);
    
    NvCtrlGetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_BLUE, &blue);
    
    sprintf(str, "#%2.2X%2.2X%2.2X", red, green, blue);

    gdk_color_parse (str, &color);

    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_NORMAL, &color);

    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_ACTIVE, &color);

    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_PRELIGHT, &color);

    /* pack cursor color selector button */

    gtk_container_add(GTK_CONTAINER(frame),
                      ctk_cursor_shadow->cursor_shadow_bg);
    
    label = gtk_label_new("Cursor Shadow Color Selector");

    hbox  = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start( GTK_BOX(hbox), frame, TRUE, TRUE, 2);

    gtk_box_pack_end( GTK_BOX(hbox), label, FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(ctk_cursor_shadow->color_selector_button),
                      hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_cursor_shadow->color_selector_button,
                       FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_cursor_shadow->color_selector_button),
         FALSE);
    
    g_signal_connect(G_OBJECT(ctk_cursor_shadow->color_selector_button),
                     "clicked", G_CALLBACK(color_toggled),
                     (gpointer) ctk_cursor_shadow);
    
    ctk_config_set_tooltip(ctk_config,
                           ctk_cursor_shadow->color_selector_button,
                           __color_selector_help);
   
    /* Color Selector */

    init_color_selector(ctk_cursor_shadow);

    
    /* reset button */

    label = gtk_label_new("Reset Hardware Defaults");
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_cursor_shadow->reset_button = gtk_button_new();

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_cursor_shadow->reset_button), hbox);
    
    g_signal_connect(G_OBJECT(ctk_cursor_shadow->reset_button), "clicked",
                     G_CALLBACK(reset_defaults), (gpointer) ctk_cursor_shadow);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_cursor_shadow->reset_button);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);
 
    ctk_config_set_tooltip(ctk_config, ctk_cursor_shadow->reset_button,
                           __reset_button_help);

    /* set the sensitivity of the scales and the reset button */
    
    ctk_cursor_shadow->reset_button_sensitivity =
        get_initial_reset_button_sensitivity(ctk_cursor_shadow);

    set_cursor_shadow_sensitivity(ctk_cursor_shadow, enabled);

    /* finally, show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_cursor_shadow));

    return GTK_WIDGET(ctk_cursor_shadow);
    
} /* ctk_cursor_shadow_new() */



/*
 * color_toggled() - called when the shadow color check button has
 * been toggled.
 */

static void color_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    gboolean enabled;
    
    /* get the enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {
        gtk_widget_show_all(ctk_cursor_shadow->color_selector_window);
    } else {
        gtk_widget_hide(ctk_cursor_shadow->color_selector_window);
    }
    
    ctk_config_statusbar_message(ctk_cursor_shadow->ctk_config,
                                 "Cursor Shadow Color Selector %s.",
                                 enabled ? "enabled" : "disabled");

} /* color_toggled() */



/*
 * post_shadow_toggled() - helper function for shadow_toggled() and
 * cursor_shadow_update_received(); this does whatever work is
 * necessary after the cursor shadow enable/disable state has been
 * toggled -- update the reset button's sensitivity and post a
 * statusbar message.
 */

static void post_shadow_toggled(CtkCursorShadow *ctk_cursor_shadow,
                                gboolean enabled)
{
    /* update the sliders and reset button sensitivity */
    
    set_cursor_shadow_sensitivity(ctk_cursor_shadow, enabled);
    
    /* update the status bar */
    
    ctk_config_statusbar_message(ctk_cursor_shadow->ctk_config,
                                 "Cursor Shadow %s.",
                                 enabled ? "enabled" : "disabled");
    
} /* post_shadow_toggled() */



/*
 * shadow_toggled() - callback when the "Enable Cursor Shadow"
 * checkbox is toggled.
 */

static void shadow_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    gboolean enabled;
    
    /* get the enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* update the server as appropriate */

    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW, enabled);
    
    post_shadow_toggled(ctk_cursor_shadow, enabled);

} /* shadow_toggled() */



/*
 * create_slider() - a single slider
 */

static GtkWidget *create_slider(CtkCursorShadow *ctk_cursor_shadow,
                                GtkWidget *vbox, const gchar *name,
                                const char *help, gint attribute)
{
    GtkObject *adjustment;
    GtkWidget *scale, *widget;
    gint min, max, val, step_incr, page_incr;
    NVCTRLAttributeValidValuesRec range;
    ReturnStatus ret;

    /* get the attribute value */
    
    ret = NvCtrlGetAttribute(ctk_cursor_shadow->handle, attribute, &val);

    if (ret != NvCtrlSuccess) return NULL;
    
    /* get the range for the attribute */
    
    ret = NvCtrlGetValidAttributeValues(ctk_cursor_shadow->handle,
                                        attribute, &range);
    
    if (ret != NvCtrlSuccess) return NULL;
    
    if (range.type != ATTRIBUTE_TYPE_RANGE) return NULL;
    
    min = range.u.range.min;
    max = range.u.range.max;
    
    step_incr = ((max) - (min))/250;
    if (step_incr <= 0) step_incr = 1;
    
    page_incr = ((max) - (min))/25;
    if (page_incr <= 0) page_incr = 1;
    
    /* create the adjustment and scale */
    
    adjustment = gtk_adjustment_new(val, min, max,
                                    step_incr, page_incr, 0.0);
    
    g_object_set_data(G_OBJECT(adjustment), "cursor_shadow_attribute",
                      GINT_TO_POINTER(attribute));
    
    g_signal_connect(G_OBJECT(adjustment), "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_cursor_shadow);

    scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), name,
                          ctk_cursor_shadow->ctk_config, G_TYPE_INT);

    /* pack the scale in the vbox */

    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
    
    /* set the tooltip for the slider */

    widget = CTK_SCALE(scale)->gtk_scale;
    ctk_config_set_tooltip(ctk_cursor_shadow->ctk_config, widget, help);

    return scale;

} /* create_slider() */



/*
 * reset_slider() - reset the slider; called by reset_defaults after
 * the reset button was pressed.
 */

static void reset_slider(CtkCursorShadow *ctk_cursor_shadow,
                         GtkWidget *widget, gint attribute, gint value)
{
    GtkAdjustment *adjustment;
    
    adjustment = CTK_SCALE(widget)->gtk_adjustment;

    /* set the default value for this attribute */

    NvCtrlSetAttribute(ctk_cursor_shadow->handle, attribute, value);
    
    /* reset the slider, but ignore any signals while we reset it */

    g_signal_handlers_block_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(adjustment_value_changed), NULL);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), value);
    
    g_signal_handlers_unblock_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(adjustment_value_changed), NULL);

} /* reset_slider() */



/*
 * reset_defaults() - called when the "reset defaults" button is
 * pressed; clears the sliders and the reset button.
 */

static void reset_defaults(GtkButton *button, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    GdkColor color;
    int i;

    for (i = 0; i < 3; i++) {
        reset_slider(ctk_cursor_shadow, ctk_cursor_shadow->scales[i],
                     cursorShadowSliderDefaultsTable[i].attribute,
                     cursorShadowSliderDefaultsTable[i].value);
    }
    
    /* reset color the selector */

    g_signal_handlers_block_matched
        (G_OBJECT(ctk_cursor_shadow->color_selector),
         G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(color_selector_changed), NULL);

    color.red = nvctrl2gtk_color
        (&ctk_cursor_shadow->red_range,
         cursorShadowSliderDefaultsTable[RED_INDEX].value);
    
    color.green = nvctrl2gtk_color
        (&ctk_cursor_shadow->green_range,
         cursorShadowSliderDefaultsTable[GREEN_INDEX].value);
    
    color.blue = nvctrl2gtk_color
        (&ctk_cursor_shadow->blue_range,
         cursorShadowSliderDefaultsTable[BLUE_INDEX].value);

    gtk_color_selection_set_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);

    post_color_selector_changed(ctk_cursor_shadow,
                                color.red, color.green, color.blue);
    
    g_signal_handlers_unblock_matched
        (G_OBJECT(ctk_cursor_shadow->color_selector),
         G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(color_selector_changed), NULL);
    
    /* send the default colors to the server */

    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_RED,
                       cursorShadowSliderDefaultsTable[RED_INDEX].value);

    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_GREEN,
                       cursorShadowSliderDefaultsTable[GREEN_INDEX].value);

    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_BLUE,
                       cursorShadowSliderDefaultsTable[BLUE_INDEX].value);

    /* clear the sensitivity of the reset button */

    ctk_cursor_shadow->reset_button_sensitivity = FALSE;
    
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);

    ctk_config_statusbar_message(ctk_cursor_shadow->ctk_config,
                                 "Reset Cursor Shadow hardware defaults.");

} /* reset_defaults() */



/*
 * post_adjustment_value_changed() - helper function for
 * adjustment_value_changed() and adjustment_update_received(); this
 * does whatever work is necessary after an adjustment has been
 * changed -- update the reset button's sensitivity and post a
 * statusbar message.
 */

static void post_adjustment_value_changed(CtkCursorShadow *ctk_cursor_shadow,
                                          gint attribute, gint value)
{
    gchar *attribute_str;
    
    /* make the reset button sensitive */

    ctk_cursor_shadow->reset_button_sensitivity = TRUE;
    gtk_widget_set_sensitive
        (GTK_WIDGET(ctk_cursor_shadow->reset_button), TRUE);
    
    /*
     * get a string description of this attribute (for use in the
     * statusbar message)
     */

    switch(attribute) {
    case NV_CTRL_CURSOR_SHADOW_X_OFFSET: attribute_str = "X Offset"; break;
    case NV_CTRL_CURSOR_SHADOW_Y_OFFSET: attribute_str = "Y Offset"; break;
    case NV_CTRL_CURSOR_SHADOW_ALPHA:    attribute_str = "Alpha";    break;
    default:
        return;
    }
    
    /* update the status bar */

    ctk_config_statusbar_message(ctk_cursor_shadow->ctk_config,
                                 "Cursor Shadow %s set to %d.",
                                 attribute_str, value);
    
} /* post_adjustment_value_changed() */



/*
 * adjustment_value_changed() - called when any of the adjustments are
 * changed.
 */

static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    gint attribute, value;
    
    /* retrieve which attribute this adjustment controls */

    attribute = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adjustment),
                                                  "cursor_shadow_attribute"));
    /* get the current value in the adjustment */
    
    value = (gint) adjustment->value;

    /* send the new value to the server */

    NvCtrlSetAttribute(ctk_cursor_shadow->handle, attribute, value);

    post_adjustment_value_changed(ctk_cursor_shadow, attribute, value);

} /* adjustment_value_changed() */



/*
 * set_cursor_shadow_sensitivity() - set the sensitivity for the
 * sliders and the reset button, based on whether the cursor shadow is
 * enabled.
 */

static void set_cursor_shadow_sensitivity(CtkCursorShadow *ctk_cursor_shadow,
                                          gboolean enabled)
{
    int i;

    for (i = 0; i < 3; i++) {
        gtk_widget_set_sensitive(ctk_cursor_shadow->scales[i], enabled);
    }
    
    gtk_widget_set_sensitive(ctk_cursor_shadow->color_selector, enabled);

    gtk_widget_set_sensitive(ctk_cursor_shadow->color_selector_button, enabled);

    /*
     * We separately track whether the reset button should be
     * sensitive because, unlike the sliders (which should be
     * sensitive whenever CursorShadow is enabled), the reset button
     * should only be sensitive when the CursorShadow is enabled *and*
     * when the sliders have been altered.
     *
     * So, here we only want to make the reset button sensitive if
     * CursorShadow is enabled and our separate tracking says the
     * reset button should be sensitive.
     */

    if (enabled && ctk_cursor_shadow->reset_button_sensitivity) {
        enabled = TRUE;
    } else {
        enabled = FALSE;
    }
        
    gtk_widget_set_sensitive(GTK_WIDGET(ctk_cursor_shadow->reset_button),
                             enabled);

} /* set_cursor_shadow_sensitivity() */



/*
 * get_initial_reset_button_sensitivity() - determine if all the
 * sliders are in their default position; this is done by looking
 * through the defaults table and comparing the default value with the
 * current value.  If any of the values are different, return TRUE to
 * indicate that the reset button should be sensitive.  If all the
 * values were the same, return FALSE to indicate that the reset
 * button should not be sensitive.
 */

static gboolean
get_initial_reset_button_sensitivity(CtkCursorShadow *ctk_cursor_shadow)
{
    CtkScale *ctk_scale;
    gint i, value, red, green, blue;
    GdkColor color;
    
    for (i = 0; i < 3; i++) {
        ctk_scale = CTK_SCALE(ctk_cursor_shadow->scales[i]);
        value = gtk_adjustment_get_value(ctk_scale->gtk_adjustment);
        if (value != cursorShadowSliderDefaultsTable[i].value) {
            return TRUE;
        }
    }
    
    /* check if the color selector needs resetting */

    gtk_color_selection_get_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);

    /* convert the values from GTK ranges [0,65536) to NV-CONTROL ranges */

    red   = gtk2nvctrl_color(&ctk_cursor_shadow->red_range,   color.red);
    green = gtk2nvctrl_color(&ctk_cursor_shadow->green_range, color.green);
    blue  = gtk2nvctrl_color(&ctk_cursor_shadow->blue_range,  color.blue);
    
    /* check the current colors against the defaults */

    if ((red != cursorShadowSliderDefaultsTable[RED_INDEX].value) ||
        (green != cursorShadowSliderDefaultsTable[GREEN_INDEX].value) ||
        (blue != cursorShadowSliderDefaultsTable[BLUE_INDEX].value)) {
        return TRUE;
    }
    
    return FALSE;

} /* get_initial_reset_button_sensitivity() */



/*
 * init_color_selector() - initialize the color selector window
 */

static void init_color_selector(CtkCursorShadow *ctk_cursor_shadow)
{
    GdkColor color;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *button;
    GtkWidget *alignment;
    guint ret;

    /* create the color selector window */
    
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Cursor Shadow Color Selector");
    gtk_container_set_border_width(GTK_CONTAINER(window), CTK_WINDOW_PAD);

    /* create a vbox to pack all the window contents in */

    vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    /* add a banner */
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    banner = ctk_banner_image_new(BANNER_ARTWORK_CURSOR_SHADOW);
    gtk_box_pack_start(GTK_BOX(hbox), banner, TRUE, TRUE, 0);
    
    /* create the color selector */

    ctk_cursor_shadow->color_selector = gtk_color_selection_new();
    gtk_box_pack_start(GTK_BOX(vbox), ctk_cursor_shadow->color_selector,
                       TRUE, TRUE, 0);

    /* place a horizontal separator */
    
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);
    
    /* create and place the close button */
    
    hbox = gtk_hbox_new(FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);
    
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(color_selector_close_button_clicked),
                     (gpointer) ctk_cursor_shadow);
    
    /* handle destructive events to the window */
    
    g_signal_connect(G_OBJECT(window), "destroy-event",
                     G_CALLBACK(color_selector_window_destroy),
                     (gpointer) ctk_cursor_shadow);
    g_signal_connect(G_OBJECT(window), "delete-event",
                     G_CALLBACK(color_selector_window_destroy),
                     (gpointer) ctk_cursor_shadow);
    
    
    /* update settings in the color selector */
    
    
    /* turn off the palette and alpha */
        
    gtk_color_selection_set_has_opacity_control
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), FALSE);
    
    gtk_color_selection_set_has_palette
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), FALSE);

    g_object_set(G_OBJECT(ctk_cursor_shadow->color_selector),
                 "has-opacity-control", FALSE, NULL);
    
    g_object_set(G_OBJECT(ctk_cursor_shadow->color_selector),
                 "has-palette", FALSE, NULL);
    
    /* update the color continuously XXX this is deprecated? */
    
    gtk_color_selection_set_update_policy
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector),
         GTK_UPDATE_CONTINUOUS);
    
    /* retrieve the current values, and initialize the ranges */

    ret = 0;
   
    ret |= get_value_and_range(ctk_cursor_shadow,
                               NV_CTRL_CURSOR_SHADOW_RED,
                               &color.red,
                               &ctk_cursor_shadow->red_range);
    
    ret |= get_value_and_range(ctk_cursor_shadow,
                               NV_CTRL_CURSOR_SHADOW_GREEN,
                               &color.green,
                               &ctk_cursor_shadow->green_range);
    
    ret |= get_value_and_range(ctk_cursor_shadow,
                               NV_CTRL_CURSOR_SHADOW_BLUE,
                               &color.blue,
                               &ctk_cursor_shadow->blue_range);
    
    /* something failed? give up */
    
    if (ret) return;
    
    g_signal_connect(G_OBJECT(ctk_cursor_shadow->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_RED),
                     G_CALLBACK(color_update_received),
                     (gpointer) ctk_cursor_shadow);

    g_signal_connect(G_OBJECT(ctk_cursor_shadow->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_GREEN),
                     G_CALLBACK(color_update_received),
                     (gpointer) ctk_cursor_shadow);
    
    g_signal_connect(G_OBJECT(ctk_cursor_shadow->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURSOR_SHADOW_BLUE),
                     G_CALLBACK(color_update_received),
                     (gpointer) ctk_cursor_shadow);
    
    gtk_color_selection_set_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);
    
    /* register the callback */
    
    g_signal_connect(G_OBJECT(ctk_cursor_shadow->color_selector),
                     "color-changed",
                     G_CALLBACK(color_selector_changed),
                     (gpointer) ctk_cursor_shadow);

    ctk_cursor_shadow->color_selector_window = window;

} /* init_color_selector() */


static void 
color_selector_close_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_cursor_shadow->color_selector_button),
         FALSE);
    
} /* color_selector_close_button_clicked() */



static gboolean
color_selector_window_destroy(GtkWidget *widget, GdkEvent *event,
                              gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_cursor_shadow->color_selector_button),
         FALSE);
    
    return TRUE;

} /* color_selector_window_destroy() */




/*
 * get_value_and_range() - helper function for init_color_selector();
 * retrieve the current value and the valid range for the given
 * attribute.  On success, return 0; on failure, return 1 (so that
 * init_color_selector() can accumulate failures by or'ing the return
 * values together).
 */

static int get_value_and_range(CtkCursorShadow *ctk_cursor_shadow,
                               gint attribute, guint16 *value,
                               NVCTRLAttributeValidValuesRec *range)
{
    ReturnStatus ret0, ret1;
    gint val;

    ret0 = NvCtrlGetAttribute(ctk_cursor_shadow->handle, attribute, &val);
    
    ret1 = NvCtrlGetValidAttributeValues(ctk_cursor_shadow->handle,
                                         attribute, range);
    
    if ((ret0 == NvCtrlSuccess) &&
        (ret1 == NvCtrlSuccess) &&
        (range->type == ATTRIBUTE_TYPE_RANGE)) {
        
        *value = nvctrl2gtk_color(range, val);
        
        return 0;
    }

    return 1;

} /* get_value_and_range() */



/*
 * post_color_selector_changed() - helper function for
 * color_selector_changed() and color_update_received(); this does
 * whatever work is necessary after the color selector has been
 * changed -- update the reset button's sensitivity and post a
 * statusbar message.
 */

static void post_color_selector_changed(CtkCursorShadow *ctk_cursor_shadow,
                                        gint red, gint green, gint blue)
{
    GdkColor color;
    char     str[16];
    sprintf(str, "#%2.2X%2.2X%2.2X", red, green, blue);


    /* Update the color square */

    gdk_color_parse(str, &color);
    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_NORMAL, &color);

    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_ACTIVE, &color);

    gtk_widget_modify_bg(ctk_cursor_shadow->cursor_shadow_bg,
                         GTK_STATE_PRELIGHT, &color);

    /* make the reset button sensitive */
    
    ctk_cursor_shadow->reset_button_sensitivity = TRUE;
    gtk_widget_set_sensitive
        (GTK_WIDGET(ctk_cursor_shadow->reset_button), TRUE);
    
    /* update the status bar */

    ctk_config_statusbar_message(ctk_cursor_shadow->ctk_config,
                                 "Cursor Shadow Color set to "
                                 "[R:%d G:%d B:%d].", red, green, blue);
    
} /* post_color_selector_changed() */



/*
 * color_selector_changed() - called whenever the color selector
 * changes
 */

static void color_selector_changed(GtkColorSelection *colorselection,
                                   gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    GdkColor color;
    gint red, green, blue;

    /* retrieve the current color */

    gtk_color_selection_get_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);

    /* convert the values from GTK ranges [0,65536) to NV-CONTROL ranges */

    red   = gtk2nvctrl_color(&ctk_cursor_shadow->red_range,   color.red);
    green = gtk2nvctrl_color(&ctk_cursor_shadow->green_range, color.green);
    blue  = gtk2nvctrl_color(&ctk_cursor_shadow->blue_range,  color.blue);
    
    /* send the values to the server */

    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_RED, red);
    
    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_GREEN, green);
    
    NvCtrlSetAttribute(ctk_cursor_shadow->handle,
                       NV_CTRL_CURSOR_SHADOW_BLUE, blue);
    
    post_color_selector_changed(ctk_cursor_shadow, red, green, blue);

} /* color_selector_changed() */



/*
 * nvctrl2gtk_color() - convert a color value in the NV-CONTROL range
 * (given by the range argument) to the GTK color range [0,65536)
 */

static guint16 nvctrl2gtk_color(NVCTRLAttributeValidValuesRec *range, int val)
{
    gdouble d0, d1, x0, x1;

    d0 = (double) (range->u.range.max - range->u.range.min);
    d1 = 65535.0;
    x0 = (double) (val - range->u.range.min);
    x1 = x0 * (d1/d0);

    return (guint16) x1;

} /* nvctrl2gtk_color() */



/*
 * gtk2nvctrl_color() - convert a color value in the GTK range
 * [0,65536) to the NV-CONTROL range (given by the range argument).
 */

static int gtk2nvctrl_color(NVCTRLAttributeValidValuesRec *range,
                            guint16 value)
{
    gdouble d0, d1, x0, x1;

    d0 = (double) (range->u.range.max - range->u.range.min);
    d1 = 65535.0;
    x1 = (double) value;
    x0 = x1 * (d0/d1);
   
    return ((guint16) x0) + range->u.range.min;

} /* gtk2nvctrl_color() */



/*
 * cursor_shadow_update_received() - callback function for when the
 * NV_CTRL_CURSOR_SHADOW attribute is changed by another NV-CONTROL
 * client.
 */

static void cursor_shadow_update_received(GtkObject *object,
                                          gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    gboolean enabled = event_struct->value;
    GtkWidget *w = ctk_cursor_shadow->cursor_shadow_check_button;

    g_signal_handlers_block_by_func(G_OBJECT(w),
                                    G_CALLBACK(shadow_toggled),
                                    (gpointer) ctk_cursor_shadow);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), enabled);

    post_shadow_toggled(ctk_cursor_shadow, enabled);
    
    g_signal_handlers_unblock_by_func(G_OBJECT(w),
                                      G_CALLBACK(shadow_toggled),
                                      (gpointer) ctk_cursor_shadow);
    
} /* cursor_shadow_update_received() */



/*
 * set_reset_button() - helper function for
 * adjustment_update_received() and color_update_received(); evaluate
 * whether any of the attributes have non-default values, and set the
 * sensitivity of the reset button appropriately (ie: only make the
 * button sensitive if any attribute has a non-default value).
 */

static void set_reset_button(CtkCursorShadow *ctk_cursor_shadow)
{
    ctk_cursor_shadow->reset_button_sensitivity =
        get_initial_reset_button_sensitivity(ctk_cursor_shadow);
        
    gtk_widget_set_sensitive(GTK_WIDGET(ctk_cursor_shadow->reset_button),
                             ctk_cursor_shadow->reset_button_sensitivity);

} /* set_reset_button() */



/*
 * adjustment_update_received() - callback function that handles an
 * event where another NV-CONTROL client modified any of the cursor
 * shadow attributes that we have sliders for (x offset, y offset, and
 * alpha).  In that case, we need to update the slider with the new
 * value.
 */

static void adjustment_update_received(GtkObject *object,
                                       gpointer arg1, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    gint value = event_struct->value;
    gint attribute = event_struct->attribute;
    gint index;
    CtkScale *ctk_scale;
    GtkAdjustment *adjustment;
    
    /* choose the index into the scales array */

    switch (event_struct->attribute) {
    case NV_CTRL_CURSOR_SHADOW_X_OFFSET: index = X_OFFSET_INDEX; break;
    case NV_CTRL_CURSOR_SHADOW_Y_OFFSET: index = Y_OFFSET_INDEX; break;
    case NV_CTRL_CURSOR_SHADOW_ALPHA:    index = ALPHA_INDEX; break;
    default:
        return;
    }
    
    /* get the appropriate adjustment */

    ctk_scale = CTK_SCALE(ctk_cursor_shadow->scales[index]);
    adjustment = GTK_ADJUSTMENT(ctk_scale->gtk_adjustment);

    /* update the adjustment with the new value */
    
    g_signal_handlers_block_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(adjustment_value_changed), NULL);
    
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), value);
    
    post_adjustment_value_changed(ctk_cursor_shadow, attribute, value);
    
    g_signal_handlers_unblock_matched
        (G_OBJECT(adjustment), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(adjustment_value_changed), NULL);
    
    /* update the state of the reset button */
    
    set_reset_button(ctk_cursor_shadow);

} /* adjustment_update_received() */



/*
 * color_update_received() - callback function that handles an event
 * where another NV-CONTROL client modified the cursor shadow color.
 * In that case, we need to retrieve the current color, update the
 * appropriate channel with the new value, and update the color
 * selector with the new color.
 */

static void color_update_received(GtkObject *object,
                                  gpointer arg1, gpointer user_data)
{
    CtkCursorShadow *ctk_cursor_shadow = CTK_CURSOR_SHADOW(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    gint red, green, blue;
    GdkColor color;

    /* retrieve the current color */

    gtk_color_selection_get_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);

    /* convert the values from GTK ranges [0,65536) to NV-CONTROL ranges */

    red   = gtk2nvctrl_color(&ctk_cursor_shadow->red_range,   color.red);
    green = gtk2nvctrl_color(&ctk_cursor_shadow->green_range, color.green);
    blue  = gtk2nvctrl_color(&ctk_cursor_shadow->blue_range,  color.blue);

    /* modify the color, keying off of which attribute was updated */

    switch(event_struct->attribute) {
    case NV_CTRL_CURSOR_SHADOW_RED:
        red = event_struct->value;
        color.red = nvctrl2gtk_color(&ctk_cursor_shadow->red_range, red);
        break;
    case NV_CTRL_CURSOR_SHADOW_GREEN:
        green = event_struct->value;
        color.green = nvctrl2gtk_color(&ctk_cursor_shadow->green_range, green);
        break;
    case NV_CTRL_CURSOR_SHADOW_BLUE:
        blue = event_struct->value;
        color.blue = nvctrl2gtk_color(&ctk_cursor_shadow->blue_range, blue);
        break;
    default:
        return;
    }
    
    /* update the color selector*/

    g_signal_handlers_block_matched
        (G_OBJECT(ctk_cursor_shadow->color_selector),
         G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(color_selector_changed), NULL);

    gtk_color_selection_set_current_color
        (GTK_COLOR_SELECTION(ctk_cursor_shadow->color_selector), &color);
    
    post_color_selector_changed(ctk_cursor_shadow, red, green, blue);

    g_signal_handlers_unblock_matched
        (G_OBJECT(ctk_cursor_shadow->color_selector),
         G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
         G_CALLBACK(color_selector_changed), NULL);
    
    /* update the state of the reset button */

    set_reset_button(ctk_cursor_shadow);

} /* color_update_received() */



/*
 * ctk_cursor_shadow_create_help() - 
 */

GtkTextBuffer *
ctk_cursor_shadow_create_help(GtkTextTagTable *table,
                              CtkCursorShadow *ctk_cursor_shadow)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Cursor Shadow Help");

    ctk_help_para(b, &i, "The Cursor Shadow page allows you to configure "
                  "a shadow beneath X core cursors.  This extends the "
                  "functionality exposed with the \"CursorShadow\" "
                  "X config file option.");

    ctk_help_para(b, &i, "Note that this functionality cannot be applied "
                  "to ARGB cursors, which already have their own built-in "
                  "shadows.  Most recent distributions and desktop "
                  "environments enable ARGB cursors by default.  If you wish "
                  "to disable ARGB cursors, add the line "
                  "\"Xcursor.core:true\" to your ~/.Xresources file.");
    
    ctk_help_heading(b, &i, "Enable Cursor Shadow");
    ctk_help_para(b, &i, __enable_cursor_shadow_help);

    ctk_help_heading(b, &i, "Cursor Shadow X Offset");
    ctk_help_para(b, &i, "The cursor shadow's X offset is the offset, "
                  "in pixels, that the shadow image will be shifted to the "
                  "right from the real cursor image.  This functionality can "
                  "also be configured with the \"CursorShadowXOffset\" X "
                  "config file option.");

    ctk_help_heading(b, &i, "Cursor Shadow Y Offset");
    ctk_help_para(b, &i, "The cursor shadow's Y offset is the offset, "
                  "in pixels, that the shadow image will be shifted down "
                  "from the real cursor image.  This functionality can "
                  "also be configured with the \"CursorShadowYOffset\" X "
                  "config file option.");

    ctk_help_heading(b, &i, "Cursor Shadow Alpha");
    ctk_help_para(b, &i, "The cursor shadow's alpha affects how transparent "
                  "or opaque the cursor shadow is.  This functionality can "
                  "also be configured with the \"CursorShadowAlpha\" X "
                  "config file option.");
    
    ctk_help_heading(b, &i, "Cursor Shadow Color Selector");
    ctk_help_para(b, &i, __color_selector_help);
    
    ctk_help_heading(b, &i, "Reset Hardware Defaults");
    ctk_help_para(b, &i, __reset_button_help);

    ctk_help_finish(b);

    return b;

} /* ctk_cursor_shadow_create_help() */
