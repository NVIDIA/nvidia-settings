/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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

#include <stdlib.h> /* malloc */
#include <stdio.h> /* snprintf */
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "ctkbanner.h"
#include "msg.h"
#include "ctkvcs.h"
#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkutils.h"

#define DEFAULT_UPDATE_VCS_INFO_TIME_INTERVAL 5000

#define VCS_PSU_STATE_NORMAL                  0
#define VCS_PSU_STATE_ABNORMAL                1

static gboolean update_vcs_info(gpointer);
static gboolean update_fan_status(CtkVcs *ctk_object);

static void apply_fan_entry_token(char *token, char *value, void *data)
{
    FanEntryPtr pFanEntry = (FanEntryPtr) data;

    if (!strcasecmp("fan", token)) {
        pFanEntry->fan_number = atoi(value);
    } else if (!strcasecmp("speed", token)) {
        pFanEntry->fan_speed = atoi(value);
    } else if (!strcasecmp("fail", token)) {
        pFanEntry->fan_failed = atoi(value);
    } else {
        nv_warning_msg("Unknown Fan Entry token value pair: %s=%s",
                       token, value);
    }
}

static void apply_thermal_entry_token(char *token, char *value, void *data)
{
    ThermalEntryPtr pThermalEntry = (ThermalEntryPtr) data;

    if (!strcasecmp("intake", token)) {
        pThermalEntry->intake_temp = atoi(value);
    } else if (!strcasecmp("exhaust", token)) {
        pThermalEntry->exhaust_temp = atoi(value);
    } else if (!strcasecmp("board", token)) {
        pThermalEntry->board_temp = atoi(value);
    } else {
        nv_warning_msg("Unknown Thermal Entry token value pair: %s=%s",
                       token, value);
    }
}

static void apply_psu_entry_token(char *token, char *value, void *data)
{
    PSUEntryPtr pPSUEntry = (PSUEntryPtr) data;

    if (!strcasecmp("current", token)) {
        pPSUEntry->psu_current = atoi(value);
    } else if (!strcasecmp("power", token)) {
        if (!strcasecmp("unknown", value)) {
            pPSUEntry->psu_power = -1;
        } else {
            pPSUEntry->psu_power = atoi(value);
        }
    } else if (!strcasecmp("voltage", token)) {
        if (!strcasecmp("unknown", value)) {
            pPSUEntry->psu_voltage = -1;
        } else {
            pPSUEntry->psu_voltage = atoi(value);
        }
    } else if (!strcasecmp("state", token)) {
        if (!strcasecmp("normal", value)) {
            pPSUEntry->psu_state = VCS_PSU_STATE_NORMAL;
        } else {
            pPSUEntry->psu_state = VCS_PSU_STATE_ABNORMAL;
        }
    } else {
        nv_warning_msg("Unknown PSU Entry token value pair: %s=%s",
                       token, value);
    }
}

