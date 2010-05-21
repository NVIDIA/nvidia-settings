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
#include <sys/mman.h>
#include <errno.h>

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkditheringcontrols.h"

/* function prototypes */
static gboolean build_dithering_mode_table(CtkDitheringControls *ctk_dithering_controls,
                                           NVCTRLAttributeValidValuesRec valid);

static gint map_nvctrl_value_to_table(CtkDitheringControls *ctk_dithering_controls,
                                      gint val);

static void enable_dithering_toggled(GtkWidget *widget, gpointer user_data);

static Bool update_dithering_mode_menu_info(gpointer user_data);

static void update_dithering_mode_menu_event(GtkObject *object,
                                             gpointer arg1,
                                             gpointer user_data);

static void dithering_mode_menu_changed(GtkOptionMenu *dithering_mode_menu,
                                        gpointer user_data);

static void dithering_update_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

/* macros */
#define FRAME_PADDING 5

/* help text */
static const char * __dithering_help =
"The Dithering Controls show the current state of dithering and allow "
"changing the dithering configuration.  Dithering will be performed "
"when dithering is enabled here, and the flat panel's bitdepth is "
"less than that of the GPU's internal pixel pipeline.";

GType ctk_dithering_controls_get_type(void)
{
    static GType ctk_dithering_controls_type = 0;

    if (!ctk_dithering_controls_type) {
        static const GTypeInfo ctk_dithering_controls_info = {
            sizeof (CtkDitheringControlsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDitheringControls),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_dithering_controls_type =
            g_type_register_static (GTK_TYPE_VBOX,
                                    "CtkDitheringControls",
                                    &ctk_dithering_controls_info, 0);
    }

    return ctk_dithering_controls_type;
} /* ctk_dithering_controls_get_type() */

GtkWidget* ctk_dithering_controls_new(NvCtrlAttributeHandle *handle,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      GtkWidget *reset_button,
                                      unsigned int display_device_mask)
{
    GObject *object;
    CtkDitheringControls *ctk_dithering_controls;
    GtkWidget *frame, *vbox, *hbox, *label;
    GtkWidget *menu, *table, *menu_item = NULL;
    GtkWidget *button, *vseparator;
    ReturnStatus ret1, ret2;
    NVCTRLAttributeValidValuesRec valid;
    gint val, i, dithering_config, dithering_mode;

    /* check if dithering modes are supported */
    ret1 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_FLATPANEL_DITHERING_MODE,
                                                &valid);

    ret2 = NvCtrlGetDisplayAttribute(handle, display_device_mask,
                                     NV_CTRL_FLATPANEL_DITHERING_MODE,
                                     &val);

    if ((ret1 != NvCtrlSuccess) || (ret2 != NvCtrlSuccess) ||
        (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        return NULL;
    }

    /* create the object */
    object = g_object_new(CTK_TYPE_DITHERING_CONTROLS, NULL);
    if (!object) {
        return NULL;
    }

    ctk_dithering_controls = CTK_DITHERING_CONTROLS(object);
    ctk_dithering_controls->handle = handle;
    ctk_dithering_controls->ctk_config = ctk_config;
    ctk_dithering_controls->reset_button = reset_button;
    ctk_dithering_controls->display_device_mask = display_device_mask;

    /* build a table holding available dithering modes */
    if (!build_dithering_mode_table(ctk_dithering_controls, valid)) {
        return NULL;
    }

    /* cache the default dithering config & mode values */
    ret1 =
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DEFAULT_DITHERING,
                                  &dithering_config);

    ret2 =
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DEFAULT_DITHERING_MODE,
                                  &dithering_mode);

    if (ret1 != NvCtrlSuccess || ret2 != NvCtrlSuccess) {
        dithering_config = NV_CTRL_FLATPANEL_DITHERING_ENABLED;
        dithering_mode = NV_CTRL_FLATPANEL_DITHERING_MODE_DYNAMIC_2X2;
    }

    ctk_dithering_controls->default_dithering_config = dithering_config;
    ctk_dithering_controls->default_dithering_mode = dithering_mode;

    /* create main dithering box & frame */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);
    ctk_dithering_controls->dithering_controls_main = hbox;

    frame = gtk_frame_new("Dithering Controls");
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    /* add checkbox */
    button = gtk_check_button_new_with_label("Enable");
    ctk_dithering_controls->enable_dithering_button = button;
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
    ctk_config_set_tooltip(ctk_config, button, __dithering_help);

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(enable_dithering_toggled),
                     (gpointer) ctk_dithering_controls);

    /* add vseparator */
    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, TRUE, TRUE, 0);

    /* add dropdown menu */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    ctk_dithering_controls->dithering_mode_box = vbox;

    /* Specifying drop down list */
    menu = gtk_menu_new();

    for (i = 0; i < ctk_dithering_controls->dithering_mode_table_size; i++) {
        switch (ctk_dithering_controls->dithering_mode_table[i]) {
        case NV_CTRL_FLATPANEL_DITHERING_MODE_DYNAMIC_2X2:
            menu_item = gtk_menu_item_new_with_label("Dynamic 2X2 mode");
            break;
        case NV_CTRL_FLATPANEL_DITHERING_MODE_STATIC_2X2:
            menu_item = gtk_menu_item_new_with_label("Static 2X2 mode");
            break;
        }
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
    }

    ctk_dithering_controls->dithering_mode_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_mode_menu),
         menu);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
                     "changed", G_CALLBACK(dithering_mode_menu_changed),
                     (gpointer) ctk_dithering_controls);

    /* Packing the drop down list */
    table = gtk_table_new(1, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Mode:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_dithering_controls->dithering_mode_menu,
                       FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    ctk_dithering_controls_setup(ctk_dithering_controls);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_DITHERING_MODE),
                     G_CALLBACK(update_dithering_mode_menu_event),
                     (gpointer) ctk_dithering_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_DITHERING),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);

    return GTK_WIDGET(object);

} /* ctk_dithering_controls_new() */


