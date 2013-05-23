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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "msg.h"

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkpowermizer.h"
#include "ctkbanner.h"
#include "ctkdropdownmenu.h"



#define FRAME_PADDING 10
#define DEFAULT_UPDATE_POWERMIZER_INFO_TIME_INTERVAL 1000

static gboolean update_powermizer_info(gpointer);
static void update_powermizer_menu_info(gpointer);
static void powermizer_menu_changed(GtkWidget*, gpointer);
static void update_powermizer_menu_event(GtkObject *object,
                                         gpointer arg1,
                                         gpointer user_data);
static void dp_config_button_toggled(GtkWidget *, gpointer);
static void dp_set_config_status(CtkPowermizer *);
static void dp_update_config_status(CtkPowermizer *, gboolean);
static void dp_configuration_update_received(GtkObject *, gpointer, gpointer);
static void post_dp_configuration_update(CtkPowermizer *);
static void show_dp_toggle_warning_dlg(CtkPowermizer *ctk_powermizer);

typedef struct {
    const char *label;
    int attr;
} PowerMizerMode;

PowerMizerMode  __powermizer_modes[] =
{
    { "Adaptive", NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE },
    { "Prefer Maximum Performance", NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_MAXIMUM_PERFORMANCE },
};

static const char *__adaptive_clock_help =
"The Adaptive Clocking status describes if this feature "
"is currently enabled in this GPU.";

static const char *__power_source_help =
"The Power Source indicates whether the machine "
"is running on AC or Battery power.";

static const char *__current_pcie_link_width_help =
"This is the current PCIe link width of the GPU, in number of lanes.";

static const char *__current_pcie_link_speed_help =
"This is the current PCIe link speed of the GPU, "
"in gigatransfers per second (GT/s).";

static const char *__performance_level_help =
"This indicates the current Performance Level of the GPU.";

static const char *__gpu_clock_freq_help =
"This indicates the current Graphics Clock frequency.";

static const char *__memory_clock_freq_help =
"This indicates the current Memory Clock frequency.";

static const char *__processor_clock_freq_help =
"This indicates the current Processor Clock frequency.";

static const char *__performance_levels_table_help =
"This indicates the Performance Levels available for the GPU.  Each "
"performance level is indicated by a Performance Level number, along with "
"the Graphics, Memory and Processor clocks for that level.  The currently active "
"performance level is shown in regular text.  All other performance "
"levels are shown in gray.";

static const char *__powermizer_menu_help =
"The Preferred Mode menu allows you to choose the preferred Performance "
"State for the GPU, provided the GPU has multiple Performance Levels.  "
"'Adaptive' mode allows the GPU clocks to be adjusted based on GPU "
"utilization.  'Prefer Maximum Performance' hints to the driver to prefer "
"higher GPU clocks, when possible.  If a single X server is running, the "
"mode selected in nvidia-settings is what the system will be using; if two or "
"more X servers are running, the behavior is undefined.";

static const char *__dp_configuration_button_help =
"CUDA - Double Precision lets you enable "
"increased double-precision calculations in CUDA applications.  Available on "
"GPUs with the capability for increased double-precision performance."
"  NOTE: Selecting a GPU reduces performance for non-CUDA applications, "
"including games.  To increase game performance, disable this checkbox.";

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
            NULL  /* value_table */
        };

        ctk_powermizer_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkPowermizer",
                                   &ctk_powermizer_info, 0);
    }

    return ctk_powermizer_type;

} /* ctk_powermizer_get_type() */



typedef struct {
    gint perf_level;
    gint nvclock;
    gint memclock;
    gint processorclock;
} perfModeEntry, * perfModeEntryPtr;


static void apply_perf_mode_token(char *token, char *value, void *data)
{
    perfModeEntryPtr pEntry = (perfModeEntryPtr) data;

    if (!strcasecmp("perf", token)) {
        pEntry->perf_level = atoi(value);
    } else if (!strcasecmp("nvclock", token)) {
        pEntry->nvclock = atoi(value);
    } else if (!strcasecmp("memclock", token)) {
        pEntry->memclock = atoi(value);
    } else if (!strcasecmp("processorclock", token)) {
        pEntry->processorclock = atoi(value);
    } else {
        nv_warning_msg("Unknown Perf Mode token value pair: %s=%s",
                       token, value);
    }
}


