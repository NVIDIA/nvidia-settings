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

/**** INCLUDES ***************************************************************/

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "ctkbanner.h"

#include "ctkclocks.h"

#include "ctklicense.h"
#include "ctkscale.h"
#include "ctkhelp.h"
#include "ctkevent.h"
#include "ctkconstants.h"



/**** DEFINES ****************************************************************/

/* GUI padding space around frames */

#define FRAME_PADDING 5



/**** MACROS *****************************************************************/

/* Setting/Getting GPU and Memory frequencies from packed int */

#define GET_GPU_CLOCK(C)   ( (C) >> 16 )
#define GET_MEM_CLOCK(C)   ( (C) & 0xFFFF )
#define MAKE_CLOCKS(G, M)  ( ((G) << 16) | ((M) & 0xFFFF) )



/**** PROTOTYPES *************************************************************/

static void overclocking_state_update_gui(CtkClocks *ctk_object);
static void overclocking_state_toggled(GtkWidget *widget, gpointer user_data);
static void overclocking_state_received(GtkObject *object, gpointer arg1,
                                        gpointer user_data);

static void auto_detection_state_received(GtkObject *object, gpointer arg1,
                                          gpointer user_data);

static void sync_gui_to_modify_clocks(CtkClocks *ctk_object, int which_clocks);

static void set_clocks_value(CtkClocks *ctk_object, int clocks,
                             int which_clocks);
static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data);

static void clock_menu_changed(GtkOptionMenu *option_menu, gpointer user_data);

static void apply_clocks_clicked(GtkWidget *widget, gpointer user_data);
static void detect_clocks_clicked(GtkWidget *widget, gpointer user_data);
static void reset_clocks_clicked(GtkWidget *widget, gpointer user_data);

static void clocks_received(GtkObject *object, gpointer arg1,
                            gpointer user_data);



/**** GLOBALS ****************************************************************/

static gboolean __license_accepted = FALSE;

/* Tooltips */

static const char * __enable_button_help =
"The Enable Overclocking checkbox enables access to GPU and graphics card "
"memory interface overclocking functionality.  Note that overclocking your "
"GPU and/or graphics card memory interface is not recommended and is done "
"at your own risk.  You should never have to enable this.";

static const char * __clock_menu_help = 
"Selects which clock frequencies to modify.  Standard (2D) only affects 2D "
"applications.  Performance (3D) only affects 3D applications.";

static const char * __graphics_clock_help =
"The Graphics Clock Frequency is the core clock speed that the NVIDIA "
"GPU will be set to when the graphics card is operating in this mode (2D/3D).";

static const char * __mem_clock_help =
"The Memory Clock Frequency is the clock speed of the memory interface on "
"the graphics card.  On some systems, the clock frequency is required to "
"be the same for both 2D and 3D modes.  For these systems, setting the 2D "
"memory clock frequency will also set the 3D memory clock frequency.";

static const char * __apply_button_help =
"The Apply button allows you to set the desired clock frequencies for the "
"GPU and graphics card memory interface.  Slider positions are only applied "
"after clicking this button.";

static const char * __detect_button_help =
"The Auto Detect button determines the maximum clock setting that is safe "
"on your system at this instant.  The maximum clock setting determined here "
"can vary on consecutive runs and depends on how well the system handles the "
"auto-detection stress tests.   This is only available for 3D clock "
"frequencies.  You must click the Apply button to set the results found.";

static const char * __cancel_button_help =
"The Cancel Detection button allows you to cancel testing for the optimal 3D "
"clock frequencies.";

static const char * __reset_button_help =
"The Reset Hardware Defaults button lets you restore the original GPU and "
"memory interface clock frequencies.";


/* Messages */

static const char * __detect_confirm_msg = 
"To find the best 3D clock frequencies your system supports,\n"
"a series of tests will take place.  This testing may take several "
"minutes.\n";


static const char * __detect_wait_msg =
"Optimal 3D clock frequencies are being probed, please wait...";

static const char * __canceled_msg =
"Probing for optimal 3D clock frequencies has been canceled.";



/**** FUNCTIONS **************************************************************/

/*****
 *
 * Returns the gpu overclocking ctk object type.
 *
 */
