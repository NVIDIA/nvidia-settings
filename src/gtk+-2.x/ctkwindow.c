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

/*
 * ctkwindow.c - implementation of the CtkWindow widget; this widget
 * displays a treeview on the left side, and one of several children
 * widgets on the right side, depending on which item in the treeview
 * is selected.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctkwindow.h"

#include "ctkframelock.h"
#include "ctkconfig.h"
#include "ctkutils.h"

#include "ctkscreen.h"
#include "ctkslimm.h"
#include "ctkgpu.h"
#include "ctkcolorcorrection.h"
#include "ctkcolorcorrectionpage.h"
#include "ctkxvideo.h"
#include "ctkopengl.h"
#include "ctkglx.h"
#include "ctkmultisample.h"
#include "ctkthermal.h"
#include "ctkpowermizer.h"
#include "ctk3dvisionpro.h"

#include "ctkdisplaydevice.h"

#include "ctkdisplayconfig.h"
#include "ctkserver.h"
#include "ctkecc.h"
#include "ctkgridlicense.h"
#include "ctkvdpau.h"

#include "ctkappprofile.h"

#include "ctkhelp.h"
#include "ctkevent.h"
#include "ctkconstants.h"

#include "msg.h"
#include "common-utils.h"
#include "query-assign.h"

#include "opengl_loading.h"

#include "ctkpowermode.h"

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
    CtrlTarget *gpu_target;
    GtkTextTagTable *tag_table;

    GtkTreeIter parent_iter;

    GtkTreeIter *display_iters;
    CtkEvent **display_events;
    int num_displays;

} UpdateDisplaysData;


typedef void (*config_file_attributes_func_t)(GtkWidget *, ParsedAttribute *);
typedef void (*select_widget_func_t)(GtkWidget *);
typedef void (*unselect_widget_func_t)(GtkWidget *);

static void ctk_window_class_init(CtkWindowClass *, gpointer);

#ifdef CTK_GTK3
static void ctk_window_real_destroy(GtkWidget *);
#else
static void ctk_window_real_destroy(GtkObject *);
#endif

static void add_page(GtkWidget *, GtkTextBuffer *, CtkWindow *,
                     GtkTreeIter *, GtkTreeIter *, const gchar *,
                     config_file_attributes_func_t func,
                     select_widget_func_t load_func,
                     unselect_widget_func_t unload_func);

static GtkWidget *create_quit_dialog(CtkWindow *ctk_window);

static void quit_response(GtkWidget *, gint, gpointer);
static void save_settings_and_exit(CtkWindow *);

static void add_display_devices(CtkWindow *ctk_window, GtkTreeIter *iter,
                                CtrlTarget *gpu_target,
                                CtkEvent *ctk_event,
                                GtkTextTagTable *tag_table,
                                UpdateDisplaysData *data,
                                ParsedAttribute *p);

static void update_display_devices(GtkWidget *object, CtrlEvent *event,
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
            NULL                /* value_table */
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

static void ctk_window_class_init(CtkWindowClass *ctk_window_class,
                                  gpointer class_data)
{
#ifdef CTK_GTK3
    GtkWidgetClass *widget_class;

    widget_class = (GtkWidgetClass *) ctk_window_class;

    parent_class = g_type_class_peek_parent(ctk_window_class);

    widget_class->destroy = ctk_window_real_destroy;
#else
    GtkObjectClass *object_class;

    object_class = (GtkObjectClass *) ctk_window_class;

    parent_class = g_type_class_peek_parent(ctk_window_class);

    object_class->destroy = ctk_window_real_destroy;
#endif
} /* ctk_window_class_init() */



/*
 * ctk_window_real_destroy() - quit gtk.  XXX Maybe we should write
 * the configuration file here?
 */

#ifdef CTK_GTK3
static void ctk_window_real_destroy(GtkWidget *object)
{
    GTK_WIDGET_CLASS(parent_class)->destroy(object);
    gtk_main_quit();

} /* ctk_window_real_destroy() */
#else
static void ctk_window_real_destroy(GtkObject *object)
{
    GTK_OBJECT_CLASS(parent_class)->destroy(object);
    gtk_main_quit();

} /* ctk_window_real_destroy() */
#endif



/*
 * confirm_quit_and_save() - Shows the user the quit dialog if there are
 * pending changes and saves configuration information before exiting.
 */

