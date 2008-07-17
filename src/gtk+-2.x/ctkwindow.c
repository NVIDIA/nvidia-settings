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
#include <stdlib.h>

#include "ctkwindow.h"

#include "ctkframelock.h"
#include "ctkgvo.h"
#include "ctkgvo-csc.h"
#include "ctkconfig.h"

#include "ctkscreen.h"
#include "ctkgpu.h"
#include "ctkcolorcorrection.h"
#include "ctkxvideo.h"
#include "ctkrandr.h"
#include "ctkcursorshadow.h"
#include "ctkopengl.h"
#include "ctkglx.h"
#include "ctkmultisample.h"
#include "ctkthermal.h"
#include "ctkclocks.h"
#include "ctkvcsc.h"

#include "ctkdisplaydevice-crt.h"
#include "ctkdisplaydevice-tv.h"
#include "ctkdisplaydevice-dfp.h"

#include "ctkdisplayconfig.h"
#include "ctkserver.h"

#include "ctkhelp.h"
#include "ctkevent.h"
#include "ctkconstants.h"

/* column enumeration */

enum {
    CTK_WINDOW_LABEL_COLUMN = 0,
    CTK_WINDOW_WIDGET_COLUMN,
    CTK_WINDOW_HELP_COLUMN,
    CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
    CTK_WINDOW_SELECT_WIDGET_FUNC_COLUMN,
    CTK_WINDOW_UNSELECT_WIDGET_FUNC_COLUMN,
    CTK_WINDOW_NUM_COLUMNS
};


typedef struct {
    CtkWindow *window;
    CtkEvent *event;
    NvCtrlAttributeHandle *gpu_handle;
    GtkTextTagTable *tag_table;

    GtkTreeIter parent_iter;

    GtkTreeIter *display_iters;
    int num_displays;

} UpdateDisplaysData;


typedef void (*config_file_attributes_func_t)(GtkWidget *, ParsedAttribute *);
typedef void (*select_widget_func_t)(GtkWidget *);
typedef void (*unselect_widget_func_t)(GtkWidget *);

static void ctk_window_class_init(CtkWindowClass *);

static void ctk_window_real_destroy(GtkObject *);

static void add_page(GtkWidget *, GtkTextBuffer *, CtkWindow *,
                     GtkTreeIter *, GtkTreeIter *, const gchar *,
                     config_file_attributes_func_t func,
                     select_widget_func_t load_func,
                     unselect_widget_func_t unload_func);

static GtkWidget *create_quit_dialog(CtkWindow *ctk_window);

static void quit_response(GtkWidget *, gint, gpointer);
static void save_settings_and_exit(CtkWindow *);

static void add_special_config_file_attributes(CtkWindow *ctk_window);

static void add_display_devices(CtkWindow *ctk_window, GtkTreeIter *iter,
                                NvCtrlAttributeHandle *handle,
                                CtkEvent *ctk_event,
                                GtkTextTagTable *tag_table,
                                UpdateDisplaysData *data);