GType ctk_clocks_get_type(void)
{
    static GType ctk_object_type = 0;

    if (!ctk_object_type) {
        static const GTypeInfo ctk_object_info = {
            sizeof (CtkClocksClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkClocks),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_object_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkClocks", &ctk_object_info, 0);
    }

    return ctk_object_type;

} /* ctk_clocks_get_type() */


/*****
 *
 * Main CTK widget creation.
 *
 */
GtkWidget* ctk_clocks_new(NvCtrlAttributeHandle *handle,
                          CtkConfig *ctk_config,
                          CtkEvent *ctk_event)
{
    GObject *object;
    CtkClocks *ctk_object;
    GtkObject *adjustment;
    GtkWidget *alignment;
    GtkWidget *scale;
    GtkWidget *menu;
    GtkWidget *menu_item;

    GtkWidget *label;   

    GtkWidget *frame;
    GtkWidget *banner;
    GtkWidget *hbox;
    GtkWidget *vbox;


    ReturnStatus ret;  /* NvCtrlxxx function return value */
    int value;
    int clocks_2D;
    NVCTRLAttributeValidValuesRec ranges_2D;
    NVCTRLAttributeValidValuesRec range_detection;
    int clocks_3D;
    NVCTRLAttributeValidValuesRec ranges_3D;

    Bool overclocking_enabled;
    Bool auto_detection_available = FALSE;
    Bool probing_optimal = FALSE;
    Bool can_access_2d_clocks;
    Bool can_access_3d_clocks;
   

    /* Make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /* If we can't query the overclocking state, don't load the page */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_OVERCLOCKING_STATE,
                             &value);
    if ( ret != NvCtrlSuccess )
        return NULL;
    overclocking_enabled =
        (value==NV_CTRL_GPU_OVERCLOCKING_STATE_MANUAL)?True:False;
   
    /* Check if overclocking is busy */
    
    if ( overclocking_enabled ) {
        ret = NvCtrlGetValidAttributeValues(handle,
                                 NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION,
                                 &range_detection);
        if ( ret == NvCtrlSuccess ) {
            ret = NvCtrlGetAttribute(handle,
                                     NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE,
                                     &value);
            if ( ret != NvCtrlSuccess )
                return NULL;
            probing_optimal =
                (value == NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_BUSY);
            auto_detection_available = TRUE;
        }
    }

    /* Can we access the 2D clocks? */

    can_access_2d_clocks = True;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_2D_CLOCK_FREQS, &clocks_2D);
    if ( ret != NvCtrlSuccess )
        can_access_2d_clocks = False;
    ret = NvCtrlGetValidAttributeValues(handle, NV_CTRL_GPU_2D_CLOCK_FREQS,
                                        &ranges_2D);
    if ( ret != NvCtrlSuccess )
        can_access_2d_clocks = False;

    /* Can we access the 3D clocks? */

    can_access_3d_clocks = True;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_3D_CLOCK_FREQS, &clocks_3D);
    if ( ret != NvCtrlSuccess )
        can_access_3d_clocks = False;
    ret = NvCtrlGetValidAttributeValues(handle, NV_CTRL_GPU_3D_CLOCK_FREQS,
                                        &ranges_3D);
    if ( ret != NvCtrlSuccess )
        can_access_3d_clocks = False;

    /* If we can't access either of the clocks, don't load the page */

    if ( !can_access_2d_clocks && !can_access_3d_clocks )
        return NULL;

    /* Create the ctk object */

    object = g_object_new(CTK_TYPE_CLOCKS, NULL);
    ctk_object = CTK_CLOCKS(object);

    /* Cache the handle and configuration */

    ctk_object->handle               = handle;
    ctk_object->ctk_config           = ctk_config;
    ctk_object->overclocking_enabled = overclocking_enabled;
    ctk_object->auto_detection_available  = auto_detection_available;
    ctk_object->probing_optimal      = probing_optimal;

    if ( overclocking_enabled ) {
        __license_accepted = TRUE;
    }

    /* Create the Clock menu widget */

    menu = gtk_menu_new();
        
    if ( can_access_2d_clocks ) {
        menu_item = gtk_menu_item_new_with_label("2D Clock Frequencies");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }

    if ( can_access_3d_clocks ) {
        menu_item = gtk_menu_item_new_with_label("3D Clock Frequencies");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }
    
    ctk_object->clock_menu = gtk_option_menu_new ();
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->clock_menu), menu);

    g_signal_connect(G_OBJECT(ctk_object->clock_menu), "changed",
                     G_CALLBACK(clock_menu_changed),
                     (gpointer) ctk_object);
    
    ctk_config_set_tooltip(ctk_config, ctk_object->clock_menu,
                           __clock_menu_help);

    gtk_widget_set_sensitive(ctk_object->clock_menu,
                             overclocking_enabled && !probing_optimal);
    
    /* Create the Graphics clock frequency slider widget */

    if ( can_access_2d_clocks ) {
        adjustment =
            gtk_adjustment_new(GET_GPU_CLOCK(clocks_2D),
                               GET_GPU_CLOCK(ranges_2D.u.range.min),
                               GET_GPU_CLOCK(ranges_2D.u.range.max),
                               1, 5, 0.0);
        ctk_object->clocks_being_modified = CLOCKS_2D;
    } else {
        adjustment =
            gtk_adjustment_new(GET_GPU_CLOCK(clocks_3D),
                               GET_GPU_CLOCK(ranges_3D.u.range.min),
                               GET_GPU_CLOCK(ranges_3D.u.range.max),
                               1, 5, 0.0);
        ctk_object->clocks_being_modified = CLOCKS_3D;
    }

    scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), "GPU (MHz)",
                          ctk_config, G_TYPE_INT);
    ctk_object->gpu_clk_scale = scale;
    
    g_signal_connect(adjustment, "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_object);

    ctk_config_set_tooltip(ctk_config,
                           CTK_SCALE(ctk_object->gpu_clk_scale)->gtk_scale,
                           __graphics_clock_help);

    gtk_widget_set_sensitive(ctk_object->gpu_clk_scale,
                             overclocking_enabled && !probing_optimal);

    /* Create the Memory clock frequency slider widget */
    
    if ( can_access_2d_clocks ) {
        adjustment =
            gtk_adjustment_new(GET_MEM_CLOCK(clocks_2D),
                               GET_MEM_CLOCK(ranges_2D.u.range.min),
                               GET_MEM_CLOCK(ranges_2D.u.range.max),
                               1, 5, 0.0);
    } else {
        adjustment =
            gtk_adjustment_new(GET_MEM_CLOCK(clocks_3D),
                               GET_MEM_CLOCK(ranges_3D.u.range.min),
                               GET_MEM_CLOCK(ranges_3D.u.range.max),
                               1, 5, 0.0);
    }

    scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), "Memory (MHz)",
                          ctk_config, G_TYPE_INT);
    ctk_object->mem_clk_scale = scale;
    
    g_signal_connect(adjustment, "value_changed",
                     G_CALLBACK(adjustment_value_changed),
                     (gpointer) ctk_object);
    
    ctk_config_set_tooltip(ctk_config,
                           CTK_SCALE(ctk_object->mem_clk_scale)->gtk_scale,
                           __mem_clock_help);

    gtk_widget_set_sensitive(ctk_object->mem_clk_scale,
                             overclocking_enabled && !probing_optimal);

    /* Create the Enable Overclocking checkbox widget */

    ctk_object->enable_checkbox =
        gtk_check_button_new_with_label("Enable Overclocking");

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->enable_checkbox),
         overclocking_enabled);

    gtk_widget_set_sensitive(ctk_object->enable_checkbox,
                             overclocking_enabled && !probing_optimal);

    g_signal_connect(G_OBJECT(ctk_object->enable_checkbox), "toggled",
                     G_CALLBACK(overclocking_state_toggled),
                     (gpointer) ctk_object);

    ctk_config_set_tooltip(ctk_config, ctk_object->enable_checkbox,
                           __enable_button_help);
    
    gtk_widget_set_sensitive(ctk_object->enable_checkbox, !probing_optimal);

    /* Create the Apply button widget */

    ctk_object->apply_button =
        gtk_button_new_with_label("Apply");

    g_signal_connect(G_OBJECT(ctk_object->apply_button), "clicked",
                     G_CALLBACK(apply_clocks_clicked),
                     (gpointer) ctk_object);
    
    ctk_config_set_tooltip(ctk_config, ctk_object->apply_button,
                           __apply_button_help);

    gtk_widget_set_sensitive(ctk_object->apply_button, False);
    
    /* Create the Auto Detect button widget */

    ctk_object->detect_button =
        gtk_button_new_with_label("Auto Detect");

    g_signal_connect(G_OBJECT(ctk_object->detect_button), "clicked",
                     G_CALLBACK(detect_clocks_clicked),
                     (gpointer) ctk_object);
    
    ctk_config_set_tooltip(ctk_config, ctk_object->detect_button,
                           __detect_button_help);
    
    if ( ctk_object->clocks_being_modified == CLOCKS_2D ) {
        gtk_widget_set_sensitive(ctk_object->detect_button, False);
    } else {
        gtk_widget_set_sensitive(ctk_object->detect_button,
                                 overclocking_enabled &&
                                 auto_detection_available && !probing_optimal);
    }

    /* Create the Reset hardware button widget */

    ctk_object->reset_button =
        gtk_button_new_with_label("Reset Hardware Defaults");

    g_signal_connect(G_OBJECT(ctk_object->reset_button), "clicked",
                     G_CALLBACK(reset_clocks_clicked),
                     (gpointer) ctk_object);
    
    ctk_config_set_tooltip(ctk_config, ctk_object->reset_button,
                           __reset_button_help);

    gtk_widget_set_sensitive(ctk_object->reset_button, False);

    /* Create the enable dialog */

    ctk_object->license_dialog = ctk_license_dialog_new(GTK_WIDGET(ctk_object),
                                                        "Clock Frequencies");

    /* Create the auto detect dialog */

    ctk_object->detect_dialog =
        gtk_dialog_new_with_buttons("Auto Detect Optimal 3D Clock Frequencies?",
                                    GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
                                    GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
                                    GTK_STOCK_OK,
                                    GTK_RESPONSE_ACCEPT,
                                    GTK_STOCK_CANCEL,
                                    GTK_RESPONSE_REJECT,
                                    NULL
                                    );
    
    label = gtk_label_new(__detect_confirm_msg);
    hbox = gtk_hbox_new(TRUE, 15);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ctk_object->detect_dialog)->vbox),
                       hbox, FALSE, FALSE, 15);

    /*
     * Now that we've created all the widgets we care about, we're
     * ready to compose the panel
     */

    /* Set container properties of the ctk object */

    gtk_box_set_spacing(GTK_BOX(ctk_object), 10);

    banner = ctk_banner_image_new(BANNER_ARTWORK_CLOCK);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /* Add Overclocking checkbox */

    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(hbox), ctk_object->enable_checkbox,
                       FALSE, FALSE, 0);

    /* Add Clock frequency frame */

    frame = gtk_frame_new("Clock Frequencies");
    vbox = gtk_vbox_new(FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 0);
            
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
            
    
    gtk_box_pack_start(GTK_BOX(hbox), ctk_object->clock_menu,
                       FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox,
                       FALSE, FALSE, 5);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_object->gpu_clk_scale,
                       FALSE, FALSE, 5);
            
    gtk_box_pack_start(GTK_BOX(vbox), ctk_object->mem_clk_scale,
                       FALSE, FALSE, 5);                
        
    /* Add the Apply, Auto Detect, and Reset buttons */

    hbox = gtk_hbox_new(FALSE, 5);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_object->apply_button,
                       FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(hbox), ctk_object->detect_button);
    gtk_container_add(GTK_CONTAINER(hbox), ctk_object->reset_button);
        
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* Setup the initial gui state */
    
    sync_gui_to_modify_clocks(ctk_object, ctk_object->clocks_being_modified);
    
    /* Handle events from other NV-CONTROL clients */
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_OVERCLOCKING_STATE),
                     G_CALLBACK(overclocking_state_received),
                     (gpointer) ctk_object);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_2D_CLOCK_FREQS),
                     G_CALLBACK(clocks_received),
                     (gpointer) ctk_object);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_3D_CLOCK_FREQS),
                     G_CALLBACK(clocks_received),
                     (gpointer) ctk_object);
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS),
                     G_CALLBACK(clocks_received),
                     (gpointer) ctk_object);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE),
                     G_CALLBACK(auto_detection_state_received),
                     (gpointer) ctk_object);

    /* Show the widget */

    gtk_widget_show_all(GTK_WIDGET(ctk_object));

    return GTK_WIDGET(ctk_object);

} /* ctk_clocks_new() */