GType ctk_vcs_get_type(void)
{
    static GType ctk_vcs_type = 0;

    if (!ctk_vcs_type) {
        static const GTypeInfo ctk_vcs_info = {
            sizeof (CtkVcsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkVcs),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_vcs_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkVcs", &ctk_vcs_info, 0);
    }

    return ctk_vcs_type;
}


static gboolean update_vcs_info(gpointer user_data)
{
    char output_str[16];
    char *temp_str = NULL;
    char *psu_str = NULL;
    CtkVcs *ctk_object = CTK_VCS(user_data);
    ThermalEntry thermEntry;
    PSUEntry psuEntry; 
    gboolean high_perf_mode;

    /* These queries should always succeed for Canoas 2.0 and above */
    if ((NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_VCSC_HIGH_PERF_MODE, 
                            &high_perf_mode) != NvCtrlSuccess) ||
        (NvCtrlGetStringAttribute(ctk_object->handle,
                                  NV_CTRL_STRING_VCSC_TEMPERATURES,
                                   &temp_str) != NvCtrlSuccess) ||
        (NvCtrlGetStringAttribute(ctk_object->handle,
                                  NV_CTRL_STRING_VCSC_PSU_INFO,
                                  &psu_str) != NvCtrlSuccess)) {
            return FALSE;
    }

    /* Extract out thermal and PSU entry tokens */

    /* First Invalidate thermal and psu entries */
    thermEntry.intake_temp  = -1;
    thermEntry.exhaust_temp = -1;
    thermEntry.board_temp   = -1;

    psuEntry.psu_current    = -1;
    psuEntry.psu_power      = -1;
    psuEntry.psu_voltage    = -1;
    psuEntry.psu_state      = -1;

    if (temp_str) {
        parse_token_value_pairs(temp_str, apply_thermal_entry_token, &thermEntry);
    }
    if (psu_str) {
        parse_token_value_pairs(psu_str, apply_psu_entry_token, &psuEntry);
    }

    if ((thermEntry.intake_temp  != -1) &&
        (thermEntry.exhaust_temp != -1) &&
        (thermEntry.board_temp   != -1)) {
        if (ctk_object->intake_temp) {
            g_snprintf(output_str, 16, "%d C", thermEntry.intake_temp);
            gtk_label_set_text(GTK_LABEL(ctk_object->intake_temp), output_str);
        }
        if (ctk_object->exhaust_temp) {
            g_snprintf(output_str, 16, "%d C", thermEntry.exhaust_temp);
            gtk_label_set_text(GTK_LABEL(ctk_object->exhaust_temp), output_str);
        }
        if (ctk_object->board_temp) {
            g_snprintf(output_str, 16, "%d C", thermEntry.board_temp);
            gtk_label_set_text(GTK_LABEL(ctk_object->board_temp), output_str);
        }
    }

    if ((psuEntry.psu_current != -1) &&
        (psuEntry.psu_state   != -1)) {
        if (ctk_object->psu_current) {
            g_snprintf(output_str, 16, "%d A", psuEntry.psu_current);
            gtk_label_set_text(GTK_LABEL(ctk_object->psu_current), output_str);
        }
        if (ctk_object->psu_state) {
            switch (psuEntry.psu_state) {
            case VCS_PSU_STATE_NORMAL:
                g_snprintf(output_str, 16, "Normal");
                break;
            case VCS_PSU_STATE_ABNORMAL:
                g_snprintf(output_str, 16, "Abnormal");
                break;
            default:
                g_snprintf(output_str, 16, "Unknown");
                break;
            }
            gtk_label_set_text(GTK_LABEL(ctk_object->psu_state), output_str);
        }
    }
    if (ctk_object->psu_power && psuEntry.psu_power != -1) {
        g_snprintf(output_str, 16, "%d W", psuEntry.psu_power);
        gtk_label_set_text(GTK_LABEL(ctk_object->psu_power), output_str);
    }

    if (ctk_object->psu_voltage && psuEntry.psu_voltage != -1) {
        g_snprintf(output_str, 16, "%d V", psuEntry.psu_voltage);
        gtk_label_set_text(GTK_LABEL(ctk_object->psu_voltage), output_str);
    }

    if (!update_fan_status(ctk_object)) {
        return FALSE;
    }

    return TRUE;
}



/** create_error_dialog() *********************************
 *
 * Creates a generic error message dialog widget
 *
 **/

static GtkWidget * create_error_dialog(CtkVcs *ctk_object)
{
    GtkWidget *dialog;
    GtkWidget *image;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;

    /* Display validation override confirmation dialog */
    dialog = gtk_dialog_new_with_buttons
        ("Cannot Apply",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL |
         GTK_DIALOG_DESTROY_WITH_PARENT,
         NULL);

    /* Main horizontal box */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       hbox, TRUE, TRUE, 5);

    /* Pack the information icon */
    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
                                     GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment(GTK_MISC(image), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
    
    /* Main vertical box */
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    ctk_object->error_dialog_label = label;

    /* Action Buttons */
    gtk_dialog_add_button(GTK_DIALOG(dialog), "OK",
                          GTK_RESPONSE_ACCEPT);

    gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

    return dialog;

} /* create_error_dialog() */


static void vcs_perf_checkbox_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkVcs *ctk_object = CTK_VCS(user_data);
    gint enabled;
    gint ret;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    ret = NvCtrlSetAttribute(ctk_object->handle, NV_CTRL_VCSC_HIGH_PERF_MODE, enabled);

    if (ret != NvCtrlSuccess) {
        if (ctk_object->error_dialog_label) {
            gchar *str;
            str = g_strdup_printf("Failed to %s High Performance mode!",
                                  (enabled ? "enable" : "disable"));
            gtk_label_set_text(GTK_LABEL(ctk_object->error_dialog_label), str);
            gtk_window_set_resizable(GTK_WINDOW(ctk_object->error_dialog), FALSE);
            gtk_window_set_transient_for
                (GTK_WINDOW(ctk_object->error_dialog),
                 GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
            gtk_widget_show(ctk_object->error_dialog);
            gtk_dialog_run(GTK_DIALOG(ctk_object->error_dialog));
            gtk_widget_hide(ctk_object->error_dialog);
        }
        goto fail;
    }
    return;
 fail:
    g_signal_handlers_block_by_func(G_OBJECT(widget),
                                    G_CALLBACK(vcs_perf_checkbox_toggled),
                                    (gpointer) ctk_object);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), !enabled);
    gtk_widget_set_sensitive(widget, FALSE);
    g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                      G_CALLBACK(vcs_perf_checkbox_toggled),
                                      (gpointer) ctk_object);
}