static void update_display_devices(GtkObject *object, gpointer arg1,
                                   gpointer user_data);


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
 * close_button_clicked() - called when the "Quit" button is clicked.
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

    if (enabled) {
        if (ctk_window->ctk_help == NULL) {
            ctk_window->ctk_help = ctk_help_new(GTK_WIDGET(button),
                    ctk_window->help_tag_table);
            ctk_help_set_page(CTK_HELP(ctk_window->ctk_help),
                    ctk_window->help_text_buffer);
        }
        gtk_widget_show_all(ctk_window->ctk_help);
    } else {
        gtk_widget_hide_all(ctk_window->ctk_help);
    }

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

    select_widget_func_t select_func;
    unselect_widget_func_t unselect_func;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, CTK_WINDOW_LABEL_COLUMN, &str, -1);
    gtk_tree_model_get(model, &iter, CTK_WINDOW_WIDGET_COLUMN, &widget, -1);
    gtk_tree_model_get(model, &iter, CTK_WINDOW_HELP_COLUMN, &help, -1);
    gtk_tree_model_get(model, &iter, CTK_WINDOW_SELECT_WIDGET_FUNC_COLUMN,
                       &select_func, -1);

    /*
     * remove the existing widget from the page viewer, if anything is
     * presently there
     */

    if (ctk_window->page) {
        gtk_container_remove(GTK_CONTAINER(ctk_window->page_viewer),
                             ctk_window->page);
        ctk_window->page = NULL;
    }

    /* Call the unselect func for the existing widget, if any */

    if (ctk_window->widget) {
        gtk_tree_model_get(model, &(ctk_window->iter),
                           CTK_WINDOW_UNSELECT_WIDGET_FUNC_COLUMN,
                           &unselect_func, -1);
        if (unselect_func) {
            (*unselect_func)(ctk_window->widget);
        }
    }

    /* Pack the new widget */

    if (widget) {
        ctk_window->page = widget;
        gtk_box_pack_start(GTK_BOX(ctk_window->page_viewer), widget,
                           TRUE, TRUE, 2);
    }

    /* Call the select func for the new widget */

    if (select_func) (*select_func)(widget);

    /* update the help page */

    if (ctk_window->ctk_help != NULL)
        ctk_help_set_page(CTK_HELP(ctk_window->ctk_help), help);
    ctk_window->help_text_buffer = help;

    /* Keep track of the selected widget */

    ctk_window->iter = iter;
    ctk_window->widget = widget;
    
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

