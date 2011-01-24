/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "msg.h"

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkecc.h"
#include "ctkgpu.h"
#include "ctkbanner.h"

#define DEFAULT_UPDATE_ECC_STATUS_INFO_TIME_INTERVAL 1000

static const char *__ecc_settings_help =
"This page allows you to change the Error Correction Code (ECC) "
"setting for this GPU. You can also view memory details and the number "
"of ECC events.";

static const char *__ecc_status_help =
"Returns the current hardware ECC setting "
"for the targeted GPU.";

static const char *__dbit_error_help =
"Returns the number of double-bit ECC errors detected by "
"the targeted GPU since the last system reboot.";

static const char *__aggregate_dbit_error_help =
"Returns the number of double-bit ECC errors detected by the "
"targeted GPU since the last counter reset.";

static const char *__configuration_status_help =
"Returns the current ECC configuration setting or specifies new "
"settings.  Changes to these settings do not take effect until the next "
"system reboot.";

static const char *__clear_button_help =
"This button is used to clear the ECC errors detected since the last system reboot.";

static const char *__clear_aggregate_button_help =
"This button is used to reset the aggregate ECC errors counter.";

static const char *__reset_default_config_button_help =
"The button is used to restore the GPU's default ECC configuration setting.";

static void ecc_config_button_toggled(GtkWidget *, gpointer);
static void show_ecc_toggle_warning_dlg(CtkEcc *);

GType ctk_ecc_get_type(void)
{
    static GType ctk_ecc_type = 0;

    if (!ctk_ecc_type) {
        static const GTypeInfo ctk_ecc_info = {
            sizeof (CtkEccClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkEcc),
            0,    /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_ecc_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkEcc",
                                   &ctk_ecc_info, 0);
    }

    return ctk_ecc_type;

} /* ctk_ecc_get_type() */



static void set_label_value(GtkWidget *widget, uint64_t val)
{
    gchar *s;

    s = g_strdup_printf("%" PRIu64, val);
    gtk_label_set_text(GTK_LABEL(widget), s);
    g_free(s);
}



/*
 * add_table_int_row() - helper function to add label-value pair to table.
 */

static GtkWidget *add_table_int_row(CtkConfig *ctk_config, GtkWidget *table,
                                    const gchar *help, gchar *label1,
                                    uint64_t val, gint row,
                                    gboolean ecc_enabled)
{
    GtkWidget *hbox2, *label, *eventbox;

    gtk_table_resize(GTK_TABLE(table), row+1, 2);
    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, row, row+1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(label1);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
    gtk_widget_set_sensitive(label, ecc_enabled);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, row, row+1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    set_label_value(label, val);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, help);
    gtk_widget_set_sensitive(label, ecc_enabled);

    return label;
} /* add_table_int_row() */



/*
 * update_ecc_info() - update ECC status and configuration
 */

static gboolean update_ecc_info(gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    int64_t val;
    gint ecc_config;
    ReturnStatus ret;

    if ( ctk_ecc->ecc_enabled == FALSE ) {
        goto end;
    }
    
    /* Query ECC Errors */

    if ( ctk_ecc->dbit_error ) {
        ret = NvCtrlGetAttribute64(ctk_ecc->handle,
                                   NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,
                                   &val);
        if ( ret != NvCtrlSuccess ) {
            val = 0;
        }
        set_label_value(ctk_ecc->dbit_error, val);
    }

    if ( ctk_ecc->aggregate_dbit_error ) {
        ret = NvCtrlGetAttribute64(ctk_ecc->handle,
                                   NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS,
                                   &val);
        if ( ret != NvCtrlSuccess ) {
            val = 0;
        }
        set_label_value(ctk_ecc->aggregate_dbit_error, val);
    }
end:
    /* Query ECC configuration */

    if ( ctk_ecc->configuration_status ) {
        ret = NvCtrlGetAttribute(ctk_ecc->handle,
                                 NV_CTRL_GPU_ECC_CONFIGURATION,
                                 &ecc_config);
        if (ret != NvCtrlSuccess ||
            ecc_config != NV_CTRL_GPU_ECC_CONFIGURATION_ENABLED) {
            ecc_config = 0;
        } else {
            ecc_config = 1;
        }
        g_signal_handlers_block_by_func(G_OBJECT(ctk_ecc->configuration_status),
                                        G_CALLBACK(ecc_config_button_toggled),
                                        (gpointer) ctk_ecc);
        gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(ctk_ecc->configuration_status),
                            ecc_config);
        g_signal_handlers_unblock_by_func(G_OBJECT(ctk_ecc->configuration_status),
                                          G_CALLBACK(ecc_config_button_toggled),
                                          (gpointer) ctk_ecc);
    }
    
    return TRUE;
} /* update_ecc_info() */



