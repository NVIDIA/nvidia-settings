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

/*
 * ctkwindow.c - implementation of the CtkWindow widget; this widget
 * displays a treeview on the left side, and one of several children
 * widgets on the right side, depending on which item in the treeview
 * is selected.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>

#include "ctkwindow.h"

#include "ctkframelock.h"
#include "ctkgvo.h"
#include "ctkconfig.h"

#include "ctkdevice.h"
#include "ctkcolorcorrection.h"
#include "ctkxvideo.h"
#include "ctkrandr.h"
#include "ctkcursorshadow.h"
#include "ctkopengl.h"
#include "ctkglx.h"
#include "ctkmultisample.h"
#include "ctkthermal.h"

#include "ctkdisplaydevice.h"
#include "ctkdisplaydevice-crt.h"
#include "ctkdisplaydevice-tv.h"
#include "ctkdisplaydevice-dfp.h"

#include "ctkhelp.h"
#include "ctkevent.h"
#include "ctkconstants.h"

/* column enumeration */

enum {
    CTK_WINDOW_LABEL_COLUMN = 0,
    CTK_WINDOW_WIDGET_COLUMN,
    CTK_WINDOW_HELP_COLUMN,
    CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
    CTK_WINDOW_NUM_COLUMNS
};


typedef void (*config_file_attributes_func_t)(GtkWidget *, ParsedAttribute *);

static void ctk_window_class_init(CtkWindowClass *);

static void ctk_window_real_destroy(GtkObject *);

static void add_page(GtkWidget *, GtkTextBuffer *, CtkWindow *,
                     GtkTreeIter *, const gchar *,
                     config_file_attributes_func_t func);

static GtkWidget *create_quit_dialog(CtkWindow *ctk_window);

static void quit_response(GtkWidget *, gint, gpointer);
static void save_settings_and_exit(CtkWindow *);

static void add_special_config_file_attributes(CtkWindow *ctk_window);

static void add_display_devices(GtkWidget *widget, GtkTextBuffer *help,
                                CtkWindow *ctk_window, GtkTreeIter *iter,
                                NvCtrlAttributeHandle *handle,
                                CtkEvent *ctk_event,
                                GtkTextTagTable *tag_table);


static GObjectClass *parent_class;


/*
 * ctk_window_get_type() - when first called, tells GTK about the
 * CtkWindow widget class, and gets an ID that uniquely identifies the
 * widget class. Upon subsequent calls, it just returns the ID.
 */

GType ctk_window_get_type(void)
{
    static GType ctk_window_type = 0;

    if (!ctk_window_type) {
        static const GTypeInfo ctk_window_info = {
            sizeof(CtkWindowClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) ctk_window_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(CtkWindow),
            0,                  /* n_preallocs */
            NULL,               /* instance_init */
        };

        ctk_window_type = g_type_register_static
            (GTK_TYPE_WINDOW, "CtkWindow", &ctk_window_info, 0);
    }

    return ctk_window_type;
    
} /* ctk_window_get_type() */



/*
 * ctk_window_class_init() - initializes the fields of the widget's
 * class structure
 */

static void ctk_window_class_init(CtkWindowClass *ctk_window_class)
{
    GObjectClass *gobject_class;
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) ctk_window_class;
    gobject_class = (GObjectClass *) ctk_window_class;

    parent_class = g_type_class_peek_parent(ctk_window_class);

    object_class->destroy = ctk_window_real_destroy;

} /* ctk_window_class_init() */



/*
 * ctk_window_real_destroy() - quit gtk.  XXX Maybe we should write
 * the configuration file here?
 */

static void ctk_window_real_destroy(GtkObject *object)
{
    GTK_OBJECT_CLASS(parent_class)->destroy(object);
    gtk_main_quit();

} /* ctk_window_real_destroy() */



/*
 * close_button_clicked() - called when the 
 */

