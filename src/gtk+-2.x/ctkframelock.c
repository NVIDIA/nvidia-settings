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
#include <NvCtrlAttributes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ctkframelock.h"
#include "ctkhelp.h"

#include "frame_lock_banner.h"
#include "led_green.h"
#include "led_red.h"
#include "led_grey.h"

#include "rj45_input.h"
#include "rj45_output.h"

#include "parse.h"
#include "msg.h"


#define DEFAULT_UPDATE_STATUS_TIME_INTERVAL 1000
#define DEFAULT_TEST_LINK_TIME_INTERVAL 2000
#define DEFAULT_CHECK_FOR_ETHERNET_TIME_INTERVAL 10000

#define POLARITY_RISING 0x1
#define POLARITY_FALLING 0x2
#define POLARITY_BOTH 0x3

/*
 * functions for the FrameLock Widget
 */

static void ctk_framelock_class_init(CtkFramelockClass *ctk_framelock_class);

static gpointer add_x_screen(CtkFramelock *, const gchar *, gboolean);
static GtkWidget *create_add_x_screen_dialog(CtkFramelock *ctk_framelock);
static GtkWidget *create_remove_x_screen_dialog(CtkFramelock *ctk_framelock);
static GtkWidget *create_error_msg_dialog(CtkFramelock *ctk_framelock);
static GtkWidget *create_sync_state_button(CtkFramelock *ctk_framelock);

static gboolean update_status(gpointer user_data);
static gboolean check_for_ethernet(gpointer user_data);

static void test_link(GtkWidget *button, CtkFramelock *ctk_framelock);
static gint test_link_done(gpointer data);

static void toggle_sync_state_button(GtkWidget *button,
                                     CtkFramelock *ctk_framelock);

static void show_remove_x_screen_dialog(GtkWidget *, CtkFramelock *);
static void error_msg(CtkFramelock *ctk_framelock, const gchar *fmt, ...);



static void create_list_store(CtkFramelock *ctk_framelock);
static void add_member_to_list_store(CtkFramelock *ctk_framelock,
                                     const gpointer handle);

static void apply_parsed_attribute_list(CtkFramelock *ctk_framelock,
                                        ParsedAttribute *p);

static GtkWidget *add_house_sync_controls(CtkFramelock *ctk_framelock);
static void update_house_sync_controls(CtkFramelock *ctk_framelock);

static void add_columns_to_treeview(CtkFramelock *ctk_framelock);

static void sync_interval_entry_activate(GtkEntry *, gpointer);
static void house_sync_format_entry_activate(GtkEditable *, gpointer);

static gboolean find_master(CtkFramelock *, GtkTreeIter *,
                            NvCtrlAttributeHandle **);

enum
{
    COLUMN_HANDLE,
    COLUMN_DISPLAY_MASK,
    COLUMN_DISPLAY_NAME,
    COLUMN_MASTER,
    COLUMN_STEREO_SYNC,
    COLUMN_TIMING,
    COLUMN_SYNC_READY,
    COLUMN_SYNC_RATE,
    COLUMN_HOUSE,
    COLUMN_RJ45_PORT0,
    COLUMN_RJ45_PORT1,
    COLUMN_POLARITY,
    COLUMN_SYNC_SKEW,
    COLUMN_SYNC_INTERVAL,
    COLUMN_HOUSE_FORMAT,
    NUM_COLUMNS
};




/*
 * helper functions for displaying the correct thing in the columns of
 * the tree view
 */

static void led_renderer_func       (GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data);

static void rj45_renderer_func      (GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data);

static void rate_renderer_func      (GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data);

static void polarity_renderer_func  (GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data);

static void sync_skew_renderer_func (GtkTreeViewColumn *tree_column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data);

/* callback functions */

static void master_toggled(GtkCellRendererToggle *cell, 
                           gchar                 *path_str,
                           gpointer               data);

static void rising_edge_toggled(GtkCellRendererToggle *cell,
                                gchar                 *path_string,
                                gpointer               user_data);

static void falling_edge_toggled(GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

static void sync_skew_edited(GtkCellRendererText *cell,
                             const gchar         *path_string,
                             const gchar         *new_text,
                             gpointer             data);

static GObjectClass *parent_class;



/*
 * ctk_framelock_get_type() - register the FrameLock class and
 * return the unique type id.
 */

GType ctk_framelock_get_type(
    void
)
{
    static GType ctk_framelock_type = 0;

    if (!ctk_framelock_type) {
        static const GTypeInfo ctk_framelock_info = {
            sizeof (CtkFramelockClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_framelock_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkFramelock),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };
        
        ctk_framelock_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkFramelock", &ctk_framelock_info, 0);
    }

    return ctk_framelock_type;

} /* ctk_framelock_get_type() */




/*
 * ctk_framelock_class_init() - initialize the object structure
 */

static void ctk_framelock_class_init(
    CtkFramelockClass *ctk_framelock_class
)
{
    GObjectClass *gobject_class;

    gobject_class = (GObjectClass *) ctk_framelock_class;
    parent_class = g_type_class_peek_parent(ctk_framelock_class);

} /* ctk_framelock_class_init() */



/*
 * ctk_framelock_new() - return a new instance of the FrameLock
 * class.
 */

GtkWidget* ctk_framelock_new(NvCtrlAttributeHandle *handle,
                             GtkWidget *parent_window, CtkConfig *ctk_config,
                             ParsedAttribute *p)
{
    GObject *object;
    CtkFramelock *ctk_framelock;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *label;
    GtkWidget *frame;
    GtkWidget *image;
    GtkWidget *sw;
    GtkWidget *hseparator;
    gint value;

    guint8 *image_buffer = NULL;
    const nv_image_t *img;

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /*
     * Only expose FrameLock if the current NV-CONTROL handle supports
     * FrameLock.  This isn't absolutely necessary, because the
     * FrameLock control page does not have to include the current
     * NV-CONTROL handle in the FrameLock Group.  However, we don't
     * want to expose the FrameLock page unconditionally (it would
     * only confuse most users), so this is as good a condition as
     * anything else.
     */
    
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK, &value);
    if (value != NV_CTRL_FRAMELOCK_SUPPORTED) return NULL;
    
    /* create a new instance of the object */
    
    object = g_object_new(CTK_TYPE_FRAMELOCK, NULL);
    
    ctk_framelock = CTK_FRAMELOCK(object);
    ctk_framelock->attribute_handle = handle;
    ctk_framelock->parent_window = GTK_WINDOW(parent_window);

    gtk_box_set_spacing(GTK_BOX(ctk_framelock), 10);

    /* banner */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
    
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    img = &frame_lock_banner_image;

    image_buffer = decompress_image_data(img);

    image = gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));

    gtk_container_add(GTK_CONTAINER(frame), image);

    /* scrollable list */

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                        GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_ALWAYS);
    gtk_box_pack_start(GTK_BOX(object), sw, TRUE, TRUE, 0);
    
    /* create the list store and treeview */

    create_list_store(ctk_framelock);

    /* plug the treeview into the scrollable window */
    
    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(ctk_framelock->treeview));
      
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(object), hseparator, FALSE, TRUE, 0);

    /* Sync Interval and House Sync Format controls */
    
    hbox = add_house_sync_controls(ctk_framelock);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, TRUE, 0);
    
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(object), hseparator, FALSE, TRUE, 0);

    /* create any needed dialog windows */

    ctk_framelock->add_x_screen_dialog =
        create_add_x_screen_dialog(ctk_framelock);

    ctk_framelock->remove_x_screen_dialog =
        create_remove_x_screen_dialog(ctk_framelock);

    ctk_framelock->error_msg_dialog =
        create_error_msg_dialog(ctk_framelock);

    /* create buttons */
        
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, TRUE, 0);
    
    /* "Add X Screen..." button */

    label = gtk_label_new("Add X Screen...");
    hbox2 = gtk_hbox_new(FALSE, 0);
    ctk_framelock->add_x_screen_button = gtk_button_new();
    
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_framelock->add_x_screen_button),
                      hbox2);
    
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->add_x_screen_button,
                       FALSE, TRUE, 0);
    
    g_signal_connect_swapped(G_OBJECT(ctk_framelock->add_x_screen_button),
                             "clicked", G_CALLBACK(gtk_widget_show_all),
                             (gpointer) ctk_framelock->add_x_screen_dialog);

    /* "Remove X Screen..." button */

    label = gtk_label_new("Remove X Screen...");
    hbox2 = gtk_hbox_new(FALSE, 0);
    ctk_framelock->remove_x_screen_button = gtk_button_new();

    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_framelock->remove_x_screen_button),
                      hbox2);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->remove_x_screen_button,
                       FALSE, TRUE, 0);
    gtk_widget_set_sensitive(ctk_framelock->remove_x_screen_button, FALSE);
    
    g_signal_connect(G_OBJECT(ctk_framelock->remove_x_screen_button),
                     "clicked", G_CALLBACK(show_remove_x_screen_dialog),
                     GTK_OBJECT(ctk_framelock));
    
    /* "Test Link" button */

    label = gtk_label_new("Test Link");
    hbox2 = gtk_hbox_new(FALSE, 0);
    ctk_framelock->test_link_button = gtk_toggle_button_new();
    
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_framelock->test_link_button), hbox2);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->test_link_button,
                       FALSE, TRUE, 0);
        
    gtk_widget_set_sensitive(ctk_framelock->test_link_button, FALSE);

    g_signal_connect(G_OBJECT(ctk_framelock->test_link_button), "toggled",
                     G_CALLBACK(test_link), GTK_OBJECT(ctk_framelock));
    
    /* Sync State button */

    ctk_framelock->sync_state_button = create_sync_state_button(ctk_framelock);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->sync_state_button,
                       FALSE, TRUE, 0);

    gtk_widget_set_sensitive(ctk_framelock->sync_state_button, FALSE);
    
    g_signal_connect(G_OBJECT(ctk_framelock->sync_state_button), "toggled",
                     G_CALLBACK(toggle_sync_state_button),
                     GTK_OBJECT(ctk_framelock));
    
    /* show the page */
    
    gtk_widget_show_all(GTK_WIDGET(object));
    
    /* register a timer callback to update the status of the page */

    ctk_config_add_timer(ctk_config, DEFAULT_UPDATE_STATUS_TIME_INTERVAL,
                         "FrameLock Connection Status",
                         (GSourceFunc) update_status,
                         (gpointer) ctk_framelock);
    
    /* register a timer callback to check the rj45 ports */

    ctk_config_add_timer(ctk_config, DEFAULT_CHECK_FOR_ETHERNET_TIME_INTERVAL,
                         "FrameLock RJ45 Check",
                         (GSourceFunc) check_for_ethernet,
                         (gpointer) ctk_framelock);

    ctk_framelock->ctk_config = ctk_config;

    /* create the watch cursor */

    ctk_framelock->wait_cursor = gdk_cursor_new(GDK_WATCH);
    
    /* apply the parsed attribute list */

    apply_parsed_attribute_list(ctk_framelock, p);
    
    return GTK_WIDGET(object);
    
} /* ctk_framelock_new() */