/*****
 *
 * GPU overclocking help screen.
 *
 */
GtkTextBuffer *ctk_clocks_create_help(GtkTextTagTable *table,
                                      CtkClocks *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Clock Frequency Overclocking Help");
    ctk_help_para(b, &i,
                  "WARNING: Overclocking has the potential of destroying your "
                  "graphics card, CPU, RAM and any other component.  It may "
                  "also reduce the life expectancy of your components and "
                  "void manufacturer warranties.  DO THIS AT YOUR OWN RISK."
                  );
    ctk_help_heading(b, &i, "Enabeling Clock Frequencies");
    ctk_help_para(b, &i, __enable_button_help);
    ctk_help_para(b, &i,
                  "GPU Overclocking functionality is currently limited to "
                  "GeForce FX and newer non-mobile GPUs."
                  );
    ctk_help_heading(b, &i, "2D/3D Clock Frequencies");
    ctk_help_para(b, &i,
                  "The 2D clock frequencies are the standard clock "
                  "frequencies used when only 2D applications are running."
                  );
    ctk_help_para(b, &i,
                  "The 3D clock frequencies are the performance clock "
                  "frequencies used when running 3D applications."
                  );
    ctk_help_heading(b, &i, "Graphics Clock Frequency");
    ctk_help_para(b, &i, __graphics_clock_help);
    ctk_help_heading(b, &i, "Memory Clock Frequency");
    ctk_help_para(b, &i, __mem_clock_help);
    ctk_help_heading(b, &i, "Applying Custom Clock Frequencies");
    ctk_help_para(b, &i, __apply_button_help);
    ctk_help_heading(b, &i, "Auto Detect Optimal 3D Clock Frequencies");
    ctk_help_para(b, &i, __detect_button_help);
    ctk_help_heading(b, &i, "Canceling Optimal 3D Clock Frequency Auto "
                     "Dection.");
    ctk_help_para(b, &i, __cancel_button_help);
    ctk_help_para(b, &i,
                  "This button is only available if the Optimal "
                  "clocks are currently being probed.");
    ctk_help_heading(b, &i, "Restoring Hardware Default Frequencies");
    ctk_help_para(b, &i, __reset_button_help);

    ctk_help_finish(b);

    return b;

} /* ctk_clocks_create_help() */



