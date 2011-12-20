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

#ifndef __CTK_DROP_DOWN_MENU_H__
#define __CTK_DROP_DOWN_MENU_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_DROP_DOWN_MENU (ctk_drop_down_menu_get_type())

#define CTK_DROP_DOWN_MENU(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DROP_DOWN_MENU, \
                                 CtkDropDownMenu))

#define CTK_DROP_DOWN_MENU_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DROP_DOWN_MENU, \
                              CtkDropDownMenuClass))

#define CTK_IS_DROP_DOWN_MENU(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DROP_DOWN_MENU))

#define CTK_IS_DROP_DOWN_MENU_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DROP_DOWN_MENU))

#define CTK_DROP_DOWN_MENU_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DROP_DOWN_MENU, \
                                CtkDropDownMenuClass))


#define CTK_DROP_DOWN_MENU_FLAG_MONOSPACE 0x1


typedef struct _CtkDropDownMenu       CtkDropDownMenu;
typedef struct _CtkDropDownMenuClass  CtkDropDownMenuClass;

typedef struct _CtkDropDownMenuValue  CtkDropDownMenuValue;

struct _CtkDropDownMenuValue {
    GtkWidget *menu_item;
    gint value;
};


struct _CtkDropDownMenu
{
    GtkVBox parent;

    GtkWidget *menu;
    GtkWidget *option_menu;
    
    guint flags;

    gint num_entries;
    CtkDropDownMenuValue *values;
};

struct _CtkDropDownMenuClass
{
    GtkVBoxClass parent_class;
};

GType      ctk_drop_down_menu_get_type            (void) G_GNUC_CONST;
GtkWidget* ctk_drop_down_menu_new                 (guint flags);
GtkWidget* ctk_drop_down_menu_append_item         (CtkDropDownMenu *d,
                                                   const gchar *name,
                                                   const gint value);
gint       ctk_drop_down_menu_get_current_value   (CtkDropDownMenu *d);
void       ctk_drop_down_menu_set_current_value   (CtkDropDownMenu *d,
                                                   gint value);
void       ctk_drop_down_menu_set_value_sensitive (CtkDropDownMenu *d,
                                                   gint value,
                                                   gboolean sensitive);
void       ctk_drop_down_menu_reset               (CtkDropDownMenu *d);



G_END_DECLS

#endif /* __CTK_DROP_DOWN_MENU_H__*/
