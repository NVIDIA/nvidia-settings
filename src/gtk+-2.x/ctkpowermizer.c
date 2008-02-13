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
#include "ctkpowermizer.h"
#include "ctkimage.h"

#define FRAME_PADDING 10
#define DEFAULT_UPDATE_POWERMIZER_INFO_TIME_INTERVAL 1000

static gboolean update_powermizer_info(gpointer);

static const char *__adaptive_clock_help =
"The Adaptive Clocking status describes if this feature "
"is currently enabled in this GPU.";

static const char *__power_source_help =
"The Power Source indicates whether the machine "
"is running on AC or Battery power.";

static const char *__performance_level_help =
"This indicates the current Performance Level of the GPU.";

static const char *__performance_mode_short_help =
"This indicates the current Performance Mode of the GPU.\n";

static const char *__performance_mode_help =
"This indicates the current Performance Mode of the GPU.\n"
"Performance Mode can be either \"Desktop\" or\n"
"\"Maximum Performance\".";

static const char *__gpu_clock_freq_help =
"This indicates the current GPU Clock frequency.";

static const char *__memory_clock_freq_help =
"This indicates the current Memory Clock frequency.";

static const char *__clock_freq_help =
"This indicates the current GPU Clock and Memory Clock frequencies.";

GType ctk_powermizer_get_type(void)
{
    static GType ctk_powermizer_type = 0;

    if (!ctk_powermizer_type) {
        static const GTypeInfo ctk_powermizer_info = {
            sizeof (CtkPowermizerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkPowermizer),
            0,    /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_powermizer_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkPowermizer",
                                   &ctk_powermizer_info, 0);
    }

    return ctk_powermizer_type;

} /* ctk_powermizer_get_type() */

static gboolean update_powermizer_info(gpointer user_data)
{
    gint power_source, perf_mode, adaptive_clock, perf_level;
    gint clockret, gpu_clock, memory_clock;

    CtkPowermizer *ctk_powermizer;
    NvCtrlAttributeHandle *handle;
    gint ret; gchar *s;

    ctk_powermizer = CTK_POWERMIZER(user_data);
    handle = ctk_powermizer->attribute_handle;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE, &adaptive_clock);
    if (ret != NvCtrlSuccess) { 
        return FALSE;
    }

    if (adaptive_clock == NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE_ENABLED) {
        s = g_strdup_printf("Enabled");
    }
    else if (adaptive_clock == NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE_DISABLED) {
        s = g_strdup_printf("Disabled");
    }
    else {
        s = g_strdup_printf("Error");
    }

    gtk_label_set_text(GTK_LABEL(ctk_powermizer->adaptive_clock_status), s);
    g_free(s);
 
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_CLOCK_FREQS, 
                             &clockret);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    memory_clock = clockret & 0x0000FFFF;
    gpu_clock = (clockret >> 16);
    
    s = g_strdup_printf("%d Mhz", gpu_clock);
    gtk_label_set_text(GTK_LABEL(ctk_powermizer->gpu_clock), s);
    g_free(s);

    s = g_strdup_printf("%d Mhz", memory_clock);
    gtk_label_set_text(GTK_LABEL(ctk_powermizer->memory_clock), s);
    g_free(s);

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_POWER_SOURCE, &power_source);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    if (power_source == NV_CTRL_GPU_POWER_SOURCE_AC) {
        s = g_strdup_printf("AC");
    }
    else if (power_source == NV_CTRL_GPU_POWER_SOURCE_BATTERY) {
        s = g_strdup_printf("Battery");
    }
    else {
        s = g_strdup_printf("Error");
    }

    gtk_label_set_text(GTK_LABEL(ctk_powermizer->power_source), s);
    g_free(s);

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, 
                             &perf_level);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    s = g_strdup_printf("%d", perf_level);
    gtk_label_set_text(GTK_LABEL(ctk_powermizer->performance_level), s);
    g_free(s);

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE, 
                             &perf_mode);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }
       
    if (perf_mode == NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE_DESKTOP) {
        s = g_strdup_printf("Desktop");
    }
    else if (perf_mode == NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE_MAXPERF) {
        s = g_strdup_printf("Maximum Performance");
    }
    else {
        s = g_strdup_printf("Default");
    }

    gtk_label_set_text(GTK_LABEL(ctk_powermizer->performance_mode), s);
    g_free(s);
    

    return TRUE;
}

