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
#include <libintl.h>

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkditheringcontrols.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"

#define _(STRING) gettext(STRING)
#define N_(STRING) STRING

/* function prototypes */
static void
ctk_dither_controls_class_init(CtkDitheringControlsClass *ctk_object_class);

static void ctk_dither_controls_finalize(GObject *object);

static gboolean build_dithering_mode_table(CtkDitheringControls *ctk_dithering_controls);

static gint map_nvctrl_value_to_table(CtkDitheringControls *ctk_dithering_controls,
                                      gint val);

static Bool update_dithering_info(gpointer user_data);

static void setup_dithering_info(CtkDitheringControls *ctk_dithering_controls);

static void setup_reset_button(CtkDitheringControls *ctk_dithering_controls);

static void setup_dithering_config_menu(CtkDitheringControls *ctk_dithering_controls);

static void setup_dithering_mode_menu(CtkDitheringControls *ctk_dithering_controls);

static void setup_dithering_depth_menu(CtkDitheringControls *ctk_dithering_controls);

static void dithering_depth_menu_changed(GtkWidget *dithering_depth_menu,
                                         gpointer user_data);
static void dithering_mode_menu_changed(GtkWidget *dithering_mode_menu,
                                        gpointer user_data);
static void dithering_config_menu_changed(GtkWidget *dithering_config_menu,
                                          gpointer user_data);