/**************************************************************************/

static gchar *houseFormatStrings[] = {
    "Composite, Auto",      /* VIDEO_MODE_COMPOSITE_AUTO */
    "TTL",                  /* VIDEO_MODE_TTL */
    "Composite, Bi-Level",  /* VIDEO_MODE_COMPOSITE_BI_LEVEL */
    "Composite, Tri-Level", /* VIDEO_MODE_COMPOSITE_TRI_LEVEL */
    };

static void detect_house_sync_format_toggled(GtkToggleButton *togglebutton,
                                             gpointer user_data);

/*
 * House Sync autodetection scheme: a modal push button is used to
 * request auto detection.  When the button is pressed, we program the
 * first format type and then start a timer.
 *
 * From the timer, we check if we are getting a house sync; if we are,
 * then update the settings and unpress the button.  If we are not,
 * program the next format in the sequence and try again.
 */


/*
 * detect_house_sync_format_timer() - 
 */

static gboolean detect_house_sync_format_timer(gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    NvCtrlAttributeHandle *handle = NULL;
    gint house;
    GtkTreeIter iter;

    if (!find_master(ctk_framelock, &iter, &handle)) {
        goto done;
    }

    /* check if we now have house sync */
    
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_HOUSE_STATUS, &house);

    if (house) {
        GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
        /*
         * We found house sync; use the current_detect_format
         */

        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_HOUSE, house, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_HOUSE_FORMAT,
                           ctk_framelock->current_detect_format, -1);
        
        update_house_sync_controls(ctk_framelock);

        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "House Sync format detected as %s.",
                                     houseFormatStrings
                                     [ctk_framelock->current_detect_format]);
        
        goto done;
    }

    /*
     * we did not find house sync, yet, so move to the next format
     */
    
    switch (ctk_framelock->current_detect_format) {

    case NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO:
        ctk_framelock->current_detect_format =
            NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_BI_LEVEL;
        break;
        
    case NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_BI_LEVEL:
        ctk_framelock->current_detect_format =
            NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_TRI_LEVEL;
        break;
        
    case NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_TRI_LEVEL:
        ctk_framelock->current_detect_format =
            NV_CTRL_FRAMELOCK_VIDEO_MODE_TTL;
        break;

    case NV_CTRL_FRAMELOCK_VIDEO_MODE_TTL:
        ctk_framelock->current_detect_format =
            NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO;
        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "Unable to detect house sync format.");
        goto done;
        break;
    }
    
    /*
     * Set the new video format
     */

    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,
                       ctk_framelock->current_detect_format);

    return TRUE;

 done:

    /* untoggle the detect button */
    
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->house_format_detect),
         G_CALLBACK(detect_house_sync_format_toggled),
         (gpointer) ctk_framelock);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->house_format_detect), FALSE);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->house_format_detect),
         G_CALLBACK(detect_house_sync_format_toggled),
         (gpointer) ctk_framelock);

    /* do not call this timer any more */
    
    return FALSE;
    
} /* detect_house_sync_format_timer() */



/*
 * detect_house_sync_format_toggled() - called when the house sync
 * "detect" button is toggled.  If the toggle button is active, then
 * start the detect sequence by programming
 * NV_CTRL_FRAMELOCK_VIDEO_MODE to COMPOSITE_AUTO
 *
 * XXX what happens if the master gets changed while we are doing
 * this?
 */

static void detect_house_sync_format_toggled(GtkToggleButton *togglebutton,
                                             gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    NvCtrlAttributeHandle *handle = NULL;
    
    if (gtk_toggle_button_get_active(togglebutton)) {
        
        /*
         * the toggle button is active: we now start scanning through
         * the possible input video modes and enable the house sync
         * format timer.
         */
        
        if (!find_master(ctk_framelock, NULL, &handle)) {

            g_signal_handlers_block_by_func
                (G_OBJECT(ctk_framelock->house_format_detect),
                 G_CALLBACK(detect_house_sync_format_toggled),
                 (gpointer) ctk_framelock);
            
            gtk_toggle_button_set_active
                (GTK_TOGGLE_BUTTON(ctk_framelock->house_format_detect), FALSE);

            g_signal_handlers_unblock_by_func
                (G_OBJECT(ctk_framelock->house_format_detect),
                 G_CALLBACK(detect_house_sync_format_toggled),
                 (gpointer) ctk_framelock);
            
            return;
        }
        
        ctk_framelock->current_detect_format =
            NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO;

        NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,
                           ctk_framelock->current_detect_format);
        
        ctk_framelock->house_format_detect_timer =
            g_timeout_add(500, detect_house_sync_format_timer, user_data);
        
        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "Attempting to detect house sync...");
    } else {
        
        /*
         * the toggle button is no longer active: disable the timer
         */
        
        g_source_remove(ctk_framelock->house_format_detect_timer);
        ctk_framelock->house_format_detect_timer = 0;

        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "Aborted house sync detection.");
    }
    
} /* detect_house_sync_format_toggled() */



/*
 * add_house_sync_controls() -
 */

static GtkWidget *add_house_sync_controls(CtkFramelock *ctk_framelock)
{
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *label;
    GList *glist;
    
    hbox = gtk_hbox_new(FALSE, 5);
    
    /* sync interval */

    ctk_framelock->sync_interval_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->sync_interval_frame,
                       FALSE, TRUE, 0);
    
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(ctk_framelock->sync_interval_frame),hbox2);
        
    label = gtk_label_new("Sync Interval:");
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 5);
    
    ctk_framelock->sync_interval_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ctk_framelock->sync_interval_entry), "0");
    gtk_entry_set_width_chars
        (GTK_ENTRY(ctk_framelock->sync_interval_entry), 4);
    gtk_box_pack_start
        (GTK_BOX(hbox2), ctk_framelock->sync_interval_entry, FALSE, TRUE, 5);

    g_signal_connect(G_OBJECT(ctk_framelock->sync_interval_entry),
                     "activate", G_CALLBACK(sync_interval_entry_activate),
                     (gpointer) ctk_framelock);
    
    /* house format */

    ctk_framelock->house_format_frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_framelock->house_format_frame, FALSE, TRUE, 0);
    
    hbox2 = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(ctk_framelock->house_format_frame), hbox2);
    
    label = gtk_label_new("House Sync Format:");
    gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, TRUE, 5);
    
    ctk_framelock->house_format_combo = gtk_combo_new();
    glist = NULL;
    
    glist = g_list_append
        (glist,
         houseFormatStrings[NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO]);
    
    glist = g_list_append
        (glist,
         houseFormatStrings[NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_BI_LEVEL]);
    
    glist = g_list_append
        (glist,
         houseFormatStrings[NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_TRI_LEVEL]);
    
    glist = g_list_append
        (glist, houseFormatStrings[NV_CTRL_FRAMELOCK_VIDEO_MODE_TTL]);
    
    gtk_combo_set_popdown_strings
        (GTK_COMBO(ctk_framelock->house_format_combo), glist);
    gtk_editable_set_editable
        (GTK_EDITABLE(GTK_COMBO(ctk_framelock->house_format_combo)->entry),
         FALSE);
    
    g_signal_connect
        (G_OBJECT(GTK_EDITABLE
                  (GTK_COMBO(ctk_framelock->house_format_combo)->entry)),
         "changed", G_CALLBACK(house_sync_format_entry_activate),
         (gpointer) ctk_framelock);
    
    gtk_box_pack_start(GTK_BOX(hbox2),
                       ctk_framelock->house_format_combo, FALSE, TRUE, 5);

    /* detect button */

    ctk_framelock->house_format_detect =
        gtk_toggle_button_new_with_label("Detect");
    gtk_box_pack_start(GTK_BOX(hbox2),
                       ctk_framelock->house_format_detect, FALSE, TRUE, 5);

    g_signal_connect(G_OBJECT(ctk_framelock->house_format_detect), "toggled",
                     G_CALLBACK(detect_house_sync_format_toggled),
                     ctk_framelock);

    return hbox;
    
} /* add_house_sync_controls() */



