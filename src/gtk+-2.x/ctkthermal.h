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

#ifndef __CTK_THERMAL_H__
#define __CTK_THERMAL_H__

#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS

#define CTK_TYPE_THERMAL (ctk_thermal_get_type())

#define CTK_THERMAL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_THERMAL, CtkThermal))

#define CTK_THERMAL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_THERMAL, CtkThermalClass))

#define CTK_IS_THERMAL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_THERMAL))

#define CTK_IS_THERMAL_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_THERMAL))

#define CTK_THERMAL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_THERMAL, CtkThermalClass))


typedef struct _CtkThermal       CtkThermal;
typedef struct _CtkThermalClass  CtkThermalClass;

typedef struct _CoolerControl {
    CtrlAttributeValidValues range;
    CtrlTarget *ctrl_target;

    int level;
    Bool changed;              /* Cooler level moved by user */ 
    GtkWidget *widget;         /* Cooler level control widget */
    GtkAdjustment *adjustment; /* Track adjustment */
    CtkEvent *event;           /* Receive NV_CONTROL events */
} CoolerControlRec, *CoolerControlPtr;

typedef struct {
    int format;
    const char *name;
} nvctrlFormatName;

typedef struct _SensorInfo {
    CtrlTarget *ctrl_target;
    int currentTemp;
    int minTemp;
    int maxTemp;

    GtkWidget *target_type;
    GtkWidget *provider_type;
    GtkWidget *temp_label;
    GtkWidget *core_gauge;
} SensorInfoRec, *SensorInfoPtr;

struct _CtkThermal
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;

    GtkWidget *core_label;
    GtkWidget *core_gauge;
    GtkWidget *ambient_label;
    GtkWidget *apply_button;
    GtkWidget *reset_button;
    GtkWidget *enable_checkbox;
    GtkWidget *fan_control_frame;
    GtkWidget *adaptive_clock_status;
    GtkWidget *fan_target;
    GtkWidget *fan_signal;
    GtkWidget *fan_control_policy;
    GtkWidget *cooler_table_hbox;
    GtkWidget *fan_information_box;

    gboolean cooler_control_enabled;
    gboolean settings_changed;
    gboolean show_fan_control_frame;
    gboolean enable_reset_button;
    CoolerControlPtr cooler_control;
    SensorInfoPtr sensor_info;
    int cooler_count;
    int sensor_count;
    gboolean thermal_sensor_target_type_supported;
    gboolean thermal_cooler_extra_info_supported;
};

struct _CtkThermalClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_thermal_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_thermal_new         (CtrlTarget *, CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_thermal_create_help (GtkTextTagTable *, CtkThermal *);

void           ctk_thermal_start_timer (GtkWidget *);
void           ctk_thermal_stop_timer  (GtkWidget *);

G_END_DECLS

#endif /* __CTK_THERMAL_H__ */
