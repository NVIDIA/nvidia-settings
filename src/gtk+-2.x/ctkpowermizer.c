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
static void update_powermizer_menu_info(CtkPowermizer *ctk_powermizer);
static void set_powermizer_menu_label_txt(CtkPowermizer *ctk_powermizer,
                                          gint powerMizerMode);
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

static const char *__memory_transfer_rate_freq_help =
"This indicates the current Memory transfer rate.";

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
"If a single X server is running, the mode selected in nvidia-settings is what "
"the system will be using; if two or more X servers are running, the behavior "
"is undefined.  ";

static const char *__powermizer_auto_mode_help =
"'Auto' mode lets the driver choose the best Performance State for your GPU.  ";

static const char *__powermizer_adaptive_mode_help =
"'Adaptive' mode allows the GPU clocks to be adjusted based on GPU "
"utilization.  ";

static const char *__powermizer_prefer_maximum_performance_help =
"'Prefer Maximum Performance' hints to the driver to prefer higher GPU clocks, "
"when possible.  ";

static const char *__powermizer_prefer_consistent_performance_help =
"'Prefer Consistent Performance' hints to the driver to lock to GPU base clocks, "
"when possible.  ";

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
    gint processorclock;
    gint nvclockmin;
    gint nvclockmax;
    gint memtransferrate;
    gint memtransferratemin;
    gint memtransferratemax;
    gint processorclockmin;
    gint processorclockmax;
} perfModeEntry, * perfModeEntryPtr;


