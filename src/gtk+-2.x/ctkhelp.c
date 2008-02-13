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
#include "ctkhelp.h"

#include "msg.h"
#include "ctkconstants.h"

#include "ctkimage.h"

#include <stdlib.h>

static GtkTextBuffer *create_default_help(CtkHelp *ctk_help);
static void close_button_clicked(GtkButton *button, gpointer user_data);
static gboolean window_destroy(GtkWidget *widget, GdkEvent *event,
                               gpointer user_data);



GType ctk_help_get_type(
    void
)
{
    static GType ctk_help_type = 0;

    if (!ctk_help_type) {
        static const GTypeInfo ctk_help_info = {
            sizeof (CtkHelpClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkHelp),
            0,    /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_help_type = g_type_register_static
            (GTK_TYPE_WINDOW, "CtkHelp", &ctk_help_info, 0);
    }

    return ctk_help_type;
}

GtkWidget* ctk_help_new(GtkWidget *toggle_button, GtkTextTagTable *tag_table)
{
    GObject *object;
    CtkHelp *ctk_help;
    GtkWidget *vbox, *hbox;
    GtkWidget *hseparator;
    GtkWidget *alignment;
    GtkWidget *button;
    GtkWidget *sw;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *textview;

    object = g_object_new(CTK_TYPE_HELP, NULL);

    ctk_help = CTK_HELP(object);
    
    ctk_help->toggle_button = toggle_button;

    gtk_window_set_title(GTK_WINDOW(ctk_help),
                         "NVIDIA X Server Settings Help");

    gtk_window_set_default_size(GTK_WINDOW(ctk_help), -1, 400);

    gtk_container_set_border_width(GTK_CONTAINER(ctk_help), CTK_WINDOW_PAD);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(ctk_help), vbox);
    
    /* create the banner */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    banner = ctk_banner_image_new(BANNER_ARTWORK_HELP);
    gtk_box_pack_start(GTK_BOX(hbox), banner, TRUE, TRUE, 0);
    
    /* create the scroll window to hold the text viewer */

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    
    gtk_container_add(GTK_CONTAINER(frame), sw);

    /* create the text viewer */
    
    textview = gtk_text_view_new();
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    ctk_help->text_viewer = textview;
    gtk_container_add(GTK_CONTAINER(sw), ctk_help->text_viewer);
    
    g_object_set(G_OBJECT(ctk_help->text_viewer),
                 "pixels-inside-wrap", 10, NULL);

    /* save the tag table */

    ctk_help->tag_table = tag_table;

    /* create the default help text */
    
    ctk_help->default_help = create_default_help(ctk_help);
    gtk_text_view_set_buffer
        (GTK_TEXT_VIEW(ctk_help->text_viewer), ctk_help->default_help);

    /* place a horizontal separator */
    
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);
    
    /* create and place the close button */
    
    hbox = gtk_hbox_new(FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);

    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(close_button_clicked),
                     (gpointer) ctk_help);

    /* handle destructive events to the window */

    g_signal_connect(G_OBJECT(ctk_help), "destroy-event",
                     G_CALLBACK(window_destroy), (gpointer) ctk_help);
    g_signal_connect(G_OBJECT(ctk_help), "delete-event",
                     G_CALLBACK(window_destroy), (gpointer) ctk_help);
    
    return GTK_WIDGET(ctk_help);
}

void ctk_help_set_page(CtkHelp *ctk_help, GtkTextBuffer *buffer)
{
    GtkTextBuffer *b;
    GtkTextIter iter;
    GtkTextView *view;
    GtkTextMark *mark;

    if (buffer) {
        b = buffer;
    } else {
        b = ctk_help->default_help;
    }

    view = GTK_TEXT_VIEW(ctk_help->text_viewer);
    
    /* set the buffer in the TextView */

    gtk_text_view_set_buffer(view, b);

    /* ensure that the top of the buffer is displayed */

    gtk_text_buffer_get_start_iter(b, &iter);
    mark = gtk_text_buffer_create_mark(b, NULL, &iter, TRUE);
    gtk_text_view_scroll_to_mark(view, mark, 0.0, TRUE, 0.0, 0.0);
    gtk_text_buffer_place_cursor(b, &iter);
}


static GtkTextBuffer *create_default_help(CtkHelp *ctk_help)
{
    GtkTextIter iter, start, end;
    GtkTextBuffer *buffer;

    buffer = gtk_text_buffer_new(ctk_help->tag_table);

    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    gtk_text_buffer_insert_with_tags_by_name
        (buffer, &iter, "\nNVIDIA X Server Settings Help", -1,
         CTK_HELP_TITLE_TAG, NULL);
    
    gtk_text_buffer_insert(buffer, &iter, "\n\nThere is no help available "
                           "for this page.", -1);

    /*
     * Apply CTK_HELP_HEADING_NOT_EDITABLE_TAG and
     * CTK_HELP_WORD_WRAP_TAG to the whole buffer
     */
    
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_HEADING_NOT_EDITABLE_TAG, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_WORD_WRAP_TAG, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_MARGIN_TAG, &start, &end);

    return buffer;
}