static void update_perf_mode_table(CtkPowermizer *ctk_powermizer,
                                   gint perf_level)
{
    GtkWidget *table;
    GtkWidget *label;
    char *perf_modes = NULL;
    char *tokens;
    char tmp_str[24];
    perfModeEntry entry;
    gint ret;
    gint row_idx; /* Where to insert into the perf mode table */
    gboolean active;

    /* Since table cell management in GTK lacks, just remove and rebuild
     * the table from scratch.
     */
    
    /* Dump out the old table */

    ctk_empty_container(ctk_powermizer->performance_table_hbox);
    
    /* Generate a new table */

    table = gtk_table_new(1, 4, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    gtk_box_pack_start(GTK_BOX(ctk_powermizer->performance_table_hbox),
                       table, FALSE, FALSE, 0);

    if (ctk_powermizer->performance_level) {
        label = gtk_label_new("Performance Level");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    }

    if (ctk_powermizer->gpu_clock && ctk_powermizer->memory_clock) {
        label = gtk_label_new("Graphics Clock");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        label = gtk_label_new("Memory Clock");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    }

    if (ctk_powermizer->processor_clock) {
        label = gtk_label_new("Processor Clock");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    }
    /* Get the current list of perf levels */

    ret = NvCtrlGetStringAttribute(ctk_powermizer->attribute_handle,
                                   NV_CTRL_STRING_PERFORMANCE_MODES,
                                   &perf_modes); 

    if (ret != NvCtrlSuccess) {
        gtk_widget_show_all(table);
        /* Bail */
        return;
    }

    /* Parse the perf levels and populate the table */

    row_idx = 1;
    for (tokens = strtok(perf_modes, ";");
         tokens;
         tokens = strtok(NULL, ";")) {

        /* Invalidate perf mode entry */
        entry.perf_level = -1;
        entry.nvclock = -1;
        entry.memclock = -1;
        entry.processorclock = -1;
        
        parse_token_value_pairs(tokens, apply_perf_mode_token,
                                &entry);
        
        /* Only add complete perf mode entries */
        if ((entry.perf_level != -1) &&
            (entry.nvclock != -1) &&
            (entry.memclock != -1)) {
            
            active = (entry.perf_level == perf_level);

            /* XXX Assume the perf levels are sorted by the server */

            gtk_table_resize(GTK_TABLE(table), row_idx+1, 4);

            if (ctk_powermizer->performance_level) {
                g_snprintf(tmp_str, 24, "%d", entry.perf_level);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 0, 1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            }

            if (ctk_powermizer->gpu_clock && ctk_powermizer->memory_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", entry.nvclock);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 1, 2,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

                g_snprintf(tmp_str, 24, "%d MHz", entry.memclock);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 2, 3,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            }
            if (ctk_powermizer->processor_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", entry.processorclock);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 3, 4, row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            }
            row_idx++;
        } else {
            nv_warning_msg("Incomplete Perf Mode (perf=%d, nvclock=%d,"
                           " memclock=%d)",
                           entry.perf_level, entry.nvclock,
                           entry.memclock);
        }
    }

    gtk_widget_show_all(table);

    XFree(perf_modes);
}



static gboolean update_powermizer_info(gpointer user_data)
{
    gint power_source, adaptive_clock, perf_level;
    gint clockret, gpu_clock, memory_clock, processor_clock;

    CtkPowermizer *ctk_powermizer;
    NvCtrlAttributeHandle *handle;
    gint ret;
    gchar *s;

    ctk_powermizer = CTK_POWERMIZER(user_data);
    handle = ctk_powermizer->attribute_handle;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE,
                             &adaptive_clock);
    if (ret == NvCtrlSuccess && ctk_powermizer->adaptive_clock_status) { 

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
    }
 
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_CLOCK_FREQS, 
                             &clockret);
    if (ret == NvCtrlSuccess && ctk_powermizer->gpu_clock &&
        ctk_powermizer->memory_clock) {

        memory_clock = clockret & 0x0000FFFF;
        gpu_clock = (clockret >> 16);

        s = g_strdup_printf("%d Mhz", gpu_clock);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->gpu_clock), s);
        g_free(s);

        s = g_strdup_printf("%d Mhz", memory_clock);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->memory_clock), s);
        g_free(s);
    }

    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_GPU_CURRENT_PROCESSOR_CLOCK_FREQS,
                             &processor_clock);
    if (ret == NvCtrlSuccess && ctk_powermizer->processor_clock) {
        s = g_strdup_printf("%d Mhz", processor_clock);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->processor_clock), s);
        g_free(s);
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_POWER_SOURCE, &power_source);
    if (ret == NvCtrlSuccess && ctk_powermizer->power_source) {

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
    }

    if (ctk_powermizer->pcie_gen_queriable) {
        /* NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH */
        s = get_pcie_link_width_string(handle,
                                       NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->link_width), s);
        g_free(s);

        /* NV_CTRL_GPU_PCIE_MAX_LINK_SPEED */
        s = get_pcie_link_speed_string(handle,
                                       NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->link_speed), s);
        g_free(s);
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, 
                             &perf_level);
    if (ret == NvCtrlSuccess && ctk_powermizer->performance_level) {
        s = g_strdup_printf("%d", perf_level);
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->performance_level), s);
        g_free(s);
    }

    if (ctk_powermizer->performance_level && ctk_powermizer->gpu_clock &&
        ctk_powermizer->memory_clock) {
        /* update the perf table */

        update_perf_mode_table(ctk_powermizer, perf_level);
    }

    return TRUE;
}