/*
 * reset_default_config_button_clicked() - callback function for reset default
 * configuration button.
 */

static void reset_default_config_button_clicked(GtkWidget *widget,
                                                gpointer user_data)
{
    gboolean status;
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    
    /* get default status and set it to ECC configuration */
    NvCtrlGetAttribute(ctk_ecc->handle,
                       NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION,
                       &status);
    NvCtrlSetAttribute(ctk_ecc->handle,
                       NV_CTRL_GPU_ECC_CONFIGURATION,
                       status);

    /* show popup dialog*/
    show_ecc_toggle_warning_dlg(ctk_ecc);
    
    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button, FALSE);
    
    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 "Set to default configuration.");
} /* reset_default_config_button_clicked() */



/* 
 * clear_ecc_errors_button_clicked() - callback function for clear ecc errors
 * button
 */

static void clear_ecc_errors_button_clicked(GtkWidget *widget,
                                            gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    
    NvCtrlSetAttribute(ctk_ecc->handle,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_VOLATILE);
    
    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 "ECC errors cleared.");
} /* clear_ecc_errors_button_clicked() */



/*
 * clear_aggregate_ecc_errors_button_clicked() - callback function for
 * clear aggregate ecc errors button.
 */

static void clear_aggregate_ecc_errors_button_clicked(GtkWidget *widget,
                                                      gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    
    NvCtrlSetAttribute(ctk_ecc->handle,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_AGGREGATE);
    
    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 "ECC aggregate errors cleared.");
} /* clear_aggregate_ecc_errors_button_clicked() */



static void show_ecc_toggle_warning_dlg(CtkEcc *ctk_ecc)
{
    GtkWidget *dlg, *parent;
    
    /* return early if message dialog already shown */
    if (ctk_ecc->ecc_toggle_warning_dlg_shown) {
        return;
    }
    ctk_ecc->ecc_toggle_warning_dlg_shown = TRUE;
    parent = ctk_get_parent_window(GTK_WIDGET(ctk_ecc));

    dlg = gtk_message_dialog_new (GTK_WINDOW(parent),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_OK,
                                  "Changes to the ECC setting "
                                  "require a system reboot before "
                                  "taking effect.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy (dlg);
}



/*
 * ecc_config_button_toggled() - callback function for
 * enable ECC checkbox.
 */

static void ecc_config_button_toggled(GtkWidget *widget,
                                      gpointer user_data)
{
    gboolean enabled;
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* show popup dialog when user first time click ECC config */
    show_ecc_toggle_warning_dlg(ctk_ecc);

    /* set the newly specified ECC value */
    NvCtrlSetAttribute(ctk_ecc->handle,
                       NV_CTRL_GPU_ECC_CONFIGURATION,
                       enabled);

    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button, TRUE);

    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 "ECC %s.",
                                 enabled ? "enabled" : "disabled");
} /* ecc_config_button_toggled() */



