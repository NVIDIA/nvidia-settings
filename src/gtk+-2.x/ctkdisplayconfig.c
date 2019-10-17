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


#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <NvCtrlAttributes.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "msg.h"
#include "parse.h"
#include "lscf.h"

#include "nvvr.h"

#include "ctkutils.h"

#include "ctkbanner.h"
#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkdisplayconfig.h"
#include "ctkdisplaylayout.h"
#include "ctkdisplayconfig-utils.h"


void layout_selected_callback(nvLayoutPtr layout, void *data);
void layout_modified_callback(nvLayoutPtr layout, void *data);

static void setup_layout_frame(CtkDisplayConfig *ctk_object);

static void setup_selected_item_dropdown(CtkDisplayConfig *ctk_object);

static void selected_item_changed(GtkWidget *widget, gpointer user_data);

static void update_selected_page(CtkDisplayConfig *ctk_object);

static void setup_display_page(CtkDisplayConfig *ctk_object);

static void mosaic_state_toggled(GtkWidget *widget, gpointer user_data);

static void display_config_changed(GtkWidget *widget, gpointer user_data);
static void display_resolution_changed(GtkWidget *widget, gpointer user_data);
static void display_refresh_changed(GtkWidget *widget, gpointer user_data);

static void display_stereo_changed(GtkWidget *widget, gpointer user_data);
static void display_rotation_changed(GtkWidget *widget, gpointer user_data);
static void display_reflection_changed(GtkWidget *widget, gpointer user_data);

static void display_underscan_value_changed(GtkAdjustment *adjustment,
                                           gpointer user_data);
static void display_underscan_activate(GtkWidget *widget, gpointer user_data);

static void display_position_type_changed(GtkWidget *widget, gpointer user_data);
static void display_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void display_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void display_viewport_in_activate(GtkWidget *widget, gpointer user_data);
static void display_viewport_out_activate(GtkWidget *widget,
                                          gpointer user_data);
static void display_panning_activate(GtkWidget *widget, gpointer user_data);

static void update_force_gsync_button(CtkDisplayConfig *ctk_object);

static void setup_screen_page(CtkDisplayConfig *ctk_object);

static void screen_virtual_size_activate(GtkWidget *widget, gpointer user_data);
static gboolean txt_focus_out(GtkWidget *widget, GdkEvent *event,
                              gpointer user_data);

static void screen_depth_changed(GtkWidget *widget, gpointer user_data);

static void screen_stereo_changed(GtkWidget *widget, gpointer user_data);

static void screen_position_type_changed(GtkWidget *widget, gpointer user_data);
static void screen_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void screen_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void screen_metamode_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_activate(GtkWidget *widget, gpointer user_data);
static void screen_metamode_add_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_delete_clicked(GtkWidget *widget, gpointer user_data);

static void setup_prime_display_page(CtkDisplayConfig *ctk_object);

static void xinerama_state_toggled(GtkWidget *widget, gpointer user_data);
static void apply_clicked(GtkWidget *widget, gpointer user_data);
static void save_clicked(GtkWidget *widget, gpointer user_data);
static void probe_clicked(GtkWidget *widget, gpointer user_data);
static void advanced_clicked(GtkWidget *widget, gpointer user_data);
static void reset_clicked(GtkWidget *widget, gpointer user_data);
static void validation_details_clicked(GtkWidget *widget, gpointer user_data);

static void display_config_attribute_changed(GtkWidget *object,
                                             CtrlEvent *event,
                                             gpointer user_data);
static void reset_layout(CtkDisplayConfig *ctk_object);
static gboolean force_layout_reset(gpointer user_data);
static void user_changed_attributes(CtkDisplayConfig *ctk_object);
static void update_forcecompositionpipeline_buttons(CtkDisplayConfig
                                                    *ctk_object);

static XConfigPtr xconfig_generate(XConfigPtr xconfCur,
                                   Bool merge,
                                   Bool *merged,
                                   void *callback_data);




/*** D E F I N I T I O N S ***************************************************/


#define DEFAULT_SWITCH_MODE_TIMEOUT 15 /* When switching modes, this is the
                                        * number of seconds the user has to
                                        * accept the new mode before we switch
                                        * back to the original mode.
                                        */

#define TAB    "  "
#define BIGTAB "      "

#define GTK_RESPONSE_USER_DISPLAY_ENABLE_TWINVIEW 1
#define GTK_RESPONSE_USER_DISPLAY_ENABLE_XSCREEN  2

#define MIN_LAYOUT_SCREENSIZE 600
typedef struct SwitchModeCallbackInfoRec {
    CtkDisplayConfig *ctk_object;
    int screen;
} SwitchModeCallbackInfo;

/* Return values used by X config generation functions */
#define XCONFIG_GEN_OK    0
#define XCONFIG_GEN_ERROR 1
#define XCONFIG_GEN_ABORT 2

/* Validation types */
#define VALIDATE_APPLY 0
#define VALIDATE_SAVE  1

/* Underscan range of values */
#define UNDERSCAN_MIN_PERCENT 0
#define UNDERSCAN_MAX_PERCENT 35

/*** G L O B A L S ***********************************************************/

static int __position_table[] = { CONF_ADJ_ABSOLUTE,
                                  CONF_ADJ_RIGHTOF,
                                  CONF_ADJ_LEFTOF,
                                  CONF_ADJ_ABOVE,
                                  CONF_ADJ_BELOW,
                                  CONF_ADJ_RELATIVE };


/* Layout tooltips */

static const char * __layout_hidden_label_help =
"To select a display, use the \"Selection\" dropdown menu.";

static const char * __layout_xinerama_button_help =
"The Enable Xinerama checkbox enables the Xinerama X extension; changing "
"this option will require restarting your X server.  Note that when Xinerama "
"is enabled, resolution changes will also require restarting your X server.";

static const char * __selected_item_help =
"The Selection drop-down allows you to pick which X screen or display device "
"to configure.";

/* Display tooltips */

static const char * __dpy_configuration_mnu_help =
"The Configure drop-down allows you to select the desired configuration "
"for the currently selected display device.";

static const char * __layout_sli_mosaic_button_help =
"The Enable SLI Mosaic checkbox enables SLI Mosaic for all GPUs";

static const char * __layout_base_mosaic_surround_button_help =
"The Enable Base Mosaic (Surround) checkbox enables Surround, where up to 3 "
"displays are supported.";

static const char * __layout_base_mosaic_full_button_help =
"The Enable Base Mosaic checkbox enables Base Mosaic.";

static const char * __dpy_resolution_mnu_help =
"The Resolution drop-down allows you to select a desired resolution "
"for the currently selected display device.  The 'scaled' qualifier indicates "
"an aspect-scaled common resolution simulated through a MetaMode ViewPort "
"configuration.";

static const char * __dpy_refresh_mnu_help =
"The Refresh drop-down allows you to select a desired refresh rate "
"for the currently selected display device.  Note that the selected "
"resolution may restrict the available refresh rates.";

static const char * __dpy_stereo_help =
"The Display Passive Stereo Eye drop-down allows you to select a desired "
"stereo eye the display should output when Passive Stereo (Mode 4) is "
"enabled.";

static const char * __dpy_rotation_help =
"The Display Rotation drop-down allows you to select the desired orientation "
"for the display.";

static const char * __dpy_reflection_help =
"The Display Reflection drop-down allows you to choose the axes across which "
"monitor contents should be reflected.";

static const char * __dpy_viewport_in_help =
"This defines the width and height in pixels of the region that should be "
"displayed from the desktop.";

static const char * __dpy_viewport_out_help =
"This defines the width, height, and offset of the output region in raster "
"space, into which the ViewPortIn is to be displayed (along with any "
"transform, such as rotation, reflection, etc.)";

static const char * __dpy_position_type_help =
"The Position Type drop-down allows you to set how the selected display "
"device is placed within the X screen.  This is only available when "
"multiple display devices are present.";

static const char * __dpy_position_relative_help =
"The Position Relative drop-down allows you to set which other display "
"device (within the X screen) the selected display device should be "
"relative to.  This is only available when multiple display "
"devices are present.";

static const char * __dpy_underscan_text_help =
"The Underscan feature allows configuration of an underscan border "
"(in pixels) around the ViewPortOut.";

static const char * __dpy_position_offset_help =
"The Position Offset identifies the top left of the display device "
"as an offset from the top left of the X screen position.  This is only "
"available when multiple display devices are present.";

static const char * __dpy_panning_help =
"The Panning Domain sets the total width/height that the display "
"device may pan within.";

static const char * __dpy_primary_help =
"The primary display is often used by window managers to know which of the "
"displays in a multi-display setup to show information and other "
"important windows etc; changing this option may require restarting your X "
"server, depending on your window manager.";

static const char * __dpy_forcecompositionpipeline_help =
"The NVIDIA X driver can use a composition pipeline to apply X screen "
"transformations and rotations. \"ForceCompositionPipeline\" can be used to "
"force the use of this pipeline, even when no transformations or rotations are "
"applied to the screen. This option is implicitly set by "
"ForceFullCompositionPipeline.";

static const char * __dpy_forcefullcompositionpipeline_help =
"This option implicitly enables \"ForceCompositionPipeline\" and additionally "
"makes use of the composition pipeline to apply ViewPortOut scaling.";

static const char * __dpy_force_allow_gsync_help =
"This option allows enabling G-SYNC on displays that are not validated as "
"G-SYNC Compatible.";

/* Screen tooltips */

static const char * __screen_virtual_size_help =
"The Virtual Size allows setting the size of the resulting X screen.  "
"The virtual size must be at least large enough to hold all the display "
"devices that are currently enabled for scanout.";

static const char * __screen_depth_help =
"The Depth drop-down allows setting of the color quality for the selected "
"screen; changing this option will require restarting your X server.";

static const char * __screen_stereo_help =
"The Stereo Mode drop-down allows setting of the stereo mode for the selected "
"screen; changing this option will require restarting your X server.";

static const char * __screen_position_type_help =
"The Position Type drop-down appears when two or more X screens are active.  "
"This allows you to set how the selected screen "
"is placed within the X server layout; changing this option will require "
"restarting your X server.";

static const char * __screen_position_relative_help =
"The Position Relative drop-down appears when two or more X screens "
"are active.  This allows you to set which other Screen "
"the selected screen should be relative to; changing this option will "
"require restarting your X server.";

static const char * __screen_position_offset_help =
"The Position Offset drop-down appears when two or more X screens "
"are active.  This identifies the top left of the selected Screen as "
"an offset from the top left of the X server layout in absolute coordinates; "
"changing this option will require restarting your X server.";

static const char * __screen_metamode_help =
"The MetaMode selection menu allows you to set the currently displayed "
"MetaMode for the selected screen;  This option can be applied to "
"your currently running X server.";

static const char * __screen_metamode_add_button_help =
"The Add MetaMode button allows you to create a new MetaMode for the "
"selected screen;  This option can be applied to your currently "
"running X server.";

static const char * __screen_metamode_delete_button_help =
"The Delete MetaMode button allows you to delete the currently selected "
"MetaMode for the screen;  This option can be applied to your currently "
"running X server.";

/* Prime Display tooltips */

static const char *__prime_viewport_help =
"This shows the width, height, and offset in pixels of the region that "
"should be displayed from the desktop.";

static const char *__prime_name_help =
"This is the name of the display.";

static const char *__prime_sync_help =
"This shows the status of synchronization for the PRIME display. Without "
"synchronization, applications will not be able to sync to the display's "
"vblank.";

/* General button tooltips */

static const char * __apply_button_help =
"The Apply button allows you to apply changes made to the server layout.";

static const char * __detect_displays_button_help =
"The Detect Displays button allows you to probe for new display devices "
"that may have been hotplugged.";

static const char * __advanced_button_help =
"The Advanced/Basic button toggles between a basic view, and an advanced view "
"with extra configuration options.";

static const char * __reset_button_help =
"The Reset button will re-probe the X server for current configuration.  Any "
"alterations you may have made (and not applied) will be lost.";

static const char * __save_button_help =
"The Save to X Configuration File button allows you to save the current "
"X server configuration settings to an X configuration file.";




/*** F U N C T I O N S *******************************************************/


/** get_cur_screen_pos() *********************************************
 *
 * Grabs a copy of the currently selected screen position.
 *
 **/

static void get_cur_screen_pos(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

    ctk_object->cur_screen_pos.x = screen->dim.x;
    ctk_object->cur_screen_pos.y = screen->dim.y;

} /* get_cur_screen_pos() */



/** check_screen_pos_changed() ***************************************
 *
 * Checks to see if the screen's position changed.  If so this
 * function sets the apply_possible flag to FALSE.
 *
 **/

static void check_screen_pos_changed(CtkDisplayConfig *ctk_object)
{
    GdkPoint old_pos;

    /* Cache the old position */
    old_pos.x = ctk_object->cur_screen_pos.x;
    old_pos.y = ctk_object->cur_screen_pos.y;

    /* Get the new position */
    get_cur_screen_pos(ctk_object);

    if (old_pos.x != ctk_object->cur_screen_pos.x ||
        old_pos.y != ctk_object->cur_screen_pos.y) {
        ctk_object->apply_possible = FALSE;
    }

} /* check_screen_pos_changed() */



/** layout_supports_depth_30() ***************************************
 *
 * Returns TRUE if all the screens in the layout are driven by GPUs
 * that support depth 30.
 *
 **/

static gboolean layout_supports_depth_30(nvLayoutPtr layout)
{
    nvScreenPtr screen;

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        if (!screen->allow_depth_30) {
            return FALSE;
        }
    }
    return TRUE;

} /* layout_supports_depth_30() */



/** register_layout_events() *****************************************
 *
 * Registers to display-configuration related events relating to all
 * parts of the given layout structure.
 *
 **/

static void register_layout_events(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvGpuPtr gpu;


    /* Register for GPU events */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {

        if (gpu->ctrl_target == NULL) {
            continue;
        }

        g_signal_connect(G_OBJECT(gpu->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);

        g_signal_connect(G_OBJECT(gpu->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_MODE_SET_EVENT),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);
    }

    /* Register for X screen events */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        if (screen->ctrl_target == NULL) {
            continue;
        }

        g_signal_connect(G_OBJECT(screen->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);

        g_signal_connect(G_OBJECT(screen->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_STRING_MOVE_METAMODE),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);

        g_signal_connect(G_OBJECT(screen->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_STRING_DELETE_METAMODE),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);
    }

} /* register_layout_events() */



/** unregister_layout_events() *****************************************
 *
 * Unregisters display-configuration related events relating to all
 * parts of the given layout structure as registered by 

 * Unregisters all Screen/Gpu events.
 *
 **/

static void unregister_layout_events(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvGpuPtr gpu;


    /* Unregister GPU events */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {

        if (gpu->ctrl_target == NULL) {
            continue;
        }

        g_signal_handlers_disconnect_matched(G_OBJECT(gpu->ctk_event),
                                             G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                             0, // Signal ID
                                             0, // Signal Detail
                                             NULL, // Closure
                                             G_CALLBACK(display_config_attribute_changed),
                                             (gpointer) ctk_object);
    }

    /* Unregister X screen events */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        if (screen->ctrl_target == NULL) {
            continue;
        }

        g_signal_handlers_disconnect_matched(G_OBJECT(screen->ctk_event),
                                             G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                             0, // Signal ID
                                             0, // Signal Detail
                                             NULL, // Closure
                                             G_CALLBACK(display_config_attribute_changed),
                                             (gpointer) ctk_object);
    }

} /* unregister_layout_events() */



/** consolidate_xinerama() *******************************************
 *
 * Ensures that all X screens have the same depth if Xinerama is
 * enabled.
 *
 **/

static void consolidate_xinerama(CtkDisplayConfig *ctk_object,
                                 nvScreenPtr screen)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr other;

    if (!layout->xinerama_enabled) return;

    /* If no screen was given, pick one */
    if (!screen) {
        screen = layout_get_a_screen(layout, NULL);
    }
    if (!screen) return;

    /**
     * Make sure all screens support depth 30, and if not,
     * we should set depth 24.
     **/

    if ((screen->depth == 30) && (layout_supports_depth_30(layout) == FALSE)) {
        screen->depth = 24;
    }

    /* If Xinerama is enabled, all screens must have the same depth. */
    for (other = layout->screens; other; other = other->next_in_layout) {

        if (other == screen) continue;

        other->depth = screen->depth;
    }

} /* consolidate_xinerama() */



/** update_btn_apply() **************************************************
 *
 * Updates the apply button's sensitivity
 *
 **/

static void update_btn_apply(CtkDisplayConfig *ctk_object, Bool sensitive)
{
    /*
     * Both the apply and write quit warnings are sent whenever anything changes
     * in the display configuration so that the apply button is enabled. The
     * apply warning is cleared when the config is applied and the button reset
     * here. The save warning is cleared when the config is successfully saved.
     */
    if (sensitive) {
        ctk_object->ctk_config->pending_config |=
            CTK_CONFIG_PENDING_APPLY_DISPLAY_CONFIG;
        ctk_object->ctk_config->pending_config |=
            CTK_CONFIG_PENDING_WRITE_DISPLAY_CONFIG;
    } else {
        ctk_object->ctk_config->pending_config &=
            ~CTK_CONFIG_PENDING_APPLY_DISPLAY_CONFIG;
    }
    gtk_widget_set_sensitive(ctk_object->btn_apply, sensitive);

} /* update_btn_apply() */



/** xconfigPrint() ******************************************************
 *
 * xconfigPrint() - this is the one entry point that a user of the
 * XF86Config-Parser library must provide.
 *
 **/

void xconfigPrint(MsgType t, const char *msg)
{
    typedef struct {
        MsgType msg_type;
        char *prefix;
        FILE *stream;
        int newline;
    } MessageTypeAttributes;

    char *prefix = NULL;
    int i, newline = FALSE;
    FILE *stream = stdout;
    
    const MessageTypeAttributes msg_types[] = {
        { ParseErrorMsg,      "PARSE ERROR: ",      stderr, TRUE  },
        { ParseWarningMsg,    "PARSE WARNING: ",    stderr, TRUE  },
        { ValidationErrorMsg, "VALIDATION ERROR: ", stderr, TRUE  },
        { InternalErrorMsg,   "INTERNAL ERROR: ",   stderr, TRUE  },
        { WriteErrorMsg,      "ERROR: ",            stderr, TRUE  },
        { WarnMsg,            "WARNING: ",          stderr, TRUE  },
        { ErrorMsg,           "ERROR: ",            stderr, TRUE  },
        { DebugMsg,           "DEBUG: ",            stdout, FALSE },
        { UnknownMsg,          NULL,                stdout, FALSE },
    };
    
    for (i = 0; msg_types[i].msg_type != UnknownMsg; i++) {
        if (msg_types[i].msg_type == t) {
            prefix  = msg_types[i].prefix;
            newline = msg_types[i].newline;
            stream  = msg_types[i].stream;
            break;
        }
    }
    
    if (newline) fprintf(stream, "\n");
    fprintf(stream, "%s %s\n", prefix, msg);
    if (newline) fprintf(stream, "\n");
    
} /* xconfigPrint */



/** generate_xconf_metamode_str() ************************************
 *
 * Returns the metamode strings of a screen:
 *
 * "mode1_1, mode1_2, mode1_3 ... ; mode 2_1, mode 2_2, mode 2_3 ... ; ..."
 *
 **/

static int generate_xconf_metamode_str(CtkDisplayConfig *ctk_object,
                                       nvScreenPtr screen,
                                       gchar **pMetamode_strs)
{
    nvLayoutPtr layout = screen->layout;
    CtrlTarget *ctrl_target;
    gchar *metamode_strs = NULL;
    gchar *metamode_str;
    gchar *tmp;
    int metamode_idx;
    nvMetaModePtr metamode;
    int len = 0;
    int start_width;
    int start_height;

    int vendrel;
    char *vendstr;
    int xorg_major;
    int xorg_minor;
    Bool longStringsOK;

    ctrl_target = NvCtrlGetDefaultTarget(layout->system);
    if (ctrl_target == NULL) {
        return XCONFIG_GEN_ABORT;
    }

    vendrel = NvCtrlGetVendorRelease(ctrl_target);
    vendstr = NvCtrlGetServerVendor(ctrl_target);

    /* Only X.Org 7.2 or > supports long X config lines */
    xorg_major = (vendrel / 10000000);
    xorg_minor = (vendrel / 100000) % 100;

    if (g_strrstr(vendstr, "X.Org") &&
        ((xorg_major > 7) || ((xorg_major == 7) && (xorg_minor >= 2)))) {
        longStringsOK = TRUE;
    } else {
        longStringsOK = FALSE;
    }


    /* In basic view, always specify the currently selected
     * metamode first in the list so the X server starts
     * in this mode.
     */
    if (!ctk_object->advanced_mode) {
        metamode_strs = screen_get_metamode_str(screen,
                                                screen->cur_metamode_idx, 0);
        len = strlen(metamode_strs);
        start_width = screen->cur_metamode->edim.width;
        start_height = screen->cur_metamode->edim.height;
    } else {
        start_width = screen->metamodes->edim.width;
        start_height = screen->metamodes->edim.height;
    }

    for (metamode_idx = 0, metamode = screen->metamodes;
         (metamode_idx < screen->num_metamodes) && metamode;
         metamode_idx++, metamode = metamode->next) {

        int metamode_len;

        /* Only write out metamodes that were specified by the user */
        if (!IS_METAMODE_SOURCE_USER(metamode->source)) {
            continue;
        }

        /* The current mode was already included */
        if (!ctk_object->advanced_mode &&
            (metamode_idx == screen->cur_metamode_idx))
            continue;

        /* XXX In basic mode, only write out metamodes that are smaller than
         *     the starting (selected) metamode.  This is to work around
         *     a bug in XRandR where starting with a root window that is
         *     smaller that the bounding box of all the metamodes will result
         *     in an unwanted panning domain being setup for the first mode.
         */
        if ((!ctk_object->advanced_mode) &&
            ((metamode->edim.width > start_width) ||
             (metamode->edim.height > start_height)))
            continue;

        metamode_str = screen_get_metamode_str(screen, metamode_idx, 0);

        if (!metamode_str) continue;

        metamode_len = strlen(metamode_str);
        if (!longStringsOK && (len + metamode_len > 900)) {
            GtkWidget *dlg;
            gchar *msg;
            GtkWidget *parent;
            gint result;

            msg = g_strdup_printf
                ("Truncate the MetaMode list?\n"
                 "\n"
                 "Long MetaMode strings (greater than 900 characters) are not\n"
                 "supported by the current X server.  Truncating the MetaMode\n"
                 "list, so that the MetaMode string fits within 900 characters,\n"
                 "will cause only the first %d MetaModes to be written to the X\n"
                 "configuration file.\n"
                 "\n"
                 "NOTE: Writing all the MetaModes to the X Configuration\n"
                 "file may result in parse errors and failing to start the\n"
                 "X server.",
                 metamode_idx);
            
            parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));
            if (!parent) {
                nv_warning_msg("%s", msg);
                g_free(msg);
                break;
            }
            
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_NONE,
                 "%s", msg);
            
            gtk_dialog_add_buttons(GTK_DIALOG(dlg),
                                   "Truncate MetaModes",
                                   GTK_RESPONSE_YES,
                                   "Write all MetaModes", GTK_RESPONSE_NO,
                                   "Cancel", GTK_RESPONSE_CANCEL,
                                   NULL);
            
            result = gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
            g_free(msg);
            
            if (result == GTK_RESPONSE_YES) {
                break; /* Crop the list of metamodes */
            } else if (result == GTK_RESPONSE_NO) {
                longStringsOK = 1; /* Write the full list of metamodes */
            } else {
                return XCONFIG_GEN_ABORT; /* Don't save the X config file */
            }
        }

        if (!metamode_strs) {
            metamode_strs = metamode_str;
            len += metamode_len;
        } else {
            tmp = g_strconcat(metamode_strs, "; ", metamode_str, NULL);
            g_free(metamode_str);
            g_free(metamode_strs);
            metamode_strs = tmp;
            len += metamode_len +2;
        }
    }


    *pMetamode_strs = metamode_strs;

    return XCONFIG_GEN_OK;

} /* generate_xconf_metamode_str() */



/** assign_screen_positions() ****************************************
 *
 * Assign the initial position of the X screens.
 *
 * - If Xinerama is enabled or the X server ABI >= 12,
     query to the SCREEN_RECTANGLE returns position.
 *
 * - Otherwise assume "right-of" orientation.
 *
 **/

static void assign_screen_positions(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr prev_screen = NULL;
    nvScreenPtr screen;

    char *screen_info;
    GdkRectangle screen_parsed_info;
    ReturnStatus ret;


    /* Setup screen positions */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        screen_info = NULL;
        if (screen->ctrl_target != NULL) {
            ret = NvCtrlGetStringAttribute
                (screen->ctrl_target,
                 NV_CTRL_STRING_SCREEN_RECTANGLE,
                 &screen_info);

            if (ret != NvCtrlSuccess) {
                screen_info = NULL;
            }
        }

        if (screen_info) {

            /* Parse the positioning information */

            screen_parsed_info.x = -1;
            screen_parsed_info.y = -1;
            screen_parsed_info.width = -1;
            screen_parsed_info.height = -1;

            parse_token_value_pairs(screen_info, apply_screen_info_token,
                                    &screen_parsed_info);

            if (screen_parsed_info.x >= 0 &&
                screen_parsed_info.y >= 0 &&
                screen_parsed_info.width >= 0 &&
                screen_parsed_info.height) {

                ctk_display_layout_set_screen_position
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     screen, CONF_ADJ_ABSOLUTE, NULL,
                     screen_parsed_info.x,
                     screen_parsed_info.y);
            }
            free(screen_info);

        } else if (prev_screen) {
            /* Set this screen right of the previous */
            ctk_display_layout_set_screen_position
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 screen, CONF_ADJ_RIGHTOF, prev_screen, 0, 0);
        }

        prev_screen = screen;
    }

} /* assign_screen_positions() */



/* Widget creation functions *****************************************/


/** ctk_display_config_get_type() ************************************
 *
 * Returns the display configuration type.
 *
 **/

GType ctk_display_config_get_type(void)
{
    static GType ctk_display_config_type = 0;

    if (!ctk_display_config_type) {
        static const GTypeInfo ctk_display_config_info = {
            sizeof (CtkDisplayConfigClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkDisplayConfig),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_display_config_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkDisplayConfig", &ctk_display_config_info, 0);
    }

    return ctk_display_config_type;

} /* ctk_display_config_get_type() */



/** create_validation_dialog() ***************************************
 *
 * Creates the Validation Information dialog widget.
 *
 **/

static GtkWidget * create_validation_dialog(CtkDisplayConfig *ctk_object)
{
    GtkWidget *dialog;
    GtkWidget *image;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *scrolled_window;
    GtkWidget *textview;
    GtkTextBuffer *buffer;


    /* Display validation override confirmation dialog */
    dialog = gtk_dialog_new_with_buttons
        ("Layout Inconsistencie(s)",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    
    /* Main horizontal box */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox, TRUE, TRUE, 5);

    /* Pack the information icon */
    image = ctk_image_new_from_str(CTK_STOCK_DIALOG_INFO,
                                   GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment(GTK_MISC(image), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);
    
    /* Main vertical box */
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    
    /* Pack the main message */
    label = gtk_label_new("The current layout has some inconsistencies.");
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    
    /* Details button */
    button = gtk_button_new();
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);    
    g_signal_connect(G_OBJECT(button), "clicked",
                     G_CALLBACK(validation_details_clicked),
                     (gpointer) ctk_object);
    ctk_object->btn_validation_override_show = button;

    /* Text view */
    textview = gtk_text_view_new();
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), 5);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(textview), 5);
    
    buffer = gtk_text_buffer_new(NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(textview),
                             GTK_TEXT_BUFFER(buffer));
    
    ctk_object->buf_validation_override = buffer;

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type
        (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(scrolled_window), textview);

    /* Pack the scrolled window */
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    ctk_object->box_validation_override_details = hbox;
        
    /* Action Buttons */
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Auto Fix", GTK_RESPONSE_APPLY);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Ignore", GTK_RESPONSE_ACCEPT);
    /* Keep track of the cancel button so we can set focus on it */
    button = gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel",
                                   GTK_RESPONSE_REJECT);
    ctk_object->btn_validation_override_cancel = button;

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dialog)));

    return dialog;

} /* create_validation_dialog() */



/** create_validation_apply_dialog() *********************************
 *
 * Creates the Validation Apply Information dialog widget.
 *
 **/

static GtkWidget * create_validation_apply_dialog(CtkDisplayConfig *ctk_object)
{
    GtkWidget *dialog;
    GtkWidget *image;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *label;
    gchar bullet[8]; // UTF8 Bullet string
    int len;
    gchar *str;


    /* Convert the Unicode "Bullet" Character into a UTF8 string */
    len = g_unichar_to_utf8(0x2022, bullet);
    bullet[len] = '\0';

    /* Display validation override confirmation dialog */
    dialog = gtk_dialog_new_with_buttons
        ("Cannot Apply",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL |
         GTK_DIALOG_DESTROY_WITH_PARENT,
         NULL);
    ctk_object->dlg_validation_apply = dialog;

    /* Main horizontal box */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       hbox, TRUE, TRUE, 5);

    /* Pack the information icon */
    image = ctk_image_new_from_str(CTK_STOCK_DIALOG_INFO,
                                   GTK_ICON_SIZE_DIALOG);
    gtk_misc_set_alignment(GTK_MISC(image), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 5);

    /* Main vertical box */
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

    /* Pack the main message */
    str = g_strdup_printf("The current settings cannot be completely applied\n"
                          "due to one or more of the following reasons:\n"
                          "\n"
                          "%s The location of an X screen has changed.\n"
                          "%s The location type of an X screen has changed.\n"
                          "%s The color depth of an X screen has changed.\n"
                          "%s An X screen has been added or removed.\n"
                          "%s Xinerama is being enabled/disabled.\n"
                          "\n"
                          "For all the requested settings to take effect,\n"
                          "you must save the configuration to the X config\n"
                          "file and restart the X server.",
                          bullet, bullet, bullet, bullet, bullet);
    label = gtk_label_new(str);
    g_free(str);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    /* Action Buttons */
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Apply What Is Possible",
                          GTK_RESPONSE_ACCEPT);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Cancel", GTK_RESPONSE_REJECT);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dialog)));

    return dialog;

} /* create_validation_apply_dialog() */



/** user_changed_attributes() *************************************
 *
 * Turns off forced reset (of the layout config when the current
 * X server configuration changes).
 *
 **/

static void user_changed_attributes(CtkDisplayConfig *ctk_object)
{
    if (ctk_object->forced_reset_allowed) {
        update_btn_apply(ctk_object, TRUE);
        ctk_object->forced_reset_allowed = FALSE;
    }

} /* user_changed_attributes() */



/** display_forcecompositionpipeline_toggled() ************************
 *
 * Sets ForceCompositionPipeline for a dpy.
 *
 **/
static void display_forcecompositionpipeline_toggled(GtkWidget *widget,
                                                     gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (enabled) {
        display->cur_mode->forceCompositionPipeline = TRUE;
    } else {
        display->cur_mode->forceCompositionPipeline = FALSE;
    }

    update_forcecompositionpipeline_buttons(ctk_object);
    user_changed_attributes(ctk_object);
}



