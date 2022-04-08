/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
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
#include <inttypes.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "msg.h"

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkecc.h"
#include "ctkgpu.h"
#include "ctkbanner.h"

#define _(STRING) gettext(STRING)
#define N_(STRING) STRING

#define DEFAULT_UPDATE_ECC_STATUS_INFO_TIME_INTERVAL 1000

static const char *__ecc_settings_help =
N_("This page allows you to change the Error Correction Code (ECC) "
"setting for this GPU.");

static const char *__ecc_status_help =
N_("Returns the current hardware ECC setting "
"for the targeted GPU.");

static const char *__sbit_error_help =
N_("Returns the number of single-bit ECC errors detected by "
"the targeted GPU since the last system reboot.");

static const char *__dbit_error_help =
N_("Returns the number of double-bit ECC errors detected by "
"the targeted GPU since the last system reboot.");

static const char *__aggregate_sbit_error_help =
N_("Returns the number of single-bit ECC errors detected by the "
"targeted GPU since the last counter reset.");

static const char *__aggregate_dbit_error_help =
N_("Returns the number of double-bit ECC errors detected by the "
"targeted GPU since the last counter reset.");

static const char *__configuration_status_help =
N_("Returns the current ECC configuration setting or specifies new "
"settings.  Changes to these settings do not take effect until the next "
"system reboot.");

static const char *__clear_button_help =
N_("This button is used to clear the ECC errors detected since the last system reboot.");

static const char *__clear_aggregate_button_help =
N_("This button is used to reset the aggregate ECC errors counter.");

static const char *__reset_default_config_button_help =
N_("The button is used to restore the GPU's default ECC configuration setting.");

static void ecc_config_button_toggled(GtkWidget *, gpointer);
static void show_ecc_toggle_warning_dlg(CtkEcc *);
static void ecc_set_config_status(CtkEcc *);
static void ecc_configuration_update_received(GObject *, CtrlEvent *, gpointer);
static void post_ecc_configuration_update(CtkEcc *);
static void reset_default_config_button_clicked(GtkWidget *, gpointer);

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
            NULL  /* value_table */
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
    CtrlTarget *ctrl_target = ctk_ecc->ctrl_target;
    int64_t val;
    gboolean status;
    ReturnStatus ret;


    if (!ctk_ecc->ecc_config_supported && !ctk_ecc->ecc_enabled ) {
        return FALSE;
    }

    /*
     * The ECC Configuration may be changed by non NV-CONTROL clients so we
     * can't rely on an event to update the configuration state.
     */

    if (ctk_ecc->ecc_config_supported) {

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_ECC_CONFIGURATION,
                                 &status);

        if (ret != NvCtrlSuccess ||
            status == NV_CTRL_GPU_ECC_CONFIGURATION_DISABLED) {
                ctk_ecc->ecc_configured = FALSE;
        } else {
                ctk_ecc->ecc_configured = TRUE;
        }

        ecc_set_config_status(ctk_ecc);
    }

    /* If ECC is not enabled, don't query ECC details but continue updating */

    if (ctk_ecc->ecc_enabled == FALSE) {
        return TRUE;
    }

    /* Query ECC Errors */

    if (ctk_ecc->sbit_error) {
        ret = NvCtrlGetAttribute64(ctrl_target,
                                   NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS,
                                   &val);
        if (ret != NvCtrlSuccess) {
            val = 0;
        }
        set_label_value(ctk_ecc->sbit_error, val);
    }

    if (ctk_ecc->dbit_error) {
        ret = NvCtrlGetAttribute64(ctrl_target,
                                   NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,
                                   &val);
        if (ret != NvCtrlSuccess) {
            val = 0;
        }
        set_label_value(ctk_ecc->dbit_error, val);
    }

    if (ctk_ecc->aggregate_sbit_error) {
        ret = NvCtrlGetAttribute64(ctrl_target,
                                   NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS,
                                   &val);
        if (ret != NvCtrlSuccess) {
            val = 0;
        }
        set_label_value(ctk_ecc->aggregate_sbit_error, val);
    }

    if (ctk_ecc->aggregate_dbit_error) {
        ret = NvCtrlGetAttribute64(ctrl_target,
                                   NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS,
                                   &val);
        if (ret != NvCtrlSuccess) {
            val = 0;
        }
        set_label_value(ctk_ecc->aggregate_dbit_error, val);
    }

    return TRUE;
} /* update_ecc_info() */



