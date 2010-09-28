/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkcolorcontrols.h"

/* function prototypes */
static gboolean build_color_space_table(CtkColorControls *ctk_color_controls,
                                        NVCTRLAttributeValidValuesRec valid);

static gint map_nvctrl_value_to_table(CtkColorControls *ctk_color_controls,
                                      gint val);

static Bool update_color_space_menu_info(gpointer user_data);

static void color_space_menu_changed(GtkOptionMenu *color_space_menu,
                                     gpointer user_data);
static void color_range_menu_changed(GtkOptionMenu *color_range_menu,
                                     gpointer user_data);

static void color_control_update_received(GtkObject *object, gpointer arg1,
                                          gpointer user_data);

static
void post_color_range_update(CtkColorControls *ctk_color_controls,
                             gint color_range);

static
void post_color_space_update(CtkColorControls *ctk_color_controls,
                             gint color_space);

/* macros */
#define FRAME_PADDING 5

/* help text */
static const char * __color_controls_help =
"The Color Controls allow changing the color space and color range "
"of the display device.  The possible values for Color Space vary "
"depending on the capabilities of the display device and the GPU, but "
"may contain \"RGB\", \"YCbCr422\", and \"YCbCr444\".  The possible values "
"for Color Range are \"Limited\" and \"Full\".";

GType ctk_color_controls_get_type(void)
{
    static GType ctk_color_controls_type = 0;

    if (!ctk_color_controls_type) {
        static const GTypeInfo ctk_color_controls_info = {
            sizeof (CtkColorControlsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkColorControls),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_color_controls_type =
            g_type_register_static (GTK_TYPE_VBOX,
                                    "CtkColorControls",
                                    &ctk_color_controls_info, 0);
    }

    return ctk_color_controls_type;
} /* ctk_color_controls_get_type() */

GtkWidget* ctk_color_controls_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config,
                                  CtkEvent *ctk_event,
                                  GtkWidget *reset_button,
                                  unsigned int display_device_mask,
                                  char *name)
{
    GObject *object;
    CtkColorControls *ctk_color_controls;
    GtkWidget *frame, *hbox, *label, *eventbox;
    GtkWidget *menu, *table, *menu_item = NULL, *separator;
    ReturnStatus ret1, ret2;
    NVCTRLAttributeValidValuesRec valid1, valid2;
    gint i;

    /* check if color configuration is supported */
    ret1 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_COLOR_SPACE,
                                                &valid1);
    ret2 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_COLOR_RANGE,
                                                &valid2);

    if ((ret1 != NvCtrlSuccess) || (ret2 != NvCtrlSuccess)) {
        return NULL;
    }

    /* create the object */
    object = g_object_new(CTK_TYPE_COLOR_CONTROLS, NULL);
    if (!object) {
        return NULL;
    }

    ctk_color_controls = CTK_COLOR_CONTROLS(object);
    ctk_color_controls->handle = handle;
    ctk_color_controls->ctk_config = ctk_config;
    ctk_color_controls->reset_button = reset_button;
    ctk_color_controls->display_device_mask = display_device_mask;
    ctk_color_controls->name = strdup(name);

    /* build a table holding available color space */
    if (!build_color_space_table(ctk_color_controls, valid1)) {
        return NULL;
    }

    /* create main color box & frame */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);
    ctk_color_controls->color_controls_main = hbox;

    frame = gtk_frame_new("Color Controls");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), frame);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_config, eventbox, __color_controls_help);

    table = gtk_table_new(1, 6, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* dropdown list for color space */
    menu = gtk_menu_new();

    for (i = 0; i < ctk_color_controls->color_space_table_size; i++) {
        switch (ctk_color_controls->color_space_table[i]) {
        case NV_CTRL_COLOR_SPACE_YCbCr422:
            menu_item = gtk_menu_item_new_with_label("YCbCr422");
            break;
        case NV_CTRL_COLOR_SPACE_YCbCr444:
            menu_item = gtk_menu_item_new_with_label("YCbCr444");
            break;
        default:
        case NV_CTRL_COLOR_SPACE_RGB:
            menu_item = gtk_menu_item_new_with_label("RGB");
            break;
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
    }

    ctk_color_controls->color_space_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_color_controls->color_space_menu),
         menu);

    g_signal_connect(G_OBJECT(ctk_color_controls->color_space_menu),
                     "changed", G_CALLBACK(color_space_menu_changed),
                     (gpointer) ctk_color_controls);


    /* pack the label & drop down */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Color Space: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    ctk_color_controls->color_space_box = hbox;
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_color_controls->color_space_menu,
                       FALSE, FALSE, 0);

    /* V-bar */
    hbox = gtk_hbox_new(FALSE, 0);
    separator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Build color widgets & pack them in table */
    /* dropdown list for color range */
    menu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label("Full");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label("Limited");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    ctk_color_controls->color_range_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_color_controls->color_range_menu),
         menu);

    g_signal_connect(G_OBJECT(ctk_color_controls->color_range_menu),
                     "changed", G_CALLBACK(color_range_menu_changed),
                     (gpointer) ctk_color_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Color Range: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 4, 5, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_color_controls->color_range_menu,
                       FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    ctk_color_controls_setup(ctk_color_controls);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_COLOR_RANGE),
                     G_CALLBACK(color_control_update_received),
                     (gpointer) ctk_color_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_COLOR_SPACE),
                     G_CALLBACK(color_control_update_received),
                     (gpointer) ctk_color_controls);

    return GTK_WIDGET(object);

} /* ctk_color_controls_new() */


