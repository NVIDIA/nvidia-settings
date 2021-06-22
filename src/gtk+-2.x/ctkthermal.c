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

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>
#include <stdlib.h>

#include "ctkutils.h"
#include "ctkscale.h"
#include "ctkhelp.h"
#include "ctkthermal.h"
#include "ctkgauge.h"
#include "ctkbanner.h"

#define FRAME_PADDING 10
#define DEFAULT_UPDATE_THERMAL_INFO_TIME_INTERVAL 1000

static gboolean update_thermal_info(gpointer);
static gboolean update_cooler_info(gpointer);
static void sync_gui_sensitivity(CtkThermal *ctk_thermal);
static void sync_gui_to_modify_cooler_level(CtkThermal *ctk_thermal);
static gboolean sync_gui_to_update_cooler_event(gpointer user_data);
static void cooler_control_checkbox_toggled(GtkWidget *widget, gpointer user_data);
static void cooler_operating_level_changed(GObject *object, CtrlEvent *event,
                                           gpointer user_data);
static void apply_button_clicked(GtkWidget *widget, gpointer user_data);
static void reset_button_clicked(GtkWidget *widget, gpointer user_data);
static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data);

static void draw_sensor_gui(GtkWidget *vbox1, CtkThermal *ctk_thermal,
                            gboolean new_target_type, gint cur_sensor_idx,
                            gint reading, gint lower, gint upper,
                            gint target, gint provider, gint slowdown);
static GtkWidget *pack_gauge(GtkWidget *hbox, gint lower, gint upper,
                             CtkConfig *ctk_config, const char *help);

static const char *__slowdown_threshold_help =
"The Slowdown Threshold Temperature is the temperature "
"at which the NVIDIA Accelerated Graphics driver will throttle "
"the GPU to prevent damage, in \xc2\xb0"
/* split for g_utf8_validate() */ "C.";

static const char *__core_threshold_help =
"The Core Slowdown Threshold Temperature is the temperature "
"at which the NVIDIA Accelerated Graphics driver will throttle "
"the GPU to prevent damage, in \xc2\xb0"
/* split for g_utf8_validate() */ "C.";

static const char *__core_temp_help =
"The Core Temperature is the Graphics Processing Unit's "
"(GPU) current core temperature, in \xc2\xb0"
/* split for g_utf8_validate() */ "C.";

static const char *__ambient_temp_help =
"The Ambient Temperature is the current temperature in the "
"GPU's immediate neighbourhood, in \xc2\xb0"
/* split for g_utf8_validate() */ "C.";

static const char *__temp_level_help =
"This is a graphical representation of the current GPU core "
"temperature relative to the maximum GPU Core Slowdown "
"Threshold temperature.";

static const char *__thermal_sensor_id_help =
"This shows the thermal sensor's index.";

static const char *__thermal_sensor_target_help =
"This shows what hardware component the thermal sensor is measuring.";

static const char *__thermal_sensor_provider_help =
"This shows the hardware device that provides the thermal sensor.";

static const char *__thermal_sensor_reading_help =
"This shows the thermal sensor's current reading.";

static const char * __enable_button_help =
"The Enable GPU Fan Settings checkbox enables access to control GPU Fan "
"Speed.  Manually configuring the GPU fan speed is not normally required; the "
"speed should adjust automatically based on current temperature and load.";

static const char * __fan_id_help =
"This shows the GPU Fan's index.";

static const char * __fan_rpm_help =
"This shows the current GPU Fan Speed in rotations per minute (RPM).";

static const char * __fan_speed_help =
"This shows the current GPU Fan Speed level as a percentage.";

static const char * __fan_control_type_help =
"Fan Type indicates if and how this fan may be controlled.  Possible "
"types are Variable, Toggle or Restricted.  Variable fans can be "
"freely adjusted within a given range, while Toggle fans can "
"be turned either ON or OFF.  Restricted fans are not adjustable "
"under end user control.";

static const char * __fan_cooling_target_help =
"Fan target shows which graphics device component is being cooled by "
"a given fan.  The target may be GPU, Memory, Power Supply or "
"All.";

static const char * __apply_button_help =
"The Apply button allows you to set the desired speed for the "
"GPU Fans. Slider positions are only applied "
"after clicking this button.";

static const char * __reset_button_help =
"The Reset Hardware Defaults button lets you restore the original GPU "
"Fan Speed and Fan control policy.";

GType ctk_thermal_get_type(void)
{
    static GType ctk_thermal_type = 0;

    if (!ctk_thermal_type) {
        static const GTypeInfo ctk_thermal_info = {
            sizeof (CtkThermalClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkThermal),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_thermal_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkThermal",
                                   &ctk_thermal_info, 0);
    }

    return ctk_thermal_type;

} /* ctk_thermal_get_type() */



/*
 * update_cooler_info() - Update all cooler information
 */