static void apply_perf_mode_token(char *token, char *value, void *data)
{
    perfModeEntryPtr pEntry = (perfModeEntryPtr) data;

    if (!strcasecmp("perf", token)) {
        pEntry->perf_level = atoi(value);
    } else if (!strcasecmp("nvclock", token)) {
        pEntry->nvclock = atoi(value);
    } else if (!strcasecmp("nvclockmin", token)) {
        pEntry->nvclockmin = atoi(value);
    } else if (!strcasecmp("nvclockmax", token)) {
        pEntry->nvclockmax = atoi(value);
    } else if (!strcasecmp("memtransferrate", token)) {
        pEntry->memtransferrate = atoi(value);
    } else if (!strcasecmp("memtransferratemin", token)) {
        pEntry->memtransferratemin = atoi(value);
    } else if (!strcasecmp("memtransferratemax", token)) {
        pEntry->memtransferratemax = atoi(value);
    } else if (!strcasecmp("processorclock", token)) {
        pEntry->processorclock = atoi(value);
    } else if (!strcasecmp("processorclockmin", token)) {
        pEntry->processorclockmin = atoi(value);
    } else if (!strcasecmp("processorclockmax", token)) {
        pEntry->processorclockmax = atoi(value);
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
    char *tmp_perf_modes = NULL;
    char *tokens;
    char tmp_str[24];
    gint ret;
    gint row_idx = 0; /* Where to insert into the perf mode table */
    gint col_idx = 0; /* Column index adjustment factor */
    gboolean active;
    GtkWidget *vsep;
    perfModeEntryPtr pEntry = NULL;
    perfModeEntryPtr tmpEntry = NULL;
    gint index = 0;
    gint i = 0;

    /* Get the current list of perf levels */

    ret = NvCtrlGetStringAttribute(ctk_powermizer->attribute_handle,
                                   NV_CTRL_STRING_PERFORMANCE_MODES,
                                   &perf_modes);

    if (ret != NvCtrlSuccess) {
        /* Bail */
        return;
    }

    /* Calculate the number of rows we needed vseparator in the table */
    tmp_perf_modes = g_strdup(perf_modes);
    for (tokens = strtok(tmp_perf_modes, ";");
         tokens;
         tokens = strtok(NULL, ";")) {

        tmpEntry = realloc(pEntry, sizeof(*pEntry) * (index + 1));

        if (!tmpEntry) {
            continue;
        }
        pEntry = tmpEntry;
        tmpEntry = NULL;

        /* Invalidate perf mode entry */
        memset(pEntry + index, -1, sizeof(*pEntry));

        parse_token_value_pairs(tokens, apply_perf_mode_token,
                                (void *) &pEntry[index]);

        /* Only add complete perf mode entries */
        if ((pEntry[index].perf_level != -1) &&
            (pEntry[index].nvclockmax != -1)) {
            /* Set hasDecoupledClocks flag to decide new/old clock
             * interface to show.
             */
            if (!ctk_powermizer->hasDecoupledClock &&
                ((pEntry[index].nvclockmax != pEntry[index].nvclockmin) ||
                 (pEntry[index].memtransferratemax !=
                  pEntry[index].memtransferratemin) ||
                 (pEntry[index].processorclockmax !=
                  pEntry[index].processorclockmin))) {
                ctk_powermizer->hasDecoupledClock = TRUE;
            }
            row_idx++;
        }
        index++;
    }
    g_free(tmp_perf_modes);

    /* Since table cell management in GTK lacks, just remove and rebuild
     * the table from scratch.
     */

    /* Dump out the old table */

    ctk_empty_container(ctk_powermizer->performance_table_hbox);
    
    /* Generate a new table */

    if (ctk_powermizer->hasDecoupledClock) {
        table = gtk_table_new(2, 15, FALSE);
        row_idx = row_idx + 3;
        col_idx = 0;
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        gtk_container_set_border_width(GTK_CONTAINER(table), 5);

        gtk_box_pack_start(GTK_BOX(ctk_powermizer->performance_table_hbox),
                           table, FALSE, FALSE, 0);

        if (ctk_powermizer->performance_level) {
            label = gtk_label_new("Level");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            
            /* Vertical separator */
            vsep = gtk_vseparator_new();
            gtk_table_attach(GTK_TABLE(table), vsep, 1, 2, 0, row_idx,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
        }

        if (ctk_powermizer->gpu_clock) {
            /* Graphics clock */
            label = gtk_label_new("Graphics Clock");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+4, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Min");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Max");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+3, col_idx+4, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            /* Vertical separator */
            vsep = gtk_vseparator_new();
            gtk_table_attach(GTK_TABLE(table), vsep, col_idx+4, col_idx+5, 0,
                             row_idx,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
            col_idx += 4;
        }

        /* Memory transfer rate */
        if (ctk_powermizer->memory_transfer_rate &&
            pEntry[i].memtransferrate != -1) {
            label = gtk_label_new("Memory Transfer Rate");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+3, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Min");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Max");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            /* Vertical separator */
            vsep = gtk_vseparator_new();
            gtk_table_attach(GTK_TABLE(table), vsep, col_idx+3, col_idx+4,
                             0, row_idx,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
            col_idx += 4;
        }
        if (ctk_powermizer->processor_clock) {
            /* Processor clock */
            label = gtk_label_new("Processor Clock");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+3, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Min");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            label = gtk_label_new("Max");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3, 1, 2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            
            /* Vertical separator */
            vsep = gtk_vseparator_new();
            gtk_table_attach(GTK_TABLE(table), vsep, col_idx+3, col_idx+4,
                             0, row_idx,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
        }
    } else {

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

        if (ctk_powermizer->gpu_clock) {
            label = gtk_label_new("Graphics Clock");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            col_idx++;
        }

        if (pEntry[i].memtransferrate != -1 &&
            ctk_powermizer->memory_transfer_rate) {
            label = gtk_label_new("Memory Transfer Rate");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            col_idx++;
        }

        if (ctk_powermizer->processor_clock) {
            label = gtk_label_new("Processor Clock");
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2, 0, 1,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }
    }

    /* Parse the perf levels and populate the table */
    row_idx = 0; //reset value used to calculate vseparator.
    row_idx = 3;
    for (i = 0; i < index; i++) {   
        col_idx = 0;
        /* Only add complete perf mode entries */
        if (ctk_powermizer->hasDecoupledClock &&
            (pEntry[i].perf_level != -1) &&
            (pEntry[i].nvclockmax != -1)) {

            active = (pEntry[i].perf_level == perf_level);

            /* XXX Assume the perf levels are sorted by the server */

            gtk_table_resize(GTK_TABLE(table), row_idx+1, 10);

            if (ctk_powermizer->performance_level) {
                g_snprintf(tmp_str, 24, "%d", pEntry[i].perf_level);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 0, 1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx +=1;
            }

            if (ctk_powermizer->gpu_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].nvclockmin);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].nvclockmax);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx +=3;
            }
            if (ctk_powermizer->memory_transfer_rate &&
                pEntry[i].memtransferrate != -1) {
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].memtransferratemin);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].memtransferratemax);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx +=3;
            }
            if (ctk_powermizer->processor_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].processorclockmin);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+1, col_idx+2,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].processorclockmax);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx+2, col_idx+3,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            }
            row_idx++;
        } else if ((pEntry[i].perf_level != -1) &&
                   (pEntry[i].nvclock != -1)) {

            active = (pEntry[i].perf_level == perf_level);

            /* XXX Assume the perf levels are sorted by the server */

            gtk_table_resize(GTK_TABLE(table), row_idx+1, 10);

            if (ctk_powermizer->performance_level) {
                g_snprintf(tmp_str, 24, "%d", pEntry[i].perf_level);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, 0, 1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx++;
            }

            if (ctk_powermizer->gpu_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].nvclock);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx, col_idx+1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx++;
            }
            if (ctk_powermizer->memory_transfer_rate &&
                pEntry[i].memtransferrate != -1) {

                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].memtransferrate);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx, col_idx+1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
                col_idx++;
            }
            if (ctk_powermizer->processor_clock) {
                g_snprintf(tmp_str, 24, "%d MHz", pEntry[i].processorclock);
                label = gtk_label_new(tmp_str);
                gtk_widget_set_sensitive(label, active);
                gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
                gtk_table_attach(GTK_TABLE(table), label, col_idx, col_idx+1,
                                 row_idx, row_idx+1,
                                 GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
            }
            row_idx++;
        } else {
            nv_warning_msg("Incomplete Perf Mode (perf=%d, nvclock=%d,"
                           " memtransferrate=%d)",
                           pEntry[i].perf_level, pEntry[i].nvclock,
                           pEntry[i].memtransferrate);
        }
    }

    gtk_widget_show_all(table);

    XFree(perf_modes);
    XFree(pEntry);
    pEntry = NULL;
}



