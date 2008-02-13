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
#include <assert.h>

#include "ctkframelock.h"
#include "ctkhelp.h"
#include "ctkevent.h"

#include "ctkimage.h"

#include "led_green.h"
#include "led_red.h"
#include "led_grey.h"

#include "rj45_input.h"
#include "rj45_output.h"
#include "rj45_unused.h"

#include "bnc_cable.h"

#include "parse.h"
#include "msg.h"


#define DEFAULT_UPDATE_STATUS_TIME_INTERVAL       1000
#define DEFAULT_TEST_LINK_TIME_INTERVAL           2000
#define DEFAULT_CHECK_FOR_ETHERNET_TIME_INTERVAL  10000

#define DEFAULT_ENABLE_CONFIRM_TIMEOUT 30 /* When enabling Frame Lock when
                                           * no server is specified, this
                                           * is the number of seconds the
                                           * user has to confirm that
                                           * everything is ok.
                                           */

#define POLARITY_RISING   0x1
#define POLARITY_FALLING  0x2
#define POLARITY_BOTH     0x3

#define FRAME_PADDING 5


enum 
{
    ENTRY_DATA_FRAMELOCK = 0,
    ENTRY_DATA_GPU,
    ENTRY_DATA_DISPLAY
};

/*
 * These signals get hooked up (to the gpu_state_received() function)
 * for all frame lock devices that are included in the list.  When the
 * frame lock device entry is removed, these signals also get removed for
 * that entry.
 */

#define NUM_GPU_SIGNALS 5

const char *__GPUSignals[NUM_GPU_SIGNALS] = 
    {
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_MASTER),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_SLAVES),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_SYNC),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_TEST_SIGNAL),
        CTK_EVENT_NAME(NV_CTRL_REFRESH_RATE)
    };

/*
 * These signals get hooked up (to the framelock_state_received() function)
 * for all frame lock devices that are included in the list.  When the
 * frame lock device entry is removed, these signals also get removed for
 * that entry.
 */

#define NUM_FRAMELOCK_SIGNALS 4

const char *__FrameLockSignals[NUM_FRAMELOCK_SIGNALS] =
    {
        CTK_EVENT_NAME(NV_CTRL_USE_HOUSE_SYNC),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_SYNC_INTERVAL),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_POLARITY),
        CTK_EVENT_NAME(NV_CTRL_FRAMELOCK_VIDEO_MODE)
    };

typedef struct _nvListTreeRec      nvListTreeRec, *nvListTreePtr;
typedef struct _nvListEntryRec     nvListEntryRec, *nvListEntryPtr;

typedef struct _nvDisplayDataRec   nvDisplayDataRec, *nvDisplayDataPtr;
typedef struct _nvGPUDataRec       nvGPUDataRec, *nvGPUDataPtr;
typedef struct _nvFrameLockDataRec nvFrameLockDataRec, *nvFrameLockDataPtr;


struct _nvListEntryRec {
    
    nvListTreePtr tree;

    GtkWidget *vbox; /* Holds all entry widgets and children */

    GtkWidget *ebox; /* Event box for this entry's stuff */
    GtkWidget *hbox; /* Box inside ebox */

    GtkWidget *title_hbox;            /* This entry's title data */
    GtkWidget *padding_hbox;          /* Padding to denote nested hierarchy */
    GtkWidget *expander_hbox;
    GtkWidget *expander_button_image; /* Expander button widgets */
    GtkWidget *expander_button;
    GtkWidget *expander_vbox;         /* To align the button */
    gboolean   expanded;
    GtkWidget *label_hbox;

    GtkWidget *data_hbox;

    GtkWidget *child_vbox; /* Holds child entries */

    gpointer   data;      /* Data (used to render entry) */
    gint       data_type;
    CtkEvent  *ctk_event; /* For receiving events on the entry */

    nvListEntryPtr parent;
    nvListEntryPtr children;
    int            nchildren;

    nvListEntryPtr next_sibling;
};

struct _nvListTreeRec {

    GtkWidget *vbox; /* Holds top level entries */

    CtkFramelock  *ctk_framelock; /* XXX Too bad we need this here */

    nvListEntryPtr  entries; /* Top level entries */
    int             nentries;

    nvListEntryPtr  selected_entry;
    nvListEntryPtr  server_entry;
};


struct _nvDisplayDataRec {
    
    gpointer  handle; /* NV-CONTROL GPU Target */

    GtkWidget *label;

    guint      device_mask;

    GtkWidget *server_label;
    GtkWidget *server_checkbox;
    gboolean   masterable;

    GtkWidget *client_label;
    GtkWidget *client_checkbox;

    GtkWidget *rate_label;
    GtkWidget *rate_text;
    guint      rate;

    GtkWidget *stereo_label;
    GtkWidget *stereo_hbox; /* LED */
};

struct _nvGPUDataRec {

    gpointer   handle; /* NV-CONTROL GPU Target */

    guint      server_mask;
    guint      clients_mask;
    gboolean   enabled; /* Sync enabled */

    GtkWidget *timing_label;
    GtkWidget *timing_hbox; /* LED */

    /* Signal Handler IDs */
    gulong     signal_ids[NUM_GPU_SIGNALS];

    GtkWidget *label;
};

struct _nvFrameLockDataRec {

    gpointer   handle; /* NV-CONTROL Frame Lock Target */

    /* Signal Handler IDs */
    gulong     signal_ids[NUM_FRAMELOCK_SIGNALS];

    GtkWidget *label;

    GtkWidget *receiving_label;
    GtkWidget *receiving_hbox; /* LED */

    GtkWidget *rate_label;
    GtkWidget *rate_text;

    GtkWidget *delay_label;
    GtkWidget *delay_text;

    GtkWidget *house_label;
    GtkWidget *house_hbox; /* LED */

    GtkWidget *port0_label;
    GtkWidget *port0_hbox; /* IMAGE */
    guint      port0_ethernet_error;

    GtkWidget *port1_label;
    GtkWidget *port1_hbox; /* IMAGE */
    guint      port1_ethernet_error;

    GtkWidget *revision_label;
    GtkWidget *revision_text;

    GtkWidget *extra_info_hbox;
};


static gchar *houseFormatStrings[] = {
    "Composite, Auto",      /* VIDEO_MODE_COMPOSITE_AUTO */
    "TTL",                  /* VIDEO_MODE_TTL */
    "Composite, Bi-Level",  /* VIDEO_MODE_COMPOSITE_BI_LEVEL */
    "Composite, Tri-Level", /* VIDEO_MODE_COMPOSITE_TRI_LEVEL */
    NULL
    };

static gchar *syncEdgeStrings[] = {
    "",        /* None */
    "Rising",  /* NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE */
    "Falling", /* NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE */
    "Both",    /* NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES */
    NULL
    };


/* Tooltips */

static const char * __add_devices_button_help =
"The Add Devices button adds to the frame lock group all G-Sync devices found "
"on the specified X Server.";

static const char * __remove_devices_button_help =
"The Remove Devices button allows you to remove G-Sync, GPU or display "
"devices from the frame lock group.  Any device removed from the frame lock "
"group will no longer be controlled.";

static const char * __show_extra_info_button_help =
"The Show Extra Info button displays extra information and settings "
"for various devices.";

static const char * __use_house_sync_button_help =
"The Use House Sync if Present checkbox tells the server G-Sync device to "
"generate the master frame lock signal from the incoming house sync signal "
"(if a house sync signal is detected) instead of using internal timing from "
"the server GPU/display device.";

static const char * __sync_interval_entry_help =
"The Sync Interval entry allows you to set the number of incoming house sync "
"pulses the master frame lock board recieves before generating an outgoing "
"frame lock sync pulse.  A value of 0 means a frame lock sync pulse is sent "
"for every house sync pulse.";

static const char * __sync_edge_combo_help =
"The Sync Edge drop-down allows you to select which edge the master "
"frame lock device will use to decode the incoming house sync signal.";

static const char * __video_mode_combo_help =
"The Video Mode drop-down allows you to select which video mode the server "
"G-Sync device will use to decode the incoming house sync signal.";

static const char * __detect_video_mode_button_help =
"The Detect Video Mode button will attempt to automatically detect the format "
"of the house sync signal by iterating through the list of known video modes.";

static const char * __test_link_button_help =
"The Test Link button will cause the master frame lock device to output a "
"test signal for a short amount of time.  During this time, the Sync Signal "
"coming from the master frame lock device will be held high causing the rj45 "
"ports throughout the frame lock group to stop blinking.";

static const char * __sync_enable_button_help =
"The Enable/Disable Frame Lock button will enable/disable frame lock on all "
"devices listed in the G-Sync group.  Enabling frame lock will lock the "
"refresh rates of all members in the frame lock group.";

static const char * __server_checkbox_help =
"The Server checkbox sets which display device the underlying frame lock "
"device should use to generate the frame lock sync signal.  Only one display "
"device can be selected as server for a frame lock group.  To select another "
"display device, the display device currently set as server should be "
"unselected.";

static const char * __client_checkbox_help =
"The Client checkbox allows you to set whether or not this display device "
"will be synchronized to the incoming frame lock sync signal.";



static unsigned int add_framelock_devices(CtkFramelock *, gpointer);
static unsigned int add_gpu_devices(CtkFramelock *, nvListEntryPtr);
static unsigned int add_display_devices(CtkFramelock *, nvListEntryPtr);
static gint add_devices(CtkFramelock *, const gchar *, gboolean);

static GtkWidget *create_add_devices_dialog(CtkFramelock *);
static GtkWidget *create_remove_devices_dialog(CtkFramelock *);
static GtkWidget *create_enable_confirm_dialog(CtkFramelock *);

static void add_devices_response(GtkWidget *, gint, gpointer);
static void add_devices_repond_ok(GtkWidget *, gpointer);
static void remove_devices_response(GtkWidget *, gint, gpointer);

static void error_msg(CtkFramelock *, const gchar *, ...);

static void toggle_use_house_sync(GtkWidget *, gpointer);
static void toggle_extra_info(GtkWidget *, gpointer);
static void toggle_server(GtkWidget *, gpointer);
static void toggle_client(GtkWidget *, gpointer);
static void toggle_sync_enable(GtkWidget *, gpointer);
static void toggle_test_link(GtkWidget *, gpointer);
static void activate_sync_interval(GtkEntry *, gpointer);
static void changed_video_mode(GtkEditable *, gpointer);
static void toggle_detect_video_mode(GtkToggleButton *, gpointer);

static gboolean update_framelock_status(gpointer);
static gboolean check_for_ethernet(gpointer);

static void update_framelock_controls(CtkFramelock *);
static void update_house_sync_controls(CtkFramelock *);

static void apply_parsed_attribute_list(CtkFramelock *ctk_framelock,
                                        ParsedAttribute *list);

static void gpu_state_received(GtkObject *object,
                               gpointer arg1, gpointer user_data);
static void framelock_state_received(GtkObject *object,
                                     gpointer arg1, gpointer user_data);


#define SELECT_WIDGET(W, S)                                    \
    gtk_widget_modify_fg(GTK_WIDGET(W), GTK_STATE_NORMAL,      \
                         &((GTK_WIDGET(W))->style->text[S]));  \
    gtk_widget_modify_bg(GTK_WIDGET(W), GTK_STATE_NORMAL,      \
                         &((GTK_WIDGET(W))->style->base[S]));



/************************************************************************/

/*
 * Widget creation hepher functions
 */


/** create_error_msg_dialog() ****************************************
 *
 * Creates the error message dialog.  This dialog is used by various
 * parts of the GUI to report errors.
 *
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



/** create_sync_state_button() ***************************************
 *
 * Creates the enable/disable frame lock button.  This button has
 * two labels - one for each state it can be in such that an
 * informative icon.
 *
 */
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
                                    "enable frame lock");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Enable Frame Lock");

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
                                    "disable frame lock");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Disable Frame Lock");
    
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
    
    ctk_framelock->selected_syncing_label =
        ctk_framelock->enable_syncing_label;
    gtk_container_add(GTK_CONTAINER(button),
                      ctk_framelock->selected_syncing_label);

    return (button);
}



/** create_add_devices_dialog() **************************************
 *
 * Creates the dialog that will query for a server name from which
 * frame lock/gpu/display devices will be added to the current
 * frame lock group.
 *
 */
static GtkWidget *create_add_devices_dialog(CtkFramelock *ctk_framelock)
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
                      G_CALLBACK(add_devices_response),
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

    label = gtk_label_new("X Server:");
    descr = gtk_label_new("Please specify the X server to be added to the "
                          "frame lock group.");
    
    ctk_framelock->add_devices_entry = gtk_entry_new();
    
    g_signal_connect(G_OBJECT(ctk_framelock->add_devices_entry),
                     "activate", G_CALLBACK(add_devices_repond_ok),
                     (gpointer) ctk_framelock);

    gtk_entry_set_text(GTK_ENTRY(ctk_framelock->add_devices_entry),
                       NvCtrlGetDisplayName
                       (ctk_framelock->attribute_handle));
    
    gtk_entry_set_width_chars
        (GTK_ENTRY(ctk_framelock->add_devices_entry), 16);

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
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->add_devices_entry,
                       TRUE, TRUE, 0);
    
    return dialog;

}



/** create_remove_devices_dialog() ***********************************
 *
 * Creates the dialog that will query for a server name from which
 * frame lock/gpu/display devices will be added to the current
 * frame lock group.
 *
 */
static GtkWidget *create_remove_devices_dialog(CtkFramelock *ctk_framelock)
{
    GtkWidget *dialog;
    GtkWidget *hbox;
    GtkWidget *image;
    GdkPixbuf *pixbuf;
    GtkWidget *alignment;
    

    dialog = gtk_dialog_new_with_buttons("Remove Device(s)",
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
                     G_CALLBACK(remove_devices_response),
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

    ctk_framelock->remove_devices_label = gtk_label_new(NULL);
    
    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), image);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 2);

    alignment = gtk_alignment_new(0.0, 0.0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_framelock->remove_devices_label);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

    return dialog;
}



/** create_enable_confirm_dialog() ***********************************
 *
 * Creates the dialog that will confirm with the user when Frame Lock
 * is enabled without a server device specified.
 *
 */
static GtkWidget *create_enable_confirm_dialog(CtkFramelock *ctk_framelock)
{
    GtkWidget *dialog;
    GtkWidget *hbox;

    /* Display ModeSwitch confirmation dialog */
    dialog = gtk_dialog_new_with_buttons
        ("Confirm ModeSwitch",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_framelock))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
         | GTK_DIALOG_NO_SEPARATOR,
         GTK_STOCK_OK,
         GTK_RESPONSE_ACCEPT,
         NULL);

    ctk_framelock->enable_confirm_cancel_button =
        gtk_dialog_add_button(GTK_DIALOG(dialog),
                              GTK_STOCK_CANCEL,
                              GTK_RESPONSE_REJECT);

    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    /* Display confirm dialog text (Dynamically generated) */
    ctk_framelock->enable_confirm_text = gtk_label_new("");

    /* Add the text to the dialog */
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->enable_confirm_text,
                       TRUE, TRUE, 20);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
                       TRUE, TRUE, 20);
    gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

    return dialog;
}




/************************************************************************/

/*
 * Helper functions
 */



/** my_button_new_with_label() ***************************************
 *
 * Creates a button with padding.
 *
 */
GtkWidget *my_button_new_with_label(const gchar *txt,
                                    gint hpad,
                                    gint vpad)
{
    GtkWidget *btn;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    
    btn   = gtk_button_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    vbox  = gtk_vbox_new(FALSE, 0);
    label = gtk_label_new(txt);

    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, hpad);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, vpad);
    gtk_container_add(GTK_CONTAINER(btn), vbox);

    return btn;
}



/** my_toggle_button_new_with_label() ********************************
 *
 * Creates a toggle button with padding.
 *
 */
GtkWidget *my_toggle_button_new_with_label(const gchar *txt,
                                           gint hpad,
                                           gint vpad)
{
    GtkWidget *btn;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    
    btn   = gtk_toggle_button_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    vbox  = gtk_vbox_new(FALSE, 0);
    label = gtk_label_new(txt);

    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, hpad);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, vpad);
    gtk_container_add(GTK_CONTAINER(btn), vbox);

    return btn;
}



/** update_image() ***************************************************
 *
 * Updates the container to hold a duplicate of the given image.
 *
 */
void update_image(GtkWidget *container, GtkWidget *new_image)
{
    GList *list;

    list = gtk_container_get_children(GTK_CONTAINER(container));
    if (list) {
        gtk_container_remove(GTK_CONTAINER(container),
                             (GtkWidget *)(list->data));
        g_list_free(list);
    }
    gtk_box_pack_start(GTK_BOX(container),
                       ctk_image_dupe(GTK_IMAGE(new_image)),
                       FALSE, FALSE, 0);
    gtk_widget_show_all(container);
}



/** get_display_name() ***********************************************
 *
 * Returns the name of the given display device.
 *
 * If 'simple' is 0, then the display device type will be
 * included in the name returned.
 *
 */
static gchar *get_display_name(nvDisplayDataPtr data, gboolean simple)
{
    ReturnStatus  ret;
    char         *display_name;
    char         *display_type;
    char         *name;
    
    ret = NvCtrlGetStringDisplayAttribute(data->handle,
                                          data->device_mask,
                                          NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                          &display_name);
    if (ret != NvCtrlSuccess) {
        display_name = NULL;
    }

    display_type =
        display_device_mask_to_display_device_name(data->device_mask);

    if (simple) {
        name = g_strconcat(display_name?display_name:"Unknown Display",
                           NULL);
    } else {
        name = g_strconcat(display_name?display_name:"Unknown Display",
                           " (", display_type, ")", NULL);
    }

    if (display_name) {
        free(display_name);
    }

    return name;
}



/** get_gpu_name() ***************************************************
 *
 * Returns the name of the given GPU.
 *
 * If 'simple' is 0, then the GPU ID will be included in the name
 * returned.
 *
 */
static gchar *get_gpu_name(nvGPUDataPtr data, gboolean simple)
{
    ReturnStatus  ret;
    char         *product_name;
    char          tmp[32];
    char         *name;

    ret = NvCtrlGetStringAttribute(data->handle,
                                   NV_CTRL_STRING_PRODUCT_NAME,
                                   &product_name);
    if (ret != NvCtrlSuccess) {
        product_name = NULL;
    }

    snprintf(tmp, 32, " (GPU %d)", NvCtrlGetTargetId(data->handle));

    if (simple) {
        name = g_strconcat(product_name?product_name:"Unknown GPU",
                           NULL);
    } else {
        name = g_strconcat(product_name?product_name:"Unknown GPU",
                           tmp, NULL);
    }

    if (product_name) {
        free(product_name);
    }

    return name;
}



