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
 * The CtkConfig widget controls configuration options of the control
 * panel itself (rather than configuration options of the NVIDIA X/GLX
 * driver).
 */

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkwindow.h"
#include "ctkutils.h"
#include "ctkbanner.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *__tooltip_help =
"When ToolTips are enabled, descriptions will be displayed next to options "
"when the mouse is held over them.";

static const char *__status_bar_help =
"The status bar in the bottom "
"left of the nvidia-settings GUI displays the most "
"recent change that has been sent to the X "
"server.  The 'Display Status Bar' check box "
"controls whether this status bar is displayed.";

static const char *__slider_text_entries_help =
"When the \"Slider Text Entries\" option is enabled, the current "
"value of an attribute controlled by a slider is "
"displayed and can be modified with a text entry "
"shown next to the slider.";

static const char *__x_display_names_help =
"When the current settings are saved to the "
"configuration file, the attributes can either be "
"qualified with just the screen to which the attribute "
"should be applied, or the attribute can be qualifed with "
"the entire X Display name.  If you want to be able to "
"use the same configuration file across multiple "
"computers, be sure to leave this option unchecked.  "
"It is normally recommended to leave this option "
"unchecked.";

static const char *__show_quit_dialog_help =
"When this option is enabled, nvidia-settings will ask if you "
"really want to quit when the quit button is pressed.";

static const char *__save_current_config_help =
"When nvidia-settings exits, it saves the current X server "
"configuration to a configuration file (\"~/.nvidia-settings-rc\", "
"by default).  Use this button to save the current X server "
"configuration immediately, optionally to a different file.";

static const char *__update_rules_on_profile_name_change_help =
"If this option is enabled, changing the name of a profile in the "
"Application Profile page of nvidia-settings will cause any rules "
"that refer to that profile to also be updated to refer to the new "
"profile name.";

static void ctk_config_class_init(CtkConfigClass *ctk_config_class);

static void display_status_bar_toggled(GtkWidget *, gpointer);
static void tooltips_toggled(GtkWidget *, gpointer);
static void slider_text_entries_toggled(GtkWidget *, gpointer);
static void display_name_toggled(GtkWidget *widget, gpointer user_data);
static void show_quit_dialog_toggled(GtkWidget *widget, gpointer user_data);
static void update_rules_on_profile_name_change_toggled(GtkWidget *widget,
                                                        gpointer user_data);

static void save_rc_clicked(GtkWidget *widget, gpointer user_data);

static GtkWidget *create_timer_list(CtkConfig *);

static guint signals[1];