GtkWidget* ctk_powermizer_new(NvCtrlAttributeHandle *handle,
                              CtkConfig *ctk_config,
                              CtkEvent *ctk_event)
{
    GObject *object;
    CtkPowermizer *ctk_powermizer;
    GtkWidget *hbox, *hbox2, *vbox, *vbox2, *hsep, *table;
    GtkWidget *banner, *label;
    CtkDropDownMenu *menu;
    ReturnStatus ret, ret1;
    gint attribute;
    gint val;
    gint row = 0;
    gchar *s = NULL;
    gint tmp;
    gint i;
    gboolean processor_clock_available = FALSE;
    gboolean power_source_available = FALSE;
    gboolean perf_level_available = FALSE;
    gboolean adaptive_clock_state_available = FALSE;
    gboolean clock_freqs_available = FALSE;
    gboolean cuda_dp_ui = FALSE;
    gboolean pcie_gen_queriable = FALSE;

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);
    
    /* check if this screen supports powermizer querying */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_POWER_SOURCE, &val);
    if (ret == NvCtrlSuccess) {
        power_source_available = TRUE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL, 
                             &val);
    if (ret == NvCtrlSuccess) {
        perf_level_available = TRUE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE, 
                             &val);
    if (ret == NvCtrlSuccess) {
        adaptive_clock_state_available = TRUE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_CLOCK_FREQS, 
                             &val);
    if (ret == NvCtrlSuccess) {
        clock_freqs_available = TRUE;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CURRENT_PROCESSOR_CLOCK_FREQS,
                             &val);
    if (ret == NvCtrlSuccess) {
        processor_clock_available = TRUE;
    }

    /* NV_CTRL_GPU_PCIE_GENERATION */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_PCIE_GENERATION, &tmp);
    if (ret == NvCtrlSuccess) {
        pcie_gen_queriable = TRUE;
    }

    /* return early if query to attributes fail */
    if (!power_source_available && !perf_level_available &&
        !adaptive_clock_state_available && clock_freqs_available &&
        !processor_clock_available && !pcie_gen_queriable) {
        return NULL;
    }
    /* create the CtkPowermizer object */

    object = g_object_new(CTK_TYPE_POWERMIZER, NULL);
    
    ctk_powermizer = CTK_POWERMIZER(object);
    ctk_powermizer->attribute_handle = handle;
    ctk_powermizer->ctk_config = ctk_config;
    ctk_powermizer->pcie_gen_queriable = pcie_gen_queriable;

    /* set container properties for the CtkPowermizer widget */

    gtk_box_set_spacing(GTK_BOX(ctk_powermizer), 5);

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

    table = gtk_table_new(21, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 10);

    /* Adaptive Clocking State */
    if (adaptive_clock_state_available) {
        ctk_powermizer->adaptive_clock_status =
            add_table_row_with_help_text(table, ctk_config,
                                         __adaptive_clock_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Adaptive Clocking:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->adaptive_clock_status = NULL;
    }

    /* Clock Frequencies */

    if (clock_freqs_available) {
        /* spacing */
        row += 3;
        ctk_powermizer->gpu_clock =
            add_table_row_with_help_text(table, ctk_config,
                                         __gpu_clock_freq_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Graphics Clock:",
                                         0.0,
                                         0.5,
                                         NULL);

        ctk_powermizer->memory_clock =
            add_table_row_with_help_text(table, ctk_config,
                                         __memory_clock_freq_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Memory Clock:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->gpu_clock = NULL;
        ctk_powermizer->memory_clock = NULL;
    }
    /* Processor clock */
    if (processor_clock_available) {
        /* spacing */
        row += 3;
        ctk_powermizer->processor_clock =
            add_table_row_with_help_text(table, ctk_config,
                                         __processor_clock_freq_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Processor Clock:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->processor_clock = NULL;
    }
    /* Power Source */
    if (power_source_available) {
        /* spacing */
        row += 3;
        ctk_powermizer->power_source =
            add_table_row_with_help_text(table, ctk_config,
                                         __power_source_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Power Source:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->power_source = NULL;
    }
    /* Current PCIe Link Width */
    if (ctk_powermizer->pcie_gen_queriable) {
        /* spacing */
        row += 3;
        ctk_powermizer->link_width =
            add_table_row_with_help_text(table, ctk_config,
                                         __current_pcie_link_width_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Current PCIe Link Width:",
                                         0.0,
                                         0.5,
                                         NULL);

        /* Current PCIe Link Speed */
        ctk_powermizer->link_speed =
            add_table_row_with_help_text(table, ctk_config,
                                         __current_pcie_link_speed_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Current PCIe Link Speed:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->link_width = NULL;
        ctk_powermizer->link_speed = NULL;
    }

    /* Performance Level */
    if (perf_level_available) {
        /* spacing */
        row += 3;
        ctk_powermizer->performance_level =
            add_table_row_with_help_text(table, ctk_config,
                                         __performance_level_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Performance Level:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->performance_level = NULL;
    }
    gtk_table_resize(GTK_TABLE(table), row, 2);

    /* Available Performance Level Title */

    if (perf_level_available && clock_freqs_available) {
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        label = gtk_label_new("Performance Levels");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        hsep = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

        /* Available Performance Level Table */

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        ctk_powermizer->performance_table_hbox = hbox;
    }

    /* Register a timer callback to update the temperatures */

    s = g_strdup_printf("PowerMizer Monitor (GPU %d)",
                        NvCtrlGetTargetId(handle));
    
    ctk_config_add_timer(ctk_powermizer->ctk_config,
                         DEFAULT_UPDATE_POWERMIZER_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_powermizer_info,
                         (gpointer) ctk_powermizer);
    g_free(s);

    /* PowerMizer Settings */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    vbox2 = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
    ctk_powermizer->box_powermizer_menu = vbox2;

    /* H-separator */

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox2, FALSE, FALSE, 0);

    label = gtk_label_new("PowerMizer Settings");
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox2), hsep, TRUE, TRUE, 5);

    /* Specifying drop down list */

    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_COMBO);

    for (i=0; i < ARRAY_LEN(__powermizer_modes); i++) {
        ctk_drop_down_menu_append_item(menu, __powermizer_modes[i].label, i);
    }

    ctk_powermizer->powermizer_menu = GTK_WIDGET(menu);
    update_powermizer_menu_info(ctk_powermizer);

    g_signal_connect(G_OBJECT(ctk_powermizer->powermizer_menu), "changed",
                     G_CALLBACK(powermizer_menu_changed),
                     (gpointer) ctk_powermizer);

    ctk_config_set_tooltip(ctk_config,
                           ctk_powermizer->powermizer_menu,
                           __powermizer_menu_help);

    /* Packing the drop down list */

    table = gtk_table_new(1, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new("Preferred Mode:");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox2, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox2),
                       ctk_powermizer->powermizer_menu,
                       FALSE, FALSE, 0);

    /*
     * check if CUDA - Double Precision Boost support available.
     */

    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE,
                             &val);
    if (ret == NvCtrlSuccess) {
        attribute = NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE;
        cuda_dp_ui = TRUE;
    } else {
        ret1 = NvCtrlGetAttribute(handle,
                                  NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT,
                                  &val);
        if (ret1 == NvCtrlSuccess) {
            attribute = NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT;
            cuda_dp_ui = TRUE;
        }
    }

    if (cuda_dp_ui) {
        ctk_powermizer->attribute = attribute;
        ctk_powermizer->dp_toggle_warning_dlg_shown = FALSE;

        /* Query CUDA - Double Precision Boost Status */

        dp_update_config_status(ctk_powermizer, val);

        /* CUDA - Double Precision Boost configuration settings */

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

        label = gtk_label_new("CUDA");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

        hsep = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

        hbox2 = gtk_hbox_new(FALSE, 0);
        ctk_powermizer->configuration_button =
            gtk_check_button_new_with_label("CUDA - Double precision");
        gtk_box_pack_start(GTK_BOX(hbox2),
                           ctk_powermizer->configuration_button,
                           FALSE, FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(hbox2), 0);
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_powermizer->configuration_button),
             ctk_powermizer->dp_enabled);

        /* Packing */

        table = gtk_table_new(1, 1, FALSE);
        gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);

        gtk_table_attach(GTK_TABLE(table), hbox2, 0, 1, 0, 1,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        if (attribute == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT) {
            GtkWidget *separator;

            gtk_table_resize(GTK_TABLE(table), 1, 3);
            /* V-bar */
            hbox2 = gtk_hbox_new(FALSE, 0);
            separator = gtk_vseparator_new();
            gtk_box_pack_start(GTK_BOX(hbox2), separator, FALSE, FALSE, 0);
            gtk_table_attach(GTK_TABLE(table), hbox2, 1, 2, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            ctk_powermizer->status = gtk_label_new("");
            gtk_misc_set_alignment(GTK_MISC(ctk_powermizer->status), 0.0f, 0.5f);
            hbox2 = gtk_hbox_new(FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hbox2),
                               ctk_powermizer->status, FALSE, FALSE, 0);

            gtk_table_attach(GTK_TABLE(table), hbox2, 2, 3, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }

        ctk_config_set_tooltip(ctk_config, ctk_powermizer->configuration_button,
                               __dp_configuration_button_help);
        g_signal_connect(G_OBJECT(ctk_powermizer->configuration_button),
                         "clicked",
                         G_CALLBACK(dp_config_button_toggled),
                         (gpointer) ctk_powermizer);
        if (attribute == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE) {
            g_signal_connect(G_OBJECT(ctk_event),
                   CTK_EVENT_NAME(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE),
                   G_CALLBACK(dp_configuration_update_received),
                   (gpointer) ctk_powermizer);
        } else if (attribute == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT) {
            g_signal_connect(G_OBJECT(ctk_event),
                   CTK_EVENT_NAME(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT),
                   G_CALLBACK(dp_configuration_update_received),
                   (gpointer) ctk_powermizer);
        }
    } else {
        ctk_powermizer->configuration_button = NULL;
    }

    /* Updating the powermizer page */

    update_powermizer_info(ctk_powermizer);
    gtk_widget_show_all(GTK_WIDGET(ctk_powermizer));

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_POWER_MIZER_MODE),
                     G_CALLBACK(update_powermizer_menu_event),
                     (gpointer) ctk_powermizer);

    return GTK_WIDGET(ctk_powermizer);
}