/*
 * post_ecc_configuration_update() - this function update status bar string.
 */

static void post_ecc_configuration_update(CtkEcc *ctk_ecc)
{
    gboolean configured = ctk_ecc->ecc_configured;
    gboolean enabled = ctk_ecc->ecc_enabled;

    const char *conf_string = configured ? _("enabled") : _("disabled");
    char message[128];

    if (configured != enabled) {
        snprintf(message, sizeof(message), _("ECC will be %s after reboot."),
                 conf_string);
    } else {
        snprintf(message, sizeof(message), "ECC %s.", conf_string);
    }

    ctk_config_statusbar_message(ctk_ecc->ctk_config, "%s", message);
} /* post_ecc_configuration_update() */



/*
 * ecc_set_config_status() - set ECC configuration button status: caller
 * should make sure ctk_ecc->ecc_configured is set correctly before calling.
 */

static void ecc_set_config_status(CtkEcc *ctk_ecc)
{
    g_signal_handlers_block_by_func(G_OBJECT(ctk_ecc->configuration_status),
                                    G_CALLBACK(ecc_config_button_toggled),
                                    (gpointer) ctk_ecc);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_ecc->configuration_status),
                                 ctk_ecc->ecc_configured);

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_ecc->configuration_status),
                                      G_CALLBACK(ecc_config_button_toggled),
                                      (gpointer) ctk_ecc);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_ecc->reset_default_config_button),
         G_CALLBACK(reset_default_config_button_clicked),
         (gpointer) ctk_ecc);
    gtk_widget_set_sensitive
        (ctk_ecc->reset_default_config_button,
         ctk_ecc->ecc_config_supported &&
         (ctk_ecc->ecc_configured != ctk_ecc->ecc_default_status));
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_ecc->reset_default_config_button),
         G_CALLBACK(reset_default_config_button_clicked),
         (gpointer) ctk_ecc);

}



/*
 * ecc_configuration_update_received() - this function is called when the
 * NV_CTRL_GPU_ECC_CONFIGURATION attribute is changed by another
 * NV-CONTROL client.
 */

static void ecc_configuration_update_received(GObject *object,
                                              CtrlEvent *event,
                                              gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    ctk_ecc->ecc_configured = event->int_attr.value;
 
    /* set ECC configuration button status */
    ecc_set_config_status(ctk_ecc);

    /* Update status bar message */
    post_ecc_configuration_update(ctk_ecc);
}



/*
 * reset_default_config_button_clicked() - callback function for reset default
 * configuration button.
 */

static void reset_default_config_button_clicked(GtkWidget *widget,
                                                gpointer user_data)
{
    ReturnStatus ret;
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    CtrlTarget *ctrl_target = ctk_ecc->ctrl_target;

    /* set default status to ECC configuration */
    ret =  NvCtrlSetAttribute(ctrl_target,
                              NV_CTRL_GPU_ECC_CONFIGURATION,
                              ctk_ecc->ecc_default_status);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                     _("Failed to set default configuration!"));
        return;
    }

    ctk_ecc->ecc_configured = ctk_ecc->ecc_default_status;

    /* update ECC configuration button status */
    ecc_set_config_status(ctk_ecc);

    /* show popup dialog*/
    show_ecc_toggle_warning_dlg(ctk_ecc);
    
    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button, FALSE);
    
    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 _("Set to default configuration."));
} /* reset_default_config_button_clicked() */



/* 
 * clear_ecc_errors_button_clicked() - callback function for clear ecc errors
 * button
 */

static void clear_ecc_errors_button_clicked(GtkWidget *widget,
                                            gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    CtrlTarget *ctrl_target = ctk_ecc->ctrl_target;

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_VOLATILE);

    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 _("ECC errors cleared."));
} /* clear_ecc_errors_button_clicked() */



/*
 * clear_aggregate_ecc_errors_button_clicked() - callback function for
 * clear aggregate ecc errors button.
 */

static void clear_aggregate_ecc_errors_button_clicked(GtkWidget *widget,
                                                      gpointer user_data)
{
    CtkEcc *ctk_ecc = CTK_ECC(user_data);
    CtrlTarget *ctrl_target = ctk_ecc->ctrl_target;

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS,
                       NV_CTRL_GPU_ECC_RESET_ERROR_STATUS_AGGREGATE);

    ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                 _("ECC aggregate errors cleared."));
} /* clear_aggregate_ecc_errors_button_clicked() */



