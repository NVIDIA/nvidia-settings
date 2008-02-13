/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
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

#include "ctkhelp.h"
#include "ctkthermal.h"
#include "ctkgauge.h"
#include "thermal_banner.h"
#include "ctkimage.h"

#define FRAME_PADDING 10
#define DEFAULT_UPDATE_THERMAL_INFO_TIME_INTERVAL 1000

static gboolean update_thermal_info(gpointer);

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
        };

        ctk_thermal_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkThermal",
                                   &ctk_thermal_info, 0);
    }

    return ctk_thermal_type;

} /* ctk_thermal_get_type() */

static gboolean update_thermal_info(gpointer user_data)
{
    gint core, ambient;
    CtkThermal *ctk_thermal;
    NvCtrlAttributeHandle *handle;
    gint ret; gchar *s;

    ctk_thermal = CTK_THERMAL(user_data);
    handle = ctk_thermal->attribute_handle;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CORE_TEMPERATURE, &core);
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
        ret = NvCtrlGetAttribute(handle, NV_CTRL_AMBIENT_TEMPERATURE, &ambient);
        if (ret != NvCtrlSuccess) {
            /* thermal information no longer available */
            return FALSE;
        }
        s = g_strdup_printf(" %d C ", ambient);
        gtk_label_set_text(GTK_LABEL(ctk_thermal->ambient_label), s);
        g_free(s);
    }

    return TRUE;
}

GtkWidget* ctk_thermal_new(NvCtrlAttributeHandle *handle,
                           CtkConfig *ctk_config)
{
    GObject *object;
    CtkThermal *ctk_thermal;
    GtkWidget *gauge;
    GtkWidget *hbox, *hbox2, *vbox, *table;
    GtkWidget *frame, *banner, *label;
    GtkWidget *eventbox, *entry;
    ReturnStatus ret;
    
    gint trigger, ambient;
    gint core, upper;
    gchar *s;


    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);
    
    /* check if this screen supports thermal querying */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CORE_TEMPERATURE, &core);
    if (ret != NvCtrlSuccess) {
        /* thermal information unavailable */
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_MAX_CORE_THRESHOLD, &upper);
    if (ret != NvCtrlSuccess) {
        /* thermal information unavailable */
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CORE_THRESHOLD, &trigger);
    if (ret != NvCtrlSuccess) {
        /* thermal information unavailable */
        return NULL;
    }

    /* create the CtkThermal object */

    object = g_object_new(CTK_TYPE_THERMAL, NULL);
    
    ctk_thermal = CTK_THERMAL(object);
    ctk_thermal->attribute_handle = handle;
    ctk_thermal->ctk_config = ctk_config;

    /* set container properties for the CtkThermal widget */

    gtk_box_set_spacing(GTK_BOX(ctk_thermal), 10);

    /* banner */

    banner = ctk_banner_image_new(&thermal_banner_image);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);


    /* Thermal Information */

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);


    /* GPU Core Treshold Temperature */

    frame = gtk_frame_new("Slowdown Threshold");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox2), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), hbox2);

    label = gtk_label_new("Degrees: ");
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_box_pack_start(GTK_BOX(hbox2), eventbox, FALSE, FALSE, 0);

    entry = gtk_entry_new_with_max_length(5);
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
    gtk_box_pack_end(GTK_BOX(vbox), table, FALSE, FALSE, 0);

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

    ret = NvCtrlGetAttribute(handle, NV_CTRL_AMBIENT_TEMPERATURE, &ambient);

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

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    frame = gtk_frame_new("Level");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    gauge = ctk_gauge_new(25, upper);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), gauge);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    ctk_thermal->core_gauge = gauge;

    ctk_config_set_tooltip(ctk_config, eventbox, __temp_level_help);

    /* Register a timer callback to update the temperatures */

    s = g_strdup_printf("Thermal Monitor (GPU %d)",
                        NvCtrlGetTargetId(handle));
    
    ctk_config_add_timer(ctk_thermal->ctk_config,
                         DEFAULT_UPDATE_THERMAL_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_thermal_info,
                         (gpointer) ctk_thermal);
    g_free(s);

    update_thermal_info(ctk_thermal);
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

    ctk_help_title(b, &i, "Thermal Monitor Help");
    
    ctk_help_heading(b, &i, "Slowdown Threshold");
    ctk_help_para(b, &i, __core_threshold_help);

    ctk_help_heading(b, &i, "Core Temperature");
    ctk_help_para(b, &i, __core_temp_help);

    if (ctk_thermal->ambient_label) {
        ctk_help_heading(b, &i, "Ambient Temperature");
        ctk_help_para(b, &i, __ambient_temp_help);
    }

    ctk_help_heading(b, &i, "Temperature Level");
    ctk_help_para(b, &i, __temp_level_help);

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