static void update_powermizer_menu_event(GtkObject *object,
                                         gpointer arg1,
                                         gpointer user_data)
{
    update_powermizer_menu_info(user_data);
}

static void update_powermizer_menu_info(gpointer user_data)
{
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(user_data);
    gint powerMizerMode = NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE;
    NVCTRLAttributeValidValuesRec valid;
    ReturnStatus ret0, ret1;
    CtkDropDownMenu *menu;

    menu = CTK_DROP_DOWN_MENU(ctk_powermizer->powermizer_menu);

    ret0 = NvCtrlGetValidAttributeValues(ctk_powermizer->attribute_handle,
                                         NV_CTRL_GPU_POWER_MIZER_MODE,
                                         &valid);

    ret1 = NvCtrlGetAttribute(ctk_powermizer->attribute_handle,
                              NV_CTRL_GPU_POWER_MIZER_MODE,
                              &powerMizerMode);

    if ((ret0 != NvCtrlSuccess) || (ret1 != NvCtrlSuccess)) {
        gtk_widget_hide(ctk_powermizer->box_powermizer_menu);
    } else {
        g_signal_handlers_block_by_func(G_OBJECT(ctk_powermizer->powermizer_menu),
                                        G_CALLBACK(powermizer_menu_changed),
                                        (gpointer) ctk_powermizer);

        ctk_drop_down_menu_set_current_value(menu, powerMizerMode);

        g_signal_handlers_unblock_by_func(G_OBJECT(ctk_powermizer->powermizer_menu),
                                          G_CALLBACK(powermizer_menu_changed),
                                          (gpointer) ctk_powermizer);

        gtk_widget_show(ctk_powermizer->box_powermizer_menu);
    }
}