static void close_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkWindow *ctk_window = CTK_WINDOW(user_data);
    CtkConfig *ctk_config = ctk_window->ctk_config;

    if (ctk_config->conf->booleans & CONFIG_PROPERTIES_SHOW_QUIT_DIALOG) {
        /* ask for confirmation */
        gtk_widget_show_all(ctk_window->quit_dialog);
    } else {
        /* doesn't return */
        save_settings_and_exit(ctk_window);
    }
} /* close_button_clicked() */



/*
 * help_button_toggled() - callback when the help button is toggled;
 * hides or shows the help window.
 */

static void help_button_toggled(GtkToggleButton *button, gpointer user_data)
{
    CtkWindow *ctk_window;
    gboolean enabled;

    ctk_window = CTK_WINDOW(user_data);

    enabled = gtk_toggle_button_get_active(button);

    if (enabled) gtk_widget_show_all(ctk_window->ctk_help);
    else gtk_widget_hide_all(ctk_window->ctk_help);

} /* help_button_toggled() */



/*
 * tree_selection_changed() - the selection in the tree changed;
 * change which page is viewed accordingly.
 */

static void tree_selection_changed(GtkTreeSelection *selection,
                                   gpointer user_data)
{
    GtkTreeIter iter;
    CtkWindow *ctk_window = CTK_WINDOW(user_data);
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_window->tree_store);
    gchar *str;
    GtkWidget *widget;
    GtkTextBuffer *help;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, CTK_WINDOW_LABEL_COLUMN, &str, -1);
    gtk_tree_model_get(model, &iter, CTK_WINDOW_WIDGET_COLUMN, &widget, -1);
    gtk_tree_model_get(model, &iter, CTK_WINDOW_HELP_COLUMN, &help, -1);

    /*
     * remove the existing widget from the page viewer, if anything is
     * presently there
     */

    if (ctk_window->page) {
        gtk_container_remove(GTK_CONTAINER(ctk_window->page_viewer),
                             ctk_window->page);
        ctk_window->page = NULL;
    }

    if (widget) {
        ctk_window->page = widget;
        gtk_box_pack_start(GTK_BOX(ctk_window->page_viewer), widget,
                           TRUE, TRUE, 2);
    }

    /* update the help page */

    ctk_help_set_page(CTK_HELP(ctk_window->ctk_help), help);
    
} /* tree_selection_changed() */



/*
 * tree_view_key_event() - callback for additional keyboard events we
 * want to track (space and Return) to expand and collapse collapsable
 * categories in the treeview.
 */

static gboolean tree_view_key_event(GtkWidget *tree_view, GdkEvent *event, 
                                    gpointer user_data)
{
    GdkEventKey *key_event = (GdkEventKey *) event;
    CtkWindow *ctk_window = CTK_WINDOW(user_data);

    if ((key_event->keyval == GDK_space) ||
        (key_event->keyval == GDK_Return)) {
        
        GtkTreeSelection* selection;
        GtkTreeModel *model;
        GtkTreeIter iter;
        GtkTreePath* path;

        selection = gtk_tree_view_get_selection(ctk_window->treeview);
        
        if (!gtk_tree_selection_get_selected(selection, &model, &iter))
            return FALSE;
        
        path = gtk_tree_model_get_path(model, &iter);

        if (gtk_tree_view_row_expanded(ctk_window->treeview, path)) {
            gtk_tree_view_collapse_row(ctk_window->treeview, path);
        } else {
            gtk_tree_view_expand_row(ctk_window->treeview, path, FALSE);
        }

        gtk_tree_path_free(path);

        return TRUE;
    }
    
    return FALSE;
    
} /* tree_view_key_event() */


/*
 * ctk_window_new() - create a new CtkWindow widget
 */