static void dithering_update_received(GObject *object, CtrlEvent *event,
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

static gint map_dithering_config_menu_idx_to_nvctrl(gint idx);

static gint map_dithering_depth_menu_idx_to_nvctrl(gint idx);

/* macros */
#define FRAME_PADDING 5

/* help text */
static const char * __dithering_help =
N_("The Dithering Controls show the current state of dithering and allow "
"changing the dithering configuration, mode and/or depth.");

static const char * __dithering_config_help =
N_("Dithering will be performed when dithering is enabled here and the "
"panel's bitdepth is less than that of the GPU's internal pixel pipeline.");

static const char * __dithering_mode_help = 
N_("Dithering mode can be Dynamic 2x2, Static 2x2 or Temporal "
"depending on the type of the display device.");

static const char * __dithering_depth_help = 
N_("The depth can be adjusted to 6 or 8 bits per channel depending on "
"the type of display device.");

GType ctk_dithering_controls_get_type(void)
{
    static GType ctk_dithering_controls_type = 0;

    if (!ctk_dithering_controls_type) {
        static const GTypeInfo ctk_dithering_controls_info = {
            sizeof (CtkDitheringControlsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_dither_controls_class_init, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDitheringControls),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_dithering_controls_type =
            g_type_register_static (GTK_TYPE_VBOX,
                                    "CtkDitheringControls",
                                    &ctk_dithering_controls_info, 0);
    }

    return ctk_dithering_controls_type;
} /* ctk_dithering_controls_get_type() */

static void
ctk_dither_controls_class_init(CtkDitheringControlsClass *ctk_object_class)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_dither_controls_finalize;
}

static void ctk_dither_controls_finalize(GObject *object)
{
    CtkDitheringControls *ctk_object = CTK_DITHERING_CONTROLS(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}

GtkWidget* ctk_dithering_controls_new(CtrlTarget *ctrl_target,
                                      CtkConfig *ctk_config,
                                      CtkEvent *ctk_event,
                                      GtkWidget *reset_button,
                                      char *name)
{
    GObject *object;
    CtkDitheringControls *ctk_dithering_controls;
    GtkWidget *frame, *vbox, *hbox, *label;
    GtkWidget *table, *separator;
    CtkDropDownMenu *menu;
    ReturnStatus ret;
    int tmp;

    /* test that dithering is available before creating the widget */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_DITHERING, &tmp);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* create the object */
    object = g_object_new(CTK_TYPE_DITHERING_CONTROLS, NULL);
    if (!object) {
        return NULL;
    }

    ctk_dithering_controls = CTK_DITHERING_CONTROLS(object);
    ctk_dithering_controls->ctrl_target = ctrl_target;
    ctk_dithering_controls->ctk_event = ctk_event;
    ctk_dithering_controls->ctk_config = ctk_config;
    ctk_dithering_controls->reset_button = reset_button;
    ctk_dithering_controls->name = strdup(name);

    /* create main dithering box & frame */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);
    ctk_dithering_controls->dithering_controls_box = hbox;

    frame = gtk_frame_new(_("Dithering Controls"));
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    table = gtk_table_new(5, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* Build Dithering widgets & pack them in table */
    /* dropdown list for dithering configuration */
    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    ctk_drop_down_menu_append_item(menu, _("Auto"), 0);
    ctk_drop_down_menu_append_item(menu, _("Enabled"), 1);
    ctk_drop_down_menu_append_item(menu, _("Disabled"), 2);

    ctk_dithering_controls->dithering_config_menu = GTK_WIDGET(menu);
    
    ctk_config_set_tooltip(ctk_config, 
                           ctk_dithering_controls->dithering_config_menu, 
                           __dithering_config_help);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_config_menu),
                     "changed", G_CALLBACK(dithering_config_menu_changed),
                     (gpointer) ctk_dithering_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(_("Dithering: "));
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
    label = gtk_label_new(_("Current Dithering: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_config_txt = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-bar 1 */
    vbox = gtk_vbox_new(FALSE, 0);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 4, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* dropdown list for dithering modes - populated in setup */
    ctk_dithering_controls->dithering_mode_menu = 
                ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    ctk_config_set_tooltip(ctk_config, 
                           ctk_dithering_controls->dithering_mode_menu, 
                           __dithering_mode_help);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
                     "changed", G_CALLBACK(dithering_mode_menu_changed),
                     (gpointer) ctk_dithering_controls);


    /* pack the label & drop down */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(_("Mode: "));
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
    label = gtk_label_new(_("Current Mode: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_mode_txt = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-bar 2 */
    vbox = gtk_vbox_new(FALSE, 0);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 4, 3, 4,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* dithering depth */
    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    ctk_drop_down_menu_append_item(menu, _("Auto"), 0);
    ctk_drop_down_menu_append_item(menu, _("6 bpc"), 1);
    ctk_drop_down_menu_append_item(menu, _("8 bpc"), 2);

    ctk_dithering_controls->dithering_depth_menu = GTK_WIDGET(menu);
    
    ctk_config_set_tooltip(ctk_config, 
                           ctk_dithering_controls->dithering_depth_menu, 
                           __dithering_depth_help);

    g_signal_connect(G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
                     "changed", G_CALLBACK(dithering_depth_menu_changed),
                     (gpointer) ctk_dithering_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(_("Depth: "));
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
    label = gtk_label_new(_("Current Depth: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 4, 5,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_dithering_controls->dithering_depth_txt = label;
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
 * setup_reset_button() - enables the reset button if any of the current
 * settings are not the default.
 */
static void setup_reset_button(CtkDitheringControls *ctk_dithering_controls)
{
    gint history;
    gint val;
    CtkDropDownMenu *dithering_config_menu;
    CtkDropDownMenu *dithering_mode_menu;
    CtkDropDownMenu *dithering_depth_menu;

    if (!ctk_widget_get_sensitive(ctk_dithering_controls->dithering_controls_box)) {
        /* Nothing is available, don't bother enabling the reset button yet. */
        return;
    }

    /* The config menu is always available */
    dithering_config_menu =
        CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_config_menu);
    history = ctk_drop_down_menu_get_current_value(dithering_config_menu);
    val = map_dithering_config_menu_idx_to_nvctrl(history);
    if (val != NV_CTRL_DITHERING_AUTO) {
        goto enable;
    }

    if (ctk_widget_get_sensitive(ctk_dithering_controls->dithering_mode_box)) {
        dithering_mode_menu =
            CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_mode_menu);
        history = ctk_drop_down_menu_get_current_value(dithering_mode_menu);
        val = ctk_dithering_controls->dithering_mode_table[history];
        if (val != NV_CTRL_DITHERING_MODE_AUTO) {
            goto enable;
        }
    }

    if (ctk_widget_get_sensitive(ctk_dithering_controls->dithering_depth_box)) {
        dithering_depth_menu =
            CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_depth_menu);
        history = ctk_drop_down_menu_get_current_value(dithering_depth_menu);
        val = map_dithering_depth_menu_idx_to_nvctrl(history);
        if (val != NV_CTRL_DITHERING_DEPTH_AUTO) {
            goto enable;
        }
    }

    /* Don't disable reset button here, since other settings that are not
     * managed by the ctk_image_slider here may need it enabled
     */
    return;

 enable:
    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
}



static void
setup_dithering_depth_menu(CtkDitheringControls *ctk_dithering_controls)
{
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *dithering_depth_menu;
    gint val;
    ReturnStatus ret;
    dithering_depth_menu =
        CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_depth_menu);

    /* dithering depth */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_DITHERING_DEPTH, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_DITHERING_DEPTH_AUTO;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);

    ctk_drop_down_menu_set_current_value(dithering_depth_menu, val);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_depth_menu),
         G_CALLBACK(dithering_depth_menu_changed),
         (gpointer) ctk_dithering_controls);
}



