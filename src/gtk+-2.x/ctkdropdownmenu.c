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

#include "ctkdropdownmenu.h"

enum {
    DROP_DOWN_MENU_CHANGED_SIGNAL,
    LAST_SIGNAL
};

static guint __signals[LAST_SIGNAL] = { 0 };


static void
ctk_drop_down_menu_class_init(CtkDropDownMenuClass *ctk_drop_down_menu_class);

static void ctk_drop_down_menu_free(GObject *object);


GType ctk_drop_down_menu_get_type(
    void
)
{
    static GType ctk_drop_down_menu_type = 0;

    if (!ctk_drop_down_menu_type) {
        static const GTypeInfo ctk_drop_down_menu_info = {
            sizeof (CtkDropDownMenuClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_drop_down_menu_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDropDownMenu),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_drop_down_menu_type = g_type_register_static(GTK_TYPE_VBOX,
                        "CtkDropDownMenu", &ctk_drop_down_menu_info, 0);
    }

    return ctk_drop_down_menu_type;
}


static void
ctk_drop_down_menu_class_init(CtkDropDownMenuClass *ctk_drop_down_menu_class)
{
    GObjectClass *gobject_class;
    
    gobject_class = (GObjectClass *) ctk_drop_down_menu_class;
    gobject_class->finalize = ctk_drop_down_menu_free;
    
    __signals[DROP_DOWN_MENU_CHANGED_SIGNAL] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(ctk_drop_down_menu_class),
                     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                     0, NULL, NULL,
                     g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    
}



/*
 * changed() - emit the "changed" signal
 */

static void changed(GtkOptionMenu *option_menu, gpointer user_data)
{
    CtkDropDownMenu *d = CTK_DROP_DOWN_MENU(user_data);
    
    g_signal_emit(G_OBJECT(d), __signals[DROP_DOWN_MENU_CHANGED_SIGNAL], 0);
    
} /* changed() */



/*
 * ctk_drop_down_menu_free() - free internal data allocated by the
 * CtkDropDownMenu
 */

static void ctk_drop_down_menu_free(GObject *object)
{
    CtkDropDownMenu *d;
    
    d = CTK_DROP_DOWN_MENU(object);
    
    g_free(d->values);
    
} /* ctk_drop_down_menu_free() */



/*
 * ctk_drop_down_menu_new() - constructor for the CtkDropDownMenu widget
 */

GtkWidget* ctk_drop_down_menu_new(guint flags)
{
    GObject *object;
    CtkDropDownMenu *d;
    
    object = g_object_new(CTK_TYPE_DROP_DOWN_MENU, NULL);
    
    d = CTK_DROP_DOWN_MENU(object);

    d->flags = flags;
    d->values = NULL;
    d->num_entries = 0;
    
    d->menu = gtk_menu_new();
    d->option_menu = gtk_option_menu_new();
    
    gtk_box_set_spacing(GTK_BOX(d), 0);
    gtk_box_pack_start(GTK_BOX(d), d->option_menu, FALSE, FALSE, 0);

    return GTK_WIDGET(d);
    
} /* ctk_drop_down_menu_new() */



/*
 * ctk_drop_down_menu_append_item() - add a new entry to the drop down
 * menu
 */

void ctk_drop_down_menu_append_item(CtkDropDownMenu *d,
                                    const gchar *name,
                                    const gint value)
{
    GtkWidget *menu_item, *label, *alignment;
    gchar *str;
    
    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(d->menu), menu_item);
    
    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_MONOSPACE) {
        str = g_strconcat("<tt><small>", name, "</small></tt>", NULL);
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), str);
        g_free(str);
    } else {
        label = gtk_label_new(name);
    }
    
    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_container_add(GTK_CONTAINER(menu_item), alignment);
    
    d->values = g_realloc(d->values,
                          sizeof(CtkDropDownMenuValue) * (d->num_entries + 1));
    d->values[d->num_entries].menu_item = menu_item;
    d->values[d->num_entries].value = value;
    
    d->num_entries++;
    
} /* ctk_drop_down_menu_append_item() */



/*
 * ctk_drop_down_menu_finalize() - to be called once all menu entries
 * have been added.
 */

void ctk_drop_down_menu_finalize(CtkDropDownMenu *d)
{
    gtk_option_menu_set_menu(GTK_OPTION_MENU(d->option_menu), d->menu);

    g_signal_connect(G_OBJECT(d->option_menu), "changed",
                     G_CALLBACK(changed), (gpointer) d);

    gtk_widget_show_all(GTK_WIDGET(d));

} /* ctk_drop_down_menu_finalize() */



/*
 * ctk_drop_down_menu_get_current_value() - return the current value
 * selected in the drop down menu.
 */

gint ctk_drop_down_menu_get_current_value(CtkDropDownMenu *d)
{
    gint i;
    
    i = gtk_option_menu_get_history(GTK_OPTION_MENU(d->option_menu));
    
    if (i < d->num_entries) {
        return d->values[i].value;
    } else {
        return 0; /* XXX??? */
    }
    
} /* ctk_drop_down_menu_get_current_value() */



/*
 * ctk_drop_down_menu_set_current_value() - set the current value in
 * the menu
 */

void ctk_drop_down_menu_set_current_value(CtkDropDownMenu *d, gint value)
{
    gint i;
    
    for (i = 0; i < d->num_entries; i++) {
        if (d->values[i].value == value) {
            gtk_option_menu_set_history(GTK_OPTION_MENU(d->option_menu), i);
            return;
        }
    }
} /* ctk_drop_down_menu_set_current_value() */



/*
 * ctk_drop_down_menu_set_value_sensitive() - set the specified
 * value's sensitivity
 */

void ctk_drop_down_menu_set_value_sensitive(CtkDropDownMenu *d,
                                            gint value, gboolean sensitive)
{
    gint i;
    
    for (i = 0; i < d->num_entries; i++) {
        if (d->values[i].value == value) {
            gtk_widget_set_sensitive(d->values[i].menu_item, sensitive);
            return;
        }
    }
} /* ctk_drop_down_menu_set_value_sensitive() */