static void confirm_quit_and_save(CtkWindow *ctk_window)
{
    CtkConfig *ctk_config = ctk_window->ctk_config;

    /* Ask for confirmation if there are pending changes. */
    if (ctk_config->pending_config) {
        const char *beg = "You have pending changes on following page(s):\n\n";
        const char *end = "Do you really want to quit?";
        const char *prefix;
        char *pages, *tmp;

        pages = strdup("");
        if (ctk_config->pending_config &
            CTK_CONFIG_PENDING_APPLY_DISPLAY_CONFIG) {
            prefix = (strlen(pages) == 0) ? "" : ",\n";
            tmp = nvstrcat(pages, prefix,
                           "X Server Display Configuration - Apply", NULL);
            free(pages);
            pages = tmp;
        }
        if (ctk_config->pending_config &
            CTK_CONFIG_PENDING_WRITE_DISPLAY_CONFIG) {
            prefix = (strlen(pages) == 0) ? "" : ",\n";
            tmp = nvstrcat(pages, prefix,
                           "X Server Display Configuration - "
                           "Save to X Configuration File", NULL);
            free(pages);
            pages = tmp;
        }
        if (ctk_config->pending_config &
            CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG) {
            prefix = (strlen(pages) == 0) ? "" : ",\n";
            tmp = nvstrcat(pages, prefix,
                           "SLI Mosaic Mode Settings - "
                           "Save to X Configuration File", NULL);
            free(pages);
            pages = tmp;
        }
        if (ctk_config->pending_config &
            CTK_CONFIG_PENDING_WRITE_APP_PROFILES) {
            prefix = (strlen(pages) == 0) ? "" : ",\n";
            tmp = nvstrcat(pages, prefix,
                           "Application Profiles - Save Changes", NULL);
            free(pages);
            pages = tmp;
        }

        if (pages[0] != '\0') {
            tmp = nvstrcat(beg, pages, "\n\n", end, NULL);
            gtk_label_set_text(
                GTK_LABEL(ctk_window->quit_dialog_pending_label),
                tmp);
        } else {
            gtk_label_set_text(
                GTK_LABEL(ctk_window->quit_dialog_pending_label),
                "You have pending changes.\n\n"
                "Do you really want to quit?");
        }
        gtk_widget_show_all(ctk_window->quit_dialog);
        return;
    }

    /* doesn't return */
    save_settings_and_exit(ctk_window);
}



/*
 * close_button_clicked() - called when the "Quit" button is clicked.
 */

static void close_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkWindow *ctk_window = CTK_WINDOW(user_data);

    confirm_quit_and_save(ctk_window);
}



static gboolean ctk_window_delete_event(GObject *object)
{
    CtkWindow *ctk_window = CTK_WINDOW(object);

    confirm_quit_and_save(ctk_window);

    /* gtk_main_quit() will be called above if the user really wants to quit,
     * so halt the progress of the delete-event here so we don't exit
     * prematurely.
     */
    return TRUE;
}



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
        gtk_widget_hide(ctk_window->ctk_help);
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
    GtkWidget *widget;
    GtkTextBuffer *help;

    select_widget_func_t select_func;
    unselect_widget_func_t unselect_func;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
        return;

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
 * row_activated_event() - callback for row-activated event
 * - handles key presses automatically
 * - allows the mouse to collapse/expand the menu even when the
 *   expand button/triangle is not working.
 */

static void row_activated_event(GtkTreeView        *view,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            user_data)
{
    CtkWindow *ctk_window = CTK_WINDOW(user_data);
    
    if (gtk_tree_view_row_expanded(ctk_window->treeview, path)) {
        gtk_tree_view_collapse_row(ctk_window->treeview, path);
    } else {
        gtk_tree_view_expand_row(ctk_window->treeview, path, FALSE);
    }
}



static gboolean has_randr_gamma(CtrlTarget *target)
{
    ReturnStatus ret;
    int val;

    ret = NvCtrlGetAttribute(target, NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE,
                             &val);

    return ((ret == NvCtrlSuccess) && (val == 1));
}

/*
 * ctk_window_new() - create a new CtkWindow widget
 */