/** get_framelock_name() *********************************************
 *
 * Returns the name of the given frame lock (G-Sync) device.
 *
 */
static char *get_framelock_name(nvFrameLockDataPtr data, gboolean simple)
{
    char *server_name;
    char  tmp[32];
    char *name;

    /* NOTE: The display name of a non-X Screen target will
     *       return the server name and server # only.
     *       (ie, it does not return a screen #)
     */
    server_name = NvCtrlGetDisplayName(data->handle);

    snprintf(tmp, 32, " (G-Sync %d)", NvCtrlGetTargetId(data->handle));
    
    name = g_strconcat(server_name?server_name:"Unknown X Server", tmp, NULL);

    return name;
}



/** get_entry_label() ************************************************
 *
 * Returns the correct label for the given entry.
 *
 */
static gchar *get_entry_label(nvListEntryPtr entry, gboolean simple)
{
    char *str = NULL;

    if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
        str = get_framelock_name((nvFrameLockDataPtr)(entry->data), simple);

    } else if (entry->data_type == ENTRY_DATA_GPU) {
        str = get_gpu_name((nvGPUDataPtr)(entry->data), simple);

    } else if (entry->data_type == ENTRY_DATA_DISPLAY) {
        str = get_display_name((nvDisplayDataPtr)(entry->data), simple);

    }

    return str;
}



/** update_entry_label() *********************************************
 *
 * Sets the correct label for the given entry.
 *
 */
static void update_entry_label(CtkFramelock *ctk_framelock,
                               nvListEntryPtr entry)
{
    char *str = NULL;
    gboolean simple;

    simple = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->short_labels_button));

    if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
        str = get_framelock_name((nvFrameLockDataPtr)(entry->data), simple);
        gtk_label_set_text(GTK_LABEL
                           (((nvFrameLockDataPtr)(entry->data))->label),
                           str?str:"Unknown G-Sync");

    } else if (entry->data_type == ENTRY_DATA_GPU) {
        str = get_gpu_name((nvGPUDataPtr)(entry->data), simple);
        gtk_label_set_text(GTK_LABEL
                           (((nvGPUDataPtr)(entry->data))->label),
                           str?str:"Unknown GPU");

    } else if (entry->data_type == ENTRY_DATA_DISPLAY) {
        str = get_display_name((nvDisplayDataPtr)(entry->data), simple);
        gtk_label_set_text(GTK_LABEL
                           (((nvDisplayDataPtr)(entry->data))->label),
                           str?str:"Unknown Display");
    }
    
    if (str) {
        g_free(str);
    }
}



/** error_msg() ******************************************************
 *
 * Displays an error message dialog using the error message dialog.
 *
 */
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



/** show_remove_devices_dialog() *************************************
 *
 * Displays the remove devices dialog.
 *
 */
static void show_remove_devices_dialog(GtkWidget *button,
                                       CtkFramelock *ctk_framelock)
{
    nvListTreePtr tree;
    nvListEntryPtr entry;
    gchar *str = NULL, *label;

    tree = (nvListTreePtr)(ctk_framelock->tree);
    entry = tree->selected_entry;

    if (!entry) return;

    label = get_entry_label(entry, 0);
    if (!label) {
        str = g_strconcat("Would you like to remove the selected entry "
                          "from the group?"
                          "\n\nNOTE: This will also remove any entries "
                          "under this one.",
                          NULL);
    } else if (entry->nchildren) {
        str = g_strconcat("Would you like to remove the following entry "
                          "from the group?\n\n<span weight=\"bold\" "
                          "size=\"larger\">", label, "</span>",
                          "\n\nNOTE: This will also remove any entries "
                          "under this one.",
                          NULL);
        g_free(label);
    } else {
        str = g_strconcat("Would you like to remove the following entry "
                          "from the group?\n\n<span weight=\"bold\" "
                          "size=\"larger\">", label, "</span>",
                          NULL);
        g_free(label);
    }

    gtk_label_set_line_wrap(GTK_LABEL(ctk_framelock->remove_devices_label),
                            TRUE);
    gtk_label_set_use_markup(GTK_LABEL(ctk_framelock->remove_devices_label),
                             TRUE);
    if (str) {
        gtk_label_set_markup(GTK_LABEL(ctk_framelock->remove_devices_label),
                             str);
        g_free(str);
    }
    
    gtk_widget_show_all(ctk_framelock->remove_devices_dialog);
}



/** get_framelock_server_entry() *************************************
 *
 * Retrieves the frame lock list entry that is related to the currently
 * selected server (display) list entry, if any.
 *
 */
static nvListEntryPtr get_framelock_server_entry(nvListTreePtr tree)
{
    nvListEntryPtr entry;

    if (!tree || !tree->server_entry) {
        return NULL;
    }
    entry = tree->server_entry;
    while (entry) {
        if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
            return entry;
        }
        entry = entry->parent;
    }

    return NULL;
}



/** get_gpu_server_entry() *******************************************
 *
 * Retrieves the GPU list entry that is related to the currently
 * selected server (display) list entry, if any.
 *
 */
static nvListEntryPtr get_gpu_server_entry(nvListTreePtr tree)
{
    nvListEntryPtr entry;

    if (!tree || !tree->server_entry) {
        return NULL;
    }
    entry = tree->server_entry;
    while (entry) {
        if (entry->data_type == ENTRY_DATA_GPU) {
            return entry;
        }
        entry = entry->parent;
    }

    return NULL;
}



/** get_display_server_entry() ***************************************
 *
 * Retrieves the display list entry that is the currently selected
 * server.
 *
 */
static nvListEntryPtr get_display_server_entry(nvListTreePtr tree)
{
    return tree->server_entry;
}



/** list_entry_update_framelock_controls() ***************************
 *
 * Updates a G-Sync list entry's GUI controls based on the current
 * frame lock status.
 *
 */
static void list_entry_update_framelock_controls(CtkFramelock *ctk_framelock,
                                                 nvListEntryPtr entry)
{
    nvFrameLockDataPtr data = (nvFrameLockDataPtr)(entry->data);
    gboolean framelock_enabled = ctk_framelock->framelock_enabled;

    gboolean show_all =
        gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->extra_info_button));

    /* Show/Hide frame lock widgets */
    if (show_all) {
        gtk_widget_show(data->extra_info_hbox);
    } else {
        gtk_widget_hide(data->extra_info_hbox);
    }
    
    /* Activate Sync Rate when frame lock is enabled */
    gtk_widget_set_sensitive(data->rate_label, framelock_enabled);
    gtk_widget_set_sensitive(data->rate_text, framelock_enabled);
    
    /* Activate Sync Delay when frame lock is enabled */
    gtk_widget_set_sensitive(data->delay_label, framelock_enabled);
    gtk_widget_set_sensitive(data->delay_text, framelock_enabled);
}



/** list_entry_update_display_controls() *****************************
 *
 * Updates a display device list entry's GUI controls based on
 * current frame lock status.
 *
 */
static void list_entry_update_display_controls(CtkFramelock *ctk_framelock,
                                               nvListEntryPtr entry)
{
    nvDisplayDataPtr data = (nvDisplayDataPtr)(entry->data);
    gboolean framelock_enabled = ctk_framelock->framelock_enabled;
    gboolean server_checked;
    gboolean client_checked;
    gboolean sensitive;
    
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr server_entry = get_display_server_entry(tree);
    nvDisplayDataPtr server_data = NULL;
    

    if (server_entry) {
        server_data = (nvDisplayDataPtr)(server_entry->data);
        if (!server_data) {
            return; /* Oops */
        }
    }

    server_checked = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(data->server_checkbox));
    client_checked = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(data->client_checkbox));
    
    /* Server Checkbox is unavailable when framelock is disabled,
     * this display is set as client, this display cannot be master
     * (display is driven by GPU that is connected through a
     * secondary connector.), or another server is already selected.
     */
    sensitive = (!framelock_enabled &&
                 !client_checked &&
                 data->masterable &&
                 (!server_entry || data == server_entry->data));
    gtk_widget_set_sensitive(data->server_label, sensitive);
    gtk_widget_set_sensitive(data->server_checkbox, sensitive);
    
    /* When a server is selected, this display can only become a
     * client if its refresh rate matches that of the client.
     */
    sensitive = (!framelock_enabled &&
                 !server_checked &&
                 (!server_data || data->rate == server_data->rate));
    gtk_widget_set_sensitive(data->client_label, sensitive);
    gtk_widget_set_sensitive(data->client_checkbox, sensitive);

    /* Gray out the display device's refresh rate when it is not
     * the same as the current server's.
     */
    sensitive = (!server_data || data->rate == server_data->rate);
    gtk_widget_set_sensitive(data->rate_label, sensitive);
    gtk_widget_set_sensitive(data->rate_text, sensitive);
    gtk_widget_set_sensitive(data->label, sensitive);

    ctk_config_set_tooltip(entry->tree->ctk_framelock->ctk_config, entry->ebox,
                           sensitive ? NULL : "This display device cannot be "
                           "included in the frame lock group since it has a "
                           "different refresh rate than that of the server.");

    /* Remove display device from the GPU's clients list */
    if (!sensitive && gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(data->client_checkbox))) {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(data->client_checkbox),
             FALSE);
        ((nvGPUDataPtr)(entry->parent->data))->clients_mask &=
            data->device_mask;
    }
}



/** list_entry_update_controls() *************************************
 *
 * Updates the controls in the given entry list to reflect frame lock
 * sync status.  This function is used to disable access to some
 * widgets while frame lock sync is enabled.
 *
 */
static void list_entry_update_controls(CtkFramelock *ctk_framelock,
                                       nvListEntryPtr entry)
{
    if (!entry) return;

    list_entry_update_controls(ctk_framelock, entry->children);

    if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
        list_entry_update_framelock_controls(ctk_framelock, entry);
        
    } else if (entry->data_type == ENTRY_DATA_GPU) {
        /* Do nothing */
        
    } else if (entry->data_type == ENTRY_DATA_DISPLAY) {
        list_entry_update_display_controls(ctk_framelock, entry);
    }

    list_entry_update_controls(ctk_framelock, entry->next_sibling);
}



/** any_gpu_has_selection() ******************************************
 *
 * Returns TRUE if any of the gpus have a server/client selected
 *
 */
static gboolean any_gpu_has_selection(nvListEntryPtr entry)
{
    if (!entry) return FALSE;

    if (entry->data_type == ENTRY_DATA_GPU &&
        (((nvGPUDataPtr)(entry->data))->server_mask ||
         ((nvGPUDataPtr)(entry->data))->clients_mask)) {
        return TRUE;
    }

    if (any_gpu_has_selection(entry->children)) {
        return TRUE;
    }

    if (any_gpu_has_selection(entry->next_sibling)) {
        return TRUE;
    }

    return FALSE;
}



/** update_framelock_controls() **************************************
 *
 * Enable/disable access to various GUI controls on the frame lock
 * page depending on the state of frame lock sync (frame lock
 * enabled/disabled).  Also validates on client refresh rates
 * vs server refresh rate.
 *
 */
static void update_framelock_controls(CtkFramelock *ctk_framelock)
{
    nvListTreePtr tree;
    gboolean enabled;
    gboolean something_selected;


    tree = (nvListTreePtr)(ctk_framelock->tree);
    enabled = ctk_framelock->framelock_enabled;

    /* G-Sync Buttons */
    gtk_widget_set_sensitive(ctk_framelock->remove_devices_button,
                             tree->nentries);

    gtk_widget_set_sensitive(ctk_framelock->extra_info_button,
                             tree->nentries);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->sync_state_button),
         G_CALLBACK(toggle_sync_enable),
         (gpointer) ctk_framelock);
    
    something_selected =
        any_gpu_has_selection(((nvListTreePtr)ctk_framelock->tree)->entries);

    gtk_widget_set_sensitive(ctk_framelock->sync_state_button,
                             tree->nentries && something_selected);

    gtk_container_remove
        (GTK_CONTAINER(ctk_framelock->sync_state_button),
         ctk_framelock->selected_syncing_label);

    if (tree->nentries && enabled) {
        ctk_framelock->selected_syncing_label =
            ctk_framelock->disable_syncing_label;
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_framelock->sync_state_button), TRUE);
    } else {
        ctk_framelock->selected_syncing_label =
            ctk_framelock->enable_syncing_label;
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_framelock->sync_state_button), FALSE);
    }

    gtk_container_add(GTK_CONTAINER(ctk_framelock->sync_state_button),
                      ctk_framelock->selected_syncing_label);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->sync_state_button),
         G_CALLBACK(toggle_sync_enable),
         (gpointer) ctk_framelock);

    gtk_widget_show_all(ctk_framelock->sync_state_button);

    /* Test link */
    gtk_widget_set_sensitive(ctk_framelock->test_link_button,
                             (enabled && tree->server_entry));

    /* Update the frame lock G-Sync frame */
    list_entry_update_controls(ctk_framelock, tree->entries);

    /* House Sync */
    update_house_sync_controls(ctk_framelock);
}



/** get_display_on_gpu() *********************************************
 *
 * Returns the display list entry that matches the device mask and
 * is connected to the gpu list entry.
 *
 */
static nvListEntryPtr get_display_on_gpu(nvListEntryPtr gpu_entry,
                                         guint device_mask)
{
    nvListEntryPtr   display_entry;
    nvDisplayDataPtr display_data;
    
    if (!device_mask) {
        return NULL;
    }

    /* Gather bitmask of server/clients */
    display_entry = gpu_entry->children;
    for (display_entry = gpu_entry->children; display_entry;
         display_entry = display_entry->next_sibling) {

        if (display_entry->data_type != ENTRY_DATA_DISPLAY) {
            continue;
        }

        display_data = (nvDisplayDataPtr)(display_entry->data);
        if (display_data->device_mask & device_mask) {
            return display_entry;
        }
    }
    return NULL;
}



/** any_gpu_enabled() ************************************************
 *
 * Returns TRUE if any of the gpus have frame lock enabled.
 *
 */
static gboolean any_gpu_enabled(nvListEntryPtr entry)
{
    if (!entry) return FALSE;

    if (entry->data_type == ENTRY_DATA_GPU &&
        ((nvGPUDataPtr)(entry->data))->enabled) {
        return TRUE;
    }

    if (any_gpu_enabled(entry->children)) {
        return TRUE;
    }

    if (any_gpu_enabled(entry->next_sibling)) {
        return TRUE;
    }

    return FALSE;
}



/************************************************************************/

/*
 * List Entry functions
 */


/** do_select_framelock_data() ***************************************
 *
 * This function defines how to set all the widgets in a frame lock
 * row as "selected" or "not selected"
 *
 */
static void do_select_framelock_data(nvFrameLockDataPtr data, gint select)
{
    if (!data) {
        return;
    }
    SELECT_WIDGET(data->label, select);
    SELECT_WIDGET(data->receiving_label, select);
    SELECT_WIDGET(data->rate_label, select);
    SELECT_WIDGET(data->rate_text, select);
    SELECT_WIDGET(data->delay_label, select);
    SELECT_WIDGET(data->delay_text, select);
    SELECT_WIDGET(data->house_label, select);
    SELECT_WIDGET(data->port0_label, select);
    SELECT_WIDGET(data->port1_label, select);
}



/** do_select_gpu_data() *********************************************
 *
 * This function defines how to set all the widgets in a GPU
 * row as "selected" or "not selected"
 *
 */
static void do_select_gpu_data(nvGPUDataPtr data, gint select)
{
    if (!data) {
        return;
    }
    SELECT_WIDGET(data->label, select);
    SELECT_WIDGET(data->timing_label, select);
}



/** do_select_display_data() *****************************************
 *
 * This function defines how to set all the widgets in a display
 * device row as "selected" or "not selected"
 *
 */
static void do_select_display_data(nvDisplayDataPtr data, gint select)
{
    if (!data) {
        return;
    }
    SELECT_WIDGET(data->label, select);
    SELECT_WIDGET(data->server_label, select);
    SELECT_WIDGET(data->client_label, select);
    SELECT_WIDGET(data->rate_label, select);
    SELECT_WIDGET(data->rate_text, select);
    SELECT_WIDGET(data->stereo_label, select);
}



/** list_entry_set_select() ******************************************
 *
 * This function sets which entry in the list is selected.  If an
 * entry is already selected, it is unselected recursively.
 *
 */
static void list_entry_set_select(nvListEntryPtr entry, gint selected)
{
    gint state;

    if (!entry || !entry->tree) {
        return;
    }


    /* Do the selection */

    if (selected) {
        state = GTK_STATE_SELECTED;
        if (entry->tree->selected_entry) {
            /* Unselect previous entry */
            list_entry_set_select
                ((nvListEntryPtr) entry->tree->selected_entry, False);
        }
        entry->tree->selected_entry = (void *)entry;
    } else {
        state = GTK_STATE_NORMAL;
        entry->tree->selected_entry = NULL;
    }


    /* Update the state of the entry's widgets */
   
    SELECT_WIDGET(entry->ebox, state);

    if (!entry->data) {
        return;
    }
    switch (entry->data_type) {
    case ENTRY_DATA_FRAMELOCK:
        do_select_framelock_data((nvFrameLockDataPtr)(entry->data), state);
        break;
    case ENTRY_DATA_GPU:
        do_select_gpu_data((nvGPUDataPtr)(entry->data), state);
        break;
    case ENTRY_DATA_DISPLAY:
        do_select_display_data((nvDisplayDataPtr)(entry->data), state);
        break;
    default:
        break;
    }
}



/** list_entry_clicked() *********************************************
 *
 * Called on the entry that the user clicked on.
 *
 */
static void list_entry_clicked(GtkWidget *widget, GdkEventButton *event,
                               gpointer user_data)
{
    nvListEntryPtr entry = (nvListEntryPtr)(user_data);

    if (!entry || !entry->tree) {
        return;
    }
    if (entry != entry->tree->selected_entry) {
        list_entry_set_select(entry, True);
    }
}



/** expander_button_clicked() ****************************************
 *
 * - Handles button clicks on an nvListEntry's expantion button
 *   widget.
 *
 *   This function either shows or hides the list entry's children
 *   based on the state of the expansion widget.
 *
 */