GType ctk_config_get_type(
    void
)
{
    static GType ctk_config_type = 0;

    if (!ctk_config_type) {
        static const GTypeInfo ctk_config_info = {
            sizeof (CtkConfigClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_config_class_init, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkConfig),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_config_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkConfig", &ctk_config_info, 0);
    }

    return ctk_config_type;
}

static void config_finalize(GObject *object)
{
    CtkConfig *ctk_config = CTK_CONFIG(object);
    ctk_help_data_list_free_full(ctk_config->help_data);
}

static void ctk_config_class_init(CtkConfigClass *ctk_config_class)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(ctk_config_class);
    gobject_class->finalize = config_finalize;
    signals[0] = g_signal_new("slider_text_entry_toggled",                   
                              G_OBJECT_CLASS_TYPE(ctk_config_class),
                              G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
}

void ctk_statusbar_init(CtkStatusBar *status_bar)
{

    status_bar->widget = gtk_statusbar_new();
    status_bar->prev_message_id = 0;
    status_bar->enabled = TRUE;
    
    gtk_statusbar_set_has_resize_grip
        (GTK_STATUSBAR(status_bar->widget), FALSE);
    
    /* XXX force the status bar window to be vertically centered */

    gtk_misc_set_alignment
        (GTK_MISC(GTK_STATUSBAR(status_bar->widget)->label),
         0.0, 0.5);
}

GtkWidget* ctk_config_new(ConfigProperties *conf, CtrlHandles *pCtrlHandles)
{
    gint i;
    GObject *object;
    CtkConfig *ctk_config;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *banner;
    GtkWidget *label;
    GtkWidget *hseparator;
    GtkWidget *check_button;
    GtkWidget *alignment;
    gboolean b;

    struct {
        const char *label;
        unsigned int mask;
        GCallback toggled_callback;
        const char *help_text;
    } config_check_button_entries[] =
    {
        { 
           "Enable ToolTips",
           CONFIG_PROPERTIES_TOOLTIPS,
           G_CALLBACK(tooltips_toggled),
           __tooltip_help
         },
        {
            "Display Status Bar",
            CONFIG_PROPERTIES_DISPLAY_STATUS_BAR,
            G_CALLBACK(display_status_bar_toggled),
            __status_bar_help
        },
        {
            "Slider Text Entries",
            CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES,
            G_CALLBACK(slider_text_entries_toggled),
            __slider_text_entries_help
        },
        {
            "Include X Display Names in the Config File",
            CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE,
            G_CALLBACK(display_name_toggled),
            __x_display_names_help
        },
        {
            "Show \"Really Quit?\" Dialog",
            CONFIG_PROPERTIES_SHOW_QUIT_DIALOG,
            G_CALLBACK(show_quit_dialog_toggled),
            __show_quit_dialog_help
        },
        {
            "Update Rules when an Application Profile Name changes",
            CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE,
            G_CALLBACK(update_rules_on_profile_name_change_toggled),
            __update_rules_on_profile_name_change_help
        },

    };

    object = g_object_new(CTK_TYPE_CONFIG, NULL);

    ctk_config = CTK_CONFIG(object);

    ctk_config->conf = conf;
    ctk_config->pCtrlHandles = pCtrlHandles;

    gtk_box_set_spacing(GTK_BOX(ctk_config), 10);
    

    /* initialize the statusbar widget */
    ctk_statusbar_init(&ctk_config->status_bar);
    
    /* XXX force the status bar window to be vertically centered */

    gtk_misc_set_alignment
        (GTK_MISC(GTK_STATUSBAR(ctk_config->status_bar.widget)->label),
         0.0, 0.5);
    

    /* initialize the tooltips widget */

    ctk_config->tooltips.object = gtk_tooltips_new();

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_CONFIG);
    gtk_box_pack_start(GTK_BOX(ctk_config), banner, FALSE, FALSE, 0);
    
    /* "nvidia-settings Configuration" */

    hbox = gtk_hbox_new (FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_config), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("nvidia-settings Configuration");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);
    
    /* check buttons: Enable tooltips, Display statusbar, and Display
       slider text entries */

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(ctk_config), vbox, FALSE, FALSE, 0);

    ctk_config->help_data = NULL;

    for (i = 0; i < ARRAY_LEN(config_check_button_entries); i++) {
        label = gtk_label_new(config_check_button_entries[i].label);

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        b = !!(ctk_config->conf->booleans & config_check_button_entries[i].mask);
        if (config_check_button_entries[i].mask == CONFIG_PROPERTIES_TOOLTIPS) {
            if (b) {
                gtk_tooltips_enable(ctk_config->tooltips.object);
            } else {
                gtk_tooltips_disable(ctk_config->tooltips.object);
            }
        }
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), b);
        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);
        g_signal_connect(G_OBJECT(check_button), "toggled",
                         config_check_button_entries[i].toggled_callback,
                         ctk_config);
        ctk_config_set_tooltip_and_add_help_data(ctk_config,
                                                 check_button,
                                                 &ctk_config->help_data,
                                                 config_check_button_entries[i].label,
                                                 config_check_button_entries[i].help_text,
                                                 NULL);
    }

    ctk_config->help_data = g_list_reverse(ctk_config->help_data);
    
    /* timer list */
    
    ctk_config->timer_list_box = gtk_hbox_new(FALSE, 0);
    ctk_config->timer_list = create_timer_list(ctk_config);
    g_object_ref(ctk_config->timer_list);
    ctk_config->timer_list_visible = FALSE;

    gtk_box_pack_start(GTK_BOX(ctk_config), ctk_config->timer_list_box,
                       TRUE, TRUE, 0); 


    /* "Save Current Configuration" button */

    label = gtk_label_new("Save Current Configuration");
    hbox  = gtk_hbox_new(FALSE, 0);
    ctk_config->button_save_rc = gtk_button_new();
    alignment = gtk_alignment_new(1, 1, 0, 0);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_config->button_save_rc), hbox);
    gtk_container_add(GTK_CONTAINER(alignment), ctk_config->button_save_rc);
    gtk_box_pack_start(GTK_BOX(ctk_config), alignment, TRUE, TRUE, 0);

    /* Create the file selector for rc file */
    ctk_config->rc_file_selector =
        gtk_file_selection_new ("Please select a file to save to");

    g_signal_connect(G_OBJECT(ctk_config->button_save_rc), "clicked",
                     G_CALLBACK(save_rc_clicked),
                     (gpointer) ctk_config);

    gtk_file_selection_set_filename
        (GTK_FILE_SELECTION(ctk_config->rc_file_selector), DEFAULT_RC_FILE);

    ctk_config_set_tooltip(ctk_config, ctk_config->button_save_rc,
                           __save_current_config_help);

    gtk_widget_show_all(GTK_WIDGET(ctk_config));

    return GTK_WIDGET(ctk_config);
}