static void
setup_dithering_mode_menu(CtkDitheringControls *ctk_dithering_controls)
{
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *dithering_mode_menu;
    gint val, i;
    ReturnStatus ret;
    dithering_mode_menu =
        CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_mode_menu);

    /* setup dithering modes */
    build_dithering_mode_table(ctk_dithering_controls);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);

    /* populate dropdown list for dithering modes */

    ctk_drop_down_menu_reset(dithering_mode_menu);

    for (i = 0; i < ctk_dithering_controls->dithering_mode_table_size; i++) {
        switch (ctk_dithering_controls->dithering_mode_table[i]) {
        case NV_CTRL_DITHERING_MODE_DYNAMIC_2X2:
            ctk_drop_down_menu_append_item(dithering_mode_menu, _("Dynamic 2x2"), i);
            break;
        case NV_CTRL_DITHERING_MODE_STATIC_2X2:
            ctk_drop_down_menu_append_item(dithering_mode_menu, _("Static 2x2"), i);
            break;
        case NV_CTRL_DITHERING_MODE_TEMPORAL:
            ctk_drop_down_menu_append_item(dithering_mode_menu, _("Temporal"), i);
            break;
        default:
        case NV_CTRL_DITHERING_MODE_AUTO:
            ctk_drop_down_menu_append_item(dithering_mode_menu, _("Auto"), i);
            break;
        }
    }

    /* dithering mode */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_DITHERING_MODE, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_DITHERING_MODE_AUTO;
    }

    val = map_nvctrl_value_to_table(ctk_dithering_controls, val);

    ctk_drop_down_menu_set_current_value(dithering_mode_menu, val);


    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_mode_menu),
         G_CALLBACK(dithering_mode_menu_changed),
         (gpointer) ctk_dithering_controls);
}



static void
setup_dithering_config_menu(CtkDitheringControls *ctk_dithering_controls)
{
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *dithering_config_menu;
    gint val;
    dithering_config_menu =
        CTK_DROP_DOWN_MENU(ctk_dithering_controls->dithering_config_menu);

    /* dithering */
    if (NvCtrlSuccess !=
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_DITHERING, &val)) {
        val = NV_CTRL_DITHERING_AUTO;
        return;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);

    ctk_drop_down_menu_set_current_value(dithering_config_menu, val);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_dithering_controls->dithering_config_menu),
         G_CALLBACK(dithering_config_menu_changed),
         (gpointer) ctk_dithering_controls);
}



/*
 * ctk_dithering_controls_setup() - Setup routine for dithering attributes. Used
 * in DFP setup stage as well as for updating the GUI when there is change in
 * dithering mode or config (enabled/disabled).
 */
void ctk_dithering_controls_setup(CtkDitheringControls *ctk_dithering_controls)
{

    if (!ctk_dithering_controls) {
        return;
    }
    
    /* setup dithering config menu */
    setup_dithering_config_menu(ctk_dithering_controls);
    
    /* setup dithering mode menu */
    setup_dithering_mode_menu(ctk_dithering_controls);
    
    /* setup dithering depth menu */
    setup_dithering_depth_menu(ctk_dithering_controls);

    setup_dithering_info(ctk_dithering_controls);

} /* ctk_dithering_controls_setup() */

static void setup_dithering_info(CtkDitheringControls *ctk_dithering_controls)
{

    if (!update_dithering_info((gpointer)ctk_dithering_controls)) {
        gtk_widget_hide(ctk_dithering_controls->dithering_controls_box);
    } else {
        gtk_widget_show(ctk_dithering_controls->dithering_controls_box);
    }
    setup_reset_button(ctk_dithering_controls);

} /* setup_dithering_info() */