static void powermizer_menu_changed(GtkWidget *widget,
                                    gpointer user_data)
{
    CtkPowermizer *ctk_powermizer;
    gint history, powerMizerMode = NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    ctk_powermizer = CTK_POWERMIZER(user_data);

    history = ctk_drop_down_menu_get_current_value(menu);

    powerMizerMode = __powermizer_modes[history].attr;

    if (NvCtrlSuccess != NvCtrlSetAttribute(ctk_powermizer->attribute_handle,
                                            NV_CTRL_GPU_POWER_MIZER_MODE,
                                            powerMizerMode)) {
        return;
    }

    ctk_config_statusbar_message(ctk_powermizer->ctk_config, 
                                 "Preferred Mode set to %s.",
                                 __powermizer_modes[history].label);
}



static void show_dp_toggle_warning_dlg(CtkPowermizer *ctk_powermizer)
{
    GtkWidget *dlg, *parent;

    /* return early if message dialog already shown */
    if (ctk_powermizer->dp_toggle_warning_dlg_shown) {
        return;
    }
    ctk_powermizer->dp_toggle_warning_dlg_shown = TRUE;
    parent = ctk_get_parent_window(GTK_WIDGET(ctk_powermizer));

    dlg = gtk_message_dialog_new (GTK_WINDOW(parent),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_OK,
                                  "Changes to the CUDA - Double precision "
                                  "setting "
                                  "require a system reboot before "
                                  "taking effect.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy (dlg);

} /* show_dp_toggle_warning_dlg() */



/*
 * post_dp_configuration_update() - this function updates status bar string.
 */

static void post_dp_configuration_update(CtkPowermizer *ctk_powermizer)
{
    gboolean enabled = ctk_powermizer->dp_enabled;

    const char *conf_string = enabled ? "enabled" : "disabled";
    char message[128];

    if (ctk_powermizer->attribute == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT) {
        snprintf(message, sizeof(message), "CUDA - Double precision will "
                 "be %s after reboot.",
                 conf_string);
    } else {
        snprintf(message, sizeof(message), "CUDA - Double precision %s.",
                 conf_string);
    }

    ctk_config_statusbar_message(ctk_powermizer->ctk_config, message);
} /* post_dp_configuration_update() */



/*
 * dp_set_config_status() - set CUDA - Double Precision Boost configuration
 * button status.
 */

static void dp_set_config_status(CtkPowermizer *ctk_powermizer)
{
    GtkWidget *configuration_button = ctk_powermizer->configuration_button;

    g_signal_handlers_block_by_func(G_OBJECT(configuration_button),
                                    G_CALLBACK(dp_config_button_toggled),
                                    (gpointer) ctk_powermizer);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(configuration_button),
                                 ctk_powermizer->dp_enabled);

    g_signal_handlers_unblock_by_func(G_OBJECT(configuration_button),
                                      G_CALLBACK(dp_config_button_toggled),
                                      (gpointer) ctk_powermizer);
} /* dp_set_config_status() */