static void expander_button_clicked(GtkWidget *widget, gpointer user_data)
{
    nvListEntryPtr entry = (nvListEntryPtr) user_data;


    if (entry->expanded) {
        /* Collapse */
        gtk_container_remove(GTK_CONTAINER(entry->expander_button),
                             entry->expander_button_image);
        entry->expander_button_image =
            gtk_image_new_from_stock(GTK_STOCK_ADD,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_set_size_request(entry->expander_button, 20, 20);
        
        gtk_container_add(GTK_CONTAINER(entry->expander_button),
                          entry->expander_button_image);
        gtk_widget_show_all(entry->expander_button);
        gtk_widget_hide(entry->child_vbox);
        
    } else {
        /* Expand */
        gtk_container_remove(GTK_CONTAINER(entry->expander_button),
                             entry->expander_button_image);
        entry->expander_button_image =
            gtk_image_new_from_stock(GTK_STOCK_REMOVE,
                                     GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_widget_set_size_request(entry->expander_button, 20, 20);

        gtk_container_add(GTK_CONTAINER(entry->expander_button),
                          entry->expander_button_image);
        gtk_widget_show_all(entry->expander_button);
        gtk_widget_show(entry->child_vbox);
    }

    entry->expanded = !(entry->expanded);
}



/** list_entry_add_expander_button() *********************************
 *
 * - Adds a button to the left of a list entry's main data row.
 *
 *   This button is used to show/hide the list entry's children.
 *
 */
static void list_entry_add_expander_button(nvListEntryPtr entry)
{
    if (!entry || entry->expander_button) {
        return;
    }
    
    entry->expander_vbox = gtk_vbox_new(FALSE, 0);
    entry->expander_button = gtk_button_new();
    entry->expander_button_image =
        gtk_image_new_from_stock(GTK_STOCK_REMOVE,
                                 GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_size_request(entry->expander_button, 20, 20);    
    entry->expanded = True;
    
    g_signal_connect(G_OBJECT(entry->expander_button), "clicked",
                     G_CALLBACK(expander_button_clicked),
                     (gpointer) entry);
    
    gtk_container_add(GTK_CONTAINER(entry->expander_button),
                      entry->expander_button_image);
    gtk_box_pack_start(GTK_BOX(entry->expander_vbox), entry->expander_button,
                       TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(entry->expander_hbox), entry->expander_vbox,
                       FALSE, FALSE, 0);
}



/** list_entry_remove_expander_button() ******************************
 *
 * - Removes the button that is to the left of a list entry's main
 *   data row.
 *
 *   When a list entry has no more children, the expander button
 *   should be removed.
 *
 */
static void list_entry_remove_expander_button(nvListEntryPtr entry)
{
    if (!entry || !entry->expander_button) {
        return;
    }

    gtk_container_remove(GTK_CONTAINER(entry->expander_hbox),
                         entry->expander_vbox);
    entry->expander_button = NULL;
}



/** list_entry_new() *************************************************
 *
 * - Creates and returns a list entry.  List entries are how rows of
 *   a tree keep their parent-child relationship.
 *
 */
static nvListEntryPtr list_entry_new(void)
{
    nvListEntryPtr entry;

    entry = (nvListEntryPtr) calloc(1, sizeof(nvListEntryRec));
    if (!entry) {
        return NULL;
    }

    /* Create the vertical box that holds this entry and its children */
    entry->vbox = gtk_vbox_new(FALSE, 0);

    /* Create the (top) row that holds this entry's data */
    entry->ebox = gtk_event_box_new();
    entry->hbox = gtk_hbox_new(FALSE, 15);
    entry->title_hbox = gtk_hbox_new(FALSE, 0);
    entry->padding_hbox = gtk_hbox_new(FALSE, 0);
    entry->expander_hbox = gtk_hbox_new(FALSE, 0);
    entry->label_hbox = gtk_hbox_new(FALSE, 0);
    entry->data_hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(entry->title_hbox), entry->padding_hbox,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(entry->title_hbox), entry->expander_hbox,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(entry->title_hbox), entry->label_hbox,
                       FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(entry->hbox), entry->title_hbox,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(entry->hbox), entry->data_hbox,
                       FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(entry->ebox), GTK_WIDGET(entry->hbox));
    gtk_box_pack_start(GTK_BOX(entry->vbox), entry->ebox, TRUE, TRUE, 0);

    SELECT_WIDGET(entry->ebox, GTK_STATE_NORMAL);
    gtk_widget_set_events(entry->ebox, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(G_OBJECT(entry->ebox), "button_press_event",
                     G_CALLBACK(list_entry_clicked),
                     (gpointer)entry);

    return entry;
}



/** list_entry_free() ************************************************
 *
 * - Frees an existing list entry.
 *
 */
static void list_entry_free(nvListEntryPtr entry)
{
    int i;


    if (!entry) {
        return;
    }

    /* XXX DON"T Need these?
    gtk_widget_destroy(entry->vbox);
    gtk_widget_destroy(entry->title_hbox);
    gtk_widget_destroy(entry->data_hbox);
    gtk_widget_destroy(entry->ebox);
    */

    /* Remove signal callbacks */

    if (entry->ctk_event) {
        if (entry->data_type == ENTRY_DATA_GPU) {
            
            nvGPUDataPtr data = (nvGPUDataPtr) entry->data;
            
            for (i = 0; i < NUM_GPU_SIGNALS; i++) {
                if (g_signal_handler_is_connected(G_OBJECT(entry->ctk_event),
                                                  data->signal_ids[i])) {
                    g_signal_handler_disconnect(G_OBJECT(entry->ctk_event),
                                                data->signal_ids[i]);
                }
            }
            
        } else if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
            
            nvFrameLockDataPtr data = (nvFrameLockDataPtr) entry->data;
            
            for (i = 0; i < NUM_FRAMELOCK_SIGNALS; i++) {
                if (g_signal_handler_is_connected(G_OBJECT(entry->ctk_event),
                                                  data->signal_ids[i])) {
                    g_signal_handler_disconnect(G_OBJECT(entry->ctk_event),
                                                data->signal_ids[i]);
                }
            }
        }

        /* XXX We should probably free/destroy the ctk_event objects here */
    }

    free(entry);
}



/** list_entry_add_child() *******************************************
 *
 * - Adds the given child list entry to the parent list entry.
 *   If this is the first child to be added, an expansion button will
 *   be created for the parent.
 *
 */
static void list_entry_add_child(nvListEntryPtr parent, nvListEntryPtr child)
{
    nvListEntryPtr  entry;

    if (!parent || !child) {
        return;
    }

    /* Add the child into the paren't child list */
    
    child->parent = parent;
    child->tree   = parent->tree;
    if (!parent->children) {
        parent->children = child;
    } else {
        entry = parent->children;
        while (entry->next_sibling) {
            entry = entry->next_sibling;
        }
        entry->next_sibling = child;
    }

    /* If this is the parent's first child, create the expansion button
     * and child box that will hold the children.
     */

    parent->nchildren++;
    if (parent->nchildren == 1) {
        /* Create the child box */

        parent->child_vbox = gtk_vbox_new(FALSE, 0);

        gtk_box_pack_start(GTK_BOX(parent->vbox),
                           parent->child_vbox, FALSE, FALSE, 0);
        gtk_widget_show(parent->child_vbox);

        /* Create the expansion button */

        list_entry_add_expander_button(parent);
        gtk_widget_show(parent->expander_button);
    }

    /* Pack the child into the parent's child box */

    gtk_box_pack_start(GTK_BOX(parent->child_vbox),
                       child->vbox, FALSE, FALSE, 0);
}



/** list_entry_associate() *******************************************
 *
 * - Associates an entry (and all its children) to a tree (or no
 *   tree).  Also makes sure that the tree being unassociated
 *   no longer references the entry (or any of its children).
 *
 * - Mainly, this is a helper function for adding and removing
 *   branches from trees.
 *
 */
static void list_entry_associate(nvListEntryPtr entry, nvListTreePtr tree)
{
    nvListEntryPtr child;

    if (!entry) {
        return;
    }
    
    /* Remove references to the entry from the old tree */
    if (entry->tree && (entry->tree != tree)) {

        /* Unselect ourself */
        if (entry == entry->tree->selected_entry) {
            entry->tree->selected_entry = NULL;
        }

        /* Remove master entry */
        if (entry == entry->tree->server_entry) {
            entry->tree->server_entry = NULL;
        }
    }

    /* Associate entry to the new tree */
    entry->tree = tree;

    /* Associate entry's children to the new tree */
    child = entry->children;
    while ( child ) {

        list_entry_associate(child, tree);
        child = child->next_sibling;
    }
}



/** list_entry_unparent() ********************************************
 *
 * - Removes the given child entry from its parent.  If this is the
 *   last child to be removed, the parent's expansion button will be
 *   removed.
 *
 */
static void list_entry_unparent(nvListEntryPtr child)
{
    nvListEntryPtr entry;
    nvListEntryPtr prev;
    nvListEntryPtr parent;
   
    if (!child || !child->parent) {
        return;
    }

    /* Find the child in the parent list */

    parent = child->parent;
    entry = parent->children;
    prev  = parent;
    while (entry) {
        if (entry == child) {
            break;
        }
        prev  = entry;
        entry = entry->next_sibling;
    }
    if (!entry) {
        return; /* Child not found! */
    }

    /* Remove the child from the parent list */
    if (prev == parent) {
        parent->children = entry->next_sibling;
    } else {
        prev->next_sibling = entry->next_sibling;
    }
    list_entry_associate(child, NULL);
    child->parent = NULL;

    /* Unpack the child from the parent's child box */

    gtk_container_remove(GTK_CONTAINER(parent->child_vbox), child->vbox);

    /* If this was the parent's last child, remove the expansion button
     * and the child boxes used to hold children.
     */

    parent->nchildren--;
    if (parent->nchildren == 0) {
        gtk_container_remove(GTK_CONTAINER(parent->vbox), parent->child_vbox);
        parent->child_vbox = NULL;
        list_entry_remove_expander_button(parent);
    }
}



/** list_entry_remove_children() *************************************
 *
 * - Removes all children from the given list entry.  This call is
 *   recursive (children's children will also be removed)
 *
 */
static void list_entry_remove_children(nvListEntryPtr entry)
{
    while (entry->children) {
        nvListEntryPtr child = entry->children;

        /* Remove this child's children. */
        list_entry_remove_children(child);

        /* Unparent this child and free it */
        list_entry_unparent(child);
        list_entry_free(child);
    }
}



/** list_entry_new_with_framelock() **********************************
 *
 * - Creates a new list entry that will hold the given frame lock
 *   data.
 *
 */
static nvListEntryPtr list_entry_new_with_framelock(nvFrameLockDataPtr data)
{
    nvListEntryPtr entry;

    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *vseparator;
    GtkWidget *padding;


    entry = list_entry_new();
    if (!entry) {
        return NULL;
    }
    entry->data = (gpointer)(data);
    entry->data_type = ENTRY_DATA_FRAMELOCK;

    /* Pack the data's widgets into the list entry data hbox */
    
    gtk_box_pack_start(GTK_BOX(entry->label_hbox), data->label,
                       FALSE, FALSE, 5);

    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 5);
    padding = gtk_hbox_new(FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    
    gtk_box_pack_end(GTK_BOX(entry->data_hbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->receiving_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->receiving_label, FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->rate_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->rate_text, FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->house_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->house_label, FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->port0_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->port0_label, FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->port1_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->port1_label, FALSE, FALSE, 0);

    /* Extra Info Section */

    gtk_box_pack_start(GTK_BOX(hbox), data->extra_info_hbox, FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), vseparator,
                       FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), data->delay_label,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), data->delay_text,
                       FALSE, FALSE, 0);

    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), vseparator,
                       FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), data->revision_label,
                       FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(data->extra_info_hbox), data->revision_text,
                       FALSE, FALSE, 0);

    gtk_box_pack_end(GTK_BOX(hbox), padding, FALSE, FALSE, 0);

    return entry;
}



/** list_entry_new_with_gpu() ****************************************
 *
 * - Creates a new list entry that will hold the given gpu data.
 *
 */
static nvListEntryPtr list_entry_new_with_gpu(nvGPUDataPtr data)
{
    nvListEntryPtr entry;

    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *padding;


    entry = list_entry_new();
    if (!entry) {
        return NULL;
    }
    entry->data = (gpointer)(data);
    entry->data_type = ENTRY_DATA_GPU;

    /* Pack the data's widgets into the list entry data hbox */
    
    gtk_box_pack_start(GTK_BOX(entry->label_hbox), data->label,
                       FALSE, FALSE, 5);

    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 5);
    padding = gtk_hbox_new(FALSE, 0);
    
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);
    
    gtk_box_pack_end(GTK_BOX(entry->data_hbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->timing_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->timing_label, FALSE, FALSE, 0);
    
    gtk_box_pack_end(GTK_BOX(hbox), padding, FALSE, FALSE, 0);

    return entry;
}



/** list_entry_new_with_display() ************************************
 *
 * - Creates a new list entry that will hold the given display data.
 *
 */
static nvListEntryPtr list_entry_new_with_display(nvDisplayDataPtr data)
{
    nvListEntryPtr entry;

    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *vseparator;
    GtkWidget *padding;


    entry = list_entry_new();
    if (!entry) {
        return NULL;
    }
    entry->data = (gpointer)(data);
    entry->data_type = ENTRY_DATA_DISPLAY;

   /* Pack the data's widgets into the list entry data hbox */
    
    gtk_box_pack_start(GTK_BOX(entry->label_hbox), data->label,
                       FALSE, FALSE, 5);

    frame = gtk_frame_new(NULL);
    hbox = gtk_hbox_new(FALSE, 5);
    padding = gtk_hbox_new(FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(hbox), 2);

    gtk_box_pack_end(GTK_BOX(entry->data_hbox), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->stereo_hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->stereo_label, FALSE, FALSE, 0);
    
    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->rate_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->rate_text, FALSE, FALSE, 0);
    
    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->server_checkbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->server_label, FALSE, FALSE, 0);
    
    vseparator = gtk_vseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), vseparator, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), data->client_checkbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), data->client_label, FALSE, FALSE, 0);
    
    gtk_box_pack_end(GTK_BOX(hbox), padding, FALSE, FALSE, 0);

    return entry;
}



/************************************************************************/

/*
 * functions relating to List Trees
 */


/** list_tree_new() **************************************************
 *
 * - Creates a new list tree that will hold list entries.
 *
 */
static nvListTreePtr list_tree_new(CtkFramelock *ctk_framelock)
{
    nvListTreePtr tree;

    tree = (nvListTreePtr) calloc(1, sizeof(nvListTreeRec));
    if (!tree) {
        return NULL;
    }

    tree->vbox = gtk_vbox_new(FALSE, 0);
    tree->ctk_framelock = ctk_framelock;

    return tree;
}



/** list_tree_add_entry() ********************************************
 *
 * - Adds a list entry to the tree list
 *
 */
static void list_tree_add_entry(nvListTreePtr tree, nvListEntryPtr entry)
{
    nvListEntryPtr e;

    if (!tree || !entry || entry->tree) {
        return;
    }
    entry->tree         = tree;
    entry->next_sibling = NULL;

    /* Add entry to the end of the list */
    if (!tree->entries) {
        tree->entries = entry;
    } else {
        e = tree->entries;
        while (e->next_sibling) {
            e = e->next_sibling;
        }
        e->next_sibling = entry;
    }
    tree->nentries++;

    list_entry_associate(entry, tree);

    gtk_box_pack_start(GTK_BOX(tree->vbox), entry->vbox, FALSE, FALSE, 5);
    gtk_widget_show_all(GTK_WIDGET(entry->vbox));
}



/** list_tree_remove_entry() *****************************************
 *
 * - Removes a list entry from the tree list.
 *
 */
static void list_tree_remove_entry(nvListTreePtr tree, nvListEntryPtr entry)
{
    nvListEntryPtr parent;

    if (!tree || !entry) {
        return;
    }

    /* Remove all children from the entry */
    list_entry_remove_children(entry);

    /* Separate entry from its parent */
    parent = entry->parent;
    if (parent) {

        /*
         * This is not a top-level entry so just
         * remove it from its parent
         */
        
        list_entry_unparent(entry);

    } else {

        /*
         * This is a top-level entry, so remove it from
         *  the tree.
         */

        /* Find and remove entry from the list */
        if (tree->entries == entry) {
            tree->entries = entry->next_sibling;
        } else {
            nvListEntryPtr e = tree->entries;
            while (e && e->next_sibling != entry) {
                e = e->next_sibling;
            }
            if (!e || e->next_sibling != entry) {
                return; /* Entry not found in tree! */
            }
            e->next_sibling = entry->next_sibling;
        }
        entry->next_sibling = NULL;
        tree->nentries--;

        list_entry_associate(entry, NULL);
    
        gtk_container_remove(GTK_CONTAINER(tree->vbox), entry->vbox);
    }

    /* Get rid of the entry */
    list_entry_free(entry);

    /* Remove parent if we were the last child */
    if (parent && !parent->children) {
        list_tree_remove_entry(tree, parent);
    }
}



/** list_entry_setup_title() *****************************************
 *
 * Returns the max width
 *
 */
static int list_entry_setup_title(nvListEntryPtr entry, int depth)
{
    int max_width;
    int width;
    GtkRequisition req;

    if (!entry) return FALSE;

    /* Setup this entry's padding */
    gtk_widget_set_size_request(entry->padding_hbox, depth * 25, -1);

    /* Calculate this entry's width */
    gtk_widget_size_request(entry->title_hbox, &req);
    max_width = req.width;

    width = list_entry_setup_title(entry->children, depth +1);
    max_width = (width > max_width) ? width : max_width;

    width = list_entry_setup_title(entry->next_sibling, depth);
    max_width = (width > max_width) ? width : max_width;

    return max_width;
}



/** list_entry_set_title() *******************************************
 *
 * Sets the width of the titles
 *
 */
static void list_entry_set_title(nvListEntryPtr entry, int width)
{
    if (!entry) return;

    /* Set this entry's title width */
    gtk_widget_set_size_request(entry->title_hbox, width, -1);

    list_entry_set_title(entry->children, width);
    list_entry_set_title(entry->next_sibling, width);
}



/** list_tree_align_titles() *****************************************
 *
 * - Aligns the titles and sets up the padding of all the tree's
 *   entries.
 *
 */
static void list_tree_align_titles(nvListTreePtr tree)
{
    int max_width;

    /* Setup the left padding and calculate the max width
     * of the tree entries
     */
    max_width = list_entry_setup_title(tree->entries, 0);

    /* Make sure all entry titles are the same width */
    list_entry_set_title(tree->entries, max_width);
}



/** find_server_by_name() ********************************************
 *
 * - Looks in the list tree for a list entry with a handle to a
 *   server with the name 'server_name'.  The first list entry found
 *   with a handle to the named server is returned.
 *
 */