/*
 * save_rc_clicked() - called when "Save Current Configuration" button
 * is clicked.
 */

static void save_rc_clicked(GtkWidget *widget, gpointer user_data)
{
    gint result;
    const gchar *rc_filename = NULL;
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    CtkWindow *ctk_window =
        CTK_WINDOW(ctk_get_parent_window(GTK_WIDGET(ctk_config)));

    result = gtk_dialog_run(GTK_DIALOG(ctk_config->rc_file_selector));
    gtk_widget_hide(ctk_config->rc_file_selector);

    switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
        rc_filename = gtk_file_selection_get_filename
                          (GTK_FILE_SELECTION(ctk_config->rc_file_selector));
        break;
    default:
        return;
    }

    /* write the configuration file */
    add_special_config_file_attributes(ctk_window);
    nv_write_config_file(rc_filename, ctk_config->pCtrlHandles,
                         ctk_window->attribute_list, ctk_config->conf);
}

void ctk_statusbar_clear(CtkStatusBar *status_bar)
{
    if ((!status_bar->enabled) ||
        (!status_bar->widget)) {
        return;
    }

    if (status_bar->prev_message_id) {
        gtk_statusbar_remove(GTK_STATUSBAR(status_bar->widget),
                             1, status_bar->prev_message_id);
    }
}

void ctk_statusbar_message(CtkStatusBar *status_bar,
                           const gchar *str)
{

    if ((!status_bar->enabled) ||
        (!status_bar->widget)) {
        return;
    }

    if (status_bar->prev_message_id) {
        gtk_statusbar_remove(GTK_STATUSBAR(status_bar->widget),
                             1, status_bar->prev_message_id);
    }

    status_bar->prev_message_id =
        gtk_statusbar_push
        (GTK_STATUSBAR(status_bar->widget), 1, str);

} /* ctk_config_statusbar_message() */

void ctk_config_statusbar_message(CtkConfig *ctk_config,
                                  const char *fmt,
                                  ...)
{
    va_list ap;
    gchar *str;

    if ((!ctk_config) ||
        (!(ctk_config->conf->booleans &
           CONFIG_PROPERTIES_DISPLAY_STATUS_BAR))) {
        return;
    }

    va_start(ap, fmt);
    str = g_strdup_vprintf(fmt, ap);
    va_end(ap);

    ctk_statusbar_message(&ctk_config->status_bar,
                          str);

    g_free(str);
}



GtkWidget* ctk_config_get_statusbar(CtkConfig *ctk_config)
{
    return ctk_config->status_bar.widget;
}



void ctk_config_set_tooltip(CtkConfig *ctk_config,
                            GtkWidget *widget,
                            const gchar *text)

{
    gtk_tooltips_set_tip(ctk_config->tooltips.object, widget, text, NULL);
}


static void display_status_bar_toggled(
    GtkWidget *widget,
    gpointer user_data
)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        gtk_widget_show(ctk_config->status_bar.widget);
        ctk_config->conf->booleans |= CONFIG_PROPERTIES_DISPLAY_STATUS_BAR;
        ctk_config_statusbar_message(ctk_config, "Status bar enabled.");
    } else {
        gtk_widget_hide(ctk_config->status_bar.widget);

        if (ctk_config->status_bar.prev_message_id) {
            gtk_statusbar_remove(GTK_STATUSBAR(ctk_config->status_bar.widget),
                                 1, ctk_config->status_bar.prev_message_id);
        }
    
        ctk_config->status_bar.prev_message_id = 0;

        ctk_config->conf->booleans &= ~CONFIG_PROPERTIES_DISPLAY_STATUS_BAR;
    }
}