GtkWidget* ctk_powermizer_new(NvCtrlAttributeHandle *handle,
                           CtkConfig *ctk_config)
{
    GObject *object;
    CtkPowermizer *ctk_powermizer;
    GtkWidget *hbox, *hbox2, *vbox, *hsep, *table;
    GtkWidget *banner, *label;
    GtkWidget *eventbox;
    ReturnStatus ret;
    gchar *s;    
    gint val;

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);
    
    /* check if this screen supports powermizer querying */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_POWER_SOURCE, &val);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, 
                             &val);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE, 
                             &val);
    if (ret != NvCtrlSuccess) {
        return NULL; 
    }
       

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE, 
                             &val);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_CLOCK_FREQS, 
                             &val);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    /* create the CtkPowermizer object */

    object = g_object_new(CTK_TYPE_POWERMIZER, NULL);
    
    ctk_powermizer = CTK_POWERMIZER(object);
    ctk_powermizer->attribute_handle = handle;
    ctk_powermizer->ctk_config = ctk_config;

    /* set container properties for the CtkPowermizer widget */

    gtk_box_set_spacing(GTK_BOX(ctk_powermizer), 7);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_THERMAL);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("PowerMizer Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

    table = gtk_table_new(15, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* Adaptive Clocking State */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 0, 1,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Adaptive Clocking:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 0, 1,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __adaptive_clock_help);
    ctk_powermizer->adaptive_clock_status = label;

    /* Clock Frequencies */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 4, 5,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("GPU Clock:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 4, 5,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __gpu_clock_freq_help);
    ctk_powermizer->gpu_clock = label;

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 5, 6,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Memory Clock:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 5, 6,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __memory_clock_freq_help);
    ctk_powermizer->memory_clock = label;

    /* Power Source */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 9, 10,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Power Source:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 9, 10,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __power_source_help);
    ctk_powermizer->power_source = label;


    /* Performance Level */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 13, 14,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Performance Level:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 13, 14,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __performance_level_help);
    ctk_powermizer->performance_level = label;

    /* Performance Mode */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 14, 15,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Performance Mode:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    eventbox = gtk_event_box_new();
    gtk_table_attach(GTK_TABLE(table), eventbox, 1, 2, 14, 15,
            GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(NULL);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    ctk_config_set_tooltip(ctk_config, eventbox, __performance_mode_short_help);
    ctk_powermizer->performance_mode = label;

    /* Register a timer callback to update the temperatures */

    s = g_strdup_printf("PowerMizer Monitor (GPU %d)",
                        NvCtrlGetTargetId(handle));
    
    ctk_config_add_timer(ctk_powermizer->ctk_config,
                         DEFAULT_UPDATE_POWERMIZER_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_powermizer_info,
                         (gpointer) ctk_powermizer);
    g_free(s);

    update_powermizer_info(ctk_powermizer);
    gtk_widget_show_all(GTK_WIDGET(ctk_powermizer));

    return GTK_WIDGET(ctk_powermizer);
}

GtkTextBuffer *ctk_powermizer_create_help(GtkTextTagTable *table,
                                       CtkPowermizer *ctk_powermizer)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "PowerMizer Monitor Help");
    
    ctk_help_heading(b, &i, "Adaptive Clocking");
    ctk_help_para(b, &i, __adaptive_clock_help);

    ctk_help_heading(b, &i, "Power Source");
    ctk_help_para(b, &i, __power_source_help);

    ctk_help_heading(b, &i, "Clock Frequencies");
    ctk_help_para(b, &i, __clock_freq_help);

    ctk_help_heading(b, &i, "Performance Level");
    ctk_help_para(b, &i, __performance_level_help);

    ctk_help_heading(b, &i, "Performance Mode");
    ctk_help_para(b, &i, __performance_mode_help);
    ctk_help_finish(b);

    return b;
}

void ctk_powermizer_start_timer(GtkWidget *widget)
{
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(widget);

    /* Start the powermizer timer */

    ctk_config_start_timer(ctk_powermizer->ctk_config,
                           (GSourceFunc) update_powermizer_info,
                           (gpointer) ctk_powermizer);
}

void ctk_powermizer_stop_timer(GtkWidget *widget)
{
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(widget);

    /* Stop the powermizer timer */

    ctk_config_stop_timer(ctk_powermizer->ctk_config,
                          (GSourceFunc) update_powermizer_info,
                          (gpointer) ctk_powermizer);
}
