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
#include "ctkutils.h"
#include "msg.h"




GtkWidget *add_table_row(GtkWidget *table,
                         const gint row,
                         const gfloat name_xalign, // 0 = left, 1 = right
                         const gfloat name_yalign, // 0 = top, 1 = bottom
                         const gchar *name,
                         const gfloat value_xalign,
                         const gfloat value_yalign,
                         const gchar *value)
{
    GtkWidget *label;

    label = gtk_label_new(name);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), name_xalign, name_yalign);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, row, row + 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    if (value == NULL)
        label = gtk_label_new("Unknown");
    else
        label = gtk_label_new(value);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), value_xalign, value_yalign);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, row, row + 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    return label;
}



/** ctk_get_parent_window() ******************************************
 *
 * Returns the parent window of a widget, if one exists
 *
 **/
GtkWidget * ctk_get_parent_window(GtkWidget *child)
{
    GtkWidget *parent = gtk_widget_get_parent(child);
    

    while (parent && !GTK_IS_WINDOW(parent)) {
        GtkWidget *last = parent;

        parent = gtk_widget_get_parent(last);
        if (!parent || parent == last) {
            /* GTK Error, can't find parent window! */
            parent = NULL;
            break;
        }
    }

    return parent;
    
} /* ctk_get_parent_window() */




/** ctk_display_error_msg() ******************************************
 *
 * Displays an error message.
 *
 **/
void ctk_display_error_msg(GtkWidget *parent, gchar * msg)
{
    GtkWidget *dlg;
    
    if (msg) {
        nv_error_msg(msg);

        if (parent) {
            dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_ERROR,
             GTK_BUTTONS_OK,
             msg);
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }

} /* ctk_display_error_msg() */



/** ctk_display_warning_msg() ****************************************
 *
 * Displays a warning message.
 *
 **/
void ctk_display_warning_msg(GtkWidget *parent, gchar * msg)
{
    GtkWidget *dlg;
    
    if (msg) {
        nv_warning_msg(msg);

        if (parent) {
            dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_WARNING,
             GTK_BUTTONS_OK,
             msg);
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }

} /* ctk_display_warning_msg() */



/** ctk_empty_container() ********************************************
 *
 * Removes all (non internal) children widgets from a container
 *
 * XXX It might be useful later on to allow a callback to be called
 *     for each widget that gets removed etc.
 *
 **/

void ctk_empty_container(GtkWidget *container)
{
    GList *list;
    GList *node;

    list = gtk_container_get_children(GTK_CONTAINER(container));
    node = list;
    while (node) {
        gtk_container_remove(GTK_CONTAINER(container),
                             (GtkWidget *)(node->data));
        node = node->next;
    }
    g_list_free(list);

} /* ctk_empty_container() */