/****
 *
 * Updates sensitivity of widgets in relation to the state
 * of overclocking.
 *
 */
static void sync_gui_sensitivity(CtkClocks *ctk_object)
{
    gboolean enabled = ctk_object->overclocking_enabled;
    gboolean probing = ctk_object->probing_optimal;
    gboolean modified = ctk_object->clocks_modified;
    gboolean moved = ctk_object->clocks_moved;


    /* Update the enable checkbox */

    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->enable_checkbox),
                                    G_CALLBACK(overclocking_state_toggled),
                                    (gpointer) ctk_object);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctk_object->enable_checkbox),
                                 enabled);
    
    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->enable_checkbox),
                                      G_CALLBACK(overclocking_state_toggled),
                                      (gpointer) ctk_object);

    gtk_widget_set_sensitive(ctk_object->enable_checkbox,
                             !probing);
    
    
    /* Update the clock selection dropdown */

    gtk_widget_set_sensitive(ctk_object->clock_menu, enabled && !probing);

    /* Update the Graphics clock slider */
    
    gtk_widget_set_sensitive(ctk_object->gpu_clk_scale, enabled && !probing);

    /* Update the Memory clock slider */

    gtk_widget_set_sensitive(ctk_object->mem_clk_scale, enabled && !probing);

    /* Update the Apply button */

    gtk_widget_set_sensitive(ctk_object->apply_button,
                             enabled && !probing && moved);


    /* Enable the Auto Detect button for 3D clocks only */

    if ( probing ) {
        gtk_button_set_label(GTK_BUTTON(ctk_object->detect_button), "Cancel Detection");
        gtk_widget_set_sensitive(ctk_object->detect_button, True);
        ctk_config_set_tooltip(ctk_object->ctk_config, ctk_object->detect_button,
                               __cancel_button_help);

    } else {
        gboolean set_sensitive;
        gtk_button_set_label(GTK_BUTTON(ctk_object->detect_button), "Auto Detect");
        set_sensitive = ((ctk_object->auto_detection_available) &&
                         (ctk_object->clocks_being_modified == CLOCKS_3D))
                        ? enabled : False;
        gtk_widget_set_sensitive(ctk_object->detect_button, set_sensitive);
        ctk_config_set_tooltip(ctk_object->ctk_config, ctk_object->detect_button,
                               __detect_button_help);
    }

    /* Update the Reset hardware defaults button */

    gtk_widget_set_sensitive(ctk_object->reset_button,
                             enabled && !probing && (moved || modified));

} /* sync_gui_sensitivity() */