/*
 * update_house_sync_controls() - update the gui with the current
 * sw-state of the house sync control values.
 */

static void update_house_sync_controls(CtkFramelock *ctk_framelock)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
    gboolean house = FALSE, sensitive;
    gint sync_interval, house_format;
    gchar str[32];
    GtkTreeIter iter;
    
    if (find_master(ctk_framelock, &iter, NULL)) {
        gtk_tree_model_get(model,                &iter,
                           COLUMN_HOUSE,         &house,
                           COLUMN_SYNC_INTERVAL, &sync_interval,
                           COLUMN_HOUSE_FORMAT,  &house_format,
                           -1);
        
        snprintf(str, 32, "%d", sync_interval);
        gtk_entry_set_text(GTK_ENTRY(ctk_framelock->sync_interval_entry),str); 
        
        if (house_format < NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE;
        if (house_format > NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV;
        
        gtk_entry_set_text
            (GTK_ENTRY(GTK_COMBO(ctk_framelock->house_format_combo)->entry),
             houseFormatStrings[house_format]);
    }
    
    if (ctk_framelock->framelock_enabled) {
        sensitive = FALSE;
    } else {
        sensitive = TRUE;
    }
    
    gtk_widget_set_sensitive(ctk_framelock->sync_interval_frame, sensitive);
    gtk_widget_set_sensitive(ctk_framelock->house_format_frame, sensitive);  
    
} /* update_house_sync_controls() */



static void sync_interval_entry_activate(GtkEntry *entry, gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    NvCtrlAttributeHandle *handle = NULL;
    const gchar *str = gtk_entry_get_text(entry);
    gint interval;
    
    interval = strtol(str, NULL, 10);
    
    if (find_master(ctk_framelock, NULL, &handle)) {
        NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_INTERVAL, interval);
    }
}

static void house_sync_format_entry_activate(GtkEditable *editable,
                                             gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    const gchar *str = gtk_entry_get_text
        (GTK_ENTRY(GTK_COMBO(ctk_framelock->house_format_combo)->entry));
    NvCtrlAttributeHandle *handle = NULL;
    gint mode;

    for (mode = NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE;
         mode <= NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV; mode++) {
        
        if (strcmp(houseFormatStrings[mode], str) == 0) {

            if (find_master(ctk_framelock, NULL, &handle)) {
                NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_VIDEO_MODE, mode);
            }
            return;
        }
    }
} /* house_sync_format_entry_activate() */



/*
 * add_columns_to_treeview() - add the columns to the treeview,
 * assigning renderer functions as necessary
 */

static void add_columns_to_treeview(CtkFramelock *ctk_framelock)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    /* column for display name */
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Display",
                                                      renderer,
                                                      "text",
                                                      COLUMN_DISPLAY_NAME,
                                                      NULL);
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    
    /* column for master toggles */
    
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_radio((GtkCellRendererToggle*)renderer, TRUE);
    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(master_toggled), ctk_framelock);
    
    column = gtk_tree_view_column_new_with_attributes("Master",
                                                      renderer,
                                                      "active",
                                                      COLUMN_MASTER,
                                                      NULL);
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    

    /* column for stereo */
    
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("Stereo Sync",
                                                      renderer,
                                                      NULL);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            led_renderer_func,
                                            GINT_TO_POINTER
                                            (COLUMN_STEREO_SYNC),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    /* column for timing */

    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("Timing",
                                                      renderer,
                                                      NULL);

    /*
     * led_renderer_func() needs the ctk_framelock, but only when
     * dealing with the Timing column; so hook a pointer to
     * ctk_framelock off of this ViewColumn widget.
     */
    
    g_object_set_data(G_OBJECT(column), "ctk_framelock", ctk_framelock);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            led_renderer_func,
                                            GINT_TO_POINTER(COLUMN_TIMING),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    /* column for sync_ready */
    
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("Sync Ready",
                                                      renderer,
                                                      NULL);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            led_renderer_func,
                                            GINT_TO_POINTER(COLUMN_SYNC_READY),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    
    /* column for sync_rate */
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Sync Rate",
                                                      renderer,
                                                      NULL);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            rate_renderer_func,
                                            GINT_TO_POINTER(COLUMN_SYNC_RATE),
                                            NULL);
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);


    /* column for house */
    
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("House",
                                                      renderer,
                                                      NULL);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            led_renderer_func,
                                            GINT_TO_POINTER(COLUMN_HOUSE),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    /* column for rj45 port0 */
    
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("Port0",
                                                      renderer,
                                                      NULL);
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            rj45_renderer_func,
                                            GINT_TO_POINTER(COLUMN_RJ45_PORT0),
                                            NULL);

    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    /* column for rj45 port1 */
    
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new_with_attributes("Port1",
                                                      renderer,
                                                      NULL);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            rj45_renderer_func,
                                            GINT_TO_POINTER(COLUMN_RJ45_PORT1),
                                            NULL);

    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    
    
    /* column for rising edge */
    
    renderer = gtk_cell_renderer_toggle_new();

    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(rising_edge_toggled), ctk_framelock);

    column = gtk_tree_view_column_new_with_attributes("Rising", renderer,
                                                      NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            polarity_renderer_func,
                                            GUINT_TO_POINTER(POLARITY_RISING),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);
    
    
    /* column for falling edge */
    
    renderer = gtk_cell_renderer_toggle_new();

    g_signal_connect(renderer, "toggled",
                     G_CALLBACK(falling_edge_toggled), ctk_framelock);

    column = gtk_tree_view_column_new_with_attributes("Falling",
                                                      renderer,
                                                      NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            polarity_renderer_func,
                                            GUINT_TO_POINTER(POLARITY_FALLING),
                                            NULL);
    
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

    
    /* column for sync skew */
    
    renderer = gtk_cell_renderer_text_new();

    g_signal_connect(renderer, "edited",
                     G_CALLBACK(sync_skew_edited), ctk_framelock);
    
    column = gtk_tree_view_column_new_with_attributes("Sync Skew",
                                                      renderer,
                                                      "text",
                                                      COLUMN_SYNC_SKEW,
                                                      "editable",
                                                      TRUE,
                                                      NULL);
    
    gtk_tree_view_column_set_cell_data_func(column,
                                            renderer,
                                            sync_skew_renderer_func,
                                            GINT_TO_POINTER(COLUMN_SYNC_SKEW),
                                            NULL);
    
    gtk_tree_view_append_column(ctk_framelock->treeview, column);
    gtk_tree_view_column_set_resizable(column, TRUE);

} /* add_columns_to_treeview() */



/*
 * led_renderer_func() - set the cell's pixbuf to either a green or a
 * red LED, based on the value in the model for this iter.
 */

static void led_renderer_func(GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
    static GdkPixbuf *led_green_pixbuf = NULL;
    static GdkPixbuf *led_red_pixbuf = NULL;
    static GdkPixbuf *led_grey_pixbuf = NULL;
    gboolean value, master, framelock_enabled;
    gpointer obj;
    gint column, house = 0;

    /*
     * we hooked a pointer to the ctk_framelock off the ViewColumn
     * widget
     */

    obj = g_object_get_data(G_OBJECT(tree_column), "ctk_framelock");
    
    column = GPOINTER_TO_INT(data);
    
    gtk_tree_model_get(model, iter, column, &value,
                       COLUMN_MASTER, &master, -1);

    framelock_enabled = FALSE;  
    if (obj) {
        CtkFramelock *ctk_framelock = CTK_FRAMELOCK(obj);
        if (ctk_framelock->framelock_enabled) framelock_enabled = TRUE;
    }

    gtk_tree_model_get(model, iter, COLUMN_HOUSE, &house, -1);
    
    /* 
     * make the master's Timing LED grey if framelock is enabled
     * (otherwise, it will be red when framelock is enabled, which is
     * confusing).
     *
     * If we are receiving house sync, then light the LED green.
     */
    
    if ((column == COLUMN_TIMING) && master && framelock_enabled && !house) {
        if (!led_grey_pixbuf)
            led_grey_pixbuf = gdk_pixbuf_new_from_xpm_data(led_grey_xpm);
        g_object_set(GTK_CELL_RENDERER(cell), "pixbuf", led_grey_pixbuf, NULL);
    } else if (value || (house && framelock_enabled && master)) {
        if (!led_green_pixbuf)
            led_green_pixbuf = gdk_pixbuf_new_from_xpm_data(led_green_xpm);
        g_object_set(GTK_CELL_RENDERER(cell),
                     "pixbuf", led_green_pixbuf, NULL);
    } else {
        if (!led_red_pixbuf)
            led_red_pixbuf = gdk_pixbuf_new_from_xpm_data(led_red_xpm);
        g_object_set(GTK_CELL_RENDERER(cell), "pixbuf", led_red_pixbuf, NULL);
    }
} /* led_renderer_func() */