GtkWidget *ctk_window_new(NvCtrlAttributeHandle **screen_handles,
                          gint num_screen_handles,
                          NvCtrlAttributeHandle **gpu_handles,
                          gint num_gpu_handles,
                          NvCtrlAttributeHandle **vcsc_handles,
                          gint num_vcsc_handles,
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
    GtkTreeIter iter;
    GtkTextTagTable *tag_table;

    GtkTextBuffer *help;
        
    CtkEvent *ctk_event;
    CtkConfig *ctk_config;
    
    gint column_offset, i;
    
    /* create the new object */

    object = g_object_new(CTK_TYPE_WINDOW, NULL);

    ctk_window = CTK_WINDOW(object);
    gtk_container_set_border_width(GTK_CONTAINER(ctk_window), CTK_WINDOW_PAD);

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

    ctk_window->ctk_help = NULL;
    tag_table = ctk_help_create_tag_table();
    ctk_window->help_tag_table = tag_table;
    
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

    ctk_window->tree_store =
        gtk_tree_store_new(CTK_WINDOW_NUM_COLUMNS,
                           G_TYPE_STRING,   /* Label */
                           G_TYPE_POINTER,  /* Main widget */
                           G_TYPE_POINTER,  /* Help widget */
                           G_TYPE_POINTER,  /* Config file attr func */
                           G_TYPE_POINTER,  /* Load widget func */
                           G_TYPE_POINTER); /* Unload widget func */
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


    /* X Server info & configuration */

    if (num_screen_handles) {

        NvCtrlAttributeHandle *screen_handle = NULL;
        GtkWidget *child;
        int i;

        /*
         * XXX For now, just use the first handle in the list
         *     to communicate with the X server for these two
         *     pages.
         */

        for (i = 0 ; i < num_screen_handles; i++) {
            if (screen_handles[i]) {
                screen_handle = screen_handles[i];
                break;
            }
        }
        if (screen_handle) {

            /* X Server information */
            
            child = ctk_server_new(screen_handle, ctk_config);
            add_page(child,
                     ctk_server_create_help(tag_table,
                                            CTK_SERVER(child)),
                     ctk_window, NULL, NULL, "X Server Information",
                     NULL, NULL, NULL);

            /* X Server Display Configuration */

            child = ctk_display_config_new(screen_handle, ctk_config);
            if (child) {
                add_page(child,
                         ctk_display_config_create_help(tag_table,
                                                        CTK_DISPLAY_CONFIG(child)),
                         ctk_window, NULL, NULL,
                         "X Server Display Configuration",
                         NULL, NULL, NULL);
            }
        }
    }


    /* add the per-screen entries into the tree model */

    for (i = 0; i < num_screen_handles; i++) {

        gchar *screen_name;
        GtkWidget *child;
        NvCtrlAttributeHandle *screen_handle = screen_handles[i];

        if (!screen_handle) continue;

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(screen_handle));
        
        screen_name = g_strdup_printf("X Screen %d",
                                      NvCtrlGetTargetId(screen_handle));

        /* create the screen entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, screen_name, -1);

        /* Screen information */

        screen_name = NvCtrlGetDisplayName(screen_handle);

        child = ctk_screen_new(screen_handle, ctk_event);
        gtk_object_ref(GTK_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN, 
                           ctk_screen_create_help(tag_table, screen_name), -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);

        /* color correction */

        child = ctk_color_correction_new(screen_handle, ctk_config,
                                         ctk_window->attribute_list,
                                         ctk_event);
        if (child) {
            help = ctk_color_correction_create_help(tag_table);
            add_page(child, help, ctk_window, &iter, NULL,
                     "X Server Color Correction", NULL, NULL, NULL);
        }

        /* xvideo settings  */

        child = ctk_xvideo_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_xvideo_create_help(tag_table, CTK_XVIDEO(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "X Server XVideo Settings", NULL, NULL, NULL);
        }

        /* randr settings */

        child = ctk_randr_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_randr_create_help(tag_table, CTK_RANDR(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "Rotation Settings", NULL, NULL, NULL);
        }

        /* cursor shadow */

        child = ctk_cursor_shadow_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_cursor_shadow_create_help(tag_table,
                                                 CTK_CURSOR_SHADOW(child));
            add_page(child, help, ctk_window, &iter, NULL, "Cursor Shadow",
                     NULL, NULL, NULL);
        }

        /* opengl settings */

        child = ctk_opengl_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_opengl_create_help(tag_table, CTK_OPENGL(child));
            add_page(child, help, ctk_window, &iter, NULL, "OpenGL Settings",
                     NULL, NULL, NULL);
        }


        /* GLX Information */

        child = ctk_glx_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_glx_create_help(tag_table, CTK_GLX(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "OpenGL/GLX Information", NULL, ctk_glx_probe_info, NULL);
        }


        /* multisample settings */

        child = ctk_multisample_new(screen_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_multisample_create_help(tag_table,
                                               CTK_MULTISAMPLE(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "Antialiasing Settings", NULL, NULL, NULL);
        }

        /* gvo (Graphics To Video Out) */
        
        child = ctk_gvo_new(screen_handle, GTK_WIDGET(ctk_window),
                            ctk_config, ctk_event);
        if (child) {
            GtkWidget *gvo_parent = child;
            GtkTreeIter child_iter;
            help = ctk_gvo_create_help(tag_table);
            add_page(child, help, ctk_window, &iter, &child_iter,
                     "Graphics to Video Out", NULL,
                     ctk_gvo_select, ctk_gvo_unselect);

            /* add GVO sub-pages */

            child = ctk_gvo_csc_new(screen_handle, ctk_config, ctk_event,
                                    CTK_GVO(gvo_parent));
            if (child) {
                add_page(child, NULL, ctk_window, &child_iter, NULL,
                         "Color Space Conversion", NULL,
                         ctk_gvo_csc_select, ctk_gvo_csc_unselect);
            }
        }
    }

    /* add the per-gpu entries into the tree model */

    for (i = 0; i < num_gpu_handles; i++) {
        
        gchar *gpu_product_name;
        gchar *gpu_name;
        GtkWidget *child;
        ReturnStatus ret;
        NvCtrlAttributeHandle *gpu_handle = gpu_handles[i];
        UpdateDisplaysData *data;


        if (!gpu_handle) continue;

        /* create the gpu entry name */

        ret = NvCtrlGetStringDisplayAttribute(gpu_handle, 0,
                                              NV_CTRL_STRING_PRODUCT_NAME,
                                              &gpu_product_name);
        if (ret == NvCtrlSuccess && gpu_product_name) {
            gpu_name = g_strdup_printf("GPU %d - (%s)",
                                       NvCtrlGetTargetId(gpu_handle),
                                       gpu_product_name);
        } else {
            gpu_name =  g_strdup_printf("GPU %d - (Unknown)",
                                        NvCtrlGetTargetId(gpu_handle));
        }
        if (!gpu_name) continue;

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(gpu_handle));
       
        /* create the gpu entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, gpu_name, -1);
        child = ctk_gpu_new(gpu_handle, screen_handles, ctk_event);
        gtk_object_ref(GTK_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN, 
                           ctk_gpu_create_help(tag_table), -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);

        /* thermal information */

        child = ctk_thermal_new(gpu_handle, ctk_config);
        if (child) {
            help = ctk_thermal_create_help(tag_table, CTK_THERMAL(child));
            add_page(child, help, ctk_window, &iter, NULL, "Thermal Monitor",
                     NULL, ctk_thermal_start_timer, ctk_thermal_stop_timer);
        }
        
        /* clocks (GPU overclocking) */

        child = ctk_clocks_new(gpu_handle, ctk_config, ctk_event);
        if (child) {
            help = ctk_clocks_create_help(tag_table, CTK_CLOCKS(child));
            add_page(child, help, ctk_window, &iter, NULL, "Clock Frequencies",
                     NULL, ctk_clocks_select, NULL);
        }

        /* display devices */
        data = (UpdateDisplaysData *)calloc(1, sizeof(UpdateDisplaysData));
        data->window = ctk_window;
        data->event = ctk_event;
        data->gpu_handle = gpu_handle;
        data->parent_iter = iter;
        data->tag_table = tag_table;
        
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                         G_CALLBACK(update_display_devices),
                         (gpointer) data);            
        
        add_display_devices(ctk_window, &iter, gpu_handle, ctk_event,
                            tag_table, data);
    }

    /* add the per-vcsc (e.g. Quadro Plex) entries into the tree model */

    for (i = 0; i < num_vcsc_handles; i++) {
        
        gchar *vcsc_product_name;
        gchar *vcsc_name;
        GtkWidget *child;
        ReturnStatus ret;
        NvCtrlAttributeHandle *vcsc_handle = vcsc_handles[i];

        if (!vcsc_handle) continue;

        /* create the vcsc entry name */

        ret = NvCtrlGetStringDisplayAttribute(vcsc_handle, 0,
                                              NV_CTRL_STRING_VCSC_PRODUCT_NAME,
                                              &vcsc_product_name);
        if (ret == NvCtrlSuccess && vcsc_product_name) {
            vcsc_name = g_strdup_printf("VCSC %d - (%s)",
                                        NvCtrlGetTargetId(vcsc_handle),
                                        vcsc_product_name);
        } else {
            vcsc_name =  g_strdup_printf("VCSC %d - (Unknown)",
                                        NvCtrlGetTargetId(vcsc_handle));
        }
        if (!vcsc_name) continue;
        
        /* create the object for receiving NV-CONTROL events */
        
        ctk_event = CTK_EVENT(ctk_event_new(vcsc_handle));
        
        /* create the vcsc entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, vcsc_name, -1);
        child = ctk_vcsc_new(vcsc_handle, ctk_config);
        gtk_object_ref(GTK_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN, 
                           ctk_gpu_create_help(tag_table), -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);
    }

    /*
     * add the frame lock page, if any of the X screens support
     * frame lock
     */

    for (i = 0; i < num_screen_handles; i++) {

        NvCtrlAttributeHandle *screen_handle = screen_handles[i];

        if (!screen_handle) continue;
        
        widget = ctk_framelock_new(screen_handle, GTK_WIDGET(ctk_window),
                                   ctk_config, ctk_window->attribute_list);
        if (!widget) continue;

        add_page(widget, ctk_framelock_create_help(tag_table),
                 ctk_window, NULL, NULL, "Frame Lock",
                 ctk_framelock_config_file_attributes,
                 ctk_framelock_select,
                 ctk_framelock_unselect);
        break;
    }

    /* nvidia-settings configuration */

    add_page(GTK_WIDGET(ctk_window->ctk_config),
             ctk_config_create_help(tag_table),
             ctk_window, NULL, NULL, "nvidia-settings Configuration",
             NULL, NULL, NULL);

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

    /* Make sure the first item is selected */
    if ( gtk_tree_model_get_iter_first(model, &iter) ) {
        gtk_tree_selection_select_iter(selection, &iter);
    }


    /* set the window title */
    
    gtk_window_set_title(GTK_WINDOW(object), "NVIDIA X Server Settings");
    
    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);

} /* ctk_window_new() */