GtkWidget* ctk_ecc_new(NvCtrlAttributeHandle *handle,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkEcc *ctk_ecc;
    GtkWidget *hbox, *hbox2, *vbox, *hsep, *hseparator, *table;
    GtkWidget *banner, *label, *eventbox;
    int64_t dbit_error;
    int64_t aggregate_dbit_error;
    gint ecc_config_supported;
    gint val, row = 0;
    gboolean dbit_error_available;
    gboolean aggregate_dbit_error_available;
    gboolean ecc_enabled;
    gboolean ecc_default_status;
    ReturnStatus ret;
    gchar *ecc_enabled_string;

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /*
     * check if ECC support available.
     */

    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_GPU_ECC_SUPPORTED,
                             &val);
    if (ret != NvCtrlSuccess || val != NV_CTRL_GPU_ECC_SUPPORTED_TRUE) {
       return NULL; 
    }

    /* create the CtkEcc object */

    object = g_object_new(CTK_TYPE_ECC, NULL);
    
    ctk_ecc = CTK_ECC(object);
    ctk_ecc->handle = handle;
    ctk_ecc->ctk_config = ctk_config;
    ctk_ecc->ecc_toggle_warning_dlg_shown = FALSE;

    dbit_error_available = TRUE;
    aggregate_dbit_error_available = TRUE;
    
    dbit_error = 0;
    aggregate_dbit_error = 0;

    /* Query ECC Status */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_ECC_STATUS,
                             &val);
    if (ret != NvCtrlSuccess || val == NV_CTRL_GPU_ECC_STATUS_DISABLED) {
        ecc_enabled = FALSE;
        ecc_enabled_string = "Disabled";
    } else {
        ecc_enabled = TRUE;
        ecc_enabled_string = "Enabled";
    }
    ctk_ecc->ecc_enabled = ecc_enabled; 

    /* get default status */
    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION,
                             &val);
    if (ret != NvCtrlSuccess ||
        val == NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION_DISABLED) {
        ecc_default_status = FALSE;
    } else {
        ecc_default_status = TRUE;
    }

    /* Query ECC errors */
    
    ret = NvCtrlGetAttribute64(handle, NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,
                             &dbit_error);
    if ( ret != NvCtrlSuccess ) {
        dbit_error_available = FALSE;
    }
    ret = NvCtrlGetAttribute64(handle,
                             NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS,
                             &aggregate_dbit_error);
    if ( ret != NvCtrlSuccess ) {
        aggregate_dbit_error_available = FALSE;
    }
    ctk_ecc->dbit_error_available = dbit_error_available;
    ctk_ecc->aggregate_dbit_error_available = aggregate_dbit_error_available;
    /* Query ECC configuration supported */

    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED,
                             &ecc_config_supported);
    if ( ret != NvCtrlSuccess ) {
        ecc_config_supported = 0;
    }

    /* set container properties for the CtkEcc widget */

    gtk_box_set_spacing(GTK_BOX(ctk_ecc), 5);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("ECC Status");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(1, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* ECC Status */
    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, row, row+1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("ECC:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, row, row+1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(ecc_enabled_string);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __ecc_status_help);
    ctk_ecc->status = label;
    
    row += 3;
    
    /* Add ECC Errors */

    if ( dbit_error_available ) {
        ctk_ecc->dbit_error =
            add_table_int_row(ctk_config, table, __dbit_error_help,
                              "Double-bit ECC Errors:", dbit_error,
                              row, ecc_enabled);
        row += 3; // add vertical padding between rows
    }
    
    if ( aggregate_dbit_error_available ) {
        ctk_ecc->aggregate_dbit_error =
            add_table_int_row(ctk_config, table, __aggregate_dbit_error_help,
                              "Aggregate Double-bit ECC Errors:",
                              aggregate_dbit_error, row, ecc_enabled);
    }
    
    /* ECC configuration settings */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("ECC Configuration");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

    hbox2 = gtk_hbox_new(FALSE, 0);
    ctk_ecc->configuration_status =
        gtk_check_button_new_with_label("Enable ECC");
    gtk_box_pack_start(GTK_BOX(hbox2),
                       ctk_ecc->configuration_status, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox2), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_config, ctk_ecc->configuration_status,
                           __configuration_status_help);
    g_signal_connect(G_OBJECT(ctk_ecc->configuration_status), "clicked",
                     G_CALLBACK(ecc_config_button_toggled),
                     (gpointer) ctk_ecc);
    gtk_widget_set_sensitive(ctk_ecc->configuration_status, ecc_config_supported);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctk_ecc), hbox, FALSE, FALSE, 0);
    
    /* Add buttons */

    if ( dbit_error_available ) {
        ctk_ecc->clear_button = gtk_button_new_with_label("Clear ECC Errors");
        gtk_box_pack_end(GTK_BOX(hbox), ctk_ecc->clear_button, FALSE, FALSE, 0);
        ctk_config_set_tooltip(ctk_config, ctk_ecc->clear_button,
                               __clear_button_help);
        gtk_widget_set_sensitive(ctk_ecc->clear_button, ecc_enabled);
        g_signal_connect(G_OBJECT(ctk_ecc->clear_button), "clicked",
                         G_CALLBACK(clear_ecc_errors_button_clicked),
                         (gpointer) ctk_ecc);
    }

    if ( aggregate_dbit_error_available ) {
        ctk_ecc->clear_aggregate_button =
            gtk_button_new_with_label("Clear Aggregate ECC Errors");
        gtk_box_pack_end(GTK_BOX(hbox), ctk_ecc->clear_aggregate_button,
                         FALSE, FALSE, 0);
        ctk_config_set_tooltip(ctk_config, ctk_ecc->clear_button,
                               __clear_aggregate_button_help);
        gtk_widget_set_sensitive(ctk_ecc->clear_aggregate_button, ecc_enabled);
        g_signal_connect(G_OBJECT(ctk_ecc->clear_aggregate_button),
                         "clicked",
                         G_CALLBACK(clear_aggregate_ecc_errors_button_clicked),
                         (gpointer) ctk_ecc);
    }

    ctk_ecc->reset_default_config_button =
        gtk_button_new_with_label("Reset Default Configuration");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),
                      ctk_ecc->reset_default_config_button);
    gtk_box_pack_end(GTK_BOX(hbox), eventbox, FALSE, FALSE, 5);
    ctk_config_set_tooltip(ctk_config, ctk_ecc->reset_default_config_button,
                           __reset_default_config_button_help);
    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button,
                             ecc_config_supported &&
                             (ecc_enabled != ecc_default_status));
    g_signal_connect(G_OBJECT(ctk_ecc->reset_default_config_button),
                     "clicked",
                     G_CALLBACK(reset_default_config_button_clicked),
                     (gpointer) ctk_ecc);

    /* Register a timer callback to update Ecc status info */

    ctk_config_add_timer(ctk_ecc->ctk_config,
                         DEFAULT_UPDATE_ECC_STATUS_INFO_TIME_INTERVAL,
                         "ECC Settings",
                         (GSourceFunc) update_ecc_info,
                         (gpointer) ctk_ecc);
    
    gtk_widget_show_all(GTK_WIDGET(ctk_ecc));

    update_ecc_info(ctk_ecc);

    return GTK_WIDGET(ctk_ecc);
}