static nvListEntryPtr find_server_by_name(nvListTreePtr tree,
                                          gchar *server_name)
{
    nvListEntryPtr entry;

    entry = tree->entries;
    while (entry) {
        gchar *name;

        switch (entry->data_type) {
        case ENTRY_DATA_FRAMELOCK:
            name = NvCtrlGetDisplayName
                (((nvFrameLockDataPtr)(entry->data))->handle);
            break;
        case ENTRY_DATA_GPU:
            name = NvCtrlGetDisplayName
                (((nvGPUDataPtr)(entry->data))->handle);
            break;
        case ENTRY_DATA_DISPLAY:
            name = NvCtrlGetDisplayName
                (((nvDisplayDataPtr)(entry->data))->handle);
            break;
        default:
            name = NULL;
        break;
        }
        
        if (name && !strcasecmp(server_name, name)){
            free(name);
            return entry;
        }
        free(name);
        
        entry = entry->next_sibling;
    }

    return entry;
}



/** find_entry_by_name() *********************************************
 *
 * - Looks in the list tree for the first list entry to have a handle
 *   to the given server name with the given entry data type and
 *   target id.
 *
 */
static nvListEntryPtr find_entry_by_name(nvListEntryPtr entry,
                                         gchar *server_name,
                                         int entry_type,
                                         int entry_id
                                         )
{
    nvListEntryPtr found_entry = NULL;

    if (!entry) return NULL;

    /* Check this entry */

    if ((entry->data_type == ENTRY_DATA_FRAMELOCK &&
         entry_type == NV_CTRL_TARGET_TYPE_FRAMELOCK) ||
        (entry->data_type == ENTRY_DATA_GPU &&
         entry_type == NV_CTRL_TARGET_TYPE_GPU)) {
        gpointer  handle;

        switch (entry->data_type) {
        case ENTRY_DATA_FRAMELOCK:
            handle = ((nvFrameLockDataPtr)(entry->data))->handle;
            break;
        case ENTRY_DATA_GPU:
            handle = ((nvGPUDataPtr)(entry->data))->handle;
            break;
        default:
            handle = NULL;
            break;
        }

        if (handle) {
            gchar  *name = NvCtrlGetDisplayName(handle);
            int     id   = NvCtrlGetTargetId(handle);
            
            if (name && !strcasecmp(server_name, name) &&
                id == entry_id) {
                free(name);
                return entry;
            }
            free(name);
        }
    }

    /* Check children */

    found_entry = find_entry_by_name(entry->children,
                                     server_name,
                                     entry_type,
                                     entry_id);
    if (found_entry) return found_entry;

    /* Check siblings */

    found_entry = find_entry_by_name(entry->next_sibling,
                                     server_name,
                                     entry_type,
                                     entry_id);
    if (found_entry) return found_entry;

    /* Entry not found in this branch */

    return NULL;
}



/**************************************************************************/

/*
 * Widget event and helper functions
 */


/** toggle_use_house_sync() ******************************************
 *
 * Callback function for the 'use house sync' button.
 * This button allows access to other house sync settings.
 *
 */
static void toggle_use_house_sync(GtkWidget *widget, gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    nvListEntryPtr entry;
    gboolean enabled;
    nvFrameLockDataPtr data;

    entry = get_framelock_server_entry((nvListTreePtr)(ctk_framelock->tree));
    if (!entry) return;

    data = (nvFrameLockDataPtr)(entry->data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(data->handle, NV_CTRL_USE_HOUSE_SYNC, enabled);

    update_house_sync_controls(ctk_framelock);

    NvCtrlGetAttribute(data->handle, NV_CTRL_USE_HOUSE_SYNC, &enabled);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "%s use of house sync signal.",
                                 (enabled ? "Enabled" : "Disabled"));
}



/** toggle_extra_info() **********************************************
 *
 * Callback function for the 'show all info' button.
 * This button shows/hides extra information from the
 * frame lock list entries.
 *
 */
static void toggle_extra_info(GtkWidget *widget, gpointer data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(data);
    gboolean enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    gtk_button_set_label(GTK_BUTTON(widget),
                         enabled?"Hide Extra Info":"Show Extra Info");

    update_framelock_controls(ctk_framelock);
    
    update_framelock_status(ctk_framelock);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "%s extra information.",
                                 (enabled ? "Showing" : "Hiding"));
}



/** toggle_server() **************************************************
 *
 * Callback function when a user toggles the 'server' checkbox of
 * a display device.
 *
 */
static void toggle_server(GtkWidget *widget, gpointer data)
{
    nvListEntryPtr entry = (nvListEntryPtr)data;
    nvDisplayDataPtr display_data = (nvDisplayDataPtr)(entry->data);
    nvGPUDataPtr gpu_data = (nvGPUDataPtr)(entry->parent->data);
    gboolean server_checked;

    if (entry->data_type != ENTRY_DATA_DISPLAY) {
        return;
    }

    server_checked = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(display_data->server_checkbox));

    entry->tree->server_entry = (server_checked ? entry : NULL);
    gpu_data->server_mask = (server_checked ? display_data->device_mask : 0);

    /* Update X server state, making sure FrameLock sync is disabled */
    NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_SYNC,
                       NV_CTRL_FRAMELOCK_SYNC_DISABLE);

    NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_MASTER,
                       gpu_data->server_mask);

    gpu_data->enabled = FALSE;
    entry->tree->ctk_framelock->framelock_enabled = 
        any_gpu_enabled(entry->tree->entries);

    /* Update GUI state */
    update_framelock_controls(entry->tree->ctk_framelock);

    ctk_config_statusbar_message(entry->tree->ctk_framelock->ctk_config,
                                 "%s frame lock server device.",
                                 (server_checked ? "Selected" : "Unselected"));
}



/** toggle_client() **************************************************
 *
 * Callback function when a user toggles the 'client' checkbox of
 * a display device.
 *
 */
static void toggle_client(GtkWidget *widget, gpointer data)
{
    nvListEntryPtr entry = (nvListEntryPtr)data;
    nvDisplayDataPtr display_data = (nvDisplayDataPtr)(entry->data);
    nvGPUDataPtr gpu_data = (nvGPUDataPtr)(entry->parent->data);
    gboolean client_checked;

    if (entry->data_type != ENTRY_DATA_DISPLAY) {
        return;
    }

    client_checked = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(display_data->client_checkbox));
    
    if (client_checked) {
        gpu_data->clients_mask |= display_data->device_mask;
    } else {
        gpu_data->clients_mask &= ~(display_data->device_mask);
    }

    /* Update X server state, make sure FrameLock Sync is disabled */
    NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_SYNC,
                       NV_CTRL_FRAMELOCK_SYNC_DISABLE);

    NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_SLAVES,
                       gpu_data->clients_mask);

    gpu_data->enabled = FALSE;
    entry->tree->ctk_framelock->framelock_enabled = 
        any_gpu_enabled(entry->tree->entries);

    /* Update GUI state */
    update_framelock_controls(entry->tree->ctk_framelock);

    ctk_config_statusbar_message(entry->tree->ctk_framelock->ctk_config,
                                 "%s frame lock client device.",
                                 (client_checked ? "Selected" : "Unselected"));
}



/** set_enable_sync_server() *****************************************
 *
 * Function to enable/disable frame lock sync on the server gpu
 * device.
 *
 * This function returns TRUE if something was enabled.
 *
 */
static gboolean set_enable_sync_server(nvListTreePtr tree, gboolean enable)
{
    nvListEntryPtr entry = get_gpu_server_entry(tree);
    nvGPUDataPtr data;
    ReturnStatus ret;

    if (!entry) return FALSE;

    data = (nvGPUDataPtr)(entry->data);

    ret = NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC, enable);
    if (ret != NvCtrlSuccess) return FALSE;

    ret = NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC, &enable);
    if (ret != NvCtrlSuccess) return FALSE;

    data->enabled = enable;

    return enable;
}



/** set_enable_sync_client() *****************************************
 *
 * Function to enable/disable frame lock sync on a client gpu device.
 *
 * This function returns TRUE if something was enabled.
 *
 */
static gboolean set_enable_sync_clients(nvListEntryPtr entry_list,
                                        gboolean enable)
{
    nvListEntryPtr entry;
    nvListEntryPtr server_gpu_entry;

    gint framelock_enabled = 0;
    gint something_enabled = 0;
    ReturnStatus ret;


    if (!entry_list) return FALSE;

    /* Get the server GPU entry */

    server_gpu_entry = get_gpu_server_entry(entry_list->tree);

    /* Go through all entries and activate/disable all entries that
     * aren't the server.
     */
    
    for (entry = entry_list; entry; entry = entry->next_sibling) {
        nvGPUDataPtr data;

        if (entry->children) {
            something_enabled = set_enable_sync_clients(entry->children,
                                                        enable);
            framelock_enabled = (framelock_enabled || something_enabled);
        }
        
        if (entry == server_gpu_entry || entry->data_type != ENTRY_DATA_GPU) {
            continue;
        }

        data = (nvGPUDataPtr)(entry->data);
        
        /* Only send protocol if there is something to enable */
        if (!data->clients_mask) continue;

        ret = NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC, enable);
        if (ret != NvCtrlSuccess) continue;

        ret = NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC,
                                 &(something_enabled));
        if (ret != NvCtrlSuccess) continue;

        data->enabled = something_enabled;
        framelock_enabled = framelock_enabled ? 1 : something_enabled;
    }

    return framelock_enabled?TRUE:FALSE;
}



/** update_enable_confirm_text() *************************************
 *
 * Generates the text used in the confirmation dialog.
 *
 **/
static void update_enable_confirm_text(CtkFramelock *ctk_framelock)
{
    gchar *str;
    str = g_strdup_printf("Frame Lock has been enabled but no server\n"
                          "device was selected.  Would you like to keep\n"
                          "the current settings?\n"
                          "\n"
                          "Disabling Frame Lock in %d seconds...",
                          ctk_framelock->enable_confirm_countdown);
    gtk_label_set_text(GTK_LABEL(ctk_framelock->enable_confirm_text), str);
    g_free(str);
}



/** do_enable_confirm_countdown() ************************************
 *
 * Timeout callback for reverting enabling of Frame Lock.
 *
 **/
static gboolean do_enable_confirm_countdown(gpointer *user_data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *) user_data;

    ctk_framelock->enable_confirm_countdown--;
    if (ctk_framelock->enable_confirm_countdown > 0) {
        update_enable_confirm_text(ctk_framelock);
        return True;
    }

    /* Force dialog to cancel */
    gtk_dialog_response(GTK_DIALOG(ctk_framelock->enable_confirm_dialog),
                        GTK_RESPONSE_REJECT);

    return False;
}



/** confirm_serverless_framelock() ***********************************
 *
 * Confirms with the user that Frame Lock has been enabled properly
 * in the case where no server was found in the configuration.
 *
 */
static Bool confirm_serverless_framelock(CtkFramelock *ctk_framelock)
{ 
    gint result;


    /* Start the countdown timer */
    ctk_framelock->enable_confirm_countdown = DEFAULT_ENABLE_CONFIRM_TIMEOUT;
    update_enable_confirm_text(ctk_framelock);
    ctk_framelock->enable_confirm_timer =
        g_timeout_add(1000,
                      (GSourceFunc)do_enable_confirm_countdown,
                      (gpointer)(ctk_framelock));

    /* Show the confirm dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_framelock->enable_confirm_dialog),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_framelock))));
    gtk_widget_show_all(ctk_framelock->enable_confirm_dialog);
    gtk_widget_grab_focus(ctk_framelock->enable_confirm_cancel_button);

    result =
        gtk_dialog_run(GTK_DIALOG(ctk_framelock->enable_confirm_dialog));
    gtk_widget_hide(ctk_framelock->enable_confirm_dialog);

    /* Kill the timer */
    g_source_remove(ctk_framelock->enable_confirm_timer);

    return (result == GTK_RESPONSE_ACCEPT);
}



/** toggle_sync_enable() *********************************************
 *
 * Callback function when a user toggles the 'Enable Frame Lock'
 * button.
 *
 */
static void toggle_sync_enable(GtkWidget *button, gpointer data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *) data;
    guint val;
    gboolean enabled;
    gboolean something_enabled;
    gboolean framelock_enabled = FALSE;
    gboolean server_enabled = FALSE;
    nvListTreePtr tree = (nvListTreePtr) ctk_framelock->tree;
    nvListEntryPtr entry;


    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    if (enabled) val = NV_CTRL_FRAMELOCK_SYNC_ENABLE;
    else         val = NV_CTRL_FRAMELOCK_SYNC_DISABLE;

    /* If we are enabling frame lock, enable the master first */
    if (enabled) {
        something_enabled = set_enable_sync_server(tree, val);
        framelock_enabled = (framelock_enabled || something_enabled);
        server_enabled = something_enabled;
    }

    /* Enable/Disable slaves */
    something_enabled = set_enable_sync_clients(tree->entries, val);
    framelock_enabled = (framelock_enabled || something_enabled);

    /* If we are disabling frame lock, disable the master last */
    if (!enabled) {
        something_enabled = set_enable_sync_server(tree, val);
        framelock_enabled = (framelock_enabled || something_enabled);
    }

    /* 
     * toggle the TEST_SIGNAL, to guarantee accuracy of the universal
     * frame count (as returned by the glXQueryFrameCountNV() function
     * in the GLX_NV_swap_group extension)
     */

    entry = get_gpu_server_entry(tree);
    if (enabled && entry && framelock_enabled) {
        nvGPUDataPtr data = (nvGPUDataPtr)(entry->data);
        NvCtrlSetAttribute(data->handle,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE);
        NvCtrlSetAttribute(data->handle,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                           NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE);
    }

    /* If frame lock was enabled but there was no server
     * specified, we should confirm with the user that
     * all is well since this may result in losing signal
     * on the client devices.
     */

    if (framelock_enabled && !server_enabled) {

        /* If confirmation fails, disable frame lock */
        if (!confirm_serverless_framelock(ctk_framelock)) {

            set_enable_sync_clients(tree->entries,
                                    NV_CTRL_FRAMELOCK_SYNC_DISABLE);

            set_enable_sync_server(tree, NV_CTRL_FRAMELOCK_SYNC_DISABLE);

            framelock_enabled = FALSE;
        }
    }

    ctk_framelock->framelock_enabled = framelock_enabled;

    update_framelock_controls(ctk_framelock);

    update_framelock_status(ctk_framelock);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Frame Lock %s.",
                                 (enabled ? "enabled" : "disabled"));
}



/** test_link_done() *************************************************
 *
 * Callback function for the frame lock test signal functionality.
 * This function is called once the test signal has finished.
 *
 */
static gint test_link_done(gpointer data) 
{
    CtkFramelock *ctk_framelock;
    nvListEntryPtr entry;

    ctk_framelock = CTK_FRAMELOCK(data);
    
    entry = get_gpu_server_entry((nvListTreePtr)ctk_framelock->tree);

    if (!entry) return FALSE;
    
    /* Test signal already disabled? */

    if (!ctk_framelock->test_link_enabled) return FALSE;
    
    /* Disable the test signal */
    
    ctk_framelock->test_link_enabled = FALSE;

    NvCtrlSetAttribute(((nvGPUDataPtr)(entry->data))->handle,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE);
    
    gtk_grab_remove(ctk_framelock->test_link_button);
        
    gdk_window_set_cursor((GTK_WIDGET(ctk_framelock->parent_window))->window,
                          NULL);

    /* un-press the testlink button */
   
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->test_link_button),
         G_CALLBACK(toggle_test_link),
         (gpointer) ctk_framelock);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button), FALSE);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->test_link_button),
         G_CALLBACK(toggle_test_link),
         (gpointer) ctk_framelock);

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Test link complete.");

    return FALSE;
}



/** toggle_test_link() ***********************************************
 *
 * Callback function for the 'test link' button.  This button
 * activates the frame lock test signal.
 *
 */
static void toggle_test_link(GtkWidget *button, gpointer data)
{
    CtkFramelock *ctk_framelock;
    gboolean      enabled = FALSE;
    nvListEntryPtr entry;

    ctk_framelock = CTK_FRAMELOCK(data);

    if (!ctk_framelock->framelock_enabled) goto fail;

    /* User cancels the test signal */
    
    if (ctk_framelock->test_link_enabled) {
        test_link_done(ctk_framelock);
        return;
    }

    enabled = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button));

    if (!enabled) goto fail;

    entry = get_gpu_server_entry((nvListTreePtr)ctk_framelock->tree);

    if (!entry) {
        enabled = FALSE;
        goto fail;
    }

    /* enable the test signal */
    
    ctk_framelock->test_link_enabled = TRUE;

    gdk_window_set_cursor
        ((GTK_WIDGET(ctk_framelock->parent_window))->window,
         ctk_framelock->wait_cursor);
        
    gtk_grab_add(button);
    
    NvCtrlSetAttribute(((nvGPUDataPtr)(entry->data))->handle,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                       NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE);
    
    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Test link started.");

    /* register the "done" function */

    g_timeout_add(DEFAULT_TEST_LINK_TIME_INTERVAL,
                  test_link_done, (gpointer) ctk_framelock);

    return;
    
 fail:

    /* Reset the button */
    g_signal_handlers_block_by_func(G_OBJECT(ctk_framelock->test_link_button),
                                    G_CALLBACK(toggle_test_link),
                                    (gpointer) ctk_framelock);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button), enabled);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->test_link_button),
         G_CALLBACK(toggle_test_link),
         (gpointer) ctk_framelock);
}    



/** activate_sync_interval() *****************************************
 *
 * Callback function for when the user changes the house sync
 * interval.
 *
 */
static void activate_sync_interval(GtkEntry *widget, gpointer user_data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *)user_data;
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = get_framelock_server_entry(tree);
    nvFrameLockDataPtr data;
    const gchar *str = gtk_entry_get_text(widget);
    gint interval;
    
    if (!entry || !str) return;

    interval = strtol(str, NULL, 10);

    data = (nvFrameLockDataPtr)(entry->data);

    NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_INTERVAL,
                       interval);
}



/** changed_sync_edge() **********************************************
 *
 * Callback function for when the user changes a frame lock device's
 * sync edge.
 *
 */
static void changed_sync_edge(GtkEditable *editable, gpointer user_data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *)user_data;
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = get_framelock_server_entry(tree);
    nvFrameLockDataPtr data;
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(editable));
    gint edge;

    if (!entry || !str) return;

    data = (nvFrameLockDataPtr) entry->data;

    for (edge = NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE;
         edge <= NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES; edge++) {
        if (strcmp(syncEdgeStrings[edge], str) == 0) {
            NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_POLARITY, edge);
            return;
        }
    }
}



/** changed_video_mode() *********************************************
 *
 * Callback function for when the user changes the house sync video
 * mode.
 *
 */