/** display_forcefullcompositionpipeline_toggled() ********************
 *
 * Sets ForceFullCompositionPipeline for a dpy.
 *
 **/
static void display_forcefullcompositionpipeline_toggled(GtkWidget *widget,
                                                         gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (enabled) {
        display->cur_mode->forceFullCompositionPipeline = TRUE;
        /* forceFullCompositionPipeline implies forceCompositionPipeline in the
         * X driver, so we should reflect that within nvidia-settings even
         * before actually changing the current X MetaMode.
         */
        display->cur_mode->forceCompositionPipeline = TRUE;
    } else {
        display->cur_mode->forceFullCompositionPipeline = FALSE;
    }

    update_forcecompositionpipeline_buttons(ctk_object);
    user_changed_attributes(ctk_object);
}



/** display_gsync_compatible_toggled() ********************************
 *
 * Sets AllowGSYNCCompatible for a dpy.
 *
 **/
static void display_gsync_compatible_toggled(GtkWidget *widget,
                                             gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (enabled) {
        display->cur_mode->allowGSYNCCompatibleSpecified = TRUE;
        display->cur_mode->allowGSYNCCompatible = TRUE;
    } else {
        display->cur_mode->allowGSYNCCompatibleSpecified = FALSE;
        display->cur_mode->allowGSYNCCompatible = FALSE;
    }

    update_force_gsync_button(ctk_object);
    user_changed_attributes(ctk_object);
}



/** update_forcecompositionpipeline_buttons() *************************
 *
 * Updates the buttons for Force{Full,}CompositionPipeline to reflect their
 * state and sensitivity.
 *
 **/

static void update_forcecompositionpipeline_buttons(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_forcecompositionpipeline_enabled),
        G_CALLBACK(display_forcecompositionpipeline_toggled),
        (gpointer)ctk_object);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_forcefullcompositionpipeline_enabled),
         G_CALLBACK(display_forcefullcompositionpipeline_toggled),
         (gpointer)ctk_object);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->chk_forcecompositionpipeline_enabled),
         display->cur_mode->forceCompositionPipeline |
            display->cur_mode->forceFullCompositionPipeline);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->chk_forcefullcompositionpipeline_enabled),
         display->cur_mode->forceFullCompositionPipeline);

    gtk_widget_set_sensitive
        (GTK_WIDGET(ctk_object->chk_forcecompositionpipeline_enabled),
         !display->cur_mode->forceFullCompositionPipeline);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_forcecompositionpipeline_enabled),
         G_CALLBACK(display_forcecompositionpipeline_toggled),
         (gpointer)ctk_object);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_forcefullcompositionpipeline_enabled),
         G_CALLBACK(display_forcefullcompositionpipeline_toggled),
         (gpointer)ctk_object);
}



/** update_force_gsync_button() ***************************************
 *
 * Updates the button for AllowGSYNCCompatible to reflect its current
 * state.
 *
 **/

static void update_force_gsync_button(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_force_allow_gsync),
         G_CALLBACK(display_gsync_compatible_toggled),
         (gpointer)ctk_object);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->chk_force_allow_gsync),
         display->cur_mode->allowGSYNCCompatibleSpecified &&
         display->cur_mode->allowGSYNCCompatible);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_force_allow_gsync),
         G_CALLBACK(display_gsync_compatible_toggled),
         (gpointer)ctk_object);
}



/** screen_primary_display_toggled() ******************************
 *
 * Sets the primary display for a screen.
 *
 **/

static void screen_primary_display_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvScreenPtr screen = display->screen;
    
    if (enabled) {
        screen->primaryDisplay = display;
        ctk_object->primary_display_changed = TRUE;
    }

    user_changed_attributes(ctk_object);

} /* screen_primary_display_toggled() */



/** screen_size_changed() *****************************************
 *
 * Hides layout widget.
 *
 **/

static void screen_size_changed(GdkScreen *screen,
                                gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint h = gdk_screen_get_height(screen);

    if ( h < MIN_LAYOUT_SCREENSIZE ) {
        gtk_widget_hide(ctk_object->obj_layout);
        gtk_widget_show(ctk_object->label_layout);
        return;
    }

    gtk_widget_hide(ctk_object->label_layout);
    gtk_widget_show_all(ctk_object->obj_layout);
} /* screen_size_changed() */



/** update_gui() *****************************************************
 *
 * Sync state of all widgets to reflect current configuration
 *
 **/

static void update_gui(CtkDisplayConfig *ctk_object)
{
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);
    setup_prime_display_page(ctk_object);
    setup_selected_item_dropdown(ctk_object);
    update_selected_page(ctk_object);
    setup_layout_frame(ctk_object);

} /* update_gui() */



/** ctk_display_config_new() *****************************************
 *
 * Display Configuration widget creation.
 *
 **/

GtkWidget* ctk_display_config_new(CtrlTarget *ctrl_target,
                                  CtkConfig *ctk_config)
{
    GObject *object;
    CtkDisplayConfig *ctk_object;

    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GdkScreen *screen;
    GtkWidget *label;
    GtkWidget *eventbox;
    GtkWidget *hseparator;
    GtkRequisition req;

    GSList *labels = NULL;
    GSList *slitem;
    gint max_width;

    gchar *err_str = NULL;
    gchar *layout_str = NULL;
    ReturnStatus ret;

    const char *stereo_mode_str;
    CtrlAttributeValidValues valid;
    int stereo_mode, stereo_mode_max = NV_CTRL_STEREO_MAX;

    /*
     * Create the ctk object
     *
     */

    object = g_object_new(CTK_TYPE_DISPLAY_CONFIG, NULL);
    ctk_object = CTK_DISPLAY_CONFIG(object);

    ctk_object->ctrl_target = ctrl_target;
    ctk_object->ctk_config = ctk_config;

    ctk_object->apply_possible = TRUE;

    ctk_object->reset_required = FALSE;
    ctk_object->forced_reset_allowed = TRUE;
    ctk_object->notify_user_of_reset = TRUE;
    ctk_object->ignore_reset_events = FALSE;
    ctk_object->primary_display_changed = FALSE;

    ctk_object->last_resolution_idx = -1;

    /* Set container properties of the object & pack the banner */
    gtk_box_set_spacing(GTK_BOX(ctk_object), 5);

    banner = ctk_banner_image_new(BANNER_ARTWORK_DISPLAY_CONFIG);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);



    /*
     * Create the display configuration widgets
     *
     */

    /* Load the layout structure from the X server */
    ctk_object->layout = layout_load_from_server(ctrl_target, &err_str);

    /* If we failed to load, tell the user why */
    if (err_str || !ctk_object->layout) {
        gchar *str;

        if (!err_str) {
            str = g_strdup("Unable to load X Server Display "
                           "Configuration page.");
        } else {
            str = g_strdup_printf("Unable to load X Server Display "
                                  "Configuration page:\n\n%s", err_str);
            g_free(err_str);
        }

        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_container_add(GTK_CONTAINER(object), label);
        
        /* Show the GUI */
        gtk_widget_show_all(GTK_WIDGET(ctk_object));

        return GTK_WIDGET(ctk_object);
    }

    /* Register all Screen/Gpu events. */
    register_layout_events(ctk_object);

    /* Create the layout widget */
    ctk_object->obj_layout = ctk_display_layout_new(ctk_config,
                                                    ctk_object->layout,
                                                    300, /* min width */
                                                    225); /* min height */

    /* Make sure all X screens have the same depth if Xinerama is enabled */
    consolidate_xinerama(ctk_object, NULL);

    /* Make sure we have some kind of positioning */
    assign_screen_positions(ctk_object);

    /* Grab the current screen position for "apply possible" tracking */
    get_cur_screen_pos(ctk_object);
    

    /*
     * Create the widgets
     *
     */

    /* Create label to replace layout widget */

    eventbox = gtk_event_box_new();
    layout_str = g_strdup_printf("(hidden because screen height is less than %d pixels)", MIN_LAYOUT_SCREENSIZE);
    ctk_object->label_layout = gtk_label_new(layout_str);
    gtk_container_add(GTK_CONTAINER(eventbox), ctk_object->label_layout);
    ctk_config_set_tooltip(ctk_config, eventbox, __layout_hidden_label_help);
    g_free(layout_str);
    screen = gtk_widget_get_screen(GTK_WIDGET(ctk_object));
    g_signal_connect(G_OBJECT(screen), "size-changed",
                     G_CALLBACK(screen_size_changed),
                     (gpointer) ctk_object);

    /* Mosaic button */
    ctk_object->chk_mosaic_enabled =
        gtk_check_button_new_with_label("");
    g_signal_connect(G_OBJECT(ctk_object->chk_mosaic_enabled), "toggled",
                     G_CALLBACK(mosaic_state_toggled),
                     (gpointer) ctk_object);

    /* Xinerama button */
    ctk_object->chk_xinerama_enabled =
        gtk_check_button_new_with_label("Enable Xinerama");
    ctk_config_set_tooltip(ctk_config, ctk_object->chk_xinerama_enabled,
                           __layout_xinerama_button_help);
    g_signal_connect(G_OBJECT(ctk_object->chk_xinerama_enabled), "toggled",
                     G_CALLBACK(xinerama_state_toggled),
                     (gpointer) ctk_object);

    /* Selected display/X screen dropdown */
    ctk_object->mnu_selected_item = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_selected_item,
                           __selected_item_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_selected_item), "changed",
                     G_CALLBACK(selected_item_changed),
                     (gpointer) ctk_object);

    /* Display configuration (Disabled, TwinView, Separate X screen) */
    ctk_object->mnu_display_config = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_config,
                           __dpy_configuration_mnu_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_config), "changed",
                     G_CALLBACK(display_config_changed),
                     (gpointer) ctk_object);

    /* Display disable dialog */
    ctk_object->txt_display_disable = gtk_label_new("");
    ctk_object->dlg_display_disable = gtk_dialog_new_with_buttons
        ("Disable Display Device",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, NULL);
    ctk_object->btn_display_disable_off =
        gtk_dialog_add_button(GTK_DIALOG(ctk_object->dlg_display_disable),
                              "Remove",
                              GTK_RESPONSE_ACCEPT);
    ctk_object->btn_display_disable_cancel =
        gtk_dialog_add_button(GTK_DIALOG(ctk_object->dlg_display_disable),
                              "Ignore",
                              GTK_RESPONSE_CANCEL);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_display_disable),
                             FALSE);


    /* Display resolution */
    ctk_object->mnu_display_resolution = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_resolution,
                           __dpy_resolution_mnu_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_resolution), "changed",
                     G_CALLBACK(display_resolution_changed),
                     (gpointer) ctk_object);


    /* Display refresh */
    ctk_object->mnu_display_refresh = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_refresh,
                            __dpy_refresh_mnu_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_refresh), "changed",
                     G_CALLBACK(display_refresh_changed),
                     (gpointer) ctk_object);

    /* Display modeline modename */
    ctk_object->txt_display_modename = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ctk_object->txt_display_modename), TRUE);

    /* Display passive stereo eye dropdown */
    ctk_object->mnu_display_stereo = ctk_combo_box_text_new();
    ctk_combo_box_text_append_text(ctk_object->mnu_display_stereo,
                                   "None");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_stereo,
                                   "Left");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_stereo,
                                   "Right");
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_stereo,
                           __dpy_stereo_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_stereo),
                     "changed", G_CALLBACK(display_stereo_changed),
                     (gpointer) ctk_object);

    /* Display rotation dropdown */
    ctk_object->mnu_display_rotation = ctk_combo_box_text_new();
    ctk_combo_box_text_append_text(ctk_object->mnu_display_rotation,
                                   "No Rotation");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_rotation,
                                   "Rotate Left");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_rotation,
                                   "Invert");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_rotation,
                                   "Rotate Right");
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_rotation,
                           __dpy_rotation_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_rotation),
                     "changed", G_CALLBACK(display_rotation_changed),
                     (gpointer) ctk_object);

    /* Display reflection dropdown */
    ctk_object->mnu_display_reflection = ctk_combo_box_text_new();
    ctk_combo_box_text_append_text(ctk_object->mnu_display_reflection,
                                   "No Reflection");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_reflection,
                                   "Reflect along X");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_reflection,
                                   "Reflect along Y");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_reflection,
                                   "Reflect along XY");
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_reflection,
                           __dpy_reflection_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_reflection),
                     "changed", G_CALLBACK(display_reflection_changed),
                     (gpointer) ctk_object);

    /* Display Underscan text box and slider */
    ctk_object->txt_display_underscan = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(ctk_object->txt_display_underscan), 6);
    gtk_entry_set_width_chars(GTK_ENTRY(ctk_object->txt_display_underscan), 6);
    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_underscan), "0");
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_underscan,
                           __dpy_underscan_text_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_underscan), "activate",
                              G_CALLBACK(display_underscan_activate),
                              (gpointer) ctk_object);

    ctk_object->adj_display_underscan =
        GTK_ADJUSTMENT(gtk_adjustment_new(0,
                                          UNDERSCAN_MIN_PERCENT,
                                          UNDERSCAN_MAX_PERCENT,
                                          1, 1, 0.0));
    ctk_object->sld_display_underscan =
        gtk_hscale_new(GTK_ADJUSTMENT(ctk_object->adj_display_underscan));
    gtk_scale_set_draw_value(GTK_SCALE(ctk_object->sld_display_underscan),
                             FALSE);
    ctk_config_set_tooltip(ctk_config, ctk_object->sld_display_underscan,
                           __dpy_underscan_text_help);
    g_signal_connect(G_OBJECT(ctk_object->adj_display_underscan), "value_changed",
                              G_CALLBACK(display_underscan_value_changed),
                              (gpointer) ctk_object);

    /* Display Position Type (Absolute/Relative Menu) */
    ctk_object->mnu_display_position_type = ctk_combo_box_text_new();
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Absolute");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Right of");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Left of");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Above");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Below");
    ctk_combo_box_text_append_text(ctk_object->mnu_display_position_type,
                                   "Same as");
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_position_type,
                           __dpy_position_type_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_position_type),
                     "changed", G_CALLBACK(display_position_type_changed),
                     (gpointer) ctk_object);

    /* Display Position Relative (Display device to be relative to) */
    ctk_object->mnu_display_position_relative = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config,
                           ctk_object->mnu_display_position_relative,
                           __dpy_position_relative_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_position_relative),
                     "changed",
                     G_CALLBACK(display_position_relative_changed),
                     (gpointer) ctk_object);

    /* Display Position Offset (Absolute position) */
    ctk_object->txt_display_position_offset = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_position_offset,
                           __dpy_position_offset_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_position_offset),
                     "activate", G_CALLBACK(display_position_offset_activate),
                     (gpointer) ctk_object);

    /* Display ViewPortIn */
    ctk_object->txt_display_viewport_in = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_viewport_in,
                           __dpy_viewport_in_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_viewport_in), "activate",
                     G_CALLBACK(display_viewport_in_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_viewport_in), "focus-out-event",
                     G_CALLBACK(txt_focus_out),
                     (gpointer) ctk_object);

    /* Display ViewPortOut */
    ctk_object->txt_display_viewport_out = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_viewport_out,
                           __dpy_viewport_out_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_viewport_out), "activate",
                     G_CALLBACK(display_viewport_out_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_viewport_out), "focus-out-event",
                     G_CALLBACK(txt_focus_out),
                     (gpointer) ctk_object);

    /* Display Panning */
    ctk_object->txt_display_panning = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_panning,
                           __dpy_panning_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_panning), "activate",
                     G_CALLBACK(display_panning_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_panning), "focus-out-event",
                     G_CALLBACK(txt_focus_out),
                     (gpointer) ctk_object);

    /* X screen virtual size */
    ctk_object->txt_screen_virtual_size = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_screen_virtual_size,
                           __screen_virtual_size_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_screen_virtual_size), "activate",
                     G_CALLBACK(screen_virtual_size_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_screen_virtual_size),
                     "focus-out-event",
                     G_CALLBACK(txt_focus_out),
                     (gpointer) ctk_object);

    /* X screen depth */
    ctk_object->mnu_screen_depth = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_screen_depth,
                           __screen_depth_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_screen_depth), "changed",
                     G_CALLBACK(screen_depth_changed),
                     (gpointer) ctk_object);

    /* Screen Stereo Mode */
    ret = NvCtrlGetValidAttributeValues(ctrl_target, NV_CTRL_STEREO, &valid);

    if (ret == NvCtrlSuccess) {

        ctk_object->mnu_screen_stereo = ctk_combo_box_text_new();

        ctk_object->stereo_table_size = 0;
        memset(ctk_object->stereo_table, 0,
               sizeof(int) * ARRAY_LEN(ctk_object->stereo_table));

        /*
         * The current driver will return type _INT_BITS that we can use to
         * list the available stereo modes. Older drivers will return the type
         * _INTEGER that we can use as a flag to list all possible stereo modes
         * before the change was made. The newest at that time was HDMI_3D.
         */

        if (valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_INTEGER) {
            stereo_mode_max = NV_CTRL_STEREO_HDMI_3D;
        }

        for (stereo_mode = NV_CTRL_STEREO_OFF;
             stereo_mode <= stereo_mode_max;
             stereo_mode++) {
            stereo_mode_str = NvCtrlGetStereoModeNameIfExists(stereo_mode);

            if (stereo_mode_str == (const char *) -1) {
                break;
            }

            if (stereo_mode_str &&
                ((valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_INTEGER) ||
                 (((valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS) &&
                   (valid.allowed_ints & (1 << stereo_mode)))))) {

                ctk_object->stereo_table[ctk_object->stereo_table_size++] =
                    stereo_mode;
                ctk_combo_box_text_append_text(
                    ctk_object->mnu_screen_stereo,
                    stereo_mode_str);
            }
        }

        ctk_config_set_tooltip(ctk_config, ctk_object->mnu_screen_stereo,
                               __screen_stereo_help);
        g_signal_connect(G_OBJECT(ctk_object->mnu_screen_stereo),
                         "changed", G_CALLBACK(screen_stereo_changed),
                         (gpointer) ctk_object);
    } else {
        ctk_object->mnu_screen_stereo = NULL;
    }

    /* Screen Position Type (Absolute/Relative Menu) */
    ctk_object->mnu_screen_position_type = ctk_combo_box_text_new();
    ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
                                   "Absolute");
    ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
                                   "Right of");
    ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
                                   "Left of");
    ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
                                   "Above");
    ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
                                   "Below");
    // XXX Add better support for this later.
    //ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_type,
    //                               "Relative to");
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_screen_position_type,
                           __screen_position_type_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_screen_position_type),
                     "changed", G_CALLBACK(screen_position_type_changed),
                     (gpointer) ctk_object);

    /* Screen Position Relative (Screen to be relative to) */
    ctk_object->mnu_screen_position_relative = ctk_combo_box_text_new();
    ctk_config_set_tooltip(ctk_config,
                           ctk_object->mnu_screen_position_relative,
                           __screen_position_relative_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_screen_position_relative),
                     "changed",
                     G_CALLBACK(screen_position_relative_changed),
                     (gpointer) ctk_object);

    /* Screen Position Offset (Absolute position) */
    ctk_object->txt_screen_position_offset = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_screen_position_offset,
                           __screen_position_offset_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_screen_position_offset),
                     "activate", G_CALLBACK(screen_position_offset_activate),
                     (gpointer) ctk_object);

    /* X screen metamode */
    ctk_object->btn_screen_metamode = gtk_button_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_screen_metamode,
                           __screen_metamode_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_screen_metamode), "clicked",
                     G_CALLBACK(screen_metamode_clicked),
                     (gpointer) ctk_object);

    ctk_object->btn_screen_metamode_add = gtk_button_new_with_label("Add");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_screen_metamode_add,
                           __screen_metamode_add_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_screen_metamode_add), "clicked",
                     G_CALLBACK(screen_metamode_add_clicked),
                     (gpointer) ctk_object);

    ctk_object->btn_screen_metamode_delete =
        gtk_button_new_with_label("Delete");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_screen_metamode_delete,
                           __screen_metamode_delete_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_screen_metamode_delete),
                     "clicked",
                     G_CALLBACK(screen_metamode_delete_clicked),
                     (gpointer) ctk_object);

    
    /* Create the Validation dialog */
    ctk_object->dlg_validation_override = create_validation_dialog(ctk_object);


    /* Create the Apply Validation dialog */
    ctk_object->dlg_validation_apply =
        create_validation_apply_dialog(ctk_object);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_validation_apply),
                             FALSE);


    /* Reset confirmation dialog */
    ctk_object->dlg_reset_confirm = gtk_dialog_new_with_buttons
        ("Confirm Reset",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_OK,
         GTK_RESPONSE_ACCEPT,
         NULL);
    ctk_object->btn_reset_cancel =
        gtk_dialog_add_button(GTK_DIALOG(ctk_object->dlg_reset_confirm),
                              GTK_STOCK_CANCEL,
                              GTK_RESPONSE_REJECT);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_reset_confirm),
                             FALSE);


    /* Display ModeSwitch confirmation dialog */
    ctk_object->dlg_display_confirm = gtk_dialog_new_with_buttons
        ("Confirm ModeSwitch",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_OK,
         GTK_RESPONSE_ACCEPT,
         NULL);
    ctk_object->btn_display_apply_cancel =
        gtk_dialog_add_button(GTK_DIALOG(ctk_object->dlg_display_confirm),
                              GTK_STOCK_CANCEL,
                              GTK_RESPONSE_REJECT);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_display_confirm),
                             FALSE);


    /* Display confirm dialog text (Dynamically generated) */
    ctk_object->txt_display_confirm = gtk_label_new("");


    /* X config save dialog */
    ctk_object->save_xconfig_dlg =
        create_save_xconfig_dialog(GTK_WIDGET(ctk_object),
                                   TRUE, // Merge toggleable
                                   xconfig_generate,
                                   (void *)ctk_object);


    /* Apply button */
    ctk_object->btn_apply = gtk_button_new_with_label("Apply");
    update_btn_apply(ctk_object, FALSE);
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_apply,
                           __apply_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_apply), "clicked",
                     G_CALLBACK(apply_clicked),
                     (gpointer) ctk_object);


    /* Probe button */
    ctk_object->btn_probe = gtk_button_new_with_label("Detect Displays");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_probe,
                           __detect_displays_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_probe), "clicked",
                     G_CALLBACK(probe_clicked),
                     (gpointer) ctk_object);


    /* Advanced button */
    ctk_object->btn_advanced = gtk_button_new_with_label("Advanced...");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_advanced,
                           __advanced_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_advanced), "clicked",
                     G_CALLBACK(advanced_clicked),
                     (gpointer) ctk_object);


    /* Reset button */
    ctk_object->btn_reset = gtk_button_new_with_label("Reset");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_reset,
                           __reset_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_reset), "clicked",
                     G_CALLBACK(reset_clicked),
                     (gpointer) ctk_object);


    /* Save button */
    ctk_object->btn_save = gtk_button_new_with_label
        ("Save to X Configuration File");
    ctk_config_set_tooltip(ctk_config, ctk_object->btn_save,
                           __save_button_help);
    g_signal_connect(G_OBJECT(ctk_object->btn_save), "clicked",
                     G_CALLBACK(save_clicked),
                     (gpointer) ctk_object);

    { /* Layout section */

        frame = gtk_frame_new("Layout"); /* main panel */
        gtk_box_pack_start(GTK_BOX(ctk_object), frame, FALSE, FALSE, 0);
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* Pack the layout widget */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->obj_layout,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), eventbox, TRUE, TRUE, 0);

        /* Mosaic checkbox */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->chk_mosaic_enabled,
                           FALSE, FALSE, 0);

        /* Xinerama checkbox */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->chk_xinerama_enabled,
                           FALSE, FALSE, 0);
    }


    /* Selection */
    label = gtk_label_new("Selection:");
    labels = g_slist_append(labels, label);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_selected_item,
                       TRUE, TRUE, 0);



    { /* Display page */
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(ctk_object), vbox, FALSE, FALSE, 0);
        ctk_object->display_page = vbox;

        /* Info on how to drag X screens around */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("");
        labels = g_slist_append(labels, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);

        label = gtk_label_new("(CTRL-Click + Drag to move X screens)");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        ctk_object->box_screen_drag_info_display = hbox;

        /* Display Configuration */
        label = gtk_label_new("Configuration:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_config,
                           TRUE, TRUE, 0);
        ctk_object->box_display_config = hbox;

        /* Display resolution and refresh dropdowns */
        label = gtk_label_new("Resolution:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_resolution,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_refresh,
                           TRUE, TRUE, 0);
        ctk_object->box_display_resolution = hbox;

        /* Modeline modename info */
        label = gtk_label_new("Mode Name:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_modename,
                           FALSE, FALSE, 0);
        ctk_object->box_display_modename = hbox;

        /* Display passive stereo eye dropdown */
        label = gtk_label_new("Stereo Eye:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_stereo,
                           TRUE, TRUE, 0);
        ctk_object->box_display_stereo = hbox;

        /* Display rotation & reflection dropdowns */
        {
            GtkWidget *hbox2 = gtk_hbox_new(TRUE, 5);

            label = gtk_label_new("Orientation:");
            labels = g_slist_append(labels, label);

            hbox = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
            gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_rotation,
                               TRUE, TRUE, 0);

            hbox = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_end(GTK_BOX(hbox2), hbox, FALSE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_reflection,
                               TRUE, TRUE, 0);

            gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 0);
            ctk_object->box_display_orientation = hbox2;
        }

        /* Display underscan */
        {
            GtkWidget *hbox2 = gtk_hbox_new(TRUE, 0);
            label = gtk_label_new("Underscan:");
            labels = g_slist_append(labels, label);

            hbox = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
            gtk_box_pack_start(GTK_BOX(hbox),
                               ctk_object->txt_display_underscan,
                               FALSE, FALSE, 0);

            gtk_box_pack_start(GTK_BOX(hbox),
                               ctk_object->sld_display_underscan,
                               TRUE, TRUE, 3);

            gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 0);
            ctk_object->box_display_underscan = hbox2;
        }

        /* Display positioning */
        label = gtk_label_new("Position:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->mnu_display_position_type,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->mnu_display_position_relative,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->txt_display_position_offset,
                           TRUE, TRUE, 0);
        ctk_object->box_display_position = hbox;

        /* Display ViewPortIn */
        label = gtk_label_new("ViewPortIn:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->txt_display_viewport_in,
                           TRUE, TRUE, 0);
        ctk_object->box_display_viewport_in = hbox;

        /* Display ViewPortOut */
        label = gtk_label_new("ViewPortOut:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->txt_display_viewport_out,
                           TRUE, TRUE, 0);
        ctk_object->box_display_viewport_out = hbox;

        /* Display panning text entry */
        label = gtk_label_new("Panning:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_panning,
                           TRUE, TRUE, 0);
        ctk_object->box_display_panning = hbox;

        /* checkbox for primary display of X screen */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        ctk_object->chk_primary_display =
            gtk_check_button_new_with_label("Make this the primary display "
                                            "for the X screen");
        ctk_config_set_tooltip(ctk_config, ctk_object->chk_primary_display,
                               __dpy_primary_help);
        g_signal_connect(G_OBJECT(ctk_object->chk_primary_display), "toggled",
                         G_CALLBACK(screen_primary_display_toggled),
                         (gpointer) ctk_object);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->chk_primary_display,
                           TRUE, TRUE, 0);

        /* checkboxes for Force{Full,}CompositionPipeline */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        ctk_object->chk_forcecompositionpipeline_enabled =
            gtk_check_button_new_with_label("Force Composition Pipeline");
        ctk_config_set_tooltip(ctk_config,
                               ctk_object->chk_forcecompositionpipeline_enabled,
                               __dpy_forcecompositionpipeline_help);
        g_signal_connect(G_OBJECT(ctk_object->chk_forcecompositionpipeline_enabled),
                         "toggled",
                         G_CALLBACK(display_forcecompositionpipeline_toggled),
                         (gpointer)ctk_object);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->chk_forcecompositionpipeline_enabled,
                           TRUE, TRUE, 0);

        ctk_object->chk_forcefullcompositionpipeline_enabled =
            gtk_check_button_new_with_label("Force Full Composition Pipeline");
        ctk_config_set_tooltip(ctk_config,
                               ctk_object->
                                chk_forcefullcompositionpipeline_enabled,
                               __dpy_forcefullcompositionpipeline_help);
        g_signal_connect(G_OBJECT(ctk_object->chk_forcefullcompositionpipeline_enabled),
                         "toggled",
                         G_CALLBACK(display_forcefullcompositionpipeline_toggled),
                         (gpointer)ctk_object);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->chk_forcefullcompositionpipeline_enabled,
                           TRUE, TRUE, 0);

        /* checkbox for AllowGSYNCCompatible */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        ctk_object->chk_force_allow_gsync =
            gtk_check_button_new_with_label(
                "Allow G-SYNC on monitor not validated as G-SYNC Compatible");
        ctk_config_set_tooltip(ctk_config, ctk_object->chk_force_allow_gsync,
                               __dpy_force_allow_gsync_help);
        g_signal_connect(G_OBJECT(ctk_object->chk_force_allow_gsync),
                         "toggled",
                         G_CALLBACK(display_gsync_compatible_toggled),
                         (gpointer) ctk_object);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->chk_force_allow_gsync,
                           TRUE, TRUE, 0);

        /* Up the object ref count to make sure that the page and its widgets
         * do not get freed if/when the page is removed from the notebook.
         */
        g_object_ref(ctk_object->display_page);
        gtk_widget_show_all(ctk_object->display_page);

    } /* Display sub-section */


    { /* X screen page */
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(ctk_object), vbox, FALSE, FALSE, 0);
        ctk_object->screen_page = vbox;

        /* Info on how to drag X screens around */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("");
        labels = g_slist_append(labels, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);

        label = gtk_label_new("(CTRL-Click + Drag to move X screens)");
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        ctk_object->box_screen_drag_info_screen = hbox;

        /* X screen virtual size */
        label = gtk_label_new("Virtual Size:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5); 
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_screen_virtual_size,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_virtual_size = hbox;

        /* X screen depth dropdown */
        label = gtk_label_new("Color Depth:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5); 
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_screen_depth,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_depth = hbox;

        /* X screen stereo dropdown */
        if (ctk_object->mnu_screen_stereo) {
            label = gtk_label_new("Stereo Mode:");
            labels = g_slist_append(labels, label);

            hbox = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
            gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_screen_stereo,
                               TRUE, TRUE, 0);
            ctk_object->box_screen_stereo = hbox;
        } else {
            ctk_object->box_screen_stereo = NULL;
        }

        /* X screen positioning */
        label = gtk_label_new("Position:");
        labels = g_slist_append(labels, label);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->mnu_screen_position_type,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->mnu_screen_position_relative,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->txt_screen_position_offset,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_position = hbox;

        /* X screen metamode drop down & buttons */
        label = gtk_label_new("MetaMode:");
        labels = g_slist_append(labels, label);

        hbox  = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->btn_screen_metamode,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->btn_screen_metamode_add,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->btn_screen_metamode_delete,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_metamode = hbox;

        /* Up the object ref count to make sure that the page and its widgets
         * do not get freed if/when the page is removed from the notebook.
         */
        g_object_ref(ctk_object->screen_page);
        gtk_widget_show_all(ctk_object->screen_page);

    } /* X screen sub-section */

    { /* Prime Display page */
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(ctk_object), vbox, FALSE, FALSE, 0);
        ctk_object->prime_display_page = vbox;

        /* Disclaimer about inability to control non-nvidia displays */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("PRIME Displays cannot be controlled by "
            "nvidia-settings and must be configured by an external "
            "RandR capable tool. The display is shown in the layout "
            "window above for informational purposes only.");
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("Viewport:");
        labels = g_slist_append(labels, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

        ctk_object->lbl_prime_display_view = gtk_label_new("");
        ctk_config_set_tooltip(ctk_config, ctk_object->lbl_prime_display_view,
                               __prime_viewport_help);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->lbl_prime_display_view,
                           FALSE, FALSE, 5);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("Name:");
        labels = g_slist_append(labels, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
        ctk_object->lbl_prime_display_name = gtk_label_new("");
        ctk_config_set_tooltip(ctk_config, ctk_object->lbl_prime_display_name,
                               __prime_name_help);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->lbl_prime_display_name,
                           FALSE, FALSE, 5);
        ctk_object->box_prime_display_name = hbox;

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

        label = gtk_label_new("Synchronization:");
        labels = g_slist_append(labels, label);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
        ctk_object->lbl_prime_display_sync = gtk_label_new("");
        ctk_config_set_tooltip(ctk_config, ctk_object->lbl_prime_display_sync,
                               __prime_sync_help);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->lbl_prime_display_sync,
                           FALSE, FALSE, 5);

        g_object_ref(ctk_object->prime_display_page);
        gtk_widget_show_all(ctk_object->prime_display_page);
    }


    /* Align all the configuration labels */
    max_width = 0;
    for (slitem = labels; slitem; slitem = slitem->next) {
        label = slitem->data;
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        ctk_widget_get_preferred_size(label, &req);
        if (req.width > max_width) {
            max_width = req.width;
        }
    }
    for (slitem = labels; slitem; slitem = slitem->next) {
        label = slitem->data;
        gtk_widget_set_size_request(label, max_width, -1);
    }
    g_slist_free(labels);


    { /* Buttons */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_end(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 0);

        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_save,
                         FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_end(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 0);

        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_reset,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_advanced,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_probe,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_apply,
                         FALSE, FALSE, 0);

        hseparator = gtk_hseparator_new();
        gtk_box_pack_end(GTK_BOX(ctk_object), hseparator, FALSE, TRUE, 5);
    }


    { /* Dialogs */

        /* Display Disable Dialog */
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_disable,
                           FALSE, FALSE, 20);
        gtk_box_pack_start
            (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_display_disable))),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_display_disable)));

        /* Reset Confirm Dialog */
        label = gtk_label_new("Do you really want to reset the "
                              "configuration?");
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);
        gtk_box_pack_start
            (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_reset_confirm))),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_reset_confirm)));

        /* Apply Confirm Dialog */
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_confirm,
                           TRUE, TRUE, 20);
        gtk_box_pack_start
            (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_display_confirm))),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(ctk_object->dlg_display_confirm)));
    }


    /* If mosaic mode is enabled, start in advanced mode */
    if (ctk_object->layout &&
        ctk_object->layout->gpus &&
        ctk_object->layout->gpus->mosaic_enabled) {
        advanced_clicked(ctk_object->btn_advanced, ctk_object);
    }


    /* Show the GUI */
    gtk_widget_show_all(GTK_WIDGET(ctk_object));

    update_gui(ctk_object);

    /* Register to receive updates when layout changed */
    ctk_display_layout_register_callbacks(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                          layout_selected_callback,
                                          (void *)ctk_object,
                                          layout_modified_callback,
                                          (void *)ctk_object);

    return GTK_WIDGET(ctk_object);

} /* ctk_display_config_new() */