/*
 * ctk_color_controls_setup() - Setup routine for color attributes. Used
 * in DFP setup stage as well as for updating the GUI when there is change in
 * color range or color space.
 */
void ctk_color_controls_setup(CtkColorControls *ctk_color_controls)
{
    gint val;

    if (!ctk_color_controls) {
        return;
    }

    /* color range */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_color_controls->handle,
                                  ctk_color_controls->display_device_mask,
                                  NV_CTRL_COLOR_RANGE, &val)) {
        val = NV_CTRL_COLOR_RANGE_FULL;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_color_controls->color_range_menu),
         val);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);
    
    /* color space */
    if (!update_color_space_menu_info
        ((gpointer)ctk_color_controls)) {
        gtk_widget_set_sensitive(ctk_color_controls->color_controls_main,
                                 FALSE);
        gtk_widget_hide_all(ctk_color_controls->color_controls_main);
    }

} /* ctk_color_controls_setup() */


static Bool update_color_space_menu_info(gpointer user_data)
{
    CtkColorControls *ctk_color_controls =
        CTK_COLOR_CONTROLS(user_data);
    gint color_space = NV_CTRL_COLOR_SPACE_RGB;

    /* color space */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_color_controls->handle,
                                  ctk_color_controls->display_device_mask,
                                  NV_CTRL_COLOR_SPACE,
                                  &color_space)) {
        free(ctk_color_controls->color_space_table);
        return FALSE;
    }

    color_space = map_nvctrl_value_to_table(ctk_color_controls,
                                            color_space);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_color_controls->color_space_menu),
         color_space);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    return True;
} /* update_color_space_menu_info() */

static
void post_color_range_update(CtkColorControls *ctk_color_controls,
                             gint color_range)
{
    static const char *color_range_table[] = {
        "Full",   /* NV_CTRL_COLOR_RANGE_FULL */
        "Limited" /* NV_CTRL_COLOR_RANGE_LIMITED */
    };

    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 "Color Range set to %s for %s.",
                                 color_range_table[color_range],
                                 ctk_color_controls->name);
}

static
void post_color_space_update(CtkColorControls *ctk_color_controls,
                             gint color_space)
{
    static const char *color_space_table[] = {
        "RGB",     /* NV_CTRL_COLOR_SPACE_RGB */
        "YCbCr422",  /* NV_CTRL_COLOR_SPACE_YCbCr422 */
        "YCbCr444"   /* NV_CTRL_COLOR_SPACE_YCbCr444 */
    };

    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 "Color Space set to %s for %s.",
                                 color_space_table[color_space],
                                 ctk_color_controls->name);
}

static void color_range_menu_changed(GtkOptionMenu *color_range_menu,
                                     gpointer user_data)
{
    CtkColorControls *ctk_color_controls =
        CTK_COLOR_CONTROLS(user_data);
    gint history, color_range = NV_CTRL_COLOR_RANGE_FULL;

    history = gtk_option_menu_get_history(color_range_menu);

    switch (history) {
    case 1:
        color_range = NV_CTRL_COLOR_RANGE_LIMITED;
        break;
    default:
    case 0:
        color_range = NV_CTRL_COLOR_RANGE_FULL;
        break;
    }

    NvCtrlSetDisplayAttribute(ctk_color_controls->handle,
                              ctk_color_controls->display_device_mask,
                              NV_CTRL_COLOR_RANGE,
                              color_range);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_color_controls->color_range_menu),
         color_range);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);

    /* reflecting the change in color range to other widgets & reset button */
    ctk_color_controls_setup(ctk_color_controls);
    post_color_range_update(ctk_color_controls, color_range);
    gtk_widget_set_sensitive(ctk_color_controls->reset_button, TRUE);

} /* color_range_menu_changed() */