static void tooltips_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (active) {
        gtk_tooltips_enable(ctk_config->tooltips.object);
        ctk_config->conf->booleans |= CONFIG_PROPERTIES_TOOLTIPS;
    } else {
        gtk_tooltips_disable(ctk_config->tooltips.object);
        ctk_config->conf->booleans &= ~CONFIG_PROPERTIES_TOOLTIPS;
    }

    ctk_config_statusbar_message(ctk_config, "Tooltips %s.", 
                                 active ? "enabled" : "disabled");
}


static void slider_text_entries_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (active) {
        ctk_config->conf->booleans |= CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES;
    } else {
        ctk_config->conf->booleans &= ~CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES;
    }

    ctk_config_statusbar_message(ctk_config, 
                                 "Slider text entries %s.",
                                 active ? "enabled" : "disabled");
    
    g_signal_emit(ctk_config, signals[0], 0);
}

static void display_name_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (active) {
        ctk_config->conf->booleans |=
            CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE;
    } else {
        ctk_config->conf->booleans &=
            ~CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE;
    }

    ctk_config_statusbar_message(ctk_config, 
                                 "Including X Display Names in Config File %s.",
                                 active ? "enabled" : "disabled");
}

static void show_quit_dialog_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (active) {
        ctk_config->conf->booleans |= CONFIG_PROPERTIES_SHOW_QUIT_DIALOG;
    } else {
        ctk_config->conf->booleans &= ~CONFIG_PROPERTIES_SHOW_QUIT_DIALOG;
    }

    ctk_config_statusbar_message(ctk_config, 
                                 "Quit confirmation dialog %s.",
                                 active ? "enabled" : "disabled");
}

static void update_rules_on_profile_name_change_toggled(GtkWidget *widget,
                                                        gpointer user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (active) {
        ctk_config->conf->booleans |=
            CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE;
    } else {
        ctk_config->conf->booleans &=
            ~CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE;
    }

    ctk_config_statusbar_message(ctk_config,
                                 "Updating rules when an application profile "
                                 "name changes is %s.",
                                 active ? "enabled" : "disabled");
}


gboolean ctk_config_slider_text_entry_shown(CtkConfig *ctk_config)
{
    return !!(ctk_config->conf->booleans &
              CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES);
}

GtkTextBuffer *ctk_config_create_help(CtkConfig *ctk_config, GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "nvidia-settings Configuration Help");

    ctk_help_data_list_print_sections(b, &i, ctk_config->help_data);
    
    ctk_help_heading(b, &i, "Active Timers");
    ctk_help_para(b, &i, "Some attributes are polled periodically "
                  "to ensure the reported values are up-to-date.  "
                  "Each row in the 'Active Timers' table reflects "
                  "the configuration of one of these timers and "
                  "controls how frequently, if at all, a given "
                  "attribute is polled.  The 'Description' field "
                  "describes the function of a timer, the 'Enabled' "
                  "field allows enabling/disabling it, the 'Time "
                  "Interval' field controls the delay between two "
                  "consecutive polls (in milliseconds).  The Active "
                  "Timers table is only visible when timers are active.");

    ctk_help_finish(b);

    return b;
    
} /* create_help() */

/****************************************************************************/

/* max time interval is 60 seconds, and min time interval is .1 seconds */

#define MAX_TIME_INTERVAL (60 * 1000)
#define MIN_TIME_INTERVAL (100)

static void enabled_renderer_func(GtkTreeViewColumn*, GtkCellRenderer*,
                                  GtkTreeModel*, GtkTreeIter*, gpointer);

static void description_renderer_func(GtkTreeViewColumn*, GtkCellRenderer*,
                                      GtkTreeModel*, GtkTreeIter*, gpointer);

static void time_interval_renderer_func(GtkTreeViewColumn*, GtkCellRenderer*,
                                        GtkTreeModel*, GtkTreeIter*, gpointer);