/*
 * dp_update_config_status - get current CUDA - Double Precision Boost status.
 */

static void dp_update_config_status(CtkPowermizer *ctk_powermizer, gboolean val)
{
    if ((ctk_powermizer->attribute ==
         NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE &&
         val == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE_DISABLED) ||
        (ctk_powermizer->attribute ==
         NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT &&
         val == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT_DISABLED)) {
        ctk_powermizer->dp_enabled = FALSE;
    } else {
        ctk_powermizer->dp_enabled = TRUE;
    }
} /* dp_update_config_status() */



/*
 * dp_configuration_update_received() - this function is called when the
 * NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE attribute is changed by another
 * NV-CONTROL client.
 */

static void dp_configuration_update_received(GtkObject *object,
                                             gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(user_data);

    ctk_powermizer->dp_enabled = event_struct->value;

    /* set CUDA - Double Precision Boost configuration buttion status */
    dp_set_config_status(ctk_powermizer);

    /* Update status bar message */
    post_dp_configuration_update(ctk_powermizer);
} /* dp_configuration_update_received() */



/*
 * dp_config_button_toggled() - callback function for
 * enable CUDA - Double Precision Boost checkbox.
 */

static void dp_config_button_toggled(GtkWidget *widget,
                                     gpointer user_data)
{
    gboolean enabled;
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(user_data);
    ReturnStatus ret;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* show popup dialog when user first time click DP config */
    if (ctk_powermizer->attribute == NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT) {
        show_dp_toggle_warning_dlg(ctk_powermizer);
    }

    /* set the newly specified CUDA - Double Precision Boost value */
    ret = NvCtrlSetAttribute(ctk_powermizer->attribute_handle,
                             ctk_powermizer->attribute,
                             enabled);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_powermizer->ctk_config,
                                     "Failed to set "
                                     "CUDA - Double precision "
                                     "configuration!");
        return;
    }

    ctk_powermizer->dp_enabled = enabled;
    dp_set_config_status(ctk_powermizer);
    if (ctk_powermizer->status) {
        gtk_label_set_text(GTK_LABEL(ctk_powermizer->status),
                           "pending reboot");
    }

    /* Update status bar message */
    post_dp_configuration_update(ctk_powermizer);
} /* dp_config_button_toggled() */