static gboolean update_cooler_info(gpointer user_data)
{
    int i, speed, level, cooler_type, cooler_target;
    gchar *tmp_str;
    CtkThermal *ctk_thermal;
    GtkWidget *table, *label, *eventbox;
    gint ret;
    gint row_idx; /* Where to insert into the cooler info table */
    
    ctk_thermal = CTK_THERMAL(user_data);

    /* Since table cell management in GTK lacks, just remove and rebuild
     * the table from scratch.
     */

    /* Dump out the old table */

    ctk_empty_container(ctk_thermal->cooler_table_hbox);

    /* Generate a new table */

    table = gtk_table_new(1, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    gtk_box_pack_start(GTK_BOX(ctk_thermal->cooler_table_hbox),
                       table, FALSE, FALSE, 0);

    label = gtk_label_new("ID");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox, __fan_id_help);

    label = gtk_label_new("Speed (RPM)");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox, __fan_rpm_help);

    label = gtk_label_new("Speed (%)");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox, __fan_speed_help);

    label = gtk_label_new("Control Type");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox,
                           __fan_control_type_help);

    label = gtk_label_new("Cooling Target");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 4, 5, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox,
                           __fan_cooling_target_help);

    /* Fill the cooler info */
    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        row_idx = i+1;

        gtk_table_resize(GTK_TABLE(table), row_idx+1, 5);

        tmp_str = g_strdup_printf("%d", i);
        label = gtk_label_new(tmp_str);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, row_idx, row_idx+1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        free(tmp_str);

        ret = NvCtrlGetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_SPEED,
                                 &speed);
        if (ret == NvCtrlSuccess) {
            tmp_str = g_strdup_printf("%d", speed);
        }
        else {
            tmp_str = g_strdup_printf("Unsupported");
        }
        label = gtk_label_new(tmp_str);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, row_idx, row_idx+1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        free(tmp_str);

        ret = NvCtrlGetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_LEVEL,
                                 &level);
        if (ret != NvCtrlSuccess) {
            /* cooler information no longer available */
            return FALSE;
        }
        tmp_str = g_strdup_printf("%d", level);
        label = gtk_label_new(tmp_str);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, row_idx, row_idx+1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        free(tmp_str);

        ret = NvCtrlGetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_CONTROL_TYPE,
                                 &cooler_type);
        if (ret != NvCtrlSuccess) {
            /* cooler information no longer available */
            return FALSE;
        }
        if (cooler_type == NV_CTRL_THERMAL_COOLER_CONTROL_TYPE_VARIABLE) {
            tmp_str = g_strdup_printf("Variable");
        } else if (cooler_type == NV_CTRL_THERMAL_COOLER_CONTROL_TYPE_TOGGLE) {
            tmp_str = g_strdup_printf("Toggle");
        } else if (cooler_type == NV_CTRL_THERMAL_COOLER_CONTROL_TYPE_NONE) {
            tmp_str = g_strdup_printf("Restricted");
        }
        label = gtk_label_new(tmp_str);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 3, 4, row_idx, row_idx+1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        free(tmp_str);

        ret = NvCtrlGetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_TARGET,
                                 &cooler_target);
        if (ret != NvCtrlSuccess) {
            /* cooler information no longer available */
            return FALSE;
        }
        switch(cooler_target) {
            case NV_CTRL_THERMAL_COOLER_TARGET_GPU: 
                tmp_str = g_strdup_printf("GPU");
                break;
            case NV_CTRL_THERMAL_COOLER_TARGET_MEMORY:      
                tmp_str = g_strdup_printf("Memory");
                break;
            case NV_CTRL_THERMAL_COOLER_TARGET_POWER_SUPPLY:             
                tmp_str = g_strdup_printf("Power Supply");
                break;    
            case NV_CTRL_THERMAL_COOLER_TARGET_GPU_RELATED:  
                tmp_str = g_strdup_printf("GPU, Memory, and Power Supply");
                break;
            default:
                break;
        }
        label = gtk_label_new(tmp_str);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 4, 5, row_idx, row_idx+1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        free(tmp_str);
    }
    gtk_widget_show_all(table);
     
    /* X driver takes fraction of second to refresh newly set value */

    if (!ctk_thermal->cooler_control_enabled) {
        sync_gui_to_modify_cooler_level(ctk_thermal);
    }

    return TRUE;
} /* update_cooler_info() */



static gboolean update_thermal_info(gpointer user_data)
{
    gint reading, ambient;
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);
    gint ret, i, core;
    gchar *s;

    if (!ctk_thermal->thermal_sensor_target_type_supported) {
        CtrlTarget *ctrl_target = ctk_thermal->ctrl_target;

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_CORE_TEMPERATURE,
                                 &core);
        if (ret != NvCtrlSuccess) {
            /* thermal information no longer available */
            return FALSE;
        }

        s = g_strdup_printf(" %d C ", core);
        gtk_label_set_text(GTK_LABEL(ctk_thermal->core_label), s);
        g_free(s);

        ctk_gauge_set_current(CTK_GAUGE(ctk_thermal->core_gauge), core);
        ctk_gauge_draw(CTK_GAUGE(ctk_thermal->core_gauge));

        if (ctk_thermal->ambient_label) {
            ret = NvCtrlGetAttribute(ctrl_target,
                                     NV_CTRL_AMBIENT_TEMPERATURE,
                                     &ambient);
            if (ret != NvCtrlSuccess) {
                /* thermal information no longer available */
                return FALSE;
            }
            s = g_strdup_printf(" %d C ", ambient);
            gtk_label_set_text(GTK_LABEL(ctk_thermal->ambient_label), s);
            g_free(s);
        }
    } else {
        for (i = 0; i < ctk_thermal->sensor_count; i++) {
            CtrlTarget *ctrl_target = ctk_thermal->sensor_info[i].ctrl_target;

            ret = NvCtrlGetAttribute(ctrl_target,
                                     NV_CTRL_THERMAL_SENSOR_READING,
                                     &reading);
            /* querying THERMAL_SENSOR_READING failed: assume the temperature is 0 */
            if (ret != NvCtrlSuccess) {
                reading = 0;
            }
            
            if (ctk_thermal->sensor_info[i].temp_label) {
                s = g_strdup_printf(" %d C ", reading);
                gtk_label_set_text(
                                   GTK_LABEL(ctk_thermal->sensor_info[i].temp_label), s);
                g_free(s);
            }
            
            if (ctk_thermal->sensor_info[i].core_gauge) {
                ctk_gauge_set_current(
                          CTK_GAUGE(ctk_thermal->sensor_info[i].core_gauge),
                          reading);
                ctk_gauge_draw(CTK_GAUGE(ctk_thermal->sensor_info[i].core_gauge));
            }
        }
    }
    if ( ctk_thermal->cooler_count ) {
        update_cooler_info(ctk_thermal);
    }
    
    return TRUE;
} /* update_thermal_info() */



/****
 *
 * Updates widgets in relation to current cooler control state.
 *
 */
static void cooler_control_state_update_gui(CtkThermal *ctk_thermal)
{
    CtrlTarget *ctrl_target = ctk_thermal->ctrl_target;
    ReturnStatus ret;
    int value;
    gboolean enabled;


    /* We need to check the cooler control state status with 
     * the server every time someone tries to change the state
     * because the set might have failed.
     */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                             &value);
    if (ret != NvCtrlSuccess) {
        enabled = FALSE;
    } else {
        enabled = (value==NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);
    }

    ctk_thermal->cooler_control_enabled = enabled;
    
    /* Sync the gui to be able to modify the fan speed */

    sync_gui_to_modify_cooler_level(ctk_thermal);

    /* Update the status bar */

    ctk_config_statusbar_message(ctk_thermal->ctk_config,
                                 "GPU Fan control %sabled.",
                                 enabled?"en":"dis");

} /* cooler_control_state_update_gui() */



/*****
 *
 * Signal handler - Called when the user toggles the "Enable Cooler control"
 * button.
 *
 */
static void cooler_control_state_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);
    CtrlTarget *ctrl_target = ctk_thermal->ctrl_target;
    gboolean enabled;
    int value;

    /* Get enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    value = (enabled==1) ? NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE :
        NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE;

    /* Update the server */

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                       value);

    /* Update the GUI */

    cooler_control_state_update_gui(ctk_thermal);

} /* cooler_control_state_toggled() */