static void time_interval_edited(GtkCellRendererText*,
                                 const gchar*, const gchar*, gpointer);

static void timer_enable_toggled(GtkCellRendererToggle*, gchar*, gpointer);



enum {

    TIMER_CONFIG_COLUMN = 0,
    FUNCTION_COLUMN,
    DATA_COLUMN,
    HANDLE_COLUMN,
    OWNER_ENABLE_COLUMN,
    NUM_COLUMNS,
};

static GtkWidget *create_timer_list(CtkConfig *ctk_config)
{
    GtkTreeModel *model;
    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *sw;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *alignment;
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    ctk_config->list_store =
        gtk_list_store_new(NUM_COLUMNS,
                           G_TYPE_POINTER,  /* TIMER_CONFIG_COLUMN */
                           G_TYPE_POINTER,  /* FUNCTION_COLUMN */
                           G_TYPE_POINTER,  /* DATA_COLUMN */
                           G_TYPE_UINT,     /* HANDLE_COLUMN */
                           G_TYPE_BOOLEAN); /* OWNER_ENABLE_COLUMN */
    
    model = GTK_TREE_MODEL(ctk_config->list_store);
    
    treeview = gtk_tree_view_new_with_model(model);
    
    g_object_unref(ctk_config->list_store);

    /* Enable */

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(timer_enable_toggled), ctk_config);
    column = gtk_tree_view_column_new_with_attributes("Enabled", renderer,
                                                      NULL);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_resizable(column, FALSE);

    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            enabled_renderer_func,
                                            GINT_TO_POINTER
                                            (TIMER_CONFIG_COLUMN),
                                            NULL);

    /* Description */
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Description",
                                                      renderer,
                                                      NULL);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            description_renderer_func,
                                            GINT_TO_POINTER
                                            (TIMER_CONFIG_COLUMN),
                                            NULL);
    
    /* Time interval */

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Time Interval",
                                                      renderer,
                                                      NULL);

    g_signal_connect(renderer, "edited",
                     G_CALLBACK(time_interval_edited), ctk_config);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            time_interval_renderer_func,
                                            GINT_TO_POINTER
                                            (TIMER_CONFIG_COLUMN),
                                            NULL);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    gtk_tree_view_column_set_resizable(column, FALSE);


    gtk_container_add(GTK_CONTAINER(sw), treeview);

    vbox = gtk_vbox_new(FALSE, 5);
    
    label = gtk_label_new("Active Timers:");
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    
    /* create the tooltip for the treeview (can't do it per column) */
    
    ctk_config_set_tooltip(ctk_config, treeview,
                           "The Active Timers describe operations that "
                           "nvidia-settings will perform at regular "
                           "intervals.");
    
    return vbox;

} /* create_timer_list() */


static void enabled_renderer_func(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer   *cell,
                                  GtkTreeModel      *model,
                                  GtkTreeIter       *iter,
                                  gpointer           data)
{
    gint column;
    TimerConfigProperty *timer_config;
    gboolean value;

    column = GPOINTER_TO_INT(data);
    gtk_tree_model_get(model, iter, column, &timer_config, -1);

    value = timer_config->user_enabled;
    g_object_set(GTK_CELL_RENDERER(cell), "active", value, NULL);
}

static void description_renderer_func(GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer   *cell,
                                      GtkTreeModel      *model,
                                      GtkTreeIter       *iter,
                                      gpointer           data)
{
    gint column;
    TimerConfigProperty *timer_config;
    gchar *value;

    column = GPOINTER_TO_INT(data);
    gtk_tree_model_get(model, iter, column, &timer_config, -1);

    value = timer_config->description;
    g_object_set(GTK_CELL_RENDERER(cell), "text", value, NULL);
}

static void time_interval_renderer_func(GtkTreeViewColumn *tree_column,
                                        GtkCellRenderer   *cell,
                                        GtkTreeModel      *model,
                                        GtkTreeIter       *iter,
                                        gpointer           data)
{
    gint column = GPOINTER_TO_INT(data);
    TimerConfigProperty *timer_config;
    guint value;
    gchar str[32];
    
    gtk_tree_model_get(model, iter, column, &timer_config, -1);
    value = timer_config->interval;

    snprintf(str, 32, "%d ms", value);
    
    g_object_set(GTK_CELL_RENDERER(cell), "text", str, NULL);
    g_object_set(GTK_CELL_RENDERER(cell), "editable", TRUE, NULL);
}