GtkWidget *ctk_window_new(NvCtrlAttributeHandle **handles, gint num_handles,
                          ParsedAttribute *p, ConfigProperties *conf)
{
    GObject *object;
    CtkWindow *ctk_window;
    GtkWidget *sw;
    GtkWidget *hpane;
    GtkWidget *frame;
    GtkWidget *vbox, *hbox;
    GtkWidget *widget;
    GtkWidget *button;
    GtkWidget *toggle_button;
    GtkWidget *statusbar;
    GtkWidget *eventbox;

    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTextTagTable *tag_table;

    GtkTextBuffer *help;
        
    CtkEvent *ctk_event;
    CtkConfig *ctk_config;
    
    gint column_offset, i;
    
    /* create the new object */

    object = g_object_new(CTK_TYPE_WINDOW, NULL);

    ctk_window = CTK_WINDOW(object);
    gtk_container_set_border_width(GTK_CONTAINER(ctk_window), CTK_WINDOW_PAD);

    ctk_window->handles = handles;
    ctk_window->num_handles = num_handles;
    ctk_window->attribute_list = p;
    
    /* create the config object */

    ctk_window->ctk_config = CTK_CONFIG(ctk_config_new(conf));
    ctk_config = ctk_window->ctk_config;
    
    /* create the quit dialog */

    ctk_window->quit_dialog = create_quit_dialog(ctk_window);


    /* pack the bottom row of the window */

    hbox = gtk_hbox_new(FALSE, 5);
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(ctk_window), vbox);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    
    /* place the status bar */
    
    statusbar = ctk_config_get_statusbar(ctk_config);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), statusbar);
    
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, TRUE, TRUE, 0);
    
    ctk_config_set_tooltip(ctk_config, eventbox, "The status bar displays "
                           "the most recent change that has been sent to the "
                           "X server.");
    
    /* create and place the help toggle button */
  
    toggle_button = gtk_toggle_button_new();

    g_object_set(G_OBJECT(toggle_button),
                 "label", GTK_STOCK_HELP,
                 "use_stock", GINT_TO_POINTER(TRUE), NULL);

    gtk_widget_set_size_request(toggle_button, 100, -1);

    gtk_box_pack_start(GTK_BOX(hbox), toggle_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(toggle_button), "toggled",
                     G_CALLBACK(help_button_toggled),
                     (gpointer) ctk_window);
    
    ctk_window->ctk_help = ctk_help_new(toggle_button);
    tag_table = CTK_HELP(ctk_window->ctk_help)->tag_table;
    
    ctk_config_set_tooltip(ctk_config, toggle_button, "The Help button "
                           "toggles the display of a help window which "
                           "provides a detailed explanation of the available "
                           "options in the current page.");

    /* create and place the close button */

    button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
    gtk_widget_set_size_request(button, 100, -1);

    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(close_button_clicked),
                     (gpointer) ctk_window);
    
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    
    ctk_config_set_tooltip(ctk_config, button, "The Quit button causes the "
                           "current settings to be saved to the configuration "
                           "file (~/.nvidia-settings-rc), and nvidia-settings "
                           "to exit.");
    
    /* create the horizontal pane */

    hpane = gtk_hpaned_new();
    gtk_box_pack_start(GTK_BOX(vbox), hpane, TRUE, TRUE, 0);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_paned_pack1(GTK_PANED(hpane), frame, FALSE, FALSE);

    /* scrollable window */
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    gtk_container_add(GTK_CONTAINER(frame), sw);

    /* create the tree model */

    ctk_window->tree_store = gtk_tree_store_new(CTK_WINDOW_NUM_COLUMNS,
                                                G_TYPE_STRING,
                                                G_TYPE_POINTER,
                                                G_TYPE_POINTER,
                                                G_TYPE_POINTER);
    model = GTK_TREE_MODEL(ctk_window->tree_store);

    /* create the tree view */

    ctk_window->treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(model));
    g_object_unref(ctk_window->tree_store);

    /* catch keyboard events to the tree view */

    g_signal_connect(ctk_window->treeview, "key_press_event",
                     G_CALLBACK(tree_view_key_event), GTK_OBJECT(ctk_window));

    selection = gtk_tree_view_get_selection(ctk_window->treeview);

    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    g_object_set(G_OBJECT(ctk_window->treeview), "headers-visible",
                 FALSE, NULL);

    /* create the one visible column in the tree view */

    renderer = gtk_cell_renderer_text_new();

    column_offset = gtk_tree_view_insert_column_with_attributes
        (ctk_window->treeview, -1, NULL, renderer, "text",
         CTK_WINDOW_LABEL_COLUMN, NULL);

    column = gtk_tree_view_get_column(ctk_window->treeview, column_offset - 1);
    gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column), TRUE);

    /* pack the treeview in the scrollable window */

    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(ctk_window->treeview));

    /* create the container widget for the "pages" */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_paned_pack2(GTK_PANED(hpane), hbox, TRUE, TRUE);

    ctk_window->page_viewer = hbox;
    ctk_window->page = NULL;

    /* add the per-screen entries into the tree model */

    for (i = 0; i < num_handles; i++) {

        GtkTreeIter iter;
        gchar *screen_name;
        GtkWidget *child;

        if (!handles[i]) continue;

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(handles[i]));
        
        screen_name = NvCtrlGetDisplayName(handles[i]);

        /* create the screen entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, screen_name, -1);

        /* device information */

        child = ctk_device_new(handles[i]);
        gtk_object_ref(GTK_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN, 
                           ctk_device_create_help(tag_table, screen_name), -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);

        /* color correction */

        child = ctk_color_correction_new(handles[i], ctk_config,
                                         ctk_window->attribute_list,
                                         ctk_event);
        if (child) {
            help = ctk_color_correction_create_help(tag_table);
            add_page(child, help, ctk_window, &iter,
                     "X Server Color Correction", NULL);
        }

        /* xvideo settings  */

        child = ctk_xvideo_new(handles[i], ctk_config);
        if (child) {
            help = ctk_xvideo_create_help(tag_table, CTK_XVIDEO(child));
            add_page(child, help, ctk_window, &iter,
                     "X Server XVideo Settings", NULL);
        }

        /* randr settings */

        child = ctk_randr_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_randr_create_help(tag_table, CTK_RANDR(child));
            add_page(child, help, ctk_window, &iter,
                     "Rotation Settings", NULL);
        }

        /* cursor shadow */

        child = ctk_cursor_shadow_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_cursor_shadow_create_help(tag_table,
                                                 CTK_CURSOR_SHADOW(child));
            add_page(child, help, ctk_window, &iter, "Cursor Shadow", NULL);
        }

        /* opengl settings */

        child = ctk_opengl_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_opengl_create_help(tag_table, CTK_OPENGL(child));
            add_page(child, help, ctk_window, &iter, "OpenGL Settings", NULL);
        }


        /* GLX Information */

        child = ctk_glx_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_glx_create_help(tag_table, CTK_GLX(child));
            add_page(child, help, ctk_window, &iter, "OpenGL/GLX Information", NULL);
        }


        /* multisample settings */

        child = ctk_multisample_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_multisample_create_help(tag_table,
                                               CTK_MULTISAMPLE(child));
            add_page(child, help, ctk_window, &iter,
                     "Antialiasing Settings", NULL);
        }

        /* thermal information */

        child = ctk_thermal_new(handles[i], ctk_config);
        if (child) {
            help = ctk_thermal_create_help(tag_table);
            add_page(child, help, ctk_window, &iter, "Thermal Monitor", NULL);
        }
        
        /* gvo (Graphics To Video Out) */
        
        child = ctk_gvo_new(handles[i], ctk_config);
        if (child) {
            help = ctk_gvo_create_help(tag_table);
            add_page(child, help, ctk_window, &iter,
                     "Graphics to Video Out", NULL);
        }

        /* display devices */
        
        child = ctk_display_device_new(handles[i], ctk_config, ctk_event);
        if (child) {
            help = ctk_display_device_create_help(tag_table,
                                                  CTK_DISPLAY_DEVICE(child));
            add_display_devices(child, help, ctk_window,
                                &iter, handles[i], ctk_event, tag_table);
        }
    }

    /*
     * add the framelock page, if any of the X screens support
     * framelock
     */

    for (i = 0; i < num_handles; i++) {
        if (!handles[i]) continue;
        
        widget = ctk_framelock_new(handles[i], GTK_WIDGET(ctk_window),
                                   ctk_config, ctk_window->attribute_list);
        if (!widget) continue;

        add_page(widget, ctk_framelock_create_help(tag_table),
                 ctk_window, NULL, "FrameLock",
                 ctk_framelock_config_file_attributes);
        break;
    }

    /* nvidia-settings configuration */

    add_page(GTK_WIDGET(ctk_window->ctk_config),
             ctk_config_create_help(tag_table),
             ctk_window, NULL, "nvidia-settings Configuration", NULL);

    /*
     * we're done with the current data in the parsed attribute list,
     * so clean it out
     */

    nv_parsed_attribute_clean(ctk_window->attribute_list);


    /*
     * now that everything is packed in the treeview, connect the
     * signal handler, and autosize the columns in the treeview
     */

    g_signal_connect(selection, "changed", G_CALLBACK(tree_selection_changed),
                     GTK_OBJECT(ctk_window));

    gtk_widget_show_all(GTK_WIDGET(ctk_window->treeview));
    gtk_tree_view_expand_all(ctk_window->treeview);
    gtk_tree_view_columns_autosize(ctk_window->treeview);

    /* set the window title */
    
    gtk_window_set_title(GTK_WINDOW(object), "NVIDIA X Server Settings");
    
    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);

} /* ctk_window_new() */



