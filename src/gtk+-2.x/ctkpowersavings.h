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

#ifndef __CTK_POWER_SAVINGS_H__
#define __CTK_POWER_SAVINGS_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_POWER_SAVINGS (ctk_power_savings_get_type())

#define CTK_POWER_SAVINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_POWER_SAVINGS, CtkPowerSavings))

#define CTK_POWER_SAVINGS_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_POWER_SAVINGS, CtkPowerSavingsClass))

#define CTK_IS_POWER_SAVINGS(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_POWER_SAVINGS))

#define CTK_IS_POWER_SAVINGS_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_POWER_SAVINGS))

#define CTK_POWER_SAVINGS_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_POWER_SAVINGS, CtkPowerSavingsClass))


typedef struct _CtkPowerSavings       CtkPowerSavings;
typedef struct _CtkPowerSavingsClass  CtkPowerSavingsClass;

struct _CtkPowerSavings
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *vblank_control_button;
};

struct _CtkPowerSavingsClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_power_savings_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_power_savings_new       (NvCtrlAttributeHandle *,
                                         CtkConfig *, CtkEvent *);

GtkTextBuffer *ctk_power_savings_create_help(GtkTextTagTable *, CtkPowerSavings *);

G_END_DECLS

#endif /* __CTK_POWER_SAVINGS_H__ */