GtkWidget *ctk_window_new(ParsedAttribute *p, ConfigProperties *conf,
                          CtrlSystem *system)
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
    GtkWidget *label;
    GtkRequisition req;
    gint width;

    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTextTagTable *tag_table;

    GtkTextBuffer *help;

    CtrlTargetNode *node;
    CtrlTarget *default_x_target = NULL;
    CtrlTarget *default_gpu_target = NULL;
    CtrlTarget *ctrl_target;

    CtkEvent *ctk_event;
    CtkConfig *ctk_config;

    gint column_offset;

    /* create the new object */

    object = g_object_new(CTK_TYPE_WINDOW, NULL);

    ctk_window = CTK_WINDOW(object);
    gtk_container_set_border_width(GTK_CONTAINER(ctk_window), CTK_WINDOW_PAD);

    ctk_window->attribute_list = p;
    
    /* create the config object */

    ctk_window->ctk_config = CTK_CONFIG(ctk_config_new(conf, system));
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

    /* Added row activated event to the tree view */

    g_signal_connect(ctk_window->treeview, "row_activated",
                     G_CALLBACK(row_activated_event), GTK_WIDGET(ctk_window));

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
    gtk_paned_pack2(GTK_PANED(hpane), hbox, TRUE, FALSE);

    ctk_window->page_viewer = hbox;
    ctk_window->page = NULL;

    /*
     * Create generic and specific default system targets. X Screen target
     * will only exist if the X Server is available. In that case, the generic
     * default target will be the default gpu target instead.
     */
    default_x_target = NvCtrlGetDefaultTargetByType(system, X_SCREEN_TARGET);
    default_gpu_target = NvCtrlGetDefaultTargetByType(system, GPU_TARGET);

    ctrl_target = default_x_target;
    if (!ctrl_target) {
        ctrl_target = default_gpu_target;
    }

    /* System Information */

    if (ctrl_target) {

        GtkWidget *child;

        child = ctk_server_new(ctrl_target, ctk_config);
        add_page(child,
                 ctk_server_create_help(tag_table, CTK_SERVER(child)),
                 ctk_window, NULL, NULL, "System Information",
                 NULL, NULL, NULL);
    }

    /* X Server Display Configuration */

    if (default_x_target) {

        GtkWidget *child;

        child = ctk_display_config_new(default_x_target, ctk_config);
        if (child) {
            ctk_window->display_config_widget = child;
            add_page(child,
                     ctk_display_config_create_help(tag_table,
                                                    CTK_DISPLAY_CONFIG(child)),
                     ctk_window, NULL, NULL,
                     "X Server Display Configuration",
                     NULL, ctk_display_config_selected,
                     ctk_display_config_unselected);

        }
    }

    /* Platform Power Mode */

    ctk_event = CTK_EVENT(ctk_event_new(default_gpu_target));
    widget = ctk_powermode_new(default_gpu_target, ctk_config, ctk_event);
    if (widget) {
        help = ctk_powermode_create_help(tag_table, CTK_POWERMODE(widget));
        add_page(widget, help, ctk_window, NULL, NULL,
                 "Platform Power Mode",
                 NULL, ctk_powermode_start_timer, ctk_powermode_stop_timer);
    }

    /* add the per-screen entries into the tree model */

    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        gchar *screen_name;
        GtkWidget *child;
        CtrlTarget *screen_target = node->t;

        if (screen_target == NULL || screen_target->h == NULL) {
            continue;
        }

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(screen_target));

        screen_name = g_strdup_printf("X Screen %d",
                                      NvCtrlGetTargetId(screen_target));

        /* create the screen entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, screen_name, -1);

        /* Screen information */

        screen_name = NvCtrlGetDisplayName(screen_target);

        child = ctk_screen_new(screen_target, ctk_event);
        g_object_ref(G_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN,
                           ctk_screen_create_help(tag_table, CTK_SCREEN(child),
                                                  screen_name),
                           -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);

        /*
         * color correction, if RandR per-CRTC color correction is not
         * available
         */

        if (!has_randr_gamma(screen_target)) {

            child = ctk_color_correction_page_new(screen_target, ctk_config,
                                                  ctk_window->attribute_list,
                                                  ctk_event);
            if (child) {
                help = ctk_color_correction_page_create_help(tag_table);
                add_page(child, help, ctk_window, &iter, NULL,
                         "X Server Color Correction", NULL, NULL, NULL);
            }
        }

        /* xvideo settings  */

        child = ctk_xvideo_new(screen_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_xvideo_create_help(tag_table, CTK_XVIDEO(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "X Server XVideo Settings", NULL, NULL, NULL);
        }

        /* opengl settings */

        child = ctk_opengl_new(screen_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_opengl_create_help(tag_table, CTK_OPENGL(child));
            add_page(child, help, ctk_window, &iter, NULL, "OpenGL Settings",
                     NULL, NULL, NULL);
        }


        /* Graphics Information */

        child = ctk_glx_new(screen_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_glx_create_help(tag_table, CTK_GLX(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "Graphics Information", NULL, ctk_glx_probe_info, NULL);
        }


        /* multisample settings */

        child = ctk_multisample_new(screen_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_multisample_create_help(tag_table,
                                               CTK_MULTISAMPLE(child));
            add_page(child, help, ctk_window, &iter, NULL,
                     "Antialiasing Settings", NULL, NULL, NULL);
        }


        /* VDPAU Information */
        child = ctk_vdpau_new(screen_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_vdpau_create_help(tag_table, CTK_VDPAU(child));
            add_page(child, help, ctk_window, &iter, NULL, "VDPAU Information",
                     NULL, NULL, NULL);
        }
    }

    /* add the per-gpu entries into the tree model */

    for (node = system->targets[GPU_TARGET]; node; node = node->next) {

        gchar *gpu_name;
        GtkWidget *child;
        CtrlTarget *gpu_target = node->t;
        UpdateDisplaysData *data;


        if (gpu_target == NULL || gpu_target->h == NULL) {
            continue;
        }

        /* create the gpu entry name */

        gpu_name = create_gpu_name_string(gpu_target);
        if (!gpu_name) continue;

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(gpu_target));

        /* create the gpu entry */

        gtk_tree_store_append(ctk_window->tree_store, &iter, NULL);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_LABEL_COLUMN, gpu_name, -1);

        child = ctk_gpu_new(gpu_target, ctk_event, ctk_config);

        g_object_ref(G_OBJECT(child));
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_WIDGET_COLUMN, child, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_HELP_COLUMN, 
                           ctk_gpu_create_help(tag_table, CTK_GPU(child)), -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                           NULL, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_SELECT_WIDGET_FUNC_COLUMN,
                           ctk_gpu_page_select, -1);
        gtk_tree_store_set(ctk_window->tree_store, &iter,
                           CTK_WINDOW_UNSELECT_WIDGET_FUNC_COLUMN,
                           ctk_gpu_page_unselect, -1);

        /* thermal information */

        child = ctk_thermal_new(gpu_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_thermal_create_help(tag_table, CTK_THERMAL(child));
            add_page(child, help, ctk_window, &iter, NULL, "Thermal Settings",
                     NULL, ctk_thermal_start_timer, ctk_thermal_stop_timer);
        }

        /* Powermizer information */
        child = ctk_powermizer_new(gpu_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_powermizer_create_help(tag_table, CTK_POWERMIZER(child));
            add_page(child, help, ctk_window, &iter, NULL, "PowerMizer",
                     NULL, ctk_powermizer_start_timer, 
                     ctk_powermizer_stop_timer);
        }

        /* ECC Information */
        child = ctk_ecc_new(gpu_target, ctk_config, ctk_event);
        if (child) {
            help = ctk_ecc_create_help(tag_table, CTK_ECC(child));
            add_page(child, help, ctk_window, &iter, NULL, "ECC Settings",
                     NULL, ctk_ecc_start_timer, ctk_ecc_stop_timer);
        }

        /* display devices */
        data = calloc(1, sizeof(*data));
        data->window = ctk_window;
        data->gpu_target = gpu_target;
        data->parent_iter = iter;
        data->tag_table = tag_table;

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                         G_CALLBACK(update_display_devices),
                         (gpointer) data);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_MODE_SET_EVENT),
                         G_CALLBACK(update_display_devices),
                         (gpointer) data);

        add_display_devices(ctk_window, &iter, gpu_target, ctk_event, tag_table,
                            data, ctk_window->attribute_list);
    }

    /*
     * add the frame lock page, if any of the X screens support
     * frame lock
     */

    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        CtrlTarget *screen_target = node->t;

        if (screen_target == NULL || screen_target->h == NULL) {
            continue;
        }

        widget = ctk_framelock_new(screen_target, GTK_WIDGET(ctk_window),
                                   ctk_config, ctk_window->attribute_list);
        if (!widget) continue;

        add_page(widget, ctk_framelock_create_help(tag_table),
                 ctk_window, NULL, NULL, "Frame Lock",
                 ctk_framelock_config_file_attributes,
                 ctk_framelock_select,
                 ctk_framelock_unselect);
        break;
    }

    /* add NVIDIA 3D VisionPro dongle configuration page */

    for (node = system->targets[NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET];
         node;
         node = node->next) {
        CtrlTarget *svp_target = node->t;

        if (svp_target == NULL || svp_target->h == NULL) {
            continue;
        }

        /* create the object for receiving NV-CONTROL events */

        ctk_event = CTK_EVENT(ctk_event_new(svp_target));

        widget = ctk_3d_vision_pro_new(svp_target, ctk_config,
                                       ctk_window->attribute_list,
                                       ctk_event);
        if (!widget) continue;

        help = ctk_3d_vision_pro_create_help(tag_table);
        add_page(widget, help, ctk_window, NULL, NULL,
                 "NVIDIA 3D VisionPro", ctk_3d_vision_pro_config_file_attributes,
                 ctk_3d_vision_pro_select, ctk_3d_vision_pro_unselect);
    }

    /* app profile configuration */
    widget = ctk_app_profile_new(ctrl_target, ctk_config);
    if (widget) {
        add_page(widget, ctk_app_profile_create_help(CTK_APP_PROFILE(widget), tag_table),
                 ctk_window, NULL, NULL, "Application Profiles",
                 NULL, NULL, NULL);
    }

    /* Manage GRID License Information */
    for (node = system->targets[GPU_TARGET]; node; node = node->next) {
        widget = ctk_manage_grid_license_new(node->t, ctk_config);
        if (widget) {
            help = ctk_manage_grid_license_create_help(tag_table,
                                                       CTK_MANAGE_GRID_LICENSE(widget));
            add_page(widget, help, ctk_window, NULL, NULL, "Manage License",
                     NULL, ctk_manage_grid_license_start_timer,
                     ctk_manage_grid_license_stop_timer);
            break; /* only add the page once */
        }
    }

    /* nvidia-settings configuration */

    add_page(GTK_WIDGET(ctk_window->ctk_config),
             ctk_config_create_help(ctk_config, tag_table),
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
                     GTK_WIDGET(ctk_window));

    gtk_widget_show_all(GTK_WIDGET(ctk_window->treeview));
    gtk_tree_view_expand_all(ctk_window->treeview);
    gtk_tree_view_columns_autosize(ctk_window->treeview);

    /* Make sure the first item is selected */
    if ( gtk_tree_model_get_iter_first(model, &iter) ) {
        gtk_tree_selection_select_iter(selection, &iter);
    }


    /* set the window title */
    
    gtk_window_set_title(GTK_WINDOW(object), "NVIDIA Settings");
    
    gtk_widget_show_all(GTK_WIDGET(object));


    /* Set the minimum width of the tree view window to be something
     * reasonable.  To do that, check the size of a label widget
     * with a reasonably long string and limit the tree view scroll
     * window's initial width to not extent past this.
     */

    label = gtk_label_new("XXXXXX Server Display ConfigurationXXXX");
    gtk_widget_show(label);
    ctk_widget_get_preferred_size(label, &req);
    width = req.width;
    gtk_widget_destroy(label);

    /* Get the width of the tree view scroll window */
    ctk_widget_get_preferred_size(sw, &req);

    /* If the scroll window is too wide, make it slimmer and
     * allow users to scroll horizontally (also allow resizing).
     */
    if (width < req.width && width > 0) {
        gtk_widget_set_size_request(sw, width, -1);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);
    }

    /* Add callback when window destroyed */

    g_signal_connect(G_OBJECT(ctk_window), "delete-event",
                     G_CALLBACK(ctk_window_delete_event), (gpointer) ctk_window);
    
    return GTK_WIDGET(object);

} /* ctk_window_new() */



