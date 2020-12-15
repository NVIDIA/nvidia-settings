/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2020 NVIDIA Corporation.
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

#ifndef __CTK_POWERMODE_H__
#define __CTK_POWERMODE_H__

#include "ctkevent.h"
#include "ctkconfig.h"
#include "ctkdropdownmenu.h"

G_BEGIN_DECLS

#define CTK_TYPE_POWERMODE (ctk_powermode_get_type())

#define CTK_POWERMODE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_POWERMODE, CtkPowermode))

#define CTK_POWERMODE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_POWERMODE, CtkPowermodeClass))

#define CTK_IS_POWERMODE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_POWERMODE))

#define CTK_IS_POWERMODE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_POWERMODE))

#define CTK_POWERMODE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_POWERMODE, CtkPowermodeClass))



typedef struct _CtkPowermode       CtkPowermode;
typedef struct _CtkPowermodeClass  CtkPowermodeClass;

struct _CtkPowermode
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;

    CtkDropDownMenu *powermode_menu;
    GtkWidget *current_powermode;

};

struct _CtkPowermodeClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_powermode_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_powermode_new         (CtrlTarget *, CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_powermode_create_help (GtkTextTagTable *, CtkPowermode *);

void           ctk_powermode_start_timer (GtkWidget *);
void           ctk_powermode_stop_timer  (GtkWidget *);


G_END_DECLS

#endif /* __CTK_POWERMODE_H__ */
