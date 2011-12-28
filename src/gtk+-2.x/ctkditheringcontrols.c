/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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
#include "ctkditheringcontrols.h"

/* function prototypes */
static gboolean build_dithering_mode_table(CtkDitheringControls *ctk_dithering_controls,
                                           NVCTRLAttributeValidValuesRec valid);

static gint map_nvctrl_value_to_table(CtkDitheringControls *ctk_dithering_controls,
                                      gint val);

static Bool update_dithering_info(gpointer user_data);

static void dithering_depth_menu_changed(GtkOptionMenu *dithering_depth_menu,
                                         gpointer user_data);
static void dithering_mode_menu_changed(GtkOptionMenu *dithering_mode_menu,
                                        gpointer user_data);
static void dithering_config_menu_changed(GtkOptionMenu *dithering_config_menu,
                                          gpointer user_data);

static void dithering_update_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static
void post_dithering_config_update(CtkDitheringControls *ctk_dithering_controls,
                                  gint dithering_config);

static
void post_dithering_mode_update(CtkDitheringControls *ctk_dithering_controls,
                                gint dithering_mode);

static
void post_dithering_depth_update(CtkDitheringControls *ctk_dithering_controls,
                                 gint dithering_depth);

/* macros */
#define FRAME_PADDING 5

/* help text */
static const char * __dithering_help =
"The Dithering Controls show the current state of dithering and allow "
"changing the dithering configuration, mode and/or depth.  Dithering will "
"be performed when dithering is enabled here, and the panel's bitdepth is "
"less than that of the GPU's internal pixel pipeline.  The depth can be "
"adjusted to 6 or 8 bits per channel depending on the type of display "
"device.";

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
                                      unsigned int display_device_mask,
                                      char *name)
{
    GObject *object;
    CtkDitheringControls *ctk_dithering_controls;
    GtkWidget *frame, *vbox, *hbox, *label, *eventbox;
    GtkWidget *menu, *table, *menu_item = NULL, *separator;
    ReturnStatus ret1, ret2, ret3;
    NVCTRLAttributeValidValuesRec valid1, valid2, valid3;
    gint i;

    /* check if dithering configuration is supported */
    ret1 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_DITHERING,
                                                &valid1);
    ret2 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_DITHERING_MODE,
                                                &valid2);
    ret3 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_DITHERING_DEPTH,
                                                &valid3);

    if ((ret1 != NvCtrlSuccess) || (valid1.type != ATTRIBUTE_TYPE_INTEGER) ||
        (ret2 != NvCtrlSuccess) || (valid2.type != ATTRIBUTE_TYPE_INT_BITS) ||
        (ret3 != NvCtrlSuccess) || (valid3.type != ATTRIBUTE_TYPE_INTEGER)) {
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
    ctk_dithering_controls->name = strdup(name);

    /* build a table holding available dithering modes */
    if (!build_dithering_mode_table(ctk_dithering_controls, valid2)) {
        return NULL;
    }

    /* create main dithering box & frame */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);
    ctk_dithering_controls->dithering_controls_main = hbox;

    frame = gtk_frame_new("Dithering Controls");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), frame);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_config, eventbox, __dithering_help);

    table = gtk_table_new(5, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* Build Dithering widgets & pack them in table */
    /* dropdown list for dithering configuration */
    menu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label("Auto");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label("Enabled");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label("Disabled");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    ctk_dithering_controls->dithering_config_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_config_menu),
         menu);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_config_menu),
                     "changed", G_CALLBACK(dithering_config_menu_changed),
                     (gpointer) ctk_dithering_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Dithering: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_dithering_controls->dithering_config_menu,
                       FALSE, FALSE, 0);

    /* Build CurrentDithering widget  */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Current Dithering: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_current_config = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-bar 1 */
    vbox = gtk_vbox_new(FALSE, 0);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 4, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* dropdown list for dithering modes */
    menu = gtk_menu_new();

    for (i = 0; i < ctk_dithering_controls->dithering_mode_table_size; i++) {
        switch (ctk_dithering_controls->dithering_mode_table[i]) {
        case NV_CTRL_DITHERING_MODE_DYNAMIC_2X2:
            menu_item = gtk_menu_item_new_with_label("Dynamic 2X2");
            break;
        case NV_CTRL_DITHERING_MODE_STATIC_2X2:
            menu_item = gtk_menu_item_new_with_label("Static 2X2");
            break;
        case NV_CTRL_DITHERING_MODE_TEMPORAL:
            menu_item = gtk_menu_item_new_with_label("Temporal");
            break;
        default:
        case NV_CTRL_DITHERING_MODE_AUTO:
            menu_item = gtk_menu_item_new_with_label("Auto");
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


    /* pack the label & drop down */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Mode: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    ctk_dithering_controls->dithering_mode_box = hbox;
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_dithering_controls->dithering_mode_menu,
                       FALSE, FALSE, 0);

    /* Build CurrentMode widget  */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Current Mode: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_current_mode = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-bar 2 */
    vbox = gtk_vbox_new(FALSE, 0);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 4, 3, 4,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* dithering depth */
    menu = gtk_menu_new();

    menu_item = gtk_menu_item_new_with_label("Auto");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label("6 bpc");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    menu_item = gtk_menu_item_new_with_label("8 bpc");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    ctk_dithering_controls->dithering_depth_menu = gtk_option_menu_new();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_depth_menu),
         menu);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
                     "changed", G_CALLBACK(dithering_depth_menu_changed),
                     (gpointer) ctk_dithering_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Depth: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    ctk_dithering_controls->dithering_depth_box = hbox;
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_dithering_controls->dithering_depth_menu,
                       FALSE, FALSE, 0);

    /* Build CurrentDitheringDepth widget  */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Current Depth: ");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_current_depth = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    ctk_dithering_controls_setup(ctk_dithering_controls);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DITHERING),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DITHERING_MODE),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DITHERING_DEPTH),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURRENT_DITHERING),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURRENT_DITHERING_MODE),
                     G_CALLBACK(dithering_update_received),
                     (gpointer) ctk_dithering_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_CURRENT_DITHERING_DEPTH),
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

    /* dithering */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_DITHERING, &val)) {
        val = NV_CTRL_DITHERING_AUTO;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_config_menu),
         val);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);

    if (!update_dithering_info((gpointer)ctk_dithering_controls)) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_controls_main,
                                 FALSE);
        gtk_widget_hide_all(ctk_dithering_controls->dithering_controls_main);
    }

} /* ctk_dithering_controls_setup() */