/** ctk_display_config_create_help() *********************************
 *
 * Creates the Display Configuration help page.
 *
 **/

GtkTextBuffer *ctk_display_config_create_help(GtkTextTagTable *table,
                                              CtkDisplayConfig *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    nvGpuPtr gpu = NULL;

    if (ctk_object->layout) {
        gpu = ctk_object->layout->gpus;
    }

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Display Configuration Help");
    ctk_help_para(b, &i, "This page gives access to configuration of "
                  "the X server's display devices.");

    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Layout Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the X server layout.");
    ctk_help_heading(b, &i, "Layout Image");
    ctk_help_para(b, &i, "The layout image shows the geometric relationship "
                  "that display devices and X screens have to each other.  "
                  "You may drag display devices around to reposition them.  "
                  "When in advanced view, the display's panning domain may "
                  "be resized by holding SHIFT while dragging.  Also, The "
                  "X screen a display belongs to may be selected by holding "
                  "down the CONTROL key while clicking on the display, and can "
                  "be moved by holding CONTROL-Click and dragging.");
    ctk_help_heading(b, &i, "Layout Hidden Label");
    ctk_help_para(b, &i, "%s", __layout_hidden_label_help);

    if (gpu) {
        switch (gpu->mosaic_type) {
        case MOSAIC_TYPE_SLI_MOSAIC:
            ctk_help_heading(b, &i, "Enable SLI Mosaic");
            ctk_help_para(b, &i, "%s", __layout_sli_mosaic_button_help);
            break;
        case MOSAIC_TYPE_BASE_MOSAIC:
            ctk_help_heading(b, &i, "Enable Base Mosaic");
            ctk_help_para(b, &i, "%s", __layout_base_mosaic_full_button_help);
            break;
        case MOSAIC_TYPE_BASE_MOSAIC_LIMITED:
            ctk_help_heading(b, &i, "Enable Base Mosaic (Surround)");
            ctk_help_para(b, &i, "%s", __layout_base_mosaic_surround_button_help);
            break;
        default:
            break;
        }
    }

    ctk_help_heading(b, &i, "Enable Xinerama");
    ctk_help_para(b, &i, "%s  This setting is only available when multiple "
                  "X screens are present.", __layout_xinerama_button_help);
    ctk_help_heading(b, &i, "Selection");
    ctk_help_para(b, &i, "%s", __selected_item_help);

    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Display Options");
    ctk_help_para(b, &i, "The following options are available when a display "
                  "device is selected in the Selection drop-down to configure "
                  "the settings for that display device.");
    ctk_help_heading(b, &i, "Configuration");
    ctk_help_para(b, &i, "%s  \"Disabled\" disables the selected display "
                  "device. \"X screen <number>\" associates the selected "
                  "display device with the specified X Screen. \"New X screen "
                  "(requires X restart)\" creates a new X Screen and "
                  "associates the selected display device with it.",
                  __dpy_configuration_mnu_help);
    ctk_help_heading(b, &i, "Resolution");
    ctk_help_para(b, &i, "%s", __dpy_resolution_mnu_help);
    ctk_help_heading(b, &i, "Refresh");
    ctk_help_para(b, &i, "The Refresh drop-down is to the right of the "
                  "Resolution drop-down.  %s", __dpy_refresh_mnu_help);
    ctk_help_heading(b, &i, "Mode Name");
    ctk_help_para(b, &i, "The Mode name is the name of the modeline that is "
                  "currently chosen for the selected display device.  "
                  "This is only available when advanced view is enabled.");
    ctk_help_heading(b, &i, "Stereo Eye");
    ctk_help_para(b, &i, "%s", __dpy_stereo_help);
    ctk_help_heading(b, &i, "Orientation");
    ctk_help_para(b, &i, "The Orientation drop-downs control how the desktop "
                  "image is rotated and/or reflected.  %s  %s  Note that "
                  "reflection is applied before rotation.",
                  __dpy_rotation_help, __dpy_reflection_help);
    ctk_help_heading(b, &i, "Underscan");
    ctk_help_para(b, &i, "%s  The aspect ratio of the ViewPortOut is preserved "
                  " and the ViewPortIn is updated to exactly match this new "
                  "size.  This feature is formerly known as Overscan "
                  "Compensation.", __dpy_underscan_text_help);
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, "%s", __dpy_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, "%s", __dpy_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, "%s", __dpy_position_offset_help);
    ctk_help_heading(b, &i, "ViewPortIn");
    ctk_help_para(b, &i, "%s", __dpy_viewport_in_help);
    ctk_help_heading(b, &i, "ViewPortOut");
    ctk_help_para(b, &i, "%s", __dpy_viewport_out_help);
    ctk_help_heading(b, &i, "Panning");
    ctk_help_para(b, &i, "%s  This is only available when advanced "
                  "view is enabled.", __dpy_panning_help);
    ctk_help_heading(b, &i, "Primary Display");
    ctk_help_para(b, &i, "%s", __dpy_primary_help);
    ctk_help_heading(b, &i, "Force Composition Pipeline");
    ctk_help_para(b, &i, "%s", __dpy_forcecompositionpipeline_help);
    ctk_help_heading(b, &i, "Force Full Composition Pipeline");
    ctk_help_para(b, &i, "%s", __dpy_forcefullcompositionpipeline_help);
    ctk_help_heading(b, &i, "Allow G-SYNC on monitor not validated as G-SYNC Compatible");
    ctk_help_para(b, &i, "%s", __dpy_force_allow_gsync_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "X Screen Options");
    ctk_help_para(b, &i, "The following options are available when an X "
                  "screen is selected in the Selection drop-down to configure "
                  "the settings for that X screen.");
    ctk_help_heading(b, &i, "Virtual Size");
    ctk_help_para(b, &i, "%s  The Virtual screen size must be at least "
                  "304x200, and the width must be a multiple of 8.",
                  __screen_virtual_size_help);
    ctk_help_heading(b, &i, "Color Depth");
    ctk_help_para(b, &i, "%s", __screen_depth_help);
    ctk_help_heading(b, &i, "Stereo Mode");
    ctk_help_para(b, &i, "%s", __screen_stereo_help);
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, "%s", __screen_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, "%s", __screen_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, "%s", __screen_position_offset_help);
    ctk_help_heading(b, &i, "MetaMode Selection");
    ctk_help_para(b, &i, "%s  This is only available when advanced view "
                  "is enabled.", __screen_metamode_help);
    ctk_help_heading(b, &i, "Add Metamode");
    ctk_help_para(b, &i, "%s  This is only available when advanced view "
                  "is enabled.", __screen_metamode_add_button_help);
    ctk_help_heading(b, &i, "Delete Metamode");
    ctk_help_para(b, &i, "%s This is only available when advanced view "
                  "is enabled.", __screen_metamode_delete_button_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "PRIME Display Options");
    ctk_help_para(b, &i, "The following attributes are available when a "
                  "configured PRIME display is selected in the Selection "
                  "drop-down. These attributes cannot be changed within "
                  "nvidia-settings.");
    ctk_help_heading(b, &i, "Viewport");
    ctk_help_para(b, &i, "%s", __prime_viewport_help);
    ctk_help_heading(b, &i, "Name");
    ctk_help_para(b, &i, "%s  This attribute may not be available.",
                  __prime_name_help);
    ctk_help_heading(b, &i, "Synchronization");
    ctk_help_para(b, &i, "%s", __prime_sync_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Buttons");
    ctk_help_heading(b, &i, "Apply");
    ctk_help_para(b, &i, "%s  Note that not all settings can be applied to an "
                  "active X server; "
                  "these require restarting the X server after saving the "
                  "desired settings to the X configuration file.  Examples "
                  "of such settings include changing the position of any X "
                  "screen, adding/removing an X screen, and changing the X "
                  "screen color depth.", __apply_button_help);
    ctk_help_heading(b, &i, "Detect Displays");
    ctk_help_para(b, &i, "%s", __detect_displays_button_help);
    ctk_help_heading(b, &i, "Advanced/Basic...");
    ctk_help_para(b, &i, "%s  The Basic view modifies the currently active "
                  "MetaMode for an X screen, while the advanced view exposes "
                  "all the MetaModes available on an X screen, and lets you "
                  "modify each of them.", __advanced_button_help);
    ctk_help_heading(b, &i, "Reset");
    ctk_help_para(b, &i, "%s", __reset_button_help);
    ctk_help_heading(b, &i, "Save to X Configuration File");
    ctk_help_para(b, &i, "%s", __save_button_help);

    ctk_help_finish(b);

    return b;

} /* ctk_display_config_create_help() */



/* Widget setup & helper functions ***********************************/


static void setup_mosaic_config(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvGpuPtr gpu;
    const char *tooltip;
    const gchar *label;
    gboolean display_gpu_support_mosaic = (display && display->gpu &&
        display->gpu->mosaic_type != MOSAIC_TYPE_UNSUPPORTED);
    gboolean screen_gpu_support_mosaic = (screen && screen->display_owner_gpu &&
        screen->display_owner_gpu->mosaic_type != MOSAIC_TYPE_UNSUPPORTED);


    if (!ctk_object->advanced_mode ||
        (!display_gpu_support_mosaic && !screen_gpu_support_mosaic)) {
        gtk_widget_hide(ctk_object->chk_mosaic_enabled);
        return;
    }

    gtk_widget_show(ctk_object->chk_mosaic_enabled);

    if (display_gpu_support_mosaic) {
        gpu = display->gpu;
    } else {
        gpu = screen->display_owner_gpu;
    }

    switch (gpu->mosaic_type) {
    case MOSAIC_TYPE_SLI_MOSAIC:
        tooltip = __layout_sli_mosaic_button_help;
        label = "Enable SLI Mosaic";
        break;
    case MOSAIC_TYPE_BASE_MOSAIC:
        tooltip = __layout_base_mosaic_full_button_help;
        label = "Enable Base Mosaic";
        break;
    case MOSAIC_TYPE_BASE_MOSAIC_LIMITED:
        tooltip = __layout_base_mosaic_surround_button_help;
        label = "Enable Base Mosaic (Surround)";
        break;
    default:
        gtk_widget_hide(ctk_object->chk_mosaic_enabled);
        return;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_mosaic_enabled),
         G_CALLBACK(mosaic_state_toggled), (gpointer) ctk_object);

    gtk_button_set_label(GTK_BUTTON(ctk_object->chk_mosaic_enabled),
                         label);

    ctk_config_set_tooltip(ctk_object->ctk_config,
                           ctk_object->chk_mosaic_enabled,
                           tooltip);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->chk_mosaic_enabled),
         gpu->mosaic_enabled);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_mosaic_enabled),
         G_CALLBACK(mosaic_state_toggled), (gpointer) ctk_object);
}



/** setup_layout_frame() *********************************************
 *
 * Sets up the layout frame to reflect the currently selected layout.
 *
 **/

static void setup_layout_frame(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    GdkScreen *s;

    /*
     * Hide/Shows the layout widget based on the current screen size.
     * If the screen is too small, the layout widget is hidden and a
     * message is shown instead. 
     */
    s = gtk_widget_get_screen(GTK_WIDGET(ctk_object));
    screen_size_changed(s, ctk_object);

    setup_mosaic_config(ctk_object);

    /* Xinerama requires 2 or more X screens */
    if (layout->num_screens < 2) {
        layout->xinerama_enabled = 0;
        gtk_widget_hide(ctk_object->chk_xinerama_enabled);
        return;
    }
    gtk_widget_show(ctk_object->chk_xinerama_enabled);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_xinerama_enabled),
         G_CALLBACK(xinerama_state_toggled), (gpointer) ctk_object);

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_object->chk_xinerama_enabled),
         layout->xinerama_enabled);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_xinerama_enabled),
         G_CALLBACK(xinerama_state_toggled), (gpointer) ctk_object);

} /* setup_layout_frame() */



/** update_selected_page() ***********************************************
 *
 * Makes sure the correct page (Display or X Screen) is selected based on
 * the currently selected items.
 *
 **/

static void update_selected_page(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvPrimeDisplayPtr prime = ctk_display_layout_get_selected_prime_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    gtk_widget_hide(ctk_object->display_page);
    gtk_widget_hide(ctk_object->screen_page);
    gtk_widget_hide(ctk_object->prime_display_page);
    if (display) {
        gtk_widget_show(ctk_object->display_page);
    } else if (prime) {
        gtk_widget_show(ctk_object->prime_display_page);
    } else if (screen) {
        gtk_widget_show(ctk_object->screen_page);
    }

} /* update_selected_page() */



/** generate_selected_item_dropdown() ************************************
 *
 * Drop down menu for selecting current display/X screen.
 *
 **/

static void generate_selected_item_dropdown(CtkDisplayConfig *ctk_object,
                                            nvDisplayPtr cur_display,
                                            nvScreenPtr cur_screen,
                                            nvPrimeDisplayPtr cur_prime,
                                            int *cur_idx)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvDisplayPtr display;
    nvScreenPtr screen;
    nvPrimeDisplayPtr prime;
    int idx;
    char *str;
    char *tmp;
    gboolean show_gpu_info;

    /* (Re)allocate the lookup table */

    if (ctk_object->selected_item_table) {
        free(ctk_object->selected_item_table);
    }

    ctk_object->selected_item_table_len = layout->num_screens;
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        ctk_object->selected_item_table_len += gpu->num_displays;
    }

    ctk_object->selected_item_table_len += layout->num_prime_displays;

    ctk_object->selected_item_table =
        calloc(ctk_object->selected_item_table_len,
               sizeof(SelectableItem));

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
        GTK_COMBO_BOX(ctk_object->mnu_selected_item))));

    if (!ctk_object->selected_item_table) {
        ctk_object->selected_item_table_len = 0;
        gtk_widget_set_sensitive(ctk_object->mnu_selected_item, False);
        return;
    }

    /* Create the dropdown menu and fill the lookup table */

    idx = 0;
    show_gpu_info = ((layout->num_gpus > 1) || ctk_object->advanced_mode) ? True : False;

    /* Add X screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        if (!cur_display && (cur_screen == screen)) {
            *cur_idx = idx;
        }

        str = g_strdup_printf("X screen %d", screen->scrnum);

        ctk_combo_box_text_append_text(ctk_object->mnu_selected_item, str);
        g_free(str);

        ctk_object->selected_item_table[idx].type = SELECTABLE_ITEM_SCREEN;
        ctk_object->selected_item_table[idx].u.screen = screen;
        idx++;
    }

    /* Add displays */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (cur_display == display) {
                *cur_idx = idx;
            }
            str = g_strdup_printf("%s (%s", display->logName,
                                  display->randrName);
            if (show_gpu_info) {
                tmp = str;
                str = g_strdup_printf("%s on GPU-%d", tmp,
                                      NvCtrlGetTargetId(gpu->ctrl_target));
                g_free(tmp);
            }
            tmp = str;
            str = g_strdup_printf("%s)", tmp);
            g_free(tmp);

            ctk_combo_box_text_append_text(ctk_object->mnu_selected_item, str);
            g_free(str);

            ctk_object->selected_item_table[idx].type = SELECTABLE_ITEM_DISPLAY;
            ctk_object->selected_item_table[idx].u.display = display;
            idx++;
        }
    }

    /* Add prime displays */
    for (prime = layout->prime_displays; prime; prime = prime->next_in_layout) {

        if (cur_prime == prime) {
            *cur_idx = idx;
        }

        if (prime->label) {
            str = g_strdup_printf("PRIME Display: %s", prime->label);
        } else {
            str = g_strdup("PRIME Display");
        }

        ctk_combo_box_text_append_text(ctk_object->mnu_selected_item, str);
        g_free(str);

        ctk_object->selected_item_table[idx].type = SELECTABLE_ITEM_PRIME;
        ctk_object->selected_item_table[idx].u.prime = prime;

        idx++;
    }

} /* generate_selected_item_dropdown() */



/** setup_selected_item_dropdown() ******************************
 *
 * Setup display modelname dropdown menu.
 *
 **/

static void setup_selected_item_dropdown(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvPrimeDisplayPtr prime = ctk_display_layout_get_selected_prime_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int cur_idx = 0;

    if (!display && !screen && !prime) {
        gtk_widget_set_sensitive(ctk_object->mnu_selected_item, False);
        gtk_widget_hide(ctk_object->mnu_selected_item);
        return;
    }

    gtk_widget_set_sensitive(ctk_object->mnu_selected_item, True);
    gtk_widget_show(ctk_object->mnu_selected_item);

    /* Setup the menu and select the current model */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_selected_item),
         G_CALLBACK(selected_item_changed), (gpointer) ctk_object);

    generate_selected_item_dropdown(ctk_object,
                                    display, screen, prime,
                                    &cur_idx);

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_selected_item), cur_idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_selected_item),
         G_CALLBACK(selected_item_changed), (gpointer) ctk_object);

} /* setup_selected_item_dropdown() */



/** setup_display_modename() *****************************************
 *
 * Updates the modeline modename of the selected display
 *
 **/

static void setup_display_modename(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display || !display->screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_display_modename);
        return;
    }
    gtk_widget_show(ctk_object->box_display_modename);


    if (!display->cur_mode || !display->cur_mode->modeline) {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_modename), "");
        gtk_widget_set_sensitive(ctk_object->box_display_modename, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_modename, TRUE);


    gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_modename),
                  display->cur_mode->modeline->data.identifier);

} /* setup_display_modename() */



/** setup_display_config() *******************************************
 *
 * Updates the "Configure" dropdown menu to list the currently
 * available configurations of the selected display.
 *
 **/

static void setup_display_config(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen = NULL;
    int num_screens_on_gpu = 0;

    DisplayConfigOption *options;
    int max_options;
    int cur_option = -1;
    int num_options = 0;


    /* Allocate the max space we'll need for the lookup table (list all the
     * X screens + disable + new)
     */
    max_options = layout->num_screens + 2;
    options = nvalloc(max_options * sizeof(DisplayConfigOption));

    /* Don't allow disabling the last display device */
    if (layout->num_screens > 1 ||
        !display->screen ||
        display->screen->num_displays >= 1) {
        if (!display->screen) {
            cur_option = num_options;
        }
        options[num_options].config = DPY_CFG_DISABLED;
        num_options++;
    }

    /* Include the possible X screen(s) that this display can be part of */
    for (screen = layout->screens;
         screen;
         screen = screen->next_in_layout) {
        if (screen_has_gpu(screen, display->gpu) ||
            display->gpu->mosaic_enabled) {
            int max_displays = get_screen_max_displays(screen);


            num_screens_on_gpu++;
            if (display->screen == screen) {
                cur_option = num_options;
            } else if (max_displays >= 0 &&
                       screen->num_displays > max_displays) {
                /* Skip screens that are full */
                continue;
            }
            options[num_options].config = DPY_CFG_X_SCREEN;
            options[num_options].screen = screen;
            num_options++;
        }
    }

    /* Only allow creation of a new X screen if Mosaic mode is disabled, the GPU
     * can support another X screen, and the display is not already the only
     * display in the X screen.
     */

    if (!display->gpu->mosaic_enabled &&
	(num_screens_on_gpu < display->gpu->max_displays) &&
        (!display->screen || (display->screen->num_displays > 1))) {
        options[num_options].config = DPY_CFG_NEW_X_SCREEN;
        num_options++;
    }


    /* Apply the new options */
    nvfree(ctk_object->display_config_table);
    ctk_object->display_config_table = options;
    ctk_object->display_config_table_len = num_options;

    {
        int i;
        gchar *label;

        g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_display_config),
                                        G_CALLBACK(display_config_changed),
                                        (gpointer) ctk_object);

        gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
            (GTK_COMBO_BOX(ctk_object->mnu_display_config))));

        for (i = 0; i < num_options; i++) {

            switch (options[i].config) {
            case DPY_CFG_DISABLED:
                ctk_combo_box_text_append_text(ctk_object->mnu_display_config,
                                               "Disabled");
                break;
            case DPY_CFG_NEW_X_SCREEN:
                ctk_combo_box_text_append_text(
                    ctk_object->mnu_display_config,
                    "New X screen (requires X restart)");
                break;
            case DPY_CFG_X_SCREEN:
                label = g_strdup_printf("X screen %d",
                                        options[i].screen->scrnum);
                ctk_combo_box_text_append_text(ctk_object->mnu_display_config,
                                               label);
                g_free(label);
                break;
            }
        }

        gtk_combo_box_set_active(GTK_COMBO_BOX(ctk_object->mnu_display_config),
                                 cur_option);
        gtk_widget_set_sensitive(ctk_object->mnu_display_config, TRUE);

        g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->mnu_display_config),
                                          G_CALLBACK(display_config_changed),
                                          (gpointer) ctk_object);
    }

} /* setup_display_config() */



/** setup_display_refresh_dropdown() *********************************
 *
 * Generates the refresh rate dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_refresh_dropdown(CtkDisplayConfig *ctk_object)
{
    GtkWidget *combo_box = ctk_object->mnu_display_refresh;
    nvModeLinePtr modeline;
    nvModeLinePtr auto_modeline;
    nvModeLinePtr modelines;
    nvModeLinePtr cur_modeline;
    float cur_rate; /* Refresh Rate */
    int cur_idx = 0; /* Currently selected modeline */

    gchar *name; /* Modeline's label for the dropdown menu */

    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));



    /* Get selection information */
    if (!display ||
        !display->cur_mode ||
        !display->cur_mode->modeline) {
        goto fail;
    }
    modelines    = display->modelines;
    cur_modeline = display->cur_mode->modeline;
    cur_rate     = cur_modeline->refresh_rate;


    /* Create the menu index -> modeline pointer lookup table */
    if (ctk_object->refresh_table) {
        free(ctk_object->refresh_table);
        ctk_object->refresh_table_len = 0;
    }
    ctk_object->refresh_table =
        calloc(display->num_modelines, sizeof(nvModeLinePtr));
    if (!ctk_object->refresh_table) {
        goto fail;
    }


    /* Generate the refresh dropdown */
    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_display_refresh),
                                    G_CALLBACK(display_refresh_changed),
                                    (gpointer) ctk_object);

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
        (GTK_COMBO_BOX(ctk_object->mnu_display_refresh))));

    /* Special case the 'nvidia-auto-select' mode. */
    if (IS_NVIDIA_DEFAULT_MODE(cur_modeline)) {
        ctk_combo_box_text_append_text(combo_box, "Auto");
        ctk_object->refresh_table[ctk_object->refresh_table_len++] =
            cur_modeline;
        modelines = NULL; /* Skip building rest of refresh dropdown */
    }

    /* Generate the refresh rate dropdown from the modelines list */
    auto_modeline = NULL;
    for (modeline = modelines; modeline; modeline = modeline->next) {

        nvModeLinePtr m;
        int count_ref; /* # modelines with similar refresh rates */ 
        int num_ref;   /* Modeline # in a group of similar refresh rates */

        /* Ignore modelines of different resolution */
        if (modeline->data.hdisplay != cur_modeline->data.hdisplay ||
            modeline->data.vdisplay != cur_modeline->data.vdisplay) {
            continue;
        }

        /* Ignore special modes */
        if (IS_NVIDIA_DEFAULT_MODE(modeline)) {
            continue;
        }

        name = g_strdup_printf("%0.*f Hz", (display->is_sdi ? 3 : 0),
                               modeline->refresh_rate);

        /* Get a unique number for this modeline */
        count_ref = 0; /* # modelines with similar refresh rates */
        num_ref = 0;   /* Modeline # in a group of similar refresh rates */
        for (m = modelines; m; m = m->next) {
            float m_rate = m->refresh_rate;
            gchar *tmp = g_strdup_printf("%.0f Hz", m_rate);
            
            if (!IS_NVIDIA_DEFAULT_MODE(m) &&
                m->data.hdisplay == modeline->data.hdisplay &&
                m->data.vdisplay == modeline->data.vdisplay &&
                !g_ascii_strcasecmp(tmp, name) &&
                m != auto_modeline) {

                count_ref++;
                /* Modelines with similar refresh rates get a
                 * unique # (num_ref)
                 */
                if (m == modeline) {
                    num_ref = count_ref; /* This modeline's # */
                }
            }
            g_free(tmp);
        }

        /* Is default refresh rate for resolution */
        if (!ctk_object->refresh_table_len && !display->is_sdi) {
            auto_modeline = modeline;
            g_free(name);
            name = g_strdup("Auto");

        /* In advanced mode, all modelines are selectable */
        } else if (count_ref > 1 && ctk_object->advanced_mode) {
            gchar *tmp;
            tmp = g_strdup_printf("%s (%d)", name, num_ref);
            g_free(name);
            name = tmp;

        /* in simple mode only show one refresh rate */
        } else if (num_ref > 1 && !ctk_object->advanced_mode) {
            continue;
        }


        /* Add "DoubleScan", "Interlace", and "HDMI 3D" information */
        if (g_ascii_strcasecmp(name, "Auto")) {
            gchar *extra = NULL;
            gchar *tmp;
            ReturnStatus ret;
            gboolean hdmi3D = FALSE;

            if (modeline->data.flags & V_DBLSCAN) {
                extra = g_strdup_printf("DoubleScan");
            }

            if (modeline->data.flags & V_INTERLACE) {
                if (extra) {
                    tmp = g_strdup_printf("%s, Interlace", extra);
                    g_free(extra);
                    extra = tmp;
                } else {
                    extra = g_strdup_printf("Interlace");
                }
            }

            ret = NvCtrlGetAttribute(display->ctrl_target,
                                     NV_CTRL_DPY_HDMI_3D,
                                     &hdmi3D);
            if (ret == NvCtrlSuccess && hdmi3D) {
                if (extra) {
                    tmp = g_strdup_printf("%s, HDMI 3D", extra);
                    g_free(extra);
                    extra = tmp;
                } else {
                    extra = g_strdup_printf("HDMI 3D");
                }
            }


            if (extra) {
                tmp = g_strdup_printf("%s (%s)", name, extra);
                g_free(extra);
                g_free(name);
                name = tmp;
            }
        }


        /* Keep track of the selected modeline */
        if (cur_modeline == modeline) {
            cur_idx = ctk_object->refresh_table_len;
            
        /* Find a close match  to the selected modeline */
        } else if (ctk_object->refresh_table_len &&
                   ctk_object->refresh_table[cur_idx] != cur_modeline) {
            
            /* Found a better resolution */
            if (modeline->data.hdisplay == cur_modeline->data.hdisplay &&
                modeline->data.vdisplay == cur_modeline->data.vdisplay) {
                
                float prev_rate = ctk_object->refresh_table[cur_idx]->refresh_rate;
                float rate = modeline->refresh_rate;
                
                if (ctk_object->refresh_table[cur_idx]->data.hdisplay != cur_modeline->data.hdisplay ||
                    ctk_object->refresh_table[cur_idx]->data.vdisplay != cur_modeline->data.vdisplay) {
                    cur_idx = ctk_object->refresh_table_len;
                }
                
                /* Found a better refresh rate */
                if (rate == cur_rate && prev_rate != cur_rate) {
                    cur_idx = ctk_object->refresh_table_len;
                }
            }
        }

        
        /* Add the modeline entry to the dropdown */
        ctk_combo_box_text_append_text(combo_box, name);
        g_free(name);
        ctk_object->refresh_table[ctk_object->refresh_table_len++] = modeline;
    }

    


    /* Select the current modeline */
    gtk_combo_box_set_active(GTK_COMBO_BOX(ctk_object->mnu_display_refresh),
                             cur_idx);
    gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, True);

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->mnu_display_refresh),
                                      G_CALLBACK(display_refresh_changed),
                                      (gpointer) ctk_object);


    /* If dropdown only has one item, disable it */
    if (ctk_object->refresh_table_len > 1) {
        gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, True);
    } else {
        gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, False);
    }


    /* Update the modename label */
    setup_display_modename(ctk_object);
    return;


    /* Handle failures */
 fail:
    gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, False);

    setup_display_modename(ctk_object);

} /* setup_display_refresh_dropdown() */