GtkTextBuffer *ctk_powermizer_create_help(GtkTextTagTable *table,
                                          CtkPowermizer *ctk_powermizer)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    gchar *s;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "PowerMizer Monitor Help");
    ctk_help_para(b, &i, "This page shows powermizer monitor options "
                  "available on this GPU.");

    if (ctk_powermizer->adaptive_clock_status) {
        ctk_help_heading(b, &i, "Adaptive Clocking");
        ctk_help_para(b, &i, __adaptive_clock_help);
    }

    if (ctk_powermizer->gpu_clock && ctk_powermizer->memory_clock) {
        ctk_help_heading(b, &i, "Clock Frequencies");
        if (ctk_powermizer->processor_clock) {
            s = "This indicates the GPU's current Graphics Clock, "
                "Memory Clock and Processor Clock frequencies.";
        } else {
            s = "This indicates the GPU's current Graphics Clock and "
                "Memory Clock frequencies.";
        }
        ctk_help_para(b, &i, s);
    }

    if (ctk_powermizer->power_source) {
        ctk_help_heading(b, &i, "Power Source");
        ctk_help_para(b, &i, __power_source_help);
    }

    if (ctk_powermizer->pcie_gen_queriable) {
        ctk_help_heading(b, &i, "Current PCIe link width");
        ctk_help_para(b, &i, __current_pcie_link_width_help);
        ctk_help_heading(b, &i, "Current PCIe link speed");
        ctk_help_para(b, &i, __current_pcie_link_speed_help);
    }

    if (ctk_powermizer->performance_level) {
        ctk_help_heading(b, &i, "Performance Level");
        ctk_help_para(b, &i, __performance_level_help);
        ctk_help_heading(b, &i, "Performance Levels (Table)");
        ctk_help_para(b, &i, __performance_levels_table_help);
    }

    if (ctk_powermizer->powermizer_menu) {
        ctk_help_heading(b, &i, "PowerMizer Settings");
        ctk_help_para(b, &i, __powermizer_menu_help);
    }

    if (ctk_powermizer->configuration_button) {
        ctk_help_heading(b, &i, "CUDA - Double precision");
        ctk_help_para(b, &i, __dp_configuration_button_help);
    }

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