static void changed_video_mode(GtkEditable *editable, gpointer user_data)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *)user_data;
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = get_framelock_server_entry(tree);
    nvFrameLockDataPtr data;
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(editable));
    gint mode;

    if (!entry || !str) return;

    data = (nvFrameLockDataPtr) entry->data;

    for (mode = NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE;
         mode <= NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV; mode++) {
        
        if (strcmp(houseFormatStrings[mode], str) == 0) {
            NvCtrlSetAttribute(data->handle,
                               NV_CTRL_FRAMELOCK_VIDEO_MODE, mode);
            return;
        }
    }
}



/** detect_video_mode_time() *****************************************
 *
 * Callback function called every time the video mode detection
 * timer goes off.
 *
 * see toggle_detect_video_mode() for details.
 *
 */
static gboolean detect_video_mode_timer(gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = get_framelock_server_entry(tree);
    nvFrameLockDataPtr data;
    gint house;

    /* Master gone... oops */
    if (!entry) {
        goto done;
    }

    /* check if we now have house sync */

    data = (nvFrameLockDataPtr)(entry->data);

    NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_HOUSE_STATUS, &house);

    if (house) {

        /*
         * We found house sync; use the current_detect_format
         */

        update_house_sync_controls(ctk_framelock);

        ctk_config_statusbar_message
            (ctk_framelock->ctk_config,
             "House sync format detected as %s.",
             houseFormatStrings[ctk_framelock->current_detect_format]);
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

    NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,
                       ctk_framelock->current_detect_format);

    return TRUE;


 done:

    /* untoggle the detect button */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->video_mode_detect),
         G_CALLBACK(toggle_detect_video_mode),
         (gpointer) ctk_framelock);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->video_mode_detect), FALSE);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->video_mode_detect),
         G_CALLBACK(toggle_detect_video_mode),
         (gpointer) ctk_framelock);

    return FALSE;
}



/** toggle_detect_video_mode() ***************************************
 *
 * Callback function for when the user clicks on the 'Detect' (video
 * mode) button.
 *
 * House Sync autodetection scheme: a modal push button is used to
 * request auto detection.  When the button is pressed, we program the
 * first format type and then start a timer.
 *
 * From the timer, we check if we are getting a house sync; if we are,
 * then update the settings and unpress the button.  If we are not,
 * program the next format in the sequence and try again.
 *
 * XXX what happens if the master gets changed while we are doing
 * this?
 *
 */
static void toggle_detect_video_mode(GtkToggleButton *button,
                                     gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = get_framelock_server_entry(tree);
    nvFrameLockDataPtr data;


    if (!gtk_toggle_button_get_active(button)) {
        g_source_remove(ctk_framelock->video_mode_detect_timer);
        ctk_framelock->video_mode_detect_timer = 0;

        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     "Aborted house sync detection.");
        return;
    }

    if (!entry) return;

    data = (nvFrameLockDataPtr)(entry->data);
    
    ctk_framelock->current_detect_format =
        NV_CTRL_FRAMELOCK_VIDEO_MODE_COMPOSITE_AUTO;

    NvCtrlSetAttribute(data->handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,
                       ctk_framelock->current_detect_format);

    ctk_framelock->video_mode_detect_timer =
        g_timeout_add(500, detect_video_mode_timer, user_data);
        
    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Attempting to detect house sync...");
}



/** list_entry_update_framelock_status() *****************************
 *
 * Updates the state of the GUI for a frame lock list entry by
 * querying the current state of the X Server.
 *
 */
void list_entry_update_framelock_status(CtkFramelock *ctk_framelock,
                                        nvListEntryPtr entry)
{
    nvFrameLockDataPtr data = (nvFrameLockDataPtr)(entry->data);
    gint rate, delay, house, port0, port1;
    gchar str[32];
    gfloat fvalue;
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr server_entry = get_framelock_server_entry(tree);
    gboolean use_house_sync;
    gboolean framelock_enabled;
    gboolean is_server;
    ReturnStatus ret;
    
    
    NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_DELAY, &delay);
    NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_HOUSE_STATUS, &house);
    NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_PORT0_STATUS, &port0);
    NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_PORT1_STATUS, &port1);

    use_house_sync = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync));

    framelock_enabled = ctk_framelock->framelock_enabled;

    is_server = (server_entry && (server_entry->data == data));

    /* Receiving Sync */
    if (!framelock_enabled || (is_server && !use_house_sync)) {
        gtk_widget_set_sensitive(data->receiving_label, FALSE);
        update_image(data->receiving_hbox, ctk_framelock->led_grey);
    } else {
        gint receiving;
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_READY,
                           &receiving);
        gtk_widget_set_sensitive(data->receiving_label, TRUE);
        update_image(data->receiving_hbox,
                     receiving ? ctk_framelock->led_green :
                     ctk_framelock->led_red);
    }

    /* Sync Rate */
    gtk_widget_set_sensitive(data->rate_label, framelock_enabled);
    gtk_widget_set_sensitive(data->rate_text, framelock_enabled);

    ret =
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_RATE_4, &rate);
    if (ret == NvCtrlSuccess) {
        snprintf(str, 32, "%d.%.4d Hz", (rate / 10000), (rate % 10000));
    } else {
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_RATE, &rate);
        snprintf(str, 32, "%d.%.3d Hz", (rate / 1000), (rate % 1000));
    }
    gtk_label_set_text(GTK_LABEL(data->rate_text), str);
    
    /* Sync Delay (Skew) */
    gtk_widget_set_sensitive(data->delay_label, framelock_enabled);
    gtk_widget_set_sensitive(data->delay_text, framelock_enabled);
    fvalue = ((gfloat) delay) * NV_CTRL_FRAMELOCK_SYNC_DELAY_FACTOR;
    snprintf(str, 32, "%.2f uS", fvalue); // 10.2f
    gtk_label_set_text(GTK_LABEL(data->delay_text), str);

    /* House Sync and Ports are always active */
    update_image(data->house_hbox,
                 house?ctk_framelock->led_green:ctk_framelock->led_red);
    if ( !data->port0_ethernet_error ) {
        update_image(data->port0_hbox,
                     (port0==NV_CTRL_FRAMELOCK_PORT0_STATUS_INPUT)?
                     ctk_framelock->rj45_input:ctk_framelock->rj45_output);
    } else {
        update_image(data->port0_hbox, ctk_framelock->rj45_unused);
    }
    if ( !data->port1_ethernet_error ) {
        update_image(data->port1_hbox,
                     (port1==NV_CTRL_FRAMELOCK_PORT0_STATUS_INPUT)?
                     ctk_framelock->rj45_input:ctk_framelock->rj45_output);
    } else {
        update_image(data->port1_hbox, ctk_framelock->rj45_unused);
    }
}



/** list_entry_update_gpu_status() ***********************************
 *
 * Updates the state of the GUI for a gpu list entry by querying the
 * current state of the X Server.
 *
 */
void list_entry_update_gpu_status(CtkFramelock *ctk_framelock,
                                  nvListEntryPtr entry)
{
    nvGPUDataPtr data = (nvGPUDataPtr)(entry->data);
    gboolean framelock_enabled;
    gboolean has_server;
    gboolean has_client;
    gboolean use_house_sync;
    gint house = 0;

    framelock_enabled = ctk_framelock->framelock_enabled;

    use_house_sync = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync));

    if (entry->parent && entry->parent->data) {
        nvFrameLockDataPtr framelock_data =
            (nvFrameLockDataPtr)(entry->parent->data);

        NvCtrlGetAttribute(framelock_data->handle,
                           NV_CTRL_FRAMELOCK_HOUSE_STATUS,
                           &house);
    }

    has_client = data->clients_mask;
    has_server = data->server_mask;

    /* Check Timing Sync */
    if (!framelock_enabled ||
        (!has_server && !has_client) ||
        (has_server && (use_house_sync || !house))) {
        gtk_widget_set_sensitive(data->timing_label, FALSE);
        update_image(data->timing_hbox, ctk_framelock->led_grey);
    } else {
        gint timing;
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_TIMING, &timing);
        gtk_widget_set_sensitive(data->timing_label, TRUE);
        update_image(data->timing_hbox,
                     timing ? ctk_framelock->led_green :
                     ctk_framelock->led_red);
    }
}



/** list_entry_update_display_status() *******************************
 *
 * Updates the state of the GUI for a display list entry by querying
 * the current state of the X Server.
 *
 */
void list_entry_update_display_status(CtkFramelock *ctk_framelock,
                                      nvListEntryPtr entry)
{
    nvDisplayDataPtr data = (nvDisplayDataPtr)(entry->data);
    gboolean framelock_enabled;
    gboolean is_server;
    gboolean is_client;
    gboolean gpu_is_server;
    gboolean use_house_sync;
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr gpu_server_entry = get_gpu_server_entry(tree);

    framelock_enabled = ctk_framelock->framelock_enabled;

    use_house_sync = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync));

    is_server = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(data->server_checkbox));

    is_client = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(data->client_checkbox));

    gpu_is_server = (gpu_server_entry && (gpu_server_entry == entry->parent));

    /* Check Stereo Sync.  If frame lock is disabled or this display device
     * is neither a client/server or the display device is a server and the
     * GPU driving it is not using the house sync signal, gray out the LED.
     */
    if (!framelock_enabled ||
        (!is_server && !is_client) ||
        (is_server && gpu_is_server && !use_house_sync)) {
        gtk_widget_set_sensitive(data->stereo_label, FALSE);
        update_image(data->stereo_hbox, ctk_framelock->led_grey);
    } else {
        gint stereo_sync;
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_STEREO_SYNC,
                           &stereo_sync);
        gtk_widget_set_sensitive(data->stereo_label, TRUE);
        update_image(data->stereo_hbox,
                     stereo_sync ? ctk_framelock->led_green :
                     ctk_framelock->led_red);
    }
}



/** list_entry_update_status() ***************************************
 *
 * Updates the (GUI) state of a list entry, its children and siblings
 * by queryin ghte X Server.
 *
 */
void list_entry_update_status(CtkFramelock *ctk_framelock,
                              nvListEntryPtr entry)
{
    if (!entry) return;

    list_entry_update_status(ctk_framelock, entry->children);

    if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
        list_entry_update_framelock_status(ctk_framelock, entry);
    } else if (entry->data_type == ENTRY_DATA_GPU) {
        list_entry_update_gpu_status(ctk_framelock, entry);
    } else if (entry->data_type == ENTRY_DATA_DISPLAY) {
        list_entry_update_display_status(ctk_framelock, entry);
    }

    list_entry_update_status(ctk_framelock, entry->next_sibling);
}



/** update_framelock_status() ****************************************
 *
 * Updates the (GUI) state of all the frame lock list entries status
 * fields by querying the X Server.
 *
 */
static gboolean update_framelock_status(gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);

    list_entry_update_status(ctk_framelock,
                             ((nvListTreePtr)(ctk_framelock->tree))->entries);

    return TRUE;
}



/** check_for_ethernet() *********************************************
 *
 * Queries ethernet status for all frame lock devices and reports
 * on any error.
 *
 * XXX This assumes that the frame lock (G-Sync) devices are
 *     top-level list entries, such that they are all siblings.
 *
 */
static gboolean check_for_ethernet(gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    static gboolean first_error = TRUE;
    nvListEntryPtr entry;
    nvFrameLockDataPtr error_data = NULL;
    

    /* Look through the framelock entries and check the
     * ethernet status on each one
     */
    entry = ((nvListTreePtr)(ctk_framelock->tree))->entries;
    while (entry) {
        if (entry->data_type == ENTRY_DATA_FRAMELOCK) {
            nvFrameLockDataPtr data = (nvFrameLockDataPtr)(entry->data);
            gint val;

            NvCtrlGetAttribute(data->handle,
                               NV_CTRL_FRAMELOCK_ETHERNET_DETECTED,
                               &val);

            if (val & NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_PORT0) {
                data->port0_ethernet_error = TRUE;
                error_data = data;
            } else {
                data->port0_ethernet_error = 0;
            }
            if (val & NV_CTRL_FRAMELOCK_ETHERNET_DETECTED_PORT1) {
                data->port1_ethernet_error = TRUE;
                error_data = data;
            } else {
                data->port1_ethernet_error = 0;
            }
        }
        entry = entry->next_sibling;
    }


    if (error_data) {
        if (first_error) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Frame Lock RJ45 error</span>\n\n"
                      "Either an Ethernet LAN cable is connected to the "
                      "frame lock board on X Server '%s' or the linked "
                      "PC is not turned on.  Either disconnect the LAN "
                      "cable or turn on the linked PC for proper "
                      "operation.",
                      NvCtrlGetDisplayName(error_data->handle)
                      );
        }
        first_error = FALSE;
    } else {
        first_error = TRUE;
    }

    return TRUE;
}



/** update_house_sync_controls() *************************************
 *
 * Queries the X Server for hosue sync status information for the
 * currently selected frame lock server and updates the GUI.
 *
 */
static void update_house_sync_controls(CtkFramelock *ctk_framelock)
{
    nvListEntryPtr entry;
    gboolean enabled;
    gboolean use_house;
    ReturnStatus ret;
    nvFrameLockDataPtr data;


    entry = get_framelock_server_entry((nvListTreePtr)(ctk_framelock->tree));

    /* No server selected, can't set house sync settings */
    if (!entry) {
         gtk_widget_set_sensitive(ctk_framelock->use_house_sync, FALSE);
         gtk_widget_set_sensitive(ctk_framelock->house_sync_frame, FALSE);
         return;
    }


    /* Get the current use house sync state from the X Server */
    data = (nvFrameLockDataPtr)(entry->data); 

    ret = NvCtrlGetAttribute(data->handle, NV_CTRL_USE_HOUSE_SYNC, &use_house);
    if (ret != NvCtrlSuccess) {
        use_house = TRUE; /* Can't toggle, attribute always on. */
    }

    gtk_widget_set_sensitive(ctk_framelock->use_house_sync,
                             (ret == NvCtrlSuccess));

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_framelock->use_house_sync),
         G_CALLBACK(toggle_use_house_sync),
         (gpointer) ctk_framelock);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync), use_house);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_framelock->use_house_sync),
         G_CALLBACK(toggle_use_house_sync),
         (gpointer) ctk_framelock);


    enabled = ctk_framelock->framelock_enabled;
    gtk_widget_set_sensitive(ctk_framelock->house_sync_frame, !enabled);

    if (enabled || !use_house) {
        gtk_widget_set_sensitive(ctk_framelock->house_sync_hbox, FALSE);
    } else {
        gint sync_interval;
        gint sync_edge;
        gint house_format;
        gchar str[32];

        nvFrameLockDataPtr data;
        
        gtk_widget_set_sensitive(ctk_framelock->house_sync_hbox, TRUE);

        data = (nvFrameLockDataPtr)(entry->data);

        /* Query current house sync settings from master frame lock device */
        
        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_SYNC_INTERVAL,
                           &sync_interval);

        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_POLARITY,
                           &sync_edge);

        NvCtrlGetAttribute(data->handle, NV_CTRL_FRAMELOCK_VIDEO_MODE,
                           &house_format);

        /* Update GUI to reflect server settings */

        snprintf(str, 32, "%d", sync_interval);
        gtk_entry_set_text(GTK_ENTRY(ctk_framelock->sync_interval_entry),
                           str); 

        if (sync_edge < NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE)
            sync_edge = NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE;
        if (sync_edge > NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES)
            sync_edge = NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES;

        gtk_entry_set_text
            (GTK_ENTRY(GTK_COMBO(ctk_framelock->sync_edge_combo)->entry),
             syncEdgeStrings[sync_edge]);

        if (house_format < NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE;
        if (house_format > NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV;
        
        gtk_entry_set_text
            (GTK_ENTRY(GTK_COMBO(ctk_framelock->video_mode_combo)->entry),
             houseFormatStrings[house_format]);

    }
}



/** gpu_state_received() *********************************************
 *
 * Signal handler for gpu target events.
 *
 */