/*****
 *
 * Signal handler - Called when another NV-CONTROL client has set the
 * cooler control state.
 *
 */
static void cooler_control_state_received(GObject *object,
                                          CtrlEvent *event,
                                          gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);

    /* Update GUI with enable status */

    cooler_control_state_update_gui(ctk_thermal);

} /* cooler_control_state_update_received() */



/****
 *
 * Updates sensitivity of widgets in relation to the state
 * of cooler control.
 *
 */
static void sync_gui_sensitivity(CtkThermal *ctk_thermal)
{
    gboolean enabled = ctk_thermal->cooler_control_enabled;
    gboolean settings_changed = ctk_thermal->settings_changed;
    gint i;

    if ( ctk_thermal->cooler_count && ctk_thermal->show_fan_control_frame ) {
        /* Update the enable checkbox */

        g_signal_handlers_block_by_func(G_OBJECT(ctk_thermal->enable_checkbox),
                                        G_CALLBACK(cooler_control_state_toggled),
                                        (gpointer) ctk_thermal);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_thermal->enable_checkbox),
                                     enabled);

        g_signal_handlers_unblock_by_func(G_OBJECT(ctk_thermal->enable_checkbox),
                                          G_CALLBACK(cooler_control_state_toggled),
                                          (gpointer) ctk_thermal);

        /* Update the cooler control widgets */

        for (i = 0; i < ctk_thermal->cooler_count; i++) {
            gtk_widget_set_sensitive(ctk_thermal->cooler_control[i].widget,
                                     enabled);
        }

        /* Update the Apply button */

        gtk_widget_set_sensitive(ctk_thermal->apply_button,
                                 enabled && settings_changed);

        /* Update the Reset button */

        gtk_widget_set_sensitive(ctk_thermal->reset_button,
                                 enabled && ctk_thermal->enable_reset_button);
    }
} /* sync_gui_sensitivity() */



/*****
 *
 * Signal handler - User clicked the "apply" button.
 *
 */
static void apply_button_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);

    ReturnStatus ret;
    gint cooler_level;
    gint i;


    /* Set cooler's level on server */
    
    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        if ( ctk_thermal->cooler_control[i].changed ) {
            if ( ctk_thermal->cooler_control[i].adjustment ) {
                cooler_level = gtk_adjustment_get_value(
                                    ctk_thermal->cooler_control[i].adjustment);
            } else { 
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
                                       ctk_thermal->cooler_control[i].widget))) {
                    cooler_level =
                        ctk_thermal->cooler_control[i].range.range.max;
                } else { 
                    cooler_level = 
                        ctk_thermal->cooler_control[i].range.range.min;
                }
            }

            ret =
                NvCtrlSetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                   NV_CTRL_THERMAL_COOLER_LEVEL,
                                   cooler_level);

            if ( ret != NvCtrlSuccess ) {
                ctk_config_statusbar_message(ctk_thermal->ctk_config,
                                             "Failed to set new Fan Speed!");
                return;
            }
            ctk_thermal->cooler_control[i].changed = FALSE;
        }
    }
    ctk_thermal->settings_changed = FALSE;
    ctk_thermal->enable_reset_button = TRUE;
    
    /* Sync up with the server current fan speed */
    
    sync_gui_to_modify_cooler_level(ctk_thermal);
    
    /* Update the gui sensitivity */
    
    sync_gui_sensitivity(ctk_thermal);

    /* Enable the reset button */

    gtk_widget_set_sensitive(ctk_thermal->reset_button, TRUE);
    
    ctk_config_statusbar_message(ctk_thermal->ctk_config,
                                 "Set new Fan Speed.");
} /* apply_button_clicked() */



/*****
 *
 * Signal handler - User clicked the 'reset hardware defaults' button.
 *
 */
static void reset_button_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);
    ReturnStatus ret;
    gboolean reset_failed = FALSE;
    gint i;

    /* Set cooler related values to default */

    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        ret = NvCtrlSetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT,
                                 NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT);
        if ( ret != NvCtrlSuccess ) {
            reset_failed = TRUE;
        }
    }
    ctk_thermal->enable_reset_button = FALSE;
    
    /* Update GUI to reflect current values */
    
    cooler_control_state_update_gui(ctk_thermal);

    /* Disable the apply button */

    gtk_widget_set_sensitive(ctk_thermal->apply_button, FALSE);

    /* Disable the reset button */

    gtk_widget_set_sensitive(ctk_thermal->reset_button,
                             ctk_thermal->enable_reset_button);

    /* Set statusbar messages */
    
    if ( reset_failed ) {
        ctk_config_statusbar_message(ctk_thermal->ctk_config,
                                     "Failed to reset fan speed default value!");
    } else {
        ctk_config_statusbar_message(ctk_thermal->ctk_config,
                                     "Reset to fan speed default value.");
    }
    return;

} /* reset_button_clicked() */



/*****
 *
 * Signal handler - Handles slider adjustments by the user.
 *
 */
static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);
    gint i;

    /* Set flag for cooler whose operating level value changed */
    
    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        if (ctk_thermal->cooler_control[i].adjustment ==
              adjustment) {
            ctk_thermal->cooler_control[i].changed = TRUE;
        }
    }
    ctk_thermal->settings_changed = TRUE;
    /* Enable the apply button */

    gtk_widget_set_sensitive(ctk_thermal->apply_button, TRUE);

    /* Disable the reset button */

    gtk_widget_set_sensitive(ctk_thermal->reset_button, FALSE);

} /* adjustment_value_changed() */



/*****
 *
 * Syncs the gui to properly display the correct cooler level the user wants to
 * modify, or has modified with another NV_CONTROL client.
 *
 */