static gboolean update_powermizer_info(gpointer user_data)
{
    gint power_source, adaptive_clock, perf_level;
    gint gpu_clock, memory_transfer_rate;

    CtkPowermizer *ctk_powermizer;
    NvCtrlAttributeHandle *handle;
    gint ret;
    gchar *s;
    char *clock_string = NULL;
    perfModeEntry pEntry;

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

    /* Get the current values of clocks */

    ret = NvCtrlGetStringAttribute(ctk_powermizer->attribute_handle,
                                   NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS,
                                   &clock_string);

    if (ret == NvCtrlSuccess && ctk_powermizer->gpu_clock) {

        /* Invalidate the entries */
        memset(&pEntry, -1, sizeof(pEntry));

        parse_token_value_pairs(clock_string, apply_perf_mode_token,
                                &pEntry);

        if (pEntry.nvclock != -1) {
            gpu_clock = pEntry.nvclock;
            s = g_strdup_printf("%d Mhz", gpu_clock);
            gtk_label_set_text(GTK_LABEL(ctk_powermizer->gpu_clock), s);
            g_free(s);

        }

        if (ctk_powermizer->memory_transfer_rate &&
            pEntry.memtransferrate != -1) {
            memory_transfer_rate = pEntry.memtransferrate;
            s = g_strdup_printf("%d Mhz", memory_transfer_rate);
            gtk_label_set_text(GTK_LABEL(ctk_powermizer->memory_transfer_rate), s);
            g_free(s);
        }

        if (ctk_powermizer->processor_clock && pEntry.processorclock != -1) {
            s = g_strdup_printf("%d Mhz", pEntry.processorclock);
            gtk_label_set_text(GTK_LABEL(ctk_powermizer->processor_clock), s);
            g_free(s);
        }
    }
    XFree(clock_string);

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

    if (ctk_powermizer->performance_level && ctk_powermizer->gpu_clock) {
        /* update the perf table */

        update_perf_mode_table(ctk_powermizer, perf_level);
    }

    update_powermizer_menu_info(ctk_powermizer);

    return TRUE;
}