static gboolean update_fan_status(CtkVcs *ctk_object)
{
    gint ret;
    char *fan_entry_str = NULL;
    char *tokens;
    GtkWidget *table;
    GtkWidget *label;
    FanEntry current_fan;
    gchar output_str[16];
    gint current_row;

    if (!ctk_object->fan_status_container) {
        return FALSE;
    }
    ret = NvCtrlGetStringAttribute(ctk_object->handle,
                                   NV_CTRL_STRING_VCSC_FAN_STATUS,
                                   &fan_entry_str);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    ctk_empty_container(ctk_object->fan_status_container);

    /* Generate the new table */

    table = gtk_table_new(1, 3, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_box_pack_start(GTK_BOX(ctk_object->fan_status_container), 
                       table, FALSE, FALSE, 0);


    label = gtk_label_new("Fan Number");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_widget_set_size_request(label, ctk_object->req.width, -1);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Fan Speed");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Fan Status");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);


    /* Parse string of fan entries and populate table */
    current_row = 1;
    for (tokens = strtok(fan_entry_str, ";");
         tokens;
         tokens = strtok(NULL, ";")) {

        /* Invalidate fan entry */
        current_fan.fan_number = -1;
        current_fan.fan_speed = -1;
        current_fan.fan_failed = -1;

        parse_token_value_pairs(tokens, apply_fan_entry_token, &current_fan);

        if ((current_fan.fan_number != -1) &&
            (current_fan.fan_speed != -1) &&
            (current_fan.fan_failed != -1)) {
    
            gtk_table_resize(GTK_TABLE(table), current_row + 1, 3);
            g_snprintf(output_str, 16, "%d", current_fan.fan_number);
            label = gtk_label_new(output_str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_widget_set_size_request(label, ctk_object->req.width, -1);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, current_row,
                             current_row + 1, GTK_FILL,
                             GTK_FILL | GTK_EXPAND, 5, 0);
 
            g_snprintf(output_str, 16, "%d rpm", current_fan.fan_speed);
            label = gtk_label_new(output_str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, current_row,
                             current_row + 1, GTK_FILL,
                             GTK_FILL | GTK_EXPAND, 5, 0);


            if (!current_fan.fan_failed) {
                g_snprintf(output_str, 16, "Ok");
            } else {
                g_snprintf(output_str, 16, "Failed");
            }
            label = gtk_label_new(output_str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 2, 3, current_row,
                             current_row + 1, GTK_FILL,
                             GTK_FILL | GTK_EXPAND, 5, 0);

            current_row++;

        } else {
            nv_warning_msg("Incomplete Fan Entry (fan=%d, speed=%d, failFlag=%d)",
                           current_fan.fan_number,
                           current_fan.fan_speed,
                           current_fan.fan_failed);
        }
    }
    gtk_widget_show_all(table);
    XFree(fan_entry_str);
    return TRUE;
}


/*
 * CTK VCS (Visual Computing System) widget creation
 *
 */