/** get_default_modeline() *******************************************
 *
 * Finds the default modeline in the list of modelines.
 *
 * Returns the default modeline if found, NULL otherwise.
 *
 */

static nvModeLinePtr get_default_modeline(const nvDisplayPtr display)
{
    nvModeLinePtr modeline = display->modelines;

    while (modeline) {
        if (IS_NVIDIA_DEFAULT_MODE(modeline)) {
            return modeline;
        }

        modeline = modeline->next;
    }

    return NULL;
}



/** allocate_selected_mode() *****************************************
 *
 * Allocates, fills and returns a nvSelectedModePtr.
 *
 */

static nvSelectedModePtr
allocate_selected_mode(char *name,
                       nvModeLinePtr modeline,
                       Bool isSpecial,
                       NVVRSize *viewPortIn,
                       NVVRBoxRecXYWH *viewPortOut)
{
    nvSelectedModePtr selected_mode;

    selected_mode = (nvSelectedModePtr)nvalloc(sizeof(nvSelectedMode));

    selected_mode->text = g_strdup(name);

    selected_mode->modeline = modeline;
    selected_mode->isSpecial = isSpecial;
    selected_mode->isScaled = (viewPortIn || viewPortOut);

    if (viewPortIn) {
        selected_mode->viewPortIn.width = viewPortIn->w;
        selected_mode->viewPortIn.height = viewPortIn->h;
    }

    if (viewPortOut) {
        selected_mode->viewPortOut.x = viewPortOut->x;
        selected_mode->viewPortOut.y = viewPortOut->y;
        selected_mode->viewPortOut.width = viewPortOut->w;
        selected_mode->viewPortOut.height = viewPortOut->h;
    }

    return selected_mode;
}



/** free_selected_modes() ********************************************
 *
 * Recursively frees each item of a list of selected modes.
 *
 */

static void
free_selected_modes(nvSelectedModePtr selected_mode)
{
    if (selected_mode) {
        free_selected_modes(selected_mode->next);
        g_free(selected_mode->text);
        free(selected_mode);
    }
}



/** append_unique_selected_mode() ************************************
 *
 * Appends a selected mode to the given list only if it doesn't already exist.
 * Special modes ("Auto", "Off") are not checked.  Two selected modes are unique
 * if their [hv]display differ in the case of regular modes, or if the
 * ViewPortIn of the given mode doesn't match any existing [hv]display.
 * Returns TRUE if the selected mode has been added, FALSE otherwise.
 *
 */

static Bool
append_unique_selected_mode(nvSelectedModePtr head,
                            const nvSelectedModePtr mode)
{
    int targetWidth, targetHeight;
    nvSelectedModePtr iter, prev = NULL;

    if (mode->isScaled) {
        targetWidth = mode->viewPortIn.width;
        targetHeight = mode->viewPortIn.height;
    } else {
        targetWidth = mode->modeline->data.hdisplay;
        targetHeight = mode->modeline->data.vdisplay;
    }

    /* Keep the list sorted by targeted resolution */
    iter = head;
    while (iter) {
        int currentWidth, currentHeight;
        nvModeLinePtr ml = iter->modeline;

        if (!ml || iter->isSpecial) {
            goto next;
        }

        if (iter->isScaled) {
            currentWidth = iter->viewPortIn.width;
            currentHeight = iter->viewPortIn.height;
        } else {
            currentWidth = ml->data.hdisplay;
            currentHeight = ml->data.vdisplay;
        }

        /* If we are past the sort order, stop looping */
        if ((targetWidth > currentWidth) ||
            ((targetWidth == currentWidth) && (targetHeight > currentHeight))) {
            break;
        }

        if (ml && !mode->isSpecial &&
            (targetWidth == currentWidth) && (targetHeight == currentHeight)) {
            return FALSE;
        }

next:
        prev = iter;
        iter = iter->next;
    }

    if (prev == NULL) {
        return FALSE;
    }

    /* Insert the selected mode */
    mode->next = prev->next;
    prev->next = mode;

    return TRUE;
}



/** matches_current_selected_mode() **********************************
 *
 * Checks whether the provided selected mode matches the current mode.
 *
 * We need to distinguish between custom modes and scaled modes.
 *
 * Custom modes are modes with custom ViewPort settings, such as an
 * Underscan configuration.  These modes don't have an entry in the
 * resolution dropdown menu.  Instead, the corresponding modeline must be
 * selected.
 *
 * Scaled modes are generated by the CPL, have a fixed ViewPort{In,Out}
 * configuration and are displayed in the dropdown menu in basic mode.
 *
 * Therefore, we compare the raster size and the ViewPorts first, then only
 * the raster size.  This works because the list of selected_modes is
 * generated before the scaled ones.  The latter can then overwrite the
 * cur_selected_mode if we find a better match.
 *
 * Returns TRUE if the provided selected mode matches the current mode, FALSE
 * otherwise.
 *
 */

static Bool matches_current_selected_mode(const nvDisplayPtr display,
                                          const nvSelectedModePtr selected_mode,
                                          const Bool compare_viewports)
{
    nvModeLinePtr ml1, ml2;
    nvModePtr cur_mode;
    Bool mode_match;

    if (!display || !display->cur_mode || !selected_mode) {
        return FALSE;
    }

    cur_mode = display->cur_mode;
    ml1 = cur_mode->modeline;
    ml2 = selected_mode->modeline;

    if (!ml1 || !ml2) {
        return FALSE;
    }

    mode_match = ((ml1->data.hdisplay == ml2->data.hdisplay) &&
                  (ml1->data.vdisplay == ml2->data.vdisplay));

    if (compare_viewports) {
        nvSize rotatedViewPortIn;

        memcpy(&rotatedViewPortIn, &selected_mode->viewPortIn, sizeof(nvSize));

        if (cur_mode->rotation == ROTATION_90 ||
            cur_mode->rotation == ROTATION_270) {
            int temp = rotatedViewPortIn.width;
            rotatedViewPortIn.width = rotatedViewPortIn.height;
            rotatedViewPortIn.height = temp;
        }

        return (mode_match &&
                viewports_in_match(cur_mode->viewPortIn,
                                   rotatedViewPortIn) &&
                viewports_out_match(cur_mode->viewPortOut,
                                    selected_mode->viewPortOut));
    } else {
        return (!IS_NVIDIA_DEFAULT_MODE(ml1) && mode_match);
    }
}



/** generate_selected_modes() ****************************************
 *
 * Generates a list of selected modes.  The list is generated by parsing
 * modelines.  This function makes sure that each item of the list is unique
 * and sorted.
 *
 */

static void generate_selected_modes(const nvDisplayPtr display)
{
    nvSelectedModePtr selected_mode = NULL;
    nvModeLinePtr modeline;

    display->num_selected_modes = 0;
    display->selected_modes = NULL;

    /* Add the off item if we have more than one display */
    if (display->screen->num_displays > 1) {
        selected_mode = allocate_selected_mode("Off",
                                               NULL /* modeline */,
                                               TRUE /* isSpecial */,
                                               NULL /* viewPortIn */,
                                               NULL /* viewPortOut */);

        display->num_selected_modes = 1;
        display->selected_modes = selected_mode;
    }

    modeline = display->modelines;
    while (modeline) {
        gchar *name;
        Bool isSpecial;
        Bool mode_added;

        if (IS_NVIDIA_DEFAULT_MODE(modeline)) {
            name = g_strdup_printf("Auto");
            isSpecial = TRUE;
        } else {
            name = g_strdup_printf("%dx%d",
                                   modeline->data.hdisplay,
                                   modeline->data.vdisplay);
            isSpecial = FALSE;
        }

        selected_mode = allocate_selected_mode(name, modeline, isSpecial,
                                               NULL /* viewPortIn */,
                                               NULL /* viewPortOut */);
        g_free(name);

        if (!display->selected_modes) {
            display->selected_modes = selected_mode;
            mode_added = TRUE;
        } else {
            mode_added = append_unique_selected_mode(display->selected_modes,
                                                     selected_mode);
        }

        if (mode_added) {
            display->num_selected_modes++;

            if (matches_current_selected_mode(display, selected_mode,
                                              FALSE /* compare_viewports */)) {
                display->cur_selected_mode = selected_mode;
            }
        } else {
            free(selected_mode);
        }

        modeline = modeline->next;
    }
}



/** generate_scaled_selected_modes() *********************************
 *
 * Appends a list of scaled selected modes.  The list is generated by parsing
 * an array of common resolutions.  This function makes sure that each item
 * of the list is unique and sorted.  The generated items are appended to the
 * list of selected modes returned by generate_selected_modes().
 *
 */

static void generate_scaled_selected_modes(const nvDisplayPtr display)
{
    int resIndex;
    nvModeLinePtr default_modeline;
    nvSelectedModePtr selected_mode = NULL;
    const NVVRSize *commonResolutions;
    NVVRSize raster;
    gchar *name;

    if (!display || !display->modelines) {
        return;
    }

    default_modeline = get_default_modeline(display);
    if (default_modeline == NULL) {
        return;
    }

    raster.w = default_modeline->data.hdisplay;
    raster.h = default_modeline->data.vdisplay;

    commonResolutions = NVVRGetCommonResolutions();

    resIndex = 0;
    while ((commonResolutions[resIndex].w != -1) &&
           (commonResolutions[resIndex].h != -1)) {
        NVVRBoxRecXYWH viewPortOut;
        NVVRSize viewPortIn = commonResolutions[resIndex];

        resIndex++;

        /* Skip resolutions that are bigger than the maximum raster size */
        if ((viewPortIn.w > raster.w) || (viewPortIn.h > raster.h)) {
            continue;
        }

        viewPortOut = NVVRGetScaledViewPortOut(&raster, &viewPortIn,
                                               NVVR_SCALING_ASPECT_SCALED);

        name = g_strdup_printf("%dx%d (scaled)", viewPortIn.w, viewPortIn.h);
        selected_mode = allocate_selected_mode(name, default_modeline,
                                               FALSE /* isSpecial */,
                                               &viewPortIn, &viewPortOut);
        g_free(name);

        if (append_unique_selected_mode(display->selected_modes,
                                        selected_mode)) {
            display->num_selected_modes++;

            if (matches_current_selected_mode(display, selected_mode,
                                              TRUE /* compare_viewports */)) {
                display->cur_selected_mode = selected_mode;
            }
        } else {
            free(selected_mode);
        }
    }
}



/** setup_display_resolution_dropdown() ******************************
 *
 * Generates the resolution dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_resolution_dropdown(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    nvSelectedModePtr selected_mode;

    int cur_idx = 0;  /* Currently selected modeline (resolution) */

    /* Get selection information */
    if (!display->screen || !display->cur_mode) {
        gtk_widget_hide(ctk_object->box_display_resolution);
        return;
    }
    gtk_widget_show(ctk_object->box_display_resolution);
    gtk_widget_set_sensitive(ctk_object->box_display_resolution, TRUE);

    /* Generate dropdown content */
    free_selected_modes(display->selected_modes);

    /* Create the selected modes lookup table for the dropdown */
    display->cur_selected_mode = NULL;
    generate_selected_modes(display);

    if (!ctk_object->advanced_mode) {
        generate_scaled_selected_modes(display);
    }

    if (ctk_object->resolution_table) {
        free(ctk_object->resolution_table);
        ctk_object->resolution_table_len = 0;
    }
    ctk_object->resolution_table =
        calloc(display->num_selected_modes, sizeof(nvSelectedModePtr));
    if (!ctk_object->resolution_table) {
        goto fail;
    }


    if (display->cur_mode->modeline && display->screen->num_displays > 1) {
        /*
         * Modeline is set and we have more than 1 display, start off as
         * 'nvidia-auto-select'
         */
        cur_idx = 1;
    } else {
        /*
         * Modeline not set, start off as 'off'. If we do not have more than
         * 1 display, 'auto' will be at index 0.
         */
        cur_idx = 0;
    }

    /* Setup the menu */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
        (GTK_COMBO_BOX(ctk_object->mnu_display_resolution))));

    /* Fill dropdown menu */
    selected_mode = display->selected_modes;
    while (selected_mode) {

        ctk_combo_box_text_append_text(ctk_object->mnu_display_resolution,
                                       selected_mode->text);

        ctk_object->resolution_table[ctk_object->resolution_table_len] =
            selected_mode;

        if (selected_mode == display->cur_selected_mode) {
            cur_idx = ctk_object->resolution_table_len;
        }

        ctk_object->resolution_table_len++;
        selected_mode = selected_mode->next;
    }

    /* Select the current mode */
    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_resolution), cur_idx);
    ctk_object->last_resolution_idx = cur_idx;

    /* If dropdown has only one item, disable menu selection */
    if (ctk_object->resolution_table_len > 1) {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, TRUE);
    } else {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, FALSE);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

     /* Update refresh dropdown */
    setup_display_refresh_dropdown(ctk_object);
    return;


    /* Handle failures */
 fail:
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(
        GTK_COMBO_BOX(ctk_object->mnu_display_resolution))));

    gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, FALSE);

    setup_display_refresh_dropdown(ctk_object);

} /* setup_display_resolution_dropdown() */



/** setup_display_stereo_dropdown() **********************************
 *
 * Configures the display stereo mode dropdown to reflect the
 * stereo eye for the currently selected display.
 *
 **/

static void setup_display_stereo_dropdown(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvModePtr mode;
    int idx;

    if (!display->cur_mode || !screen ||
        !screen->stereo_supported ||
        (screen->stereo != NV_CTRL_STEREO_PASSIVE_EYE_PER_DPY)) {
        gtk_widget_hide(ctk_object->box_display_stereo);
        return;
    }

    mode = display->cur_mode;


    /* Set the selected passive stereo eye */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_stereo),
         G_CALLBACK(display_stereo_changed), (gpointer) ctk_object);

    switch (mode->passive_stereo_eye) {
    default:
        /* Oops */
    case PASSIVE_STEREO_EYE_NONE:
        idx = 0;
        break;
    case PASSIVE_STEREO_EYE_LEFT:
        idx = 1;
        break;
    case PASSIVE_STEREO_EYE_RIGHT:
        idx = 2;
        break;
    }

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_stereo), idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_stereo),
         G_CALLBACK(display_stereo_changed), (gpointer) ctk_object);

    gtk_widget_show(ctk_object->box_display_stereo);

} /* setup_display_stereo_dropdown() */



/** are_display_composition_transformations_allowed() ****************
 *
 * Checks whether display composition transformations are allowed
 * given the list of GPU flags.
 *
 **/

static Bool are_display_composition_transformations_allowed(nvScreenPtr screen)
{
    int i, j;
    Bool ret = TRUE;

    if (!screen) {
        return FALSE;
    }

    for (i = 0; i < screen->num_gpus; i++) {
        nvGpuPtr gpu = screen->gpus[i];

        for (j = 0; j < gpu->num_flags; j++) {
            switch (gpu->flags[j]) {
            case NV_CTRL_BINARY_DATA_GPU_FLAGS_STEREO_DISPLAY_TRANSFORM_EXCLUSIVE:
                if (screen->stereo != NV_CTRL_STEREO_OFF) {
                    ret = FALSE;
                }
                break;
            case NV_CTRL_BINARY_DATA_GPU_FLAGS_OVERLAY_DISPLAY_TRANSFORM_EXCLUSIVE:
                if (screen->overlay != NV_CTRL_OVERLAY_OFF) {
                    ret = FALSE;
                }
                break;
            default:
                /* We don't care about other flags */
                break;
            }
        }
    }

    return ret;
}



/** setup_display_rotation_dropdown() ********************************
 *
 * Configures the display rotation dropdown to reflect the current
 * rotation configuration.
 *
 **/

static void setup_display_rotation_dropdown(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int idx;

    /* Set the selected rotation */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_rotation),
         G_CALLBACK(display_rotation_changed), (gpointer) ctk_object);

    switch (display->cur_mode->rotation) {
    default:
        /* Oops */
    case ROTATION_0:
        idx = 0;
        break;
    case ROTATION_90: // Rotate left
        idx = 1;
        break;
    case ROTATION_180: // Invert
        idx = 2;
        break;
    case ROTATION_270: // Rotate right
        idx = 3;
        break;
    }

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_rotation), idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_rotation),
         G_CALLBACK(display_rotation_changed), (gpointer) ctk_object);

} /* setup_display_rotation_dropdown() */



/** setup_display_reflection_dropdown() ******************************
 *
 * Configures the display reflection dropdown to reflect the current
 * reflection configuration.
 *
 **/

static void setup_display_reflection_dropdown(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int idx;

    /* Set the selected reflection */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_reflection),
         G_CALLBACK(display_reflection_changed), (gpointer) ctk_object);

    switch (display->cur_mode->reflection) {
    default:
        /* Oops */
    case REFLECTION_NONE:
        idx = 0;
        break;
    case REFLECTION_X:
        idx = 1;
        break;
    case REFLECTION_Y:
        idx = 2;
        break;
    case REFLECTION_XY:
        idx = 3;
        break;
    }

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_reflection), idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_reflection),
         G_CALLBACK(display_reflection_changed), (gpointer) ctk_object);

} /* setup_display_reflection_dropdown() */



/** setup_display_orientation() **************************************
 *
 * Sets up the display orientation section to reflect the rotation
 * and reflection settings for the currently selected display.
 *
 **/

static void setup_display_orientation(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Display needs to be included in an X screen to show widgets */
    if (!display || !display->screen) {
        gtk_widget_hide(ctk_object->box_display_orientation);
        return;
    }
    gtk_widget_show(ctk_object->box_display_orientation);

    /* If the display is off, disable the orientation widgets */
    if (!display->cur_mode || !display->cur_mode->modeline ||
        !are_display_composition_transformations_allowed(display->screen)) {
        gtk_widget_set_sensitive(ctk_object->box_display_orientation, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_orientation, TRUE);

    /* Setup the display orientation widgets */
    setup_display_rotation_dropdown(ctk_object);
    setup_display_reflection_dropdown(ctk_object);
}



/** setup_display_underscan() *****************************************
 *
 * Sets up the display underscan to reflect the ViewPortOut settings
 * for the currently selected display.
 *
 * Tries to detect whether the current ViewPortOut configuration
 * corresponds to a border; then sets the underscan text entry and
 * slider accordingly.  Defaults to 0.
 **/

static void setup_display_underscan(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvModePtr cur_mode;
    nvSize raster_size;
    gfloat adj_value;
    gint hpixel_value;
    gchar *txt_entry;

    if (!display || !display->screen || ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_display_underscan);
        return;
    }
    gtk_widget_show(ctk_object->box_display_underscan);

    cur_mode = display->cur_mode;

    /*
     * If the display is off or a scaled mode is selected, disable the
     * underscan widget.
     */
    if (!cur_mode || !cur_mode->modeline ||
        (display->cur_selected_mode && display->cur_selected_mode->isScaled)) {
        gtk_widget_set_sensitive(ctk_object->box_display_underscan, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_underscan, TRUE);

    raster_size.height = cur_mode->modeline->data.vdisplay;
    raster_size.width = cur_mode->modeline->data.hdisplay;

    get_underscan_settings_from_viewportout(raster_size, cur_mode->viewPortOut,
                                            &adj_value, &hpixel_value);

    /* Setup the slider */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->adj_display_underscan),
         G_CALLBACK(display_underscan_value_changed), (gpointer) ctk_object);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(ctk_object->adj_display_underscan),
                             (adj_value < 0) ? 0 : adj_value);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object-> adj_display_underscan),
         G_CALLBACK(display_underscan_value_changed), (gpointer) ctk_object);


    /* Setup the text entry */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->txt_display_underscan),
         G_CALLBACK(display_underscan_activate), (gpointer) ctk_object);

    if (hpixel_value < 0) {
        txt_entry = g_strdup_printf("n/a");
    } else {
        txt_entry = g_strdup_printf("%d", hpixel_value);
    }
    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_underscan), txt_entry);
    g_free(txt_entry);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object-> txt_display_underscan),
         G_CALLBACK(display_underscan_activate), (gpointer) ctk_object);

} /* setup_display_underscan() */



/** setup_display_viewport_in() **************************************
 *
 * Sets up the display ViewPortIn text entry to reflect the currently
 * selected display device/mode.
 *
 **/

static void setup_display_viewport_in(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvModePtr mode;

    if (!display || !display->screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_display_viewport_in);
        return;
    }
    gtk_widget_show(ctk_object->box_display_viewport_in);

    if (!display->cur_mode || !display->cur_mode->modeline) {
        gtk_widget_set_sensitive(ctk_object->box_display_viewport_in, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_viewport_in, TRUE);

    /* Update the text */
    mode = display->cur_mode;

    tmp_str = g_strdup_printf("%dx%d",
                              mode->viewPortIn.width,
                              mode->viewPortIn.height);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_viewport_in),
                       tmp_str);

    g_free(tmp_str);

} /* setup_display_viewport_in() */



/** setup_display_viewport_out() *************************************
 *
 * Sets up the display ViewPortOut text entry to reflect the currently
 * selected display device/mode.
 *
 **/

static void setup_display_viewport_out(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    nvModePtr mode;

    if (!display || !display->screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_display_viewport_out);
        return;
    }
    gtk_widget_show(ctk_object->box_display_viewport_out);

    if (!display->cur_mode || !display->cur_mode->modeline) {
        gtk_widget_set_sensitive(ctk_object->box_display_viewport_out, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_viewport_out, TRUE);


    /* Update the text */
    mode = display->cur_mode;

    tmp_str = g_strdup_printf("%dx%d%+d%+d",
                              mode->viewPortOut.width,
                              mode->viewPortOut.height,
                              mode->viewPortOut.x,
                              mode->viewPortOut.y);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_viewport_out),
                       tmp_str);

    g_free(tmp_str);

} /* setup_display_viewport_out() */



/** setup_display_position_type() ************************************
 *
 * Sets up the display position type dropdown to reflect the position
 * settings for the currently selected display (absolute/relative/
 * none).
 *
 **/

static void setup_display_position_type(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    
    /* Handle cases where the position type should be hidden */
    if (!display || !display->screen || !display->cur_mode) {
        gtk_widget_hide(ctk_object->mnu_display_position_type);
        return;
    }
    gtk_widget_show(ctk_object->mnu_display_position_type);


    /* Set absolute/relative positioning */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_position_type),
         G_CALLBACK(display_position_type_changed), (gpointer) ctk_object);

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_type),
         display->cur_mode->position_type);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_position_type),
         G_CALLBACK(display_position_type_changed), (gpointer) ctk_object);

} /* setup_display_position_type() */



/** setup_display_position_relative() ********************************
 *
 * Setup which display the selected display is relative to.
 *
 **/

static void setup_display_position_relative(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display;
    nvDisplayPtr relative_to;
    int idx;
    int selected_idx;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* No need to show dropdown if display position is absolute */
    if (!display || !display->screen || !display->cur_mode || !display->gpu) {
        goto fail;
    }


    /* Allocate the display lookup table for the dropdown */
    if (ctk_object->display_position_table) {
        free(ctk_object->display_position_table);
    }

    ctk_object->display_position_table_len =
        display->screen->num_displays -1;

    ctk_object->display_position_table =
        calloc(ctk_object->display_position_table_len, sizeof(nvDisplayPtr));

    if (!ctk_object->display_position_table) {
        goto fail;
    }


    /* Generate the lookup table and display dropdown */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_position_relative),
         G_CALLBACK(display_position_relative_changed), (gpointer) ctk_object);

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_relative))));

    idx = 0;
    selected_idx = 0;
    for (relative_to = display->screen->displays;
         relative_to;
         relative_to = relative_to->next_in_screen) {

        if (relative_to == display) continue;

        if (relative_to == display->cur_mode->relative_to) {
            selected_idx = idx;
        }

        ctk_object->display_position_table[idx] = relative_to;

        ctk_combo_box_text_append_text(
            ctk_object->mnu_display_position_relative,
            relative_to->logName);
        idx++;
    }


    /* Set the menu and the selected display */
    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_relative),
         selected_idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_position_relative),
         G_CALLBACK(display_position_relative_changed), (gpointer) ctk_object);


    /* Disable the widget if there is only one possibility */
    gtk_widget_set_sensitive
        (ctk_object->mnu_display_position_relative,
         (idx > 1));


    /* Hide the dropdown if the display position is absolute */
    if (display->cur_mode->position_type == CONF_ADJ_ABSOLUTE) {
        gtk_widget_hide(ctk_object->mnu_display_position_relative);
        return;
    }

    gtk_widget_show(ctk_object->mnu_display_position_relative);
    return;


 fail:
    if (ctk_object->display_position_table) {
        free(ctk_object->display_position_table);
        ctk_object->display_position_table = NULL;
    }
    ctk_object->display_position_table_len = 0;
    gtk_widget_hide(ctk_object->mnu_display_position_relative);
    
} /* setup_display_position_relative() */



/** setup_display_position_offset() **********************************
 *
 * Sets up the display position offset text entry to reflect the
 * currently selected display device.
 *
 **/

static void setup_display_position_offset(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvDisplayPtr display;
    nvModePtr mode;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Handle cases where the position offset should be hidden */
    if (!display || !display->screen || !display->cur_mode ||
        !display->cur_mode->modeline ||
        display->cur_mode->position_type != CONF_ADJ_ABSOLUTE) {
        gtk_widget_hide(ctk_object->txt_display_position_offset);
        return;
    }
    gtk_widget_show(ctk_object->txt_display_position_offset);


    /* Update the position text */
    mode = display->cur_mode;

    tmp_str = g_strdup_printf("%+d%+d",
                              mode->pan.x - mode->metamode->edim.x,
                              mode->pan.y - mode->metamode->edim.y);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_position_offset),
                       tmp_str);

    g_free(tmp_str);

} /* setup_display_position_offset() */



/** setup_display_position() *****************************************
 *
 * Sets up the display position section to reflect the position
 * settings for the currently selected display (absolute/relative/
 * none).
 *
 **/

static void setup_display_position(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Need at least 2 displays in the X screen to configure position */
    if (!display || !display->screen || (display->screen->num_displays < 2)) {
        gtk_widget_hide(ctk_object->box_display_position);
        return;
    }
    gtk_widget_show(ctk_object->box_display_position);


    /* If the display is off, disable the position widgets */
    if (!display->cur_mode || !display->cur_mode->modeline) {
        gtk_widget_set_sensitive(ctk_object->box_display_position, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_position, TRUE);


    /* Setup the display position widgets */
    setup_display_position_type(ctk_object);

    setup_display_position_relative(ctk_object);

    setup_display_position_offset(ctk_object);

} /* setup_display_position */



/** setup_forcecompositionpipeline_buttons() *************************
 *
 * Sets up the ForceCompositionPipeline and ForceFullCompositionPipeline
 * checkboxes to reflect whether or not they are selected in the current
 * MetaMode.
 *
 **/

static void setup_forcecompositionpipeline_buttons(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display || !display->screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->chk_forcecompositionpipeline_enabled);
        gtk_widget_hide(ctk_object->chk_forcefullcompositionpipeline_enabled);
        return;
    }

    if (!display->cur_mode) {
        gtk_widget_hide(ctk_object->chk_forcecompositionpipeline_enabled);
        gtk_widget_hide(ctk_object->chk_forcefullcompositionpipeline_enabled);
        return;
    }

    gtk_widget_show(ctk_object->chk_forcecompositionpipeline_enabled);
    gtk_widget_show(ctk_object->chk_forcefullcompositionpipeline_enabled);

    update_forcecompositionpipeline_buttons(ctk_object);
}



/** setup_primary_display() ******************************************
 *
 * Sets up the primary display device for an X screen.
 *
 **/

static void setup_primary_display(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* Hide the checkbox if this screen only has one display */
    if (!display->screen || display->screen->num_displays <= 1) {
        gtk_widget_hide(ctk_object->chk_primary_display);
        return;
    }

    gtk_widget_show(ctk_object->chk_primary_display);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->chk_primary_display),
         G_CALLBACK(screen_primary_display_toggled), (gpointer) ctk_object);

    if ( display->screen && display == display->screen->primaryDisplay ) {
        // Primary display checkbox should be checked.
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->chk_primary_display),
             TRUE);
    } else {
        // This display does not have a screen or is not the screen's
        // primary display.
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->chk_primary_display),
             FALSE);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->chk_primary_display),
         G_CALLBACK(screen_primary_display_toggled),
         (gpointer) ctk_object);

}  /* setup_primary_display() */



/** setup_display_panning() ******************************************
 *
 * Sets up the display panning text entry to reflect the currently
 * selected display. 
 *
 **/

static void setup_display_panning(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvDisplayPtr display;
    nvModePtr mode;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display || !display->screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_display_panning);
        return;
    }
    gtk_widget_show(ctk_object->box_display_panning);


    if (!display->cur_mode || !display->cur_mode->modeline) {
        gtk_widget_set_sensitive(ctk_object->box_display_panning, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_panning, TRUE);


    /* Update the panning text */
    mode    = display->cur_mode;
    tmp_str = g_strdup_printf("%dx%d", mode->pan.width, mode->pan.height);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_panning),
                       tmp_str);
    g_free(tmp_str);

} /* setup_display_panning */


/** setup_prime_display_page() *********************************************
 *
 *
 **/
static void setup_prime_display_page(CtkDisplayConfig *ctk_object)
{
    nvPrimeDisplayPtr prime = ctk_display_layout_get_selected_prime_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    char *tmp_str = NULL;

    if (!prime) {
        return;
    }

    tmp_str = g_strdup_printf("%dx%d+%d+%d", prime->rect.width,
                                             prime->rect.height,
                                             prime->rect.x,
                                             prime->rect.y);
    gtk_label_set_text(GTK_LABEL(ctk_object->lbl_prime_display_view), tmp_str);
    g_free(tmp_str);

    if (prime->label) {
        gtk_label_set_text(GTK_LABEL(ctk_object->lbl_prime_display_name),
                           prime->label);
        gtk_widget_show_all(ctk_object->box_prime_display_name);
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_object->lbl_prime_display_name), "");
        gtk_widget_hide(ctk_object->box_prime_display_name);
    }

    gtk_label_set_text(GTK_LABEL(ctk_object->lbl_prime_display_sync),
                       prime->sync ? "On" : "Off");

    gtk_widget_set_sensitive(ctk_object->prime_display_page, True);
}


/** setup_force_gsync() ***********************************************
 *
 * Control whether to make visible the checkbox that allows enabling G-SYNC
 * on displays not validated as G-SYNC compatible.
 *
 **/

static void setup_force_gsync(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    ReturnStatus ret;
    int val;

    if (!display || !display->screen || !display->cur_mode ||
        !ctk_object->advanced_mode) {
        goto hide;
    }

    ret = NvCtrlGetAttribute(display->ctrl_target,
                             NV_CTRL_DISPLAY_VRR_MODE, &val);
    if (ret != NvCtrlSuccess) {
        goto hide;
    }

    /*
     * Show the checkbox only in advanced mode, and only if the display is not
     * validated as G-SYNC Compatible.
     */
    switch (val) {
    case NV_CTRL_DISPLAY_VRR_MODE_GSYNC_COMPATIBLE_UNVALIDATED:
        gtk_widget_show(ctk_object->chk_force_allow_gsync);
        break;
    case NV_CTRL_DISPLAY_VRR_MODE_GSYNC:
    case NV_CTRL_DISPLAY_VRR_MODE_GSYNC_COMPATIBLE:
    case NV_CTRL_DISPLAY_VRR_MODE_NONE:
    default:
        goto hide;
        break;
    }

    update_force_gsync_button(ctk_object);
    return;

hide:
    gtk_widget_hide(ctk_object->chk_force_allow_gsync);
}