GtkTextBuffer *ctk_ecc_create_help(GtkTextTagTable *table,
                                   CtkEcc *ctk_ecc)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_heading(b, &i, "ECC Settings Help");
    ctk_help_para(b, &i, __ecc_settings_help);
    
    ctk_help_heading(b, &i, "ECC");
    ctk_help_para(b, &i, __ecc_status_help);

    if (ctk_ecc->dbit_error_available) {
        ctk_help_heading(b, &i, "Double-bit ECC Errors");
        ctk_help_para(b, &i, __dbit_error_help);
    }
    if (ctk_ecc->aggregate_dbit_error_available) {
        ctk_help_heading(b, &i, "Aggregate Double-bit ECC Errors");
        ctk_help_para(b, &i, __aggregate_dbit_error_help);
    }
    ctk_help_heading(b, &i, "ECC Configuration");
    ctk_help_para(b, &i, __configuration_status_help);

    ctk_help_heading(b, &i, "Enable ECC");
    ctk_help_para(b, &i, __ecc_status_help);

    if (ctk_ecc->dbit_error_available) {
        ctk_help_heading(b, &i, "Clear ECC Errors");
        ctk_help_para(b, &i, __clear_button_help);
    }
    if (ctk_ecc->aggregate_dbit_error_available) {
        ctk_help_heading(b, &i, "Clear Aggregate ECC Errors");
        ctk_help_para(b, &i, __clear_aggregate_button_help);
    }
    
    ctk_help_heading(b, &i, "Reset Default Configuration");
    ctk_help_para(b, &i, __reset_default_config_button_help);

    ctk_help_finish(b);

    return b;
}

void ctk_ecc_start_timer(GtkWidget *widget)
{
    CtkEcc *ctk_ecc = CTK_ECC(widget);

    /* Start the ECC timer */

    ctk_config_start_timer(ctk_ecc->ctk_config,
                           (GSourceFunc) update_ecc_info,
                           (gpointer) ctk_ecc);
}

void ctk_ecc_stop_timer(GtkWidget *widget)
{
    CtkEcc *ctk_ecc = CTK_ECC(widget);

    /* Stop the ECC timer */

    ctk_config_stop_timer(ctk_ecc->ctk_config,
                          (GSourceFunc) update_ecc_info,
                          (gpointer) ctk_ecc);
}