/*
 * rj45_renderer_func() - set the cell's pixbuf either to the "input"
 * rj45 pixbuf or the "output" rj45 pixbuf, based on the value in the
 * model for this iter.
 *
 * XXX should there be an "unknown" state?
 */

static void rj45_renderer_func(GtkTreeViewColumn *tree_column,
                               GtkCellRenderer   *cell,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           data)
{
    static GdkPixbuf *rj45_input_pixbuf = NULL;
    static GdkPixbuf *rj45_output_pixbuf = NULL;
    gboolean value;
    gint column;

    column = GPOINTER_TO_INT(data);

    gtk_tree_model_get (model, iter, column, &value, -1);

    if (value == NV_CTRL_FRAMELOCK_PORT0_STATUS_INPUT) {
        if (!rj45_input_pixbuf)
            rj45_input_pixbuf = gdk_pixbuf_new_from_xpm_data(rj45_input_xpm);
        g_object_set(GTK_CELL_RENDERER(cell), "pixbuf",
                      rj45_input_pixbuf, NULL);
    } else {
        if (!rj45_output_pixbuf)
            rj45_output_pixbuf = gdk_pixbuf_new_from_xpm_data(rj45_output_xpm);
        g_object_set(GTK_CELL_RENDERER(cell), "pixbuf",
                     rj45_output_pixbuf, NULL);
    }
} /* rj45_renderer_func() */



/*
 * rate_renderer_func() - set the cell's "text" attribute to a string
 * representation of the rate in the model at this iter.
 */

static void rate_renderer_func(GtkTreeViewColumn *tree_column,
                               GtkCellRenderer   *cell,
                               GtkTreeModel      *model,
                               GtkTreeIter       *iter,
                               gpointer           data)
{
    gint column = GPOINTER_TO_INT(data);
    guint value;
    gfloat fvalue;
    gchar str[32];

    gtk_tree_model_get (model, iter, column, &value, -1);

    fvalue = (float) value / 1000.0;

    snprintf(str, 32, "%6.2f Hz", fvalue);
    
    g_object_set(GTK_CELL_RENDERER(cell), "text", str, NULL);

} /* rate_renderer_func() */



/*
 * polarity_renderer_func() - set the attribute "active" to either
 * true or false, based on if the bit specified in mask is present in
 * the model for this iter.
 */

static void polarity_renderer_func(GtkTreeViewColumn *tree_column,
                                   GtkCellRenderer   *cell,
                                   GtkTreeModel      *model,
                                   GtkTreeIter       *iter,
                                   gpointer           data)
{
    guint value, mask = GPOINTER_TO_UINT(data);
    
    gtk_tree_model_get(model, iter, COLUMN_POLARITY, &value, -1);
    
    if (value & mask) {
        g_object_set(GTK_CELL_RENDERER(cell), "active", TRUE, NULL);
    } else {
        g_object_set(GTK_CELL_RENDERER(cell), "active", FALSE, NULL);
    }
} /* polarity_renderer_func() */



/*
 * sync_skew_renderer_func() - set the cell's "text" attribute to a
 * string representation of the sync delay in the model at this iter.
 */

static void sync_skew_renderer_func(GtkTreeViewColumn *tree_column,
                                    GtkCellRenderer   *cell,
                                    GtkTreeModel      *model,
                                    GtkTreeIter       *iter,
                                    gpointer           data)
{
    gint column = GPOINTER_TO_INT(data);
    guint value;
    gchar str[32];
    gfloat delay;
    
    gtk_tree_model_get (model, iter, column, &value, -1);

    delay = ((gfloat) value) * NV_CTRL_FRAMELOCK_SYNC_DELAY_FACTOR;

    snprintf(str, 32, "%10.2f uS", delay);
    
    g_object_set (GTK_CELL_RENDERER(cell), "text", str, NULL);
    
} /* sync_skew_renderer_func() */



/*
 * master_toggled() - called whenever a master is assigned.
 */

static void master_toggled(GtkCellRendererToggle *cell, 
                           gchar                 *path_str,
                           gpointer               data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(data);
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
    GtkTreeIter   iter, walking_iter;
    GtkTreePath  *path = gtk_tree_path_new_from_string(path_str);
    gboolean master, valid;
    gchar *display_name;
    
    NvCtrlAttributeHandle *handle;

    /* do not change the master while framelock is enabled */

    if (ctk_framelock->framelock_enabled) {
        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "Cannot change master while "
                                     "FrameLock is enabled.");
        return;
    }

    /* get toggled iter */
    
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);
    
    /* if we're already the master, do nothing */

    gtk_tree_model_get(model, &iter, COLUMN_MASTER, &master,
                       COLUMN_DISPLAY_NAME, &display_name, -1);
    
    if (master) return;
    
    /* walk through the model, and turn off any other masters */

    valid = gtk_tree_model_get_iter_first(model, &walking_iter);
    while (valid) {
        gboolean walking_master;
        gtk_tree_model_get(model, &walking_iter,
                           COLUMN_MASTER, &walking_master, -1);
        
        if (walking_master) {
            gtk_tree_model_get(model, &walking_iter,
                               COLUMN_HANDLE, &handle, -1);
            if (handle) {
                gtk_list_store_set(GTK_LIST_STORE(model), &walking_iter,
                                   COLUMN_MASTER, FALSE, -1);
                NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_MASTER,
                                   NV_CTRL_FRAMELOCK_MASTER_FALSE);
            }
        }
        
        valid = gtk_tree_model_iter_next(model, &walking_iter);
    }
    
    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_MASTER,
                       TRUE, -1);
    
    gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle, -1);
    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_MASTER,
                       NV_CTRL_FRAMELOCK_MASTER_TRUE);
    
    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "X Screen '%s' assigned master.",
                                 display_name);

    update_house_sync_controls(ctk_framelock);
    
} /* master_toggled() */

static void polarity_toggled(GtkCellRendererToggle *cell,
                             CtkFramelock          *ctk_framelock,
                             gchar                 *path_string,
                             guint                  mask)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
    GtkTreePath *path;
    GtkTreeIter iter;
    gint polarity;
    gboolean enabled;
    NvCtrlAttributeHandle *handle;
    gchar *polarity_str;

    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    gtk_tree_model_get(model, &iter, COLUMN_POLARITY, &polarity, -1);

    g_object_get(GTK_CELL_RENDERER(cell), "active", &enabled, NULL);

    enabled ^= 1;

    if (enabled) polarity |= mask;
    else         polarity &= ~mask;
    

    /* if the last bit was turned off, turn back on the other bit */

    if (!polarity) {
        if (mask == NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE)
            polarity = NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE;
        if (mask == NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE)
            polarity = NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE;
    }

    gtk_list_store_set(ctk_framelock->list_store, &iter,
                       COLUMN_POLARITY, polarity, -1);
    
    gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle, -1);
    
    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_POLARITY, polarity);

    switch (polarity) {
    case NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE:
        polarity_str = "rising";
        break;
    case NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE:
        polarity_str = "falling";
        break;
    case NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES:
        polarity_str = "rising and falling";
        break;
    default:
        return;
    }

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Set edge polarity to %s.", polarity_str);
}

static void rising_edge_toggled(GtkCellRendererToggle *cell,
                                gchar                 *path_string,
                                gpointer               user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    
    polarity_toggled(cell, ctk_framelock, path_string, POLARITY_RISING);
}

static void falling_edge_toggled(GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    
    polarity_toggled(cell, ctk_framelock, path_string, POLARITY_FALLING);
}


static void sync_skew_edited(GtkCellRendererText *cell,
                             const gchar         *path_string,
                             const gchar         *new_text,
                             gpointer             data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(data);
    NvCtrlAttributeHandle *handle;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gfloat delay;
    gint value;

    delay = strtod(new_text, (char **)NULL);

    value = (gint) (delay / NV_CTRL_FRAMELOCK_SYNC_DELAY_FACTOR);

    if (value < 0) value = 0;
    if (value > NV_CTRL_FRAMELOCK_SYNC_DELAY_MAX)
        value = NV_CTRL_FRAMELOCK_SYNC_DELAY_MAX;
    
    delay = ((gfloat) value) * NV_CTRL_FRAMELOCK_SYNC_DELAY_FACTOR;
    
    model = GTK_TREE_MODEL(ctk_framelock->list_store);
    path = gtk_tree_path_new_from_string(path_string);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_path_free(path);

    gtk_list_store_set(ctk_framelock->list_store, &iter,
                       COLUMN_SYNC_SKEW, value, -1);
    
    gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle, -1);

    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_DELAY, value);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Sync delay set to %.2f uS", delay);
}