static void gpu_state_received(GtkObject *object,
                               gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event = (CtkEventStruct *) arg1;
    nvListEntryPtr gpu_entry = (nvListEntryPtr) user_data;
    nvGPUDataPtr gpu_data = (nvGPUDataPtr) gpu_entry->data;

    nvListEntryPtr display_entry = NULL;
    nvDisplayDataPtr display_data = NULL;

    CtkFramelock *ctk_framelock = gpu_entry->tree->ctk_framelock;
    gboolean sensitive;
    gboolean checked;

    switch (event->attribute) {
    case NV_CTRL_FRAMELOCK_MASTER:

        /* Unset the previous master */
        display_entry = get_display_server_entry(gpu_entry->tree);
        if (display_entry) {
            display_data = (nvDisplayDataPtr)(display_entry->data);
            
            /* Clear the server checkbox */
            g_signal_handlers_block_by_func
                (G_OBJECT(display_data->server_checkbox),
                 G_CALLBACK(toggle_server),
                 (gpointer) display_entry);
            
            gtk_toggle_button_set_active
                (GTK_TOGGLE_BUTTON(display_data->server_checkbox), 0);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(display_data->server_checkbox),
                 G_CALLBACK(toggle_server),
                 (gpointer) display_entry);

            /* If the server display device is on another gpu, tell the
             * X Server we are unsetting it.
             */
            if (display_entry->parent != gpu_entry) {
                nvGPUDataPtr gpu_data =
                    (nvGPUDataPtr)(display_entry->parent->data);

                NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_SYNC,
                                   NV_CTRL_FRAMELOCK_SYNC_DISABLE);
                gpu_data->enabled = FALSE;
                
                NvCtrlSetAttribute(display_data->handle,
                                   NV_CTRL_FRAMELOCK_MASTER, 0);
            }
            ((nvGPUDataPtr)(display_entry->parent->data))->server_mask = 0;
            gpu_entry->tree->server_entry = NULL;
        }

        /* Set the new master */
        display_entry = get_display_on_gpu(gpu_entry, event->value);
        if (display_entry) {
            display_data = (nvDisplayDataPtr)(display_entry->data);

            /* Set the server checkbox */
            g_signal_handlers_block_by_func
                (G_OBJECT(display_data->server_checkbox),
                 G_CALLBACK(toggle_server),
                 (gpointer) display_entry);
            
            gtk_toggle_button_set_active
                (GTK_TOGGLE_BUTTON(display_data->server_checkbox), 1);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(display_data->server_checkbox),
                 G_CALLBACK(toggle_server),
                 (gpointer) display_entry);
            
            gpu_entry->tree->server_entry = display_entry;
        }
        
        gpu_data->server_mask = event->value;

        /* See if anything was disabled */
        ctk_framelock->framelock_enabled =
            any_gpu_enabled(gpu_entry->tree->entries);

        update_framelock_controls(gpu_entry->tree->ctk_framelock);
        break;

    case NV_CTRL_FRAMELOCK_SLAVES:
        
        /* Set all client devices on this GPU.  If a client is found
         * to not match the selected server's refresh rate, unselect
         * the server.  The user will have to reselect the server.
         */
        for (display_entry = gpu_entry->children; display_entry;
             display_entry = display_entry->next_sibling) {
            display_data =
                (nvDisplayDataPtr)(display_entry->data);
            
            sensitive = 
                GTK_WIDGET_IS_SENSITIVE(display_data->client_checkbox);
            checked = ((display_data->device_mask) & event->value);
            
            /* Update the display list entry gui */
            
            g_signal_handlers_block_by_func
                (G_OBJECT(display_data->client_checkbox),
                 G_CALLBACK(toggle_client),
                 (gpointer) display_entry);
            
            gtk_toggle_button_set_active
                (GTK_TOGGLE_BUTTON(display_data->client_checkbox), checked);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(display_data->client_checkbox),
                 G_CALLBACK(toggle_client),
                 (gpointer) display_entry);
            
            /* If there is an inconsistensy, unselect the server */

            if (checked && !sensitive && gpu_entry->tree->server_entry) {
                
                nvListEntryPtr server_entry =
                    get_display_server_entry(gpu_entry->tree);
                nvDisplayDataPtr server_data = 
                     (nvDisplayDataPtr)(server_entry->data);
                
                /* Clear the server checkbox */
                g_signal_handlers_block_by_func
                    (G_OBJECT(server_data->server_checkbox),
                     G_CALLBACK(toggle_server),
                     (gpointer) display_entry);
                
                gtk_toggle_button_set_active
                    (GTK_TOGGLE_BUTTON(server_data->server_checkbox), 0);
                
                g_signal_handlers_unblock_by_func
                    (G_OBJECT(server_data->server_checkbox),
                     G_CALLBACK(toggle_server),
                     (gpointer) display_entry);

                NvCtrlSetAttribute(server_data->handle,
                                   NV_CTRL_FRAMELOCK_MASTER, 0);
                ((nvGPUDataPtr)(server_entry->parent->data))->server_mask = 0;
                gpu_entry->tree->server_entry = NULL;
            }
        }

        /* Save the client state */
        gpu_data->clients_mask = event->value;

        /* See if anything was disabled */
        ctk_framelock->framelock_enabled =
            any_gpu_enabled(gpu_entry->tree->entries);

        update_framelock_controls(gpu_entry->tree->ctk_framelock);
        break;
        

    case NV_CTRL_FRAMELOCK_SYNC:
        /* Cache the enable/disable state of the gpu sync */
        gpu_data->enabled = event->value;
        
        /* Look to see if any gpu is enabled/disabled */
        ctk_framelock->framelock_enabled =
            any_gpu_enabled(gpu_entry->tree->entries);

        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_framelock->sync_state_button),
             G_CALLBACK(toggle_sync_enable),
             (gpointer) ctk_framelock);

        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_framelock->sync_state_button),
             ctk_framelock->framelock_enabled ? 1 : 0);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_framelock->sync_state_button),
             G_CALLBACK(toggle_sync_enable),
             (gpointer) ctk_framelock);

        update_framelock_controls(gpu_entry->tree->ctk_framelock);
        break;


    case NV_CTRL_FRAMELOCK_TEST_SIGNAL:
        switch (event->value) {
        case NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE:
            ctk_framelock->test_link_enabled = TRUE;
            gdk_window_set_cursor
                ((GTK_WIDGET(ctk_framelock->parent_window))->window,
                 ctk_framelock->wait_cursor);
            gtk_grab_add(ctk_framelock->test_link_button);
            break;
        case NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE:
            ctk_framelock->test_link_enabled = FALSE;
            gtk_grab_remove(ctk_framelock->test_link_button);
            gdk_window_set_cursor
                ((GTK_WIDGET(ctk_framelock->parent_window))->window,
                 NULL);
            break;
        default:
            /* Unknwon state, ignore */
            break;
        }

        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_framelock->test_link_button),
             G_CALLBACK(toggle_test_link),
             (gpointer) ctk_framelock);
        
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_framelock->test_link_button),
             ctk_framelock->test_link_enabled);
    
        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_framelock->test_link_button),
             G_CALLBACK(toggle_test_link),
             (gpointer) ctk_framelock);

        ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                     (ctk_framelock->test_link_enabled ?
                                      "Test link started." :
                                      "Test link complete."));
        break;


    case NV_CTRL_REFRESH_RATE:
        /* Update the display device's refresh rate */
        display_entry = get_display_on_gpu(gpu_entry, event->display_mask);
        if (display_entry && display_entry->data) {
            float fvalue;
            char str[32];

            display_data =
                (nvDisplayDataPtr)(display_entry->data);

            display_data->rate = event->value;
            fvalue = ((float)(display_data->rate)) / 100.0f;
            snprintf(str, 32, "%.2f Hz", fvalue);
            gtk_label_set_text(GTK_LABEL(display_data->rate_text), str);   
        }
        
        /* Make sure the framelock controls are in a consistent state */
        update_framelock_controls(ctk_framelock);
        break;


    default:
        /* Oops */
        break;
    }
    
} /* gpu_state_received() */



/** framelock_state_received() ***************************************
 *
 * Signal handler for frame lock target events.
 *
 */
static void framelock_state_received(GtkObject *object,
                                     gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event = (CtkEventStruct *) arg1;
    nvListEntryPtr entry = (nvListEntryPtr) user_data;
    CtkFramelock *ctk_framelock = entry->tree->ctk_framelock;

    nvListEntryPtr server_entry =
        get_framelock_server_entry(entry->tree);

    char str[32];
    gint sync_edge;
    gint house_format;

    if (server_entry && entry != server_entry) {
        /* Setting is being made to a non-server frame lock device, ignore */
        return;
    }

    /* Process the new frame lock device setting */

    switch (event->attribute) {
    case NV_CTRL_USE_HOUSE_SYNC:
        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_framelock->use_house_sync),
             G_CALLBACK(toggle_use_house_sync),
             (gpointer) ctk_framelock);
        
         gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync),
             event->value);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_framelock->use_house_sync),
             G_CALLBACK(toggle_use_house_sync),
             (gpointer) ctk_framelock);
        break;

    case NV_CTRL_FRAMELOCK_SYNC_INTERVAL:
        g_signal_handlers_block_by_func
            (G_OBJECT(ctk_framelock->sync_interval_entry),
             G_CALLBACK(activate_sync_interval),
             (gpointer) ctk_framelock);

        snprintf(str, 32, "%d", event->value);
        gtk_entry_set_text(GTK_ENTRY(ctk_framelock->sync_interval_entry),
                           str);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(ctk_framelock->sync_interval_entry),
             G_CALLBACK(activate_sync_interval),
             (gpointer) ctk_framelock);
        break;

    case NV_CTRL_FRAMELOCK_POLARITY:
        sync_edge = event->value;
        if (sync_edge < NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE)
            sync_edge = NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE;
        if (sync_edge > NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES)
            sync_edge = NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES;

       g_signal_handlers_block_by_func
            (G_OBJECT(GTK_COMBO(ctk_framelock->sync_edge_combo)->entry),
             G_CALLBACK(changed_sync_edge),
             (gpointer) ctk_framelock);

        gtk_entry_set_text
            (GTK_ENTRY(GTK_COMBO(ctk_framelock->sync_edge_combo)->entry),
             syncEdgeStrings[sync_edge]);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(GTK_COMBO(ctk_framelock->sync_edge_combo)->entry),
             G_CALLBACK(changed_sync_edge),
             (gpointer) ctk_framelock);

        break;

    case NV_CTRL_FRAMELOCK_VIDEO_MODE:
        house_format = event->value;
        if (house_format < NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_NONE;
        if (house_format > NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV)
            house_format = NV_CTRL_FRAMELOCK_VIDEO_MODE_HDTV;

        g_signal_handlers_block_by_func
            (G_OBJECT(GTK_COMBO(ctk_framelock->video_mode_combo)->entry),
             G_CALLBACK(changed_video_mode),
             (gpointer) ctk_framelock);

        gtk_entry_set_text
            (GTK_ENTRY(GTK_COMBO(ctk_framelock->video_mode_combo)->entry),
             houseFormatStrings[house_format]);
        
        g_signal_handlers_unblock_by_func
            (G_OBJECT(GTK_COMBO(ctk_framelock->video_mode_combo)->entry),
             G_CALLBACK(changed_video_mode),
             (gpointer) ctk_framelock);
        break;

    default:
        /* Oops */
        break;
    }


    update_house_sync_controls(ctk_framelock);
}




/**************************************************************************/

/*
 * Main Frame Lock Page Widget
 */


static GObjectClass *parent_class;



/** ctk_framelock_class_init() ***************************************
 *
 * Initialize the object structure
 *
 */
static void ctk_framelock_class_init(
    CtkFramelockClass *ctk_framelock_class
)
{
    GObjectClass *gobject_class;

    gobject_class = (GObjectClass *) ctk_framelock_class;
    parent_class = g_type_class_peek_parent(ctk_framelock_class);

}



/** ctk_framelock_get_type() *****************************************
 *
 * registers the frame lock class and return the unique type id.
 *
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

}



/** ctk_framelock_new() **********************************************
 *
 * returns a new instance of the frame lock class.
 *
 */
GtkWidget* ctk_framelock_new(NvCtrlAttributeHandle *handle,
                             GtkWidget *parent_window, CtkConfig *ctk_config,
                             ParsedAttribute *p)
{
    GObject *object;
    CtkFramelock *ctk_framelock;
    GtkWidget *banner;

    ReturnStatus ret;
    unsigned int num_framelocks;

    GtkWidget *frame;
    GtkWidget *padding;
    GtkWidget *sw;     /* Scrollable window */
    GtkWidget *vp;     /* Viewport */

    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *combo;
    GList *glist;
    GtkWidget *button;
    GtkWidget *image;

    

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /* Only expose frame lock if there are frame lock boards in
     * the system.  This isn't absolutely necessary, because the
     * frame lock control page does not have to include the current
     * NV-CONTROL handle in the frame lock group.  However, we don't
     * want to expose the frame lock page unconditionally (it would
     * only confuse most users), so this is as good a condition as
     * anything else.
     *
     * XXX We could also add yet another checkbox in the nvidia-settings
     * Options page.
     */

    ret = NvCtrlQueryTargetCount(handle,
                                 NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                 (int *)&num_framelocks);
    if (ret != NvCtrlSuccess) return NULL;
    if (!num_framelocks) return NULL;

    /* 1. - Create the frame lock widgets */

    /* create the frame lock page object */
    
    object = g_object_new(CTK_TYPE_FRAMELOCK, NULL);
    
    ctk_framelock = CTK_FRAMELOCK(object);
    ctk_framelock->attribute_handle = handle;
    ctk_framelock->ctk_config = ctk_config;
    ctk_framelock->parent_window = GTK_WINDOW(parent_window);

    /* create the watch cursor */

    ctk_framelock->wait_cursor = gdk_cursor_new(GDK_WATCH);

    /* create dialog windows */

    ctk_framelock->add_devices_dialog =
        create_add_devices_dialog(ctk_framelock);

    ctk_framelock->remove_devices_dialog =
        create_remove_devices_dialog(ctk_framelock);

    ctk_framelock->error_msg_dialog =
        create_error_msg_dialog(ctk_framelock);

    ctk_framelock->enable_confirm_dialog =
        create_enable_confirm_dialog(ctk_framelock);

    /* create buttons */

    button = my_button_new_with_label("Add Devices...", 15, 0);
    g_signal_connect_swapped(G_OBJECT(button),
                             "clicked", G_CALLBACK(gtk_widget_show_all),
                             (gpointer) ctk_framelock->add_devices_dialog);
    ctk_config_set_tooltip(ctk_config, button, __add_devices_button_help);
    ctk_framelock->add_devices_button = button;

    button = my_button_new_with_label("Remove Devices...", 15, 0);
    g_signal_connect(G_OBJECT(button),
                     "clicked", G_CALLBACK(show_remove_devices_dialog),
                     GTK_OBJECT(ctk_framelock));
    ctk_config_set_tooltip(ctk_config, button,
                           __remove_devices_button_help);
    ctk_framelock->remove_devices_button = button;

    button = my_toggle_button_new_with_label("Short Names", 15, 0);
    //    g_signal_connect(G_OBJECT(button),
    //                     "toggled", G_CALLBACK(toggle_short_names),
    //                     GTK_OBJECT(ctk_framelock));
    ctk_framelock->short_labels_button = button;

    button = my_toggle_button_new_with_label("Show Extra Info", 15, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button),
                     "toggled", G_CALLBACK(toggle_extra_info),
                     (gpointer) ctk_framelock);
    ctk_config_set_tooltip(ctk_config, button,
                           __show_extra_info_button_help);
    ctk_framelock->extra_info_button = button;


    button = gtk_check_button_new_with_label("Use House Sync if Present");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    g_signal_connect(G_OBJECT(button),
                     "toggled", G_CALLBACK(toggle_use_house_sync),
                     (gpointer) ctk_framelock);
    ctk_config_set_tooltip(ctk_config, button,
                           __use_house_sync_button_help);
    ctk_framelock->use_house_sync = button;


    button = my_toggle_button_new_with_label("Detect", 15, 0);
    g_signal_connect(G_OBJECT(button),
                     "toggled", G_CALLBACK(toggle_detect_video_mode),
                     GTK_OBJECT(ctk_framelock));
    ctk_config_set_tooltip(ctk_config, button,
                           __detect_video_mode_button_help);
    ctk_framelock->video_mode_detect = button;


    button = my_toggle_button_new_with_label("Test Link", 15, 0);
    gtk_widget_set_sensitive(button, FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(toggle_test_link),
                     GTK_OBJECT(ctk_framelock));
    ctk_config_set_tooltip(ctk_config, button,
                           __test_link_button_help);
    ctk_framelock->test_link_button = button;
    

    button = create_sync_state_button(ctk_framelock);
    gtk_widget_set_sensitive(button, FALSE);
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(toggle_sync_enable),
                     GTK_OBJECT(ctk_framelock));
    ctk_config_set_tooltip(ctk_config, button, __sync_enable_button_help);
    ctk_framelock->sync_state_button = button;

    /* Create combo boxes */

    combo = gtk_combo_new();
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
    
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), glist);
    gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
    g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(combo)->entry)),
                     "changed", G_CALLBACK(changed_video_mode),
                     (gpointer) ctk_framelock);
    ctk_config_set_tooltip(ctk_config, combo,
                           __video_mode_combo_help);
    ctk_framelock->video_mode_combo = combo;


    combo = gtk_combo_new();
    glist = NULL;
    glist = g_list_append
        (glist,
         syncEdgeStrings[NV_CTRL_FRAMELOCK_POLARITY_RISING_EDGE]);
    glist = g_list_append
        (glist,
         syncEdgeStrings[NV_CTRL_FRAMELOCK_POLARITY_FALLING_EDGE]);
    glist = g_list_append
        (glist,
         syncEdgeStrings[NV_CTRL_FRAMELOCK_POLARITY_BOTH_EDGES]);

    gtk_combo_set_popdown_strings(GTK_COMBO(combo), glist);
    gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);
    g_signal_connect(G_OBJECT(GTK_EDITABLE(GTK_COMBO(combo)->entry)),
                     "changed", G_CALLBACK(changed_sync_edge),
                     (gpointer) ctk_framelock);
    ctk_config_set_tooltip(ctk_config, combo,
                           __sync_edge_combo_help);
    ctk_framelock->sync_edge_combo = combo;


    /* Cache images */

    ctk_framelock->led_grey  = ctk_image_new(&led_grey);
    ctk_framelock->led_green = ctk_image_new(&led_green);
    ctk_framelock->led_red   = ctk_image_new(&led_red);

    ctk_framelock->rj45_input  = ctk_image_new(&rj45_input);
    ctk_framelock->rj45_output = ctk_image_new(&rj45_output);
    ctk_framelock->rj45_unused = ctk_image_new(&rj45_unused);

    g_object_ref(ctk_framelock->led_grey);
    g_object_ref(ctk_framelock->led_green);
    g_object_ref(ctk_framelock->led_red);

    g_object_ref(ctk_framelock->rj45_input);
    g_object_ref(ctk_framelock->rj45_output);
    g_object_ref(ctk_framelock->rj45_unused);

    /* create the custom tree */

    ctk_framelock->tree = (gpointer)(list_tree_new(ctk_framelock));


    /* 2. - Pack frame lock widgets */

    gtk_box_set_spacing(GTK_BOX(ctk_framelock), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_FRAMELOCK);
    gtk_box_pack_start(GTK_BOX(ctk_framelock), banner, FALSE, FALSE, 0);

    /* G-Sync Frame */
    
    frame = gtk_frame_new(NULL);
    gtk_frame_set_label(GTK_FRAME(frame), "G-Sync Devices");
    gtk_box_pack_start(GTK_BOX(ctk_framelock), frame, TRUE, TRUE, 0);

    /* scrollable window */
    
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    padding = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(padding), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(padding), sw);
    gtk_container_add(GTK_CONTAINER(frame), padding);
    
    /* create a viewport so we can have a white background */
    
    vp = gtk_viewport_new(NULL, NULL);
    SELECT_WIDGET(vp, GTK_STATE_NORMAL);
    gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(vp));
    /** XXX **/gtk_widget_set_size_request(sw, -1, 200);/** XXX **/

    /* add the custom tree & buttons */

    vbox = ((nvListTreePtr)(ctk_framelock->tree))->vbox;
    
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(vp), vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->extra_info_button,
                     FALSE, FALSE, 0);
    // XXX Add me later....
    //
    //    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->short_labels_button,
    //                     FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->remove_devices_button,
                     FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->add_devices_button,
                     FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(padding), hbox, FALSE, FALSE, 0);

    /* add the house sync frame */

    frame = gtk_frame_new(NULL);
    ctk_framelock->house_sync_frame = frame;
    gtk_frame_set_label(GTK_FRAME(frame), "House Sync");
    gtk_box_pack_start(GTK_BOX(ctk_framelock), frame, FALSE, FALSE, 0);
    
    padding = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(padding), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), padding);

    /* add house sync BNC connector image */
    image = ctk_image_new(&bnc_cable);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), image, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(padding), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->use_house_sync,
                       FALSE, FALSE, 0);
    
    padding = gtk_hbox_new(FALSE, 5);
    ctk_framelock->house_sync_hbox = padding;
    gtk_box_pack_start(GTK_BOX(vbox), padding, FALSE, FALSE, 0);

    /* add the house sync interval */
    {
        GtkWidget *frame2 = gtk_frame_new(NULL);
        hbox = gtk_hbox_new(FALSE, 5);
        label = gtk_label_new("Sync Interval:");
        entry = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry), "0");
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 4);
        g_signal_connect(G_OBJECT(entry),
                         "activate", G_CALLBACK(activate_sync_interval),
                         (gpointer) ctk_framelock);
        ctk_config_set_tooltip(ctk_config, entry, __sync_interval_entry_help);
        ctk_framelock->sync_interval_frame = frame2;
        ctk_framelock->sync_interval_entry = entry;

        gtk_box_pack_start(GTK_BOX(padding), frame2, FALSE, FALSE, 0);

        gtk_entry_set_text(GTK_ENTRY(entry), "0");
        gtk_entry_set_width_chars(GTK_ENTRY(entry), 4);

        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 5);

        gtk_container_add(GTK_CONTAINER(frame2), hbox);
    }

    /* add the house sync video mode & detect */
    {
        GtkWidget *frame2 = gtk_frame_new(NULL);
        hbox = gtk_hbox_new(FALSE, 5);
        label = gtk_label_new("Sync Edge:");

        ctk_framelock->sync_edge_frame = frame2;
        
        gtk_box_pack_start(GTK_BOX(padding), frame2, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(frame2), hbox);

        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->sync_edge_combo,
                           FALSE, FALSE, 5);
    }

    /* add the house sync video mode & detect */
    {
        GtkWidget *frame2 = gtk_frame_new(NULL);
        hbox = gtk_hbox_new(FALSE, 5);
        label = gtk_label_new("Video Mode:");

        ctk_framelock->video_mode_frame = frame2;
        
        gtk_box_pack_start(GTK_BOX(padding), frame2, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(frame2), hbox);

        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->video_mode_combo,
                           FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_framelock->video_mode_detect,
                           FALSE, TRUE, 5);
    }

    /* add main buttons */

    hbox = gtk_hbox_new(FALSE, 5);

    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->sync_state_button,
                     FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_framelock->test_link_button,
                     FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(ctk_framelock), hbox, FALSE, FALSE, 0);

    /* show everything */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* apply the parsed attribute list */

    apply_parsed_attribute_list(ctk_framelock, p);

    /* update state of frame lock controls */

    update_framelock_controls(ctk_framelock);

    /* register a timer callback to update the status of the page */

    ctk_config_add_timer(ctk_config, DEFAULT_UPDATE_STATUS_TIME_INTERVAL,
                         "Frame Lock Connection Status",
                         (GSourceFunc) update_framelock_status,
                         (gpointer) ctk_framelock);
    
    /* register a timer callback to check the rj45 ports */

    ctk_config_add_timer(ctk_config, DEFAULT_CHECK_FOR_ETHERNET_TIME_INTERVAL,
                         "Frame Lock RJ45 Check",
                         (GSourceFunc) check_for_ethernet,
                         (gpointer) ctk_framelock);

    return GTK_WIDGET(object);
    
} /* ctk_framelock_new() */