/****
 *
 * Updates widgets in relation to current overclocking state.
 *
 */
static void overclocking_state_update_gui(CtkClocks *ctk_object)
{
    ReturnStatus ret;
    int value;
    NVCTRLAttributeValidValuesRec range_detection;
    gboolean probing_optimal = TRUE;
    gboolean enabled;


    /* We need to check the overclocking state status with 
     * the server everytime someone tries to change the state
     * because the set might have failed.
     */

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_GPU_OVERCLOCKING_STATE,
                             &value);
    if ( ret != NvCtrlSuccess )
        enabled = False;
    else
        enabled = (value==NV_CTRL_GPU_OVERCLOCKING_STATE_MANUAL)?True:False;

    ctk_object->overclocking_enabled = enabled;


    /* We need to also make sure the server is not busy probing
     * for the optimal clocks.
     */

    if ( enabled ) {
        ret = NvCtrlGetValidAttributeValues(ctk_object->handle,
                                 NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION,
                                 &range_detection);
        if ( ret == NvCtrlSuccess ) {
            ret = NvCtrlGetAttribute(ctk_object->handle,
                                     NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE,
                                     &value);
            if ( ret == NvCtrlSuccess ) {
                probing_optimal =
                    (value == NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_BUSY);
            }
            ctk_object->probing_optimal = probing_optimal;

            ctk_object->auto_detection_available = TRUE;
        }
    }

    /* Sync the gui to be able to modify the clocks */

    sync_gui_to_modify_clocks(ctk_object, ctk_object->clocks_being_modified);

    /* Update the status bar */
    
    ctk_config_statusbar_message(ctk_object->ctk_config,
                                 "GPU overclocking %sabled.",
                                 enabled?"en":"dis");

} /* overclocking_state_update_gui() */



/*****
 *
 * Signal handler - Called when the user toggles the "Enable Overclocking"
 * button.
 *
 */
static void overclocking_state_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    gboolean enabled;
    ReturnStatus ret;
    int value;
    gint result;
    
    /* Get enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    value = (enabled==1) ? NV_CTRL_GPU_OVERCLOCKING_STATE_MANUAL :
        NV_CTRL_GPU_OVERCLOCKING_STATE_NONE;

    /* Verify user knows the risks involved */

    if ( enabled && !__license_accepted ) {

        result = 
            ctk_license_run_dialog(CTK_LICENSE_DIALOG(ctk_object->license_dialog)); 
        
        switch (result)
        {
        case GTK_RESPONSE_ACCEPT:
            __license_accepted = TRUE;
            break;
            
        case GTK_RESPONSE_REJECT:
        default:
            /* Cancel */
            g_signal_handlers_block_by_func(G_OBJECT(ctk_object->enable_checkbox),
                                            G_CALLBACK(overclocking_state_toggled),
                                            (gpointer) ctk_object);
            
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
                                         FALSE);
            
            g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->enable_checkbox),
                                              G_CALLBACK(overclocking_state_toggled),
                                              (gpointer) ctk_object);
            return;
        }
    }

    
    /* Update the server */
    
    ret = NvCtrlSetAttribute(ctk_object->handle,
                             NV_CTRL_GPU_OVERCLOCKING_STATE, value);
    
    /* Update the GUI */
    
    overclocking_state_update_gui(ctk_object);

} /* enable_overclocking_toggled() */