/************************************************************************/
/*
 * functions relating to add_x_screen_dialog
 */


/*
 * add_x_screen_response() - this function gets called in response to
 * the "response" event from the "Add X Screen..." dialog box.
 */

static void add_x_screen_response(GtkWidget *button, gint response,
                                  gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    const gchar *display_name;
    
    /* hide the dialog box */
 
    gtk_widget_hide_all(ctk_framelock->add_x_screen_dialog);
    
    /* set the focus back to the text entry */
    
    gtk_widget_grab_focus(ctk_framelock->add_x_screen_entry);
    
    /* if the response is not "OK" then we're done */
    
    if (response != GTK_RESPONSE_OK) return;
    
    /* get the display name specified by the user */

    display_name =
        gtk_entry_get_text(GTK_ENTRY(ctk_framelock->add_x_screen_entry));
    
    add_x_screen(ctk_framelock, display_name, TRUE);
    
}


static gpointer add_x_screen(CtkFramelock *ctk_framelock,
                             const gchar *display_name, gboolean error_dialog)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gpointer h;
    Display *display;
    gint screen, value;
    gboolean valid;
    const gchar *tmp;

    /* if no display name specified, print an error and return */

    if (!display_name || (display_name[0] == '\0')) {

        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" size=\"larger\">"
                      "Unable to add X screen to FrameLock group.</span>\n\n"
                      "No X Screen specified.");
        } else {
            nv_error_msg("Unable to add X screen to FrameLock group; "
                         "no X Screen specified.");
        }
        return NULL;
    }

    /*
     * try to prevent users from adding the same X screen more than
     * once XXX this is not an absolute check: this does not catch
     * "localhost:0.0" versus ":0.0", for example.
     */
    
    model = GTK_TREE_MODEL(ctk_framelock->list_store);    
    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_DISPLAY_NAME, &tmp, -1);
        if (nv_strcasecmp(display_name, tmp)) {
            if (error_dialog) {
                error_msg(ctk_framelock, "<span weight=\"bold\" "
                          "size=\"larger\">Unable to add X screen "
                          "to FrameLock Group</span>\n\n"
                          "The X screen %s already belongs to the FrameLock "
                          "Group.", display_name);
            } else {
                nv_error_msg("Unable to add X screen to FrameLock Group; "
                             "the X screen %s already belongs to the "
                             "FrameLock Group.", display_name);
            }
            return NULL;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    /* open an X Display connection to that X screen */
    
    display = XOpenDisplay(display_name);
    if (!display) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable "
                      "to add X screen to FrameLock group</span>\n\nUnable to "
                      "connect to X Display '%s'.", display_name);
        } else {
            nv_error_msg("Unable to add X screen to FrameLock group; unable "
                         "to connect to X Display '%s'.", display_name);
        }
        return NULL;
    }
    
    /* create a new NV-CONTROL handle */
    
    screen = DefaultScreen(display);
    
    h = NvCtrlAttributeInit(display, screen,
                            NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
    
    if (!h) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable "
                      "to add X screen to FrameLock group</span>\n\nXXX need "
                      "descriptive message.");
        } else {
            nv_error_msg("Unable to add X screen to FrameLock group.");
        }
        return NULL;
    }

    /* does this NV-CONTROL handle support FrameLock? */
    
    NvCtrlGetAttribute(h, NV_CTRL_FRAMELOCK, &value);
    if (value != NV_CTRL_FRAMELOCK_SUPPORTED) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable "
                      "to add X screen to FrameLock group</span>\n\n"
                      "This X Screen does not support FrameLock.");
        } else {
            nv_error_msg("Unable to add X screen to FrameLock group; "
                         "this X Screen does not support FrameLock.");
        }
        NvCtrlAttributeClose(h);
        return NULL;
    }

    /* XXX need to check that the current modeline matches */

    /* add the screen to the list store */
    
    add_member_to_list_store(ctk_framelock, h);

    /* update the house sync controls */

    update_house_sync_controls(ctk_framelock);

    /* enable the "Test Link" and "Enable Framelock" buttons */

    gtk_widget_set_sensitive(ctk_framelock->sync_state_button, TRUE);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Added X Screen '%s'", display_name);
    return h;
}

static GtkWidget *create_add_x_screen_dialog(CtkFramelock *ctk_framelock)
{
    GtkWidget *dialog;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label, *descr;
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    GtkWidget *alignment;
 
    dialog = gtk_dialog_new_with_buttons("Add X Screen",
                                         ctk_framelock->parent_window,
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT |
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK,
                                         NULL);

    g_signal_connect (GTK_OBJECT(dialog), "response",
                      G_CALLBACK(add_x_screen_response),
                      GTK_OBJECT(ctk_framelock));

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    hbox = gtk_hbox_new(FALSE, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

    pixbuf = gtk_widget_render_icon(dialog, GTK_STOCK_DIALOG_QUESTION,
                                    GTK_ICON_SIZE_DIALOG, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    label = gtk_label_new("X Screen:");
    descr = gtk_label_new("Please specify the X screen to be added to the "
                          "FrameLock group.");
    
    ctk_framelock->add_x_screen_entry = gtk_entry_new();
    
    gtk_entry_set_text(GTK_ENTRY(ctk_framelock->add_x_screen_entry),
                       NvCtrlGetDisplayName
                       (ctk_framelock->attribute_handle));
    
    gtk_entry_set_width_chars
        (GTK_ENTRY(ctk_framelock->add_x_screen_entry), 16);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);

    vbox = gtk_vbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), descr);
    gtk_box_pack_start(GTK_BOX(vbox), alignment, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->add_x_screen_entry,
                       TRUE, TRUE, 0);
    
    return dialog;

} /* create_add_x_screen_dialog() */


/************************************************************************/
/*
 * functions relating to remove_x_screen_dialog
 */


static void tree_selection_changed(GtkTreeSelection *selection,
                                   gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);

    if (gtk_tree_selection_get_selected(selection, NULL, NULL)) {
        gtk_widget_set_sensitive(ctk_framelock->remove_x_screen_button, TRUE);
    } else {
        gtk_widget_set_sensitive(ctk_framelock->remove_x_screen_button,FALSE);
    }
}



static void remove_x_screen(GtkWidget *button, gint response,
                            gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    gboolean valid;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *display_name;
    
    gtk_widget_hide_all(ctk_framelock->remove_x_screen_dialog);

    if (response != GTK_RESPONSE_OK) return;

    gtk_tree_model_get(GTK_TREE_MODEL(ctk_framelock->list_store),
                       &ctk_framelock->remove_x_screen_iter,
                       COLUMN_DISPLAY_NAME, &display_name, -1);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Removed X Screen '%s'", display_name);
    
    /* XXX disable anything that we need to in the X server */
    
    gtk_list_store_remove(GTK_LIST_STORE(ctk_framelock->list_store),
                          &ctk_framelock->remove_x_screen_iter);
    
    /*
     * if there are no entries left, then disable the "Test Link" and
     * "Enable FrameLock" buttons.
     */

    model = GTK_TREE_MODEL(ctk_framelock->list_store);
    valid = gtk_tree_model_get_iter_first(model, &iter);
    if (!valid) {
        gtk_widget_set_sensitive(ctk_framelock->sync_state_button, FALSE);
    }
}

static void show_remove_x_screen_dialog(GtkWidget *button,
                                        CtkFramelock *ctk_framelock)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    gchar *str, *display_name;

    selection = gtk_tree_view_get_selection(ctk_framelock->treeview);
    
    if (!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;
    
    gtk_tree_model_get(GTK_TREE_MODEL(ctk_framelock->list_store), &iter,
                       COLUMN_DISPLAY_NAME, &display_name, -1);
    
    str = g_strconcat("Remove X Screen '", display_name, "'?", NULL);
    gtk_label_set_text(GTK_LABEL(ctk_framelock->remove_x_screen_label), str);
    g_free(str);
    
    ctk_framelock->remove_x_screen_iter = iter;
    
    gtk_widget_show_all(ctk_framelock->remove_x_screen_dialog);
}



static GtkWidget *create_remove_x_screen_dialog(
    CtkFramelock *ctk_framelock
)
{
    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    GtkWidget *alignment;
    

    dialog = gtk_dialog_new_with_buttons("Remove X Screen",
                                         ctk_framelock->parent_window,
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT |
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK,
                                         NULL);

    g_signal_connect(GTK_OBJECT(dialog), "response",
                     G_CALLBACK(remove_x_screen),
                     GTK_OBJECT(ctk_framelock));

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    
    hbox = gtk_hbox_new(FALSE, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
    
    pixbuf = gtk_widget_render_icon(dialog, GTK_STOCK_DIALOG_QUESTION,
                                    GTK_ICON_SIZE_DIALOG, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);

    ctk_framelock->remove_x_screen_label = gtk_label_new(NULL);
    
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_framelock->remove_x_screen_label);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

    return dialog;

} /* create_remove_x_screen_dialog() */



/************************************************************************/
/*
 * function for updating the status
 */