static Bool update_dithering_info(gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint val, dithering_mode, dithering_depth;

    /* requested dithering */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_DITHERING, &val)) {
        val = NV_CTRL_DITHERING_DISABLED;
    }

    if (val == NV_CTRL_DITHERING_ENABLED ||
        val == NV_CTRL_DITHERING_AUTO) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box, TRUE);
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_depth_box, TRUE);
        gtk_widget_show(ctk_dithering_controls->dithering_mode_box);
        gtk_widget_show(ctk_dithering_controls->dithering_depth_box);
    } else if (val == NV_CTRL_DITHERING_DISABLED) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box, FALSE);
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_depth_box, FALSE);
    }

    /* current dithering */
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_CURRENT_DITHERING, &val)) {
        val = NV_CTRL_CURRENT_DITHERING_DISABLED;
    }

    if (val == NV_CTRL_CURRENT_DITHERING_ENABLED) {
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_config),
                           "Enabled");
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_config),
                           "Disabled");
    }

    /* dithering mode */
    dithering_mode = NV_CTRL_DITHERING_MODE_AUTO;
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_DITHERING_MODE,
                                  &dithering_mode)) {
        goto fail;
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

    /* current dithering mode */
    dithering_mode = NV_CTRL_CURRENT_DITHERING_MODE_NONE;
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_CURRENT_DITHERING_MODE,
                                  &dithering_mode)) {
        goto fail;
    }

    switch (dithering_mode) {
    case NV_CTRL_CURRENT_DITHERING_MODE_DYNAMIC_2X2:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_mode),
                           "Dynamic 2x2");
        break;
    case NV_CTRL_CURRENT_DITHERING_MODE_STATIC_2X2:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_mode),
                           "Static 2x2");
        break;
    case NV_CTRL_CURRENT_DITHERING_MODE_TEMPORAL:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_mode),
                           "Temporal");
        break;
    default:
    case NV_CTRL_CURRENT_DITHERING_MODE_NONE:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_mode),
                           "None");
        break;
    }

    /* dithering depth */
    dithering_depth = NV_CTRL_DITHERING_DEPTH_AUTO;
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_DITHERING_DEPTH,
                                  &dithering_depth)) {
        goto fail;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_depth_menu),
         dithering_depth);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);

    /* current dithering depth */
    dithering_depth = NV_CTRL_CURRENT_DITHERING_DEPTH_NONE;
    if (NvCtrlSuccess !=
        NvCtrlGetDisplayAttribute(ctk_dithering_controls->handle,
                                  ctk_dithering_controls->display_device_mask,
                                  NV_CTRL_CURRENT_DITHERING_DEPTH,
                                  &dithering_depth)) {
        goto fail;
    }

    switch (dithering_depth) {
    case NV_CTRL_CURRENT_DITHERING_DEPTH_6_BITS:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_depth),
                           "6 bpc");
        break;
    case NV_CTRL_CURRENT_DITHERING_DEPTH_8_BITS:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_depth),
                           "8 bpc");
        break;
    default:
    case NV_CTRL_CURRENT_DITHERING_DEPTH_NONE:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_current_depth),
                           "None");
        break;
    }

    return True;

 fail:
    free(ctk_dithering_controls->dithering_mode_table);
    return False;
} /* update_dithering_info() */