static void show_ecc_toggle_warning_dlg(CtkEcc *ctk_ecc)
{
    GtkWidget *dlg, *parent;
    
    /* return early if message dialog already shown */
    if (ctk_ecc->ecc_toggle_warning_dlg_shown) {
        return;
    }

    ctk_ecc_stop_timer(GTK_WIDGET(ctk_ecc));

    ctk_ecc->ecc_toggle_warning_dlg_shown = TRUE;
    parent = ctk_get_parent_window(GTK_WIDGET(ctk_ecc));

    dlg = gtk_message_dialog_new (GTK_WINDOW(parent),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_OK,
                                  _("Changes to the ECC setting "
                                  "require a system reboot before "
                                  "taking effect."));
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy (dlg);

    ctk_ecc_start_timer(GTK_WIDGET(ctk_ecc));
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
    CtrlTarget *ctrl_target = ctk_ecc->ctrl_target;
    ReturnStatus ret;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* show popup dialog when user first time click ECC config */
    show_ecc_toggle_warning_dlg(ctk_ecc);

    /* set the newly specified ECC value */
    ret = NvCtrlSetAttribute(ctrl_target,
                             NV_CTRL_GPU_ECC_CONFIGURATION,
                             enabled);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_ecc->ctk_config,
                                     _("Failed to set ECC configuration!"));
        return;
    }

    ctk_ecc->ecc_configured = enabled;

    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button,
                             ctk_ecc->ecc_config_supported &&
                             (enabled != ctk_ecc->ecc_default_status));

    /* Update status bar message */
    post_ecc_configuration_update(ctk_ecc);
} /* ecc_config_button_toggled() */