/*
 * ctk_dithering_controls_setup() - Setup routine for dithering attributes. Used
 * in DFP setup stage as well as for updating the GUI when there is change in
 * dithering mode or config (enabled/disabled).
 */
void ctk_dithering_controls_setup(CtkDitheringControls *ctk_dithering_controls)
{
    gint val;

    if (!ctk_dithering_controls) {
        return;
    }

    /* update flatpanel dithering controls */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING, &val)) {
        val = NV_CTRL_FLATPANEL_DITHERING_DISABLED;
    }

    if (val == NV_CTRL_FLATPANEL_DITHERING_ENABLED) {
        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_dithering_controls->enable_dithering_button),
             G_CALLBACK(enable_dithering_toggled),
             (gpointer) ctk_dithering_controls);

        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_dithering_controls->enable_dithering_button),
             TRUE);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_dithering_controls->enable_dithering_button),
             G_CALLBACK(enable_dithering_toggled),
             (gpointer) ctk_dithering_controls);

        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box,
                                 TRUE);
    } else if (val == NV_CTRL_FLATPANEL_DITHERING_DISABLED) {
        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_dithering_controls->enable_dithering_button),
             G_CALLBACK(enable_dithering_toggled),
             (gpointer) ctk_dithering_controls);

        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_dithering_controls->enable_dithering_button),
             FALSE);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_dithering_controls->enable_dithering_button),
             G_CALLBACK(enable_dithering_toggled),
             (gpointer) ctk_dithering_controls);

        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box,
                                 FALSE);
    }

    if (!update_dithering_mode_menu_info
        ((gpointer)ctk_dithering_controls)) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_controls_main,
                                 FALSE);
        gtk_widget_hide_all(ctk_dithering_controls->dithering_controls_main);
    }

} /* ctk_dithering_controls_setup() */


static void update_dithering_mode_menu_event(GtkObject *object,
                                             gpointer arg1,
                                             gpointer user_data)
{
    update_dithering_mode_menu_info(user_data);
} /* update_dithering_mode_menu_event() */

static Bool update_dithering_mode_menu_info(gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint dithering_mode = NV_CTRL_FLATPANEL_DITHERING_MODE_DYNAMIC_2X2;

    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING_MODE,
                                  &dithering_mode)) {
        free(ctk_dithering_controls->dithering_mode_table);
        return FALSE;
    }

    dithering_mode = map_nvctrl_value_to_table(ctk_dithering_controls,
                                               dithering_mode);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_mode_menu),
         dithering_mode);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_menu, TRUE);
    gtk_widget_show(ctk_dithering_controls->dithering_mode_menu);

    return TRUE;
} /* update_dithering_mode_menu_info() */