static
void post_dithering_config_update(CtkDitheringControls *ctk_dithering_controls,
                                  gint dithering_config)
{
    static const char *dither_config_table[] = {
        "Auto",    /* NV_CTRL_DITHERING_AUTO */
        "Enabled", /* NV_CTRL_DITHERING_ENABLED */
        "Disabled" /* NV_CTRL_DITHERING_DISABLED */
    };

    if (dithering_config < NV_CTRL_DITHERING_AUTO ||
        dithering_config > NV_CTRL_DITHERING_DISABLED) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 "Dithering set to %s for %s.",
                                 dither_config_table[dithering_config],
                                 ctk_dithering_controls->name);
}

static
void post_dithering_mode_update(CtkDitheringControls *ctk_dithering_controls,
                                gint dithering_mode)
{
    static const char *dither_mode_table[] = {
        "Auto",        /* NV_CTRL_DITHERING_MODE_AUTO */
        "Dynamic 2x2", /* NV_CTRL_DITHERING_MODE_DYNAMIC_2X2 */
        "Static 2x2",  /* NV_CTRL_DITHERING_MODE_STATIC_2X2 */
        "Temporal",    /* NV_CTRL_DITHERING_MODE_TEMPORAL */
    };

    if (dithering_mode < NV_CTRL_DITHERING_MODE_AUTO ||
        dithering_mode > NV_CTRL_DITHERING_MODE_TEMPORAL) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 "Dithering mode set to %s for %s.",
                                 dither_mode_table[dithering_mode],
                                 ctk_dithering_controls->name);
}

static
void post_dithering_depth_update(CtkDitheringControls *ctk_dithering_controls,
                                 gint dithering_depth)
{
    static const char *dither_depth_table[] = {
        "Auto",  /* NV_CTRL_DITHERING_DEPTH_AUTO */
        "6 bpc", /* NV_CTRL_DITHERING_DEPTH_6_BITS */
        "8 bpc"  /* NV_CTRL_DITHERING_DEPTH_8_BITS */
    };

    if (dithering_depth < NV_CTRL_DITHERING_DEPTH_AUTO ||
        dithering_depth > NV_CTRL_DITHERING_DEPTH_8_BITS) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 "Dithering depth set to %s for %s.",
                                 dither_depth_table[dithering_depth],
                                 ctk_dithering_controls->name);
}

