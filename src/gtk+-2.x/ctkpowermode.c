/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2020 NVIDIA Corporation.
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkbanner.h"
#include "ctkpowermode.h"
#include "ctkdropdownmenu.h"

#define DEFAULT_UPDATE_POWERMODE_INFO_TIME_INTERVAL 1000

static void setup_powermode_menu(CtkPowermode *ctk_powermode);
static gboolean update_current_powermode(CtkPowermode *ctk_powermode);
static void update_powermode_menu_info(CtkPowermode *ctk_powermode);
static void powermode_menu_changed(GtkWidget*, gpointer);
static void update_powermode_menu_event(GObject *object,
                                        gpointer arg1,
                                        gpointer user_data);

static const char *__powermode_menu_help =
"This platform supports three possible power modes: Performance, "
"Balanced (default), and Quiet.  This setting is applied "
"when AC power is connected.";
static const char *__performance_power_mode_help =
"Performance Power Mode allows the platform to run at higher power "
"and thermal settings, that are still within the platform's supported "
"limits.  Performance mode is applied when the system is on AC power and the "
"battery is charged more than 25%%.";
static const char *__balanced_power_mode_help =
"Balanced Power Mode is the default; it provides a performance and acoustic trade-off.";
static const char *__quiet_power_mode_help =
"Quiet Power Mode prioritizes thermal and acoustics over performance.";
static const char *__current_mode_help =
"This setting shows the current Platform Power mode of the system.  When the system is "
"powered by battery or an undersized power source (such as power over USB-C), the GPU "
"runs under a limited power policy.";

GType ctk_powermode_get_type(void)
{
    static GType ctk_powermode_type = 0;

    if (!ctk_powermode_type) {
        static const GTypeInfo ctk_powermode_info = {
            sizeof (CtkPowermodeClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkPowermode),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_powermode_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkPowermode",
                                   &ctk_powermode_info, 0);
    }

    return ctk_powermode_type;
}



static gchar *get_powermode_menu_label(const unsigned int val)
{
    gchar *label = NULL;

    switch (val) {
    case NV_CTRL_PLATFORM_POWER_MODE_PERFORMANCE:
        label = g_strdup_printf("Performance");
        break;
    case NV_CTRL_PLATFORM_POWER_MODE_BALANCED:
        label = g_strdup_printf("Balanced");
        break;
    case NV_CTRL_PLATFORM_POWER_MODE_QUIET:
        label = g_strdup_printf("Quiet");
        break;
    case NV_CTRL_PLATFORM_CURRENT_POWER_MODE_LIMITED_POWER_POLICY:
        label = g_strdup_printf("Limited Power Policy");
        break;
    default:
        label = g_strdup_printf("");
        break;
    }

    return label;
}



static void create_powermode_menu_entry(CtkDropDownMenu *menu,
                                        const unsigned int bit_mask,
                                        const unsigned int val)
{
    gchar *label;

    if (!(bit_mask & (1 << val))) {
        return;
    }

    label = get_powermode_menu_label(val);

    ctk_drop_down_menu_append_item(menu, label, val);
    g_free(label);
}



/*
 * setup_powermode_menu() - Create the power mode dropdown.
 */