static void dithering_mode_menu_changed(GtkOptionMenu *dithering_mode_menu,
                                        gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint history, dithering_mode = NV_CTRL_FLATPANEL_DITHERING_MODE_DYNAMIC_2X2;

    history = gtk_option_menu_get_history(dithering_mode_menu);

    dithering_mode = ctk_dithering_controls->dithering_mode_table[history];

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_FLATPANEL_DITHERING_MODE,
                              dithering_mode);

    dithering_mode = map_nvctrl_value_to_table(ctk_dithering_controls,
                                               dithering_mode);
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_mode_menu),
         dithering_mode);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);

    /* reflecting the change in dithering mode to reset button */
    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);

} /* dithering_mode_menu_changed() */


/*
 * enable_dithering_toggled() - Callback routine for Enable Dithering
 * check box.
 */
static void enable_dithering_toggled(GtkWidget *widget, gpointer user_data)
{
    gboolean checked = FALSE;
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);

    if (!ctk_dithering_controls) {
        return;
    }

    /* Get the checkbox status */
    checked = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_dithering_controls->enable_dithering_button));

    if (checked) {
        /* Enable the dithering effects */
        NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING,
                                  NV_CTRL_FLATPANEL_DITHERING_ENABLED);
    } else {
        /* Disable the dithering effects. */
        NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING,
                                  NV_CTRL_FLATPANEL_DITHERING_DISABLED);
    }

    ctk_dithering_controls_setup(ctk_dithering_controls);

    /* Reflecting the change in dithering configuration to reset button */
    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);

} /* enable_dithering_toggled() */


/*
 * ctk_dithering_controls_reset() - Resets the dithering config (enabled/disabled)
 * & dithering mode when Reset HW Defaults is clicked
 */
void ctk_dithering_controls_reset(CtkDitheringControls *ctk_dithering_controls)
{
    if (!ctk_dithering_controls) {
        return;
    }

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_FLATPANEL_DITHERING,
                              ctk_dithering_controls->default_dithering_config);

    if (ctk_dithering_controls->default_dithering_config ==
        NV_CTRL_FLATPANEL_DITHERING_ENABLED) {

        /* Setting dithering mode only makes sense when dithering is
         * enabled by default
         */
        NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_FLATPANEL_DITHERING_MODE,
                                  ctk_dithering_controls->default_dithering_mode);
    }

    ctk_dithering_controls_setup(ctk_dithering_controls);
} /* ctk_dithering_controls_reset() */


/*
 * add_dithering_controls_help() -
 */
void add_dithering_controls_help(CtkDitheringControls *ctk_dithering_controls,
                                 GtkTextBuffer *b,
                                 GtkTextIter *i)
{
    ctk_help_heading(b, i, "Dithering Controls");
    ctk_help_para(b, i, __dithering_help);
} /* add_dithering_controls_help() */


/*
 * When DFP dithering configuration is enabled/disabled,
 * we should update the GUI to reflect the current state.
 */
static void dithering_update_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
    CtkDitheringControls *ctk_object = CTK_DITHERING_CONTROLS(user_data);
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;

    /* if the event is not for this display device, return */

    if (!(event_struct->display_mask & ctk_object->display_device_mask)) {
        return;
    }

    ctk_dithering_controls_setup(ctk_object);
} /* dithering_update_received()  */


/*
 * build_dithering_mode_table() - build a table of dithering modes presently
 * supported by the HW.
 */
static gboolean build_dithering_mode_table(CtkDitheringControls *ctk_dithering_controls,
                                           NVCTRLAttributeValidValuesRec valid)
{
    gint i, n = 0, num_of_modes = 0;
    gint mask = valid.u.bits.ints;

    if (valid.type != ATTRIBUTE_TYPE_INT_BITS) {
        return False;
    }

    /* count no. of supported modes */
    while(mask) {
        mask = mask & (mask - 1);
        num_of_modes++;
    }

    ctk_dithering_controls->dithering_mode_table_size = num_of_modes;
    ctk_dithering_controls->dithering_mode_table = calloc(1, num_of_modes);
    if (!ctk_dithering_controls->dithering_mode_table) {
        return False;
    }

    for (i = 0; i <= num_of_modes; i++) {
        if (valid.u.bits.ints & (1 << i)) {
            ctk_dithering_controls->dithering_mode_table[n] = i;
            n++;
        }
    }

    return True;

} /* build_dithering_mode_table() */


static gint map_nvctrl_value_to_table(CtkDitheringControls *ctk_dithering_controls,
                                      gint val)
{
    int i;
    for (i = 0; i < ctk_dithering_controls->dithering_mode_table_size; i++) {
        if (val == ctk_dithering_controls->dithering_mode_table[i]) {
            return i;
        }
    }

    return 0;
} /*map_nvctrl_value_to_table() */