static gchar* get_powermizer_menu_label(const unsigned int val)
{
    gchar *label = NULL;

    switch (val) {
    case NV_CTRL_GPU_POWER_MIZER_MODE_AUTO:
        label = g_strdup_printf("Auto");
        break;
    case NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE:
        label = g_strdup_printf("Adaptive");
        break;
    case NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_MAXIMUM_PERFORMANCE:
        label = g_strdup_printf("Prefer Maximum Performance");
        break;
    case NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_CONSISTENT_PERFORMANCE:
        label = g_strdup_printf("Prefer Consistent Performance");
        break;
    default:
        label = g_strdup_printf("");
        break;
    }

    return label;
}



static gchar* get_powermizer_help_text(const unsigned int bit_mask)
{
    const gboolean bAuto =
        bit_mask & (1 << NV_CTRL_GPU_POWER_MIZER_MODE_AUTO);
    const gboolean bAdaptive =
        bit_mask & (1 << NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE);
    const gboolean bMaximum =
        bit_mask & (1 << NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_MAXIMUM_PERFORMANCE);
    const gboolean bConsistent =
        bit_mask & (1 << NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_CONSISTENT_PERFORMANCE);

    gchar *text = NULL;

    text =
        g_strdup_printf("%s%s%s%s%s",
                        __powermizer_menu_help,
                        bAuto ? __powermizer_auto_mode_help : "",
                        bAdaptive ? __powermizer_adaptive_mode_help : "",
                        bMaximum ? __powermizer_prefer_maximum_performance_help : "",
                        bConsistent ? __powermizer_prefer_consistent_performance_help : "");

    return text;
}


static void create_powermizer_menu_entry(CtkDropDownMenu *menu,
                                         const unsigned int bit_mask,
                                         const unsigned int val)
{
    gchar *label;

    if (!(bit_mask & (1 << val))) {
        return;
    }

    label = get_powermizer_menu_label(val);

    ctk_drop_down_menu_append_item(menu, label, val);
    g_free(label);
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
    gboolean gpu_clock_available = FALSE;
    gboolean mem_transfer_rate_available = FALSE;
    gboolean processor_clock_available = FALSE;
    gboolean power_source_available = FALSE;
    gboolean perf_level_available = FALSE;
    gboolean adaptive_clock_state_available = FALSE;
    gboolean cuda_dp_ui = FALSE;
    gboolean pcie_gen_queriable = FALSE;
    NVCTRLAttributeValidValuesRec valid_modes;
    char *clock_string = NULL;
    perfModeEntry pEntry;

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

    /* Check if reporting value of the clock supported */

    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS,
                                   &clock_string);

    if (ret == NvCtrlSuccess) {

        /* Invalidate the entries */
        memset(&pEntry, -1, sizeof(pEntry));

        parse_token_value_pairs(clock_string, apply_perf_mode_token,
                                &pEntry);

        if (pEntry.nvclock != -1) {
            gpu_clock_available = TRUE;
        } 
        if (pEntry.memtransferrate != -1) {
            mem_transfer_rate_available = TRUE;
        }
        if (pEntry.processorclock != -1) {
            processor_clock_available = TRUE;
        }
    }
    XFree(clock_string);

    /* NV_CTRL_GPU_PCIE_GENERATION */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_PCIE_GENERATION, &tmp);
    if (ret == NvCtrlSuccess) {
        pcie_gen_queriable = TRUE;
    }
    
    /* return early if query to attributes fail */
    if (!power_source_available && !perf_level_available &&
        !adaptive_clock_state_available && !gpu_clock_available &&
        !processor_clock_available && !pcie_gen_queriable) {
        return NULL;
    }
    /* create the CtkPowermizer object */

    object = g_object_new(CTK_TYPE_POWERMIZER, NULL);

    ctk_powermizer = CTK_POWERMIZER(object);
    ctk_powermizer->attribute_handle = handle;
    ctk_powermizer->ctk_config = ctk_config;
    ctk_powermizer->pcie_gen_queriable = pcie_gen_queriable;
    ctk_powermizer->hasDecoupledClock = FALSE;

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

    if (gpu_clock_available) {
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
    } else {
        ctk_powermizer->gpu_clock = NULL;
    }
    if (mem_transfer_rate_available) {
        ctk_powermizer->memory_transfer_rate =
            add_table_row_with_help_text(table, ctk_config,
                                         __memory_transfer_rate_freq_help,
                                         row++, //row
                                         0,  // column
                                         0.0f,
                                         0.5,
                                         "Memory Transfer Rate:",
                                         0.0,
                                         0.5,
                                         NULL);
    } else {
        ctk_powermizer->memory_transfer_rate = NULL;
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

    if (perf_level_available && gpu_clock_available) {
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

    ret = NvCtrlGetValidAttributeValues(ctk_powermizer->attribute_handle,
                                        NV_CTRL_GPU_POWER_MIZER_MODE,
                                        &valid_modes);

    if ((ret == NvCtrlSuccess) &&
        (valid_modes.type == ATTRIBUTE_TYPE_INT_BITS)) {
        const unsigned int bit_mask = valid_modes.u.bits.ints;

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

        create_powermizer_menu_entry(
            menu, bit_mask,
            NV_CTRL_GPU_POWER_MIZER_MODE_AUTO);
        create_powermizer_menu_entry(
            menu, bit_mask,
            NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE);
        create_powermizer_menu_entry(
            menu, bit_mask,
            NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_MAXIMUM_PERFORMANCE);
        create_powermizer_menu_entry(
            menu, bit_mask,
            NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_CONSISTENT_PERFORMANCE);

        ctk_powermizer->powermizer_menu = GTK_WIDGET(menu);

        g_signal_connect(G_OBJECT(ctk_powermizer->powermizer_menu), "changed",
                         G_CALLBACK(powermizer_menu_changed),
                         (gpointer) ctk_powermizer);

        ctk_powermizer->powermizer_menu_help =
            get_powermizer_help_text(bit_mask);

        ctk_config_set_tooltip(ctk_config,
                               ctk_powermizer->powermizer_menu,
                               ctk_powermizer->powermizer_menu_help);

        /* Packing the drop down list */

        table = gtk_table_new(1, 4, FALSE);
        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 0);
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
        ctk_powermizer->powermizer_txt = label;
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);

        update_powermizer_menu_info(ctk_powermizer);
    }

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