static Bool update_dithering_info(gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    ReturnStatus ret;
    gint val;

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_DITHERING, &val);
    if (ret != NvCtrlSuccess) {
        /* Dithering is not currently available */
        return FALSE;
    }

    if (val == NV_CTRL_DITHERING_ENABLED ||
        val == NV_CTRL_DITHERING_AUTO) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box,
                                 TRUE);
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_depth_box,
                                 TRUE);
    } else if (val == NV_CTRL_DITHERING_DISABLED) {
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_mode_box,
                                 FALSE);
        gtk_widget_set_sensitive(ctk_dithering_controls->dithering_depth_box,
                                 FALSE);
    }

    /* current dithering */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_CURRENT_DITHERING, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_CURRENT_DITHERING_DISABLED;
    }

    if (val == NV_CTRL_CURRENT_DITHERING_ENABLED) {
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_config_txt),
                           _("Enabled"));
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_config_txt),
                           _("Disabled"));
    }

    /* current dithering mode */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_CURRENT_DITHERING_MODE,
                             &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_CURRENT_DITHERING_MODE_NONE;
    }

    switch (val) {
    case NV_CTRL_CURRENT_DITHERING_MODE_DYNAMIC_2X2:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_mode_txt),
                           _("Dynamic 2x2"));
        break;
    case NV_CTRL_CURRENT_DITHERING_MODE_STATIC_2X2:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_mode_txt),
                           _("Static 2x2"));
        break;
    case NV_CTRL_CURRENT_DITHERING_MODE_TEMPORAL:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_mode_txt),
                           _("Temporal"));
        break;
    default:
    case NV_CTRL_CURRENT_DITHERING_MODE_NONE:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_mode_txt),
                           _("None"));
        break;
    }
    /* current dithering depth */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_CURRENT_DITHERING_DEPTH,
                             &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_CURRENT_DITHERING_DEPTH_NONE;
    }

    switch (val) {
    case NV_CTRL_CURRENT_DITHERING_DEPTH_6_BITS:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_depth_txt),
                           _("6 bpc"));
        break;
    case NV_CTRL_CURRENT_DITHERING_DEPTH_8_BITS:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_depth_txt),
                           _("8 bpc"));
        break;
    default:
    case NV_CTRL_CURRENT_DITHERING_DEPTH_NONE:
        gtk_label_set_text(GTK_LABEL(ctk_dithering_controls->dithering_depth_txt),
                           _("None"));
        break;
    }

    return TRUE;

} /* update_dithering_info() */

static
void post_dithering_config_update(CtkDitheringControls *ctk_dithering_controls,
                                  gint dithering_config)
{
    static const char *dither_config_table[] = {
        N_("Auto"),    /* NV_CTRL_DITHERING_AUTO */
        N_("Enabled"), /* NV_CTRL_DITHERING_ENABLED */
        N_("Disabled") /* NV_CTRL_DITHERING_DISABLED */
    };

    if (dithering_config < NV_CTRL_DITHERING_AUTO ||
        dithering_config > NV_CTRL_DITHERING_DISABLED) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 _("Dithering set to %s for %s."),
                                 _(dither_config_table[dithering_config]),
                                 ctk_dithering_controls->name);
}

static
void post_dithering_mode_update(CtkDitheringControls *ctk_dithering_controls,
                                gint dithering_mode)
{
    static const char *dither_mode_table[] = {
        N_("Auto"),        /* NV_CTRL_DITHERING_MODE_AUTO */
        N_("Dynamic 2x2"), /* NV_CTRL_DITHERING_MODE_DYNAMIC_2X2 */
        N_("Static 2x2"),  /* NV_CTRL_DITHERING_MODE_STATIC_2X2 */
        N_("Temporal"),    /* NV_CTRL_DITHERING_MODE_TEMPORAL */
    };

    if (dithering_mode < NV_CTRL_DITHERING_MODE_AUTO ||
        dithering_mode > NV_CTRL_DITHERING_MODE_TEMPORAL) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 _("Dithering mode set to %s for %s."),
                                 _(dither_mode_table[dithering_mode]),
                                 ctk_dithering_controls->name);
}