/*
 * Set the currently active page to be the page that matches the
 * specified label.
 *
 * Note that child pages of X screens and GPUs cannot be uniquely
 * identified by their label alone (e.g., each GPU has a "PowerMizer"
 * page).  To allow explicit control over which instance of a child
 * page is selected, the label can have the optional format
 *
 *  "[PARENT LABEL], [CHILD LABEL]"
 *
 * E.g.,
 *
 *  "GPU 0 - (GeForce 7600 GT), PowerMizer"
 */

typedef struct {
    CtkWindow *ctk_window;
    const gchar *label;
} ctk_window_set_active_page_args;

static gboolean ctk_window_set_active_page_callback(GtkTreeModel *model,
                                                    GtkTreePath *path,
                                                    GtkTreeIter *iter,
                                                    gpointer data)
{
    GtkTreeIter parent;
    gboolean valid;
    gchar *iter_label;
    gchar *parent_label = "no parent";
    gchar *qualified_label;
    ctk_window_set_active_page_args *args = data;

    /* get the parent's label, if there is a parent */

    valid = gtk_tree_model_iter_parent(model, &parent, iter);
    if (valid) {
        gtk_tree_model_get(model, &parent,
                           CTK_WINDOW_LABEL_COLUMN, &parent_label, -1);
    }

    gtk_tree_model_get(model, iter, CTK_WINDOW_LABEL_COLUMN, &iter_label, -1);

    qualified_label = nvstrcat(parent_label, ", ", iter_label, NULL);

    if ((strcmp(iter_label, args->label) == 0) ||
        (strcmp(qualified_label, args->label) == 0)) {
        GtkTreeSelection* selection;
        selection = gtk_tree_view_get_selection(args->ctk_window->treeview);
        gtk_tree_selection_select_iter(selection, iter);
        free(qualified_label);
        return TRUE; /* stop walking the tree */
    }

    free(qualified_label);

    return FALSE; /* keep walking the tree */
}