/** setup_display_page() ********************************************
 *
 * Updates the display frame to reflect the current state of the
 * currently selected display.
 *
 **/

static void setup_display_page(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display) {
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->display_page, True);

    if (display->gpu->layout->num_screens > 1) {
        gtk_widget_show(ctk_object->box_screen_drag_info_display);
    } else {
        gtk_widget_hide(ctk_object->box_screen_drag_info_display);
    }

    setup_display_config(ctk_object);
    setup_display_modename(ctk_object);
    setup_display_resolution_dropdown(ctk_object);
    setup_display_stereo_dropdown(ctk_object);
    setup_display_orientation(ctk_object);
    setup_display_underscan(ctk_object);
    setup_display_viewport_in(ctk_object);
    setup_display_viewport_out(ctk_object);
    setup_display_position(ctk_object);
    setup_display_panning(ctk_object);
    setup_forcecompositionpipeline_buttons(ctk_object);
    setup_primary_display(ctk_object);
    setup_force_gsync(ctk_object);

} /* setup_display_page() */



/** setup_screen_virtual_size() **************************************
 *
 * Sets up the UI for configuring the virtual width/height of the
 * currently selected X screen.
 *
 **/

static void setup_screen_virtual_size(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvScreenPtr screen;

    screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Only show this box for no-scanout screens */
    if (!screen || !screen->no_scanout) {
        gtk_widget_hide(ctk_object->box_screen_virtual_size);
        return;
    }
    gtk_widget_show(ctk_object->box_screen_virtual_size);


    /* Update the virtual size text */
    tmp_str = g_strdup_printf("%dx%d", screen->dim.width, screen->dim.height);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_screen_virtual_size),
                       tmp_str);
    g_free(tmp_str);

} /* setup_screen_virtual_size() */



/** grow_screen_depth_table() **************************************
 *
 * realloc the screen_depth_table, if possible.
 *
 **/

static gboolean grow_screen_depth_table(CtkDisplayConfig *ctk_object)
{
    int *tmp = realloc(ctk_object->screen_depth_table,
                       sizeof(int) *
                       (ctk_object->screen_depth_table_len + 1));
    if (!tmp) {
        return False;
    }

    ctk_object->screen_depth_table = tmp;
    ctk_object->screen_depth_table_len++;

    return True;

} /* grow_screen_depth_table() */



/** setup_screen_depth_dropdown() ************************************
 *
 * Generates the color depth dropdown based on the currently selected
 * display device.
 *
 **/

static void setup_screen_depth_dropdown(CtkDisplayConfig *ctk_object)
{
    int cur_idx;
    gboolean add_depth_30_option;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) {
        gtk_widget_hide(ctk_object->box_screen_depth);
        return;
    }
    if (ctk_object->screen_depth_table) {
        free(ctk_object->screen_depth_table);
    }
    ctk_object->screen_depth_table = NULL;
    ctk_object->screen_depth_table_len = 0;

    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_screen_depth),
                                    G_CALLBACK(screen_depth_changed),
                                    (gpointer) ctk_object);

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
        (GTK_COMBO_BOX(ctk_object->mnu_screen_depth))));


    /* If Xinerama is enabled, only allow depth 30 if all
     * gpu/screens have support for depth 30.
     */

    if (ctk_object->layout->xinerama_enabled) {
        add_depth_30_option = layout_supports_depth_30(screen->layout);
    } else {
        add_depth_30_option = screen->allow_depth_30;
    }

    if (add_depth_30_option) {

        if (grow_screen_depth_table(ctk_object)) {
            ctk_combo_box_text_append_text
                (ctk_object->mnu_screen_depth,
                 "1.1 Billion Colors (Depth 30) - Experimental");
            ctk_object->screen_depth_table[ctk_object->screen_depth_table_len-1] = 30;
        }
    }

    if (grow_screen_depth_table(ctk_object)) {
        ctk_combo_box_text_append_text(ctk_object->mnu_screen_depth,
                                       "16.7 Million Colors (Depth 24)");
        ctk_object->screen_depth_table[ctk_object->screen_depth_table_len-1] = 24;
    }

    if (grow_screen_depth_table(ctk_object)) {
        ctk_combo_box_text_append_text(ctk_object->mnu_screen_depth,
                                       "65,536 Colors (Depth 16)");
        ctk_object->screen_depth_table[ctk_object->screen_depth_table_len-1] = 16;
    }

    if (grow_screen_depth_table(ctk_object)) {
        ctk_combo_box_text_append_text(ctk_object->mnu_screen_depth,
                                       "32,768 Colors (Depth 15)");
        ctk_object->screen_depth_table[ctk_object->screen_depth_table_len-1] = 15;
    }

    if (grow_screen_depth_table(ctk_object)) {
        ctk_combo_box_text_append_text(ctk_object->mnu_screen_depth,
                                       "256 Colors (Depth 8)");
        ctk_object->screen_depth_table[ctk_object->screen_depth_table_len-1] = 8;
    }

    
    for (cur_idx = 0; cur_idx < ctk_object->screen_depth_table_len; cur_idx++) {
        if (screen->depth == ctk_object->screen_depth_table[cur_idx]) {
            gtk_combo_box_set_active
                (GTK_COMBO_BOX(ctk_object->mnu_screen_depth), cur_idx);
        }
    }

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->mnu_screen_depth),
                                      G_CALLBACK(screen_depth_changed),
                                      (gpointer) ctk_object);

    gtk_widget_show(ctk_object->box_screen_depth);

    return;

} /* setup_screen_depth_dropdown() */



/** setup_screen_stereo_dropdown() ***********************************
 *
 * Configures the screen stereo mode dropdown to reflect the
 * stereo mode for the currently selected screen.
 *
 **/

static void setup_screen_stereo_dropdown(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int i, index = screen->stereo;

    if (!ctk_object->box_screen_stereo) return;

    /* Handle cases where the position type should be hidden */
    if (!screen || !screen->stereo_supported) {
        gtk_widget_hide(ctk_object->box_screen_stereo);
        return;
    }

    /* translation from stereo mode to dropdown index */
    for (i = 0; i < ctk_object->stereo_table_size; i++) {
        if (ctk_object->stereo_table[i] == screen->stereo) {
            index = i;
            break;
        }
    }

    /* Set the selected positioning type */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_screen_stereo),
         G_CALLBACK(screen_stereo_changed), (gpointer) ctk_object);

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_stereo),
         index);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_screen_stereo),
         G_CALLBACK(screen_stereo_changed), (gpointer) ctk_object);

    gtk_widget_show(ctk_object->box_screen_stereo);

} /* setup_screen_stereo_dropdown() */



/** setup_screen_position_type() *************************************
 *
 * Configures the screen position type dropdown to reflect the
 * position setting for the currently selected screen.
 *
 **/

static void setup_screen_position_type(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    
    /* Handle cases where the position type should be hidden */
    if (!screen) {
        gtk_widget_hide(ctk_object->mnu_screen_position_type);
        return;
    }
    gtk_widget_show(ctk_object->mnu_screen_position_type);


    /* Set the selected positioning type */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_screen_position_type),
         G_CALLBACK(screen_position_type_changed), (gpointer) ctk_object);

    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_type),
         screen->position_type);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_screen_position_type),
         G_CALLBACK(screen_position_type_changed), (gpointer) ctk_object);

} /* setup_screen_position_type() */



/** setup_screen_position_relative() *********************************
 *
 * Setup which screen the selected screen is relative to.
 *
 **/

static void setup_screen_position_relative(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvScreenPtr relative_to;
    int idx;
    int selected_idx;

    screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!screen) {
        goto fail;
    }


    /* Count the number of screens, not including the current one */
    ctk_object->screen_position_table_len = layout->num_screens;
    if (ctk_object->screen_position_table_len > 0) {
        ctk_object->screen_position_table_len--;
    }

    /* Allocate the screen lookup table for the dropdown */
    if (ctk_object->screen_position_table) {
        free(ctk_object->screen_position_table);
    }
    ctk_object->screen_position_table =
        calloc(ctk_object->screen_position_table_len, sizeof(nvScreenPtr));

    if (!ctk_object->screen_position_table) {
        goto fail;
    }


    /* Generate the lookup table and screen dropdown */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_screen_position_relative),
         G_CALLBACK(screen_position_relative_changed), (gpointer) ctk_object);

    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_relative))));

    idx = 0;
    selected_idx = 0;

    for (relative_to = layout->screens;
         relative_to;
         relative_to = relative_to->next_in_layout) {

        gchar *tmp_str;

        if (relative_to == screen) continue;

        if (relative_to == screen->relative_to) {
            selected_idx = idx;
        }

        ctk_object->screen_position_table[idx] = relative_to;

        tmp_str = g_strdup_printf("X screen %d",
                                  relative_to->scrnum);
        ctk_combo_box_text_append_text(ctk_object->mnu_screen_position_relative,
                                       tmp_str);
        g_free(tmp_str);
        idx++;
    }


    /* Set the menu and the selected display */
    gtk_combo_box_set_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_relative),
         selected_idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_screen_position_relative),
         G_CALLBACK(screen_position_relative_changed), (gpointer) ctk_object);


    /* Disable the widget if there is only one possibility */
    gtk_widget_set_sensitive
        (ctk_object->mnu_screen_position_relative,
         (idx > 1));


    /* Hide the dropdown if the screen position is absolute */
    if (screen->position_type == CONF_ADJ_ABSOLUTE) {
        gtk_widget_hide(ctk_object->mnu_screen_position_relative);
        return;
    }

    gtk_widget_show(ctk_object->mnu_screen_position_relative);
    return;


 fail:
    if (ctk_object->screen_position_table) {
        free(ctk_object->screen_position_table);
        ctk_object->screen_position_table = NULL;
    }
    ctk_object->screen_position_table_len = 0;
    gtk_widget_hide(ctk_object->mnu_screen_position_relative);

} /* setup_screen_position_relative() */



/** setup_screen_position_offset() ***********************************
 *
 * Sets up the screen position offset text entry to reflect the
 * currently selected screen.
 *
 **/

static void setup_screen_position_offset(CtkDisplayConfig *ctk_object)
{
    char *tmp_str;
    nvScreenPtr screen;

    screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Handle cases where the position offset should be hidden */
    if (!screen ||
        (screen->position_type != CONF_ADJ_ABSOLUTE && 
         screen->position_type != CONF_ADJ_RELATIVE)) {
        gtk_widget_hide(ctk_object->txt_screen_position_offset);
        return;
    }
    gtk_widget_show(ctk_object->txt_screen_position_offset);


    /* Update the position text */
    tmp_str = g_strdup_printf("%+d%+d", screen->dim.x, screen->dim.y);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_screen_position_offset),
                       tmp_str);

    g_free(tmp_str);

} /* setup_screen_position_offset() */



/** setup_screen_position() ******************************************
 *
 * Sets up the screen position section to reflect the position
 * settings for the currently selected screen.
 *
 **/

static void setup_screen_position(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Need at least 2 X screens to configure position */
    if (!screen || (layout->num_screens < 2)) {
        gtk_widget_hide(ctk_object->box_screen_position);
        return;
    }
    gtk_widget_show(ctk_object->box_screen_position);


    /* Setup the screen position widgets */
    setup_screen_position_type(ctk_object);

    setup_screen_position_relative(ctk_object);

    setup_screen_position_offset(ctk_object);

} /* setup_screen_position() */



/** setup_screen_metamode() ******************************************
 *
 * Generates the metamode dropdown for the selected screen
 *
 **/

static void setup_screen_metamode(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gchar *str;


    /* Only show the metamodes in advanced mode for screens
     * that support scanout.
     */
    if (!screen || screen->no_scanout || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_screen_metamode);
        return;
    }


    /* Update the metamode selector button */
    str = g_strdup_printf("%d - ...", screen->cur_metamode_idx +1);
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_screen_metamode), str);
    g_free(str);

    /* Only allow deletes if there are more than 1 metamodes */
    gtk_widget_set_sensitive(ctk_object->btn_screen_metamode_delete,
                             ((screen->num_metamodes > 1) ? True : False));
    
    gtk_widget_show(ctk_object->box_screen_metamode);

} /* setup_screen_metamode() */



/** setup_screen_page() *********************************************
 *
 * Sets up the screen frame to reflect the currently selected screen.
 *
 **/

static void setup_screen_page(CtkDisplayConfig *ctk_object)
{
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!screen) {
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->screen_page, True);

    if (screen->layout->num_screens > 1) {
        gtk_widget_show(ctk_object->box_screen_drag_info_screen);
    } else {
        gtk_widget_hide(ctk_object->box_screen_drag_info_screen);
    }

    setup_screen_virtual_size(ctk_object);
    setup_screen_depth_dropdown(ctk_object);
    setup_screen_stereo_dropdown(ctk_object);
    setup_screen_position(ctk_object);
    setup_screen_metamode(ctk_object);

} /* setup_screen_page() */



/** validation_fix_crowded_metamodes() *******************************
 *
 * Goes through each screen's metamodes and ensures that at most
 * (max supported) display devices are active (have a modeline set) per
 * metamode.  This function also checks to make sure that there is at least
 * one display device active for each metamode.
 *
 **/

static gint validation_fix_crowded_metamodes(CtkDisplayConfig *ctk_object,
                                             nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvModePtr first_mode = NULL;
    nvModePtr mode;
    int num;
    int i, j;
    int max_displays = get_screen_max_displays(screen);


    /* Verify each metamode with the metamodes that come before it */
    for (i = 0; i < screen->num_metamodes; i++) {

        /* Keep track of the first mode in case we need to assign
         * a default resolution
         */
        first_mode = NULL;

        /* Count the number of display devices that have a mode
         * set for this metamode.  NULL out the modes of extra
         * display devices once we've counted max supported display devices
         * that have a (non NULL) mode set.
         */
        num = 0;
        for (display = screen->displays;
             display;
             display = display->next_in_screen) {

            /* Check the mode that corresponds with the metamode */
            mode = display->modes;
            for (j = 0; j < i; j++) {
                mode = mode->next;
            }

            if (!first_mode) {
                first_mode = mode;
            }

            if (mode->modeline) {
                num++;
            }

            /* Disable extra modes */
            if (max_displays >= 0 && num > max_displays) {
                ctk_display_layout_set_mode_modeline
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     mode,
                     NULL /* modeline */,
                     NULL /* viewPortIn */,
                     NULL /* viewPortOut */);

                nv_info_msg(TAB, "Setting display device '%s' as Off "
                            "for MetaMode %d on Screen %d.  (There are "
                            "already %d active display devices for this "
                            "MetaMode.", display->logName, i, screen->scrnum,
                            max_displays);
            }
        }

        /* Handle the case where a metamode has no active display device */
        if (!num) {

            /* There are other modelines, so we can safely delete this one */
            if (screen->num_metamodes > 1) {

                /* Delete the metamode */
                ctk_display_layout_delete_screen_metamode
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, i,
                     TRUE);

                nv_info_msg(TAB, "Removed MetaMode %d on Screen %d (No "
                            "active display devices)\n", i,
                            screen->scrnum);

                /* Since we just deleted the current metamode, we 
                 * need to check the i'th metamode "again" since this
                 * is effectively the next metamode.
                 */
                i--;

            /* This is the only modeline, activate the first display */
            } else if (first_mode) {

                /* Select the first modeline in the modepool */
                ctk_display_layout_set_mode_modeline
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     first_mode,
                     first_mode->display->modelines,
                     NULL /* viewPortIn */,
                     NULL /* viewPortOut */);

                nv_info_msg(TAB, "Activating display device '%s' for MetaMode "
                            "%d on Screen %d.  (Minimally, a Screen must have "
                            "one MetaMode with at least one active display "
                            "device.)",
                            first_mode->display->logName, i, screen->scrnum);
            }
        }
    }

    return 1;

} /* validation_fix_crowded_metamodes() */



/** validation_auto_fix_screen() *************************************
 *
 * Do what we can to make this screen conform to validation.
 *
 **/

static gint validation_auto_fix_screen(CtkDisplayConfig *ctk_object,
                                       nvScreenPtr screen)
{
    gint status = 1;

    status &= validation_fix_crowded_metamodes(ctk_object, screen);

    return status;

} /* validation_auto_fix_screen() */



/** validation_auto_fix() ********************************************
 *
 * Attempts to fix any problems found in the layout.  Returns 1 if
 * the layout is in a valid state when done.  Returns 0 if there
 * was a problem and the layout validation error could not be fixed
 * by this function.
 *
 **/

static gint validation_auto_fix(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    gint success = 1;


    /* Auto fix each screen */
    for (screen = layout->screens;
         screen;
         screen = screen->next_in_layout) {

        if (!validation_auto_fix_screen(ctk_object, screen)) {
            success = 0;
            break;
        }
    }

    if (!success) {
        nv_warning_msg("Failed to auto fix X configuration.");
        /* XXX We should pop up a dialog box to let the user know
         *     there are still problems.
         */
    }

    return success;

} /* validation_auto_fix() */



/** validate_screen() ************************************************
 *
 * This function returns NULL if the screen is found to be in a
 * valid state.  To be in a valid state the screen's metamodes must
 * adhere to the following:
 *
 * - Have at least 1 display device activated for all metamodes.
 *
 * - Have at most (max supported) display devices activated for all
 *   metamodes.
 *
 * - All metamodes must have a coherent offset (The top left corner
 *   of the bounding box of all the metamodes must be the same.)
 *
 * If the screen is found to be in an invalid state, a string
 * describing the problem is returned.  This string should be freed
 * by the user when done with it.
 *
 **/

static gchar * validate_screen(nvScreenPtr screen,
                               gboolean *can_ignore_error)
{
    nvDisplayPtr display;
    nvModePtr mode;
    int i, j;
    int max_displays = get_screen_max_displays(screen);
    int num_displays;
    gchar *err_str = NULL;
    gchar *tmp;
    gchar *tmp2;

    gchar bullet[8]; // UTF8 Bullet string
    int len;
    gboolean is_implicit;



    /* Convert the Unicode "Bullet" Character into a UTF8 string */
    len = g_unichar_to_utf8(0x2022, bullet);
    bullet[len] = '\0';


    for (i = 0; i < screen->num_metamodes; i++) {

        /* Count the number of display devices used in the metamode */
        num_displays = 0;
        is_implicit = TRUE;
        for (display = screen->displays;
             display;
             display = display->next_in_screen) {

            mode = display->modes;
            for (j = 0; j < i; j++) {
                mode = mode->next;
            }
            if (mode->modeline) {
                num_displays++;
            } else if (mode->metamode) {
                is_implicit = is_implicit &&
                    (mode->metamode->source == METAMODE_SOURCE_IMPLICIT);
            } else {
                is_implicit = FALSE;
            }
        }


        /* There must be at least one display active in the metamode. */
        if (!num_displays) {
            tmp = g_strdup_printf("%s MetaMode %d of Screen %d  does not have "
                                  "an active display device.\n\n",
                                  bullet, i+1, screen->scrnum);
            tmp2 = g_strconcat((err_str ? err_str : ""), tmp, NULL);
            g_free(err_str);
            g_free(tmp);
            err_str = tmp2;
            *can_ignore_error = *can_ignore_error && is_implicit;
        }


        /* There can be at most max supported displays active in the metamode. */
        if (max_displays >= 0 && num_displays > max_displays) {
            tmp = g_strdup_printf("%s MetaMode %d of Screen %d has more than "
                                  "%d active display devices.\n\n",
                                  bullet, i+1, screen->scrnum,
                                  max_displays);
            tmp2 = g_strconcat((err_str ? err_str : ""), tmp, NULL);
            g_free(err_str);
            g_free(tmp);
            err_str = tmp2;
            *can_ignore_error = FALSE;
        }
    }

    return err_str;

} /* validate_screen() */



/** validate_layout() ************************************************
 *
 * Makes sure that the layout is ready for applying/saving.
 *
 * If the layout is found to be invalid the user is prompted to
 * cancel the operation or to ignore and continue despite the
 * errors.
 *
 **/

static int validate_layout(CtkDisplayConfig *ctk_object, int validation_type)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    gchar *err_strs = NULL;
    gchar *err_str;
    gchar *tmp;
    gint result;
    int num_absolute = 0;
    gboolean can_ignore_error = TRUE;


    /* Validate each screen and count the number of screens using abs. pos. */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        err_str = validate_screen(screen, &can_ignore_error);
        if (err_str) {
            tmp = g_strconcat((err_strs ? err_strs : ""), err_str, NULL);
            g_free(err_strs);
            g_free(err_str);
            err_strs = tmp;
        }
        if (screen->position_type == CONF_ADJ_ABSOLUTE) {
            num_absolute++;
        }
    }

    if (validation_type == VALIDATE_SAVE) {

        /* Warn user when they are using absolute positioning with
         * multiple X screens.
         */
        if (num_absolute > 1) {
            GtkWidget *dlg;
            GtkWidget *parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));

            if (parent) {
                dlg = gtk_message_dialog_new
                    (GTK_WINDOW(parent),
                     GTK_DIALOG_DESTROY_WITH_PARENT,
                     GTK_MESSAGE_INFO,
                     GTK_BUTTONS_OK,
                     "Multiple X screens are set to use absolute "
                     "positioning.  Though it is valid to do so, one or more "
                     "X screens may be (or may become) unreachable due to "
                     "overlapping and/or dead space.  It is recommended to "
                     "only use absolute positioning for the first X screen, "
                     "and relative positioning for all subsequent X screens.");
                gtk_dialog_run(GTK_DIALOG(dlg));
                gtk_widget_destroy(dlg);
            }
        }
    }

    /* Layout is valid */
    if (!err_strs) {
        return 1;
    }

    /* Layout is not valid but inconsistencies are only due to implicit
     * metamodes not having valid displays so we will ignore them.
     */
    if (err_strs && can_ignore_error) {
        g_free(err_strs);
        return 1;
    }

    /* Layout is not valid, ask the user what we should do */
    gtk_text_buffer_set_text
        (GTK_TEXT_BUFFER(ctk_object->buf_validation_override), err_strs, -1);
    g_free(err_strs);

    gtk_widget_hide(ctk_object->box_validation_override_details);
    gtk_window_resize(GTK_WINDOW(ctk_object->dlg_validation_override),
                      350, 1);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_validation_override),
                             FALSE);
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_validation_override_show),
                         "Show Details...");

    /* Show the confirm dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_validation_override),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_grab_focus(ctk_object->btn_validation_override_cancel);
    gtk_widget_show(ctk_object->dlg_validation_override);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_validation_override));
    gtk_widget_hide(ctk_object->dlg_validation_override);
    
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        /* User wants to ignore the validation warnings */
        return 1;

    case GTK_RESPONSE_APPLY:
        /* User wants to auto fix the warnings */
        result = validation_auto_fix(ctk_object);

        /* Update the GUI to reflect any updates made by auto fix */
        update_gui(ctk_object);
        return result;

    case GTK_RESPONSE_REJECT:
    default:
        /* User wants to heed the validation warnings */
        return 0;
    }

    return 0;

} /* validate_layout() */



/** validate_apply() *************************************************
 *
 * Informs the user if we can't apply for whatever reason.  This
 * function returns FALSE if we should not continue the apply or
 * TRUE if we should continue the apply operation(s).
 *
 **/

static gboolean validate_apply(CtkDisplayConfig *ctk_object)
{
    gint result;

    if (ctk_object->apply_possible) {
        return TRUE;
    }

    /* Show the "can't apply" dialog */

    /* If we can't apply, let the user know.
     *
     * XXX - Show more details as to why exactly we can't apply.
     */

    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_validation_apply),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_show(ctk_object->dlg_validation_apply);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_validation_apply));
    gtk_widget_hide(ctk_object->dlg_validation_apply);
    

    switch (result) {
    case GTK_RESPONSE_ACCEPT:
        /* User wants to ignore the validation warnings */
        return TRUE;
        
    case GTK_RESPONSE_REJECT:
    default:
        return FALSE;
    }


    return FALSE;

} /* validate_apply() */



/* Callback handlers *************************************************/


/** layout_selected_callback() ***************************************
 *
 * Called every time the user selects a new display or screen from
 * the layout image.
 *
 **/

void layout_selected_callback(nvLayoutPtr layout, void *data)
{
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *)data;


    /* Reconfigure GUI to display information about the selected screen. */
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);
    setup_prime_display_page(ctk_object);
    setup_selected_item_dropdown(ctk_object);
    update_selected_page(ctk_object);

    get_cur_screen_pos(ctk_object);

} /* layout_selected_callback() */



/** layout_modified_callback() ***************************************
 *
 * Called every time the user moves a screen/display in the layout
 * image.
 *
 **/

void layout_modified_callback(nvLayoutPtr layout, void *data)
{
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *)data;


    /* Sync the information displayed by the GUI to match the settings
     * of the currently selected display device.
     */
    setup_display_viewport_in(ctk_object);
    setup_display_viewport_out(ctk_object);
    setup_display_position(ctk_object);
    setup_display_panning(ctk_object);

    setup_screen_position(ctk_object);
    setup_screen_virtual_size(ctk_object);

    /* If the positioning of the X screen changes, we cannot apply */
    check_screen_pos_changed(ctk_object);

    user_changed_attributes(ctk_object);

} /* layout_modified_callback()  */



/* Widget signal handlers ********************************************/


/** selected_item_changed() *********************************
 *
 * Called when user selects a new display or X screen.
 *
 **/
static void selected_item_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx;
    SelectableItem *item;

    idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    item = &(ctk_object->selected_item_table[idx]);

    switch (item->type) {
    case SELECTABLE_ITEM_SCREEN:
        ctk_display_layout_select_screen
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), item->u.screen);
        break;
    case SELECTABLE_ITEM_DISPLAY:
        ctk_display_layout_select_display
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), item->u.display);
        break;
    case SELECTABLE_ITEM_PRIME:
        ctk_display_layout_select_prime
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), item->u.prime);
        break;
    }

    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);
    setup_prime_display_page(ctk_object);
    update_selected_page(ctk_object);

} /* selected_item_changed() */



/** do_enable_display_on_new_xscreen() *******************************
 *
 * Adds the display device to a new X screen in the layout.
 *
 * This handles the "Disabled -> New X screen" transition.
 *
 **/

static void do_enable_display_on_new_xscreen(CtkDisplayConfig *ctk_object,
                                             nvDisplayPtr display)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvGpuPtr gpu;
    nvScreenPtr rightmost = NULL;
    nvScreenPtr other;
    nvMetaModePtr metamode;
    nvModePtr mode;
    int num_screens_on_gpu = 0;


    gpu = display->gpu;

    for (screen = layout->screens;
         screen;
         screen = screen->next_in_layout) {
        if (screen_has_gpu(screen, gpu)) {
            num_screens_on_gpu++;
        }
    }

    /* Make sure we're allowed to enable this display */
    if (gpu->mosaic_enabled ||
        (num_screens_on_gpu >= gpu->max_displays) ||
        display->screen) {
        return;
    }


    /* Get resources */
    screen = (nvScreenPtr)calloc(1, sizeof(nvScreen));
    metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
    if (!screen) return;
    if (!metamode) {
        free(screen);
        return;
    }


    /* Setup the display */
    screen_link_display(screen, display);


    /* Setup the mode */
    mode = display->modes;
    mode->metamode = metamode;

    mode_set_modeline(mode, display->modelines,
                      NULL /* viewPortIn */,
                      NULL /* viewPortOut */);

    mode->position_type = CONF_ADJ_ABSOLUTE;


    /* Setup the initial metamode */
    metamode->id = -1;
    metamode->source = METAMODE_SOURCE_NVCONTROL;
    metamode->switchable = True;


    /* Setup the screen */
    screen->scrnum = layout->num_screens;
    screen->display_owner_gpu_id = -1;
    link_screen_to_gpu(screen, gpu);

    other = layout_get_a_screen(layout, gpu);
    screen->depth = other ? other->depth : 24;

    screen->metamodes = metamode;
    screen->num_metamodes = 1;
    screen->cur_metamode = metamode;
    screen->cur_metamode_idx = 0;

    /* Compute the right-most screen */
    for (other = layout->screens; other; other = other->next_in_layout) {
        if (!rightmost ||
            ((other->dim.x + other->dim.width) >
             (rightmost->dim.x + rightmost->dim.width))) {
            rightmost = other;
        }
    }

    /* Make the screen right-of the right-most screen */
    if (rightmost) {
        screen->position_type = CONF_ADJ_RIGHTOF;
        screen->relative_to = rightmost;
        screen->dim.x = mode->pan.x = rightmost->dim.x;
        screen->dim.y = mode->pan.y = rightmost->dim.y;

    } else {
        screen->position_type = CONF_ADJ_ABSOLUTE;
        screen->relative_to = NULL;
        screen->dim.x = mode->pan.x;
        screen->dim.y = mode->pan.y;
    }


    /* Add the screen at the end of the layout's screen list */
    layout_add_screen(layout, screen);

    /* We can't dynamically add new X screens */
    ctk_object->apply_possible = FALSE;
}



/** do_enable_display_on_xscreen() ***********************************
 *
 * Adds the display device to an existing X screen.
 *
 * Handles the "Disabled -> Existing X screen" transition.
 *
 **/

static void do_enable_display_on_xscreen(CtkDisplayConfig *ctk_object,
                                         nvDisplayPtr display,
                                         nvScreenPtr screen)
{
    nvMetaModePtr metamode;
    nvModePtr mode;
    int max_displays = get_screen_max_displays(screen);


    /* Make sure we're allowed to enable this display */
    if (max_displays >= 0 && screen->num_displays > max_displays) {
        return;
    }

    /* Inject the display (create modes) into all the existing metamodes */
    display_remove_modes(display);

    for (metamode = screen->metamodes; metamode; metamode = metamode->next) {

        nvDisplayPtr other;
        nvModePtr rightmost = NULL;


        /* Get the right-most mode of the metamode */
        for (other = screen->displays; other; other = other->next_in_screen) {
            for (mode = other->modes; mode; mode = mode->next) {
                if (!rightmost ||
                    ((mode->pan.x + mode->pan.width) >
                     (rightmost->pan.x + rightmost->pan.width))) {
                    rightmost = mode;
                }
            }
        }


        /* Create the nvidia-auto-select mode for the display */
        mode = mode_parse(display, "nvidia-auto-select");
        mode->metamode = metamode;


        /* Set the currently selected mode */
        if (metamode == screen->cur_metamode) {
            display->cur_mode = mode;
        }


        /* Position the new mode to the right of the right-most metamode */
        if (rightmost) {
            mode->position_type = CONF_ADJ_RIGHTOF;
            mode->relative_to = rightmost->display;
            mode->pan.x = rightmost->display->cur_mode->pan.x;
            mode->pan.y = rightmost->display->cur_mode->pan.y;
        } else {
            mode->position_type = CONF_ADJ_ABSOLUTE;
            mode->relative_to = NULL;
            mode->pan.x = metamode->dim.x + metamode->dim.width;
            mode->pan.y = metamode->dim.y;
        }


        /* Add the mode at the end of the display's mode list */
        xconfigAddListItem((GenericListPtr *)(&display->modes),
                           (GenericListPtr)mode);
        display->num_modes++;
    }


    /* Link the screen and display together */
    screen_link_display(screen, display);
}