/*****
 *
 * Signal handler - Called when another NV-CONTROL client has set the
 * overclocking state.
 *
 */
static void overclocking_state_received(GtkObject *object,
                                        gpointer arg1, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);

    /* Update GUI with enable status */
    
    overclocking_state_update_gui(ctk_object);

} /* overclocking_state_update_received() */



/*****
 *
 * Signal handler - Called when overclocking becomes busy due to
 * an NV-CONTROL client probing for the optimal clocks.
 *
 */
static void auto_detection_state_received(GtkObject *object,
                                          gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);


    /* Update GUI with probing status */

    ctk_object->probing_optimal =
        (event_struct->value == NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_BUSY);
    sync_gui_sensitivity(ctk_object);

    /* Update the status bar */

    if ( ctk_object->probing_optimal ) {
        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     __detect_wait_msg);
    } else {
        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     __canceled_msg);
    }

} /* auto_detection_state_received() */



/*****
 *
 * Syncs the gui to properly display the correct clocks the user wants to
 * modify, or has modified with another NV_CONTROL client.
 *
 */
static void sync_gui_to_modify_clocks(CtkClocks *ctk_object, int which_clocks)
{
    GtkRange *gtk_range;
    GtkAdjustment *gtk_adjustment_gpu;
    GtkAdjustment *gtk_adjustment_mem;

    ReturnStatus ret;
    int clk_values;
    int default_clk_values;
    NVCTRLAttributeValidValuesRec clk_ranges;



    /* Obtain the current value and range of the desired clocks */

    switch (which_clocks) {
    case CLOCKS_2D:
        ret = NvCtrlGetAttribute(ctk_object->handle,
                                 NV_CTRL_GPU_2D_CLOCK_FREQS, &clk_values);
        if ( ret != NvCtrlSuccess )
            return;
        ret = NvCtrlGetValidAttributeValues(ctk_object->handle,
                                            NV_CTRL_GPU_2D_CLOCK_FREQS,
                                            &clk_ranges);
        if ( ret != NvCtrlSuccess )
            return;
        break;

    case CLOCKS_3D:
        ret = NvCtrlGetAttribute(ctk_object->handle,
                                 NV_CTRL_GPU_3D_CLOCK_FREQS, &clk_values);
        if ( ret != NvCtrlSuccess )
            return;
        ret = NvCtrlGetValidAttributeValues(ctk_object->handle,
                                            NV_CTRL_GPU_3D_CLOCK_FREQS,
                                            &clk_ranges);
        if ( ret != NvCtrlSuccess )
            return;
        break;

    case CLOCKS_NONE:
    default:
        return;
    }
    

    /* See if the clocks were modified */

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             which_clocks==CLOCKS_2D?
                             NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS:
                             NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS,
                             &default_clk_values);

    ctk_object->clocks_modified =
        ((ret == NvCtrlSuccess) && (default_clk_values != clk_values));

    if ( ctk_object->clocks_being_modified != which_clocks ) {
        ctk_object->clocks_moved = False;
    }

    ctk_object->clocks_being_modified = which_clocks;
    

    /* Make sure the dropdown reflects the right clock set */
    
    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->clock_menu),
                                    G_CALLBACK(clock_menu_changed),
                                    (gpointer) ctk_object);

    gtk_option_menu_set_history(GTK_OPTION_MENU(ctk_object->clock_menu),
                                (which_clocks==CLOCKS_2D)?0:1);

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->clock_menu),
                                      G_CALLBACK(clock_menu_changed),
                                      (gpointer) ctk_object);


    /* Make GPU and Memory clocks reflect the right range/values */

    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->gpu_clk_scale)->gtk_scale);
    gtk_adjustment_gpu = GTK_ADJUSTMENT(gtk_range->adjustment);

    g_signal_handlers_block_by_func(G_OBJECT(gtk_adjustment_gpu),
                                    G_CALLBACK(adjustment_value_changed),
                                    (gpointer) ctk_object);
    gtk_range_set_range(gtk_range,
                        GET_GPU_CLOCK(clk_ranges.u.range.min),
                        GET_GPU_CLOCK(clk_ranges.u.range.max));

    g_signal_handlers_unblock_by_func(G_OBJECT(gtk_adjustment_gpu),
                                      G_CALLBACK(adjustment_value_changed),
                                      (gpointer) ctk_object);

    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->mem_clk_scale)->gtk_scale);
    gtk_adjustment_mem = GTK_ADJUSTMENT(gtk_range->adjustment);

    g_signal_handlers_block_by_func(G_OBJECT(gtk_adjustment_mem),
                                    G_CALLBACK(adjustment_value_changed),
                                    (gpointer) ctk_object);
    gtk_range_set_range(gtk_range,
                        GET_MEM_CLOCK(clk_ranges.u.range.min),
                        GET_MEM_CLOCK(clk_ranges.u.range.max));

    set_clocks_value(ctk_object, clk_values, which_clocks);

    g_signal_handlers_unblock_by_func(G_OBJECT(gtk_adjustment_mem),
                                      G_CALLBACK(adjustment_value_changed),
                                      (gpointer) ctk_object);

    /* Update the gui sensitivity */
    
    sync_gui_sensitivity(ctk_object);

} /* sync_gui_to_modify_clocks() */