/*
 * update_status() - query the following from each member of the sync
 * group:
 *
 *  NV_CTRL_FRAMELOCK_STEREO_SYNC
 *  NV_CTRL_FRAMELOCK_TIMING
 *  NV_CTRL_FRAMELOCK_SYNC_READY
 *  NV_CTRL_FRAMELOCK_SYNC_RATE
 *  NV_CTRL_FRAMELOCK_HOUSE_STATUS
 *  NV_CTRL_FRAMELOCK_PORT0_STATUS
 *  NV_CTRL_FRAMELOCK_PORT1_STATUS
 *
 * XXX maybe rather than have a button to do this, the app could be
 * set to poll (and auto update the gui) periodically?
 */

static gboolean update_status(gpointer user_data)
{
    gboolean valid;
    GtkTreeIter iter;
    gint stereo_sync, timing, sync_ready, sync_rate, house, port0, port1;
    GtkTreeModel *model;
    NvCtrlAttributeHandle *handle;
    CtkFramelock *ctk_framelock;

    ctk_framelock = CTK_FRAMELOCK(user_data);

    model = GTK_TREE_MODEL(ctk_framelock->list_store);
    
    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle, -1);
        if (!handle) break;
        
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_STEREO_SYNC,  &stereo_sync);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_TIMING,       &timing);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_READY,   &sync_ready);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_RATE,    &sync_rate);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_HOUSE_STATUS, &house);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_PORT0_STATUS, &port0);
        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_PORT1_STATUS, &port1);
        
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_STEREO_SYNC, stereo_sync, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_TIMING, timing, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_SYNC_READY, sync_ready, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_SYNC_RATE, sync_rate, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_HOUSE, house, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_RJ45_PORT0, port0, -1);
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           COLUMN_RJ45_PORT1, port1, -1);

        valid = gtk_tree_model_iter_next(model, &iter);
    }

    return TRUE;
    
} /* update_status() */





/*
 * test_link() - tell the master to enable the test signal, update
 * everyone's status, and then disable the test signal.
 */

static void test_link(GtkWidget *button, CtkFramelock *ctk_framelock)
{
    gboolean valid, master, enabled;
    GtkTreeIter iter;
    NvCtrlAttributeHandle *handle;
    GtkTreeModel *model;
    
    enabled = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button));
    
    if (!enabled) return;
    
    model = GTK_TREE_MODEL(ctk_framelock->list_store);

    /* find the master handle */

    handle = NULL;

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_MASTER, &master, -1);
        if (master) {
            gtk_tree_model_get(model, &iter, COLUMN_HANDLE,
                               &handle, -1);
            break;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    if (!handle) return;

    /* enable the test signal */
    
    gdk_window_set_cursor
        ((GTK_WIDGET(ctk_framelock->parent_window))->window,
         ctk_framelock->wait_cursor);
        
    gtk_grab_add(button);
    
    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE);
    
    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Test Link started.");

    /* register the "done" function */

    g_timeout_add(DEFAULT_TEST_LINK_TIME_INTERVAL,
                  test_link_done, (gpointer) ctk_framelock);
    
} /* test_link() */


static gint test_link_done(gpointer data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *) data;
    gboolean valid, master;
    GtkTreeIter iter;
    NvCtrlAttributeHandle *handle;
    GtkTreeModel *model;

    model = GTK_TREE_MODEL(ctk_framelock->list_store);

    /* find the master handle */

    handle = NULL;

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_MASTER, &master, -1);
        if (master) {
            gtk_tree_model_get(model, &iter, COLUMN_HANDLE,
                               &handle, -1);
            break;
        }
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    if (!handle) return FALSE;

    /* disable the test signal */
        
    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE);
    
    gtk_grab_remove(ctk_framelock->test_link_button);
        
    gdk_window_set_cursor((GTK_WIDGET(ctk_framelock->parent_window))->window,
                          NULL);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Test Link complete.");

    /* un-press the testlink button */
    
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->test_link_button),
         G_CALLBACK(test_link),
         (gpointer) ctk_framelock);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button), FALSE);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->test_link_button),
         G_CALLBACK(test_link),
         (gpointer) ctk_framelock);
    
    return FALSE;
}


static GtkWidget *create_sync_state_button(CtkFramelock *ctk_framelock)
{
    GtkWidget *label;
    GtkWidget *hbox, *hbox2;
    GdkPixbuf *pixbuf;
    GtkWidget *image = NULL;
    GtkWidget *button;

    button = gtk_toggle_button_new();

    /* create the enable syncing icon */

    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_EXECUTE,
                                    GTK_ICON_SIZE_BUTTON,
                                    "enable framelock");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Enable FrameLock");

    hbox = gtk_hbox_new(FALSE, 2);

    if (image) gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, FALSE, 15);

    gtk_widget_show_all(hbox2);

    /*
     * XXX increment the reference count, so that when we do
     * gtk_container_remove() later, it doesn't get destroyed
     */

    gtk_object_ref(GTK_OBJECT(hbox2));

    ctk_framelock->enable_syncing_label = hbox2;
    

    /* create the disable syncing icon */
    
    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_STOP,
                                    GTK_ICON_SIZE_BUTTON,
                                    "disable framelock");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Disable FrameLock");
    
    hbox = gtk_hbox_new(FALSE, 2);
    
    if (image) gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, FALSE, 15);

    gtk_widget_show_all(hbox2);
    
    /*
     * XXX increment the reference count, so that when we do
     * gtk_container_remove() later, it doesn't get destroyed
     */

    gtk_object_ref(GTK_OBJECT(hbox2));
    
    ctk_framelock->disable_syncing_label = hbox2;
    
    /* start with syncing disabled */
    
    gtk_container_add(GTK_CONTAINER(button),
                      ctk_framelock->enable_syncing_label);
    
    return (button);
}


static void toggle_sync_state_button(GtkWidget *button,
                                     CtkFramelock *ctk_framelock)
{
    gboolean valid;
    GtkTreeIter iter;
    NvCtrlAttributeHandle *handle;
    guint display_mask, val;
    gboolean enabled;
    GtkTreeSelection *selection;
    
    GtkTreeModel *model;
    
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    if (enabled) val = NV_CTRL_FRAMELOCK_SYNC_ENABLE;
    else         val = NV_CTRL_FRAMELOCK_SYNC_DISABLE;
    
    /*
     * set the NV_CTRL_FRAMELOCK_SYNC status on each member of the
     * FrameLock group
     */
    
    handle = NULL;
    model = GTK_TREE_MODEL(ctk_framelock->list_store);
    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle,
                           COLUMN_DISPLAY_MASK, &display_mask, -1);
        if (!handle) return; /* XXX */
        
        NvCtrlSetDisplayAttribute(handle, display_mask,
                                  NV_CTRL_FRAMELOCK_SYNC, val);
        valid = gtk_tree_model_iter_next(model, &iter);
    }
    
    /* 
     * toggle the TEST_SIGNAL, to guarantee accuracy of the universal
     * frame count (as returned by the glXQueryFrameCountNV() function
     * in the GLX_NV_swap_group extension)
     */

    if (enabled && find_master(ctk_framelock, NULL, &handle)) {
        NvCtrlSetAttribute(handle,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE);
        NvCtrlSetAttribute(handle,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE);
    }

    /* alter the button */

    if (enabled) {
        if (!ctk_framelock->framelock_enabled) {
            
            gtk_container_remove
                (GTK_CONTAINER(ctk_framelock->sync_state_button),
                 ctk_framelock->enable_syncing_label);
            gtk_container_add(GTK_CONTAINER(ctk_framelock->sync_state_button),
                              ctk_framelock->disable_syncing_label);
        }
        
        /*
         * disable the "Add Screen" and "Remove Screen" buttons;
         * enable the "Test Link" button
         */

        gtk_widget_set_sensitive(ctk_framelock->add_x_screen_button, FALSE);
        gtk_widget_set_sensitive(ctk_framelock->remove_x_screen_button, FALSE);
        gtk_widget_set_sensitive(ctk_framelock->test_link_button, TRUE);
        
        /* disable the house sync controls */

        gtk_widget_set_sensitive(ctk_framelock->sync_interval_frame, FALSE);
        gtk_widget_set_sensitive(ctk_framelock->house_format_frame, FALSE);

    } else {
        if (ctk_framelock->framelock_enabled) {
            gtk_container_remove
                (GTK_CONTAINER(ctk_framelock->sync_state_button),
                 ctk_framelock->disable_syncing_label);
            gtk_container_add(GTK_CONTAINER(ctk_framelock->sync_state_button),
                              ctk_framelock->enable_syncing_label);
        }

        /* enable the "Add Screen" button; disable the "Test Link" button */

        gtk_widget_set_sensitive(ctk_framelock->add_x_screen_button, TRUE);
        gtk_widget_set_sensitive(ctk_framelock->test_link_button, FALSE);

        /* check if the "Remove Screen" button should be enabled */

        selection = gtk_tree_view_get_selection(ctk_framelock->treeview);
        tree_selection_changed(selection, GTK_OBJECT(ctk_framelock));

        /* enable the house sync controls */

        gtk_widget_set_sensitive(ctk_framelock->sync_interval_frame, TRUE);
        gtk_widget_set_sensitive(ctk_framelock->house_format_frame, TRUE);
    }

    ctk_framelock->framelock_enabled = enabled;

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "FrameLock %s.",
                                 enabled ? "enabled" : "disabled");

} /* toggle_sync_state_button() */