/** do_configure_display_on_new_xscreen() ****************************
 *
 * Configures the display's GPU for Multiple X screens.
 *
 * Handles the "X screen -> New X screen" transition.
 *
 **/

static void do_configure_display_on_new_xscreen(CtkDisplayConfig *ctk_object,
                                                nvDisplayPtr display)
{
    ctk_display_layout_disable_display(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                       display);

    do_enable_display_on_new_xscreen(ctk_object, display);
}



/** do_configure_display_on_xscreen() ********************************
 *
 * Moves the display from it's current screen to the new given X
 * screen.
 *
 **/

static void do_configure_display_on_xscreen(CtkDisplayConfig *ctk_object,
                                            nvDisplayPtr display,
                                            nvScreenPtr use_screen)
{
    if (display->screen == use_screen) {
        return;
    }

    ctk_display_layout_disable_display(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                       display);

    do_enable_display_on_xscreen(ctk_object, display, use_screen);
}



/** do_query_remove_display() ****************************************
 *
 * Asks the user about removing a display device from the layout.
 *
 **/

static gboolean do_query_remove_display(CtkDisplayConfig *ctk_object,
                                        nvDisplayPtr display)
{
    gint result;


    /* Show the display disable dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_display_disable),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_show_all(ctk_object->dlg_display_disable);
    gtk_widget_grab_focus(ctk_object->btn_display_disable_cancel);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_display_disable));
    gtk_widget_hide(ctk_object->dlg_display_disable);
    
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        return True;
        
    case GTK_RESPONSE_CANCEL:
    default:
        /* Cancel */
        return False;
    }

    return False;

} /* do_query_remove_display() */



/** do_disable_display() *********************************************
 *
 * Confirms disabling of the display device.
 *
 **/

static void do_disable_display(CtkDisplayConfig *ctk_object,
                               nvDisplayPtr display)
{
    nvGpuPtr gpu = display->gpu;
    gchar *str;


    /* Setup the remove display dialog */
    if (ctk_object->advanced_mode) {
        str = g_strdup_printf("Disable the display device %s (%s) "
                              "on GPU-%d (%s)?",
                              display->logName, display->typeIdName,
                              NvCtrlGetTargetId(gpu->ctrl_target),
                              gpu->name);
    } else {
        str = g_strdup_printf("Disable the display device %s (%s)?",
                              display->logName, display->typeIdName);
    }

    gtk_label_set_text
        (GTK_LABEL(ctk_object->txt_display_disable), str);
    g_free(str);

    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_display_disable_off),
                         "Disable");
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_display_disable_cancel),
                         "Cancel");

    /* Confirm with user before disabling */
    if (do_query_remove_display(ctk_object, display)) {
        gboolean screen_disabled =
            (display->screen->num_displays == 1) ? TRUE : FALSE;
        ctk_display_layout_disable_display(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                           display);
        /* If the display was the last one on the X screen, make note that we
         * can't actually remove the X screen without a restart.
         */
        if (screen_disabled) {
            ctk_object->apply_possible = FALSE;
        }
    }

} /* do_disable_display() */



static Bool display_build_modepool(nvDisplayPtr display, Bool *updated)
{
    if (!display->modelines) {
        char *tokens = NULL;
        gchar *err_str = NULL;

        NvCtrlStringOperation(display->ctrl_target, 0,
                              NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,
                              "", &tokens);
        free(tokens);
        *updated = TRUE;
        if (!display_add_modelines_from_server(display, display->gpu,
                                               &err_str)) {
            nv_warning_msg("%s", err_str);
            g_free(err_str);
            return FALSE;
        }
    }

    return display->modelines ? TRUE : FALSE;
}



static void do_enable_mosaic(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr mosaic_screen;

    /* Pick first X screen as mosaic X screen */

    mosaic_screen = layout->screens;

    /* Consolidate all GPUs */

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        if (!screen_has_gpu(mosaic_screen, gpu)) {
            link_screen_to_gpu(mosaic_screen, gpu);
        }

        gpu->mosaic_enabled = TRUE;
    }

    /* Consolidate all enabled displays */

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        nvDisplayPtr display;

        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->screen &&
                display->screen != mosaic_screen) {
                do_configure_display_on_xscreen(ctk_object, display,
                                                mosaic_screen);

                /*
                 * The display has been added to the rightmost edge of the
                 * mosaic screen with relative positioning. Update the layout
                 * to set the absolute position, so the next iteration of this
                 * loop can add the display to the new rightmost edge.
                 */
                ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
            }
        }
    }
}



static void do_disable_mosaic(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr mosaic_screen;

    /* Track the original Mosaic X screen */
    mosaic_screen = layout->screens;

    /* Disable Mosaic on all GPUs, and move the enabled displays that are not
     * on the display owner GPU to their own X screen
     */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {

        gpu->mosaic_enabled = FALSE;

        if (gpu != mosaic_screen->display_owner_gpu) {
            nvDisplayPtr display;
            for (display = gpu->displays;
                 display;
                 display = display->next_on_gpu) {
                if (!display->screen) {
                    continue;
                }
                do_configure_display_on_new_xscreen(ctk_object, display);

                /*
                 * The new X screen has been set to the rightmost edge of
                 * the rightmost X screen with relative positioning. Update
                 * the layout to set the absolute position, so the next
                 * iteration of this loop can add the display to the new
                 * rightmost edge.
                 */
                ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
            }
        }
    }

    /* Re-link the original screen to the GPU (unlinks all other gpus from the
     * screen.)
     */
    mosaic_screen->num_gpus = 0;
    link_screen_to_gpu(mosaic_screen, mosaic_screen->display_owner_gpu);
}



static void mosaic_state_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gboolean enabled;


    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* Can't dynamically toggle Mosaic */
    ctk_object->apply_possible = FALSE;

    if (enabled) {
        do_enable_mosaic(ctk_object);
    } else {
        do_disable_mosaic(ctk_object);
    }

    /* Update the GUI */
    ctk_display_layout_update_zorder(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    update_gui(ctk_object);

    user_changed_attributes(ctk_object);
}



/** display_config_changed() *****************************************
 *
 * Called when user selects an option in the display configuration menu.
 *
 **/

static void display_config_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvLayoutPtr layout = ctk_object->layout;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
                            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gboolean update = FALSE;
    nvScreenPtr screen;

    gint table_idx;
    DisplayConfigOption *option;

    if (!display) {
        return;
    }

    table_idx =
        gtk_combo_box_get_active(GTK_COMBO_BOX(ctk_object->mnu_display_config));
    option = &(ctk_object->display_config_table[table_idx]);

    switch (option->config) {

    case DPY_CFG_DISABLED:
        if (display->screen) {
            do_disable_display(ctk_object, display);
            update = TRUE;
        }
        break;

    case DPY_CFG_NEW_X_SCREEN:
        if (!display_build_modepool(display, &update)) {
            return;
        }
        if (!display->screen) {
            do_enable_display_on_new_xscreen(ctk_object, display);
        } else {
            do_configure_display_on_new_xscreen(ctk_object, display);
        }
        update = TRUE;
        break;

    case DPY_CFG_X_SCREEN:
        if (display->screen == option->screen) {
            return;
        }
        if (!display_build_modepool(display, &update)) {
            return;
        }
        if (!display->screen) {
            do_enable_display_on_xscreen(ctk_object, display, option->screen);
        } else {
            do_configure_display_on_xscreen(ctk_object, display,
                                            option->screen);
        }
        update = TRUE;
        break;
    }


    /* Sync the GUI */
    if (update) {

        /* Update the z-order */
        ctk_display_layout_update_zorder(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

        /* Recalculate */
        ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

        /* Auto fix all screens on the gpu */
        for (screen = layout->screens;
             screen;
             screen = screen->next_in_layout) {
            if (!screen_has_gpu(screen, display->gpu)) {
                continue;
            }
            validation_auto_fix_screen(ctk_object, screen);
        }

        /* Final update */
        ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

        update_gui(ctk_object);

        user_changed_attributes(ctk_object);
    }

} /* display_config_changed() */



/** display_refresh_changed() ****************************************
 *
 * Called when user selects a new refresh rate for a display.
 *
 **/

static void display_refresh_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx;
    nvModeLinePtr modeline;
    nvDisplayPtr display;
    Rotation old_rotation;
    Reflection old_reflection;


    /* Get the modeline and display to set */
    idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    modeline = ctk_object->refresh_table[idx];
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* Save the current rotation and reflection settings */
    old_rotation = display->cur_mode->rotation;
    old_reflection = display->cur_mode->reflection;

    /* In Basic view, we assume the user most likely wants
     * to change which metamode is being used.
     */
    if (!ctk_object->advanced_mode && (display->screen->num_displays == 1)) {
        int metamode_idx =
            display_find_closest_mode_matching_modeline(display, modeline);

        /* Select the new metamode */
        if (metamode_idx >= 0) {
            ctk_display_layout_set_screen_metamode
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 display->screen, metamode_idx);
        }
    }
     

    /* Update the display's currently selected mode */
    ctk_display_layout_set_mode_modeline
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display->cur_mode, modeline,
         &display->cur_mode->viewPortIn,
         &display->cur_mode->viewPortOut);

    /* If we are in Basic mode, apply the rotation and reflection settings from
     * the previous mode to the new mode.
     */
    if (!ctk_object->advanced_mode) {

        if (display->cur_mode->rotation != old_rotation) {
            ctk_display_layout_set_display_rotation(
                CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                display, old_rotation);
        }

        if (display->cur_mode->reflection != old_reflection) {
            ctk_display_layout_set_display_reflection(
                CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                display, old_reflection);
        }
    }

    /* Update the modename */
    setup_display_modename(ctk_object);

    user_changed_attributes(ctk_object);

} /* display_refresh_changed() */



/** display_resolution_changed() *************************************
 *
 * Called when user selects a new resolution for a display device.
 *
 **/

static void display_resolution_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx;
    gint last_idx;
    nvSelectedModePtr selected_mode;
    nvDisplayPtr display;
    Rotation old_rotation;
    Reflection old_reflection;


    /* Get the modeline and display to set */
    idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    selected_mode = ctk_object->resolution_table[idx];
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* cache the selected index */
    last_idx = ctk_object->last_resolution_idx;
    ctk_object->last_resolution_idx = idx;

    /* Ignore selecting same resolution */
    if (idx == last_idx) {
        return;
    }

    /* Save the current rotation and reflection settings */
    old_rotation = display->cur_mode->rotation;
    old_reflection = display->cur_mode->reflection;

    /* In Basic view, we assume the user most likely wants
     * to change which metamode is being used.
     */
    if (!ctk_object->advanced_mode && (display->screen->num_displays == 1) &&
        display->screen->num_prime_displays == 0) {
        int metamode_idx =
            display_find_closest_mode_matching_modeline(display,
                                                        selected_mode->modeline);

        /* Select the new metamode */
        if (metamode_idx >= 0) {
            ctk_display_layout_set_screen_metamode
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 display->screen, metamode_idx);
        }
    }

    /* Select the new modeline for its resolution */
    if (selected_mode->isScaled) {
        ctk_display_layout_set_mode_modeline
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             display->cur_mode,
             selected_mode->modeline,
             &selected_mode->viewPortIn,
             &selected_mode->viewPortOut);
    } else {
        ctk_display_layout_set_mode_modeline
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             display->cur_mode,
             selected_mode->modeline,
             NULL /* viewPortIn */,
             NULL /* viewPortOut */);
    }

    /* If we are in Basic mode, apply the rotation and reflection settings from
     * the previous mode to the new mode.
     */
    if (!ctk_object->advanced_mode) {

        if (display->cur_mode->rotation != old_rotation) {
            ctk_display_layout_set_display_rotation(
                CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                display, old_rotation);
        }

        if (display->cur_mode->reflection != old_reflection) {
            ctk_display_layout_set_display_reflection(
                CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                display, old_reflection);
        }
    }

    /* Update the UI */
    setup_display_page(ctk_object);

    user_changed_attributes(ctk_object);

} /* display_resolution_changed() */



/** display_stereo_changed() *****************************
 *
 * Called when user selects a new passive stereo eye
 * configuration.
 *
 **/
static void display_stereo_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    nvModePtr mode;
    gint idx;

    /* Update the current mode on the selected display */
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (display && display->cur_mode) {
        mode = display->cur_mode;

        idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
        switch (idx) {
        case 1:
            mode->passive_stereo_eye = PASSIVE_STEREO_EYE_LEFT;
            break;
        case 2:
            mode->passive_stereo_eye = PASSIVE_STEREO_EYE_RIGHT;
            break;
        default:
        case 0:
            mode->passive_stereo_eye = PASSIVE_STEREO_EYE_NONE;
            break;
        }
    }

    user_changed_attributes(ctk_object);

} /* display_stereo_changed() */



/** display_rotation_changed() ***************************************
 *
 * Called when user selects a new rotation orientation.
 *
 **/
static void display_rotation_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    gint idx;
    Rotation rotation;


    /* Update the current mode on the selected display */
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display ||
        !display->cur_mode ||
        !display->cur_mode->modeline) {
        return;
    }

    idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    switch (idx) {
    case 1:
        rotation = ROTATION_90;
        break;
    case 2:
        rotation = ROTATION_180;
        break;
    case 3:
        rotation = ROTATION_270;
        break;
    default:
    case 0:
        rotation = ROTATION_0;
        break;
    }

    ctk_display_layout_set_display_rotation
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display, rotation);

} /* display_rotation_changed() */



/** display_reflection_changed() *************************************
 *
 * Called when user selects a new reflection axis.
 *
 **/
static void display_reflection_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    gint idx;
    Reflection reflection;

    /* Update the current mode on the selected display */
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display ||
        !display->cur_mode ||
        !display->cur_mode->modeline) {
        return;
    }

    idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    switch (idx) {
    case 1:
        reflection = REFLECTION_X;
        break;
    case 2:
        reflection = REFLECTION_Y;
        break;
    case 3:
        reflection = REFLECTION_XY;
        break;
    default:
    case 0:
        reflection = REFLECTION_NONE;
        break;
    }

    ctk_display_layout_set_display_reflection
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display, reflection);

} /* display_reflection_changed() */



/** post_display_underscan_value_changed() ****************************
 *
 * Modifies the ViewPortOut of the current mode according to the value
 * of the Underscan slider.
 *
 **/
static void post_display_underscan_value_changed(CtkDisplayConfig *ctk_object,
                                                 const int hpixel_value)
{
    CtkDisplayLayout *ctk_display;
    nvDisplayPtr display;
    nvModePtr cur_mode;
    nvSize raster_size;
    GdkRectangle rotatedViewPortIn;

    ctk_display = CTK_DISPLAY_LAYOUT(ctk_object->obj_layout);
    display = ctk_display_layout_get_selected_display(ctk_display);

    cur_mode = display->cur_mode;

    if (!cur_mode || !cur_mode->modeline) {
        return;
    }

    raster_size.height = cur_mode->modeline->data.vdisplay;
    raster_size.width = cur_mode->modeline->data.hdisplay;

    /* Update ViewPortOut, ViewPortIn and panning. Erase previous data */
    apply_underscan_to_viewportout(raster_size, hpixel_value,
                                   &cur_mode->viewPortOut);

    if (cur_mode->rotation == ROTATION_90 ||
        cur_mode->rotation == ROTATION_270)
    {
        rotatedViewPortIn.width = cur_mode->viewPortOut.height;
        rotatedViewPortIn.height = cur_mode->viewPortOut.width;
    } else {
        rotatedViewPortIn.width = cur_mode->viewPortOut.width;
        rotatedViewPortIn.height = cur_mode->viewPortOut.height;
    }

    ctk_display_layout_set_mode_viewport_in(ctk_display,
                                            cur_mode,
                                            rotatedViewPortIn.width,
                                            rotatedViewPortIn.height,
                                            TRUE /* update_panning_size */);

    /* Enable the apply button */
    update_btn_apply(ctk_object, TRUE);

}



/** display_underscan_value_changed() *********************************
 *
 * Called when user modifies the value of the Underscan slider.
 *
 **/
static void display_underscan_value_changed(GtkAdjustment *adjustment,
                                           gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    nvModePtr cur_mode;
    int hpixel_value;
    gfloat value;
    gchar *txt_entry;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display) {
        return;
    }

    cur_mode = display->cur_mode;

    if (!cur_mode || !cur_mode->modeline) {
        return;
    }

    value = (gfloat) gtk_adjustment_get_value(adjustment);
    hpixel_value = cur_mode->modeline->data.hdisplay * (value / 100);

    txt_entry = g_strdup_printf("%d", hpixel_value);
    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_underscan), txt_entry);
    g_free(txt_entry);

    post_display_underscan_value_changed(ctk_object, hpixel_value);

}



/** display_underscan_activate() **************************************
 *
 * Called when user modifies the display Underscan text entry.
 *
 * This then calls display_underscan_value_changed() by
 * modifying the value of the Underscan slider.
 *
 **/
static void display_underscan_activate(GtkWidget *widget,
                                       gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *txt_entry = gtk_entry_get_text(GTK_ENTRY(widget));
    nvDisplayPtr display;
    nvModePtr cur_mode;
    int hdisplay, hpixel_value;
    gfloat adj_value;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display) {
        return;
    }

    cur_mode = display->cur_mode;

    if (!cur_mode || !cur_mode->modeline) {
        return;
    }

    parse_read_integer(txt_entry, &hpixel_value);

    hdisplay = cur_mode->modeline->data.hdisplay;
    adj_value = ((gfloat) hpixel_value / hdisplay) * 100;

    /* Sanitize adjustment value */
    adj_value = NV_MIN(adj_value, UNDERSCAN_MAX_PERCENT);
    adj_value = NV_MAX(adj_value, UNDERSCAN_MIN_PERCENT);

    /* This sends a value_changed signal to the adjustment object */
    gtk_adjustment_set_value(GTK_ADJUSTMENT(ctk_object->adj_display_underscan),
                             adj_value);
}



/** display_position_type_changed() **********************************
 *
 * Called when user selects a new display position method (relative/
 * absolute)
 *
 **/

static void display_position_type_changed(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    gint position_idx;
    int position_type;
    gint relative_to_idx;
    nvDisplayPtr relative_to;


    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* Get the new position type */
    position_idx =
        gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx =
        gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->display_position_table_len) {

        relative_to =
            ctk_object->display_position_table[relative_to_idx];

        /* Update the layout */
        ctk_display_layout_set_display_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             display, position_type, relative_to,
             display->cur_mode->pan.x,
             display->cur_mode->pan.y);
    }


    /* Cannot apply if the screen position changed */
    check_screen_pos_changed(ctk_object);


    /* Update GUI */
    setup_display_position_relative(ctk_object);

    setup_display_position_offset(ctk_object);

    user_changed_attributes(ctk_object);

} /* display_position_type_changed() */



/** display_position_relative_changed() ******************************
 *
 * Called when user selects a new display to be positioned relative
 * to.
 *
 **/

static void display_position_relative_changed(GtkWidget *widget,
                                              gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    gint position_idx;
    gint relative_to_idx;
    int position_type;
    nvDisplayPtr relative_to;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* Get the new display to be relative to */
    position_idx = gtk_combo_box_get_active
                    (GTK_COMBO_BOX(ctk_object->mnu_display_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx = gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_display_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->display_position_table_len) {

        relative_to = ctk_object->display_position_table[relative_to_idx];

        /* Update the relative position */
        ctk_display_layout_set_display_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             display, position_type, relative_to, 0, 0);
    }


    /* Cannot apply if we change the relative position */
    check_screen_pos_changed(ctk_object);


    /* Update the GUI */
    setup_display_position_offset(ctk_object);

    user_changed_attributes(ctk_object);

} /* display_position_relative_changed() */



/** display_position_offset_activate() *******************************
 *
 * Called when user modifies the display position offset text entry.
 *
 **/

static void display_position_offset_activate(GtkWidget *widget,
                                             gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int x, y;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display) return;
    
    /* Parse user input */
    str = parse_read_integer_pair(str, 0, &x, &y);
    if (!str) {
        /* Reset the display position */
        setup_display_position_offset(ctk_object);
        return;
    }

    /* Make coordinates relative to top left of Screen */
    x += display->cur_mode->metamode->edim.x;
    y += display->cur_mode->metamode->edim.y;


    /* Update the absolute position */
    ctk_display_layout_set_display_position
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display, CONF_ADJ_ABSOLUTE, NULL, x, y);

    user_changed_attributes(ctk_object);

} /* display_position_offset_activate() */



/** display_viewport_in_activate() ***********************************
 *
 * Called when user modifies the display ViewPortIn text entry.
 *
 **/

static void display_viewport_in_activate(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int w, h;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display || !display->cur_mode) {
        return;
    }

    str = parse_read_integer_pair(str, 'x', &w, &h);
    if (!str) {
        /* Reset the mode's ViewPortIn */
        setup_display_viewport_in(ctk_object);
        return;
    }

    ctk_display_layout_set_mode_viewport_in
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), display->cur_mode, w, h,
         FALSE /* update_panning_size */);

} /* display_viewport_in_activate() */



/** display_viewport_out_activate() **********************************
 *
 * Called when user modifies the display ViewPortOut text entry.
 *
 **/

static void display_viewport_out_activate(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int w, h, x, y;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display || !display->cur_mode) {
        return;
    }

    str = parse_read_integer_pair(str, 'x', &w, &h);
    if (!str) {
        /* Reset the mode's ViewPortOut */
        setup_display_viewport_out(ctk_object);
        return;
    }
    str = parse_read_integer_pair(str, 0, &x, &y);
    if (!str) {
        /* Reset the mode's ViewPortOut */
        setup_display_viewport_out(ctk_object);
        return;
    }

    ctk_display_layout_set_mode_viewport_out
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), display->cur_mode,
         x, y, w, h);

} /* display_viewport_out_activate() */



/** display_panning_activate() ***************************************
 *
 * Called when user modifies the display position text entry.
 *
 **/

static void display_panning_activate(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int x, y;
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!display) {
        return;
    }

    str = parse_read_integer_pair(str, 'x', &x, &y);
    if (!str) {
        /* Reset the display panning */
        setup_display_panning(ctk_object);
        return;
    }

    ctk_display_layout_set_display_panning
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), display, x, y);

} /* display_panning_activate() */



/** screen_virtual_size_activate() ***********************************
 *
 * Called when user modifies the screen virtual size text entry.
 *
 **/

static void screen_virtual_size_activate(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int x, y;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!screen || !screen->no_scanout) {
        return;
    }
    
    str = parse_read_integer_pair(str, 'x', &x, &y);
    if (!str) {
        /* Reset the display panning */
        setup_screen_virtual_size(ctk_object);
        return;
    }

    ctk_display_layout_set_screen_virtual_size
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, x, y);

    setup_screen_virtual_size(ctk_object);

} /* screen_virtual_size_activate() */



/** txt_focus_out() **************************************************
 *
 * Called when user leaves a txt entry
 *
 **/

static gboolean txt_focus_out(GtkWidget *widget, GdkEvent *event,
                              gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);

    if (widget == ctk_object->txt_display_viewport_in) {
        display_viewport_in_activate(widget, user_data);
    } else if (widget == ctk_object->txt_display_viewport_out) {
        display_viewport_out_activate(widget, user_data);
    } else if (widget == ctk_object->txt_display_panning) {
        display_panning_activate(widget, user_data);
    } else if (widget == ctk_object->txt_screen_virtual_size) {
        screen_virtual_size_activate(widget, user_data);
    }

    return FALSE;

} /* txt_focus_out() */



/** screen_depth_changed() *******************************************
 *
 * Called when user selects a new color depth for a screen.
 *
 **/

static void screen_depth_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    int depth;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

    if (idx >= ctk_object->screen_depth_table_len) return;

    depth = ctk_object->screen_depth_table[idx];

    if (depth == 30) {
        GtkWidget *dlg;
        GtkWidget *parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));
        dlg = gtk_message_dialog_new (GTK_WINDOW(parent),
                                      GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_OK,
                                      "Note that Depth 30 requires recent X "
                                      "server updates for correct operation.  "
                                      "Also, some X applications may not work "
                                      "correctly with depth 30.\n\n"
                                      "Please see the Chapter \"Configuring "
                                      "Depth 30 Displays\" "
                                      "in the README for details.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy (dlg);

    }
    ctk_display_layout_set_screen_depth
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, depth);
    /* Update default screen depth in SMF using libscf functions */
    update_scf_depth(depth);

    consolidate_xinerama(ctk_object, screen);

    /* Can't apply screen depth changes */
    ctk_object->apply_possible = FALSE;

    user_changed_attributes(ctk_object);

} /* screen_depth_changed() */



/** screen_stereo_changed() ******************************************
 *
 * Called when user selects a new stereo mode for a screen.
 *
 **/

static void screen_stereo_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

    screen->stereo = idx;

    if (idx >= 0 && idx < ctk_object->stereo_table_size) {

        /* translation from dropdown index to stereo mode */
        screen->stereo = ctk_object->stereo_table[idx];
    }

    /* Can't apply screen stereo changes */
    ctk_object->apply_possible = FALSE;

    user_changed_attributes(ctk_object);


    /* Changing this can modify how the display page looks */
    setup_display_page(ctk_object);

} /* screen_stereo_changed() */



/** screen_position_type_changed() ***********************************
 *
 * Called when user selects a new screen position method (relative/
 * absolute)
 *
 **/

static void screen_position_type_changed(GtkWidget *widget,
                                         gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gint position_idx;
    int position_type;
    gint relative_to_idx;
    nvScreenPtr relative_to;

    if (!screen) return;

    /* Get the new position type */
    position_idx =
        gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx =
        gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->screen_position_table_len) {
        
        relative_to =
            ctk_object->screen_position_table[relative_to_idx];

        /* Update the layout */
        ctk_display_layout_set_screen_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             screen, position_type, relative_to,
             screen->dim.x,
             screen->dim.y);
    }


    /* Cannot apply changes to screen positioning */
    ctk_object->apply_possible = FALSE;


    /* Update the GUI */
    setup_screen_position_relative(ctk_object);

    setup_screen_position_offset(ctk_object);

    user_changed_attributes(ctk_object);

} /* screen_position_type_changed() */



/** screen_position_relative_changed() *******************************
 *
 * Called when user selects a new screen to be positioned relative
 * to.
 *
 **/

static void screen_position_relative_changed(GtkWidget *widget,
                                             gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gint position_idx;
    gint relative_to_idx;
    int position_type;
    nvScreenPtr relative_to;

    if (!screen) return;

    /* Get the new X screen to be relative to */
    position_idx = gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx = gtk_combo_box_get_active
        (GTK_COMBO_BOX(ctk_object->mnu_screen_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->screen_position_table_len) {

        relative_to = ctk_object->screen_position_table[relative_to_idx];

        /* Update the relative position */
        ctk_display_layout_set_screen_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             screen, position_type, relative_to, 0, 0);
    }


    /* Cannot apply changes to screen positioning */
    ctk_object->apply_possible = FALSE;


    /* Update the GUI */
    setup_screen_position_offset(ctk_object);

    user_changed_attributes(ctk_object);

} /* screen_position_relative_changed() */



/** screen_position_offset_activate() ********************************
 *
 * Called when user modifies the screen position offset text entry.
 *
 **/

static void screen_position_offset_activate(GtkWidget *widget,
                                            gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));
    int x, y;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;
    

    /* Parse user input */
    str = parse_read_integer_pair(str, 0, &x, &y);
    if (!str) {
        /* Reset the display position */
        setup_screen_position_offset(ctk_object);
        return;
    }


    /* Cannot apply changes to screen positioning */
    ctk_object->apply_possible = FALSE;


    /* Update the absolute position */
    ctk_display_layout_set_screen_position
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         screen, screen->position_type, screen->relative_to, x, y);

    user_changed_attributes(ctk_object);

} /* screen_position_offset_activate() */



/** screen_metamode_clicked() ****************************************
 *
 * Called when user selects a new metamode for the selected screen
 *
 **/

static void screen_metamode_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    GtkWidget *menu;
    GtkWidget *menu_item;
    int i;
    gchar *str;
    gchar *tmp;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

    /* Generate the popup menu */
    menu = gtk_menu_new();
    for (i = 0; i < screen->num_metamodes; i++) {
        
        /* Setup the menu item text */
        tmp = screen_get_metamode_str(screen, i, 0);
        str = g_strdup_printf("%d - \"%s\"", i+1, tmp);
        menu_item = gtk_menu_item_new_with_label(str);
        g_free(str);
        g_free(tmp);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        g_signal_connect(G_OBJECT(menu_item),
                         "activate",
                         G_CALLBACK(screen_metamode_activate),
                         (gpointer) ctk_object);
    }
    
    /* Show the popup menu */
    gtk_menu_popup(GTK_MENU(menu),
                   NULL, NULL, NULL, NULL,
                   1, gtk_get_current_event_time());

} /* screen_metamode_clicked() */



/** screen_metamode_activate() ***************************************
 *
 * Called when user selects a new metamode for the selected screen
 *
 **/

static void screen_metamode_activate(GtkWidget *widget, gpointer user_data)
{
    GtkMenuItem *item = (GtkMenuItem *) widget;
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    const gchar *str =
        gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))));
    int idx;
    gchar *name;
   
    if (!screen || !str) return;

    idx = atoi(str) -1;

    name = g_strdup_printf("%d - ...", idx+1);
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_screen_metamode), name);
    g_free(name);


    ctk_display_layout_set_screen_metamode
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, idx);


    /* Sync the display frame */
    setup_display_page(ctk_object);

    user_changed_attributes(ctk_object);

} /* screen_metamode_activate() */



/** screen_metamode_add_clicked() ************************************
 *
 * Called when user clicks on the display's "Add" metamode button.
 *
 **/

static void screen_metamode_add_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    
    if (!screen) return;


    /* Add a new metamode to the screen */
    ctk_display_layout_add_screen_metamode
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen);


    /* Update the GUI */
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

    user_changed_attributes(ctk_object);

} /* screen_metamode_add_clicked() */



/** screen_metamode_delete_clicked() *********************************
 *
 * Called when user clicks on the display's "Delete" metamode button.
 *
 **/

static void screen_metamode_delete_clicked(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

    
    ctk_display_layout_delete_screen_metamode
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         screen, screen->cur_metamode_idx, TRUE);

    /* Update the GUI */
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

    user_changed_attributes(ctk_object);

} /* screen_metamode_delete_clicked() */



/** xinerama_state_toggled() *****************************************
 *
 * Called when user toggles the state of the "Enable Xinerama"
 * button.
 *
 **/

static void xinerama_state_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);

    ctk_object->layout->xinerama_enabled =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    /* Can't dynamically enable Xinerama */
    ctk_object->apply_possible = FALSE;

    /* Make sure all screens have the same depth when Xinerama is enabled */
    consolidate_xinerama(ctk_object, NULL);
    setup_screen_page(ctk_object);

    user_changed_attributes(ctk_object);
    
} /* xinerama_state_toggled() */