void ctk_window_set_active_page(CtkWindow *ctk_window, const gchar *label)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_window->tree_store);

    ctk_window_set_active_page_args args;

    if (!label) return;

    args.ctk_window = ctk_window;
    args.label = label;

    gtk_tree_model_foreach(model,
                           ctk_window_set_active_page_callback,
                           &args);

} /* ctk_window_set_active_page() */



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
     * Add a reference to the object and sink (remove) the floating (gtk)
     * reference.  This sink needs to happen before the page gets packed
     * to ensure that we become the proper owner of these widgets.  This way,
     * page will not be destroyed when they are hidden (removed from the page
     * viewer), and we can properly destroy them when needed (for example, the
     * display device pages are destroyed/recreated on hotplug events.)
     */

    g_object_ref(G_OBJECT(widget));
    ctk_g_object_ref_sink(G_OBJECT(widget));

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
                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_QUIT,
                                         GTK_RESPONSE_OK,
                                         NULL);

    g_signal_connect(GTK_WIDGET(dialog), "response",
                     G_CALLBACK(quit_response),
                     GTK_WIDGET(ctk_window));

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_container_add(GTK_CONTAINER(ctk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox);
    
    pixbuf = ctk_widget_render_icon(dialog, CTK_STOCK_DIALOG_QUESTION,
                                    GTK_ICON_SIZE_DIALOG, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);
    
    label = gtk_label_new("Do you really want to quit?");
    ctk_window->quit_dialog_pending_label = label;
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

    /* Prevent the dialog from being deleted when closed */

    g_signal_connect(G_OBJECT(dialog), "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete),
                     (gpointer) dialog);

    return dialog;

} /* create_quit_dialog() */