static void dithering_config_menu_changed(GtkOptionMenu *dithering_config_menu,
                                          gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint history, dithering_config = NV_CTRL_DITHERING_AUTO;

    history = gtk_option_menu_get_history(dithering_config_menu);

    switch (history) {
    case 2:
        dithering_config = NV_CTRL_DITHERING_DISABLED;
        break;
    case 1:
        dithering_config = NV_CTRL_DITHERING_ENABLED;
        break;
    default:
    case 0:
        dithering_config = NV_CTRL_DITHERING_AUTO;
        break;
    }

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_DITHERING,
                              dithering_config);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_config_menu),
         dithering_config);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);

    /* reflecting the change in configuration to other widgets & reset button */
    ctk_dithering_controls_setup(ctk_dithering_controls);
    post_dithering_config_update(ctk_dithering_controls, dithering_config);

} /* dithering_config_menu_changed() */


static void dithering_mode_menu_changed(GtkOptionMenu *dithering_mode_menu,
                                        gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint history, dithering_mode = NV_CTRL_DITHERING_MODE_AUTO;

    history = gtk_option_menu_get_history(dithering_mode_menu);

    dithering_mode = ctk_dithering_controls->dithering_mode_table[history];

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_DITHERING_MODE,
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

    /* reflecting the change in mode to other widgets & reset button */
    ctk_dithering_controls_setup(ctk_dithering_controls);
    post_dithering_mode_update(ctk_dithering_controls, dithering_mode);

} /* dithering_mode_menu_changed() */

static void dithering_depth_menu_changed(GtkOptionMenu *dithering_depth_menu,
                                         gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    gint history, dithering_depth = NV_CTRL_DITHERING_DEPTH_AUTO;

    history = gtk_option_menu_get_history(dithering_depth_menu);

    switch (history) {
    case 2:
        dithering_depth = NV_CTRL_DITHERING_DEPTH_8_BITS;
        break;
    case 1:
        dithering_depth = NV_CTRL_DITHERING_DEPTH_6_BITS;
        break;
    default:
    case 0:
        dithering_depth = NV_CTRL_DITHERING_DEPTH_AUTO;
        break;
    }

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_DITHERING_DEPTH,
                              dithering_depth);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_dithering_controls->dithering_depth_menu),
         dithering_depth);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);

    /* reflecting the change in configuration to other widgets & reset button */
    ctk_dithering_controls_setup(ctk_dithering_controls);
    post_dithering_depth_update(ctk_dithering_controls, dithering_depth);

} /* dithering_depth_menu_changed() */


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
                              NV_CTRL_DITHERING,
                              NV_CTRL_DITHERING_AUTO);

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_DITHERING_MODE,
                              NV_CTRL_DITHERING_MODE_AUTO);

    NvCtrlSetDisplayAttribute(ctk_dithering_controls->handle,
                              ctk_dithering_controls->display_device_mask,
                              NV_CTRL_DITHERING_DEPTH,
                              NV_CTRL_DITHERING_DEPTH_AUTO);

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
 * When dithering configuration is enabled/disabled,
 * we should update the GUI to reflect the current state & mode.
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

    /* update status bar message */
    switch (event_struct->attribute) {
    case NV_CTRL_DITHERING:
        post_dithering_config_update(ctk_object, event_struct->value); break;
    case NV_CTRL_DITHERING_MODE:
        post_dithering_mode_update(ctk_object, event_struct->value); break;
    case NV_CTRL_DITHERING_DEPTH:
        post_dithering_depth_update(ctk_object, event_struct->value); break;
    }
} /* dithering_update_received()  */


/*
 * build_dithering_mode_table() - build a table of dithering modes, showing
 * modes supported by the hardware.
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
    ctk_dithering_controls->dithering_mode_table =
        calloc(num_of_modes, sizeof(ctk_dithering_controls->dithering_mode_table[0]));
    if (!ctk_dithering_controls->dithering_mode_table) {
        return False;
    }

    for (i = 0; i < num_of_modes; i++) {
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