/** update_display_confirm_text() ************************************
 *
 * Generates the text used in the confirmation dialog.
 *
 **/

static void update_display_confirm_text(CtkDisplayConfig *ctk_object,
                                        int screen)
{
    gchar *str;
    str = g_strdup_printf("The mode on X screen %d has been set.\n"
                          "Would you like to keep the current settings?\n\n"
                          "Reverting in %d seconds...",
                          screen, ctk_object->display_confirm_countdown);
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_confirm), str);
    g_free(str);

} /* update_display_confirm_text() */



/** do_display_confirm_countdown() ***********************************
 *
 * timeout callback for reverting a modeline setting.
 *
 **/

static gboolean do_display_confirm_countdown(gpointer data)
{
    SwitchModeCallbackInfo *info = (SwitchModeCallbackInfo *) data;
    CtkDisplayConfig *ctk_object = info->ctk_object;
    int screen = info->screen;

    ctk_object->display_confirm_countdown--;
    if (ctk_object->display_confirm_countdown > 0) {
        update_display_confirm_text(ctk_object, screen);
        return True;
    }

    /* Force dialog to cancel */
    gtk_dialog_response(GTK_DIALOG(ctk_object->dlg_display_confirm),
                        GTK_RESPONSE_REJECT);

    return False;

} /* do_display_confirm_countdown() */



/** switch_to_current_metamode() *************************************
 *
 * Switches to the current screen metamode using NV_CTRL_CURRENT_METAMODE_ID
 *
 **/

static Bool switch_to_current_metamode(CtkDisplayConfig *ctk_object,
                                       nvScreenPtr screen,
                                       const char *cur_metamode_str)
{
    ReturnStatus ret;
    gint result;
    nvMetaModePtr metamode;
    int new_width;
    int new_height;
    int new_rate;
    int old_rate;
    static SwitchModeCallbackInfo info;
    GtkWidget *dlg;
    GtkWidget *parent;
    gchar *msg;
    Bool modified_current_metamode;


    if (screen->ctrl_target == NULL ||
        screen->cur_metamode == NULL) {
        goto fail;
    }

    metamode = screen->cur_metamode;

    new_width = metamode->edim.width;
    new_height = metamode->edim.height;
    new_rate = metamode->id;


    /* Find the parent window for displaying dialogs */

    parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));
    if (!parent) goto fail;


    /* Get the current mode so we can fall back on that if the
     * mode switch fails, or the user does not confirm.
     */

    ret = NvCtrlGetAttribute(screen->ctrl_target,
                             NV_CTRL_CURRENT_METAMODE_ID,
                             (int *)&old_rate);
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to get current (fallback) mode for "
                       "display device!");
        goto fail;
    }

    nv_info_msg(TAB, "Current mode (id: %d)", old_rate);
    nv_info_msg(TAB, "Current mode string: %s", cur_metamode_str);


    /* Switch to the new mode */

    if (new_rate > 0 ) {
        nv_info_msg(TAB, "Switching to mode: %dx%d (id: %d)...",
                    new_width, new_height, new_rate);

        ret = NvCtrlSetAttribute(screen->ctrl_target,
                                 NV_CTRL_CURRENT_METAMODE_ID,
                                 new_rate);
        modified_current_metamode = FALSE;
    } else {
        nv_info_msg(TAB, "Modifying current MetaMode to: %s...",
                    metamode->cpl_str);

        ret = NvCtrlSetStringAttribute(screen->ctrl_target,
                                       NV_CTRL_STRING_CURRENT_METAMODE,
                                       metamode->cpl_str);
        if (ret == NvCtrlSuccess) {
            metamode->id = old_rate;
        }
        modified_current_metamode = TRUE;
    }

    if (ret != NvCtrlSuccess) {

        nv_warning_msg("Failed to set MetaMode (%d) '%s' "
                       "(Mode: %dx%d, id: %d) on X screen %d!",
                       screen->cur_metamode_idx+1, metamode->cpl_str,
                       new_width,
                       new_height, new_rate,
                       NvCtrlGetTargetId(screen->ctrl_target));

        if (screen->num_metamodes > 1) {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X screen %d\n\n"
                                  "Would you like to remove this MetaMode?",
                                  screen->cur_metamode_idx+1,
                                  metamode->cpl_str,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->ctrl_target));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_YES_NO,
                 "%s", msg);
        } else {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X screen %d.",
                                  screen->cur_metamode_idx+1,
                                  metamode->cpl_str,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->ctrl_target));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_OK,
                 "%s", msg);
        }

        result = gtk_dialog_run(GTK_DIALOG(dlg));

        switch (result) {
        case GTK_RESPONSE_YES:
            ctk_display_layout_delete_screen_metamode
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 screen, screen->cur_metamode_idx, TRUE);

            nv_info_msg(TAB, "Removed MetaMode %d on Screen %d.\n",
                        screen->cur_metamode_idx+1,
                        NvCtrlGetTargetId(screen->ctrl_target));

            /* Update the GUI */
            setup_display_page(ctk_object);
            setup_screen_page(ctk_object);
            break;
        case GTK_RESPONSE_OK:
            /* Nothing to do with last metamode */
        default:
            /* Ignore the bad metamode */
            break;
        }

        g_free(msg);
        gtk_widget_destroy(dlg);
        goto fail;
    }


    /* Setup the counter callback data */
    info.ctk_object = ctk_object;
    info.screen = NvCtrlGetTargetId(screen->ctrl_target);

    /* Start the countdown timer */
    ctk_object->display_confirm_countdown = DEFAULT_SWITCH_MODE_TIMEOUT;
    update_display_confirm_text(ctk_object, info.screen);
    ctk_object->display_confirm_timer =
        g_timeout_add(1000,
                      (GSourceFunc)do_display_confirm_countdown,
                      (gpointer)(&info));

    /* Show the confirm dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_display_confirm),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_show_all(ctk_object->dlg_display_confirm);
    gtk_widget_grab_focus(ctk_object->btn_display_apply_cancel);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_display_confirm));
    gtk_widget_hide(ctk_object->dlg_display_confirm);

    /* Kill the timer */
    g_source_remove(ctk_object->display_confirm_timer);

    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        break;

    case GTK_RESPONSE_REJECT:
    default:
        /* Fall back to previous settings */
        if (!modified_current_metamode) {
            nv_info_msg(TAB, "Switching back to mode (id: %d)...", old_rate);

            ret = NvCtrlSetAttribute(screen->ctrl_target,
                                     NV_CTRL_CURRENT_METAMODE_ID,
                                     old_rate);
        } else {
            nv_info_msg(TAB, "Re-writing previous current MetaMode to: %s...",
                        cur_metamode_str);

            ret = NvCtrlSetStringAttribute(screen->ctrl_target,
                                           NV_CTRL_STRING_CURRENT_METAMODE,
                                           cur_metamode_str);
            if (ret != NvCtrlSuccess) {
                nv_warning_msg("Failed to re-write current MetaMode (%d) to "
                               "'%s' on X screen %d!",
                               old_rate,
                               cur_metamode_str,
                               NvCtrlGetTargetId(screen->ctrl_target));
            }
        }
        goto fail;
    }

    return TRUE;

 fail:
    return FALSE;

} /* switch_to_current_metamode() */



/** link_metamode_string_by_id() *************************************
 *
 * Looks in the list of strings (metamode_strs) for a metamode with
 * the given id (defined by string 'id_str').  If found, sets the metamode
 * id and x_id appropriately.
 *
 **/

static void link_metamode_string_by_id(char *metamode_strs, int match_id,
                                       nvMetaModePtr metamode)
{
    int x_idx = 0;
    char *m;

    for (m = metamode_strs; m && strlen(m); m += strlen(m) +1) {
        char *str = strstr(m, "id=");
        if (str) {
            int id = atoi(str+3);
            if (id && (id == match_id)) {
                metamode->id = id;
                metamode->x_idx = x_idx;
                metamode->x_str_entry = m;
                return;
            }
        }
        x_idx++;
    }
}



/** add_cpl_metamode_to_X() ******************************************
 *
 * Adds the given metamode to the given X screen.
 *
 **/

static Bool add_cpl_metamode_to_X(nvScreenPtr screen, nvMetaModePtr metamode,
                                  int metamode_idx)
{
    ReturnStatus ret;
    char *tokens;

    ret = NvCtrlStringOperation(screen->ctrl_target, 0,
                                NV_CTRL_STRING_OPERATION_ADD_METAMODE,
                                metamode->cpl_str, &tokens);

    /* Grab the metamode ID from the returned tokens */
    if ((ret != NvCtrlSuccess) || !tokens) {
        nv_error_msg("Failed to add MetaMode '%s' to X for "
                     "screen %d",
                     metamode->cpl_str, screen->scrnum);
        return FALSE;
    }

    parse_token_value_pairs(tokens, apply_metamode_token,
                            metamode);
    free(tokens);

    metamode->x_idx = metamode_idx;

    nv_info_msg(TAB, "Added MetaMode   (# %d,  ID: %d) > [%s]",
                metamode_idx,
                metamode->id,
                metamode->cpl_str);

    return TRUE;
}



/** stub_metamode_str() **********************************************
 *
 * Stubs out a metamode string.
 *
 **/

static void stub_metamode_str(char *str)
{
    if (str) {
        while (*str) {
            *str = ' ';
            str++;
        }
    }
}


/** setup_metamodes_for_apply() **************************************
 *
 * Prepares the list of CPL metamodes to be applied to the X server.
 *
 **/

static void setup_metamodes_for_apply(nvScreenPtr screen,
                                      char *metamode_strs)
{
    nvMetaModePtr metamode;
    ReturnStatus ret;
    char *tmp;
    int metamode_idx;


    for (metamode = screen->metamodes, metamode_idx = 0;
         metamode;
         metamode = metamode->next, metamode_idx++) {

        metamode->id = -1;
        metamode->x_idx = -1;

        /* Get metamode string from CPL */
        metamode->cpl_str = screen_get_metamode_str(screen, metamode_idx, 1);
        if (!metamode->cpl_str) {
            continue;
        }

        /* Parse CPL string into X metamode string */
        ret = NvCtrlStringOperation(screen->ctrl_target, 0,
                                    NV_CTRL_STRING_OPERATION_PARSE_METAMODE,
                                    metamode->cpl_str, &metamode->x_str);
        if ((ret != NvCtrlSuccess) ||
            !metamode->x_str) {
            continue;
        }

        /* Identify metamode id and position in X */
       tmp = strstr(metamode->x_str, "id=");
        if (tmp) {
            int id = atoi(tmp+3);
            link_metamode_string_by_id(metamode_strs, id, metamode);
        }
    }
}



/** cleanup_metamodes_for_apply() ************************************
 *
 * Releases memory used for applying metamodes to X.
 *
 **/

static void cleanup_metamodes_for_apply(nvScreenPtr screen)
{
    nvMetaModePtr metamode;

    for (metamode = screen->metamodes;
         metamode;
         metamode = metamode->next) {

        cleanup_metamode(metamode);
    }
}



/** remove_duplicate_cpl_metamodes() *********************************
 *
 * Removes duplicate metamodes in the CPL
 *
 **/

static void remove_duplicate_cpl_metamodes(CtkDisplayConfig *ctk_object,
                                           nvScreenPtr screen)
{
    nvMetaModePtr m1;
    nvMetaModePtr m2;
    int m1_idx;
    int m1_old_idx;
    int m2_idx;

    m1 = screen->metamodes;
    m1_idx = 0;
    m1_old_idx = 0;
    while (m1) {
        Bool found = FALSE;

        if (!m1->x_str) {
            m1 = m1->next;
            m1_idx++;
            m1_old_idx++;
            continue;
        }

        for (m2 = screen->metamodes, m2_idx = 0;
             m2 != m1;
             m2 = m2->next, m2_idx++) {

            if (!m2->x_str) {
                continue;
            }

            if (strcmp(m1->x_str, m2->x_str)) {
                continue;
            }

            /* m1 and m2 are the same, delete m1 (since it comes after) */
            if (m1 == screen->cur_metamode) {
                ctk_display_layout_set_screen_metamode
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     screen, m2_idx);
            }

            m1 = m1->next;

            ctk_display_layout_delete_screen_metamode
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 screen, m1_idx, FALSE);

            nv_info_msg(TAB, "Removed MetaMode %d on Screen %d (is "
                        "duplicate of MetaMode %d)\n", m1_old_idx+1,
                        screen->scrnum,
                        m2_idx+1);

            found = TRUE;
            break;
        }

        if (!found) {
            m1 = m1->next;
            m1_idx++;
        }
        m1_old_idx++;
    }
}



/** preprocess_metamodes() *******************************************
 *
 * Does preprocess work to the metamode strings:
 *
 * - Generates the metamode strings for the screen's metamodes
 *   that will be used for creating the metamode list on the X
 *   Server.
 *
 * - Stubs out each string in the metamode_strs list that should
 *   not be deleted (it has a matching metamode in "screen".)
 *
 * - Adds new metamodes to the X server screen that are specified
 *   in "screen" but not found in metamode_strs.
 *
 **/

static void preprocess_metamodes(CtkDisplayConfig *ctk_object,
                                 nvScreenPtr screen,
                                 char *x_metamode_strs,
                                 char *cur_x_metamode_str,
                                 int num_x_metamodes,
                                 int cur_x_metamode_idx)
{
    nvMetaModePtr metamode;
    Bool cur_x_metamode_matched = FALSE;


    /* Generate metamode strings and match CPL metamodes to X */
    setup_metamodes_for_apply(screen, x_metamode_strs);

    /* Remove duplicate metamodes in CPL based on parsed string */
    remove_duplicate_cpl_metamodes(ctk_object, screen);

    /* Add metamodes from the CPL that aren't in X */
    for (metamode = screen->metamodes;
         metamode;
         metamode = metamode->next) {

        /* CPL metamode was found in X, stub out the string entry in the X
         * metamodes list so we don't delete it later.
         */
        if (metamode->x_str_entry) {
            stub_metamode_str(metamode->x_str_entry);
            /* Track if the current X metamode matched a CPL metamode */
            if (metamode->x_str_entry == cur_x_metamode_str) {
                cur_x_metamode_matched = TRUE;
            }
            continue;
        }

        /* CPL metamode was not found in X, so we should add it. */

        /* Don't add the current metamode (yet).  If the current X metamode
         * string does not get stubbed out (i.e. it does not match to another
         * CPL metamode), then it can be modify via
         * NV_CTRL_STRING_CURRENT_METAMODE instead of adding a new metamode,
         * switching to it and deleting the old one.
         */
        if (metamode == screen->cur_metamode) {
            continue;
        }

        if (add_cpl_metamode_to_X(screen, metamode, num_x_metamodes)) {
            num_x_metamodes++;
        }
    }

    /* If the currently selected CPL metamode did not match any X metamode, and
     * the current active X metamode matched to another CPL metamode, then the
     * currently selected CPL metamode will need to be added and switched to.
     */
    if (screen->cur_metamode->id < 0) {
        if (cur_x_metamode_matched) {
            if (add_cpl_metamode_to_X(screen, screen->cur_metamode,
                                      num_x_metamodes)) {
                num_x_metamodes++;
            }
        } else {
            /* Current metamode will be overridden, so stub it here so that it
             * does not get deleted later.
             */
            stub_metamode_str(cur_x_metamode_str);
            screen->cur_metamode->x_idx = cur_x_metamode_idx;
        }
    }

} /* preprocess_metamodes() */



/** screen_move_metamode() *******************************************
 *
 * Updates the X ordering of the given metamode so that it appears at
 * 'metamode_idx'.
 *
 **/

static Bool screen_move_metamode(nvScreenPtr screen, nvMetaModePtr metamode,
                                 int metamode_idx)
{
    char *update_str;
    int len;
    ReturnStatus ret;

    if (!metamode->cpl_str) {
        goto fail;
    }

    /* Append the index we want */
    len = 24 + strlen(metamode->cpl_str);
    update_str = malloc(len);
    snprintf(update_str, len, "index=%d :: %s", metamode_idx,
             metamode->cpl_str);

    ret = NvCtrlSetStringAttribute(screen->ctrl_target,
                                   NV_CTRL_STRING_MOVE_METAMODE,
                                   update_str);
    if (ret != NvCtrlSuccess) {
        goto fail;
    }

    nv_info_msg(TAB, "Moved MetaMode (id:%d from idx: %d to idx %d) > %s",
                metamode->id,
                metamode->x_idx,
                metamode_idx,
                metamode->cpl_str);

    /* We moved the metamode to position metamode_idx, so bump the
     * index of all metamodes from the new position to the old one.
     * This assumes that metamodes are always moved forward in the
     * the list and not backwards.
     */
    {
        int from_idx = metamode_idx;  // New position
        int to_idx = metamode->x_idx; // Old position
        nvMetaModePtr m;

        for (m = screen->metamodes;
             m;
             m = m->next) {
            if ((m->x_idx >= from_idx) &&
                (m->x_idx < to_idx)) {
                m->x_idx++;
            }
        }
        /* Note the new location of the metamode */
        metamode->x_idx = metamode_idx;
    }
    return TRUE;

 fail:
    nv_error_msg("Failed to move MetaMode (id:%d from idx: %d to idx %d) > %s",
                 metamode->id,
                 metamode->x_idx,
                 metamode_idx,
                 metamode->cpl_str ? metamode->cpl_str : "NULL");
    return FALSE;
}



/** order_metamodes() ************************************************
 *
 * Makes sure the metamodes are ordered properly by moving each
 * metamode to its correct location in the server's metamode list.
 *
 **/

static void order_metamodes(nvScreenPtr screen)
{
    nvMetaModePtr metamode;
    int metamode_idx;


    for (metamode = screen->metamodes, metamode_idx = 0;
         metamode;
         metamode = metamode->next, metamode_idx++) {

        /* MetaMode is already in correct spot */
        if (metamode_idx == metamode->x_idx) {
            continue;
        }

        screen_move_metamode(screen, metamode, metamode_idx);
    }

} /* order_metamodes() */



/** postprocess_metamodes() ******************************************
 *
 * Does post processing work on the metamode list:
 *
 * - Deletes any metamode left in the metamode_strs
 *
 **/

static void postprocess_metamodes(nvScreenPtr screen, char *metamode_strs)
{
    char *metamode_str, *tmp;
    const char *str;
    ReturnStatus ret;
    int idx;


    /* Delete metamodes that were not cleared out from the metamode_strs */
    for (metamode_str = metamode_strs, idx = 0;
         metamode_str && strlen(metamode_str);
         metamode_str += strlen(metamode_str) +1, idx++) {

        /* Skip tokens */
        str = strstr(metamode_str, "::");
        if (!str) continue;

        str = parse_skip_whitespace(str +2);
        tmp = strdup(str);
        if (!tmp) continue;

        /* Delete the metamode */
        ret = NvCtrlSetStringAttribute(screen->ctrl_target,
                                       NV_CTRL_STRING_DELETE_METAMODE,
                                       tmp);
        if (ret == NvCtrlSuccess) {
            nvMetaModePtr metamode;

            nv_info_msg(TAB, "Removed MetaMode > %s", str);

            /* MetaModes after the one that was deleted will have
             * moved up an index, so update the book keeping here.
             */
            for (metamode = screen->metamodes;
                 metamode;
                 metamode = metamode->next) {
                if (metamode->x_idx >= idx) {
                    metamode->x_idx--;
                }
            }
        }

        free(tmp);
    }

    /* Reorder the list of metamodes */
    order_metamodes(screen);

    /* Cleanup */
    cleanup_metamodes_for_apply(screen);
}



/** update_screen_metamodes() ****************************************
 *
 * Updates the screen's metamode list.
 *
 **/

static int update_screen_metamodes(CtkDisplayConfig *ctk_object,
                                   nvScreenPtr screen)
{
    char *metamode_strs = NULL;
    char *cur_full_metamode_str = NULL;
    char *cur_metamode_ptr = NULL; /* Pointer into metamode_strs */
    int cur_metamode_id; /* ID of current MetaMode on X screen */
    int cur_metamode_idx;
    int num_metamodes_in_X;
    char *str;
    const char *cur_metamode_str;
    int len;

    int clear_apply = 0; /* Set if we should clear the apply button */
    ReturnStatus ret;


    /* Make sure the screen has a valid target to make the updates */
    if (screen->ctrl_target == NULL) {
        return 1;
    }

    nv_info_msg("", "Updating Screen %d's MetaModes:",
                NvCtrlGetTargetId(screen->ctrl_target));

    /* To update the metamode list of the screen:
     *
     * (preprocess)
     *  - Get the current list of metamodes for this screen
     *  - Add all the new metamodes at the end of the list
     *
     * (mode switch)
     *  - Do a modeswitch, if we need to
     *
     * (postprocess)
     *  - Delete any unused mode
     *  - Move metamodes to the correct location
     */

    /* Get the list of the current metamodes */

    ret = NvCtrlGetBinaryAttribute(screen->ctrl_target,
                                   0,
                                   NV_CTRL_BINARY_DATA_METAMODES_VERSION_2,
                                   (unsigned char **)&metamode_strs,
                                   &len);
    if (ret != NvCtrlSuccess) goto done;

    /* Get the current metamode for the screen */

    ret = NvCtrlGetStringAttribute(screen->ctrl_target,
                                   NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                                   &cur_full_metamode_str);
    if (ret != NvCtrlSuccess) goto done;

    /* Get the current metamode index for the screen */

    ret = NvCtrlGetAttribute(screen->ctrl_target,
                             NV_CTRL_CURRENT_METAMODE_ID,
                             &cur_metamode_id);
    if (ret != NvCtrlSuccess) goto done;

    /* Skip tokens */
    cur_metamode_str = strstr(cur_full_metamode_str, "::");
    if (cur_metamode_str) {
        cur_metamode_str = parse_skip_whitespace(cur_metamode_str +2);
    } else {
        cur_metamode_str = cur_full_metamode_str;
    }

    /* Count the number of metamodes in X */
    num_metamodes_in_X = 0;
    for (str = metamode_strs;
         str && strlen(str);
         str += strlen(str) +1) {
        num_metamodes_in_X++;
    }

    /* Find cur_metamode_str inside metamode_strs */
    cur_metamode_ptr = NULL;
    cur_metamode_idx = 0;
    for (str = metamode_strs;
         str && strlen(str);
         str += strlen(str) +1) {
        const char *tmp;

        tmp = strstr(str, "::");
        if (!tmp) continue;
        tmp = parse_skip_whitespace(tmp +2);
        if (!tmp) continue;

        if (!strcasecmp(tmp, cur_metamode_str)) {
            cur_metamode_ptr = str;
            break;
        }
        cur_metamode_idx++;
    }

    if (!cur_metamode_ptr) {
        nv_error_msg("Failed to identify current MetaMode in X list of "
                     "MetaModes for screen %d", screen->scrnum);
        return 1;
    }

    /* Add new metamodes and relate MetaModes from CPL to X */

    preprocess_metamodes(ctk_object, screen, metamode_strs, cur_metamode_ptr,
                         num_metamodes_in_X,
                         cur_metamode_idx);

    /* Update the current metamode.
     *
     * At this point, the metamode we want to set as the current metamode should
     * exist in the X server, or we will need to clobber the current X
     * metamode with new data.
     *
     * - If the current CPL MetaMode is the same as the current X MetaMode,
     *   do nothing.
     *
     * - If the current CPL MetaMode is a different X MetaMode, switch to it.
     *
     * - If the current CPL MetaMode is not the same as the current X MetaMode.
     *   and both the CPL MetaMode is not some other X MetaMode, and the current
     *   X MetaMode is not some other CPL MetaMode, then we can modify the
     *   current X MetaMode to be the CPL MetaMode.
     *
     * - If the current CPL MetaMode is not the same as the current X MetaMode,
     *   and we matched the current X MetaMode to some other CPL MetaMode, then
     *   we should add the current CPL MetaMode to X and switch to it.
     */

    if (screen->cur_metamode->id != cur_metamode_id) {

        if (switch_to_current_metamode(ctk_object, screen,
                                       cur_metamode_str)) {

            ctk_config_statusbar_message(ctk_object->ctk_config,
                                         "Switched to MetaMode %dx%d.",
                                         screen->cur_metamode->edim.width,
                                         screen->cur_metamode->edim.height);

            nv_info_msg(TAB, "Using   > %s", screen->cur_metamode->cpl_str);

            clear_apply = 1;
        }
    } else {
        clear_apply = 1;
    }

    /* Post process the metamodes list */

    postprocess_metamodes(screen, metamode_strs);

 done:

    free(metamode_strs);
    free(cur_full_metamode_str);

    return clear_apply;

} /* update_screen_metamodes() */



/** apply_clicked() **************************************************
 *
 * Called when user clicks on the "Apply" button.
 *
 **/

static void apply_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvScreenPtr screen;
    ReturnStatus ret;
    gboolean clear_apply = TRUE;


    /* Make sure we can apply */
    if (!validate_apply(ctk_object)) {
        return;
    }

    /* Make sure the layout is ready to be applied */
    if (!validate_layout(ctk_object, VALIDATE_APPLY)) {
        return;
    }

    /* Temporarily unregister events */
    unregister_layout_events(ctk_object);

    /* Update all X screens */
    for (screen = ctk_object->layout->screens;
         screen;
         screen = screen->next_in_layout) {

        if (screen->ctrl_target == NULL) {
            continue;
        }
        if (screen->no_scanout) continue;

        if (screen->primaryDisplay && ctk_object->primary_display_changed) {
            ret = NvCtrlSetStringAttribute(screen->ctrl_target,
                                           NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER,
                                           screen->primaryDisplay->typeIdName);
            if (ret != NvCtrlSuccess) {
                nv_error_msg("Failed to set primary display for screen %d",
                             screen->scrnum);
            } else {
                /* Make sure other parts of nvidia-settings get updated */
                ctk_event_emit_string(screen->ctk_event, 0,
                                      NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER);
                ctk_object->primary_display_changed = FALSE;
            }
        }
        
        if (!update_screen_metamodes(ctk_object, screen)) {
            clear_apply = FALSE;
        }

    }

    /* Clear the apply button if all went well, and we were able to apply
     * everything.
     */
    if (ctk_object->apply_possible && clear_apply) {
        gtk_widget_set_sensitive(widget, False);
        ctk_object->forced_reset_allowed = TRUE;
    }

    /* XXX Run the GTK main loop to flush any pending layout events
     * that should be ignored.  This is done because the GTK main loop
     * seems to only ignore the first blocked event received when it
     * finally runs.
     */
    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }

    /* re-register to receive events */
    register_layout_events(ctk_object);

    update_gui(ctk_object);

} /* apply_clicked() */



/** makeXConfigModeline() ********************************************
 *
 * Returns a copy of an XF86Config-parser modeline structure.
 *
 */

static XConfigModeLinePtr makeXConfigModeline(nvModeLinePtr modeline)
{
    XConfigModeLinePtr xconf_modeline;

    if (!modeline) return NULL;

    xconf_modeline = (XConfigModeLinePtr) malloc(sizeof(XConfigModeLineRec));
    if (!xconf_modeline) return NULL;

    *xconf_modeline = modeline->data;

    if (modeline->xconfig_name) {
        xconf_modeline->identifier = xconfigStrdup(modeline->xconfig_name);

    } else if (modeline->data.identifier) {
        xconf_modeline->identifier = xconfigStrdup(modeline->data.identifier);

    }

    if (modeline->data.clock) {
        xconf_modeline->clock = xconfigStrdup(modeline->data.clock);
    }
    
    if (modeline->data.comment) {
        xconf_modeline->comment = xconfigStrdup(modeline->data.comment);
    }

    return xconf_modeline;

} /* makeXConfigModeline() */



/*
 * add_modelines_to_monitor() - Given a list of modes "modes", this
 * function adds all the user-specified modelines in use to the
 * X config monitor "monitor"'s modeline list.
 */

static Bool add_modelines_to_monitor(XConfigMonitorPtr monitor,
                                     nvModePtr modes)
{
    XConfigModeLinePtr modeline;
    nvModePtr mode;

    /* Add modelines from the list of modes given */
    for (mode = modes; mode; mode = mode->next) {
        if (!mode->modeline) continue;

        /* Only add modelines that originated from the X config
         * or that were added through NV-CONTROL.
         */
        if (!(mode->modeline->source & MODELINE_SOURCE_USER)) continue;

        /* Don't add the same modeline twice */
        if ((mode->modeline->source & MODELINE_SOURCE_XCONFIG)) {
            if (xconfigFindModeLine(mode->modeline->xconfig_name,
                                    monitor->modelines)) continue;
        } else {
            if (xconfigFindModeLine(mode->modeline->data.identifier,
                                    monitor->modelines)) continue;
        }
        
        /* Dupe the modeline and add it to the monitor section */
        modeline = makeXConfigModeline(mode->modeline);
        if (!modeline) continue;

        /* Append to the end of the modeline list */
        xconfigAddListItem((GenericListPtr *)(&monitor->modelines),
                           (GenericListPtr)modeline);
    }

    return TRUE;

} /* add_modelines_to_monitor() */



/*
 * add_monitor_to_xconfig() - Adds the given display device's information
 * to the X configuration structure.
 */

