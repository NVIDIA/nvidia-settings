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

#ifndef __CTK_POWERMIZER_H__
#define __CTK_POWERMIZER_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS

#define CTK_TYPE_POWERMIZER (ctk_powermizer_get_type())

#define CTK_POWERMIZER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_POWERMIZER, CtkPowermizer))

#define CTK_POWERMIZER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_POWERMIZER, CtkPowermizerClass))

#define CTK_IS_POWERMIZER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_POWERMIZER))

#define CTK_IS_POWERMIZER_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_POWERMIZER))

#define CTK_POWERMIZER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_POWERMIZER, CtkPowermizerClass))

typedef struct _CtkPowermizer       CtkPowermizer;
typedef struct _CtkPowermizerClass  CtkPowermizerClass;

struct _CtkPowermizer
{
    GtkVBox parent;

    NvCtrlAttributeHandle *attribute_handle;
    CtkConfig *ctk_config;

    GtkWidget *adaptive_clock_status;
    GtkWidget *gpu_clock;
    GtkWidget *memory_transfer_rate;
    GtkWidget *processor_clock;
    GtkWidget *power_source;
    GtkWidget *performance_level;
    GtkWidget *performance_table_hbox;
    GtkWidget *performance_table_hbox1;
    GtkWidget *powermizer_menu;
    GtkWidget *powermizer_txt;
    GtkWidget *box_powermizer_menu;

    gchar     *powermizer_menu_help;

    GtkWidget *configuration_button;
    gboolean  dp_enabled;
    gboolean  dp_toggle_warning_dlg_shown;
    gboolean  hasDecoupledClock;
    gboolean  hasEditablePerfLevel;
    gboolean  license_accepted;
    gint      attribute;
    gint      powermizer_default_mode;
    GtkWidget *status;

    GtkWidget *enable_dialog;
    GtkWidget *enable_checkbox;
    GtkWidget *editable_perf_level_table;
    gint      num_perf_levels;

    GtkWidget *link_width;
    GtkWidget *link_speed;
    gboolean  pcie_gen_queriable;
};

struct _CtkPowermizerClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_powermizer_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_powermizer_new         (NvCtrlAttributeHandle *,
                                           CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_powermizer_create_help (GtkTextTagTable *, CtkPowermizer *);

void           ctk_powermizer_start_timer (GtkWidget *);
void           ctk_powermizer_stop_timer  (GtkWidget *);

G_END_DECLS

#endif /* __CTK_POWERMIZER_H__ */