static
void post_dithering_depth_update(CtkDitheringControls *ctk_dithering_controls,
                                 gint dithering_depth)
{
    static const char *dither_depth_table[] = {
        N_("Auto"),  /* NV_CTRL_DITHERING_DEPTH_AUTO */
        N_("6 bpc"), /* NV_CTRL_DITHERING_DEPTH_6_BITS */
        N_("8 bpc")  /* NV_CTRL_DITHERING_DEPTH_8_BITS */
    };

    if (dithering_depth < NV_CTRL_DITHERING_DEPTH_AUTO ||
        dithering_depth > NV_CTRL_DITHERING_DEPTH_8_BITS) {
        return;
    }

    gtk_widget_set_sensitive(ctk_dithering_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_dithering_controls->ctk_config,
                                 _("Dithering depth set to %s for %s."),
                                 _(dither_depth_table[dithering_depth]),
                                 ctk_dithering_controls->name);
}

static void dithering_config_menu_changed(GtkWidget *dithering_config_menu,
                                          gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(dithering_config_menu);
    gint history, dithering_config = NV_CTRL_DITHERING_AUTO;

    history = ctk_drop_down_menu_get_current_value(menu);

    dithering_config = map_dithering_config_menu_idx_to_nvctrl(history);

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_DITHERING, dithering_config);

    /* reflecting the change in configuration to other widgets & reset button */
    setup_dithering_info(ctk_dithering_controls);
    post_dithering_config_update(ctk_dithering_controls, dithering_config);

} /* dithering_config_menu_changed() */


static void dithering_mode_menu_changed(GtkWidget *dithering_mode_menu,
                                        gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(dithering_mode_menu);
    gint history, dithering_mode = NV_CTRL_DITHERING_MODE_AUTO;

    history = ctk_drop_down_menu_get_current_value(menu);

    dithering_mode = ctk_dithering_controls->dithering_mode_table[history];

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_DITHERING_MODE, dithering_mode);

    dithering_mode = map_nvctrl_value_to_table(ctk_dithering_controls,
                                               dithering_mode);
    /* reflecting the change in mode to the reset button */
    setup_dithering_info(ctk_dithering_controls);
    post_dithering_mode_update(ctk_dithering_controls, dithering_mode);

} /* dithering_mode_menu_changed() */

static void dithering_depth_menu_changed(GtkWidget *dithering_depth_menu,
                                         gpointer user_data)
{
    CtkDitheringControls *ctk_dithering_controls =
        CTK_DITHERING_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(dithering_depth_menu);
    gint history, dithering_depth = NV_CTRL_DITHERING_DEPTH_AUTO;

    history = ctk_drop_down_menu_get_current_value(menu);

    dithering_depth = map_dithering_depth_menu_idx_to_nvctrl(history);

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_DITHERING_DEPTH,
                       dithering_depth);

    /* reflecting the change in configuration to other widgets & reset button */
    setup_dithering_info(ctk_dithering_controls);
    post_dithering_depth_update(ctk_dithering_controls, dithering_depth);

} /* dithering_depth_menu_changed() */


/*
 * ctk_dithering_controls_reset() - Resets the dithering config (enabled/disabled)
 * & dithering mode when Reset HW Defaults is clicked
 */
void ctk_dithering_controls_reset(CtkDitheringControls *ctk_dithering_controls)
{
    CtrlTarget *ctrl_target;

    if (!ctk_dithering_controls) {
        return;
    }

    ctrl_target = ctk_dithering_controls->ctrl_target;

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_DITHERING,
                       NV_CTRL_DITHERING_AUTO);

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_DITHERING_MODE,
                       NV_CTRL_DITHERING_MODE_AUTO);

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_DITHERING_DEPTH,
                       NV_CTRL_DITHERING_DEPTH_AUTO);

    setup_dithering_info(ctk_dithering_controls);
} /* ctk_dithering_controls_reset() */


/*
 * add_dithering_controls_help() -
 */
void add_dithering_controls_help(CtkDitheringControls *ctk_dithering_controls,
                                 GtkTextBuffer *b,
                                 GtkTextIter *i)
{
    if (!ctk_dithering_controls) {
        return;
    }

    ctk_help_heading(b, i, _("Dithering Controls"));
    ctk_help_para(b, i, "%s", _(__dithering_help));

    ctk_help_term(b, i, _("Dithering"));
    ctk_help_para(b, i, "%s", _(__dithering_config_help));

    ctk_help_term(b, i, _("Mode"));
    ctk_help_para(b, i, "%s", _(__dithering_mode_help));

    ctk_help_term(b, i, _("Depth"));
    ctk_help_para(b, i, "%s", _(__dithering_depth_help));
} /* add_dithering_controls_help() */