static void post_powermizer_menu_update(CtkPowermizer *ctk_powermizer)
{
    CtkDropDownMenu *menu =
        CTK_DROP_DOWN_MENU(ctk_powermizer->powermizer_menu);
    const char *label = ctk_drop_down_menu_get_current_name(menu);

    ctk_config_statusbar_message(ctk_powermizer->ctk_config,
                                 "Preferred Mode set to %s.", label);
}



static void update_powermizer_menu_event(GtkObject *object,
                                         gpointer arg1,
                                         gpointer user_data)
{
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(user_data);

    update_powermizer_menu_info(ctk_powermizer);

    post_powermizer_menu_update(ctk_powermizer);
}



static void set_powermizer_menu_label_txt(CtkPowermizer *ctk_powermizer,
                                          gint powerMizerMode)
{
    gint actualPowerMizerMode;
    gchar *label;
    
    if (powerMizerMode == NV_CTRL_GPU_POWER_MIZER_MODE_AUTO) {
        actualPowerMizerMode = ctk_powermizer->powermizer_default_mode;
    } else {
        actualPowerMizerMode = powerMizerMode;
    }
    
    label = get_powermizer_menu_label(actualPowerMizerMode);
    gtk_label_set_text(GTK_LABEL(ctk_powermizer->powermizer_txt), label);
    g_free(label);
}