static void time_interval_edited(GtkCellRendererText *cell,
                                 const gchar         *path_string,
                                 const gchar         *new_text,
                                 gpointer             user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_config->list_store);
    GtkTreePath *path;
    GtkTreeIter iter;
    guint handle;
    GSourceFunc function;
    gpointer data;
    guint interval;
    TimerConfigProperty *timer_config;
    gboolean owner_enabled;

    interval = strtol(new_text, (char **)NULL, 10);
    
    if ((interval == 0) ||
        (interval == UINT_MAX)) return;

    if (interval > MAX_TIME_INTERVAL) interval = MAX_TIME_INTERVAL;
    if (interval < MIN_TIME_INTERVAL) interval = MIN_TIME_INTERVAL;

    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    gtk_tree_model_get(model, &iter,
                       TIMER_CONFIG_COLUMN, &timer_config,
                       OWNER_ENABLE_COLUMN, &owner_enabled,
                       HANDLE_COLUMN, &handle,
                       FUNCTION_COLUMN, &function,
                       DATA_COLUMN, &data,
                       -1);

    timer_config->interval = interval;
    
    /* Restart the timer if it is already running */

    if (timer_config->user_enabled && owner_enabled) {
        
        g_source_remove(handle);
        
        handle = g_timeout_add(interval, function, data);
        
        gtk_list_store_set(ctk_config->list_store, &iter,
                           HANDLE_COLUMN, handle, -1);
    }
}
     
static void timer_enable_toggled(GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data)
{
    CtkConfig *ctk_config = CTK_CONFIG(user_data);
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_config->list_store);
    GtkTreePath *path;
    GtkTreeIter iter;
    guint handle;
    GSourceFunc function;
    gpointer data;
    TimerConfigProperty *timer_config;
    gboolean owner_enabled;
    
    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    gtk_tree_model_get(model, &iter,
                       TIMER_CONFIG_COLUMN, &timer_config,
                       OWNER_ENABLE_COLUMN, &owner_enabled,
                       HANDLE_COLUMN, &handle,
                       FUNCTION_COLUMN, &function,
                       DATA_COLUMN, &data,
                       -1);

    timer_config->user_enabled ^= 1;

    /* Start/stop the timer only when the owner widget has enabled it */

    if (owner_enabled) {
        if (timer_config->user_enabled) {
            handle = g_timeout_add(timer_config->interval, function, data);
            gtk_list_store_set(ctk_config->list_store, &iter,
                               HANDLE_COLUMN, handle, -1);
        } else {
            g_source_remove(handle);
        }
    }

    ctk_config_statusbar_message(ctk_config, "Timer \"%s\" %s.",
                                 timer_config->description,
                                 timer_config->user_enabled ? 
                                     "enabled" : "disabled");
}

void ctk_config_add_timer(CtkConfig *ctk_config,
                          guint interval,
                          gchar *descr,
                          GSourceFunc function,
                          gpointer data)
{
    GtkTreeIter iter;
    ConfigProperties *conf = ctk_config->conf;
    TimerConfigProperty *timer_config;

    if (strchr(descr, '_') || strchr(descr, ','))
        return;

    timer_config = conf->timers;
    while (timer_config != NULL) {
        if (strcmp(timer_config->description, descr) == 0)
            break;
        timer_config = timer_config->next;
    }

    if (timer_config == NULL) {
        timer_config = g_malloc(sizeof(TimerConfigProperty));
        if (timer_config == NULL)
            return;

        timer_config->description = g_strdup(descr);
        timer_config->user_enabled = TRUE;
        timer_config->interval = interval;

        timer_config->next = conf->timers;
        conf->timers = timer_config;
    }

    /* Timer defaults to user enabled/owner disabled */

    gtk_list_store_append(ctk_config->list_store, &iter);
    gtk_list_store_set(ctk_config->list_store, &iter,
                       TIMER_CONFIG_COLUMN, timer_config,
                       OWNER_ENABLE_COLUMN, FALSE,
                       FUNCTION_COLUMN, function,
                       DATA_COLUMN, data, -1);

    /* make the timer list visible if it is not */

    if (!ctk_config->timer_list_visible) {
        gtk_box_pack_start(GTK_BOX(ctk_config->timer_list_box),
                           ctk_config->timer_list,
                           TRUE, TRUE, 0); 
        gtk_widget_show_all(ctk_config->timer_list_box);
        ctk_config->timer_list_visible = TRUE;
    }
}