/*
 * add_page() - add a new page to ctk_window's tree_store, using iter
 * as a parent; the new child iter is written in pchild_iter, if
 * provided.
 */

static void add_page(GtkWidget *widget, GtkTextBuffer *help,
                     CtkWindow *ctk_window, GtkTreeIter *iter,
                     GtkTreeIter *child_iter,
                     const gchar *label, config_file_attributes_func_t func,
                     select_widget_func_t select_func,
                     unselect_widget_func_t unselect_func)                     
{
    GtkTreeIter tmp_child_iter;

    if (!widget)
        return;

    if (!child_iter) child_iter = &tmp_child_iter;

    /*
     * add another reference to the widget, so that it doesn't get
     * destroyed the next time it gets hidden (removed from the page
     * viewer).
     */
    
    gtk_object_ref(GTK_OBJECT(widget));
    
    gtk_tree_store_append(ctk_window->tree_store, child_iter, iter);
    
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_LABEL_COLUMN, label, -1);
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_WIDGET_COLUMN, widget, -1);
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_HELP_COLUMN, help, -1);
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                       func, -1);
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_SELECT_WIDGET_FUNC_COLUMN,
                       select_func, -1);
    gtk_tree_store_set(ctk_window->tree_store, child_iter,
                       CTK_WINDOW_UNSELECT_WIDGET_FUNC_COLUMN,
                       unselect_func, -1);
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