static void color_space_menu_changed(GtkOptionMenu *color_space_menu,
                                     gpointer user_data)
{
    CtkColorControls *ctk_color_controls =
        CTK_COLOR_CONTROLS(user_data);
    gint history, color_space = NV_CTRL_COLOR_SPACE_RGB;

    history = gtk_option_menu_get_history(color_space_menu);

    color_space = ctk_color_controls->color_space_table[history];

    NvCtrlSetDisplayAttribute(ctk_color_controls->handle,
                              ctk_color_controls->display_device_mask,
                              NV_CTRL_COLOR_SPACE,
                              color_space);

    color_space = map_nvctrl_value_to_table(ctk_color_controls,
                                            color_space);
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_color_controls->color_space_menu),
         color_space);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    /* reflecting the change in color space to other widgets & reset button */
    ctk_color_controls_setup(ctk_color_controls);
    post_color_space_update(ctk_color_controls, color_space);
    gtk_widget_set_sensitive(ctk_color_controls->reset_button, TRUE);

} /* color_space_menu_changed() */


/*
 * ctk_color_controls_reset() - Resets the color range and 
 * & color space when Reset HW Defaults is clicked
 */
void ctk_color_controls_reset(CtkColorControls *ctk_color_controls)
{
    if (!ctk_color_controls) {
        return;
    }

    NvCtrlSetDisplayAttribute(ctk_color_controls->handle,
                              ctk_color_controls->display_device_mask,
                              NV_CTRL_COLOR_SPACE,
                              NV_CTRL_COLOR_SPACE_RGB);

    NvCtrlSetDisplayAttribute(ctk_color_controls->handle,
                              ctk_color_controls->display_device_mask,
                              NV_CTRL_COLOR_RANGE,
                              NV_CTRL_COLOR_RANGE_FULL);

    ctk_color_controls_setup(ctk_color_controls);
} /* ctk_color_controls_reset() */


/*
 * add_color_controls_help() -
 */
void add_color_controls_help(CtkColorControls *ctk_color_controls,
                             GtkTextBuffer *b,
                             GtkTextIter *i)
{
    ctk_help_heading(b, i, "Color Controls");
    ctk_help_para(b, i, __color_controls_help);
} /* add_color_controls_help() */


/*
 * When other client updated color controls
 * we should update the GUI to reflect the current color range
 * and color space.
 */
static void color_control_update_received(GtkObject *object, gpointer arg1,
                                          gpointer user_data)
{
    CtkColorControls *ctk_object = CTK_COLOR_CONTROLS(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;

    /* if the event is not for this display device, return */

    if (!(event_struct->display_mask & ctk_object->display_device_mask)) {
        return;
    }

    ctk_color_controls_setup(ctk_object);
} /* color_control_update_received()  */


/*
 * build_color_space_table() - build a table of color space, showing
 * modes supported by the DFP.
 */
static gboolean build_color_space_table(CtkColorControls *ctk_color_controls,
                                        NVCTRLAttributeValidValuesRec valid)
{
    gint i, n = 0, color_space_count = 0;
    gint mask = valid.u.bits.ints;

    if (valid.type != ATTRIBUTE_TYPE_INT_BITS) {
        return False;
    }

    /* count no. of supported color space */
    while(mask) {
        mask = mask & (mask - 1);
        color_space_count++;
    }

    ctk_color_controls->color_space_table_size = color_space_count;
    ctk_color_controls->color_space_table =
        calloc(color_space_count, sizeof(ctk_color_controls->color_space_table[0]));
    if (!ctk_color_controls->color_space_table) {
        return False;
    }

    for (i = 0, n = 0; n < ctk_color_controls->color_space_table_size; i++) {
        if (valid.u.bits.ints & (1 << i)) {
            ctk_color_controls->color_space_table[n] = i;
            n++;
        }
    }

    return True;

} /* build_color_space_table() */


static gint map_nvctrl_value_to_table(CtkColorControls *ctk_color_controls,
                                      gint val)
{
    int i;
    for (i = 0; i < ctk_color_controls->color_space_table_size; i++) {
        if (val == ctk_color_controls->color_space_table[i]) {
            return i;
        }
    }

    return 0;
} /*map_nvctrl_value_to_table() */