void ctk_config_remove_timer(CtkConfig *ctk_config, GSourceFunc function)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GSourceFunc func;
    gboolean valid;
    guint handle;
    TimerConfigProperty *timer_config;
    gboolean owner_enabled;
    
    model = GTK_TREE_MODEL(ctk_config->list_store);

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter,
                           TIMER_CONFIG_COLUMN, &timer_config,
                           FUNCTION_COLUMN, &func,
                           OWNER_ENABLE_COLUMN, &owner_enabled,
                           HANDLE_COLUMN, &handle, -1);
        if (func == function) {

            /* Remove the timer if it was running */

            if (timer_config->user_enabled && owner_enabled) {
                g_source_remove(handle);
            }

            gtk_list_store_remove(ctk_config->list_store, &iter);
            break;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    /* if there are no more entries, hide the timer list */

    valid = gtk_tree_model_get_iter_first(model, &iter);
    if (!valid) {
        gtk_container_remove(GTK_CONTAINER(ctk_config->timer_list_box),
                             ctk_config->timer_list);
        ctk_config->timer_list_visible = FALSE;
    }
}

void ctk_config_start_timer(CtkConfig *ctk_config, GSourceFunc function, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GSourceFunc func;
    gboolean valid;
    guint handle;
    TimerConfigProperty *timer_config;
    gboolean owner_enabled;
    gpointer model_data;

    model = GTK_TREE_MODEL(ctk_config->list_store);

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter,
                           TIMER_CONFIG_COLUMN, &timer_config,
                           OWNER_ENABLE_COLUMN, &owner_enabled,
                           HANDLE_COLUMN, &handle,
                           FUNCTION_COLUMN, &func,
                           DATA_COLUMN, &model_data,
                           -1);
        if ((func == function) && (model_data == data)) {

            /* Start the timer if is enabled by the user and
               it is not already running. */
            
            if (timer_config->user_enabled && !owner_enabled) {
                handle = g_timeout_add(timer_config->interval, function, data);
                gtk_list_store_set(ctk_config->list_store, &iter,
                                   HANDLE_COLUMN, handle, -1);
            }
            gtk_list_store_set(ctk_config->list_store, &iter,
                               OWNER_ENABLE_COLUMN, TRUE, -1);
            break;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }
}

void ctk_config_stop_timer(CtkConfig *ctk_config, GSourceFunc function, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GSourceFunc func;
    gboolean valid;
    guint handle;
    TimerConfigProperty *timer_config;
    gboolean owner_enabled;
    gpointer model_data;

    model = GTK_TREE_MODEL(ctk_config->list_store);

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter,
                           TIMER_CONFIG_COLUMN, &timer_config,
                           OWNER_ENABLE_COLUMN, &owner_enabled,
                           HANDLE_COLUMN, &handle,
                           FUNCTION_COLUMN, &func,
                           DATA_COLUMN, &model_data,
                           -1);
        if ((func == function) && (model_data == data)) {

            /* Remove the timer if was running. */

            if (timer_config->user_enabled && owner_enabled) {
                g_source_remove(handle);
            }
            gtk_list_store_set(ctk_config->list_store, &iter,
                               OWNER_ENABLE_COLUMN, FALSE, -1);
            break;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }
}

/*
 * Helper function to add a tooltip to a widget *and* append a section to the
 * help text for that widget, for pages which use CtkHelpDataItem lists
 */
void ctk_config_set_tooltip_and_add_help_data(CtkConfig *config,
                                              GtkWidget *widget,
                                              GList **help_data_list,
                                              const gchar *label,
                                              const gchar *help_text,
                                              const gchar *extended_help_text)
{
    ctk_help_data_list_prepend(help_data_list, label, help_text, extended_help_text); 
    ctk_config_set_tooltip(config, widget, help_text);
}