static void add_display_devices(CtkWindow *ctk_window, GtkTreeIter *iter,
                                NvCtrlAttributeHandle *handle,
                                CtkEvent *ctk_event,
                                GtkTextTagTable *tag_table,
                                UpdateDisplaysData *data)
{
    GtkWidget *widget;
    GtkTextBuffer *help;
    ReturnStatus ret;
    int i, connected, n, mask;
    char *name;
    char *type;
    gchar *title;
    
    
    /* retrieve the connected display device mask */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_CONNECTED_DISPLAYS, &connected);
    if (ret != NvCtrlSuccess) {
        return;
    }

    /* count how many display devices are connected */

    for (i = 0, n = 0; i < 24; i++) {
        if (connected & (1 << i)) n++;
    }
    if (n == 0) {
        return;
    }
    

    if (data->display_iters) {
        free(data->display_iters);
    }
    data->display_iters =
        (GtkTreeIter *)calloc(1, n * sizeof(GtkTreeIter));
    data->num_displays = 0;
    if (!data->display_iters) return;


    /*
     * create pages for each of the display devices driven by this handle.
     */

    for (i = 0; i < 24; i++) {
        mask = (1 << i);
        if (!(mask & connected)) continue;

        type = display_device_mask_to_display_device_name(mask);
        
        ret =
            NvCtrlGetStringDisplayAttribute(handle, mask,
                                            NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                            &name);
        
        if ((ret != NvCtrlSuccess) || (!name)) {
            title = g_strdup_printf("%s - (Unknown)", type);
        } else {
            title = g_strdup_printf("%s - (%s)", type, name);
            XFree(name);
        }
        free(type);
        
        if (mask & CTK_DISPLAY_DEVICE_CRT_MASK) {
            
            widget = ctk_display_device_crt_new
                (handle, ctk_window->ctk_config, ctk_event, mask, title);
            
            help = ctk_display_device_crt_create_help
                (tag_table, CTK_DISPLAY_DEVICE_CRT(widget));
            
        } else if (mask & CTK_DISPLAY_DEVICE_TV_MASK) {
            
            widget = ctk_display_device_tv_new
                (handle, ctk_window->ctk_config, ctk_event, mask, title);
            
            help = ctk_display_device_tv_create_help
                (tag_table, CTK_DISPLAY_DEVICE_TV(widget));
            
        } else if (mask & CTK_DISPLAY_DEVICE_DFP_MASK) {
            
            widget = ctk_display_device_dfp_new
                (handle, ctk_window->ctk_config, ctk_event, mask, title);
            
            help = ctk_display_device_dfp_create_help
                (tag_table, CTK_DISPLAY_DEVICE_DFP(widget));
            
        } else {
            g_free(title);
            continue;
        }

        add_page(widget, help, ctk_window, iter,
                 &(data->display_iters[data->num_displays]), title,
                 NULL, NULL, NULL);
        g_free(title);
        data->num_displays++;
    }
} /* add_display_devices() */