/*
 * add_page() - add a new page to ctk_window's tree_store, using iter
 * as a parent.
 */

static void add_page(GtkWidget *widget, GtkTextBuffer *help,
                     CtkWindow *ctk_window, GtkTreeIter *iter,
                     const gchar *label, config_file_attributes_func_t func)
{
    GtkTreeIter child_iter;

    if (!widget)
        return;

    gtk_object_ref(GTK_OBJECT(widget));
    gtk_tree_store_append(ctk_window->tree_store, &child_iter, iter);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_LABEL_COLUMN, label, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_WIDGET_COLUMN, widget, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_HELP_COLUMN, help, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                       func, -1);

} /* add_page() */



/*
 * create_quit_dialog() - create a dialog box to prompt the user
 * whether they really want to quit.
 */

static GtkWidget *create_quit_dialog(CtkWindow *ctk_window)
{
    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    GtkWidget *alignment;
    GtkWidget *label;

    dialog = gtk_dialog_new_with_buttons("Really quit?",
                                         GTK_WINDOW(ctk_window),
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT |
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_QUIT,
                                         GTK_RESPONSE_OK,
                                         NULL);

    g_signal_connect(GTK_OBJECT(dialog), "response",
                     G_CALLBACK(quit_response),
                     GTK_OBJECT(ctk_window));

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
    
    pixbuf = gtk_widget_render_icon(dialog, GTK_STOCK_DIALOG_QUESTION,
                                    GTK_ICON_SIZE_DIALOG, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);
    
    label = gtk_label_new("Do you really want to quit?");
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

    return dialog;

} /* create_quit_dialog() */


