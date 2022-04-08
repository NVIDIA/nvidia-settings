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

#ifndef __CTK_ECC_H__
#define __CTK_ECC_H__

#include "ctkevent.h"
#include "ctkconfig.h"
#include "nvml.h"

G_BEGIN_DECLS

#define CTK_TYPE_ECC (ctk_ecc_get_type())

#define CTK_ECC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_ECC, CtkEcc))

#define CTK_ECC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_ECC, CtkEccClass))

#define CTK_IS_ECC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_ECC))

#define CTK_IS_ECC_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_ECC))

#define CTK_ECC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_ECC, CtkEccClass))


typedef struct _CtkEcc       CtkEcc;
typedef struct _CtkEccClass  CtkEccClass;

typedef struct _CtkEccDetailedTableRow
{
    GtkWidget *err_type;
    GtkWidget *mem_type;
    GtkWidget *vol_count;
    GtkWidget *agg_count;
    int vol_count_value;
    int agg_count_value;
} CtkEccDetailedTableRow;

struct _CtkEcc
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;

    GtkWidget* status;
    GtkWidget* sbit_error;
    GtkWidget* dbit_error;
    GtkWidget* aggregate_sbit_error;
    GtkWidget* aggregate_dbit_error;
    GtkWidget* clear_button;
    GtkWidget* clear_aggregate_button;
    GtkWidget* reset_default_config_button;
    GtkWidget* configuration_status;
    GtkWidget* summary_table;

    gboolean ecc_enabled;
    gboolean ecc_configured;
    gboolean ecc_toggle_warning_dlg_shown;
    gboolean sbit_error_available;
    gboolean dbit_error_available;
    gboolean aggregate_sbit_error_available;
    gboolean aggregate_dbit_error_available;
    gboolean ecc_config_supported;
    gboolean ecc_default_status;

    GtkWidget* detailed_table;
    CtkEccDetailedTableRow single_errors[NVML_MEMORY_LOCATION_COUNT];
    CtkEccDetailedTableRow double_errors[NVML_MEMORY_LOCATION_COUNT];
};

struct _CtkEccClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_ecc_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_ecc_new         (CtrlTarget *, CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_ecc_create_help (GtkTextTagTable *, CtkEcc *);

void           ctk_ecc_start_timer (GtkWidget *);
void           ctk_ecc_stop_timer  (GtkWidget *);

G_END_DECLS

#endif /* __CTK_ECC_H__ */