/************************************************************************/
/*
 * functions relating to the error_msg_dialog
 */

static GtkWidget *create_error_msg_dialog(CtkFramelock *ctk_framelock)
{
    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *image;
    GtkWidget *alignment;
    GdkPixbuf *pixbuf;
    
    
    dialog = gtk_dialog_new_with_buttons("Error",
                                         ctk_framelock->parent_window,
                                         GTK_DIALOG_MODAL |
                                         GTK_DIALOG_DESTROY_WITH_PARENT |
                                         GTK_DIALOG_NO_SEPARATOR,
                                         GTK_STOCK_OK,
                                         GTK_RESPONSE_OK,
                                         NULL);

    g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
                             G_CALLBACK(gtk_widget_hide_all),
                             GTK_OBJECT(dialog));

    gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    hbox = gtk_hbox_new(FALSE, 12);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
    
    pixbuf = gtk_widget_render_icon(dialog, GTK_STOCK_DIALOG_ERROR,
                                    GTK_ICON_SIZE_DIALOG, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    
    ctk_framelock->error_msg_label = gtk_label_new(NULL);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_framelock->error_msg_label);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

    return dialog;
}

static void error_msg(CtkFramelock *ctk_framelock, const gchar *fmt, ...)
{
    gchar *msg;

    NV_VSNPRINTF(msg, fmt);
    
    gtk_label_set_line_wrap(GTK_LABEL(ctk_framelock->error_msg_label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(ctk_framelock->error_msg_label), TRUE);
    gtk_label_set_markup(GTK_LABEL(ctk_framelock->error_msg_label), msg);
    gtk_widget_show_all(ctk_framelock->error_msg_dialog);
    
    free(msg);
}



/************************************************************************/
/*
 * Functions for manipulating the List Store
 */


static void create_list_store(CtkFramelock *ctk_framelock)
{
    GtkTreeSelection *selection;

    ctk_framelock->list_store =
        gtk_list_store_new(NUM_COLUMNS,
                           G_TYPE_POINTER,  /* HANDLE */
                           G_TYPE_UINT,     /* DISPLAY_MASK */
                           G_TYPE_STRING,   /* DISPLAY_NAME */
                           G_TYPE_BOOLEAN,  /* MASTER  */
                           G_TYPE_BOOLEAN,  /* STEREO_SYNC */
                           G_TYPE_BOOLEAN,  /* TIMING */
                           G_TYPE_BOOLEAN,  /* SYNC_READY */
                           G_TYPE_UINT,     /* SYNC_RATE */
                           G_TYPE_BOOLEAN,  /* HOUSE */
                           G_TYPE_BOOLEAN,  /* RJ45_PORT0 */
                           G_TYPE_BOOLEAN,  /* RJ45_PORT1 */
                           G_TYPE_UINT,     /* POLARITY */
                           G_TYPE_UINT,     /* SYNC_SKEW */
                           G_TYPE_UINT,     /* SYNC_INTERVAL */
                           G_TYPE_UINT);    /* HOUSE_FORMAT */

    /* create the treeview */

    ctk_framelock->treeview =
        GTK_TREE_VIEW(gtk_tree_view_new_with_model
                      (GTK_TREE_MODEL(ctk_framelock->list_store)));
    
    gtk_tree_view_set_rules_hint(ctk_framelock->treeview, TRUE);
    
    g_object_unref(ctk_framelock->list_store);

    /* watch for selection changes to the treeview */
    
    selection = gtk_tree_view_get_selection(ctk_framelock->treeview);
    
    g_signal_connect(selection, "changed",
                     G_CALLBACK(tree_selection_changed),
                     GTK_OBJECT(ctk_framelock));

    /* add columns to the tree view */
    
    add_columns_to_treeview(ctk_framelock);

} /* create_list_store() */



/*
 * add_member_to_list_store()
 */

static void add_member_to_list_store(CtkFramelock *ctk_framelock,
                                     const gpointer handle)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    gint master, stereo_sync, timing, sync_ready, sync_rate;
    gint house, port0, port1, polarity, sync_skew, display_mask;
    gint sync_interval, house_format;
    gboolean valid, have_master;
    gchar *display_name;

    /*
     * If we don't have a master already, make this the master; if we
     * do have a master already, make sure this isn't the master.
     */

    have_master = FALSE;
    model = GTK_TREE_MODEL(ctk_framelock->list_store);

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_MASTER, &have_master, -1);
        if (have_master) break;
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    master = !have_master;

    NvCtrlSetAttribute(handle, NV_CTRL_FRAMELOCK_MASTER, master);
    
    /* query all the other fields */
    
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_STEREO_SYNC,  &stereo_sync);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_TIMING,       &timing);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_READY,   &sync_ready);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_RATE,    &sync_rate);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_HOUSE_STATUS, &house);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_PORT0_STATUS, &port0);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_PORT1_STATUS, &port1);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_POLARITY,     &polarity);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_DELAY,   &sync_skew);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_SYNC_INTERVAL,&sync_interval);
    NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,   &house_format);
    
    NvCtrlGetAttribute(handle, NV_CTRL_ENABLED_DISPLAYS, &display_mask);

    gtk_list_store_append(ctk_framelock->list_store, &iter);
    
    display_name = NvCtrlGetDisplayName(handle);

    gtk_list_store_set(ctk_framelock->list_store, &iter,
                       COLUMN_HANDLE,            handle,
                       COLUMN_DISPLAY_MASK,      display_mask,
                       COLUMN_DISPLAY_NAME,      display_name,
                       COLUMN_MASTER,            master,
                       COLUMN_STEREO_SYNC,       stereo_sync,
                       COLUMN_TIMING,            timing,
                       COLUMN_SYNC_READY,        sync_ready,
                       COLUMN_SYNC_RATE,         sync_rate,
                       COLUMN_HOUSE,             house,
                       COLUMN_RJ45_PORT0,        port0,
                       COLUMN_RJ45_PORT1,        port1,
                       COLUMN_POLARITY,          polarity,
                       COLUMN_SYNC_SKEW,         sync_skew,
                       COLUMN_SYNC_INTERVAL,     sync_interval,
                       COLUMN_HOUSE_FORMAT,      house_format,
                       -1);
    
} /* add_member_to_list_store() */


static gboolean check_for_ethernet(gpointer user_data)
{
    CtkFramelock *ctk_framelock;
    GtkTreeIter iter;
    GtkTreeModel *model;
    
    NvCtrlAttributeHandle *handle;
    gchar *display_name;
    gboolean valid;
    gint val;
    
    ctk_framelock = CTK_FRAMELOCK(user_data);
    model = GTK_TREE_MODEL(ctk_framelock->list_store);

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &handle,
                           COLUMN_DISPLAY_NAME, &display_name, -1);
        if (!handle) break;

        NvCtrlGetAttribute(handle, NV_CTRL_FRAMELOCK_ETHERNET_DETECTED, &val);
    
        if (val != NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_NONE) {
            error_msg(ctk_framelock, "<span weight=\"bold\" size=\"larger\">"
                      "FrameLock RJ45 Error</span>\n\n"
                      "Either an Ethernet LAN cable is connected to the "
                      "framelock board on X Screen '%s' or the linked PC is "
                      "not turned on. "
                      "Either disconnect the LAN cable or turn on the linked "
                      "PC for proper operation.",
                      display_name);
            
            ctk_config_remove_timer(ctk_framelock->ctk_config,
                                    (GSourceFunc) check_for_ethernet);
            return FALSE;
        }

        valid = gtk_tree_model_iter_next(model, &iter);
    }

    return TRUE;
}



static gboolean find_master(CtkFramelock *ctk_framelock,
                            GtkTreeIter *return_iter,
                            NvCtrlAttributeHandle **return_handle)
{
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
    NvCtrlAttributeHandle *handle = NULL;
    gboolean master = FALSE, valid;
    GtkTreeIter iter;

    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        gtk_tree_model_get(model, &iter,
                           COLUMN_MASTER, &master,
                           COLUMN_HANDLE, &handle,
                           -1);
        if (master) break;

        valid = gtk_tree_model_iter_next(model, &iter);
    }

    if (return_iter) *return_iter = iter;
    if (return_handle) *return_handle = handle;
    
    return master;
}



/*
 * ctk_framelock_config_file_attributes() - add to the ParsedAttribute
 * list any attributes that we want saved in the config file.
 */