GtkTextTagTable *ctk_help_create_tag_table(void)
{
    GtkTextTagTable *table;
    GtkTextTag *tag;

    table = gtk_text_tag_table_new();

     /* CTK_HELP_TITLE_TAG */

    tag = gtk_text_tag_new(CTK_HELP_TITLE_TAG);
    
    g_object_set(G_OBJECT(tag),
                 "weight", PANGO_WEIGHT_BOLD,
                 "size", 15 * PANGO_SCALE,
                 NULL);
    
    gtk_text_tag_table_add(table, tag);

    /* CTK_HELP_HEADING_TAG */

    tag = gtk_text_tag_new(CTK_HELP_HEADING_TAG);
    
    g_object_set(G_OBJECT(tag),
                 "weight", PANGO_WEIGHT_BOLD,
                 "size", 12 * PANGO_SCALE,
                 NULL);
    
    gtk_text_tag_table_add(table, tag);

    /* CTK_HELP_HEADING_NOT_EDITABLE */

    tag = gtk_text_tag_new(CTK_HELP_HEADING_NOT_EDITABLE_TAG);
    
    g_object_set(G_OBJECT(tag), "editable", FALSE, NULL);
    
    gtk_text_tag_table_add(table, tag);
    
    /* CTK_HELP_WORD_WRAP_TAG */

    tag = gtk_text_tag_new(CTK_HELP_WORD_WRAP_TAG);
    
    g_object_set(G_OBJECT(tag), "wrap_mode", GTK_WRAP_WORD, NULL);
    
    gtk_text_tag_table_add(table, tag);
    
    /* CTK_HELP_MARGIN_TAG */

    tag = gtk_text_tag_new(CTK_HELP_MARGIN_TAG);
    
    g_object_set(G_OBJECT(tag), "left_margin", 10, "right_margin", 10, NULL);
    
    gtk_text_tag_table_add(table, tag);
    
    /* CTK_HELP_SINGLE_SPACE_TAG */

    tag = gtk_text_tag_new(CTK_HELP_SINGLE_SPACE_TAG);

    g_object_set(G_OBJECT(tag), "pixels_inside_wrap", 0, NULL);

    gtk_text_tag_table_add(table, tag);

    /* CTK_HELP_BOLD_TAG */

    tag = gtk_text_tag_new(CTK_HELP_BOLD_TAG);

    g_object_set(G_OBJECT(tag), "weight", PANGO_WEIGHT_BOLD, NULL);
    
    gtk_text_tag_table_add(table, tag);
    
    return table;
}



static void close_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkHelp *ctk_help = CTK_HELP(user_data);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_help->toggle_button),
                                 FALSE);
    
} /* close_button_clicked() */


static gboolean window_destroy(GtkWidget *widget, GdkEvent *event,
                               gpointer user_data)
{
    CtkHelp *ctk_help = CTK_HELP(user_data);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_help->toggle_button),
                                 FALSE);

    return TRUE;
}


/*****************************************************************************/

/*
 * Utility functions for building a help GtkTextBuffer.
 */

void ctk_help_title(GtkTextBuffer *buffer, GtkTextIter *iter,
                    const gchar *fmt, ...)
{
    gchar *a, *b;

    NV_VSNPRINTF(a, fmt);

    b = g_strconcat("\n", a, "\n", NULL);

    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, b, -1,
                                             CTK_HELP_TITLE_TAG, NULL);
    g_free(b);
    free(a);
}

void ctk_help_para(GtkTextBuffer *buffer, GtkTextIter *iter,
                   const gchar *fmt, ...)
{     
    gchar *a, *b;

    NV_VSNPRINTF(a, fmt);

    b = g_strconcat("\n", a, "\n", NULL);

    gtk_text_buffer_insert(buffer, iter, b, -1);

    g_free(b);
    free(a);
}    

void ctk_help_heading(GtkTextBuffer *buffer, GtkTextIter *iter,
                      const gchar *fmt, ...)
{
    gchar *a, *b;

    NV_VSNPRINTF(a, fmt);

    b = g_strconcat("\n", a, "\n", NULL);

    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, b, -1,
                                             CTK_HELP_HEADING_TAG, NULL);
    g_free(b);
    free(a);
}

void ctk_help_term(GtkTextBuffer *buffer, GtkTextIter *iter,
                   const gchar *fmt, ...)
{
    gchar *a, *b;

    NV_VSNPRINTF(a, fmt);

    b = g_strconcat("\n", a, NULL);
    
    gtk_text_buffer_insert_with_tags_by_name(buffer, iter, b, -1,
                                             CTK_HELP_BOLD_TAG, NULL);
    g_free(b);
    free(a);
}

void ctk_help_finish(GtkTextBuffer *buffer)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(buffer, &start, &end);
    
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_HEADING_NOT_EDITABLE_TAG, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_WORD_WRAP_TAG, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_MARGIN_TAG, &start, &end);
    gtk_text_buffer_apply_tag_by_name
        (buffer, CTK_HELP_SINGLE_SPACE_TAG, &start, &end);
}