/************************************************************************/

/*
 * functions relating to add_devices_dialog
 */


/** add_devices_respond_ok() *****************************************
 *
 * Callback function used to allow user to press the <RETURN> key
 * when entering the name of the X Server to add to the frame lock
 * group in the add_devices_dialog.
 *
 */
static void add_devices_repond_ok(GtkWidget *entry, gpointer data)
{
    add_devices_response(entry, GTK_RESPONSE_OK, data);
}



/** add_devices_response() *******************************************
 *
 * Callback function for the "response" event of the "Add X Server"
 * dialog box.
 *
 */
static void add_devices_response(GtkWidget *button, gint response,
                                 gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    const gchar *display_name;
    gint devices_added;
    
    /* hide the dialog box */
 
    gtk_widget_hide_all(ctk_framelock->add_devices_dialog);
    
    /* set the focus back to the text entry */
    
    gtk_widget_grab_focus(ctk_framelock->add_devices_entry);
    
    /* if the response is not "OK" then we're done */
    
    if (response != GTK_RESPONSE_OK) return;
    
    /* get the display name specified by the user */

    display_name =
        gtk_entry_get_text(GTK_ENTRY(ctk_framelock->add_devices_entry));

    /* Add all devices found on the server */
    
    devices_added = add_devices(ctk_framelock, display_name, TRUE);
    if (!devices_added) {
        /* Nothing was added, nothing to update */
        return;
    }

   /* Update frame lock controls */
    
    update_framelock_controls(ctk_framelock);

    /* Update frame lock status */
    
    update_framelock_status(ctk_framelock);

    /* Update status bar */

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Added X server '%s'.", display_name);
}



/** remove_devices_response() ****************************************
 *
 * Callback function for the "response" event of the "Remove Devices"
 * dialog box.
 *
 */
static void remove_devices_response(GtkWidget *button, gint response,
                                    gpointer user_data)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(user_data);
    nvListTreePtr tree = (nvListTreePtr)(ctk_framelock->tree);
    nvListEntryPtr entry = tree->selected_entry;
    gchar *label;
    
    gtk_widget_hide_all(ctk_framelock->remove_devices_dialog);

    if (response != GTK_RESPONSE_OK) return;

    if (!entry) return;

    label = get_entry_label(entry, 0);

    /* Remove entry from list */
    list_tree_remove_entry(tree, entry);

    /* If there are no entries left, Update the frame lock GUI */
    if (!tree->nentries) {

        /* Nothing to house sync to */
        if (ctk_framelock->use_house_sync) {
            gtk_toggle_button_set_active
                (GTK_TOGGLE_BUTTON(ctk_framelock->use_house_sync), FALSE);
        }

        /* Force frame lock state to OFF if it was on */
        ctk_framelock->framelock_enabled = FALSE;
    }

    update_framelock_controls(ctk_framelock);


    /* Update status bar */

    ctk_config_statusbar_message(ctk_framelock->ctk_config,
                                 "Removed '%s' from the frame lock group.",
                                 label);
    g_free(label);
}



/** add_display_devices() ********************************************
 *
 * Adds (as children list entries) all enabled display devices that
 * are bound to the given GPU List Entry.
 *
 */
static unsigned int add_display_devices(CtkFramelock *ctk_framelock,
                                        nvListEntryPtr gpu_entry)
{
    unsigned int      displays_added = 0;
    nvDisplayDataPtr  display_data = NULL;

    nvGPUDataPtr      gpu_data;
    nvListEntryPtr    entry;
    ReturnStatus      ret;
    
    unsigned int enabled_displays;
    unsigned int display_mask;

    unsigned int master_mask;
    unsigned int slaves_mask;
    gfloat       fvalue; /* To print the refresh rate */
    gchar        str[32];
    
    nvListEntryPtr   server_entry = NULL;
    nvDisplayDataPtr server_data = NULL;


    if (!gpu_entry || gpu_entry->data_type != ENTRY_DATA_GPU) {
        goto fail;
    }

    server_entry =
        get_display_server_entry((nvListTreePtr)(ctk_framelock->tree));
    if (server_entry) {
        server_data  = (nvDisplayDataPtr)(server_entry->data);
    }

    gpu_data = (nvGPUDataPtr)(gpu_entry->data);

    /* Query list of devices on this GPU. */
    ret = NvCtrlGetAttribute(gpu_data->handle,
                             NV_CTRL_ENABLED_DISPLAYS,
                             (int *)&enabled_displays);
    if (ret != NvCtrlSuccess || !enabled_displays) {
        goto fail;
    }

    /* Query master device */
    ret = NvCtrlGetAttribute(gpu_data->handle,
                             NV_CTRL_FRAMELOCK_MASTER,
                             (int *)&master_mask);
    if (ret != NvCtrlSuccess) {
        goto fail;
    }

    /* Query slave devices */
    ret = NvCtrlGetAttribute(gpu_data->handle,
                             NV_CTRL_FRAMELOCK_SLAVES,
                             (int *)&slaves_mask);
    if (ret != NvCtrlSuccess) {
        goto fail;
    }
    
    /* If the tree already has a master and this display is also set
     * as master, unset this entry and make it a slave. */
    if (server_entry && master_mask) {

        /* Ensure FrameLock sync is disabled before setting server/clients */
        NvCtrlSetAttribute(gpu_data->handle, NV_CTRL_FRAMELOCK_SYNC,
                           NV_CTRL_FRAMELOCK_SYNC_DISABLE);
        gpu_data->enabled = FALSE;

        ret = NvCtrlSetAttribute(gpu_data->handle,
                                 NV_CTRL_FRAMELOCK_MASTER, 0);
        if (ret != NvCtrlSuccess) {
            goto fail;
        }
        slaves_mask |= master_mask;
        master_mask = 0;
        ret = NvCtrlSetAttribute(gpu_data->handle,
                                 NV_CTRL_FRAMELOCK_SLAVES, slaves_mask);
        if (ret != NvCtrlSuccess) {
            goto fail;
        }
    }

    /* Cache the server/clients masks */
    gpu_data->server_mask = master_mask;
    gpu_data->clients_mask = slaves_mask;
    

    /* Add all enabled displays found on the GPU */
    display_mask = 1;
    while (display_mask) {
        if (display_mask & enabled_displays) {

            /* Create the display structure */
            display_data =
                (nvDisplayDataPtr) calloc(1, sizeof(nvDisplayDataRec));
            if (!display_data) {
                goto fail;
            }
            
            /* Setup the display information */
            display_data->handle      = gpu_data->handle;
            display_data->device_mask = display_mask;
            
            ret = NvCtrlGetDisplayAttribute(gpu_data->handle,
                                            display_mask,
                                            NV_CTRL_FRAMELOCK_MASTERABLE,
                                            &(display_data->masterable));
            if (ret != NvCtrlSuccess) {
                goto fail;
            }

            ret = NvCtrlGetDisplayAttribute(gpu_data->handle,
                                            display_mask,
                                            NV_CTRL_REFRESH_RATE,
                                            (int *)&(display_data->rate));
            if (ret != NvCtrlSuccess) {
                goto fail;
            }

            display_data->label           = gtk_label_new("");
            
            display_data->server_label    = gtk_label_new("Server");
            display_data->server_checkbox = gtk_check_button_new();
            ctk_config_set_tooltip(ctk_framelock->ctk_config,
                                   display_data->server_checkbox,
                                   __server_checkbox_help);

            display_data->client_label    = gtk_label_new("Client");
            display_data->client_checkbox = gtk_check_button_new();
            ctk_config_set_tooltip(ctk_framelock->ctk_config,
                                   display_data->client_checkbox,
                                   __client_checkbox_help);
            
            display_data->rate_label      = gtk_label_new("Refresh:");
            fvalue = ((float)(display_data->rate)) / 100.0f;
            snprintf(str, 32, "%.2f Hz", fvalue);
            display_data->rate_text       = gtk_label_new(str);

            display_data->stereo_label    = gtk_label_new("Stereo");
            display_data->stereo_hbox     = gtk_hbox_new(FALSE, 0);

            /* Create the display entry */
            entry = list_entry_new_with_display(display_data);

            update_entry_label(ctk_framelock, entry);
            list_entry_update_status(ctk_framelock, entry);
                       
            /* Add display to GPU entry */
            list_entry_add_child(gpu_entry, entry);
            
            /* Setup state */
            if (!display_data->masterable) {
                gtk_widget_set_sensitive(display_data->server_label, FALSE);
                gtk_widget_set_sensitive(display_data->server_checkbox, FALSE);

            } else if (master_mask & display_mask) {
                
                /* If this entry is the new master, make the tree point
                 * point to it so other displays that may have the master
                 * mask aren't added as masters too by mistake.
                 *
                 * NOTE: At this point the entry will not actually
                 *       be in the tree.  This gets resolved since
                 *       by adding this display device the parent
                 *       GPU and frame lock devices will also be
                 *       added.  If this changes (display device
                 *       gets added but for some reason the GPU/
                 *       frame lock device get thrown out), then
                 *       more code will be required to make sure
                 *       the tree->selected_entry is set to NULL
                 *       (if it was NULL before.)
                 */
                ((nvListTreePtr)(ctk_framelock->tree))->server_entry =
                    entry;
                gtk_toggle_button_set_active
                    (GTK_TOGGLE_BUTTON(display_data->server_checkbox), TRUE);
                gtk_widget_set_sensitive(display_data->client_label, FALSE);
                gtk_widget_set_sensitive(display_data->client_checkbox, FALSE);
            }

            /* Set display device as slave */
            if (slaves_mask & display_mask) {
                gtk_toggle_button_set_active
                    (GTK_TOGGLE_BUTTON(display_data->client_checkbox), TRUE);
                gtk_widget_set_sensitive(display_data->server_label, FALSE);
                gtk_widget_set_sensitive(display_data->server_checkbox, FALSE);
            }
            
            /* Connect signals */
            g_signal_connect(G_OBJECT(display_data->server_checkbox),
                             "toggled",
                             G_CALLBACK(toggle_server),
                             (gpointer) entry);
            
            g_signal_connect(G_OBJECT(display_data->client_checkbox),
                             "toggled",
                             G_CALLBACK(toggle_client),
                             (gpointer) entry);
            
            displays_added++;
        }
        display_mask <<= 1;
    }

    return displays_added;


    /* Handle failures */
 fail:  
    if (display_data) {
        free(display_data);
    }
    return displays_added;
}



/** add_gpu_devices() ************************************************
 *
 * Adds (as children list entries) all GPU devices that are bound to
 * the given frame lock list entry.
 *
 */