static Bool add_monitor_to_xconfig(nvDisplayPtr display, XConfigPtr config,
                                   int monitor_id)
{
    XConfigMonitorPtr monitor;
    XConfigOptionPtr opt = NULL;
    ReturnStatus ret;
    char *range_str = NULL;
    char *tmp;
    char *v_source = NULL;
    char *h_source = NULL;
    float min, max;
    unsigned int i, j, len;
    
    monitor = (XConfigMonitorPtr)calloc(1, sizeof(XConfigMonitorRec));
    if (!monitor) goto fail;
    
    monitor->identifier = malloc(32);
    snprintf(monitor->identifier, 32, "Monitor%d", monitor_id);
    monitor->vendor = xconfigStrdup("Unknown");  /* XXX */

    /* Copy the model name string, stripping any '"' characters */

    len = strlen(display->logName);
    monitor->modelname = malloc(len + 1);
    for (i = 0, j = 0; i < len; i++, j++) {
        if (display->logName[i] == '\"') {
            if (++i >= len)
                break;
        }
        monitor->modelname[j] = display->logName[i];
    }
    monitor->modelname[j] = '\0';

    /* Get the Horizontal Sync ranges from nv-control */

    ret = NvCtrlGetStringAttribute(display->ctrl_target,
                                   NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES,
                                   &range_str);
    if (ret != NvCtrlSuccess) {
        nv_error_msg("Unable to determine valid horizontal sync ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->logName, display->gpu->name);
        goto fail;
    }
    
    /* Skip tokens */
    tmp = strstr(range_str, "::");
    if (tmp) {
        *tmp = '\0';
        tmp += 2;
    }
    
    if (!parse_read_float_range(tmp, &min, &max)) {
        nv_error_msg("Unable to determine valid horizontal sync ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->logName, display->gpu->name);
        goto fail;
    }

    monitor->n_hsync = 1;
    monitor->hsync[0].lo = min;
    monitor->hsync[0].hi = max;
    
    parse_token_value_pairs(range_str, apply_monitor_token,
                            (void *)(&h_source));
    free(range_str);
    range_str = NULL;

    /* Get the Horizontal Sync ranges from nv-control */

    ret = NvCtrlGetStringAttribute(display->ctrl_target,
                                   NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES,
                                   &range_str);
    if (ret != NvCtrlSuccess) {
        nv_error_msg("Unable to determine valid vertical refresh ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->logName, display->gpu->name);
        goto fail;
    }
    
    /* Skip tokens */
    tmp = strstr(range_str, "::");
    if (tmp) {
        *tmp = '\0';
        tmp += 2;
    }
    
    if (!parse_read_float_range(tmp, &min, &max)) {
        nv_error_msg("Unable to determine valid vertical refresh ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->logName, display->gpu->name);
        goto fail;
    }

    monitor->n_vrefresh = 1;
    monitor->vrefresh[0].lo = min;
    monitor->vrefresh[0].hi = max;

    parse_token_value_pairs(range_str, apply_monitor_token,
                            (void *)(&v_source));
    free(range_str);
    range_str = NULL;

    if (h_source && v_source) {
        monitor->comment =
            g_strdup_printf("    # HorizSync source: %s, "
                            "VertRefresh source: %s\n",
                            h_source, v_source);
    }
    free(h_source);
    free(v_source);

    /* Add other options */

    xconfigAddNewOption(&opt, "DPMS", NULL);
    monitor->options = opt;

    /* Add modelines used by this display */

    add_modelines_to_monitor(monitor, display->modes);

    /* Append the monitor to the end of the monitor list */

    xconfigAddListItem((GenericListPtr *)(&config->monitors),
                       (GenericListPtr)monitor);

    display->conf_monitor = monitor;
    return TRUE;


 fail:
    free(range_str);
    free(h_source);
    free(v_source);
    if (monitor) {
        xconfigFreeMonitorList(&monitor);
    }
    return FALSE;

} /* add_monitor_to_xconfig() */



/*
 * add_device_to_xconfig() - Adds the given device (GPU)'s information
 * to the X configuration file.  If a valid screen order number is given,
 * it is also included (This is required for having separate X screens
 * driven by a single GPU.)
 */

static XConfigDevicePtr add_device_to_xconfig(nvGpuPtr gpu, XConfigPtr config,
                                              int device_id, int screen_id,
                                              int print_bus_id)
{
    XConfigDevicePtr device;

    device = (XConfigDevicePtr)calloc(1, sizeof(XConfigDeviceRec));
    if (!device) goto fail;


    /* Fill out the device information */
    device->identifier = malloc(32);
    snprintf(device->identifier, 32, "Device%d", device_id);

    device->driver = xconfigStrdup("nvidia");
    device->vendor = xconfigStrdup("NVIDIA Corporation");
    device->board = xconfigStrdup(gpu->name);

    if (print_bus_id && gpu->pci_bus_id) {
        device->busid = strdup(gpu->pci_bus_id);
    }

    device->chipid = -1;
    device->chiprev = -1;
    device->irq = -1;
    device->screen = screen_id;
    

    /* Append to the end of the device list */
    xconfigAddListItem((GenericListPtr *)(&config->devices),
                       (GenericListPtr)device);

    return device;

 fail:
    if (device) {
        xconfigFreeDeviceList(&device);
    }
    return NULL;

} /* add_device_to_xconfig() */



/*
 * add_display_to_screen() - Sets up the display subsection of
 * the X config screen structure with information from the given
 * screen.
 */

static Bool add_display_to_screen(nvScreenPtr screen,
                                  XConfigScreenPtr conf_screen)
{
    XConfigDisplayPtr conf_display;

    /* Clear the display list */
    xconfigFreeDisplayList(&conf_screen->displays);


    /* Add a single display subsection for the default depth */
    xconfigAddDisplay(&conf_screen->displays, conf_screen->defaultdepth);
    if (!conf_screen->displays) goto fail;

    /* Configure the virtual screen size */
    if (screen->no_scanout) {
        conf_display = conf_screen->displays;

        conf_display->virtualX = screen->dim.width;
        conf_display->virtualY = screen->dim.height;
    }

    /* XXX Don't do any further tweaking to the display subsection.
     *     All mode configuration should be done through the 'MetaModes"
     *     X Option.  The modes generated by xconfigAddDisplay() will
     *     be used as a fallback.
     */

    return TRUE;

 fail:
    xconfigFreeDisplayList(&conf_screen->displays);

    return FALSE;

} /* add_display_to_screen() */



/*
 * add_screen_to_xconfig() - Adds the given X screen's information
 * to the X configuration structure.
 */

static int add_screen_to_xconfig(CtkDisplayConfig *ctk_object,
                                 nvScreenPtr screen, XConfigPtr config)
{
    XConfigScreenPtr conf_screen;
    nvDisplayPtr display;
    nvDisplayPtr other;
    char *metamode_strs;
    int ret;

    conf_screen = (XConfigScreenPtr)calloc(1, sizeof(XConfigScreenRec));
    if (!conf_screen) goto fail;


    /* Fill out the screen information */
    conf_screen->identifier = malloc(32);
    snprintf(conf_screen->identifier, 32, "Screen%d", screen->scrnum);

    
    /* Tie the screen to its device section */
    conf_screen->device_name =
        xconfigStrdup(screen->conf_device->identifier);
    conf_screen->device = screen->conf_device;

    
    if (screen->no_scanout) {
        /* Configure screen for no scanout */

        /* Set the UseDisplayDevice option to "none" */
        xconfigAddNewOption(&conf_screen->options, "UseDisplayDevice", "none");

    } else {
        /* Configure screen for scanout */

        /* Find the first display on the screen */
        display = screen->displays;;

        if (!display) {
            nv_error_msg("Unable to find a display device for screen %d!",
                         screen->scrnum);
            goto fail;
        }


        /* Create the screen's only Monitor section from the first display */
        if (!add_monitor_to_xconfig(display, config, screen->scrnum)) {
            nv_error_msg("Failed to add display device '%s' to screen %d!",
                         display->logName, screen->scrnum);
            goto fail;
        }


        /* Tie the screen to the monitor section */
        conf_screen->monitor_name =
            xconfigStrdup(display->conf_monitor->identifier);
        conf_screen->monitor = display->conf_monitor;


        /* Add the modelines of all other connected displays to the monitor */
        for (other = display->next_in_screen;
             other;
             other = other->next_in_screen) {
            add_modelines_to_monitor(display->conf_monitor, other->modes);
        }

        /* Set the Stereo option */
        {
            char buf[32];
            snprintf(buf, 32, "%d", screen->stereo);
            xconfigAddNewOption(&conf_screen->options, "Stereo", buf);
        }

        /* Set the nvidiaXineramaInfoOrder option */
        if (screen->primaryDisplay) {
            xconfigAddNewOption(&conf_screen->options,
                                "nvidiaXineramaInfoOrder",
                                screen->primaryDisplay->typeIdName);
        }

        /* Create the "metamode" option string. */
        ret = generate_xconf_metamode_str(ctk_object, screen, &metamode_strs);
        if (ret != XCONFIG_GEN_OK) goto bail;

        /* If no user specified metamodes were found, add
         * whatever the currently selected metamode is
         */
        if (!metamode_strs) {
            metamode_strs = screen_get_metamode_str(screen,
                                                    screen->cur_metamode_idx,
                                                    0);
        }

        if (metamode_strs) {
            xconfigAddNewOption(&conf_screen->options, "metamodes",
                                metamode_strs);
            free(metamode_strs);
        }

        /* Set Mosaic configuration */
        if (screen->display_owner_gpu->mosaic_enabled) {
            xconfigAddNewOption(&conf_screen->options, "MultiGPU", "Off");

            switch (screen->display_owner_gpu->mosaic_type) {
            case MOSAIC_TYPE_SLI_MOSAIC:
                xconfigAddNewOption(&conf_screen->options, "SLI", "Mosaic");
                xconfigAddNewOption(&conf_screen->options, "BaseMosaic", "off");
                break;
            case MOSAIC_TYPE_BASE_MOSAIC:
            case MOSAIC_TYPE_BASE_MOSAIC_LIMITED:
                xconfigAddNewOption(&conf_screen->options, "SLI", "off");
                xconfigAddNewOption(&conf_screen->options, "BaseMosaic", "on");
                break;
            default:
                nv_warning_msg("Uknonwn mosaic mode %d",
                               screen->display_owner_gpu->mosaic_type);
                xconfigAddNewOption(&conf_screen->options, "SLI",
                                    screen->sli_mode ? screen->sli_mode : "Off");
                xconfigAddNewOption(&conf_screen->options, "BaseMosaic", "off");
                break;
            }
        } else {
            /* Set SLI configuration */
            if (screen->sli_mode &&
                !g_ascii_strcasecmp(screen->sli_mode, "Mosaic")) {
                xconfigAddNewOption(&conf_screen->options, "SLI", "Off");
            } else {
                xconfigAddNewOption(&conf_screen->options, "SLI",
                                    screen->sli_mode ? screen->sli_mode : "Off");
            }

            xconfigAddNewOption(&conf_screen->options, "MultiGPU",
                                screen->multigpu_mode ? screen->multigpu_mode : "Off");

            xconfigAddNewOption(&conf_screen->options, "BaseMosaic", "off");

        }
    }


    /* Setup the display section */
    conf_screen->defaultdepth = screen->depth;


    /* Setup the display subsection of the screen */
    if (!add_display_to_screen(screen, conf_screen)) {
        nv_error_msg("Failed to add Display section for screen %d!",
                     screen->scrnum);
        goto fail;
    }


    /* Append to the end of the screen list */
    xconfigAddListItem((GenericListPtr *)(&config->screens),
                       (GenericListPtr)conf_screen);
    
    screen->conf_screen = conf_screen;

    return XCONFIG_GEN_OK;

    
    /* Handle failure cases */

 fail:
    ret = XCONFIG_GEN_ERROR;
 bail:
    if (conf_screen) {
        xconfigFreeScreenList(&conf_screen);
    }
    return ret;

} /* add_screen_to_xconfig() */



/*
 * get_device_screen_id() - Returns the screen number that should be
 * used in the device section that maps to the given screen's screen
 * section.
 */
static int get_device_screen_id(nvGpuPtr gpu, nvScreenPtr screen)
{
    nvLayoutPtr layout = gpu->layout;
    nvScreenPtr other;
    int device_screen_id;
    int num_screens_on_gpu;

    /* Go through the GPU's screens and figure out what the
     * GPU-relative screen number should be for the given
     * screen's device section.
     *
     * This is done by counting the number of screens that
     * have a screen number that is lower than the given
     * screen, and that's the relative position of this
     * screen wrt the GPU.
     */

    device_screen_id = 0;
    num_screens_on_gpu = 0;
    for (other = layout->screens; other; other = other->next_in_layout) {
        if (!screen_has_gpu(other, gpu)) {
            continue;
        }

        num_screens_on_gpu++;

        if (other == screen) continue;

        if (screen->scrnum > other->scrnum) {
            device_screen_id++;
        }
    }

    /* If there is only one screen on the GPU, the device
     * section shouldn't specify a "Screen #"
     */
    if (num_screens_on_gpu < 2) return -1;

    return device_screen_id;

} /* get_device_screen_id() */



/*
 * add_screens_to_xconfig() - Adds all the X screens in the given
 * layout to the X configuration structure.
 */

static int add_screens_to_xconfig(CtkDisplayConfig *ctk_object,
                                  nvLayoutPtr layout, XConfigPtr config)
{
    nvScreenPtr screen;
    int device_screen_id;
    int print_bus_ids;
    int ret;


    /* Clear the screen list */
    xconfigFreeMonitorList(&config->monitors);
    xconfigFreeDeviceList(&config->devices);
    xconfigFreeScreenList(&config->screens);

    /* Don't print the bus ID in the case where we have a single
     * GPU driving a single X screen
     */
    if ((layout->num_gpus == 1) &&
        (layout->num_screens == 1)) {
        print_bus_ids = 0;
    } else {
        print_bus_ids = 1;
    }

    /* Generate the Device sections and Screen sections */

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        nvGpuPtr gpu = screen->display_owner_gpu;

        /* Figure out what screen number to use for the device section. */
        device_screen_id = get_device_screen_id(gpu, screen);

        /* Each screen needs a unique device section
         *
         * Note that the device id used to name the
         * device section is the same as the screen
         * number such that the name of the two sections
         * match.
         */
        screen->conf_device = add_device_to_xconfig(gpu, config,
                                                    screen->scrnum,
                                                    device_screen_id,
                                                    print_bus_ids);
        if (!screen->conf_device) {
            nv_error_msg("Failed to add device '%s' to X config.",
                         gpu->name);
            goto fail;
        }

        ret = add_screen_to_xconfig(ctk_object, screen, config);
        if (ret == XCONFIG_GEN_ERROR) {
            nv_error_msg("Failed to add X screen %d to X config.",
                         screen->scrnum);
        }
        if (ret != XCONFIG_GEN_OK) goto bail;
    }

    return XCONFIG_GEN_OK;


    /* Handle failure cases */

 fail:
    ret = XCONFIG_GEN_ERROR;
 bail:
    xconfigFreeMonitorList(&config->monitors);
    xconfigFreeDeviceList(&config->devices);
    xconfigFreeScreenList(&config->screens);
    return ret;

} /* add_screens_to_xconfig() */



/*
 * add_adjacency_to_xconfig() - Adds the given X screen's positioning
 * information to an X config structure.
 */

static Bool add_adjacency_to_xconfig(nvScreenPtr screen, XConfigPtr config)
{
    XConfigAdjacencyPtr adj;
    XConfigLayoutPtr conf_layout = config->layouts;


    adj = (XConfigAdjacencyPtr) calloc(1, sizeof(XConfigAdjacencyRec));
    if (!adj) return FALSE;

    adj->scrnum = screen->scrnum;
    adj->screen = screen->conf_screen;
    adj->screen_name = xconfigStrdup(screen->conf_screen->identifier);

    /* Position the X screen */
    if (screen->position_type == CONF_ADJ_ABSOLUTE) {
        adj->x = screen->dim.x;
        adj->y = screen->dim.y;
    } else {
        adj->where = screen->position_type;
        adj->refscreen =
            xconfigStrdup(screen->relative_to->conf_screen->identifier);
        adj->x = screen->x_offset;
        adj->y = screen->y_offset;
    }

    /* Append to the end of the screen list */
    xconfigAddListItem((GenericListPtr *)(&conf_layout->adjacencies),
                       (GenericListPtr)adj);

    return TRUE;

} /* add_adjacency_to_xconfig() */



/*
 * add_layout_to_xconfig() - Adds layout (adjacency/X screen
 * positioning) information to the X config structure based
 * in the layout given.
 */

static Bool add_layout_to_xconfig(nvLayoutPtr layout, XConfigPtr config)
{
    XConfigLayoutPtr conf_layout;
    nvScreenPtr screen;
    int scrnum;


    /* Just modify the first layout */
    conf_layout = config->layouts;
    if (!conf_layout) {
        nv_error_msg("Unable to generate initial layout!");
        goto fail;
    }


    /* Clean up the adjacencies */
    xconfigFreeAdjacencyList(&conf_layout->adjacencies);


    /* Assign the adjacencies (in order) */
    scrnum = 0;
    do {
        /* Find the next screen to write */
        screen = NULL;
        for (screen = layout->screens;
             screen;
             screen = screen->next_in_layout) {
            if (screen->scrnum == scrnum) break;
        }
        if (screen) {
            if (!add_adjacency_to_xconfig(screen, config)) goto fail;
        }

        scrnum++;
    } while (screen);


    /* Setup for Xinerama */
    xconfigAddNewOption(&conf_layout->options, "Xinerama",
                        (layout->xinerama_enabled ? "1" : "0"));

    layout->conf_layout = conf_layout;
    return TRUE;

 fail:
    return FALSE;

} /* add_layout_to_xconfig() */



/*
 * generateXConfig() - Generates an X config structure based
 * on the layout given.
 */

static int generateXConfig(CtkDisplayConfig *ctk_object, XConfigPtr *pConfig)
{
    nvLayoutPtr layout = ctk_object->layout;
    XConfigPtr config = NULL;
    GenerateOptions go;
    int ret;


    if (!pConfig) goto fail;


    /* XXX Assume we are creating an X config file for the local system */
    xconfigGenerateLoadDefaultOptions(&go);
    xconfigGetXServerInUse(&go);

    /* Generate the basic layout */
    config = xconfigGenerate(&go);


    /* Repopulate the X config file with the right information */
    ret = add_screens_to_xconfig(ctk_object, layout, config);
    if (ret == XCONFIG_GEN_ERROR) {
        nv_error_msg("Failed to add X screens to X config.");
    }
    if (ret != XCONFIG_GEN_OK) goto bail;

    if (!add_layout_to_xconfig(layout, config)) {
        nv_error_msg("Failed to add server layout to X config.");
        goto fail;
    }

    /* Check if composite should be disabled */
    {
        char *composite_disabled_str = NULL;
        nvScreenPtr screen;

        /* See if any X screens have overlay, cioverlay, ubb or stereo enabled,
         * or depth 8.
         */
        for (screen = layout->screens; screen; screen = screen->next_in_layout) {

            composite_disabled_str =
                xconfigValidateComposite(config,
                                         &go,
                                         1, // composite_specified
                                         layout->xinerama_enabled,
                                         screen->depth,
                                         screen->overlay && screen->hw_overlay,
                                         screen->overlay && !screen->hw_overlay,
                                         screen->ubb,
                                         screen->stereo
                                         );
            if (composite_disabled_str) {
                break;
            }
        }

        if (composite_disabled_str) {
            if (!config->extensions) {
                config->extensions = nvalloc(sizeof(XConfigExtensionsRec));
            }
            xconfigRemoveNamedOption(&(config->extensions->options), 
                                     go.compositeExtensionName,
                                     NULL);
            xconfigAddNewOption(&config->extensions->options, 
                                go.compositeExtensionName, 
                                "Disable");
            nvfree(composite_disabled_str);
        }
    }

    *pConfig = config;

    return XCONFIG_GEN_OK;


    /* Handle failure cases */
    
 fail:
    ret = XCONFIG_GEN_ERROR;
 bail:
    if (config) {
        xconfigFreeConfig(&config);
    }
    return ret;

} /* generateXConfig() */



/** preserve_busid() **************************************************
 *
 * Copies the BusID value from the source to the destination
 * configuration for devices with matching identifiers.
 *
 **/

static void preserve_busid(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    XConfigDevicePtr dstDevice, srcDevice;

    for (srcDevice = srcConfig->devices;
         srcDevice;
         srcDevice = srcDevice->next) {

        if (!srcDevice->busid) {
            continue;
        }

        dstDevice =
            xconfigFindDevice(srcDevice->identifier, dstConfig->devices);

        /*
         * Only overwrite the BusID in the destination config if
         * the destination config has not generated its own
         * BusID.  If nvidia-settings determines that the new
         * requested config requires a specific BusID, a merge
         * shouldn't overwrite that specific BusID just because
         * the old config happened to have a device with a matching
         * identifier and a specified BusID, which may be different
         * and incompatible with the new config.
         */
        if (dstDevice &&
            (dstDevice->busid == NULL)) {
            dstDevice->busid = xconfigStrdup(srcDevice->busid);
        }
    }
}



/** xconfig_generate() ***********************************************
 *
 * Callback to generate an X config structure based on the current
 * display configuration.
 *
 **/
static XConfigPtr xconfig_generate(XConfigPtr xconfCur,
                                   Bool merge,
                                   Bool *merged,
                                   void *callback_data)
{
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *)callback_data;
    XConfigPtr xconfGen = NULL;
    gint result;


    *merged = FALSE;

    /* Generate an X config structure from our layout */
    result = generateXConfig(ctk_object, &xconfGen);
    if ((result != XCONFIG_GEN_OK) || !xconfGen) {
        goto fail;
    }

    /* If we're not merging, we're done */
    if (!xconfCur || !merge) {
        return xconfGen;
    }

    /* The Bus ID of devices may not be set by generateXConfig above so to
     * preserve this field, we have to copy the Bus IDs over before merging.
     */
    preserve_busid(xconfGen, xconfCur);

    /* Merge xconfGen into xconfCur */
    result = xconfigMergeConfigs(xconfCur, xconfGen);
    if (!result) {
        gchar *err_msg = g_strdup_printf("Failed to merge generated "
                                         "configuration with existing "
                                         "X config file!");
        ctk_display_warning_msg(ctk_get_parent_window(GTK_WIDGET(ctk_object)),
                                err_msg);
        g_free(err_msg);
        return xconfGen;
    }

    /* Merge worked */
    xconfigFreeConfig(&xconfGen);
    *merged = TRUE;
    return xconfCur;


 fail:
    if (xconfGen) {
        xconfigFreeConfig(&xconfGen);
    }

    return NULL;

} /* xconfig_generate() */



/** save_clicked() ***************************************************
 *
 * Called when the user clicks on the "Save" button.
 *
 **/

static void save_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);


    /* Make sure the layout is ready to be saved */
    if (!validate_layout(ctk_object, VALIDATE_SAVE)) {
        return;
    }

    /* Run the save dialog */
    if (run_save_xconfig_dialog(ctk_object->save_xconfig_dlg)) {

        /* Config file written */
        ctk_object->ctk_config->pending_config &=
            ~CTK_CONFIG_PENDING_WRITE_DISPLAY_CONFIG;
    }

} /* save_clicked() */



/** advanced_clicked() ***********************************************
 *
 * Called when user clicks on the "Advanced..." button.
 *
 **/

static void advanced_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);


    /* Toggle advanced options for the display */
    ctk_object->advanced_mode = !(ctk_object->advanced_mode);


    /* Show advanced display options */
    if (ctk_object->advanced_mode) {
        gtk_button_set_label(GTK_BUTTON(widget), "Basic...");
        ctk_display_layout_set_advanced_mode
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), 1);

    /* Show basic display options */
    } else {
        gtk_button_set_label(GTK_BUTTON(widget), "Advanced...");
        ctk_display_layout_set_advanced_mode
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), 0);
    }

    /* Update the GUI to show the right widgets */
    update_gui(ctk_object);

} /* advanced_clicked() */



/** probe_clicked() **************************************************
 *
 * Called when user clicks on the "Probe" button.
 *
 **/

static void probe_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    unsigned int probed_displays;
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    ReturnStatus ret;

    /* Probe each GPU for display changes */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {

        if (gpu->ctrl_target == NULL) {
            continue;
        }

        ret = NvCtrlGetAttribute(gpu->ctrl_target, NV_CTRL_PROBE_DISPLAYS,
                                 (int *)&probed_displays);
        if (ret != NvCtrlSuccess) {
            nv_error_msg("Failed to probe for display devices on GPU-%d '%s'.",
                         NvCtrlGetTargetId(gpu->ctrl_target), gpu->name);
            continue;
        }

        /* Emit the probe event to ourself so changes are handled
         * consistently.
         */
        ctk_event_emit(gpu->ctk_event, 0,
                       NV_CTRL_PROBE_DISPLAYS, probed_displays);
    }

} /* probe_clicked() */

/** layout_change_is_applyable() ***********************************
 *
 * Determine whether an updated layout should let the user press the Apply
 * button.
 *
 **/
static gboolean layout_change_is_applyable(const nvLayoutPtr old,
                                           const nvLayoutPtr new)
{
    const nvGpu *gpu;

    /* The update should be applyable if any active display devices were
     * removed. */
    for (gpu = old->gpus; gpu; gpu = gpu->next_in_layout) {
        const nvDisplay *dpy;

        for (dpy = gpu->displays; dpy; dpy = dpy->next_on_gpu) {

            /* See if the display was active in the old layout. */
            if (!dpy->cur_mode || !dpy->cur_mode->modeline) {
                continue;
            }

            /* This display device had an active mode in the old layout.  See if
             * it's still connected in the new layout. */
            if (!layout_get_display(new,
                                    NvCtrlGetTargetId(dpy->ctrl_target))) {
                return True;
            }
        }
    }

    return False;
}

/** reset_layout() *************************************************
 *
 * Load current X server settings.
 *
 **/

static void reset_layout(CtkDisplayConfig *ctk_object)
{
    gchar *err_str = NULL;
    nvLayoutPtr layout;
    gboolean allow_apply;

    /* Load the current layout */
    layout = layout_load_from_server(ctk_object->ctrl_target, &err_str);

    /* Handle errors loading the new layout */
    if (!layout || err_str) {
        if (err_str) {
            nv_error_msg("%s", err_str);
            g_free(err_str);
        }
        return;
    }

    /* See if we should allow the user to press the Apply button to make the new
     * layout take effect, e.g. if an active display device disappeared. */
    allow_apply = layout_change_is_applyable(ctk_object->layout, layout);

    /* Free existing layout */
    unregister_layout_events(ctk_object);
    layout_free(ctk_object->layout);


    /* Setup the new layout */
    ctk_object->layout = layout;
    ctk_display_layout_set_layout((CtkDisplayLayout *)(ctk_object->obj_layout),
                                  ctk_object->layout);

    register_layout_events(ctk_object);


    /* Make sure all X screens have the same depth if Xinerama is enabled */
    consolidate_xinerama(ctk_object, NULL);

    /* Make sure X screens have some kind of position */
    assign_screen_positions(ctk_object);

    update_gui(ctk_object);

    /* Get new position */
    get_cur_screen_pos(ctk_object);

    /* Update the apply button */
    ctk_object->apply_possible = TRUE;
    update_btn_apply(ctk_object, allow_apply);

    ctk_object->forced_reset_allowed = TRUE; /* OK to reset w/o user input */
    ctk_object->notify_user_of_reset = TRUE; /* Notify user of new changes */
    ctk_object->reset_required = FALSE; /* No reset required to apply */

} /* reset_layout() */



/** reset_clicked() **************************************************
 *
 * Called when user clicks on the "Reset" button.
 *
 **/

static void reset_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint result;


    /* Show the confirm dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_reset_confirm),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_grab_focus(ctk_object->btn_reset_cancel);
    gtk_widget_show(ctk_object->dlg_reset_confirm);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_reset_confirm));
    gtk_widget_hide(ctk_object->dlg_reset_confirm);
    
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        /* User wants to reset the configuration */
        break;
        
    case GTK_RESPONSE_REJECT:
    default:
        /* User doesn't want to reset the configuration */
        return;
    }

    reset_layout(ctk_object);

} /* reset_clicked() */



/** force_layout_reset() ******************************************
 *
 * Pop up dialog box to user when the layout needs to be reloaded
 * due to changes made to the server layout by another client.
 *
 **/

static gboolean force_layout_reset(gpointer user_data)
{
    gint result;
    GtkWidget *parent;
    GtkWidget *dlg;
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *) user_data;

    if ((ctk_object->forced_reset_allowed) ) {
        /* It is OK to force a reset of the layout since no
         * changes have been made.
         */
        reset_layout(ctk_object);
        goto done;
    }

    /* It is not OK to force a reset of the layout since the user
     * may have changed some settings.  The user will need to
     * reset the layout manually.
     */

    ctk_object->reset_required = TRUE;

    /* If the X server display configuration page is not currently
     * selected, we will need to notify the user once they get
     * back to it.
     */
    if (!ctk_object->page_selected) goto done;

    /* Notify the user of the required reset if they haven't
     * already been notified.
     */
    if (!ctk_object->notify_user_of_reset) goto done;

    parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));

    dlg = gtk_message_dialog_new 
        (GTK_WINDOW(parent),
         GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_MESSAGE_WARNING,
         GTK_BUTTONS_NONE,
         "Your current changes to the X server display configuration may no "
         "longer be applied due to changes made to the running X server.\n\n"
         "You may either reload the current X server settings and lose any "
         "configuration setup in this page, or select \"Cancel\" and save "
         "your changes to the X configuration file (requires restarting X "
         "to take effect.)\n\n"
         "If you select \"Cancel\", you will only be allowed to apply "
         "settings once you have reset the configuration.");

    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
                           "Reload current X server settings",
                           GTK_RESPONSE_YES,
                           "Cancel", GTK_RESPONSE_CANCEL,
                           NULL);

    result = gtk_dialog_run(GTK_DIALOG(dlg));
    switch (result) {
    case GTK_RESPONSE_YES:
        reset_layout(ctk_object);
        break;
    case GTK_RESPONSE_CANCEL:
        /* Fall through */
    default:
        /* User does not want to reset the layout, don't allow them
         * to apply their changes (but allow them to save their changes)
         * until they have reloaded the layout manually.
         */
        ctk_object->notify_user_of_reset = FALSE;
        update_btn_apply(ctk_object, FALSE);
        break;
    }
    gtk_widget_destroy(dlg);

done:
    ctk_object->ignore_reset_events = FALSE;
    return FALSE;

} /* force_layout_reset() */



/** display_config_attribute_changed() *******************************
 *
 * Callback function for all display config page related events
 * change.
 *
 * Display configuration changes usually involve multiple related
 * events in succession.  To avoid reloading the layout for every
 * event, we register the force_layout_reset() function (once per
 * block of events) to be called when the app becomes idle (which
 * will happen once there are no more pending events) using
 * g_idle_add().  Once force_layout_reset() is called, it will
 * unregister itself by returning FALSE.
 *
 **/

static void display_config_attribute_changed(GtkWidget *object,
                                             CtrlEvent *event,
                                             gpointer user_data)
{
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *) user_data;

    if (ctk_object->ignore_reset_events) return;

    ctk_object->ignore_reset_events = TRUE;

    /* queue force_layout_reset() to be called once all other pending
     * events are consumed.
     */
    g_idle_add(force_layout_reset, (gpointer)ctk_object);

} /* display_config_attribute_changed() */



/** ctk_display_config_unselected() **********************************
 *
 * Called when display config page is unselected.
 *
 **/

void  ctk_display_config_unselected(GtkWidget *widget) {
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(widget);

    ctk_object->page_selected = FALSE;

} /* ctk_display_config_unselected() */



/** ctk_display_config_selected() ***********************************
 *
 * Called when display config page is selected.
 *
 **/

void ctk_display_config_selected(GtkWidget *widget) {
    CtkDisplayConfig *ctk_object=CTK_DISPLAY_CONFIG(widget);

    ctk_object->page_selected = TRUE;

    /* Handle case where a layout reset is required but we could
     * not notify the user since the X server display configuration
     * page was not selected
     */
    if (ctk_object->reset_required) {
        force_layout_reset(ctk_object);
    }

} /* ctk_display_config_selected() */



/** validation_details_clicked() *************************************
 *
 * Callback for when the user clicks on the "Show/Hide Details"
 * button in the validation confirmation dialog.
 *
 **/

static void validation_details_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gboolean show =
        !(ctk_widget_get_visible(ctk_object->box_validation_override_details));

    if (show) {
        gtk_widget_show_all(ctk_object->box_validation_override_details);
        gtk_window_set_resizable
            (GTK_WINDOW(ctk_object->dlg_validation_override), TRUE);
        gtk_widget_set_size_request
            (ctk_object->box_validation_override_details, 450, 150);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->btn_validation_override_show),
             "Hide Details...");
    } else {
        gtk_widget_hide(ctk_object->box_validation_override_details);
        gtk_window_set_resizable
            (GTK_WINDOW(ctk_object->dlg_validation_override), FALSE);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->btn_validation_override_show),
             "Show Details...");
    }

} /* validation_details_clicked() */