GtkWidget* ctk_ecc_new(CtrlTarget *ctrl_target,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkEcc *ctk_ecc;
    GtkWidget *hbox, *hbox2, *vbox, *hsep, *hseparator, *table;
    GtkWidget *banner, *label, *eventbox;
    int64_t sbit_error;
    int64_t aggregate_sbit_error;
    int64_t dbit_error;
    int64_t aggregate_dbit_error;
    gint ecc_config_supported;
    gint val, row = 0;
    gboolean sbit_error_available;
    gboolean aggregate_sbit_error_available;
    gboolean dbit_error_available;
    gboolean aggregate_dbit_error_available;
    gboolean ecc_enabled;
    ReturnStatus ret;
    gchar *ecc_enabled_string;
    gchar *str = NULL;

    /* make sure we have a handle */

    g_return_val_if_fail((ctrl_target != NULL) &&
                         (ctrl_target->h != NULL), NULL);

    /*
     * check if ECC support available.
     */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GPU_ECC_SUPPORTED,
                             &val);
    if (ret != NvCtrlSuccess || val != NV_CTRL_GPU_ECC_SUPPORTED_TRUE) {
       return NULL; 
    }

    /* create the CtkEcc object */

    object = g_object_new(CTK_TYPE_ECC, NULL);
    
    ctk_ecc = CTK_ECC(object);
    ctk_ecc->ctrl_target = ctrl_target;
    ctk_ecc->ctk_config = ctk_config;
    ctk_ecc->ecc_toggle_warning_dlg_shown = FALSE;

    sbit_error_available = TRUE;
    dbit_error_available = TRUE;
    aggregate_sbit_error_available = TRUE;
    aggregate_dbit_error_available = TRUE;
    
    sbit_error = 0;
    dbit_error = 0;
    aggregate_sbit_error = 0;
    aggregate_dbit_error = 0;

    /* Query ECC Status */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_ECC_STATUS,
                             &val);
    if (ret != NvCtrlSuccess || val == NV_CTRL_GPU_ECC_STATUS_DISABLED) {
        ecc_enabled = FALSE;
        ecc_enabled_string = _("Disabled");
    } else {
        ecc_enabled = TRUE;
        ecc_enabled_string = _("Enabled");
    }
    ctk_ecc->ecc_enabled = ecc_enabled; 

    /* Query ECC Configuration */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_ECC_CONFIGURATION,
                             &val);
    if (ret != NvCtrlSuccess ||
        val == NV_CTRL_GPU_ECC_CONFIGURATION_DISABLED) {
            ctk_ecc->ecc_configured = FALSE;
    } else {
            ctk_ecc->ecc_configured = TRUE;
    }

    /* get default status */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION,
                             &val);
    if (ret != NvCtrlSuccess ||
        val == NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION_DISABLED) {
        ctk_ecc->ecc_default_status = FALSE;
    } else {
        ctk_ecc->ecc_default_status = TRUE;
    }

    /* Query ECC errors */
    
    ret = NvCtrlGetAttribute64(ctrl_target,
                               NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS,
                               &sbit_error);
    if (ret != NvCtrlSuccess) {
        sbit_error_available = FALSE;
    }
    ret = NvCtrlGetAttribute64(ctrl_target,
                               NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS,
                               &dbit_error);
    if (ret != NvCtrlSuccess) {
        dbit_error_available = FALSE;
    }
    ret = NvCtrlGetAttribute64(ctrl_target,
                               NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS,
                               &aggregate_sbit_error);
    if (ret != NvCtrlSuccess) {
        aggregate_sbit_error_available = FALSE;
    }
    ret = NvCtrlGetAttribute64(ctrl_target,
                               NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS,
                               &aggregate_dbit_error);
    if (ret != NvCtrlSuccess) {
        aggregate_dbit_error_available = FALSE;
    }
    ctk_ecc->sbit_error_available = sbit_error_available;
    ctk_ecc->aggregate_sbit_error_available = aggregate_sbit_error_available;
    ctk_ecc->dbit_error_available = dbit_error_available;
    ctk_ecc->aggregate_dbit_error_available = aggregate_dbit_error_available;
    /* Query ECC configuration supported */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED,
                             &ecc_config_supported);
    if (ret != NvCtrlSuccess) {
        ecc_config_supported = 0;
    }

    ctk_ecc->ecc_config_supported = ecc_config_supported;

    /* set container properties for the CtkEcc widget */

    gtk_box_set_spacing(GTK_BOX(ctk_ecc), 5);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("ECC Status"));
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

    label = gtk_label_new(_("ECC:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, row, row+1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(ecc_enabled_string);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, _(__ecc_status_help));
    ctk_ecc->status = label;
    
    row += 3;
    
    /* Add ECC Errors */

    if (sbit_error_available && dbit_error_available) {
        ctk_ecc->sbit_error =
            add_table_int_row(ctk_config, table, _(__sbit_error_help),
                              _("Single-bit ECC Errors:"), sbit_error,
                              row, ecc_enabled);
        row += 1; // add vertical padding between rows

        ctk_ecc->dbit_error =
            add_table_int_row(ctk_config, table, _(__dbit_error_help),
                              _("Double-bit ECC Errors:"), dbit_error,
                              row, ecc_enabled);
        row += 3; // add vertical padding between rows
    }

    if (aggregate_sbit_error_available && aggregate_dbit_error_available) {
        ctk_ecc->aggregate_sbit_error =
            add_table_int_row(ctk_config, table, _(__aggregate_sbit_error_help),
                              _("Aggregate Single-bit ECC Errors:"),
                              aggregate_sbit_error, row, ecc_enabled);
        row += 1; // add vertical padding between rows

        ctk_ecc->aggregate_dbit_error =
            add_table_int_row(ctk_config, table, _(__aggregate_dbit_error_help),
                              _("Aggregate Double-bit ECC Errors:"),
                              aggregate_dbit_error, row, ecc_enabled);
    }
    
    /* ECC configuration settings */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("ECC Configuration"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

    hbox2 = gtk_hbox_new(FALSE, 0);
    ctk_ecc->configuration_status =
        gtk_check_button_new_with_label(_("Enable ECC"));
    gtk_box_pack_start(GTK_BOX(hbox2),
                       ctk_ecc->configuration_status, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox2), 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_ecc->configuration_status),
                                 ctk_ecc->ecc_configured);
    ctk_config_set_tooltip(ctk_config, ctk_ecc->configuration_status,
                           _(__configuration_status_help));
    g_signal_connect(G_OBJECT(ctk_ecc->configuration_status), "clicked",
                     G_CALLBACK(ecc_config_button_toggled),
                     (gpointer) ctk_ecc);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_ECC_CONFIGURATION),
                     G_CALLBACK(ecc_configuration_update_received),
                     (gpointer) ctk_ecc);
    gtk_widget_set_sensitive(ctk_ecc->configuration_status, ecc_config_supported);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctk_ecc), hbox, FALSE, FALSE, 0);
    
    /* Add buttons */

    if (sbit_error_available && dbit_error_available) {
        ctk_ecc->clear_button = gtk_button_new_with_label(_("Clear ECC Errors"));
        gtk_box_pack_end(GTK_BOX(hbox), ctk_ecc->clear_button, FALSE, FALSE, 0);
        ctk_config_set_tooltip(ctk_config, ctk_ecc->clear_button,
                               _(__clear_button_help));
        gtk_widget_set_sensitive(ctk_ecc->clear_button, ecc_enabled);
        g_signal_connect(G_OBJECT(ctk_ecc->clear_button), "clicked",
                         G_CALLBACK(clear_ecc_errors_button_clicked),
                         (gpointer) ctk_ecc);
    }

    if (aggregate_sbit_error_available && aggregate_dbit_error_available) {
        ctk_ecc->clear_aggregate_button =
            gtk_button_new_with_label(_("Clear Aggregate ECC Errors"));
        gtk_box_pack_end(GTK_BOX(hbox), ctk_ecc->clear_aggregate_button,
                         FALSE, FALSE, 0);
        ctk_config_set_tooltip(ctk_config, ctk_ecc->clear_button,
                               _(__clear_aggregate_button_help));
        gtk_widget_set_sensitive(ctk_ecc->clear_aggregate_button, ecc_enabled);
        g_signal_connect(G_OBJECT(ctk_ecc->clear_aggregate_button),
                         "clicked",
                         G_CALLBACK(clear_aggregate_ecc_errors_button_clicked),
                         (gpointer) ctk_ecc);
    }

    ctk_ecc->reset_default_config_button =
        gtk_button_new_with_label(_("Reset Default Configuration"));
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox),
                      ctk_ecc->reset_default_config_button);
    gtk_box_pack_end(GTK_BOX(hbox), eventbox, FALSE, FALSE, 5);
    ctk_config_set_tooltip(ctk_config, ctk_ecc->reset_default_config_button,
                           _(__reset_default_config_button_help));
    gtk_widget_set_sensitive(ctk_ecc->reset_default_config_button,
                             ecc_config_supported &&
                             (ecc_enabled != ctk_ecc->ecc_default_status));
    g_signal_connect(G_OBJECT(ctk_ecc->reset_default_config_button),
                     "clicked",
                     G_CALLBACK(reset_default_config_button_clicked),
                     (gpointer) ctk_ecc);

    /* Register a timer callback to update Ecc status info */
    str = g_strdup_printf(_("ECC Settings (GPU %d)"),
                        NvCtrlGetTargetId(ctrl_target));

    ctk_config_add_timer(ctk_ecc->ctk_config,
                         DEFAULT_UPDATE_ECC_STATUS_INFO_TIME_INTERVAL,
                         str,
                         (GSourceFunc) update_ecc_info,
                         (gpointer) ctk_ecc);

    g_free(str);
    
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

    ctk_help_heading(b, &i, _("ECC Settings Help"));
    ctk_help_para(b, &i, "%s", _(__ecc_settings_help));
    
    ctk_help_heading(b, &i, _("ECC"));
    ctk_help_para(b, &i, "%s", _(__ecc_status_help));

    if (ctk_ecc->sbit_error_available && ctk_ecc->dbit_error_available) {
        ctk_help_heading(b, &i, _("Single-bit ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__sbit_error_help));
        ctk_help_heading(b, &i, _("Double-bit ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__dbit_error_help));
    }
    if (ctk_ecc->aggregate_sbit_error_available &&
        ctk_ecc->aggregate_dbit_error_available) {
        ctk_help_heading(b, &i, _("Aggregate Single-bit ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__aggregate_sbit_error_help));
        ctk_help_heading(b, &i, _("Aggregate Double-bit ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__aggregate_dbit_error_help));
    }
    ctk_help_heading(b, &i, _("ECC Configuration"));
    ctk_help_para(b, &i, "%s", _(__configuration_status_help));

    ctk_help_heading(b, &i, _("Enable ECC"));
    ctk_help_para(b, &i, "%s", _(__ecc_status_help));

    if (ctk_ecc->sbit_error_available && ctk_ecc->dbit_error_available) {
        ctk_help_heading(b, &i, _("Clear ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__clear_button_help));
    }
    if (ctk_ecc->aggregate_sbit_error_available &&
        ctk_ecc->aggregate_dbit_error_available) {
        ctk_help_heading(b, &i, _("Clear Aggregate ECC Errors"));
        ctk_help_para(b, &i, "%s", _(__clear_aggregate_button_help));
    }
    
    ctk_help_heading(b, &i, _("Reset Default Configuration"));
    ctk_help_para(b, &i, "%s", _(__reset_default_config_button_help));

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