/*
 * save_settings_and_exit() - save settings, perform cleanups, if
 * necessary, and terminate nvidia-settings.
 */

static void save_settings_and_exit(CtkWindow *ctk_window)
{
#ifdef CTK_GTK3
    GdkDisplay *gdk_display;
    Display *display;
    GdkScreen *gdk_screen;

    if (dGL.glXMakeContextCurrent) {
        gdk_screen = gtk_window_get_screen(GTK_WINDOW(ctk_window));
        gdk_display = gdk_screen_get_display(gdk_screen);
        display = gdk_x11_display_get_xdisplay(gdk_display);
        dGL.glXMakeContextCurrent(display, 0, 0, 0);
    }

#endif
    if (ctk_window->display_config_widget) {
        CtkDisplayConfig *display_config =
            CTK_DISPLAY_CONFIG(ctk_window->display_config_widget);
        layout_free(display_config->layout);
    }

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

    gtk_widget_hide(ctk_window->quit_dialog);

} /* quit_response() */



/*
 * Ask all child widgets for any special attributes that should be
 * saved to the config file.
 */

static gboolean add_special_config_file_attributes_callback(GtkTreeModel *model,
                                                            GtkTreePath *path,
                                                            GtkTreeIter *iter,
                                                            gpointer data)
{
    GtkWidget *widget;
    config_file_attributes_func_t func;
    CtkWindow *ctk_window = data;

    gtk_tree_model_get(model, iter,
                       CTK_WINDOW_WIDGET_COLUMN, &widget,
                       CTK_WINDOW_CONFIG_FILE_ATTRIBUTES_FUNC_COLUMN,
                       &func, -1);

    if (func) (*func)(widget, ctk_window->attribute_list);

    return FALSE; /* keep iterating over nodes in the tree */
}