GtkWidget* ctk_vcs_new(NvCtrlAttributeHandle *handle,
                        CtkConfig *ctk_config)
{
    GObject *object;
    CtkVcs *ctk_object;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *event;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;
    GtkWidget *scrollWin;
    GtkWidget *checkbutton;

    gchar *product_name;
    gchar *serial_number;
    gchar *build_date;
    gchar *product_id;
    gchar *firmware_version;
    gchar *hardware_version;

    gint current_row;
    gboolean high_perf_mode;

    ReturnStatus ret;
    gchar *s;
    char *psu_str = NULL;
    PSUEntry psuEntry;

    GtkWidget *vbox_scroll, *hbox_scroll;

    /*
     * get the static string data that we will display below
     */
    
    /* Product Name */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_PRODUCT_NAME,
                                   &product_name);
    if (ret != NvCtrlSuccess) {
        product_name = g_strdup("Unable to determine");
    }

    /* Serial Number */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_SERIAL_NUMBER,
                                   &serial_number);
    if (ret != NvCtrlSuccess) {
        serial_number = g_strdup("Unable to determine");
    }

    /* Build Date */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_BUILD_DATE,
                                   &build_date);
    if (ret != NvCtrlSuccess) {
        build_date = g_strdup("Unable to determine");
    }

    /* Product ID */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_PRODUCT_ID,
                                   &product_id);
    if (ret != NvCtrlSuccess) {
        product_id = g_strdup("Unable to determine");
    }

    /* Firmware Version */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_FIRMWARE_VERSION,
                                   &firmware_version);
    if (ret != NvCtrlSuccess) {
        firmware_version = g_strdup("Unable to determine");
    }

    /* Hardware Version */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_HARDWARE_VERSION,
                                   &hardware_version);
    if (ret != NvCtrlSuccess) {
        hardware_version = g_strdup("Unable to determine");
    }


    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_VCS, NULL);
    ctk_object = CTK_VCS(object);

    /* cache the attribute handle */

    ctk_object->handle = handle;
    ctk_object->ctk_config = ctk_config;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_object), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_VCS);
    gtk_box_pack_start(GTK_BOX(ctk_object), banner, FALSE, FALSE, 0);

    /*
     * This displays basic System information, including
     * display name, Operating system type and the NVIDIA driver version.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_object), vbox, TRUE, TRUE, 0);

    /* General purpose error dialog */
    ctk_object->error_dialog = create_error_dialog(ctk_object);

    if (NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_VCSC_HIGH_PERF_MODE,
                            &high_perf_mode) == NvCtrlSuccess) {

        hbox = gtk_hbox_new(FALSE, 0);
        checkbutton = gtk_check_button_new_with_label("Enable High Performance Mode");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), high_perf_mode);
        g_signal_connect(G_OBJECT(checkbutton), "toggled", 
                         G_CALLBACK(vcs_perf_checkbox_toggled),
                         (gpointer) ctk_object);
        gtk_box_pack_start(GTK_BOX(hbox), checkbutton, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0); 
    }

    /* Create the Scrolling Window */
    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox_scroll = gtk_hbox_new(FALSE, 0);
    vbox_scroll = gtk_vbox_new(FALSE, 5);
    event = gtk_event_box_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL, &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL, &(event->style->base[GTK_STATE_NORMAL]));
    gtk_container_add(GTK_CONTAINER(event), hbox_scroll);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          event);
    gtk_box_pack_start(GTK_BOX(hbox_scroll), vbox_scroll, TRUE, TRUE, 5);
    gtk_widget_set_size_request(scrollWin, -1, 50);
    gtk_box_pack_start(GTK_BOX(vbox), scrollWin, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_scroll), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("VCS Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox_scroll), table, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Product Name:", 0, 0.5, product_name);
    add_table_row(table, 1,
                  0, 0.5, "Serial Number:", 0, 0.5, serial_number);
    add_table_row(table, 2,
                  0, 0.5, "Build Date:", 0, 0.5, build_date);
    add_table_row(table, 3,
                  0, 0.5, "Product ID:", 0, 0.5, product_id);
    add_table_row(table, 4,
                  0, 0.5, "Firmware version:", 0, 0.5, firmware_version);
    add_table_row(table, 5,
                  0, 0.5, "Hardware version:", 0, 0.5, hardware_version);

    g_free(product_name);
    g_free(serial_number);
    g_free(build_date);
    g_free(product_id);
    g_free(firmware_version);
    g_free(hardware_version);


    /* Query Canoas 2.0 specific details */
    if ((NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_VCSC_HIGH_PERF_MODE, 
                            &high_perf_mode) == NvCtrlSuccess) && 
        (NvCtrlGetStringAttribute(ctk_object->handle,
                                  NV_CTRL_STRING_VCSC_PSU_INFO,
                                  &psu_str)  == NvCtrlSuccess)) {
        GtkWidget *vbox_padding;

        /* Show the additonal queried information */


        /* Populate scrolling window with data */
        vbox_padding = gtk_vbox_new(FALSE, 0);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), vbox_padding, FALSE, FALSE, 1);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("VCS Thermal Information");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        hseparator = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

        table = gtk_table_new(3, 2, FALSE);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), table, FALSE, FALSE, 0);

        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);

        gtk_container_set_border_width(GTK_CONTAINER(table), 5);

        label = gtk_label_new("Intake Temperature:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        label = gtk_label_new(NULL);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        ctk_object->intake_temp = label;

        label = gtk_label_new("Exhaust Temperature:");
        /* This is the current largest label.  Get its size */
        gtk_widget_size_request(label, &ctk_object->req);

        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        label = gtk_label_new(NULL);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        ctk_object->exhaust_temp = label;

        label = gtk_label_new("Board Temperature:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        label = gtk_label_new(NULL);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        ctk_object->board_temp = label;

        /* Populate table for PSU information */

        psuEntry.psu_current    = -1;
        psuEntry.psu_power      = -1;
        psuEntry.psu_voltage    = -1;
        psuEntry.psu_state      = -1;

        if (psu_str) {
            parse_token_value_pairs(psu_str, apply_psu_entry_token, &psuEntry);
        }

        vbox_padding = gtk_vbox_new(FALSE, 0);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), vbox_padding, FALSE, FALSE, 1);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("VCS Power Supply Unit Information");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        hseparator = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

        table = gtk_table_new(4, 2, FALSE);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), table, FALSE, FALSE, 0);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);

        label = gtk_label_new("PSU State:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, ctk_object->req.width, -1);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);


        label = gtk_label_new(NULL);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        ctk_object->psu_state = label;

        label = gtk_label_new("PSU Current:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, ctk_object->req.width, -1);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);


        label = gtk_label_new(NULL);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        ctk_object->psu_current = label;

        current_row = 2;

        if (psuEntry.psu_power != -1) {
            label = gtk_label_new("PSU Power:");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_widget_set_size_request(label, ctk_object->req.width, -1);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, 
                             current_row, current_row + 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            label = gtk_label_new(NULL);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, 
                             current_row, current_row + 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            ctk_object->psu_power = label;
            current_row++;
        }

        if (psuEntry.psu_voltage != -1) {
            label = gtk_label_new("PSU Voltage:");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_widget_set_size_request(label, ctk_object->req.width, -1);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, 
                             current_row, current_row + 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            label = gtk_label_new(NULL);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, 
                             current_row, current_row + 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            ctk_object->psu_voltage = label;
        }

        /* Create container for fan status table */

        vbox_padding = gtk_vbox_new(FALSE, 0);
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), vbox_padding, FALSE, FALSE, 1);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("VCS Fan Status");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        hseparator = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox_scroll), hbox, FALSE, FALSE, 0);
        ctk_object->fan_status_container = hbox;

        /* Register a timer callback to update the dynamic information */
        s = g_strdup_printf("VCS Monitor (VCS %d)",
                            NvCtrlGetTargetId(ctk_object->handle));

        ctk_config_add_timer(ctk_object->ctk_config,
                             DEFAULT_UPDATE_VCS_INFO_TIME_INTERVAL,
                             s,
                             (GSourceFunc) update_vcs_info,
                             (gpointer) ctk_object);
        g_free(s);

        update_vcs_info(ctk_object);
    }

    gtk_widget_show_all(GTK_WIDGET(object));
    
    return GTK_WIDGET(object);
}