static void setup_powermode_menu(CtkPowermode *ctk_powermode)
{
    ReturnStatus ret1;
    CtrlAttributeValidValues validPowerModes;
    CtrlTarget *ctrl_target = ctk_powermode->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(ctk_powermode->powermode_menu);

    ret1 = NvCtrlGetValidAttributeValues(ctrl_target,
                                         NV_CTRL_PLATFORM_POWER_MODE,
                                         &validPowerModes);
    if ((ret1 == NvCtrlSuccess) &&
        (validPowerModes.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
        const unsigned int bit_mask = validPowerModes.allowed_ints;

        create_powermode_menu_entry(menu, bit_mask,
                                    NV_CTRL_PLATFORM_POWER_MODE_PERFORMANCE);
        create_powermode_menu_entry(menu, bit_mask,
                                    NV_CTRL_PLATFORM_POWER_MODE_BALANCED);
        create_powermode_menu_entry(menu, bit_mask,
                                    NV_CTRL_PLATFORM_POWER_MODE_QUIET);
    }
}



GtkWidget* ctk_powermode_new(CtrlTarget *ctrl_target, CtkConfig *ctk_config,
                             CtkEvent *ctk_event)
{
    GObject *object;
    CtkPowermode *ctk_powermode;
    GtkWidget *banner;
    GtkWidget *hbox, *hbox2, *vbox, *hsep, *table;
    GtkWidget *label;
    CtkDropDownMenu *menu;
    gchar *s = NULL;
    ReturnStatus ret;
    gint powerMode;

    /* make sure we have a handle */

    g_return_val_if_fail((ctrl_target != NULL) &&
                         (ctrl_target->h != NULL), NULL);

    /* return early if power mode not supported */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_PLATFORM_POWER_MODE,
                             &powerMode);

    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* Create the ctk powermode object */
    object = g_object_new(CTK_TYPE_POWERMODE, NULL);
    ctk_powermode = CTK_POWERMODE(object);

    /* Set container properties of the object */
    ctk_powermode->ctk_config = ctk_config;
    ctk_powermode->ctrl_target = ctrl_target;
    gtk_box_set_spacing(GTK_BOX(ctk_powermode), 10);

    /* Image banner */
    banner = ctk_banner_image_new(BANNER_ARTWORK_THERMAL);
    gtk_box_pack_start(GTK_BOX(ctk_powermode), banner, FALSE, FALSE, 0);

    /* Add base information */

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctk_powermode), vbox, TRUE, TRUE, 0);

    /* Power Modes */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Platform Power Mode Settings");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-separator */
    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

    /* Specifying drop down list */

    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    ctk_powermode->powermode_menu = menu;
    setup_powermode_menu(ctk_powermode);

    /* Packing the drop down list */

    table = gtk_table_new(1, 4, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Platform Power Mode:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_powermode->ctk_config,
                           hbox2,
                           __powermode_menu_help);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox2),
                       GTK_WIDGET(ctk_powermode->powermode_menu),
                       FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Current Mode:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("");
    ctk_powermode->current_powermode = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_powermode->ctk_config,
                           hbox2,
                           __current_mode_help);

    gtk_widget_show_all(GTK_WIDGET(object));

    update_powermode_menu_info(ctk_powermode);
    update_current_powermode(ctk_powermode);

    g_signal_connect(G_OBJECT(menu), "changed",
                     G_CALLBACK(powermode_menu_changed),
                     (gpointer) ctk_powermode);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_PLATFORM_POWER_MODE),
                     G_CALLBACK(update_powermode_menu_event),
                     (gpointer) ctk_powermode);

    /* Register a timer callback to update the power mode */

    s = g_strdup_printf("Power Mode Monitor");

    ctk_config_add_timer(ctk_powermode->ctk_config,
                         DEFAULT_UPDATE_POWERMODE_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_current_powermode,
                         (gpointer) ctk_powermode);
    g_free(s);



    return GTK_WIDGET(object);
}



static void post_powermode_menu_update(CtkPowermode *ctk_powermode)
{
    CtkDropDownMenu *menu =
        CTK_DROP_DOWN_MENU(ctk_powermode->powermode_menu);
    const char *label = ctk_drop_down_menu_get_current_name(menu);

    ctk_config_statusbar_message(ctk_powermode->ctk_config,
                                 "Platform Power Mode set to %s.", label);
}



/*
 * update_current_powermode() - Update current mode label when other
 * clients like nvidia-powerd changes the power mode.
 */

static gboolean update_current_powermode(CtkPowermode *ctk_powermode)
{
    CtrlTarget *ctrl_target = ctk_powermode->ctrl_target;
    gint powerMode;
    ReturnStatus ret1;
    gchar *label = NULL;

    if (!ctk_powermode->current_powermode) {
        return FALSE;
    }

    ret1 = NvCtrlGetAttribute(ctrl_target,
                              NV_CTRL_PLATFORM_CURRENT_POWER_MODE,
                              &powerMode);

    if (ret1 != NvCtrlSuccess) {
        return FALSE;
    }

    label = get_powermode_menu_label(powerMode);
    gtk_label_set_text(GTK_LABEL(ctk_powermode->current_powermode), label);

    /* Update the status bar to show current power mode */
    ctk_config_statusbar_message(ctk_powermode->ctk_config,
                                 "Platform Power Mode set to %s.", label);

    return TRUE;
}