void ctk_framelock_config_file_attributes(GtkWidget *w, ParsedAttribute *head)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(w);
    GtkTreeModel *model = GTK_TREE_MODEL(ctk_framelock->list_store);
    GtkTreeIter   iter;
    gboolean      master, valid;
    gchar        *display_name;
    guint         polarity, display_mask;
    gint          delay, sync_interval, house_format;
    ParsedAttribute a;
    
    valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {

        /*
         * XXX is it sufficient to use sw state, or should we query
         * the hw?
         */

        gtk_tree_model_get(model, &iter,
                           COLUMN_DISPLAY_NAME, &display_name,
                           COLUMN_MASTER,       &master,
                           COLUMN_POLARITY,     &polarity,
                           COLUMN_SYNC_SKEW,    &delay,
                           COLUMN_DISPLAY_MASK, &display_mask,
                           COLUMN_SYNC_INTERVAL,&sync_interval,
                           COLUMN_HOUSE_FORMAT, &house_format,
                           -1);

#define __ADD_ATTR(x,y,z)                     \
        a.display             = display_name; \
        a.attr                = (x);          \
        a.val                 = (y);          \
        a.display_device_mask = (z);          \
        nv_parsed_attribute_add(head, &a);

        __ADD_ATTR(NV_CTRL_FRAMELOCK_MASTER, master, 0);
        __ADD_ATTR(NV_CTRL_FRAMELOCK_POLARITY, polarity, 0);
        __ADD_ATTR(NV_CTRL_FRAMELOCK_SYNC_DELAY, delay, 0);
        __ADD_ATTR(NV_CTRL_FRAMELOCK_SYNC,
                   ctk_framelock->framelock_enabled, display_mask);

        if (master) {
            __ADD_ATTR(NV_CTRL_FRAMELOCK_SYNC_INTERVAL, sync_interval, 0); 
            __ADD_ATTR(NV_CTRL_FRAMELOCK_VIDEO_MODE, house_format, 0);
        }

#undef __ADD_ATTR

        valid = gtk_tree_model_iter_next(model, &iter);
    }
} /* ctk_framelock_config_file_attributes() */



/*
 * apply_parsed_attribute_list() - given a list of parsed attributes
 * from the config file, apply the FrameLock settings contained
 * therein.
 */

static void apply_parsed_attribute_list(CtkFramelock *ctk_framelock,
                                        ParsedAttribute *p)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gpointer h, h_tmp;
    gboolean valid, enable = FALSE;
    gchar *display_name, *display_name_tmp;
    
    while (p) {
        
        if (!p->next) goto next_attribute;
    
        if (!(p->flags & NV_PARSER_TYPE_FRAMELOCK)) goto next_attribute;
        
        /*
         * canonicalize the display name, so that we have a better
         * chance of finding matches when we compare them below
         */
        
        display_name = nv_standardize_screen_name(p->display, -1);
        
        /*
         * find the NvCtrlAttributeHandle associated with this display
         * name; either there is already an entry in the model with
         * the same display name, and we can use its handle; or we add
         * this display to the model and get a new handle.  As a side
         * effect, we should end up with an iter that points into the
         * correct entry in the model.
         */
        
        h = NULL;
        
        model = GTK_TREE_MODEL(ctk_framelock->list_store);    
        valid = gtk_tree_model_get_iter_first(model, &iter);

        while (valid) {
            gtk_tree_model_get(model, &iter, COLUMN_DISPLAY_NAME,
                               &display_name_tmp, -1);
            if (nv_strcasecmp(display_name_tmp, display_name)) {
                gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &h, -1);
                break;
            }
            
            valid = gtk_tree_model_iter_next(model, &iter);
        }
    
        if (!h) {
            h = add_x_screen(ctk_framelock, display_name, FALSE);
            
            if (!h) goto next_attribute;

            model = GTK_TREE_MODEL(ctk_framelock->list_store);    
            valid = gtk_tree_model_get_iter_first(model, &iter);
            
            while (valid) {
                gtk_tree_model_get(model, &iter, COLUMN_HANDLE, &h_tmp, -1);
                
                if (h == h_tmp) break;
                valid = gtk_tree_model_iter_next(model, &iter);
            }
        }
        
        /*
         * now that we have an NvCtrlAttributeHandle and iter, apply
         * the setting; note that this only really updates the gui,
         * but the attributes have already been sent to the X server
         * by the config file parser.
         */
        
        switch (p->attr) {
        case NV_CTRL_FRAMELOCK_MASTER:
            
            /* XXX only allow one master */
            
            gtk_list_store_set(ctk_framelock->list_store, &iter,
                               COLUMN_MASTER, p->val, -1);
            break;
            
        case NV_CTRL_FRAMELOCK_POLARITY:
            gtk_list_store_set(ctk_framelock->list_store, &iter,
                               COLUMN_POLARITY, p->val, -1);
            break;
            
        case NV_CTRL_FRAMELOCK_SYNC_DELAY:
            gtk_list_store_set(ctk_framelock->list_store, &iter,
                               COLUMN_SYNC_SKEW, p->val, -1);
            break;
            
        case NV_CTRL_FRAMELOCK_SYNC:
            if (p->val) enable = TRUE;
            break;

        case NV_CTRL_FRAMELOCK_SYNC_INTERVAL:
            gtk_list_store_set(ctk_framelock->list_store, &iter,
                               COLUMN_SYNC_INTERVAL, p->val, -1);
            break;

        case NV_CTRL_FRAMELOCK_VIDEO_MODE:
            gtk_list_store_set(ctk_framelock->list_store, &iter,
                               COLUMN_HOUSE_FORMAT, p->val, -1);
            break;
        }
        
    next_attribute:
        
        p = p->next;
    }

    /*
     * set the state of the toggle button appropriately; this will
     * trigger toggle_sync_state_button()
     */

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->sync_state_button), enable);
    
} /* apply_parsed_attribute_list () */



GtkTextBuffer *ctk_framelock_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "FrameLock Help");

    ctk_help_para(b, &i, "The FrameLock control page provides a way to "
                  "manage an entire cluster of workstations in a FrameLock "
                  "group.");
    
    ctk_help_heading(b, &i, "Add X Screen...");
    ctk_help_para(b, &i, "Use this button to add an X Screen to the "
                  "FrameLock group.  If the X Screen is remote, be sure "
                  "you have configured remote access (via `xhost`, "
                  "for example) such that you are allowed to establish "
                  "a connection.");
    
    ctk_help_heading(b, &i, "Remove X Screen...");
    ctk_help_para(b, &i, "Use this button to remove the currently selected "
                  "X Screen from the FrameLock group.  If no X Screen is "
                  "currently selected, then this button is greyed out.");
    
    ctk_help_heading(b, &i, "Test Link");
    ctk_help_para(b, &i, "Use this toggle button to enable testing of "
                  "the cabling between all members of the FrameLock group.  "
                  "This will cause all FrameLock boards to receive a sync "
                  "pulse, but the GPUs will not lock to the FrameLock "
                  "pulse.  When Test Link is enabled, no other settings may "
                  "be changed until you disable Test Link.");

    ctk_help_heading(b, &i, "Enable FrameLock");
    ctk_help_para(b, &i, "This will lock the refresh rates of all members "
                  "in the FrameLock group.  When FrameLock is enabled, "
                  "you cannot add or remove any X screens from the FrameLock "
                  "group, nor can you enable the Test Link.");

    ctk_help_heading(b, &i, "Display");
    ctk_help_para(b, &i, "The 'Display' column lists the name of each X "
                  "Screen in the FrameLock group.");

    ctk_help_heading(b, &i, "Master");
    ctk_help_para(b, &i, "This radio button indicates which X Screen in the "
                  "FrameLock group will emit the master sync pulse to which "
                  "all other group members will latch.");

    ctk_help_heading(b, &i, "Stereo Sync");
    ctk_help_para(b, &i, "This indicates that the GPU stereo signal is in "
                  "sync with the framelock stereo signal.");
    
    ctk_help_heading(b, &i, "Timing");
    ctk_help_para(b, &i, "This indicates that the FrameLock board is "
                  "receiving a timing input.");

    ctk_help_heading(b, &i, "Sync Ready");
    ctk_help_para(b, &i, "This indicates whether the FrameLock board is "
                  "receiving sync pulses.  Green means syncing; red "
                  "means not syncing.");

    ctk_help_heading(b, &i, "Sync Rate");
    ctk_help_para(b, &i, "This is the sync rate that the FrameLock board "
                  "is receiving.");

    ctk_help_heading(b, &i, "House");
    ctk_help_para(b, &i, "This indicates whether the FrameLock board is "
                  "receiving synchronization from the house (BNC) connector.");

    ctk_help_heading(b, &i, "Port0, Port1");
    ctk_help_para(b, &i, "These indicate the status of the RJ45 ports on "
                  "the backplane of the FrameLock board.  Green LEDs "
                  "indicate that the port is configured for input, while "
                  "yellow LEDs indicate that the port is configured for "
                  "output.");

    ctk_help_heading(b, &i, "Rising/Falling");
    ctk_help_para(b, &i, "These control which edge(s) of the sync pulse "
                  "are latched to.");

    ctk_help_heading(b, &i, "Sync Delay");
    ctk_help_para(b, &i, "The delay (in microseconds) between the FrameLock "
                  "pulse and the GPU pulse.");
    
    ctk_help_heading(b, &i, "Miscellaneous");
    ctk_help_para(b, &i, "The FrameLock control page registers several timers "
                  "that are executed periodically; these are listed "
                  "in the 'Active Timers' section of the 'nvidia-settings "
                  "Configuration' page.  Most notably is the "
                  "'FrameLock Connection Status' "
                  "timer: this will poll all members of the FrameLock group "
                  "for status information.");

    ctk_help_finish(b);

    return b;
}