static void sync_gui_to_modify_cooler_level(CtkThermal *ctk_thermal)
{
    GtkRange *gtk_range;
    GtkAdjustment *gtk_adjustment_fan;
    CtrlAttributeValidValues cooler_range;
    gboolean can_access_cooler_level = TRUE;
    ReturnStatus ret;
    gint val, i, enabled;
    gint cooler_level;


    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        /* Obtain the current value and range of the fan speed */
        
        ret = NvCtrlGetAttribute(ctk_thermal->cooler_control[i].ctrl_target,
                                 NV_CTRL_THERMAL_COOLER_LEVEL,
                                 &cooler_level);
        if ( ret != NvCtrlSuccess ) {
            can_access_cooler_level = FALSE;
        }
        ctk_thermal->cooler_control[i].level = cooler_level; 
        if ( can_access_cooler_level && ctk_thermal->show_fan_control_frame ) {
            /* Make cooler control slider reflect the right range/values */
            
            if ( ctk_thermal->cooler_control[i].adjustment ) {

                ret = NvCtrlGetValidAttributeValues
                    (ctk_thermal->cooler_control[i].ctrl_target,
                     NV_CTRL_THERMAL_COOLER_LEVEL,
                     &cooler_range);
                if ( ret != NvCtrlSuccess ) {
                    can_access_cooler_level = FALSE;
                }
                ctk_thermal->cooler_control[i].range = cooler_range;
                if ( can_access_cooler_level ) {
                    gtk_adjustment_fan = ctk_thermal->cooler_control[i].adjustment;
                    gtk_range = GTK_RANGE(CTK_SCALE(
                                ctk_thermal->cooler_control[i].widget)->
                                gtk_scale);

                    g_signal_handlers_block_by_func(G_OBJECT(gtk_adjustment_fan),
                                        G_CALLBACK(adjustment_value_changed),
                                        (gpointer) ctk_thermal);
                    gtk_range_set_range(gtk_range,
                      ctk_thermal->cooler_control[i].range.range.min,
                      ctk_thermal->cooler_control[i].range.range.max);

                    val = gtk_adjustment_get_value(gtk_adjustment_fan);
                    if (val != ctk_thermal->cooler_control[i].level) {
                        gtk_adjustment_set_value(gtk_adjustment_fan,
                                    ctk_thermal->cooler_control[i].level);
                    }

                    g_signal_handlers_unblock_by_func(G_OBJECT(
                                                  gtk_adjustment_fan),
                                                  G_CALLBACK(
                                                  adjustment_value_changed),
                                                  (gpointer) ctk_thermal);
                }
            } else {
                /* Make cooler control checkbox reflect the right value */

                g_signal_handlers_block_by_func(G_OBJECT(
                                         ctk_thermal->cooler_control[i].widget),
                                         G_CALLBACK(
                                         cooler_control_checkbox_toggled),
                                         (gpointer) ctk_thermal);

                enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
                                        ctk_thermal->cooler_control[i].widget));

                if (enabled && ctk_thermal->cooler_control[i].level) {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
                                        ctk_thermal->cooler_control[i].widget),
                                        !enabled); 
                } 

                g_signal_handlers_unblock_by_func(G_OBJECT(
                                    ctk_thermal->cooler_control[i].widget),
                                    G_CALLBACK(cooler_control_checkbox_toggled),
                                    (gpointer) ctk_thermal);
            }
        }
    }
    /* Update the gui sensitivity */

    sync_gui_sensitivity(ctk_thermal);

} /* sync_gui_to_modify_cooler_level() */



/*****
 *
 * Helper function - calls sync_gui_to_modify_cooler_level() 
 *
 */
static gboolean sync_gui_to_update_cooler_event(gpointer user_data)
{
    CtkThermal *ctk_thermal = (CtkThermal *) user_data;

    sync_gui_to_modify_cooler_level(ctk_thermal);

    return FALSE;

} /* sync_gui_to_update_cooler_event() */



/*****
 *
 * Callback function when another NV_CONTROL client changed cooler level.
 *
 */
static void cooler_operating_level_changed(GObject *object,
                                           CtrlEvent *event,
                                           gpointer user_data)
{
    /* sync_gui_to_modify_cooler_level() to be called once when all other
     * pending events are consumed.
     */

    g_idle_add(sync_gui_to_update_cooler_event, (gpointer) user_data);

} /* cooler_operating_level_changed() */



/*****
 *
 * Signal handler - Handles checkbox toggling by the user.
 *
 */
static void cooler_control_checkbox_toggled(GtkWidget *widget,
                                            gpointer user_data)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(user_data);
    gint i;

    /* Set bit for cooler whose value want to change */
    
    for (i = 0; i < ctk_thermal->cooler_count; i++) {
        if ((ctk_thermal->cooler_control[i].widget) == widget) {
            ctk_thermal->cooler_control[i].changed = TRUE;
            ctk_thermal->settings_changed = TRUE;
        }
    }
    
    /* Enable the apply button */

    gtk_widget_set_sensitive(ctk_thermal->apply_button, TRUE);

} /* cooler_control_checkbox_toggled() */

static const nvctrlFormatName targetFormatNames[] = {

    { NV_CTRL_THERMAL_SENSOR_TARGET_NONE,         "None"         },
    { NV_CTRL_THERMAL_SENSOR_TARGET_GPU,          "GPU"          },
    { NV_CTRL_THERMAL_SENSOR_TARGET_MEMORY,       "MEMORY"       },
    { NV_CTRL_THERMAL_SENSOR_TARGET_POWER_SUPPLY, "Power Supply" }, 
    { NV_CTRL_THERMAL_SENSOR_TARGET_BOARD,        "BOARD"        },
    { -1, NULL },
};

static const nvctrlFormatName providerFormatNames[] = {
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_NONE,         "None"         },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_GPU_INTERNAL, "GPU Internal" },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_ADM1032,      "ADM1032"      },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_ADT7461,      "ADT7461"      },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_MAX6649,      "MAX6649"      },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_MAX1617,      "MAX1617"      },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_LM99,         "LM99"         },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_LM89,         "LM89"         },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_LM64,         "LM64"         },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_G781,         "G781"         },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_ADT7473,      "ADT7473"      },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_SBMAX6649,    "SBMAX6649"    },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_VBIOSEVT,     "VBIOSEVT"     },
    { NV_CTRL_THERMAL_SENSOR_PROVIDER_OS,           "OS"           },
    { -1, NULL },
};


/*
 * get_nvctrl_format_name() - return the name of the nvcontrol format
 */
static const char *get_nvctrl_format_name(const nvctrlFormatName *nvctrlFormatNames,
                                          const gint format)
{
    gint i;

    for (i = 0; nvctrlFormatNames[i].name; i++) {
        if (nvctrlFormatNames[i].format == format) {
            return nvctrlFormatNames[i].name;
        }
    }

    return "Unknown";

} /* get_nvctrl_format_name() */


/*
 * pack_gauge() - pack gauge gui in hbox
 */
static GtkWidget *pack_gauge(GtkWidget *hbox, gint lower, gint upper,
                             CtkConfig *ctk_config, const char *help)
{
    GtkWidget *vbox, *frame, *eventbox, *gauge;
    
    /* GPU Core Temperature Gauge */

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    frame = gtk_frame_new("Temperature");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    gauge = ctk_gauge_new(lower, upper);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), gauge);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_config_set_tooltip(ctk_config, eventbox, help);
    
    return gauge;
} /* pack_gauge() */


/*****
 *
 * draw_sensor_gui() - prints sensor related information
 *
 */