void add_special_config_file_attributes(CtkWindow *ctk_window)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_window->tree_store);

    gtk_tree_model_foreach(model,
                           add_special_config_file_attributes_callback,
                           ctk_window);

} /* add_special_config_file_attributes() */



/*
 * add_display_devices() - add the display device pages
 */

static void add_display_devices(CtkWindow *ctk_window, GtkTreeIter *iter,
                                CtrlTarget *gpu_target,
                                CtkEvent *ctk_event_gpu,
                                GtkTextTagTable *tag_table,
                                UpdateDisplaysData *data,
                                ParsedAttribute *p)
{
    GtkTextBuffer *help;
    ReturnStatus ret;
    int *pData = NULL;
    int len;
    int i;


    /* retrieve the list of connected display devices */

    ret = NvCtrlGetBinaryAttribute(gpu_target, 0,
                                   NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                                   (unsigned char **)(&pData), &len);
    if ((ret != NvCtrlSuccess) || (pData[0] <= 0)) {
        goto done;
    }

    nvfree(data->display_iters);
    nvfree(data->display_events);

    data->display_iters = calloc(pData[0], sizeof(GtkTreeIter));
    if (!data->display_iters) {
        goto done;
    }

    data->display_events = calloc(pData[0], sizeof(CtkEvent*));
    if (!data->display_events) {
        nvfree(data->display_iters);
        goto done;
    }

    data->num_displays = 0;


    /*
     * create pages for each of the display devices driven by this (gpu)
     * handle.
     */

    for (i = 0; i < pData[0]; i++) {
        int display_id = pData[i+1];
        char *logName;
        char *typeBaseName;
        char *randrName;
        GtkWidget *widget;
        gchar *title;
        CtkEvent *ctk_event;
        CtrlSystem *system = ctk_window->ctk_config->pCtrlSystem;
        CtrlTarget *target;

        /*
         * Get the ctrl handle that was passed into ctk_main so that updated
         * backend color slider values, cached in the handle itself, can be
         * saved to the RC file when the UI is closed.
         */

        target = NvCtrlGetTarget(system, DISPLAY_TARGET, display_id);
        if (!target) {
            target = nv_add_target(system, DISPLAY_TARGET, display_id);
            if (!target) {
                continue;
            }
        }

        /*
         * Rebuild Sub-systems of display handle
         */
        NvCtrlRebuildSubsystems(target, NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS);

        /* Query display's names */
        ret = NvCtrlGetStringAttribute(target,
                                       NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME,
                                       &typeBaseName);
        if (ret != NvCtrlSuccess) {
            continue;
        }
        ret = NvCtrlGetStringAttribute(target,
                                       NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                       &logName);
        if (ret != NvCtrlSuccess) {
            logName = NULL;
        }
        ret = NvCtrlGetStringAttribute(target,
                                       NV_CTRL_STRING_DISPLAY_NAME_RANDR,
                                       &randrName);
        if (ret != NvCtrlSuccess) {
            randrName = NULL;
        }

        if (!logName && !randrName) {
            title = g_strdup_printf("DPY-%d - (Unknown)", display_id);
        } else {
            title = g_strdup_printf("%s - (%s)", randrName, logName);
        }
        free(logName);
        free(randrName);

        /* Create the page for the display */
        ctk_event = CTK_EVENT(ctk_event_new(target));
        widget = ctk_display_device_new(target,
                                        ctk_window->ctk_config, ctk_event,
                                        ctk_event_gpu,
                                        title, typeBaseName, p);
        if (widget != NULL) {
            help = ctk_display_device_create_help(tag_table,
                                                  CTK_DISPLAY_DEVICE(widget));
            add_page(widget, help, ctk_window, iter,
                     &(data->display_iters[data->num_displays]), title,
                     NULL, NULL, NULL);
            data->display_events[data->num_displays] = ctk_event;
            data->num_displays++;
        }
        else {
            ctk_event_destroy(G_OBJECT(ctk_event));
        }

        free(typeBaseName);
        g_free(title);
    }

 done:
    free(pData);

} /* add_display_devices() */