static void update_powermizer_menu_info(CtkPowermizer *ctk_powermizer)
{
    gint powerMizerMode, defaultPowerMizerMode;
    ReturnStatus ret1, ret2;
    CtkDropDownMenu *menu;

    if (!ctk_powermizer->powermizer_menu) {
        return;
    }

    menu = CTK_DROP_DOWN_MENU(ctk_powermizer->powermizer_menu);

    ret1 = NvCtrlGetAttribute(ctk_powermizer->attribute_handle,
                             NV_CTRL_GPU_POWER_MIZER_MODE,
                             &powerMizerMode);

    ret2 = NvCtrlGetAttribute(ctk_powermizer->attribute_handle,
                              NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE,
                              &defaultPowerMizerMode);

    if ((ret1 != NvCtrlSuccess) || (ret2 != NvCtrlSuccess)) {
        gtk_widget_hide(ctk_powermizer->box_powermizer_menu);
    } else {
        g_signal_handlers_block_by_func(G_OBJECT(ctk_powermizer->powermizer_menu),
                                        G_CALLBACK(powermizer_menu_changed),
                                        (gpointer) ctk_powermizer);

        ctk_powermizer->powermizer_default_mode = defaultPowerMizerMode;

        ctk_drop_down_menu_set_current_value(menu, powerMizerMode);

        set_powermizer_menu_label_txt(ctk_powermizer, powerMizerMode);

        g_signal_handlers_unblock_by_func(G_OBJECT(ctk_powermizer->powermizer_menu),
                                          G_CALLBACK(powermizer_menu_changed),
                                          (gpointer) ctk_powermizer);

        gtk_widget_show(ctk_powermizer->box_powermizer_menu);
    }
}



static void powermizer_menu_changed(GtkWidget *widget,
                                    gpointer user_data)
{
    CtkPowermizer *ctk_powermizer = CTK_POWERMIZER(user_data);
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    guint powerMizerMode;
    ReturnStatus ret;
    const char *label = ctk_drop_down_menu_get_current_name(menu);


    powerMizerMode = ctk_drop_down_menu_get_current_value(menu);

    ret = NvCtrlSetAttribute(ctk_powermizer->attribute_handle,
                             NV_CTRL_GPU_POWER_MIZER_MODE,
                             powerMizerMode);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_powermizer->ctk_config,
                                     "Unable to set Preferred Mode to %s.",
                                     label);
        return;
    }

    set_powermizer_menu_label_txt(ctk_powermizer, powerMizerMode);

    post_powermizer_menu_update(ctk_powermizer);
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

    ctk_config_statusbar_message(ctk_powermizer->ctk_config, "%s", message);
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
        ctk_help_para(b, &i, "%s", __adaptive_clock_help);
    }

    if (ctk_powermizer->gpu_clock) {
        ctk_help_heading(b, &i, "Clock Frequencies");
        if (ctk_powermizer->memory_transfer_rate && 
            ctk_powermizer->processor_clock) {
            s = "This indicates the GPU's current Graphics Clock, "
                "Memory transfer rate and Processor Clock frequencies.";
        } else if (ctk_powermizer->memory_transfer_rate) {
            s = "This indicates the GPU's current Graphics Clock and "
                "Memory transfer rate.";
        } else {
            s = "This indicates the GPU's current Graphics Clock ferquencies.";
        }
        ctk_help_para(b, &i, "%s", s);
    }

    if (ctk_powermizer->power_source) {
        ctk_help_heading(b, &i, "Power Source");
        ctk_help_para(b, &i, "%s", __power_source_help);
    }

    if (ctk_powermizer->pcie_gen_queriable) {
        ctk_help_heading(b, &i, "Current PCIe link width");
        ctk_help_para(b, &i, "%s", __current_pcie_link_width_help);
        ctk_help_heading(b, &i, "Current PCIe link speed");
        ctk_help_para(b, &i, "%s", __current_pcie_link_speed_help);
    }

    if (ctk_powermizer->performance_level) {
        ctk_help_heading(b, &i, "Performance Level");
        ctk_help_para(b, &i, "%s", __performance_level_help);
        ctk_help_heading(b, &i, "Performance Levels (Table)");
        ctk_help_para(b, &i, "%s", __performance_levels_table_help);
    }

    if (ctk_powermizer->powermizer_menu) {
        ctk_help_heading(b, &i, "PowerMizer Settings");
        ctk_help_para(b, &i, "%s", ctk_powermizer->powermizer_menu_help);
    }

    if (ctk_powermizer->configuration_button) {
        ctk_help_heading(b, &i, "CUDA - Double precision");
        ctk_help_para(b, &i, "%s", __dp_configuration_button_help);
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