static unsigned int add_gpu_devices(CtkFramelock *ctk_framelock,
                                    nvListEntryPtr framelock_entry)
{
    unsigned int        num_gpus;
    unsigned int        gpus_added = 0;
    unsigned int        gpu_id;
    unsigned int        gpu_idx;
    nvGPUDataPtr        gpu_data = NULL;
    nvFrameLockDataPtr  framelock_data;
    nvListEntryPtr      entry;
    ReturnStatus        ret;
    
    unsigned char *data = NULL;
    int            len  = 0;
    int           *gpus;
    

    if (!framelock_entry ||
        framelock_entry->data_type != ENTRY_DATA_FRAMELOCK) {
        goto fail;
    }

    /* Get number of GPU devices connected to this frame lock board */
    framelock_data = (nvFrameLockDataPtr)(framelock_entry->data);
    ret = NvCtrlGetBinaryAttribute(framelock_data->handle,
                                   0,
                                   NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
                                   &data,
                                   &len);
    if (ret != NvCtrlSuccess) {
        goto fail;
    }

    gpus     = (int *)data;
    num_gpus = gpus[0];
    for (gpu_idx = 0; gpu_idx < num_gpus; gpu_idx++) {
        int  displays_added = 0;

        gpu_id = gpus[gpu_idx +1];

        /* Create the GPU data structure */
        gpu_data = (nvGPUDataPtr) calloc(1, sizeof(nvGPUDataRec));
        if (!gpu_data) {
            goto fail;
        }

        /* Create the GPU handle and label */
        gpu_data->handle =
            NvCtrlAttributeInit(NvCtrlGetDisplayPtr(framelock_data->handle),
                                NV_CTRL_TARGET_TYPE_GPU,
                                gpu_id,
                                NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
        gpu_data->label = gtk_label_new("");

        gpu_data->timing_label = gtk_label_new("Timing");
        gpu_data->timing_hbox = gtk_hbox_new(FALSE, 0);

        /* Create the GPU list entry */
        entry = list_entry_new_with_gpu(gpu_data);
        
        update_entry_label(ctk_framelock, entry);
        list_entry_update_status(ctk_framelock, entry);

        /* Add Displays tied to this GPU */
        displays_added = add_display_devices(ctk_framelock, entry);
        if (displays_added) {
            int i;

            list_entry_add_child(framelock_entry, entry);

            /* Check to see if we should reflect in the GUI that
             * frame lock is enabled.  This should happen if we are
             * adding a gpu that has FRAMELOCK_SYNC set to enable.
             */
            NvCtrlGetAttribute(gpu_data->handle,
                               NV_CTRL_FRAMELOCK_SYNC,
                               &(gpu_data->enabled));
            ctk_framelock->framelock_enabled |= gpu_data->enabled;

            entry->ctk_event = CTK_EVENT(ctk_event_new(gpu_data->handle));

            for (i = 0; i < NUM_GPU_SIGNALS; i++) {
                gpu_data->signal_ids[i] =
                    g_signal_connect(G_OBJECT(entry->ctk_event),
                                     __GPUSignals[i],
                                     G_CALLBACK(gpu_state_received),
                                     (gpointer) entry);
            }
            
            gpus_added++;
        } else {
            /* No Displays found, don't add this GPU device */
            list_entry_free(entry);
        }
    }

    return gpus_added;


    /* Handle failures */
 fail:
    if (gpu_data) {
        if (gpu_data->handle) {
            NvCtrlAttributeClose(gpu_data->handle);
        }
        free(gpu_data);
    }
    return gpus_added;
}



/** add_framelock_devices() ******************************************
 *
 * Adds all frame lock devices found on the given server handle to
 * the frame lock group,
 *
 */
static unsigned int add_framelock_devices(CtkFramelock *ctk_framelock,
                                          gpointer handle)
{
    unsigned int        num_framelocks;
    unsigned int        framelocks_added = 0;
    unsigned int        framelock_id;
    nvFrameLockDataPtr  framelock_data = NULL;
    nvListEntryPtr      entry;
    ReturnStatus        ret;


    /* Get number of G-Sync devices on this server */

    ret = NvCtrlQueryTargetCount(handle,
                                 NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                 (int *)&num_framelocks);
    if (ret != NvCtrlSuccess) {
        goto fail;
    }

    /* Add frame lock devices found */
    for (framelock_id = 0; framelock_id < num_framelocks; framelock_id++) {
        int gpus_added = 0;
        int val;
        char *revision_str = NULL;

        /* Create the frame lock data structure */
        framelock_data =
            (nvFrameLockDataPtr) calloc(1, sizeof(nvFrameLockDataRec));
        if (!framelock_data) {
            goto fail;
        }

        /* Create the frame lock handle */
        framelock_data->handle =
            NvCtrlAttributeInit(NvCtrlGetDisplayPtr(handle),
                                NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                framelock_id,
                                NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);

        /* Get the framelock revision information */
        ret = NvCtrlGetAttribute(framelock_data->handle,
                                 NV_CTRL_FRAMELOCK_FPGA_REVISION,
                                 &val);
        if (ret != NvCtrlSuccess) {
            goto fail;
        }
        revision_str = g_strdup_printf("%d", val);

        /* Create the frame lock widgets */
        framelock_data->label = gtk_label_new("");
        
        framelock_data->receiving_label = gtk_label_new("Receiving");
        framelock_data->receiving_hbox = gtk_hbox_new(FALSE, 0);

        framelock_data->rate_label = gtk_label_new("Rate:");
        framelock_data->rate_text = gtk_label_new("");

        framelock_data->delay_label = gtk_label_new("Delay:");
        framelock_data->delay_text = gtk_label_new("");

        framelock_data->house_label = gtk_label_new("House");
        framelock_data->house_hbox = gtk_hbox_new(FALSE, 0);

        framelock_data->port0_label = gtk_label_new("Port 0");
        framelock_data->port0_hbox = gtk_hbox_new(FALSE, 0);

        framelock_data->port1_label = gtk_label_new("Port 1");
        framelock_data->port1_hbox = gtk_hbox_new(FALSE, 0);

        framelock_data->revision_label = gtk_label_new("FPGA Revision:");
        framelock_data->revision_text = gtk_label_new(revision_str);
        g_free(revision_str);

        framelock_data->extra_info_hbox = gtk_hbox_new(FALSE, 5);

        /* Create the frame lock list entry */
        entry = list_entry_new_with_framelock(framelock_data);

        update_entry_label(ctk_framelock, entry);
        list_entry_update_status(ctk_framelock, entry);
        
        /* Add GPUs tied to this G-Sync */
        gpus_added = add_gpu_devices(ctk_framelock, entry);
        if (gpus_added) {
            int i;

            list_tree_add_entry((nvListTreePtr)(ctk_framelock->tree),
                                entry);
            
            entry->ctk_event =
                CTK_EVENT(ctk_event_new(framelock_data->handle));

            for (i = 0; i < NUM_FRAMELOCK_SIGNALS; i++) {
                framelock_data->signal_ids[i] =
                    g_signal_connect(G_OBJECT(entry->ctk_event),
                                     __FrameLockSignals[i],
                                     G_CALLBACK(framelock_state_received),
                                     (gpointer) entry);
            }

            framelocks_added++;
        } else {
            /* No GPUs found, don't add this frame lock device */
            list_entry_free(entry);
        }
    }

    return framelocks_added;


    /* Handle failures */
 fail:
    if (framelock_data) {
        if (framelock_data->handle) {
            NvCtrlAttributeClose(framelock_data->handle);
        }
        free(framelock_data);
    }
    return framelocks_added;
}



/** add_devices() ****************************************************
 *
 * Adds all frame lock devices found on the given server to the
 * frame lock group,
 *
 */
static gint add_devices(CtkFramelock *ctk_framelock,
                        const gchar *display_name,
                        gboolean error_dialog)
{
    gpointer handle = NULL;
    Display *display;
    gint devices_added = 0;
    char *server_name = NULL;
    char *ptr;

    /* if no display name specified, print an error and return */

    if (!display_name || (display_name[0] == '\0')) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" size=\"larger\">"
                      "Unable to add X Server to frame lock group.</span>\n\n"
                      "No X Server specified.");
        } else {
            nv_error_msg("Unable to add X Server to frame lock group; "
                         "no X Server specified.");
        }
        goto done;
    }

    /*
     * build the server name from the display name by removing any extra
     * server number and assuming ":0" if no server id is given
     */

    /* +2 extra characters in case we need to append ":0" */
    server_name = (char *) malloc(strlen(display_name) +3);
    if (!server_name) {
        goto done;
    }

    sprintf(server_name, "%s", display_name);
    ptr = strchr(server_name, ':');
    if (ptr) {
        /* Remove screen number information from server name */
        ptr = strchr(ptr, '.');
        if (ptr) *ptr = '\0';
    } else {
        /* Assume sever id 0 if none given */
        sprintf(server_name + strlen(server_name), ":0");
    }

    /*
     * try to prevent users from adding the same X server more than
     * once.
     *
     * XXX This is not an absolute check: this does not catch
     *     "localhost:0" versus ":0", for example, nor does it
     *     catch entering an IP vs entering a hostname.
     */
   
    if (find_server_by_name(ctk_framelock->tree, server_name)) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable to add X server "
                      "to frame lock Group</span>\n\n"
                      "The X server %s already belongs to the frame lock "
                      "Group.", server_name);
        } else {
            nv_error_msg("Unable to add X server to frame lock group; "
                         "the X server %s already belongs to the "
                         "frame lock group.", server_name);
        }
        goto done;
    }
  
    /* open an X Display connection to that X server */
    
    display = XOpenDisplay(server_name);
    if (!display) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable "
                      "to add devices to frame lock group</span>\n\nUnable to "
                      "connect to X Display '%s'.", server_name);
        } else {
            nv_error_msg("Unable to add devices to frame lock group; unable "
                         "to connect to X Display '%s'.", server_name);
        }
        goto done;
    }
    
    /* create a new NV-CONTROL handle */
    
    handle = NvCtrlAttributeInit(display, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                 DefaultScreen(display),
                                 NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
    if (!handle) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">Unable "
                      "to add devices to frame lock group</span>\n\nUnable to "
                      "create NV-CONTROL handle.");
        } else {
            nv_error_msg("Unable to add devices to frame lock group; unable "
                         "create NV-CONTROL handle.");
        }
        goto done;
    }
    
    /* Add frame lock devices found on server */

    devices_added = add_framelock_devices(ctk_framelock, handle);
    if (!devices_added) {
        if (error_dialog) {
            error_msg(ctk_framelock, "<span weight=\"bold\" "
                      "size=\"larger\">No frame lock devices "
                      "found on server.</span>\n\n"
                      "This X Server does not support frame lock or "
                      "no frame lock devices were available.");
        } else {
            nv_error_msg("No frame lock devices found on server; "
                         "This X Server does not support frame lock or "
                         "no frame lock devices were available.");
        }
        goto done;
    }


    /* Align the list entry titles */
    list_tree_align_titles((nvListTreePtr)(ctk_framelock->tree));


    /* Fall through */
 done:
    if (server_name) {
        free(server_name);
    }
    if (handle) {
        NvCtrlAttributeClose(handle);
    }

    return devices_added;
}



/** add_entry_to_parsed_attributes() *********************************
 *
 * Adds information reguarding a list entry (GPU or Frame Lock
 * device) to the parsed attribute list.
 *
 */
#define __ADD_ATTR(x,y,z)                               \
        a.display              = display_name;          \
        a.target_type          = target_type;           \
        a.target_id            = target_id;             \
        a.attr                 = (x);                   \
        a.val                  = (y);                   \
        a.display_device_mask  = (z);                   \
        a.flags               |= NV_PARSER_HAS_TARGET;  \
        nv_parsed_attribute_add(head, &a);

static void add_entry_to_parsed_attributes(nvListEntryPtr entry,
                                             ParsedAttribute *head)
{
    ParsedAttribute a;
    char *display_name = NULL;
    int target_type = 0;
    int target_id = 0;

    if (!entry) return;
   
    switch (entry->data_type) {

    case ENTRY_DATA_FRAMELOCK:
        {
            int use_house_sync;
            nvFrameLockDataPtr data = (nvFrameLockDataPtr)(entry->data);
            display_name = NvCtrlGetDisplayName(data->handle);
            target_type = NV_CTRL_TARGET_TYPE_FRAMELOCK;
            target_id = NvCtrlGetTargetId(data->handle);
            
            NvCtrlGetAttribute(data->handle, NV_CTRL_USE_HOUSE_SYNC,
                               &use_house_sync);

            __ADD_ATTR(NV_CTRL_USE_HOUSE_SYNC, use_house_sync, 0);

            /* If use house sync is enabled, also save other house sync info */
            if (use_house_sync) {
                int sync_interval;
                int sync_edge;
                int video_mode;
                
                NvCtrlGetAttribute(data->handle,
                                   NV_CTRL_FRAMELOCK_SYNC_INTERVAL,
                                   &sync_interval);
                NvCtrlGetAttribute(data->handle,
                                   NV_CTRL_FRAMELOCK_POLARITY,
                                   &sync_edge);
                NvCtrlGetAttribute(data->handle,
                                   NV_CTRL_FRAMELOCK_VIDEO_MODE,
                                   &video_mode);

                __ADD_ATTR(NV_CTRL_FRAMELOCK_SYNC_INTERVAL, sync_interval, 0);
                __ADD_ATTR(NV_CTRL_FRAMELOCK_POLARITY, sync_edge, 0);
                __ADD_ATTR(NV_CTRL_FRAMELOCK_VIDEO_MODE, video_mode, 0);
            }

            if (display_name) {
                free(display_name);
            }
        } 
        break;

    case ENTRY_DATA_GPU:
        {
            nvGPUDataPtr data = (nvGPUDataPtr)(entry->data);
            display_name = NvCtrlGetDisplayName(data->handle);
            target_type = NV_CTRL_TARGET_TYPE_GPU;
            target_id = NvCtrlGetTargetId(data->handle);

            __ADD_ATTR(NV_CTRL_FRAMELOCK_MASTER, data->server_mask, 0);
            __ADD_ATTR(NV_CTRL_FRAMELOCK_SLAVES, data->clients_mask, 0);

            if (display_name) {
                free(display_name);
            }
        }   
        break;

    case ENTRY_DATA_DISPLAY:
        /* Nothign to save */
        break;

    default:
        /* Oops */
        break;
    }
}

#undef __ADD_ATTR



/** add_entries_to_parsed_attributes() *******************************
 *
 * Adds GPU settings for server/clients to the parsed attribute
 * list.
 *
 */
static void add_entries_to_parsed_attributes(nvListEntryPtr entry,
                                             ParsedAttribute *head)
{
    if (!entry) return;

    /* Add GPU entries to parsed attributes list */
    if (entry->data_type == ENTRY_DATA_GPU) {
        add_entry_to_parsed_attributes(entry, head);
    }
    
    /* add children */
    add_entries_to_parsed_attributes(entry->children, head);

    /* add siblings */
    add_entries_to_parsed_attributes(entry->next_sibling, head);
}



/* ctk_framelock_config_file_attributes() ****************************
 *
 * Add to the ParsedAttribute list any attributes that we want saved
 * in the config file.
 *
 * This includes all the clients/server bitmasks for all GPUs and
 * the house sync settings of the selected master frame lock device.
 *
 */
void ctk_framelock_config_file_attributes(GtkWidget *w,
                                          ParsedAttribute *head)
{
    CtkFramelock *ctk_framelock = (CtkFramelock *) w;

    /* Add attributes from all the list entries */
    add_entries_to_parsed_attributes
        (((nvListTreePtr)(ctk_framelock->tree))->entries, head);

    /* Save the frame lock server's hous esync settings */
    add_entry_to_parsed_attributes
        (get_framelock_server_entry((nvListTreePtr)(ctk_framelock->tree)),
         head);
}



/** apply_parsed_attribute_list() ***********************************
 *
 * Given a list of parsed attributes from the config file, add all
 * X Servers (and their devices) that have to do with frame lock
 * to the current frame lock group.
 *
 */
static void apply_parsed_attribute_list(CtkFramelock *ctk_framelock,
                                        ParsedAttribute *list)
{
    ParsedAttribute *p;
    char *server_name = NULL;

    /* Add frame lock devices for all servers */

    for (p = list; p && p->next; p = p->next) {
        if (server_name) {
            free(server_name);
        }
        server_name = nv_standardize_screen_name(p->display, -2);
        if (!server_name) continue;
    
        /* Not a frame lock attribute */
        if (!(p->flags & NV_PARSER_TYPE_FRAMELOCK)) continue;

        /* Server already added */
        if (find_server_by_name
            ((nvListTreePtr)(ctk_framelock->tree), server_name)) continue;

        /* Add all the devices from this attribute's server */
        add_devices(ctk_framelock, server_name, FALSE);
    }

    if (server_name) {
        free(server_name);
    }
}



/** ctk_framelock_create_help() **************************************
 *
 * Function to create the frame lock help page.
 *
 */
GtkTextBuffer *ctk_framelock_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Frame Lock Help");

    ctk_help_para(b, &i, "The frame lock control page provides a way to "
                  "manage an entire cluster of workstations in a frame lock "
                  "group.");

    /* G-Sync Frame Help */

    ctk_help_heading(b, &i, "G-Sync Section");
    ctk_help_para(b, &i, "The G-Sync section allows you to configure the "
                  "individual devices that make up the frame lock group.");

    ctk_help_heading(b, &i, "G-Sync Device Entry Information");
    ctk_help_para(b, &i, "G-Sync (frame lock board) device entries display "
                  "the following information:");
    ctk_help_para(b, &i, "The X server name and G-Sync board ID.");
    ctk_help_para(b, &i, "Receiving LED: This indicates whether the frame "
                  "lock board is receiving a sync pulse.  Green means a "
                  "signal is detected; red means a signal is not detected.  "
                  "The sync pulse can come from one of the following sources: "
                  "The House Sync signal, an external signal from another "
                  "frame lock device coming into Port0/Port1, or the internal "
                  "timing from the primary GPU's display device");
    ctk_help_para(b, &i, "Rate Information: This is the sync rate that the "
                  "frame lock board is receiving.");
    ctk_help_para(b, &i, "House LED: This indicates whether the frame lock "
                  "board is receiving synchronization from the house (BNC) "
                  "connector.  This LED mirrors the status of the BNC LED on "
                  "the backplane of the frame lock board.");
    ctk_help_para(b, &i, "Port0, Port1 Images: These indicate the status of "
                  "the RJ45 ports on the backplane of the frame lock board.  "
                  "Green LEDs indicate that the port is configured for "
                  "input, while yellow LEDs indicate that the port is "
                  "configured for output.");
    ctk_help_para(b, &i, "Delay Information: The sync delay (in microseconds) "
                     "between the frame lock pulse and the GPU pulse.");

    ctk_help_heading(b, &i, "GPU Device Entry Information");
    ctk_help_para(b, &i, "GPU Device entries display the GPU name and number "
                  "of a GPU connected to a G-Sync device.  Display devices "
                  "driven by the GPU will be listed under this entry.");
    ctk_help_para(b, &i, "Timing LED: This indicates that the GPU "
                  "is synchronized with the incoming timing signal from the "
                  "G-Sync device");

    ctk_help_heading(b, &i, "Display Device Entry Information");
    ctk_help_para(b, &i, "Display Device entries display information and "
                  "configuration options for configuring how the dislay "
                  "device should behave in the frame lock group.  Setting  of "
                  "options is only available while frame lock is disabled.  "
                  "The following options are available:");
    ctk_help_para(b, &i, __server_checkbox_help);
    ctk_help_para(b, &i, __client_checkbox_help);
    ctk_help_para(b, &i, "Stereo LED: This indicates whether or not the "
                  "display device is sync'ed to the stereo signal coming from "
                  "the G-Sync device.  This LED is only available to display "
                  "devices set as clients when frame lock is enabled.");

    ctk_help_heading(b, &i, "Adding Devices");
    ctk_help_para(b, &i, __add_devices_button_help);
    ctk_help_para(b, &i, "If the X Server is remote, be sure you have "
                  "configured remote access (via `xhost`, for example) "
                  "such that you are allowed to establish a connection.");
    
    ctk_help_heading(b, &i, "Removing Devices");
    ctk_help_para(b, &i, __remove_devices_button_help);

    /* House Sync Frame Help */

    ctk_help_heading(b, &i, "House Sync Section");
    ctk_help_para(b, &i, "The House Sync section allows you to configure "
                  "the selected server G-Sync board for using an incoming "
                  "house sync signal instead of internal GPU timings.  This "
                  "section is only accesible by selecting a server display "
                  "device (See Display Device Information above.");

    ctk_help_heading(b, &i, "Use House Sync on Server");
    ctk_help_para(b, &i, __use_house_sync_button_help);
    ctk_help_para(b, &i, "If this option is checked and no house signal "
                  "is detected (House LED is red), the G-Sync device will "
                  "fall back to using internal timings from the primary GPU.");

    ctk_help_heading(b, &i, "Sync Interval");
    ctk_help_para(b, &i, __sync_interval_entry_help);

    ctk_help_heading(b, &i, "Sync Edge");
    ctk_help_para(b, &i, __sync_edge_combo_help);
    ctk_help_para(b, &i, "Syncing to the rising (leading) edge should be "
                  "suitable for bi-level and TTL signals.  Syncing to the "
                  "falling edge should be used for tri-level signals.  "
                  "Syncing to both edges should only be needed for TTL "
                  "signals that have problems syncing to the rising edge "
                  "only.");

    ctk_help_heading(b, &i, "Video Mode");
    ctk_help_para(b, &i, __video_mode_combo_help);

    ctk_help_heading(b, &i, "Video Mode Detect");
    ctk_help_para(b, &i, __detect_video_mode_button_help);

    /* Button Help */
    
    ctk_help_heading(b, &i, "Test Link");
    ctk_help_para(b, &i, "Use this toggle button to enable testing of "
                  "the cabling between all members of the frame lock group.  "
                  "This will cause all frame lock boards to receive a sync "
                  "pulse, but the GPUs will not lock to the frame lock "
                  "pulse.  When Test Link is enabled, no other settings may "
                  "be changed until you disable Test Link.");

    ctk_help_heading(b, &i, "Enable Frame Lock");
    ctk_help_para(b, &i, __sync_enable_button_help);
    ctk_help_para(b, &i, "Only devices selected as clients or server will be "
                  "enabled.");

    /* Misc Help */

    ctk_help_heading(b, &i, "Miscellaneous");
    ctk_help_para(b, &i, "The frame lock control page registers several "
                  "timers that are executed periodically; these are listed "
                  "in the 'Active Timers' section of the 'nvidia-settings "
                  "Configuration' page.  Most notably is the 'Frame Lock "
                  "Connection Status' timer: this will poll all members of "
                  "the frame lock group for status information.");

    ctk_help_finish(b);

    return b;
}



/** ctk_framelock_select() *******************************************
 *
 * Callback function for when the frame lock page is being displayed
 * in the control panel.
 *
 */
void ctk_framelock_select(GtkWidget *w)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(w);

    /* Start the frame lock timers */

    ctk_config_start_timer(ctk_framelock->ctk_config,
                           (GSourceFunc) update_framelock_status,
                           (gpointer) ctk_framelock);

    ctk_config_start_timer(ctk_framelock->ctk_config,
                           (GSourceFunc) check_for_ethernet,
                           (gpointer) ctk_framelock);
}



/** ctk_framelock_unselect() *****************************************
 *
 * Callback function for when the frame lock page is no longer being
 * displayed by the control panel.  (User clicked on another page.)
 *
 */
void ctk_framelock_unselect(GtkWidget *w)
{
    CtkFramelock *ctk_framelock = CTK_FRAMELOCK(w);

    /* Stop the frame lock timers */

    ctk_config_stop_timer(ctk_framelock->ctk_config,
                          (GSourceFunc) update_framelock_status,
                          (gpointer) ctk_framelock);

    ctk_config_stop_timer(ctk_framelock->ctk_config,
                          (GSourceFunc) check_for_ethernet,
                          (gpointer) ctk_framelock);
}