/*
 * Select display page whose name is passed through 'name' parameter
 */
static void select_display_page(UpdateDisplaysData *data, gchar *name)
{
    CtkWindow *ctk_window = data->window;
    GtkTreeSelection *tree_selection =
        gtk_tree_view_get_selection(ctk_window->treeview);
    gint num_displays = data->num_displays;

    while (num_displays) {
        GtkTreeIter *iter = &(data->display_iters[num_displays -1]);
        CtkDisplayDevice *ctk_obj = NULL;
        GtkWidget *widget = NULL;

        gtk_tree_model_get(GTK_TREE_MODEL(ctk_window->tree_store), iter,
                           CTK_WINDOW_WIDGET_COLUMN, &widget, -1);

        ctk_obj = CTK_DISPLAY_DEVICE(widget);

        if (strcmp(name, ctk_obj->name) == 0) {
            gtk_tree_selection_select_iter(tree_selection, iter);
            break;
        }

        num_displays--;
    }

}

/*
 * update_display_devices() - Callback handler for the NV_CTRL_PROBE_DISPLAYS
 * NV-CONTROL event.  Updates the list of display devices connected to the
 * GPU for which the event happened.
 *
 */

static void update_display_devices(GtkWidget *object,
                                   CtrlEvent *event,
                                   gpointer user_data)
{
    UpdateDisplaysData *data = (UpdateDisplaysData *) user_data;

    CtkWindow *ctk_window = data->window;
    CtrlTarget *gpu_target = data->gpu_target;
    GtkTreeIter parent_iter = data->parent_iter;
    GtkTextTagTable *tag_table = data->tag_table;
    GtkTreePath* parent_path;
    gboolean parent_expanded;
    GtkTreeSelection *tree_selection =
        gtk_tree_view_get_selection(ctk_window->treeview);
    GtkWidget *widget;
    gchar *selected_display_name = NULL;


    /* Keep track if the parent row is expanded */
    parent_path =
        gtk_tree_model_get_path(GTK_TREE_MODEL(ctk_window->tree_store),
                                &parent_iter);
    parent_expanded = 
        gtk_tree_view_row_expanded(ctk_window->treeview, parent_path);


    /* Remove previous display devices */
    while (data->num_displays) {

        GtkTreeIter *iter = &(data->display_iters[data->num_displays -1]);
        gboolean is_selected = FALSE;

        /* Select the parent (GPU) iter if we're removing the selected page */
        if (gtk_tree_selection_iter_is_selected(tree_selection, iter)) {
            is_selected = TRUE;
            gtk_tree_selection_select_iter(tree_selection, &parent_iter);
        }

        /* Remove the entry */
        gtk_tree_model_get(GTK_TREE_MODEL(ctk_window->tree_store), iter,
                           CTK_WINDOW_WIDGET_COLUMN, &widget, -1);

        if (is_selected) {
            CtkDisplayDevice *ctk_obj = CTK_DISPLAY_DEVICE(widget);
            selected_display_name = g_strdup(ctk_obj->name);
        }

        gtk_tree_store_remove(ctk_window->tree_store, iter);

        /* unref the page so we don't leak memory */
        g_object_unref(G_OBJECT(widget));

        /* Destroy the display CtkEvent */
        ctk_event_destroy(G_OBJECT(
                            data->display_events[data->num_displays - 1]));

        data->num_displays--;
    }

    /* Add back all the connected display devices */

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_window->tree_store), &parent_iter,
                       CTK_WINDOW_WIDGET_COLUMN, &widget, -1);

    add_display_devices(ctk_window, &parent_iter, gpu_target,
                        CTK_GPU(widget)->ctk_event,
                        tag_table, data, ctk_window->attribute_list);

    /* Expand the GPU entry if it used to be */
    if (parent_expanded) {
        gtk_tree_view_expand_row(ctk_window->treeview, parent_path, TRUE);
    }

    if (selected_display_name) {
        select_display_page(data, selected_display_name);
        g_free(selected_display_name);
    }

} /* update_display_devices() */