static void draw_sensor_gui(GtkWidget *vbox1, CtkThermal *ctk_thermal,
                            gboolean new_target_type, gint cur_sensor_idx,
                            gint reading, gint lower, gint upper,
                            gint target, gint provider, gint slowdown)
{
    GtkWidget *hbox, *hbox1, *hbox2, *vbox, *vbox2, *table;
    GtkWidget *frame, *label, *hsep;
    GtkWidget *eventbox = NULL;
    gchar *s;

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

    hbox1 = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
    
    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), vbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    /* GPU sensor ID */

    hbox2 = gtk_hbox_new(FALSE, 0);
    s = g_strdup_printf("ID: %d", cur_sensor_idx);
    label = gtk_label_new(s);
    g_free(s);
    gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    table = gtk_table_new(4, 4, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5); 

    /* sensor target type */
    if (target) {
        add_table_row_with_help_text(table, ctk_thermal->ctk_config,
                                __thermal_sensor_target_help,
                                0, 0, 0, 0.5,
                                "Target:", 0, 0.5,
                                get_nvctrl_format_name(targetFormatNames, target));
        ctk_thermal->sensor_info[cur_sensor_idx].target_type = label;
    } else {
        ctk_thermal->sensor_info[cur_sensor_idx].target_type = NULL;
    }

    /* sensor provider type */
    if (provider) {
        add_table_row_with_help_text(table, ctk_thermal->ctk_config,
                                __thermal_sensor_provider_help,
                                1, 0, 0, 0.5,
                                "Provider:", 0, 0.5,
                                get_nvctrl_format_name(providerFormatNames, provider));
        ctk_thermal->sensor_info[cur_sensor_idx].provider_type = label;
    } else {
        ctk_thermal->sensor_info[cur_sensor_idx].provider_type = NULL;
    }
    
    /* Upper limit, slowdown threshold */
    if (slowdown > 0) {
        s = g_strdup_printf("%d\xc2\xb0" /* split for g_utf8_validate() */ "C",
                             slowdown);
        add_table_row_with_help_text(table, ctk_thermal->ctk_config,
                                __slowdown_threshold_help,
                                2, 0, 0, 0.5,
                                "Slowdown Temp:", 0, 0.5,
                                s);
        ctk_thermal->sensor_info[cur_sensor_idx].provider_type = label;
        g_free(s);
    }

    /* thermal sensor reading */
    if (reading) {
        hbox2 = gtk_hbox_new(FALSE, 0);
        gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 3, 4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

        label = gtk_label_new("Temperature:");
        gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

        frame = gtk_frame_new(NULL);
        eventbox = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(eventbox), frame);
        gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 3, 4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

        label = gtk_label_new(NULL);
        gtk_container_add(GTK_CONTAINER(frame), label);
        ctk_thermal->sensor_info[cur_sensor_idx].temp_label = label;
        ctk_config_set_tooltip(ctk_thermal->ctk_config, eventbox,
                               __thermal_sensor_reading_help);
    } else {
        ctk_thermal->sensor_info[cur_sensor_idx].temp_label = NULL;
    }

    /* GPU Core Temperature Gauge */
    ctk_thermal->sensor_info[cur_sensor_idx].core_gauge =
        pack_gauge(hbox, lower, upper,
                   ctk_thermal->ctk_config, __temp_level_help);
     
    /* add horizontal bar between sensors */
    if (cur_sensor_idx+1 != ctk_thermal->sensor_count) {
        hbox1 = gtk_hbox_new(FALSE, 0);
        hsep = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(vbox2), hbox1, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox1), hsep, TRUE, TRUE, 10);
    }

} /* draw_sensor_gui() */