/*****
 *
 * Helper function - Sets the value of the clock frequencies scales
 *
 */
static void set_clocks_value(CtkClocks *ctk_object, int clocks,
                             int which_clocks)
{
    GtkRange *gtk_range;


    /* Update the clock values */

    if ( ctk_object->gpu_clk_scale ) {
        gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->gpu_clk_scale)->gtk_scale);
        gtk_range_set_value(gtk_range, GET_GPU_CLOCK(clocks));
    }

    if ( ctk_object->mem_clk_scale ) {
        gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->mem_clk_scale)->gtk_scale);
        gtk_range_set_value(gtk_range, GET_MEM_CLOCK(clocks));
    }

} /* set_clocks_value() */



/*****
 *
 * Signal handler - Handles slider adjustments by the user.
 *
 */
static void adjustment_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);


    /* Enable the apply button */

    gtk_widget_set_sensitive(ctk_object->apply_button, True);


    /* Enable the reset button */

    gtk_widget_set_sensitive(ctk_object->reset_button, True);


    /* Set the clocks moved flag */

    ctk_object->clocks_moved = True;

} /* adjustment_value_changed() */



/*****
 *
 * Signal handler - User selected a clock set from the clock menu.
 *
 */
static void clock_menu_changed(GtkOptionMenu *option_menu, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    gint history;


    /* Sync to allow user to modify the clocks */

    history = gtk_option_menu_get_history(option_menu);
    switch (history) {
    default:
        /* Fall throught */
    case 0: /* 2D */
        sync_gui_to_modify_clocks(ctk_object, CLOCKS_2D);
        break;
    case 1: /* 3D */
        sync_gui_to_modify_clocks(ctk_object, CLOCKS_3D);
        break;
    }

} /* clock_menu_changed() */



/*****
 *
 * Signal handler - User clicked the "apply" button.
 *
 */
static void apply_clocks_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    GtkRange *gtk_range;

    ReturnStatus ret;
    int gpu_clk;
    int mem_clk;
    int clocks;


    /* Update server clocks with values from scales */

    if ( !ctk_object->gpu_clk_scale || !ctk_object->mem_clk_scale )
        return;


    /* Get new clock values from sliders */

    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->gpu_clk_scale)->gtk_scale);
    gpu_clk = gtk_range_get_value(gtk_range);
    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->mem_clk_scale)->gtk_scale);
    mem_clk = gtk_range_get_value(gtk_range);
    clocks = MAKE_CLOCKS(gpu_clk, mem_clk);


    /* Set clocks on server */

    ret = NvCtrlSetAttribute(ctk_object->handle,
                             (ctk_object->clocks_being_modified==CLOCKS_2D) ?
                             NV_CTRL_GPU_2D_CLOCK_FREQS :
                             NV_CTRL_GPU_3D_CLOCK_FREQS,
                             clocks);
    if ( ret != NvCtrlSuccess ) {
        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     "Failed to set clock frequencies!");
        return;
    }


    /* Clear the clocks moved flag */

    ctk_object->clocks_moved = False;


    /* Sync up with the server */

    sync_gui_to_modify_clocks(ctk_object, ctk_object->clocks_being_modified);

    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->gpu_clk_scale)->gtk_scale);
    gpu_clk = gtk_range_get_value(gtk_range);
    gtk_range = GTK_RANGE(CTK_SCALE(ctk_object->mem_clk_scale)->gtk_scale);
    mem_clk = gtk_range_get_value(gtk_range);

    ctk_config_statusbar_message(ctk_object->ctk_config,
                                 "Set %s clocks to (GPU) %i MHz, "
                                 "(Memory) %i MHz",
                                 (ctk_object->clocks_being_modified==CLOCKS_2D)?
                                 "2D":"3D",
                                 gpu_clk,
                                 mem_clk);

} /* apply_clocks_clicked() */



/*****
 *
 * Signal handler - User clicked the 'reset hardware defaults' button.
 *
 */