/*
 * save_settings_and_exit() - save settings, perform cleanups, if
 * necessary, and terminate nvidia-settings.
 */

static void save_settings_and_exit(CtkWindow *ctk_window)
{
    add_special_config_file_attributes(ctk_window);
    gtk_main_quit();
}


/*
 * quit_response() - handle the response from the "really quit?"
 * dialog.
 */

static void quit_response(GtkWidget *button, gint response, gpointer user_data)
{
    CtkWindow *ctk_window = CTK_WINDOW(user_data);

    if (response == GTK_RESPONSE_OK) {
        /* doesn't return */
        save_settings_and_exit(ctk_window);
    }

    gtk_widget_hide_all(ctk_window->quit_dialog);

} /* quit_response() */



/*
 * Ask all child widgets for any special attributes that should be
 * saved to the config file.
 */

static void add_special_config_file_attributes(CtkWindow *ctk_window)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_window->tree_store);
    GtkTreeIter   iter;
    gboolean valid;
    config_file_attributes_func_t func;
    GtkWidget *widget;
    
    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        
        gtk_tree_model_get(model, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, &widget,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           &func, -1);
        
        if (func) (*func)(widget, ctk_window->attribute_list);
        
        valid = gtk_tree_model_iter_next(model, &iter);
    }
} /* add_special_config_file_attributes() */