GtkWidget* ctk_thermal_new(CtrlTarget *ctrl_target,
                           CtkConfig *ctk_config,
                           CtkEvent *ctk_event)
{
    GObject *object;
    CtkThermal *ctk_thermal;
    CtrlSystem *system;
    GtkAdjustment *adjustment;
    GtkWidget *hbox = NULL, *hbox1, *hbox2, *table, *vbox;
    GtkWidget *frame, *banner, *label;
    GtkWidget *vbox1;
    GtkWidget *eventbox = NULL, *entry;
    GtkWidget *fan_control_frame;
    GtkWidget *hsep;
    GtkWidget *scale;
    GtkWidget *alignment;
    ReturnStatus ret;
    ReturnStatus ret1;
    CtrlTarget *cooler_target;
    CtrlTarget *sensor_target;
    CtrlAttributeValidValues cooler_range;
    CtrlAttributeValidValues sensor_range;
    gint trigger, core, ambient;
    gint upper;
    gchar *s;
    gint i, j;
    gint cooler_level;
    gint cooler_control_type;
    gchar *name = NULL;
    int *pDataCooler = NULL, *pDataSensor = NULL;
    gint cooler_count = 0, sensor_count = 0;
    int len, value;
    int major = 0, minor = 0;
    Bool can_access_cooler_level;
    Bool cooler_control_enabled;
    int cur_cooler_idx = 0;
    int cur_sensor_idx = 0;
    int slowdown;
    Bool thermal_sensor_target_type_supported = FALSE;

    /* make sure we have a handle */

    g_return_val_if_fail((ctrl_target != NULL) &&
                         (ctrl_target->h != NULL), NULL);

    /* 
     * Check for NV-CONTROL protocol version. 
     * In version 1.23 we added support for querying per sensor information
     * This used for backward compatibility between new nvidia-settings
     * and older X driver
     */ 
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_ATTR_NV_MAJOR_VERSION, &major);
    ret1 = NvCtrlGetAttribute(ctrl_target,
                              NV_CTRL_ATTR_NV_MINOR_VERSION, &minor);

    if ((ret == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        ((major > 1) || ((major == 1) && (minor > 22)))) {
        thermal_sensor_target_type_supported = TRUE;
    }

    if (!thermal_sensor_target_type_supported) {
        /* check if this screen supports thermal querying */

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_CORE_TEMPERATURE,
                                 &core);
        if (ret != NvCtrlSuccess) {
            /* thermal information unavailable */
            return NULL;
        }

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_MAX_CORE_THRESHOLD,
                                 &upper);
        if (ret != NvCtrlSuccess) {
            /* thermal information unavailable */
            return NULL;
        }

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_CORE_THRESHOLD,
                                 &trigger);
        if (ret != NvCtrlSuccess) {
            /* thermal information unavailable */
            return NULL;
        }
    }
    /* Query the list of sensors attached to this GPU */

    ret = NvCtrlGetBinaryAttribute(ctrl_target, 0,
                                   NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU,
                                   (unsigned char **)(&pDataSensor), &len);
    if ( ret == NvCtrlSuccess ) {
        sensor_count = pDataSensor[0];
    }

    /* Query the list of coolers attached to this GPU */

    ret = NvCtrlGetBinaryAttribute(ctrl_target, 0,
                                   NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU,
                                   (unsigned char **)(&pDataCooler), &len);
    if ( ret == NvCtrlSuccess ) {
        cooler_count = pDataCooler[0];
    }
    
    /* return if sensor and Fan information not available */
    if ((thermal_sensor_target_type_supported && !sensor_count) &&
        !cooler_count) {
        free(pDataSensor);
        free(pDataCooler);
        return NULL;
    }

    /* create the CtkThermal object */

    object = g_object_new(CTK_TYPE_THERMAL, NULL);

    ctk_thermal = CTK_THERMAL(object);
    ctk_thermal->ctrl_target = ctrl_target;
    ctk_thermal->ctk_config = ctk_config;
    ctk_thermal->settings_changed = FALSE;
    ctk_thermal->show_fan_control_frame = TRUE;
    ctk_thermal->cooler_count = cooler_count;
    ctk_thermal->sensor_count = sensor_count;
    ctk_thermal->thermal_sensor_target_type_supported = thermal_sensor_target_type_supported;

    /* set container properties for the CtkThermal widget */

    gtk_box_set_spacing(GTK_BOX(ctk_thermal), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_THERMAL);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /* Check if we can control cooler state */

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_COOLER_MANUAL_CONTROL,
                             &value);
    if ( ret != NvCtrlSuccess ) {
        ctk_thermal->show_fan_control_frame = FALSE;
        value = NV_CTRL_GPU_COOLER_MANUAL_CONTROL_FALSE;
    }

    cooler_control_enabled = (value == NV_CTRL_GPU_COOLER_MANUAL_CONTROL_TRUE);

    can_access_cooler_level = TRUE;
    ctk_thermal->cooler_control_enabled = cooler_control_enabled;
    ctk_thermal->enable_reset_button = FALSE;

    /* Retrieve CtrlSystem from ctk_config */

    system = ctk_config->pCtrlSystem;

    /* Thermal Information */

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);
    
    if (thermal_sensor_target_type_supported) {
        
        if ( ctk_thermal->sensor_count == 0 ) {
            goto sensor_end;
        }
        hbox1 = gtk_hbox_new(FALSE, FRAME_PADDING);
        gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
        label = gtk_label_new("Thermal Sensor Information");
        gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);

        hsep = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox1), hsep, TRUE, TRUE, 0);
        
        ret = NvCtrlGetAttribute(ctk_thermal->ctrl_target,
                                 NV_CTRL_GPU_SLOWDOWN_THRESHOLD,
                                 &slowdown);
        if (ret != NvCtrlSuccess) {
            slowdown = 0;
        }

        if (ctk_thermal->sensor_count > 0) {
            ctk_thermal->sensor_info = (SensorInfoPtr)
                nvalloc(ctk_thermal->sensor_count * sizeof(SensorInfoRec));
        }

        for (j = 1; j <= ctk_thermal->sensor_count; j++) {
            gint reading, target, provider;

            sensor_target = NvCtrlGetTarget(system, THERMAL_SENSOR_TARGET,
                                            pDataSensor[j]);
            if (sensor_target == NULL) {
                continue;
            }

            ctk_thermal->sensor_info[cur_sensor_idx].ctrl_target =
                sensor_target;

            /* check if this screen supports thermal querying */

            ret = NvCtrlGetAttribute(sensor_target, NV_CTRL_THERMAL_SENSOR_READING,
                                     &reading);
            if (ret != NvCtrlSuccess) {
                /* sensor information unavailable */
                reading = 0;
            }

            ret = NvCtrlGetValidAttributeValues(sensor_target,
                                                NV_CTRL_THERMAL_SENSOR_READING,
                                                &sensor_range);
            if (ret != NvCtrlSuccess) {
                /* sensor information unavailable */
                sensor_range.range.min = sensor_range.range.max = 0;
            }

            ret = NvCtrlGetAttribute(sensor_target,
                                     NV_CTRL_THERMAL_SENSOR_TARGET,
                                     &target);
            if (ret != NvCtrlSuccess) {
                /* sensor information unavailable */
                target = 0;
            }

            ret = NvCtrlGetAttribute(sensor_target, NV_CTRL_THERMAL_SENSOR_PROVIDER,
                                     &provider);
            if (ret != NvCtrlSuccess) {
                /* sensor information unavailable */
                provider = 0;
            }
            /* print sensor related information */
            draw_sensor_gui(vbox, ctk_thermal, thermal_sensor_target_type_supported,
                            cur_sensor_idx,
                            reading, sensor_range.range.min,
                            sensor_range.range.max, target, provider, slowdown);
            cur_sensor_idx++;
        }
    } else {
        /* GPU Core Threshold Temperature */

        vbox1 = gtk_vbox_new(FALSE, 0);
        hbox1 = gtk_hbox_new(FALSE, 0);
        frame = gtk_frame_new("Slowdown Threshold");
        gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox1), vbox1, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox1), frame, FALSE, FALSE, 0);

        hbox2 = gtk_hbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(hbox2), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), hbox2);

        label = gtk_label_new("Degrees: ");
        gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

        eventbox = gtk_event_box_new();
        gtk_box_pack_start(GTK_BOX(hbox2), eventbox, FALSE, FALSE, 0);

        entry = gtk_entry_new();
        gtk_entry_set_max_length(GTK_ENTRY(entry), 5);
        gtk_container_add(GTK_CONTAINER(eventbox), entry);
        gtk_widget_set_sensitive(entry, FALSE);
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 5);

        s = g_strdup_printf(" %d ", trigger);
        gtk_entry_set_text(GTK_ENTRY(entry), s);
        g_free(s);
        ctk_config_set_tooltip(ctk_config, eventbox, __core_threshold_help);

        label = gtk_label_new(" C");
        gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

        /* GPU Core Temperature */

        table = gtk_table_new(2, 2, FALSE);
        gtk_box_pack_end(GTK_BOX(vbox1), table, FALSE, FALSE, 0);

        hbox2 = gtk_hbox_new(FALSE, 0);
        gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        label = gtk_label_new("Core Temperature:");
        gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

        frame = gtk_frame_new(NULL);
        eventbox = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(eventbox), frame);
        gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

        label = gtk_label_new(NULL);
        gtk_container_add(GTK_CONTAINER(frame), label);
        ctk_thermal->core_label = label;

        ctk_config_set_tooltip(ctk_config, eventbox, __core_temp_help);

        /* Ambient Temperature */

        ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_AMBIENT_TEMPERATURE,
                                 &ambient);

        if (ret == NvCtrlSuccess) {
            hbox2 = gtk_hbox_new(FALSE, 0);
            gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            label = gtk_label_new("Ambient Temperature:");
            gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

            frame = gtk_frame_new(NULL);
            eventbox = gtk_event_box_new();
            gtk_container_add(GTK_CONTAINER(eventbox), frame);
            gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);

            label = gtk_label_new(NULL);
            gtk_container_add(GTK_CONTAINER(frame), label);
            ctk_thermal->ambient_label = label;

            ctk_config_set_tooltip(ctk_config, eventbox, __ambient_temp_help);
        } else {
            ctk_thermal->ambient_label = NULL;
        }

        /* GPU Core Temperature Gauge */
        
        ctk_thermal->core_gauge = pack_gauge(hbox1, 25, upper,
                                             ctk_config, __temp_level_help);
    }