static void reset_clocks_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    int clocks;
    ReturnStatus ret;


    /* Get the default clock frequencies */

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             (ctk_object->clocks_being_modified==CLOCKS_2D) ?
                             NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS :
                             NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS,
                             &clocks);
    if ( ret != NvCtrlSuccess )
        goto fail;

    /* Set clock frequencies to use default values */

    ret = NvCtrlSetAttribute(ctk_object->handle,
                             (ctk_object->clocks_being_modified==CLOCKS_2D) ?
                             NV_CTRL_GPU_2D_CLOCK_FREQS :
                             NV_CTRL_GPU_3D_CLOCK_FREQS,
                             clocks);
    if ( ret != NvCtrlSuccess )
        goto fail;

    /* Set slider positions */

    set_clocks_value(ctk_object, clocks, ctk_object->clocks_being_modified);

    ctk_config_statusbar_message(ctk_object->ctk_config,
                                 "Reset %s clock frequency "
                                 "hardware defaults.",
                                 (ctk_object->clocks_being_modified==CLOCKS_2D)?
                                 "2D":"3D");

    /* Disable the apply button */

    gtk_widget_set_sensitive(ctk_object->apply_button, False);

    /* Disable the reset button */

    gtk_widget_set_sensitive(ctk_object->reset_button, False);

    return;


 fail:
    ctk_config_statusbar_message(ctk_object->ctk_config,
                                 "Failed to reset clock frequencies!");
    return;

} /* reset_clocks_clicked() */



/*****
 *
 * Signal handler - User clicked the 'auto detect/cancel' button.
 *
 */
static void detect_clocks_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    ReturnStatus ret;
    gint result;
    

    if ( ctk_object->probing_optimal ) {

        /* Stop the test for optimal clock freqs */
        ret = NvCtrlSetAttribute(ctk_object->handle,
                                 NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION,
                                 NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_CANCEL);
    } else {
        
        /* User must hit the OK button to start the testing */

        gtk_widget_show_all(ctk_object->detect_dialog);
        result = gtk_dialog_run (GTK_DIALOG (ctk_object->detect_dialog));
        gtk_widget_hide(ctk_object->detect_dialog);

        switch (result)
        {
        case GTK_RESPONSE_ACCEPT:

            /* Start the test for optimal clock freqs */
            ret = NvCtrlSetAttribute(ctk_object->handle,
                                     NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION,
                                     NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_START);
            break;

        case GTK_RESPONSE_REJECT:
        default:
            /* Do nothing. */
            return;
        }
    }

    return;

} /* detect_clocks_clicked() */



/*****
 *
 * Signal handler - Handles incoming NV-CONTROL messages caused by clocks
 * being changed.
 *
 */
static void clocks_received(GtkObject *object, gpointer arg1,
                            gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkClocks *ctk_object = CTK_CLOCKS(user_data);
    int clocks = event_struct->value;

    /* Some hardware requires that the memory clocks be the same for
     * both the 2D and 3D clock frequency setting.  Therefore, we
     * always need to check what the current clocks we're modifying
     * have been set to on clock events.
     */

    switch (event_struct->attribute) {
    case NV_CTRL_GPU_2D_CLOCK_FREQS:
        sync_gui_to_modify_clocks(ctk_object, CLOCKS_2D);

        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     "Set 2D clocks to (GPU) %i MHz, "
                                     "(Memory) %i MHz",
                                     GET_GPU_CLOCK(clocks),
                                     GET_MEM_CLOCK(clocks));
        break;
    case NV_CTRL_GPU_3D_CLOCK_FREQS:
        sync_gui_to_modify_clocks(ctk_object, CLOCKS_3D);

        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     "Set 3D clocks to (GPU) %i MHz, "
                                     "(Memory) %i MHz",
                                     GET_GPU_CLOCK(clocks),
                                     GET_MEM_CLOCK(clocks));
        break;
    case NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS:
        ctk_object->probing_optimal = False;

        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     "Found optimal 3D clocks: (GPU) %i MHz, "
                                     "(Memory) %i MHz",
                                     GET_GPU_CLOCK(clocks),
                                     GET_MEM_CLOCK(clocks));

        /* Only update gui if user is on 3D clocks */

        if ( ctk_object->clocks_being_modified == CLOCKS_3D ) {

            /* Update clock values */

            set_clocks_value(ctk_object, clocks, CLOCKS_3D);

            /* Allow user to apply the settings */

            if ( ctk_object->apply_button )
                gtk_widget_set_sensitive(ctk_object->apply_button, True);
        }

        break;
    default:
        break;
    }

} /* clocks_received() */



/*****
 *
 * Callback Function - This function gets called when the GPU Overclocking
 * page gets selected from the tree view.
 *
 */
void ctk_clocks_select(GtkWidget *widget)
{
    CtkClocks *ctk_object = CTK_CLOCKS(widget);
    ReturnStatus ret;
    int value;

    /* See if we're busy probing for optimal clocks so we can tell the user */

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE,
                             &value);

    if ( ret == NvCtrlSuccess &&
         value == NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE_BUSY ) {
        ctk_config_statusbar_message(ctk_object->ctk_config,
                                     __detect_wait_msg);
    }
}
