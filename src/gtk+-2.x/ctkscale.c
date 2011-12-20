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
#include <gdk/gdkkeysyms.h>

#include "ctkscale.h"
#include <stdio.h>


enum {
    PROP_0,
    PROP_GTK_ADJUSTMENT,
    PROP_LABEL
};

GType ctk_scale_get_type(
    void
)
{
    static GType ctk_scale_type = 0;

    if (!ctk_scale_type) {
        static const GTypeInfo ctk_scale_info = {
            sizeof (CtkScaleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkScale),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_scale_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkScale", &ctk_scale_info, 0);
    }

    return ctk_scale_type;
}



/*
 * ctk_scale_key_event() - GTK's default handling of the up/down keys
 * for hscale widgets is odd, so override it: the up key and the page
 * up key increase the adjustment value; similarly, the down key and
 * the page down key decrease the adjustment value.
 */

static gboolean ctk_scale_key_event(GtkWidget *widget, GdkEvent *event, 
                                    gpointer user_data)
{
    CtkScale *ctk_scale = CTK_SCALE(user_data);
    GdkEventKey *key_event = (GdkEventKey *) event;
    GtkAdjustment *adjustment = GTK_ADJUSTMENT(ctk_scale->gtk_adjustment);
    gdouble newval;
    
    switch (key_event->keyval) {

    case GDK_Left:
    case GDK_KP_Left:
    case GDK_Down:
    case GDK_KP_Down:
        newval = adjustment->value - adjustment->step_increment;
        break;
        
    case GDK_Right:
    case GDK_KP_Right:
    case GDK_Up:
    case GDK_KP_Up:
        newval = adjustment->value + adjustment->step_increment;
        break;

    case GDK_Page_Down:
    case GDK_KP_Page_Down:
        newval = adjustment->value - adjustment->page_increment;
        break;

    case GDK_Page_Up:
    case GDK_KP_Page_Up:
        newval = adjustment->value + adjustment->page_increment;
        break;

    default:
        return FALSE;
    }
    
    gtk_adjustment_set_value(adjustment, newval);
    
    return TRUE;
}


static void adjustment_value_changed(
    GtkAdjustment *adjustment,
    gpointer user_data
)
{
    CtkScale *ctk_scale = CTK_SCALE(user_data);
    gchar text[7];
    
    switch (ctk_scale->value_type) {
    case G_TYPE_INT:
        g_snprintf(text, 6, "%d", (gint) adjustment->value);
        break;
    case G_TYPE_DOUBLE:
    default:
        g_snprintf(text, 6, "%2.3f", adjustment->value);
        break;
    }
    
    gtk_entry_set_text(GTK_ENTRY(ctk_scale->text_entry), text);
}

static void text_entry_activate(
    GtkEntry *entry,
    gpointer user_data
)
{
    CtkScale *ctk_scale = CTK_SCALE(user_data);
    gdouble newval = g_strtod(gtk_entry_get_text(entry), NULL);
    gtk_adjustment_set_value(ctk_scale->gtk_adjustment, newval);
}



/*
 * text_entry_toggled() - 
 */

static void text_entry_toggled(CtkConfig *ctk_config, gpointer user_data)
{
    CtkScale *ctk_scale = CTK_SCALE(user_data);

    if (ctk_config_slider_text_entry_shown(ctk_config)) {
        if (!ctk_scale->text_entry_packed) {
            gtk_container_add(GTK_CONTAINER(ctk_scale->text_entry_container),
                              ctk_scale->text_entry);
        }
        gtk_widget_show(ctk_scale->text_entry);
        ctk_scale->text_entry_packed = TRUE;
    } else {
        if (ctk_scale->text_entry_packed) {
            gtk_container_remove
                (GTK_CONTAINER(ctk_scale->text_entry_container),
                 ctk_scale->text_entry);
        }
        gtk_widget_hide(ctk_scale->text_entry);
        ctk_scale->text_entry_packed = FALSE;
    }
} /* text_entry_toggled() */



/*
 * ctk_scale_new() - constructor for the Scale widget
 */

GtkWidget* ctk_scale_new(GtkAdjustment *gtk_adjustment,
                         const gchar *label_text,
                         CtkConfig *ctk_config,
                         gint value_type)
{
    GObject *object;
    CtkScale *ctk_scale;
    GtkWidget *label;
    GtkWidget *frame;
    GtkWidget *hbox;
    
    g_return_val_if_fail(GTK_IS_ADJUSTMENT(gtk_adjustment), NULL);
    g_return_val_if_fail(label_text != NULL, NULL);
    
    /* create and initialize the object */

    object = g_object_new(CTK_TYPE_SCALE, NULL);
    
    ctk_scale = CTK_SCALE(object);

    ctk_scale->gtk_adjustment = gtk_adjustment;
    ctk_scale->label = label_text;
    ctk_scale->ctk_config = ctk_config;
    ctk_scale->value_type = value_type;
    
    gtk_box_set_spacing (GTK_BOX (object), 2);
    
    /* scale label */
    
    label = gtk_label_new(ctk_scale->label);
    gtk_box_pack_start(GTK_BOX (object), label, FALSE, FALSE, 0);
    
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    
    /* frame around slider and text box */
    
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(object), frame, TRUE, TRUE, 0);
    
    /* event box (for tooltips) */

    ctk_scale->tooltip_widget = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(frame), ctk_scale->tooltip_widget);
    
    /* hbox to contain slider and text box */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(ctk_scale->tooltip_widget), hbox);

    /* text entry */
    
    ctk_scale->text_entry = gtk_entry_new_with_max_length(6);
    gtk_entry_set_width_chars(GTK_ENTRY(ctk_scale->text_entry), 6);
    
    /* text entry container */

    ctk_scale->text_entry_container = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(ctk_scale->text_entry_container),
                              GTK_SHADOW_NONE);
    gtk_container_set_border_width
        (GTK_CONTAINER(ctk_scale->text_entry_container), 0);
                                    

    gtk_container_add(GTK_CONTAINER(ctk_scale->text_entry_container),
                      ctk_scale->text_entry);
    ctk_scale->text_entry_packed = TRUE;
    g_object_ref(G_OBJECT(ctk_scale->text_entry));
    
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_scale->text_entry_container, FALSE, FALSE, 0);
    
    text_entry_toggled(ctk_scale->ctk_config, (gpointer) ctk_scale);

    /* wire up the adjustment events */

    adjustment_value_changed(ctk_scale->gtk_adjustment, G_OBJECT(ctk_scale));

    g_signal_connect(G_OBJECT(ctk_scale->gtk_adjustment), "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_scale);

    g_signal_connect(G_OBJECT(ctk_scale->text_entry), "activate",
                     G_CALLBACK(text_entry_activate),
                     (gpointer) ctk_scale);
                     
    g_signal_connect(G_OBJECT(ctk_config), "slider_text_entry_toggled",
                     G_CALLBACK(text_entry_toggled),
                     (gpointer) ctk_scale);

    /* the slider */

    ctk_scale->gtk_scale =
        gtk_hscale_new(GTK_ADJUSTMENT(ctk_scale->gtk_adjustment));
    
    gtk_scale_set_draw_value(GTK_SCALE(ctk_scale->gtk_scale), FALSE);
    
    gtk_scale_set_digits(GTK_SCALE(ctk_scale->gtk_scale), 0);


    gtk_box_pack_start(GTK_BOX(hbox), ctk_scale->gtk_scale, TRUE, TRUE, 3);
    
    g_signal_connect(ctk_scale->gtk_scale, "key_press_event",
                     G_CALLBACK(ctk_scale_key_event), G_OBJECT(ctk_scale));
    
    return GTK_WIDGET (object);
    
} /* ctk_scale_new() */