sensor_end:
    
    /* Check for if Fans present on GPU */

    if ( ctk_thermal->cooler_count == 0 ) {
        goto end;
    }

    /* Fan Information Title */
    
    vbox = gtk_vbox_new(FALSE, 5);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Fan Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);
    
    ctk_thermal->fan_information_box = vbox;
    
    /* Fan Information Table */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    ctk_thermal->cooler_table_hbox = hbox;

    /* Create cooler level control sliders/checkbox */
    
    ctk_thermal->cooler_control = (CoolerControlPtr)
        calloc(ctk_thermal->cooler_count, sizeof(CoolerControlRec));

    for (j = 1; j <= ctk_thermal->cooler_count; j++) {

        cooler_target = NvCtrlGetTarget(system, COOLER_TARGET, pDataCooler[j]);
        if (cooler_target == NULL) {
            continue;
        }

        /* Get current cooler level and range */

        ret = NvCtrlGetAttribute(cooler_target,
                                 NV_CTRL_THERMAL_COOLER_LEVEL,
                                 &cooler_level);
        if ( ret != NvCtrlSuccess ) {
            can_access_cooler_level = FALSE;
        }

        ret = NvCtrlGetValidAttributeValues(cooler_target,
                                            NV_CTRL_THERMAL_COOLER_LEVEL,
                                            &cooler_range);
        if ( ret != NvCtrlSuccess ) {
            can_access_cooler_level = FALSE;
        }

        ctk_thermal->cooler_control[cur_cooler_idx].level = cooler_level; 
        ctk_thermal->cooler_control[cur_cooler_idx].range = cooler_range;
        ctk_thermal->cooler_control[cur_cooler_idx].ctrl_target = cooler_target;

        /* Create the object for receiving NV-CONTROL events */

        ctk_thermal->cooler_control[cur_cooler_idx].event =
            CTK_EVENT(ctk_event_new(cooler_target));

        if ( can_access_cooler_level && ctk_thermal->show_fan_control_frame ) { 
            /* 
             * Get NV_CTRL_THERMAL_COOLER_CONTROL_TYPE to decide cooler
             * control widget should be slider or checkbox.
             */

            ret = NvCtrlGetAttribute(cooler_target,
                                     NV_CTRL_THERMAL_COOLER_CONTROL_TYPE,
                                     &cooler_control_type);
            if ((ret == NvCtrlSuccess) &&
                (cooler_control_type ==
                 NV_CTRL_THERMAL_COOLER_CONTROL_TYPE_VARIABLE)) {

                adjustment =
                    GTK_ADJUSTMENT(gtk_adjustment_new(cooler_level,
                                                      cooler_range.range.min,
                                                      cooler_range.range.max,
                                                      1, 5, 0.0));
                name = g_strdup_printf("Fan %d Speed", cur_cooler_idx);
                scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), name,
                                      ctk_config, G_TYPE_INT);
                ctk_thermal->cooler_control[cur_cooler_idx].widget   = scale;
                ctk_thermal->cooler_control[cur_cooler_idx].adjustment =
                    GTK_ADJUSTMENT(adjustment);

                g_signal_connect(adjustment, "value_changed",
                                 G_CALLBACK(adjustment_value_changed),
                                 (gpointer) ctk_thermal);
            } else if ((ret == NvCtrlSuccess) && 
                       (cooler_control_type ==
                        NV_CTRL_THERMAL_COOLER_CONTROL_TYPE_TOGGLE)) {
                name = g_strdup_printf("Fan-%d Speed", cur_cooler_idx);

                ctk_thermal->cooler_control[cur_cooler_idx].widget = 
                    gtk_check_button_new_with_label(name);

                g_signal_connect(G_OBJECT(ctk_thermal->cooler_control
                                  [cur_cooler_idx].widget),
                                 "toggled",
                                 G_CALLBACK(cooler_control_checkbox_toggled),
                                 (gpointer) ctk_thermal);

                ctk_thermal->cooler_control[cur_cooler_idx].adjustment = NULL;
            }
            free(name);
            gtk_widget_set_sensitive(ctk_thermal->cooler_control
                                     [cur_cooler_idx].widget,
                                     cooler_control_enabled);
        }
        cur_cooler_idx++;
    }
    
    if ( ctk_thermal->cooler_count && ctk_thermal->show_fan_control_frame ) {
        /* Create the Enable Cooler control checkbox widget */

        ctk_thermal->enable_checkbox =
            gtk_check_button_new_with_label("Enable GPU Fan Settings");

        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_thermal->enable_checkbox),
             cooler_control_enabled);

        g_signal_connect(G_OBJECT(ctk_thermal->enable_checkbox), "toggled",
                         G_CALLBACK(cooler_control_state_toggled),
                         (gpointer) ctk_thermal);

        ctk_config_set_tooltip(ctk_config, ctk_thermal->enable_checkbox,
                               __enable_button_help);

        /* Create the Apply button widget */

        ctk_thermal->apply_button =
            gtk_button_new_with_label("Apply");

        g_signal_connect(G_OBJECT(ctk_thermal->apply_button), "clicked",
                         G_CALLBACK(apply_button_clicked),
                         (gpointer) ctk_thermal);

        ctk_config_set_tooltip(ctk_config, ctk_thermal->apply_button,
                               __apply_button_help);

        gtk_widget_set_sensitive(ctk_thermal->apply_button, FALSE);

        /* Create the Reset hardware button widget */

        ctk_thermal->reset_button =
            gtk_button_new_with_label("Reset Hardware Defaults");

        g_signal_connect(G_OBJECT(ctk_thermal->reset_button), "clicked",
                         G_CALLBACK(reset_button_clicked),
                         (gpointer) ctk_thermal);

        ctk_config_set_tooltip(ctk_config, ctk_thermal->reset_button,
                               __reset_button_help);

        gtk_widget_set_sensitive(ctk_thermal->reset_button, FALSE);

        /* Add Cooler Control frame */

        hbox = gtk_hbox_new(FALSE, 5);

        fan_control_frame = gtk_frame_new(NULL);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);
        gtk_box_pack_start(GTK_BOX(vbox), fan_control_frame, FALSE, FALSE, 5);
        vbox = gtk_vbox_new(FALSE, 0);
        vbox1 = gtk_vbox_new(FALSE, 0);

        gtk_container_set_border_width(GTK_CONTAINER(vbox1), 5);

        gtk_container_add(GTK_CONTAINER(fan_control_frame), vbox);
        gtk_box_pack_start(GTK_BOX(vbox), ctk_thermal->enable_checkbox,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 0);

        for (i = 0; i < ctk_thermal->cooler_count; i++) {
            if (ctk_thermal->cooler_control[i].widget) {
                gtk_box_pack_start(GTK_BOX(vbox1),
                                   ctk_thermal->cooler_control[i].widget,
                                   FALSE, FALSE, 5);
            }
        }

        /* Add the Apply and Reset buttons */

        hbox = gtk_hbox_new(FALSE, 0);

        gtk_box_pack_start(GTK_BOX(hbox), ctk_thermal->apply_button,
                           FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_thermal->reset_button,
                           FALSE, FALSE, 0);

        alignment = gtk_alignment_new(1, 1, 0, 0);
        gtk_container_add(GTK_CONTAINER(alignment), hbox);
        gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 5);

        for (i = 0; i < ctk_thermal->cooler_count; i++) {
            g_signal_connect(G_OBJECT(ctk_thermal->cooler_control[i].event),
                             CTK_EVENT_NAME(NV_CTRL_THERMAL_COOLER_LEVEL),
                             G_CALLBACK(cooler_operating_level_changed),
                             (gpointer) ctk_thermal);
        }
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_GPU_COOLER_MANUAL_CONTROL),
                         G_CALLBACK(cooler_control_state_received),
                         (gpointer) ctk_thermal);
    }