/*
 * add_display_devices() - add the display device pages
 */

static void add_display_devices(GtkWidget *widget, GtkTextBuffer *help,
                                CtkWindow *ctk_window, GtkTreeIter *iter,
                                NvCtrlAttributeHandle *handle,
                                CtkEvent *ctk_event,
                                GtkTextTagTable *tag_table)
{
    GtkTreeIter child_iter, grandchild_iter;
    ReturnStatus ret;
    int i, enabled, n, mask;
    char *name;
    
    if (!widget) {
        return;
    }
    
    /* retrieve the enabled display device mask */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ENABLED_DISPLAYS, &enabled);
    if (ret != NvCtrlSuccess) {
        return;
    }

    /* count how many display devices are enabled */

    for (i = 0, n = 0; i < 24; i++) {
        if (enabled & (1 << i)) n++;
    }
    if (n == 0) {
        return;
    } else if (n == 1) {
        name = "Display Device";
    } else {
        name = "Display Devices";
    }
    
    /*
     * add another reference to the widget, so that it doesn't get
     * destroyed the next time it gets hidden (removed from the page
     * viewer).
     */

    gtk_object_ref(GTK_OBJECT(widget));
    
    gtk_tree_store_append(ctk_window->tree_store, &child_iter, iter);
    
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_LABEL_COLUMN, name, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_WIDGET_COLUMN, widget, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_HELP_COLUMN, help, -1);
    gtk_tree_store_set(ctk_window->tree_store, &child_iter,
                       CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                       NULL, -1);

    /*
     * create pages for each of the display devices driven by this X
     * screen.
     */

    for (i = 0; i < 24; i++) {
        mask = (1 << i);
        if (!(mask & enabled)) continue;
        
        ret =
            NvCtrlGetStringDisplayAttribute(handle, mask,
                                            NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                            &name);
        
        if ((ret != NvCtrlSuccess) || (!name)) {
            name = "Unknown";
        }
        
        if (mask & CTK_DISPLAY_DEVICE_CRT_MASK) {
            
            widget = ctk_display_device_crt_new
                (handle, ctk_window->ctk_config, ctk_event, mask, name);
            
            help = ctk_display_device_crt_create_help
                (tag_table, CTK_DISPLAY_DEVICE_CRT(widget));
            
        } else if (mask & CTK_DISPLAY_DEVICE_TV_MASK) {
            
            widget = ctk_display_device_tv_new
                (handle, ctk_window->ctk_config, ctk_event, mask, name);
            
            help = ctk_display_device_tv_create_help
                (tag_table, CTK_DISPLAY_DEVICE_TV(widget));
            
        } else if (mask & CTK_DISPLAY_DEVICE_DFP_MASK) {
            
            widget = ctk_display_device_dfp_new
                (handle, ctk_window->ctk_config, ctk_event, mask, name);
            
            help = ctk_display_device_dfp_create_help
                (tag_table, CTK_DISPLAY_DEVICE_DFP(widget));
            
        } else {
            continue;
        }

        gtk_object_ref(GTK_OBJECT(widget));
        
        gtk_tree_store_append(ctk_window->tree_store, &grandchild_iter,
                              &child_iter);
    
        gtk_tree_store_set(ctk_window->tree_store, &grandchild_iter,
                           CTK_WINDOW_LABEL_COLUMN, name, -1);
        
        gtk_tree_store_set(ctk_window->tree_store, &grandchild_iter,
                           CTK_WINDOW_WIDGET_COLUMN, widget, -1);
    
        gtk_tree_store_set(ctk_window->tree_store, &grandchild_iter,
                           CTK_WINDOW_HELP_COLUMN, help, -1);
    
        gtk_tree_store_set(ctk_window->tree_store, &grandchild_iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);
    }
} /* add_display_devices() */

                         