/*
 * VCS help screen
 */
GtkTextBuffer *ctk_vcs_create_help(GtkTextTagTable *table,
                                    CtkVcs *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "VCS (Visual Computing System) Help");

    ctk_help_heading(b, &i, "Product Name");
    ctk_help_para(b, &i, "This is the product name of the VCS.");
    
    ctk_help_heading(b, &i, "Serial Number");
    ctk_help_para(b, &i, "This is the unique serial number of the VCS.");

    ctk_help_heading(b, &i, "Build Date");
    ctk_help_para(b, &i, "This is the date the VCS was build, "
                  "shown in a 'week.year' format");

    ctk_help_heading(b, &i, "Product ID");
    ctk_help_para(b, &i, "This identifies the VCS configuration.");

    ctk_help_heading(b, &i, "Firmware Version");
    ctk_help_para(b, &i, "This is the firmware version currently running on "
                  "the VCS.");

    ctk_help_heading(b, &i, "Hardware Version");
    ctk_help_para(b, &i, "This is the hardware version of the VCS.");

    ctk_help_finish(b);

    return b;
}

void ctk_vcs_start_timer(GtkWidget *widget)
{
    CtkVcs *ctk_vcs = CTK_VCS(widget);

    /* Start the VCS timer */
    ctk_config_start_timer(ctk_vcs->ctk_config,
                           (GSourceFunc) update_vcs_info,
                           (gpointer) ctk_vcs);
}

void ctk_vcs_stop_timer(GtkWidget *widget)
{
    CtkVcs *ctk_vcs = CTK_VCS(widget);

    /* Stop the VCS timer */
    ctk_config_stop_timer(ctk_vcs->ctk_config,
                          (GSourceFunc) update_vcs_info,
                          (gpointer) ctk_vcs);
}