end:
    free(pDataSensor);
    pDataSensor = NULL;

    free(pDataCooler);
    pDataCooler = NULL;

    /* sync GUI to current server settings */
    
    sync_gui_to_modify_cooler_level(ctk_thermal);
    update_thermal_info(ctk_thermal);
    
    /* Register a timer callback to update the temperatures */

    s = g_strdup_printf("Thermal Monitor (GPU %d)",
                        NvCtrlGetTargetId(ctrl_target));

    ctk_config_add_timer(ctk_thermal->ctk_config,
                         DEFAULT_UPDATE_THERMAL_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_thermal_info,
                         (gpointer) ctk_thermal);
    g_free(s);
    
    gtk_widget_show_all(GTK_WIDGET(ctk_thermal));
    
    return GTK_WIDGET(ctk_thermal);
}

GtkTextBuffer *ctk_thermal_create_help(GtkTextTagTable *table,
                                       CtkThermal *ctk_thermal)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Thermal Settings Help");

    /* if sensor not available skip online help */
    if (!ctk_thermal->sensor_count) {
        goto next_help;
    }

    if (!ctk_thermal->thermal_sensor_target_type_supported) {
        ctk_help_heading(b, &i, "Slowdown Threshold");
        ctk_help_para(b, &i, "%s", __core_threshold_help);

        ctk_help_heading(b, &i, "Core Temperature");
        ctk_help_para(b, &i, "%s", __core_temp_help);

        if (ctk_thermal->ambient_label) {
            ctk_help_heading(b, &i, "Ambient Temperature");
            ctk_help_para(b, &i, "%s", __ambient_temp_help);
        }
    } else {
        ctk_help_title(b, &i, "Thermal Sensor Information Help");

        ctk_help_heading(b, &i, "ID");
        ctk_help_para(b, &i, "%s", __thermal_sensor_id_help);

        ctk_help_heading(b, &i, "Temperature");
        ctk_help_para(b, &i, "%s", __thermal_sensor_reading_help);

        ctk_help_heading(b, &i, "Target");
        ctk_help_para(b, &i, "%s", __thermal_sensor_target_help);
        
        ctk_help_heading(b, &i, "Provider");
        ctk_help_para(b, &i, "%s", __thermal_sensor_provider_help);
    }
    ctk_help_heading(b, &i, "Level");
    ctk_help_para(b, &i, "%s", __temp_level_help);

next_help:
    /* if Fan not available skip online help */
    if (!ctk_thermal->cooler_count) {
        goto done;
    }

    ctk_help_title(b, &i, "GPU Fan Settings Help");

    ctk_help_heading(b, &i, "ID");
    ctk_help_para(b, &i, "%s", __fan_id_help);

    ctk_help_heading(b, &i, "Speed (RPM)");
    ctk_help_para(b, &i,"%s", __fan_rpm_help);

    ctk_help_heading(b, &i, "Speed (%%)");
    ctk_help_para(b, &i, "%s", __fan_speed_help);

    ctk_help_heading(b, &i, "Type");
    ctk_help_para(b, &i, "%s", __fan_control_type_help);

    ctk_help_heading(b, &i, "Cooling Target");
    ctk_help_para(b, &i, "%s", __fan_cooling_target_help);

    ctk_help_heading(b, &i, "Enable GPU Fan Settings");
    ctk_help_para(b, &i, "%s", __enable_button_help);

    if ( ctk_thermal->show_fan_control_frame ) {
        ctk_help_heading(b, &i, "Enable GPU Fan Settings");
        ctk_help_para(b, &i, "%s", __apply_button_help);

        ctk_help_heading(b, &i, "Enable GPU Fan Settings");
        ctk_help_para(b, &i, "%s", __reset_button_help);
    }
done:
    ctk_help_finish(b);

    return b;
}

void ctk_thermal_start_timer(GtkWidget *widget)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(widget);

    /* Start the thermal timer */

    ctk_config_start_timer(ctk_thermal->ctk_config,
                           (GSourceFunc) update_thermal_info,
                           (gpointer) ctk_thermal);
}

void ctk_thermal_stop_timer(GtkWidget *widget)
{
    CtkThermal *ctk_thermal = CTK_THERMAL(widget);

    /* Stop the thermal timer */

    ctk_config_stop_timer(ctk_thermal->ctk_config,
                          (GSourceFunc) update_thermal_info,
                          (gpointer) ctk_thermal);
}