static void update_powermode_menu_info(CtkPowermode *ctk_powermode)
{
    CtrlTarget *ctrl_target = ctk_powermode->ctrl_target;
    gint powerMode;
    ReturnStatus ret1;
    CtkDropDownMenu *menu;

    if (!ctk_powermode->powermode_menu) {
        return;
    }

    menu = CTK_DROP_DOWN_MENU(ctk_powermode->powermode_menu);

    ret1 = NvCtrlGetAttribute(ctrl_target,
                              NV_CTRL_PLATFORM_POWER_MODE,
                              &powerMode);

    if (ret1 != NvCtrlSuccess) {
        return;
    }
    g_signal_handlers_block_by_func(G_OBJECT(ctk_powermode->powermode_menu),
                                    G_CALLBACK(powermode_menu_changed),
                                    (gpointer) ctk_powermode);

    ctk_drop_down_menu_set_current_value(menu, powerMode);

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_powermode->powermode_menu),
                                      G_CALLBACK(powermode_menu_changed),
                                      (gpointer) ctk_powermode);
}



static void powermode_menu_changed(GtkWidget *widget,
                                   gpointer user_data)
{
    CtkPowermode *ctk_powermode = CTK_POWERMODE(user_data);
    CtrlTarget *ctrl_target = ctk_powermode->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    guint powerMode;
    ReturnStatus ret;
    const char *label = ctk_drop_down_menu_get_current_name(menu);

    powerMode = ctk_drop_down_menu_get_current_value(menu);

    ret = NvCtrlSetAttribute(ctrl_target,
                             NV_CTRL_PLATFORM_POWER_MODE,
                             powerMode);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_powermode->ctk_config,
                                     "Unable to set Power Mode to %s.",
                                     label);
        return;
    }

    post_powermode_menu_update(ctk_powermode);
}



static void update_powermode_menu_event(GObject *object,
                                        gpointer arg1,
                                        gpointer user_data)
{
    CtkPowermode *ctk_powermode = CTK_POWERMODE(user_data);

    update_powermode_menu_info(ctk_powermode);

    post_powermode_menu_update(ctk_powermode);
}



GtkTextBuffer *ctk_powermode_create_help(GtkTextTagTable *table,
                                         CtkPowermode *ctk_powermode)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_heading(b, &i, "Platform Power Mode");
    ctk_help_para(b, &i, "%s", __powermode_menu_help);

    ctk_help_heading(b, &i, "Performance Power Mode");
    ctk_help_para(b, &i, "%s", __performance_power_mode_help);

    ctk_help_heading(b, &i, "Balanced Power Mode");
    ctk_help_para(b, &i, "%s", __balanced_power_mode_help);

    ctk_help_heading(b, &i, "Quiet Power Mode");
    ctk_help_para(b, &i, "%s", __quiet_power_mode_help);

    ctk_help_heading(b, &i, "Current Mode");
    ctk_help_para(b, &i, "%s", __current_mode_help);


    ctk_help_finish(b);
    return b;
}



void ctk_powermode_start_timer(GtkWidget *widget)
{
    CtkPowermode *ctk_powermode = CTK_POWERMODE(widget);

    /* Start the powermode timer */

    ctk_config_start_timer(ctk_powermode->ctk_config,
                           (GSourceFunc) update_current_powermode,
                           (gpointer) ctk_powermode);
}



void ctk_powermode_stop_timer(GtkWidget *widget)
{
    CtkPowermode *ctk_powermode = CTK_POWERMODE(widget);

    /* Stop the powermode timer */

    ctk_config_stop_timer(ctk_powermode->ctk_config,
                          (GSourceFunc) update_current_powermode,
                          (gpointer) ctk_powermode);
}