/*
 * When dithering configuration is enabled/disabled,
 * we should update the GUI to reflect the current state & mode.
 */
static void dithering_update_received(GObject *object,
                                      CtrlEvent *event,
                                      gpointer user_data)
{
    CtkDitheringControls *ctk_object = CTK_DITHERING_CONTROLS(user_data);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    ctk_dithering_controls_setup(ctk_object);

    /* update status bar message */
    switch (event->int_attr.attribute) {
    case NV_CTRL_DITHERING:
        post_dithering_config_update(ctk_object, event->int_attr.value);
        break;
    case NV_CTRL_DITHERING_MODE:
        post_dithering_mode_update(ctk_object, event->int_attr.value);
        break;
    case NV_CTRL_DITHERING_DEPTH:
        post_dithering_depth_update(ctk_object, event->int_attr.value);
        break;
    }
} /* dithering_update_received()  */


/*
 * build_dithering_mode_table() - build a table of dithering modes, showing
 * modes supported by the hardware.
 */
static gboolean build_dithering_mode_table(CtkDitheringControls *ctk_dithering_controls)
{
    CtrlTarget *ctrl_target = ctk_dithering_controls->ctrl_target;
    ReturnStatus ret;
    CtrlAttributeValidValues valid;
    gint i, n = 0, num_of_modes = 0, mask;

    if (ctk_dithering_controls->dithering_mode_table_size > 0 && 
        ctk_dithering_controls->dithering_mode_table != NULL) {
        ctk_dithering_controls->dithering_mode_table_size = 0;
        free(ctk_dithering_controls->dithering_mode_table);
    }

    ret =
        NvCtrlGetValidAttributeValues(ctrl_target,
                                      NV_CTRL_DITHERING_MODE,
                                      &valid);

    if ((ret != NvCtrlSuccess) ||
        (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
        /* 
         * We do not have valid information to build a mode table
         * so we need to create default data for the placeholder menu.
         */
        ctk_dithering_controls->dithering_mode_table_size = 1;
        ctk_dithering_controls->dithering_mode_table =
            calloc(1, sizeof(ctk_dithering_controls->dithering_mode_table[0]));
        if (ctk_dithering_controls->dithering_mode_table) {
            ctk_dithering_controls->dithering_mode_table[0] = 
                NV_CTRL_DITHERING_MODE_AUTO;
        } else {
            ctk_dithering_controls->dithering_mode_table_size = 0;
        }
        return False;
    }

    /* count no. of supported modes */
    mask = valid.allowed_ints;
    while(mask) {
        mask = mask & (mask - 1);
        num_of_modes++;
    }

    ctk_dithering_controls->dithering_mode_table_size = num_of_modes;
    ctk_dithering_controls->dithering_mode_table =
        calloc(num_of_modes, sizeof(ctk_dithering_controls->dithering_mode_table[0]));
    if (!ctk_dithering_controls->dithering_mode_table) {
        ctk_dithering_controls->dithering_mode_table_size = 0;
        return False;
    }

    for (i = 0; i < num_of_modes; i++) {
        if (valid.allowed_ints & (1 << i)) {
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


static gint map_dithering_config_menu_idx_to_nvctrl(gint idx)
{
    switch (idx) {
    case 2: return NV_CTRL_DITHERING_DISABLED;
    case 1: return NV_CTRL_DITHERING_ENABLED;
    default: /* fallthrough; w/ warning? */
    case 0: return NV_CTRL_DITHERING_AUTO;
    }
}


static gint map_dithering_depth_menu_idx_to_nvctrl(gint idx)
{
    switch (idx) {
    case 2: return NV_CTRL_DITHERING_DEPTH_8_BITS;
    case 1: return NV_CTRL_DITHERING_DEPTH_6_BITS;
    default: /* fallthrough; w/ warning? */
    case 0: return NV_CTRL_DITHERING_DEPTH_AUTO;
    }
}