/*
 * update_display_devices() - Callback handler for the NV_CTRL_PROBE_DISPLAYS
 * NV-CONTROL event.  Updates the list of display devices connected to the
 * GPU for which the event happened.
 *
 */

static void update_display_devices(GtkObject *object, gpointer arg1,
                                   gpointer user_data)
{
    UpdateDisplaysData *data = (UpdateDisplaysData *) user_data;

    CtkWindow *ctk_window = data->window;
    CtkEvent *ctk_event = data->event;
    NvCtrlAttributeHandle *gpu_handle = data->gpu_handle;
    GtkTreeIter parent_iter = data->parent_iter;
    GtkTextTagTable *tag_table = data->tag_table;
    GtkTreePath* parent_path;
    gboolean parent_expanded;
    GtkTreeSelection *tree_selection =
        gtk_tree_view_get_selection(ctk_window->treeview);


    /* Keep track if the parent row is expanded */
    parent_path =
        gtk_tree_model_get_path(GTK_TREE_MODEL(ctk_window->tree_store),
                                &parent_iter);
    parent_expanded = 
        gtk_tree_view_row_expanded(ctk_window->treeview, parent_path);


    /* Remove previous display devices */
    while (data->num_displays) {

        GtkTreeIter *iter = &(data->display_iters[data->num_displays -1]);
        GtkWidget *widget;

        /* Select the parent (GPU) iter if we're removing the selected page */
        if (gtk_tree_selection_iter_is_selected(tree_selection, iter)) {
            gtk_tree_selection_select_iter(tree_selection, &parent_iter);
        }

        /* unref the page so we don't leak memory */
        gtk_tree_model_get(GTK_TREE_MODEL(ctk_window->tree_store), iter,
                           CTK_WINDOW_WIDGET_COLUMN, &widget, -1);
        g_object_unref(widget);

        /* XXX Call a widget-specific cleanup function? */

        /* Remove the entry */
        gtk_tree_store_remove(ctk_window->tree_store, iter);

        data->num_displays--;
    }
    
    /* Add back all the connected display devices */
    add_display_devices(ctk_window, &parent_iter, gpu_handle, ctk_event,
                        tag_table, data);

    /* Expand the GPU entry if it used to be */
    if (parent_expanded) {
        gtk_tree_view_expand_row(ctk_window->treeview, parent_path, TRUE);
    }

} /* update_display_devices() */
