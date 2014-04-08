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
#include <assert.h>
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

static void ctk_drop_down_menu_set_current_index(CtkDropDownMenu *d, gint index);


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

static void changed(GtkWidget *combo_box, gpointer user_data)
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
 * ctk_drop_down_menu_change_object() - abstract out the actual widget
 * that is being used, so that users of CtkDropDownMenu don't have to
 * know if the gtk widget is GtkCombo or GtkOptionMenu or anything else.
 */

GObject *ctk_drop_down_menu_change_object(GtkWidget* widget)
{
    CtkDropDownMenu *d = CTK_DROP_DOWN_MENU(widget);

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        return G_OBJECT(GTK_EDITABLE(GTK_BIN(d->combo_box)->child));
    } else {
        return G_OBJECT(d->combo_box);
    }
} /* ctk_drop_down_menu_change_object() */



/* 
 * ctk_drop_down_menu_changed() - callback function for GtkCombo combo box
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
    
    object = g_object_new(CTK_TYPE_DROP_DOWN_MENU, NULL);
    
    d = CTK_DROP_DOWN_MENU(object);

    d->flags = flags;
    d->values = NULL;
    d->num_entries = 0;
    d->current_selected_item = -1;

    if (flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        d->combo_box = gtk_combo_box_entry_new_text();
        g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_BIN(d->combo_box)->child)),
                         "changed",
                         G_CALLBACK(ctk_drop_down_menu_changed),
                         (gpointer) d);
    } else { 
        d->combo_box = gtk_combo_box_new_text();
        g_signal_connect(G_OBJECT(d->combo_box), "changed",
                         G_CALLBACK(changed), (gpointer) d);

    }

    gtk_box_set_spacing(GTK_BOX(d), 0);
    gtk_box_pack_start(GTK_BOX(d), d->combo_box, FALSE, FALSE, 0);

    return GTK_WIDGET(d);
    
} /* ctk_drop_down_menu_new() */



/*
 * ctk_drop_down_menu_reset() - Clears the internal combo box
 */

void ctk_drop_down_menu_reset(CtkDropDownMenu *d)
{
    GtkComboBox *combobox = GTK_COMBO_BOX(d->combo_box);

    if (d->values) {
        g_free(d->values);
        d->values = NULL;
    }

    d->num_entries = 0;

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(combobox)));

} /* ctk_drop_down_menu_reset() */



/*
 * ctk_drop_down_menu_append_item() - add a new entry to the drop down
 * combo box
 */

GtkWidget *ctk_drop_down_menu_append_item(CtkDropDownMenu *d,
                                          const gchar *name,
                                          const gint value)
{
    GtkWidget *label = NULL;

    d->values = g_realloc(d->values,
                          sizeof(CtkDropDownMenuValue) * (d->num_entries + 1));

    gtk_combo_box_append_text(GTK_COMBO_BOX(d->combo_box), name);
    d->values[d->num_entries].glist_item = g_strdup(name);

    d->values[d->num_entries].value = value;
    d->num_entries++;

    if (d->num_entries == 1) {
        /*
         * If this is the first item, make this the current item.
         */
        ctk_drop_down_menu_set_current_index(d, 0);
    }

    return label;

} /* ctk_drop_down_menu_append_item() */



/*
 * ctk_drop_down_menu_get_current_value() - return the current value selected in
 * the drop down combo box.  In the case where no current item is selected and
 * the menu has one or more valid items, this has the side effect of selecting
 * the first item from the menu as its current item, then returning its value.
 *
 * In the case where the menu has no valid items, this function returns 0.
 */
gint ctk_drop_down_menu_get_current_value(CtkDropDownMenu *d)
{
    gint i;

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        i = d->current_selected_item;
    } else {
        /* This returns -1 if no item is active */
        i = gtk_combo_box_get_active(GTK_COMBO_BOX(d->combo_box));
    }

    if (d->num_entries > 0) {
        /* Default to the first item if the current selection is invalid */
        if ((i < 0) || (i >= d->num_entries)) {
            i = 0;
            ctk_drop_down_menu_set_current_index(d, 0);
        }
        return d->values[i].value;
    } else {
        return 0;
    }

} /* ctk_drop_down_menu_get_current_value() */


/*
 * ctk_drop_down_menu_get_current_name() - get the current name in the combo
 * box.  In the case where no current item is selected and the menu has one
 * or more valid items, this has the side effect of selecting the first item
 * from the menu as its current item, then returning its name.
 *
 * In the case where the menu has no valid items, this will return an empty
 * string.
 *
 * The returned string points to internally allocated storage in the widget
 * and must not be modified, freed, or stored.
 */
const char *ctk_drop_down_menu_get_current_name(CtkDropDownMenu *d)
{
    gint i;

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        i = d->current_selected_item;
    } else {
        /* This returns -1 if no item is active */
        i = gtk_combo_box_get_active(GTK_COMBO_BOX(d->combo_box));
    }

    if (d->num_entries > 0) {
        /* Default to the first item if the current selection is invalid */
        if ((i < 0) || (i >= d->num_entries)) {
            i = 0;
            ctk_drop_down_menu_set_current_index(d, 0);
        }
        return d->values[i].glist_item;
    } else {
        return "";
    }
}

/*
 * ctk_drop_down_menu_set_current_value() - set the current value in
 * the combo box
 */

void ctk_drop_down_menu_set_current_value(CtkDropDownMenu *d, gint value)
{
    gint i;

    for (i = 0; i < d->num_entries; i++) {
        if (d->values[i].value == value) {
            ctk_drop_down_menu_set_current_index(d, i);
            return;
        }
    }
} /* ctk_drop_down_menu_set_current_value() */

/*
 * ctk_drop_down_menu_set_current_index() - set the current item (name/value)
 * in the combo box to the item at the given index.  Caller is expected to
 * pass in a valid index argument.
 */
void ctk_drop_down_menu_set_current_index(CtkDropDownMenu *d, gint index)
{
    assert((index >= 0) && (index < d->num_entries));

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        gtk_entry_set_text
            (GTK_ENTRY(GTK_BIN(d->combo_box)->child),
             d->values[index].glist_item);
        d->current_selected_item = index;
    } else {
        gtk_combo_box_set_active(GTK_COMBO_BOX(d->combo_box), index);
    }
} /* ctk_drop_down_menu_set_current_index() */



/*
 * ctk_drop_down_menu_set_value_sensitive() - set the specified
 * value's sensitivity
 */

void ctk_drop_down_menu_set_value_sensitive(CtkDropDownMenu *d,
                                            gint value, gboolean sensitive)
{

    if (d->flags & CTK_DROP_DOWN_MENU_FLAG_READWRITE) {
        ctk_drop_down_menu_set_current_value(d, value);
        gtk_widget_set_sensitive(GTK_WIDGET(GTK_BIN(d->combo_box)->child),
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


/*
 * ctk_drop_down_menu_set_tooltip() - adds the tooltip to the widget used
 * for the drop down combo box
 */

void ctk_drop_down_menu_set_tooltip(CtkConfig *ctk_config, CtkDropDownMenu *d,
                                    const gchar *text)
{
    ctk_config_set_tooltip(ctk_config, GTK_WIDGET(d->combo_box), text);
}

