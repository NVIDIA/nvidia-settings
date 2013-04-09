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

#include <gtk/gtk.h>
#include <string.h>

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
            NULL  /* value_table */
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

static void changed(GtkWidget *menu, gpointer user_data)
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
    
    if (d->glist) {
        g_list_free(d->glist);
    }
    
} /* ctk_drop_down_menu_free() */



/* 
 * ctk_drop_down_menu_change_object() - abstract out the actual widget
 * that is being used, so that users of CtkDropDownMenu don't have to
 * know if the gtk widget is GtkCombo or GtkOptionMenu or anything else.
 */

GObject *ctk_drop_down_menu_change_object(GtkWidget* widget)
{
    CtkDropDownMenu *d = CTK_DROP_DOWN_MENU(widget);

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        return G_OBJECT(GTK_EDITABLE(GTK_COMBO(d->menu)->entry));
    } else {
        return G_OBJECT(d->option_menu);
    }
} /* ctk_drop_down_menu_change_object() */



/* 
 * ctk_drop_down_menu_changed() - callback function for GtkCombo menu
 * changed.
 */

static void ctk_drop_down_menu_changed(GtkEditable *editable, gpointer user_data)
{
    int i;
    CtkDropDownMenu *d = CTK_DROP_DOWN_MENU(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(editable));

    for (i = 0; i < d->num_entries; i++) {
        if (strcmp(d->values[i].glist_item, str) == 0) {
            d->current_selected_item = i;
            break;
        }
    }
    g_signal_emit(G_OBJECT(d), __signals[DROP_DOWN_MENU_CHANGED_SIGNAL], 0);
}



/*
 * ctk_drop_down_menu_new() - constructor for the CtkDropDownMenu widget
 */

GtkWidget* ctk_drop_down_menu_new(guint flags)
{
    GObject *object;
    CtkDropDownMenu *d;
    GtkWidget *menu_widget; // used to emit "changed" signal
    
    object = g_object_new(CTK_TYPE_DROP_DOWN_MENU, NULL);
    
    d = CTK_DROP_DOWN_MENU(object);

    d->flags = flags;
    d->values = NULL;
    d->num_entries = 0;

    if (flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        d->menu = gtk_combo_new();
        menu_widget = d->menu;
        g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(d->menu)->entry)),
                         "changed",
                         G_CALLBACK(ctk_drop_down_menu_changed),
                         (gpointer) d);
    } else { 
        d->option_menu = gtk_option_menu_new();
        d->menu = gtk_menu_new();
        gtk_option_menu_set_menu(GTK_OPTION_MENU(d->option_menu), d->menu);
        menu_widget = d->option_menu;
        g_signal_connect(G_OBJECT(d->option_menu), "changed",
                         G_CALLBACK(changed), (gpointer) d);

    }

    gtk_box_set_spacing(GTK_BOX(d), 0);
    gtk_box_pack_start(GTK_BOX(d), menu_widget, FALSE, FALSE, 0);

    return GTK_WIDGET(d);
    
} /* ctk_drop_down_menu_new() */



/*
 * ctk_drop_down_menu_reset() - Clears the internal menu
 */

void ctk_drop_down_menu_reset(CtkDropDownMenu *d)
{
    if (d->glist) {
        g_list_free(d->glist);
        d->glist = NULL;
    }
    if (d->values) {
        g_free(d->values);
        d->values = NULL;
    }

    d->num_entries = 0;

    if (!(d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO)) {
        d->menu = gtk_menu_new();
        gtk_option_menu_set_menu(GTK_OPTION_MENU(d->option_menu), d->menu);
    }

} /* ctk_drop_down_menu_reset() */



/*
 * ctk_drop_down_menu_append_item() - add a new entry to the drop down
 * menu
 */

GtkWidget *ctk_drop_down_menu_append_item(CtkDropDownMenu *d,
                                          const gchar *name,
                                          const gint value)
{
    GtkWidget *label = NULL;
    
    d->values = g_realloc(d->values,
                          sizeof(CtkDropDownMenuValue) * (d->num_entries + 1));
    
    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        d->glist = g_list_append(d->glist,
                                 g_strdup(name));

        gtk_combo_set_popdown_strings(GTK_COMBO(d->menu), d->glist);
        gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(d->menu)->entry), FALSE);
        d->values[d->num_entries].glist_item = g_strdup(name);
    } else {
        GtkWidget *menu_item, *alignment;
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
        d->values[d->num_entries].menu_item = menu_item;
        
    }

    d->values[d->num_entries].value = value;
    d->num_entries++;

    return label;
    
} /* ctk_drop_down_menu_append_item() */



/*
 * ctk_drop_down_menu_get_current_value() - return the current value
 * selected in the drop down menu, or 0 if the current item is undefined.
 */

gint ctk_drop_down_menu_get_current_value(CtkDropDownMenu *d)
{
    gint i;

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        i = d->current_selected_item;
    } else {
        i = gtk_option_menu_get_history(GTK_OPTION_MENU(d->option_menu));
    }

    if (i < d->num_entries) {
        return d->values[i].value;
    } else { 
        return 0; /* XXX??? */
    }

} /* ctk_drop_down_menu_get_current_value() */


/*
 * ctk_drop_down_menu_get_current_name() - get the current name in the menu, or
 * an empty string if the current item is undefined. The returned string points
 * to internally allocated storage in the widget and must not be modified,
 * freed, or stored.
 */
const char *ctk_drop_down_menu_get_current_name(CtkDropDownMenu *d)
{
    gint i;

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        i = d->current_selected_item;
    } else {
        i = gtk_option_menu_get_history(GTK_OPTION_MENU(d->option_menu));

    }

    if (i < d->num_entries) {
        return d->values[i].glist_item;
    } else { 
        return ""; /* XXX??? */
    }
}

/*
 * ctk_drop_down_menu_set_current_value() - set the current value in
 * the menu
 */

void ctk_drop_down_menu_set_current_value(CtkDropDownMenu *d, gint value)
{
    gint i;
    
    for (i = 0; i < d->num_entries; i++) {
        if (d->values[i].value == value) {
            if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
                gtk_entry_set_text
                    (GTK_ENTRY(GTK_COMBO(d->menu)->entry),
                     d->values[i].glist_item);
            } else {
                gtk_option_menu_set_history(GTK_OPTION_MENU(d->option_menu), i);
            }
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

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_COMBO) {
        ctk_drop_down_menu_set_current_value(d, value);
        gtk_widget_set_sensitive(GTK_WIDGET(GTK_COMBO(d->menu)->entry),
                                 sensitive);
    } else {
        gint i;
        for (i = 0; i < d->num_entries; i++) {
            if (d->values[i].value == value) {
                gtk_widget_set_sensitive(d->values[i].menu_item, sensitive);
                return;
            }
        }
    }
} /* ctk_drop_down_menu_set_value_sensitive() */
