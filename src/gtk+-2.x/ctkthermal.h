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

#ifndef __CTK_THERMAL_H__
#define __CTK_THERMAL_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"

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

struct _CtkThermal
{
    GtkVBox parent;

    NvCtrlAttributeHandle *attribute_handle;
    CtkConfig *ctk_config;

    GtkWidget *core_label;
    GtkWidget *core_gauge;
    GtkWidget *ambient_label;
};

struct _CtkThermalClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_thermal_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_thermal_new         (NvCtrlAttributeHandle *, CtkConfig *);
GtkTextBuffer* ctk_thermal_create_help (GtkTextTagTable *);

G_END_DECLS

#endif /* __CTK_THERMAL_H__ */
