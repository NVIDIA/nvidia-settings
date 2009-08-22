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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <NvCtrlAttributes.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "msg.h"
#include "parse.h"
#include "lscf.h"

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

static void setup_display_page(CtkDisplayConfig *ctk_object);

static void display_config_clicked(GtkWidget *widget, gpointer user_data);

static void display_resolution_changed(GtkWidget *widget, gpointer user_data);
static void display_refresh_changed(GtkWidget *widget, gpointer user_data);
static void display_modelname_changed(GtkWidget *widget, gpointer user_data);

static void display_position_type_changed(GtkWidget *widget, gpointer user_data);
static void display_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void display_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void display_panning_activate(GtkWidget *widget, gpointer user_data);
static gboolean display_panning_focus_out(GtkWidget *widget, GdkEvent *event,
                                          gpointer user_data);

static void setup_screen_page(CtkDisplayConfig *ctk_object);

static void screen_virtual_size_activate(GtkWidget *widget, gpointer user_data);
static gboolean screen_virtual_size_focus_out(GtkWidget *widget, GdkEvent *event,
                                              gpointer user_data);

static void screen_depth_changed(GtkWidget *widget, gpointer user_data);

static void screen_position_type_changed(GtkWidget *widget, gpointer user_data);
static void screen_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void screen_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void screen_metamode_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_activate(GtkWidget *widget, gpointer user_data);
static void screen_metamode_add_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_delete_clicked(GtkWidget *widget, gpointer user_data);

static void xinerama_state_toggled(GtkWidget *widget, gpointer user_data);
static void apply_clicked(GtkWidget *widget, gpointer user_data);
static void save_clicked(GtkWidget *widget, gpointer user_data);
static void probe_clicked(GtkWidget *widget, gpointer user_data);
static void advanced_clicked(GtkWidget *widget, gpointer user_data);
static void reset_clicked(GtkWidget *widget, gpointer user_data);
static void validation_details_clicked(GtkWidget *widget, gpointer user_data);

static void display_config_attribute_changed(GtkObject *object, gpointer arg1,
                                           gpointer user_data);
static void reset_layout(CtkDisplayConfig *ctk_object);
static gboolean force_layout_reset(gpointer user_data);
static void user_changed_attributes(CtkDisplayConfig *ctk_object);

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

#define SCREEN_DEPTH_COUNT 4


/*** G L O B A L S ***********************************************************/

static int __position_table[] = { CONF_ADJ_ABSOLUTE,
                                  CONF_ADJ_RIGHTOF,
                                  CONF_ADJ_LEFTOF,
                                  CONF_ADJ_ABOVE,
                                  CONF_ADJ_BELOW,
                                  CONF_ADJ_RELATIVE };


/* Layout tooltips */

static const char * __layout_hidden_label_help =
"To select a display, use the \"Model\" dropdown menu.";

static const char * __layout_xinerama_button_help =
"The Enable Xinerama checkbox enables the Xinerama X extension; changing "
"this option will require restarting your X server.  Note that when Xinerama "
"is enabled, resolution changes will also require restarting your X server.";


/* Display tooltips */

static const char * __dpy_model_help =
"The Display drop-down allows you to select a desired display device.";

static const char * __dpy_resolution_mnu_help =
"The Resolution drop-down allows you to select a desired resolution "
"for the currently selected display device.";

static const char * __dpy_refresh_mnu_help =
"The Refresh drop-down allows you to select a desired refresh rate "
"for the currently selected display device.  Note that the selected "
"resolution may restrict the available refresh rates.";

static const char * __dpy_position_type_help =
"The Position Type drop-down allows you to set how the selected display "
"device is placed within the X screen. This is only available when "
"multiple display devices are present.";

static const char * __dpy_position_relative_help =
"The Position Relative drop-down allows you to set which other display "
"device (within the X screen) the selected display device should be "
"relative to. This is only available when multiple display "
"devices are present.";

static const char * __dpy_position_offset_help =
"The Position Offset identifies the top left of the display device "
"as an offset from the top left of the X screen position. This is only "
"available when multiple display devices are present.";

static const char * __dpy_panning_help =
"The Panning Domain sets the total width/height that the display "
"device may pan within.";

static const char * __dpy_primary_help =
"The primary display is often used by window managers to know which of the "
"displays in a multi-display setup to show information and other "
"important windows etc; changing this option may require restarting your X "
"server, depending on your window manager.";

/* Screen tooltips */

static const char * __screen_virtual_size_help =
"The Virtual Size allows setting the size of the resulting X screen. "
"The virtual size must be at least large enough to hold all the display "
"devices that are currently enabled for scanout.";

static const char * __screen_depth_help =
"The Depth drop-down allows setting of the color quality for the selected "
"screen; changing this option will require restarting your X server.";

static const char * __screen_position_type_help =
"The Position Type drop-down appears when two or more display devices are active. "
"This allows you to set how the selected screen "
"is placed within the X server layout; changing this option will require "
"restarting your X server.";

static const char * __screen_position_relative_help =
"The Position Relative drop-down appears when two or more display devices are active. "
"This allows you to set which other Screen "
"the selected screen should be relative to; changing this option will "
"require restarting your X server.";

static const char * __screen_position_offset_help =
"The Position Offset drop-down appears when two or more display devices are active. "
"This identifies the top left of the selected Screen as "
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

    ctk_object->cur_screen_pos[X] = screen->dim[X];
    ctk_object->cur_screen_pos[Y] = screen->dim[Y];

} /* get_cur_screen_pos() */



/** check_screen_pos_changed() ***************************************
 *
 * Checks to see if the screen's position changed.  If so this
 * function sets the apply_possible flag to FALSE.
 *
 **/

static void check_screen_pos_changed(CtkDisplayConfig *ctk_object)
{
    int old_dim[2];

    /* Cache the old position */
    old_dim[X] = ctk_object->cur_screen_pos[X];
    old_dim[Y] = ctk_object->cur_screen_pos[Y];

    /* Get the new position */
    get_cur_screen_pos(ctk_object);

    if (old_dim[X] != ctk_object->cur_screen_pos[X] ||
        old_dim[Y] != ctk_object->cur_screen_pos[Y]) {
        ctk_object->apply_possible = FALSE;
    }

} /* check_screen_pos_changed() */



/** layout_supports_depth_30() ***************************************
 *
 * Returns TRUE if all the GPUs in the layout that have screens
 * support depth 30.
 *
 **/

static gboolean layout_supports_depth_30(nvLayoutPtr layout)
{
    nvGpuPtr gpu;

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        if ((gpu->num_screens) && (!gpu->allow_depth_30)) {
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
    

    /* Register all Screen/Gpu events. */

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {

        if (!gpu->handle) continue;

        g_signal_connect(G_OBJECT(gpu->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);

        g_signal_connect(G_OBJECT(gpu->ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_MODE_SET_EVENT),
                         G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);

        for (screen = gpu->screens; screen; screen = screen->next) {

            if (!screen->handle) continue;
            
            g_signal_connect(G_OBJECT(screen->ctk_event),
                             CTK_EVENT_NAME(NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER),
                             G_CALLBACK(display_config_attribute_changed),
                             (gpointer) ctk_object);

            g_signal_connect(G_OBJECT(screen->ctk_event),
                             CTK_EVENT_NAME(NV_CTRL_ASSOCIATED_DISPLAY_DEVICES),
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
    

    /* Unregister all Screen/Gpu events. */

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {

        if (!gpu->handle) continue;

        /* Unregister all GPU events for this GPU */
        g_signal_handlers_disconnect_matched(G_OBJECT(gpu->ctk_event),
                                             G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                             0, // Signal ID
                                             0, // Signal Detail
                                             NULL, // Closure
                                             G_CALLBACK(display_config_attribute_changed),
                                             (gpointer) ctk_object);

        for (screen = gpu->screens; screen; screen = screen->next) {

            if (!screen->handle) continue;
  
            /* Unregister all screen events for this screen */
            g_signal_handlers_disconnect_matched(G_OBJECT(screen->ctk_event),
                                                 G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
                                                 0, // Signal ID
                                                 0, // Signal Detail
                                                 NULL, // Closure
                                                 G_CALLBACK(display_config_attribute_changed),
                                                 (gpointer) ctk_object);
        }
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
    nvGpuPtr gpu;
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
    for (gpu = screen->gpu->layout->gpus; gpu; gpu = gpu->next) {
        for (other = gpu->screens; other; other = other->next) {
            if (other == screen) continue;
            other->depth = screen->depth;
        }
    }

} /* consolidate_xinerama() */



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
    
    if (newline) printf("\n");
    printf("%s %s\n", prefix, msg);
    if (newline) printf("\n");
    
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
    nvLayoutPtr layout = screen->gpu->layout;
    gchar *metamode_strs = NULL;
    gchar *metamode_str;
    gchar *tmp;
    int metamode_idx;
    nvMetaModePtr metamode;
    int len = 0;
    int start_width;
    int start_height;

    int vendrel = NvCtrlGetVendorRelease(layout->handle);
    char *vendstr = NvCtrlGetServerVendor(layout->handle);
    int xorg_major;
    int xorg_minor;
    Bool longStringsOK;


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
                                                screen->cur_metamode_idx, 1);
        len = strlen(metamode_strs);
        start_width = screen->cur_metamode->edim[W];
        start_height = screen->cur_metamode->edim[H];
    } else {
        start_width = screen->metamodes->edim[W];
        start_height = screen->metamodes->edim[H];        
    }

    for (metamode_idx = 0, metamode = screen->metamodes;
         (metamode_idx < screen->num_metamodes) && metamode;
         metamode_idx++, metamode = metamode->next) {
        
        int metamode_len;

        /* Only write out metamodes that were specified by the user */
        if (!(metamode->source & METAMODE_SOURCE_USER)) continue;

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
            ((metamode->edim[W] > start_width) ||
             (metamode->edim[H] > start_height)))
            continue;
        
        metamode_str = screen_get_metamode_str(screen, metamode_idx, 1);

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
                nv_warning_msg(msg);
                g_free(msg);
                break;
            }
            
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_NONE,
                 msg);
            
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
 * - If Xinerama is enabled, query the XINERAMA_SCREEN_INFO.
 *
 * - If Xinerama is disabled, assume "right-of" orientation. (bleh!)
 *
 **/

static void assign_screen_positions(CtkDisplayConfig *ctk_object)
{
    nvGpuPtr gpu;
    nvScreenPtr prev_screen = NULL;
    nvScreenPtr screen;
    int xinerama;
    int initialize = 0;

    char *screen_info;
    ScreenInfo screen_parsed_info;
    ReturnStatus ret;


    /* If xinerama is enabled, we can get the screen size! */
    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_XINERAMA, &xinerama);
    if (ret != NvCtrlSuccess) {
        initialize = 1; /* Fallback to right-of positioning */
    }
    

    /* Setup screen positions */
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {

            screen_info = NULL;
            if (screen->handle && !initialize) {
                ret = NvCtrlGetStringAttribute
                    (screen->handle,
                     NV_CTRL_STRING_XINERAMA_SCREEN_INFO,
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
                XFree(screen_info);
                
            } else if (prev_screen) {
                /* Set this screen right of the previous */
                ctk_display_layout_set_screen_position
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     screen, CONF_ADJ_RIGHTOF, prev_screen, 0, 0);
            }

            prev_screen = screen;
        }
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

GtkWidget * create_validation_dialog(CtkDisplayConfig *ctk_object)
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
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
         NULL);
    
    /* Main horizontal box */
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       hbox, TRUE, TRUE, 5);

    /* Pack the information icon */
    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
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

    gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

    return dialog;

} /* create_validation_dialog() */



/** create_validation_apply_dialog() *********************************
 *
 * Creates the Validation Apply Information dialog widget.
 *
 **/

GtkWidget * create_validation_apply_dialog(CtkDisplayConfig *ctk_object)
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
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                       hbox, TRUE, TRUE, 5);

    /* Pack the information icon */
    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,
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

    gtk_widget_show_all(GTK_DIALOG(dialog)->vbox);

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
        gtk_widget_set_sensitive(ctk_object->btn_apply, True);
        ctk_object->forced_reset_allowed = FALSE;
    }

} /* user_changed_attributes() */



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
        gtk_widget_hide_all(ctk_object->obj_layout);
        gtk_widget_show(ctk_object->label_layout);
        return;
    }

    gtk_widget_hide(ctk_object->label_layout);
    gtk_widget_show_all(ctk_object->obj_layout);
} /* screen_size_changed() */



/** ctk_display_config_new() *****************************************
 *
 * Display Configuration widget creation.
 *
 **/

GtkWidget* ctk_display_config_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config)
{
    GObject *object;
    CtkDisplayConfig *ctk_object;

    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *notebook;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GdkScreen *screen;
    GtkWidget *label;
    GtkWidget *eventbox;
    GtkRequisition req;


    GtkWidget *menu;
    GtkWidget *menu_item;

    gchar *err_str = NULL;
    gchar *layout_str = NULL;
    gchar *sli_mode = NULL;
    ReturnStatus ret;

    /*
     * Get SLI Mode.  If SLI Mode is "Mosaic", do not
     * load this page
     *
     */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_SLI_MODE,
                                   &sli_mode);
    if (ret == NvCtrlSuccess && !g_ascii_strcasecmp(sli_mode, "Mosaic")) {
        XFree(sli_mode);
        return NULL;
    }

    if (sli_mode) {
        XFree(sli_mode);
    }

    /*
     * Create the ctk object
     *
     */

    object = g_object_new(CTK_TYPE_DISPLAY_CONFIG, NULL);
    ctk_object = CTK_DISPLAY_CONFIG(object);

    ctk_object->handle = handle;
    ctk_object->ctk_config = ctk_config;

    ctk_object->apply_possible = TRUE;

    ctk_object->reset_required = FALSE;
    ctk_object->forced_reset_allowed = TRUE;
    ctk_object->notify_user_of_reset = TRUE;
    ctk_object->ignore_reset_events = FALSE;

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
    ctk_object->layout = layout_load_from_server(handle, &err_str);

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
    ctk_object->obj_layout = ctk_display_layout_new(handle, ctk_config,
                                                    ctk_object->layout,
                                                    300, /* min width */
                                                    225, /* min height */
                                                    layout_selected_callback,
                                                    (void *)ctk_object,
                                                    layout_modified_callback,
                                                    (void *)ctk_object);

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

    /* Xinerama button */

    ctk_object->chk_xinerama_enabled =
        gtk_check_button_new_with_label("Enable Xinerama");
    ctk_config_set_tooltip(ctk_config, ctk_object->chk_xinerama_enabled,
                           __layout_xinerama_button_help);
    g_signal_connect(G_OBJECT(ctk_object->chk_xinerama_enabled), "toggled",
                     G_CALLBACK(xinerama_state_toggled),
                     (gpointer) ctk_object);


    /* Display model name */
    ctk_object->mnu_display_model = gtk_option_menu_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_model,
                           __dpy_model_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_model), "changed",
                     G_CALLBACK(display_modelname_changed),
                     (gpointer) ctk_object);

    /* Display configuration (Disabled, TwinView, Separate X screen */
    ctk_object->btn_display_config =
        gtk_button_new_with_label("Configure...");
    g_signal_connect(G_OBJECT(ctk_object->btn_display_config), "clicked",
                     G_CALLBACK(display_config_clicked),
                     (gpointer) ctk_object);
    ctk_object->txt_display_config = gtk_label_new("Disabled");
    gtk_misc_set_alignment(GTK_MISC(ctk_object->txt_display_config), 0.0f, 0.5f);


    /* Display configuration dialog */
    ctk_object->dlg_display_config = gtk_dialog_new_with_buttons
        ("Configure Display Device",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
         | GTK_DIALOG_NO_SEPARATOR,
         GTK_STOCK_OK,
         GTK_RESPONSE_ACCEPT,
         NULL);
    ctk_object->btn_display_config_cancel =
        gtk_dialog_add_button(GTK_DIALOG(ctk_object->dlg_display_config),
                              GTK_STOCK_CANCEL,
                              GTK_RESPONSE_CANCEL);
    ctk_object->rad_display_config_disabled =
        gtk_radio_button_new_with_label(NULL, "Disabled");
    ctk_object->rad_display_config_xscreen =
        gtk_radio_button_new_with_label_from_widget
        (GTK_RADIO_BUTTON(ctk_object->rad_display_config_disabled),
         "Separate X screen");
    ctk_object->rad_display_config_twinview =
        gtk_radio_button_new_with_label_from_widget
        (GTK_RADIO_BUTTON(ctk_object->rad_display_config_disabled),
         "TwinView");
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_display_config),
                             FALSE);

    /* Display disable dialog */
    ctk_object->txt_display_disable = gtk_label_new("");
    ctk_object->dlg_display_disable = gtk_dialog_new_with_buttons
        ("Disable Display Device",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
         | GTK_DIALOG_NO_SEPARATOR,
         NULL);
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
    ctk_object->mnu_display_resolution = gtk_option_menu_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_resolution,
                           __dpy_resolution_mnu_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_resolution), "changed",
                     G_CALLBACK(display_resolution_changed),
                     (gpointer) ctk_object);


    /* Display refresh */
    ctk_object->mnu_display_refresh = gtk_option_menu_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_refresh,
                            __dpy_refresh_mnu_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_refresh), "changed",
                     G_CALLBACK(display_refresh_changed),
                     (gpointer) ctk_object);

    /* Display modeline modename */
    ctk_object->txt_display_modename = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ctk_object->txt_display_modename), TRUE);

    /* Display Position Type (Absolute/Relative Menu) */
    ctk_object->mnu_display_position_type = gtk_option_menu_new();
    menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_label("Absolute");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Right of");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Left of");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Above");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Below");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Clones");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_type), menu);
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_display_position_type,
                           __dpy_position_type_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_position_type),
                     "changed", G_CALLBACK(display_position_type_changed),
                     (gpointer) ctk_object);

    /* Display Position Relative (Display device to be relative to) */
    ctk_object->mnu_display_position_relative = gtk_option_menu_new();
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

    /* Display Panning */
    ctk_object->txt_display_panning = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_display_panning,
                           __dpy_panning_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_panning), "activate",
                     G_CALLBACK(display_panning_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_display_panning), "focus-out-event",
                     G_CALLBACK(display_panning_focus_out),
                     (gpointer) ctk_object);

    /* X screen number */
    ctk_object->txt_screen_num = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ctk_object->txt_screen_num), TRUE);
    gtk_misc_set_alignment(GTK_MISC(ctk_object->txt_screen_num), 0.0f, 0.5f);

    /* X screen virtual size */
    ctk_object->txt_screen_virtual_size = gtk_entry_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->txt_screen_virtual_size,
                           __screen_virtual_size_help);
    g_signal_connect(G_OBJECT(ctk_object->txt_screen_virtual_size), "activate",
                     G_CALLBACK(screen_virtual_size_activate),
                     (gpointer) ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->txt_screen_virtual_size),
                     "focus-out-event",
                     G_CALLBACK(screen_virtual_size_focus_out),
                     (gpointer) ctk_object);

    /* X screen depth */
    ctk_object->mnu_screen_depth = gtk_option_menu_new();
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_screen_depth,
                           __screen_depth_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_screen_depth), "changed",
                     G_CALLBACK(screen_depth_changed),
                     (gpointer) ctk_object);

    /* Screen Position Type (Absolute/Relative Menu) */
    ctk_object->mnu_screen_position_type = gtk_option_menu_new();
    menu = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_label("Absolute");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Right of");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Left of");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Above");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    menu_item = gtk_menu_item_new_with_label("Below");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    // XXX Add better support for this later.
    //menu_item = gtk_menu_item_new_with_label("Relative to");
    //gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_type), menu);
    ctk_config_set_tooltip(ctk_config, ctk_object->mnu_screen_position_type,
                           __screen_position_type_help);
    g_signal_connect(G_OBJECT(ctk_object->mnu_screen_position_type),
                     "changed", G_CALLBACK(screen_position_type_changed),
                     (gpointer) ctk_object);

    /* Screen Position Relative (Screen to be relative to) */
    ctk_object->mnu_screen_position_relative = gtk_option_menu_new();
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
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
         | GTK_DIALOG_NO_SEPARATOR,
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
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
         | GTK_DIALOG_NO_SEPARATOR,
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
    gtk_widget_set_sensitive(ctk_object->btn_apply, False);
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



    /****
     *
     * Pack the widgets
     *
     ***/

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

        /* Xinerama checkbox */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->chk_xinerama_enabled,
                           FALSE, FALSE, 0);
    }
    

    /* Panel for the notebook sections */
    notebook = gtk_notebook_new();
    ctk_object->notebook = notebook;
    gtk_box_pack_start(GTK_BOX(ctk_object), notebook, FALSE, FALSE, 0);


    { /* Display page */
        GtkWidget *longest_hbox;
        
        /* Generate the major vbox for the display section */
        vbox = gtk_vbox_new(FALSE, 5);
        ctk_object->display_page = vbox;
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

        /* Display Configuration */
        hbox = gtk_hbox_new(FALSE, 5);
        /* XXX Pack widget later.  Create it here so we can get its size */
        longest_hbox = hbox;
        label = gtk_label_new("Configuration:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_size_request(label, &req);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_config,
                           FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_display_config,
                         FALSE, FALSE, 0);

        /* Display model name */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
        label = gtk_label_new("Model:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_model,
                           TRUE, TRUE, 0);

        /* Pack the display configuration line */
        gtk_box_pack_start(GTK_BOX(vbox), longest_hbox, FALSE, TRUE, 0);

        /* Display resolution and refresh dropdowns */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
        label = gtk_label_new("Resolution:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_resolution,
                           TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_display_refresh,
                           TRUE, TRUE, 0);
        ctk_object->box_display_resolution = hbox;

        /* Modeline modename info */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Mode Name:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_modename,
                           FALSE, FALSE, 0);
        ctk_object->box_display_modename = hbox;

        /* Display positioning */
        label = gtk_label_new("Position:");
        hbox = gtk_hbox_new(FALSE, 5);

        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);

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

        /* Display panning text entry */
        label = gtk_label_new("Panning:");
        hbox = gtk_hbox_new(FALSE, 5);

        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);

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

        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->chk_primary_display,
                           TRUE, TRUE, 0);

        /* Up the object ref count to make sure that the page and its widgets
         * do not get freed if/when the page is removed from the notebook.
         */
        g_object_ref(ctk_object->display_page);
        gtk_widget_show_all(ctk_object->display_page);

    } /* Display sub-section */
    

    { /* X screen page */

        /* Generate the major vbox for the display section */
        vbox = gtk_vbox_new(FALSE, 5);
        ctk_object->screen_page = vbox;
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

        /* X screen number */      
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
        label = gtk_label_new("Screen Number:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_size_request(label, &req);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_screen_num,
                           TRUE, TRUE, 0);

        /* X screen virtual size */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Virtual Size:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5); 
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_screen_virtual_size,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_virtual_size = hbox;

        /* X screen depth dropdown */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Color Depth:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5); 
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_screen_depth,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_depth = hbox;

        /* X screen positioning */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Position:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
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
        hbox  = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("MetaMode:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
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
    
 
    { /* Buttons */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 0);

        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_reset,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_advanced,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_probe,
                         FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_apply,
                         FALSE, FALSE, 0);

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 0);

        gtk_box_pack_end(GTK_BOX(hbox), ctk_object->btn_save,
                         FALSE, FALSE, 0);
    }


    { /* Dialogs */

        /* Display Configuration Dialog */
        label = gtk_label_new("How should this display device be configured?");
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_config)->vbox),
             hbox, TRUE, TRUE, 20);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_config)->vbox),
             ctk_object->rad_display_config_disabled, FALSE, FALSE, 0);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_config)->vbox),
             ctk_object->rad_display_config_xscreen, FALSE, FALSE, 0);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_config)->vbox),
             ctk_object->rad_display_config_twinview, FALSE, FALSE, 0);
        gtk_widget_show_all(GTK_DIALOG(ctk_object->dlg_display_config)->vbox);

        /* Display Disable Dialog */
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_disable,
                           FALSE, FALSE, 20);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_disable)->vbox),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(GTK_DIALOG(ctk_object->dlg_display_disable)->vbox);

        /* Reset Confirm Dialog */
        label = gtk_label_new("Do you really want to reset the "
                              "configuration?");
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_reset_confirm)->vbox),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(GTK_DIALOG(ctk_object->dlg_reset_confirm)->vbox);

        /* Apply Confirm Dialog */
        hbox = gtk_hbox_new(TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_confirm,
                           TRUE, TRUE, 20);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_display_confirm)->vbox),
             hbox, TRUE, TRUE, 20);
        gtk_widget_show_all(GTK_DIALOG(ctk_object->dlg_display_confirm)->vbox);
    }


    /* Show the GUI */
    gtk_widget_show_all(GTK_WIDGET(ctk_object));


    /* Setup the GUI */
    setup_layout_frame(ctk_object);
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);


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
                  "be resized by holding shift while dragging.");
    ctk_help_heading(b, &i, "Layout Hidden Label");
    ctk_help_para(b, &i, __layout_hidden_label_help);
    ctk_help_heading(b, &i, "Enable Xinerama");
    ctk_help_para(b, &i, "%s  This setting is only available when multiple "
                  "X screens are present.", __layout_xinerama_button_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Display Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the currently selected display device.");
    ctk_help_heading(b, &i, "Model");
    ctk_help_para(b, &i, "The Model name is the name of the display device.");
    ctk_help_heading(b, &i, "Select Display");
    ctk_help_para(b, &i, __dpy_model_help);
    ctk_help_heading(b, &i, "Resolution");
    ctk_help_para(b, &i, __dpy_resolution_mnu_help);
    ctk_help_heading(b, &i, "Refresh");
    ctk_help_para(b, &i, "The Refresh drop-down is to the right of the "
                  "Resolution drop-down.  %s", __dpy_refresh_mnu_help);
    ctk_help_heading(b, &i, "Mode Name");
    ctk_help_para(b, &i, "The Mode name is the name of the modeline that is "
                  "currently chosen for the selected display device.  "
                  "This is only available when advanced view is enabled.");
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, __dpy_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, __dpy_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, __dpy_position_offset_help);
    ctk_help_heading(b, &i, "Panning");
    ctk_help_para(b, &i, "%s  This is only available when advanced "
                  "view is enabled.", __dpy_panning_help);
    ctk_help_heading(b, &i, "Primary Display");
    ctk_help_para(b, &i, __dpy_primary_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "X Screen Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the currently selected X screen.");
    ctk_help_heading(b, &i, "Virtual Size");
    ctk_help_para(b, &i, "%s  The Virtual screen size must be at least "
                  "304x200, and the width must be a multiple of 8.",
                  __screen_virtual_size_help);
    ctk_help_heading(b, &i, "Color Depth");
    ctk_help_para(b, &i, __screen_depth_help);
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, __screen_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, __screen_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, __screen_position_offset_help);
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
    ctk_help_heading(b, &i, "Buttons");
    ctk_help_heading(b, &i, "Apply");
    ctk_help_para(b, &i, "%s  Note that not all settings can be applied to an "
                  "active X server; these require restarting the X server "
                  "after saving the desired settings to the X configuration "
                  "file.  Examples of such settings include changing the "
                  "position of any X screen, adding/removing an X screen, and "
                  "changing the X screen color depth.", __apply_button_help);
    ctk_help_heading(b, &i, "Detect Displays");
    ctk_help_para(b, &i, __detect_displays_button_help);
    ctk_help_heading(b, &i, "Advanced/Basic...");
    ctk_help_para(b, &i, "%s  The Basic view modifies the currently active "
                  "MetaMode for an X screen, while the advanced view exposes "
                  "all the MetaModes available on an X screen, and lets you "
                  "modify each of them.", __advanced_button_help);
    ctk_help_heading(b, &i, "Reset");
    ctk_help_para(b, &i, __reset_button_help);
    ctk_help_heading(b, &i, "Save to X Configuration File");
    ctk_help_para(b, &i, __save_button_help);

    ctk_help_finish(b);

    return b;

} /* ctk_display_config_create_help() */



/* Widget setup & helper functions ***********************************/


/** setup_layout_frame() *********************************************
 *
 * Sets up the layout frame to reflect the currently selected layout.
 *
 **/

static void setup_layout_frame(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    int num_screens;
    GdkScreen *s;

    /*
     * Hide/Shows the layout widget based on the current screen size.
     * If the screen is too small, the layout widget is hidden and a
     * message is shown instead. 
     */
    s = gtk_widget_get_screen(GTK_WIDGET(ctk_object));
    screen_size_changed(s, ctk_object);

    /* Only allow Xinerama when there are multiple X screens */
    num_screens = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        num_screens += gpu->num_screens;
    }

    /* Unselect Xinerama if only one (or no) X screen */
    if (num_screens <= 1) {
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
        gtk_label_set(GTK_LABEL(ctk_object->txt_display_modename), "");
        gtk_widget_set_sensitive(ctk_object->box_display_modename, FALSE);
        return;
    }
    gtk_widget_set_sensitive(ctk_object->box_display_modename, TRUE);


    gtk_label_set(GTK_LABEL(ctk_object->txt_display_modename),
                  display->cur_mode->modeline->data.identifier);

} /* setup_display_modename() */



/** setup_display_config() *******************************************
 *
 * Updates the text of the configure button to reflect the current
 * setting of the selected display.
 *
 **/

static void setup_display_config(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display) return;

    if (!display->screen) {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_config),
                          "Disabled");
    } else if (display->screen->num_displays == 1) {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_config),
                           "Separate X screen");
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_config),
                           "TwinView");
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
    GtkWidget *menu;
    GtkWidget *menu_item;
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
        (nvModeLinePtr *)calloc(1, display->num_modelines
                                * sizeof(nvModeLinePtr));
    if (!ctk_object->refresh_table) {
        goto fail;
    }


    /* Generate the refresh dropdown */
    menu = gtk_menu_new();


    /* Special case the 'nvidia-auto-select' mode. */
    if (IS_NVIDIA_DEFAULT_MODE(cur_modeline)) {
        menu_item = gtk_menu_item_new_with_label("Auto");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
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
        int is_doublescan;
        int is_interlaced;
        

        /* Ignore modelines of different resolution */
        if (modeline->data.hdisplay != cur_modeline->data.hdisplay ||
            modeline->data.vdisplay != cur_modeline->data.vdisplay) {
            continue;
        }

        /* Ignore special modes */
        if (IS_NVIDIA_DEFAULT_MODE(modeline)) {
            continue;
        }

        is_doublescan = (modeline->data.flags & V_DBLSCAN);
        is_interlaced = (modeline->data.flags & V_INTERLACE);

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


        /* Add "DoubleScan" and "Interlace" information */
        if (g_ascii_strcasecmp(name, "Auto")) {
            gchar *extra = NULL;
            gchar *tmp;

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
        menu_item = gtk_menu_item_new_with_label(name);
        g_free(name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        ctk_object->refresh_table[ctk_object->refresh_table_len++] = modeline;
    }

    


    /* Setup the menu and select the current modeline */
    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_display_refresh),
                                    G_CALLBACK(display_refresh_changed),
                                    (gpointer) ctk_object);
    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_refresh), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(ctk_object->mnu_display_refresh), cur_idx);
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


    /* Update the nodename label */
    setup_display_modename(ctk_object);
    return;


    /* Handle failures */
 fail:
    gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, False);

    setup_display_modename(ctk_object);

} /* setup_display_refresh_dropdown() */



/** setup_display_resolution_dropdown() ******************************
 *
 * Generates the resolution dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_resolution_dropdown(CtkDisplayConfig *ctk_object)
{
    GtkWidget *menu;
    GtkWidget *menu_item;

    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    nvModeLinePtr  modeline;
    nvModeLinePtr  modelines;
    nvModeLinePtr  cur_modeline;

    int cur_idx = 0;  /* Currently selected modeline (resolution) */



    /* Get selection information */
    if (!display || !display->screen || !display->cur_mode) {
        gtk_widget_hide(ctk_object->box_display_resolution);
        return;
    }
    gtk_widget_show(ctk_object->box_display_resolution);
    gtk_widget_set_sensitive(ctk_object->box_display_resolution, TRUE);

    
    cur_modeline = display->cur_mode->modeline;

    /* Create the modeline lookup table for the dropdown */
    if (ctk_object->resolution_table) {
        free(ctk_object->resolution_table);
        ctk_object->resolution_table_len = 0;
    }
    ctk_object->resolution_table =
        (nvModeLinePtr *)calloc(1, (display->num_modelines +1)
                                *sizeof(nvModeLinePtr));
    if (!ctk_object->resolution_table) {
        goto fail;
    }


    /* Start the menu generation */
    menu = gtk_menu_new();


    /* Add the off mode */
    menu_item = gtk_menu_item_new_with_label("Off");
    if (display->screen->num_displays <= 1) {
        gtk_widget_set_sensitive(menu_item, False);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    ctk_object->resolution_table[ctk_object->resolution_table_len++] = NULL;


    /* Add the 'nvidia-auto-select' modeline */
    modelines = display->modelines;
    if (IS_NVIDIA_DEFAULT_MODE(modelines)) {
        menu_item = gtk_menu_item_new_with_label("Auto");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        ctk_object->resolution_table[ctk_object->resolution_table_len++] =
            modelines;
        modelines = modelines->next;
    }

    /* Set the selected modeline index */
    if (cur_modeline) {
        cur_idx = 1; /* Modeline is set, start off as 'nvidia-auto-select' */
    } else {
        cur_idx = 0; /* Modeline not set, start off as 'off'. */
    }
    

    /* Generate the resolution menu */
    modeline = modelines;
    while (modeline) {
        nvModeLinePtr m;
        gchar *name;
        
        /* Find the first resolution that matches the current res W & H */
        m = modelines;
        while (m != modeline) {
            if (modeline->data.hdisplay == m->data.hdisplay &&
                modeline->data.vdisplay == m->data.vdisplay) {
                break;
            }
            m = m->next;
        }

        /* Add resolution if it is the first of its kind */
        if (m == modeline) {

            /* Set the current modeline idx if not already set by default */
            if (cur_modeline) {
                if (!IS_NVIDIA_DEFAULT_MODE(cur_modeline) &&
                    cur_modeline->data.hdisplay == modeline->data.hdisplay &&
                    cur_modeline->data.vdisplay == modeline->data.vdisplay) {
                    cur_idx = ctk_object->resolution_table_len;
                }
            }

            name = g_strdup_printf("%dx%d", modeline->data.hdisplay,
                                   modeline->data.vdisplay);
            menu_item = gtk_menu_item_new_with_label(name);
            g_free(name);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            gtk_widget_show(menu_item);
            ctk_object->resolution_table[ctk_object->resolution_table_len++] =
                modeline;
        }
        modeline = modeline->next;
    }
    

    /* Setup the menu and select the current mode */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution), menu);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution), cur_idx);
    ctk_object->last_resolution_idx = cur_idx;

    /* If dropdown has only one item, disable menu selection */
    if (ctk_object->resolution_table_len > 1) {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, True);
    } else {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, False);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

     /* Update refresh dropdown */
    setup_display_refresh_dropdown(ctk_object);
    return;



    /* Handle failures */
 fail:

    gtk_option_menu_remove_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution));

    gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, False);

    setup_display_refresh_dropdown(ctk_object);

} /* setup_display_resolution_dropdown() */



/** generate_display_modelname_dropdown() ********************************
 *
 * Generate display modelname dropdown menu.
 *
 **/

static GtkWidget* generate_display_modelname_dropdown
              (CtkDisplayConfig *ctk_object, int *cur_idx)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    nvGpuPtr gpu;
    nvDisplayPtr display;
    int display_count = 0;
    nvDisplayPtr d = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    /* Create the display modelname lookup table for the dropdown */
    if (ctk_object->display_model_table) {
        free(ctk_object->display_model_table);
    }
    ctk_object->display_model_table_len = 0;
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        display_count += gpu->num_displays;
    }

    ctk_object->display_model_table =
        (nvDisplayPtr *)calloc(1, display_count * sizeof(nvDisplayPtr));

    if (!ctk_object->display_model_table) {
        gtk_option_menu_remove_menu
            (GTK_OPTION_MENU(ctk_object->mnu_display_model));
        gtk_widget_set_sensitive(ctk_object->mnu_display_model, False);
        return NULL;
    }

    /* Generate the popup menu */
    menu = gtk_menu_new();
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        for (display = gpu->displays; display; display = display->next) {
            gchar *str, *type;
            if (d == display) {
                *cur_idx = ctk_object->display_model_table_len;
            }
            /* Setup the menu item text */
            type = display_get_type_str(display->device_mask, 0);
            str = g_strdup_printf("%s (%s on GPU-%d)",
                                  display->name, type,
                                  NvCtrlGetTargetId(gpu->handle));
            menu_item = gtk_menu_item_new_with_label(str);
            g_free(str);
            g_free(type);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            gtk_widget_show(menu_item);
            ctk_object->display_model_table
                [ctk_object->display_model_table_len++] = display;
        }
    }

    return menu;

} /* generate_display_modelname_dropdown() */



/** setup_display_modelname_dropdown() **************************
 *
 * Setup display modelname dropdown menu.
 *
 **/

static void setup_display_modelname_dropdown(CtkDisplayConfig *ctk_object)
{
    GtkWidget *menu;
    int cur_idx = 0;  /* Currently selected display */
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* If no display is selected or there is no screen, hide the frame */
    if (!display) {
        gtk_widget_set_sensitive(ctk_object->mnu_display_model, False);
        gtk_widget_hide(ctk_object->mnu_display_model);
        return;
    }

    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->mnu_display_model, True);

    menu = generate_display_modelname_dropdown(ctk_object, &cur_idx);

    /* If menu not generated return */
    if (!menu) {
        return;
    }
    /* Setup the menu and select the current model */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_model),
         G_CALLBACK(display_modelname_changed), (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_model), menu);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_model), cur_idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_model),
         G_CALLBACK(display_modelname_changed), (gpointer) ctk_object);

} /* setup_display_modelname_dropdown() */



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

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_type),
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
    GtkWidget *menu;
    GtkWidget *menu_item;

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
        (nvDisplayPtr *)calloc(1, ctk_object->display_position_table_len *
                               sizeof(nvDisplayPtr));
    
    if (!ctk_object->display_position_table) {
        goto fail;
    }


    /* Generate the lookup table and display dropdown */
    idx = 0;
    selected_idx = 0;
    menu = gtk_menu_new();
    for (relative_to = display->gpu->displays;
         relative_to;
         relative_to = relative_to->next) {

        if (relative_to == display || relative_to->screen != display->screen) {
            continue;
        }

        if (relative_to == display->cur_mode->relative_to) {
            selected_idx = idx;
        }

        ctk_object->display_position_table[idx] = relative_to;
       
        menu_item = gtk_menu_item_new_with_label(relative_to->name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        idx++;
    }


    /* Set the menu and the selected display */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_position_relative),
         G_CALLBACK(display_position_relative_changed), (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_relative), menu);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_relative),
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
                              mode->dim[X] - mode->metamode->edim[X],
                              mode->dim[Y] - mode->metamode->edim[Y]);

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


    /* Hide the position box if this screen only has one display */
    if (!display || !display->screen || display->screen->num_displays <= 1) {
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
    if (!display || !display->screen || display->screen->num_displays <= 1) {
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
    nvLayoutPtr layout;
    nvDisplayPtr display;
    nvModePtr mode;

    layout = ctk_object->layout;

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
    tmp_str = g_strdup_printf("%dx%d", mode->pan[W], mode->pan[H]);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_panning),
                       tmp_str);
    g_free(tmp_str);

} /* setup_display_panning */



/** setup_display_page() ********************************************
 *
 * Sets up the display frame to reflect the currently selected
 * display. 
 *
 **/

static void setup_display_page(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gint page_num;
    GtkWidget *tab_label;


    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(ctk_object->notebook),
                                     ctk_object->display_page);

    /* If no display is selected, remove the display page */
    if (!display) {
        gtk_widget_set_sensitive(ctk_object->display_page, False);
        if (page_num >= 0) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(ctk_object->notebook),
                                     page_num);
        }
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->display_page, True);
    

    /* Setup the display modelname dropdown */
    setup_display_modelname_dropdown(ctk_object);


    /* Setup the seleted mode modename */
    setup_display_config(ctk_object);


    /* Setup the seleted mode modename */
    setup_display_modename(ctk_object);


    /* Setup display resolution menu */
    setup_display_resolution_dropdown(ctk_object);
    

    /* Setup position */
    setup_display_position(ctk_object);


    /* Setup panning */
    setup_display_panning(ctk_object);


    /* Setup first display */
    setup_primary_display(ctk_object);


    /* Make sure the page has been added to the notebook */
    if (page_num < 0) {
        tab_label = gtk_label_new("Display");
        gtk_notebook_prepend_page(GTK_NOTEBOOK(ctk_object->notebook),
                                  ctk_object->display_page, tab_label);
    }

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


    /* Only show this box for no-scannout screens */
    if (!screen || !screen->no_scanout) {
        gtk_widget_hide(ctk_object->box_screen_virtual_size);
        return;
    }
    gtk_widget_show(ctk_object->box_screen_virtual_size);


    /* Update the virtual size text */
    tmp_str = g_strdup_printf("%dx%d", screen->dim[W], screen->dim[H]);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_screen_virtual_size),
                       tmp_str);
    g_free(tmp_str);
    
} /* setup_screen_virtual_size() */



/** setup_screen_depth_dropdown() ************************************
 *
 * Generates the color depth dropdown based on the currently selected
 * display device.
 *
 **/

static void setup_screen_depth_dropdown(CtkDisplayConfig *ctk_object)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    int cur_idx;
    int screen_depth_table_len = 0;
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
    ctk_object->screen_depth_table =
        (char *) malloc(sizeof(char) * SCREEN_DEPTH_COUNT);

    menu  = gtk_menu_new();

    /* If Xinerama is enabled, only allow depth 30 if all
     * gpu/screens have support for depth 30.
     */

    if (ctk_object->layout->xinerama_enabled) {
        add_depth_30_option = layout_supports_depth_30(screen->gpu->layout);
    } else {
        add_depth_30_option = screen->gpu->allow_depth_30;
    }

    if (add_depth_30_option) {
        menu_item = gtk_menu_item_new_with_label
            ("1.1 Billion Colors (Depth 30) - Experimental");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        ctk_object->screen_depth_table[screen_depth_table_len++] = 30;
    }
    menu_item = gtk_menu_item_new_with_label("16.7 Million Colors (Depth 24)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    ctk_object->screen_depth_table[screen_depth_table_len++] = 24;

    menu_item = gtk_menu_item_new_with_label("65,536 Colors (Depth 16)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    ctk_object->screen_depth_table[screen_depth_table_len++] = 16;

    menu_item = gtk_menu_item_new_with_label("32,768 Colors (Depth 15)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    ctk_object->screen_depth_table[screen_depth_table_len++] = 15;

    menu_item = gtk_menu_item_new_with_label("256 Colors (Depth 8)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    ctk_object->screen_depth_table[screen_depth_table_len++] = 8;

    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_screen_depth),
                                    G_CALLBACK(screen_depth_changed),
                                    (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), menu);
    
    for (cur_idx = 0; cur_idx < SCREEN_DEPTH_COUNT; cur_idx++) {
        if (screen->depth == ctk_object->screen_depth_table[cur_idx]) {
            gtk_option_menu_set_history
                (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), cur_idx);
        }
    }

    g_signal_handlers_unblock_by_func(G_OBJECT(ctk_object->mnu_screen_depth),
                                      G_CALLBACK(screen_depth_changed),
                                      (gpointer) ctk_object);

    gtk_widget_show(ctk_object->box_screen_depth);

    return;

} /* setup_screen_depth_dropdown() */



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

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_type),
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
    nvScreenPtr screen;
    nvScreenPtr relative_to;
    nvGpuPtr gpu;
    int idx;
    int selected_idx;
    GtkWidget *menu;
    GtkWidget *menu_item;

    screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    if (!screen) {
        goto fail;
    }


    /* Count the number of screens */
    ctk_object->screen_position_table_len = 0;
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        ctk_object->screen_position_table_len += gpu->num_screens;
    }

    /* Don't count the current screen */
    if (ctk_object->screen_position_table_len >= 1) {
        ctk_object->screen_position_table_len--;
    }

    /* Allocate the screen lookup table for the dropdown */
    if (ctk_object->screen_position_table) {
        free(ctk_object->screen_position_table);
    }
    ctk_object->screen_position_table =
        (nvScreenPtr *)calloc(1, ctk_object->screen_position_table_len *
                              sizeof(nvScreenPtr));
    
    if (!ctk_object->screen_position_table) {
        goto fail;
    }


    /* Generate the lookup table and screen dropdown */
    idx = 0;
    selected_idx = 0;
    menu = gtk_menu_new();
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        for (relative_to = gpu->screens;
             relative_to;
             relative_to = relative_to->next) {

            gchar *tmp_str;

            if (relative_to == screen) continue;

            if (relative_to == screen->relative_to) {
                selected_idx = idx;
            }
            
            ctk_object->screen_position_table[idx] = relative_to;

            tmp_str = g_strdup_printf("X screen %d",
                                      relative_to->scrnum);
            menu_item = gtk_menu_item_new_with_label(tmp_str);
            g_free(tmp_str);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            gtk_widget_show(menu_item);
            idx++;
        }
    }


    /* Set the menu and the selected display */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_screen_position_relative),
         G_CALLBACK(screen_position_relative_changed), (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_relative), menu);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_relative),
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
    tmp_str = g_strdup_printf("%+d%+d", screen->dim[X], screen->dim[Y]);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_screen_position_offset),
                       tmp_str);

    g_free(tmp_str);

} /* setup_screen_position_offset() */



/** setup_screen_position() ******************************************
 *
 * Sets up the the screen position section to reflect the position
 * settings for the currently selected screen.
 *
 **/

static void setup_screen_position(CtkDisplayConfig *ctk_object)
{
    nvGpuPtr gpu;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int num_screens;


    /* Count the number of screens */
    num_screens = 0;
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        num_screens += gpu->num_screens;
    }


    /* Hide the position box if there is only one screen */
    if (!screen || num_screens <= 1) {
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
    gchar *tmp;
    gint page_num;
    GtkWidget *tab_label;


    page_num = gtk_notebook_page_num(GTK_NOTEBOOK(ctk_object->notebook),
                                     ctk_object->screen_page);


    /* If there is no display or no screen selected, remove the screen page */
    if (!screen) {
        gtk_widget_set_sensitive(ctk_object->screen_page, False);
        if (page_num >= 0) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(ctk_object->notebook),
                                     page_num);
        }
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->screen_page, True);


    /* Setup the screen number */
    tmp = g_strdup_printf("%d", screen->scrnum);
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_screen_num), tmp);
    g_free(tmp);
    

    /* Setup screen (virtual) size */
    setup_screen_virtual_size(ctk_object);


    /* Setup depth menu */
    setup_screen_depth_dropdown(ctk_object);


    /* Setup position */
    setup_screen_position(ctk_object);


    /* Setup metamode menu */
    setup_screen_metamode(ctk_object);


    /* Make sure the page has been added to the notebook */
    if (page_num < 0) {
        tab_label = gtk_label_new("X Screen");
        gtk_notebook_append_page(GTK_NOTEBOOK(ctk_object->notebook),
                                 ctk_object->screen_page, tab_label);
    }
    
} /* setup_screen_page() */



/** validation_fix_crowded_metamodes() *******************************
 *
 * Goes through each screen's metamodes and ensures that at most
 * two display devices are active (have a modeline set) per metamode.
 * This function also checks to make sure that there is at least
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


    /* Verify each metamode with the metamodes that come before it */
    for (i = 0; i < screen->num_metamodes; i++) {
        
        /* Keep track of the first mode in case we need to assign
         * a default resolution
         */
        first_mode = NULL;

        /* Count the number of display devices that have a mode
         * set for this metamode.  NULL out the modes of extra
         * display devices once we've counted 2 display devices
         * that have a (non NULL) mode set.
         */
        num = 0;
        for (display = screen->gpu->displays;
             display;
             display = display->next) {

            if (display->screen != screen) continue;

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
            if (num > 2) {
                ctk_display_layout_set_mode_modeline
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     mode, NULL);

                nv_info_msg(TAB, "Setting display device '%s' as Off "
                            "for MetaMode %d on Screen %d.  (There are "
                            "already two active display devices for this "
                            "MetaMode.", display->name, i, screen->scrnum);
            }
        }

        /* Handle the case where a metamode has no active display device */
        if (!num) {

            /* There are other modelines, so we can safely delete this one */
            if (screen->num_metamodes > 1) {

                /* Delete the metamode */
                ctk_display_layout_delete_screen_metamode
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, i);

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
                     first_mode->display->modelines);

                nv_info_msg(TAB, "Activating display device '%s' for MetaMode "
                            "%d on Screen %d.  (Minimally, a Screen must have "
                            "one MetaMode with at least one active display "
                            "device.)",
                            first_mode->display->name, i, screen->scrnum);
            }
        }
    }

    return 1;

} /* validation_fix_crowded_metamodes() */



/** validation_fix_crowded_metamodes() *******************************
 *
 * Goes through each screen's metamodes and ensures that each
 * metamode string is unique.  If a metamode string is not unique,
 * the duplicate metamode is removed.
 *
 **/

static gint validation_remove_dupe_metamodes(CtkDisplayConfig *ctk_object,
                                            nvScreenPtr screen)
{
    int i, j;
    char *metamode_str;

    /* Verify each metamode with the metamodes that come before it */
    for (i = 1; i < screen->num_metamodes; i++) {

        metamode_str = screen_get_metamode_str(screen, i, 0);
        for (j = 0; j < i; j++) {
            char *tmp;
            tmp = screen_get_metamode_str(screen, j, 0);
            
            /* Remove duplicates */
            if (!strcmp(metamode_str, tmp)) {
                
                /* Delete the metamode */
                ctk_display_layout_delete_screen_metamode
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, i);
                
                nv_info_msg(TAB, "Removed MetaMode %d on Screen %d (Is "
                            "Duplicate of MetaMode %d)\n", i+1, screen->scrnum,
                            j+1);
                g_free(tmp);
                i--; /* Check the new metamode in i'th position */
                break;
            }
            g_free(tmp);
        }
        g_free(metamode_str);
    }

    return 1;

} /* validation_remove_dupe_metamodes() */



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
    status &= validation_remove_dupe_metamodes(ctk_object, screen);

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
    nvGpuPtr gpu;
    nvScreenPtr screen;
    gint success = 1;


    /* Auto fix each screen */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            if (!validation_auto_fix_screen(ctk_object, screen)) {
                success = 0;
                break;
            }
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
 * adhear to the following:
 *
 * - Have at least 1 display device activated for all metamodes.
 *
 * - Have at most 2 display devices activated for all metamodes.
 *
 * - All metamodes must be unique.
 *
 * - All metamodes must have a coherent offset (The top left corner
 *   of the bounding box of all the metamodes must be the same.)
 *
 * If the screen is found to be in an invalid state, a string
 * describing the problem is returned.  This string should be freed
 * by the user when done with it.
 *
 **/

static gchar * validate_screen(nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvModePtr mode;
    int i, j;
    int num_displays;
    char *metamode_str;
    gchar *err_str = NULL;
    gchar *tmp;
    gchar *tmp2;

    gchar bullet[8]; // UTF8 Bullet string
    int len;



    /* Convert the Unicode "Bullet" Character into a UTF8 string */
    len = g_unichar_to_utf8(0x2022, bullet);
    bullet[len] = '\0';


    for (i = 0; i < screen->num_metamodes; i++) {

        /* Count the number of display devices used in the metamode */
        num_displays = 0;
        for (display = screen->gpu->displays;
             display;
             display = display->next) {

            if (display->screen != screen) continue;
            
            mode = display->modes;
            for (j = 0; j < i; j++) {
                mode = mode->next;
            }
            if (mode->modeline) {
                num_displays++;
            }
        }


        /* There must be at least one display active in the metamode. */
        if (!num_displays) {
            tmp = g_strdup_printf("%s MetaMode %d of Screen %d  does not have "
                                  "an active display devices.\n\n",
                                  bullet, i+1, screen->scrnum);
            tmp2 = g_strconcat((err_str ? err_str : ""), tmp, NULL);
            g_free(err_str);
            g_free(tmp);
            err_str = tmp2;
        }


        /* There can be at most two displays active in the metamode. */
        if (num_displays > 2) {
            tmp = g_strdup_printf("%s MetaMode %d of Screen %d has more than "
                                  "two active display devices.\n\n",
                                  bullet, i+1, screen->scrnum);
            tmp2 = g_strconcat((err_str ? err_str : ""), tmp, NULL);
            g_free(err_str);
            g_free(tmp);
            err_str = tmp2;
        }
        

        /* Verify that the metamode is unique */
        metamode_str = screen_get_metamode_str(screen, i, 0);
        for (j = 0; j < i; j++) {
            char *tmp = screen_get_metamode_str(screen, j, 0);

            /* Make sure the metamode is unique */
            if (!strcmp(metamode_str, tmp)) {
                g_free(tmp);
                tmp = g_strdup_printf("%s MetaMode %d of Screen %d is the "
                                      "same as MetaMode %d.  All MetaModes "
                                      "must be unique.\n\n",
                                      bullet, i+1, screen->scrnum, j+1);
                tmp2 = g_strconcat((err_str ? err_str : ""), tmp, NULL);
                g_free(err_str);
                err_str = tmp2;
                g_free(tmp);
                break;
            }
            g_free(tmp);
        }
        g_free(metamode_str);
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
    nvGpuPtr gpu;
    nvScreenPtr screen;
    gchar *err_strs = NULL;
    gchar *err_str;
    gchar *tmp;
    gint result;
    int num_absolute = 0;


    /* Validate each screen and count the number of screens using abs. pos. */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            err_str = validate_screen(screen);
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
                     "overlapping and/or deadspace.  It is recommended to "
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
        setup_display_page(ctk_object);
        setup_screen_page(ctk_object);
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
 * Called every time the user selects a new display from the layout
 * image.
 *
 **/

void layout_selected_callback(nvLayoutPtr layout, void *data)
{
    CtkDisplayConfig *ctk_object = (CtkDisplayConfig *)data;


    /* Reconfigure GUI to display information about the selected screen. */
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

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

    /* Setup position */
    setup_display_position(ctk_object);


    /* Setup panning */
    setup_display_panning(ctk_object);

    setup_screen_position(ctk_object);

    /* Setup screen virtual size */
    setup_screen_virtual_size(ctk_object);

    /* If the positioning of the X screen changes, we cannot apply */
    check_screen_pos_changed(ctk_object);

    user_changed_attributes(ctk_object);

} /* layout_modified_callback()  */



/* Widget signal handlers ********************************************/


/** do_enable_display_for_xscreen() **********************************
 *
 * Adds the display device to a separate X screen in the layout.
 *
 **/

void do_enable_display_for_xscreen(CtkDisplayConfig *ctk_object,
                                   nvDisplayPtr display)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvScreenPtr rightmost = NULL;
    nvScreenPtr other;
    nvMetaModePtr metamode;
    nvModePtr mode;
    int scrnum = 0;


    /* Get the next available screen number and the right-most screen */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            scrnum++;

            /* Compute the right-most screen */
            if (!rightmost ||
                ((screen->dim[X] + screen->dim[W]) >
                 (rightmost->dim[X] + rightmost->dim[W]))) {
                rightmost = screen;
            }
        }
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
    display->screen = screen;


    /* Setup the mode */
    mode = display->modes;

    mode->modeline = display->modelines;
    mode->metamode = metamode;

    /* XXX Hopefully display->modelines is 'nvidia-auto-select' */
    mode->dim[W] = display->modelines->data.hdisplay;
    mode->dim[H] = display->modelines->data.vdisplay;
    mode->pan[W] = mode->dim[W];
    mode->pan[H] = mode->dim[H];
    mode->position_type = CONF_ADJ_ABSOLUTE;


    /* Setup the initial metamode */
    metamode->id = -1;
    metamode->source = METAMODE_SOURCE_NVCONTROL;
    metamode->switchable = True;


    /* Setup the screen */
    screen->scrnum = scrnum;
    screen->gpu = display->gpu;
    
    other = layout_get_a_screen(layout, display->gpu);
    screen->depth = other ? other->depth : 24;

    screen->displays_mask = display->device_mask;
    screen->num_displays = 1;
    screen->metamodes = metamode;
    screen->num_metamodes = 1;
    screen->cur_metamode = metamode;
    screen->cur_metamode_idx = 0;


    /* Make the screen right-of the right-most screen */
    if (rightmost) {
        screen->position_type = CONF_ADJ_RIGHTOF;
        screen->relative_to = rightmost;
        screen->dim[X] = mode->dim[X] = rightmost->dim[X];
        screen->dim[Y] = mode->dim[Y] = rightmost->dim[Y];

    } else {
        screen->position_type = CONF_ADJ_ABSOLUTE;
        screen->relative_to = NULL;
        screen->dim[X] = mode->dim[X];
        screen->dim[Y] = mode->dim[Y];
    }        


    /* Add the screen at the end of the gpu's screen list */
    gpu = display->gpu;
    xconfigAddListItem((GenericListPtr *)(&gpu->screens),
                       (GenericListPtr)screen);
    gpu->num_screens++;


    /* We can't dynamically add new X screens */
    ctk_object->apply_possible = FALSE;

} /* do_enable_display_for_xscreen() */



/** prepare_gpu_for_twinview() ***************************************
 *
 * Prepares a GPU for having TwinView enabled.
 *
 * Currently, this means:
 *
 * - Deleting all the implicit metamodes from the X screens that the
 *   GPU is driving.
 *
 * - Making all X screens that are relative to displays on this GPU
 *   relative to the first X screen instead. XXX This assumes we are
 *   using the first X screen as the X screen to use for TwinView.
 *
 **/

static void prepare_gpu_for_twinview(CtkDisplayConfig *ctk_object,
                                     nvGpuPtr gpu)
{
    nvMetaModePtr metamode;
    nvScreenPtr screen;
    nvGpuPtr other;
    int m;

    if (!gpu) return;

    /* Delete implicit metamodes from all screens involved */
    for (screen = gpu->screens; screen; screen = screen->next) {
        nvMetaModePtr next;

        m = 0;
        metamode = screen->metamodes;
        while (metamode) {
            next = metamode->next;

            if ((metamode->source == METAMODE_SOURCE_IMPLICIT) &&
                (metamode != screen->cur_metamode)) {

                ctk_display_layout_delete_screen_metamode
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, m);
            } else {
                m++;
            }
            metamode = next;
        }
    }

    /* Make all other X screens in the layout relative to the GPU's
     * first X screen if they are relative to any display driven
     * by the GPU.
     */
    for (other = ctk_object->layout->gpus; other; other = other->next) {

        if (other == gpu) continue;

        for (screen = other->screens; screen; screen = screen->next) {
            if (screen->relative_to &&
                screen->relative_to->gpu == gpu) {
                screen->relative_to = gpu->screens;
            }
        }
    }

} /* prepare_gpu_for_twinview() */



/** do_enable_display_for_twinview() *********************************
 *
 * Adds the display device to the TwinView setup that already exists
 * on the GPU.
 *
 **/

static void do_enable_display_for_twinview(CtkDisplayConfig *ctk_object,
                                           nvDisplayPtr display)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvMetaModePtr metamode;
    nvModePtr mode;
    guint new_mask;
    char *msg;
    GtkWidget *dlg, *parent;


    /* Make sure a screen exists */
    gpu = display->gpu;
    screen = gpu->screens;

    if (!screen) return;

    /* attempt to associate the display device with the screen */
    new_mask = screen->displays_mask | display->device_mask;
    if (screen->handle) {
        ReturnStatus ret;
        
        ret = NvCtrlSetDisplayAttributeWithReply
            (screen->handle, 0,
             NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
             new_mask);
        
        if (ret != NvCtrlSuccess) {
            
            msg = g_strdup_printf("Failed to associate display device "
                                  "'%s' with X screen %d.  TwinView cannot "
                                  "be enabled with this combination of "
                                  "display devices.",
                                  display->name, screen->scrnum);
            
            parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));
            
            if (parent) {
                dlg = gtk_message_dialog_new
                    (GTK_WINDOW(parent),
                     GTK_DIALOG_DESTROY_WITH_PARENT,
                     GTK_MESSAGE_WARNING,
                     GTK_BUTTONS_OK,
                     msg);
                
                gtk_dialog_run(GTK_DIALOG(dlg));
                gtk_widget_destroy(dlg);
                
            } else {
                nv_error_msg(msg);
            }
            
            g_free(msg);
            
            return;
            
        } else {
            g_signal_handlers_block_by_func
                (G_OBJECT(screen->ctk_event),
                 G_CALLBACK(display_config_attribute_changed),
                 (gpointer) ctk_object);
            
            /* Make sure other parts of nvidia-settings get updated */
            ctk_event_emit(screen->ctk_event, 0,
                           NV_CTRL_ASSOCIATED_DISPLAY_DEVICES, new_mask);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(screen->ctk_event),
                 G_CALLBACK(display_config_attribute_changed),
                         (gpointer) ctk_object);
        }
    }
    
    /* Delete implicit metamodes on all X Screens driven by the GPU */
    prepare_gpu_for_twinview(ctk_object, gpu);

    /* Fix up the display's metamode list */
    display_remove_modes(display);
    
    for (metamode = screen->metamodes; metamode; metamode = metamode->next) {

        nvDisplayPtr other;
        nvModePtr rightmost = NULL;

        
        /* Get the right-most mode of the metamode */
        for (other = screen->gpu->displays; other; other = other->next) {
            if (other->screen != screen) continue;
            for (mode = other->modes; mode; mode = mode->next) {
                if (!rightmost ||
                    ((mode->dim[X] + mode->dim[W]) >
                     (rightmost->dim[X] + rightmost->dim[W]))) {
                    rightmost = mode;
                }
            }
        }


        /* Create the nvidia-auto-select mode fo the display */
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
            mode->dim[X] = rightmost->display->cur_mode->dim[X];
            mode->dim[Y] = rightmost->display->cur_mode->dim[Y];
            mode->pan[X] = mode->dim[X];
            mode->pan[Y] = mode->dim[Y];
        } else {
            mode->position_type = CONF_ADJ_ABSOLUTE;
            mode->relative_to = NULL;
            mode->dim[X] = metamode->dim[X] + metamode->dim[W];
            mode->dim[Y] = metamode->dim[Y];
            mode->pan[X] = mode->dim[X];
            mode->pan[Y] = mode->dim[Y];
        }


        /* Add the mode at the end of the display's mode list */
        xconfigAddListItem((GenericListPtr *)(&display->modes),
                           (GenericListPtr)mode);
        display->num_modes++;
    }


    /* Link the screen and display together */
    screen->displays_mask = new_mask;
    screen->num_displays++;
    display->screen = screen;

} /* do_enable_display_for_twinview() */



/** do_configure_display_for_xscreen() *******************************
 *
 * Configures the display's GPU for Multiple X screens
 *
 **/

static void do_configure_display_for_xscreen(CtkDisplayConfig *ctk_object,
                                             nvDisplayPtr display)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvMetaModePtr metamode;
    nvModePtr mode;
    int scrnum = 0;

    
    if (!display || !display->gpu) return;


    /* Get the next available screen number */
    scrnum = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            scrnum++;
        }
    }

    gpu = display->gpu;


    /* Make sure there is just one display device per X screen */
    for (display = gpu->displays; display; display = display->next) {

        nvScreenPtr new_screen;

        screen = display->screen;
        if (!screen || screen->num_displays == 1) continue;
        
        /* Create a new X screen for this display */
        new_screen = (nvScreenPtr)calloc(1, sizeof(nvScreen));
        if (!new_screen) continue; /* XXX Fail */

        new_screen->gpu = gpu;
        new_screen->scrnum = scrnum++;

        new_screen->depth = screen->depth;

        new_screen->displays_mask = display->device_mask;
        new_screen->num_displays = 1;

        /* Create a metamode for each mode on the display */
        new_screen->num_metamodes = 0;
        for (mode = display->modes; mode; mode = mode->next) {

            /* Create the metamode */
            metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
            if (!metamode) continue; /* XXX Fail ! */

            metamode->source = METAMODE_SOURCE_NVCONTROL;

            /* Make the mode point to the new metamode */
            mode->metamode = metamode;

            /* Set the current metamode if this is the current mode */
            if (display->cur_mode == mode) {
                new_screen->cur_metamode = metamode;
                new_screen->cur_metamode_idx = new_screen->num_metamodes;
            }

            /* Append the metamode */
            xconfigAddListItem((GenericListPtr *)(&new_screen->metamodes),
                               (GenericListPtr)metamode);
            new_screen->num_metamodes++;
        }

        /* Move the display to the new screen */
        screen->num_displays--;
        screen->displays_mask &= ~(display->device_mask);

        display->screen = new_screen;

        if (display == screen->primaryDisplay) {
            new_screen->primaryDisplay = display;
            screen->primaryDisplay = NULL;
        }
        
        /* Append the screen to the gpu */
        xconfigAddListItem((GenericListPtr *)(&gpu->screens),
                           (GenericListPtr)new_screen);
        gpu->num_screens++;

        /* Earlier display devices get first dibs on low screen numbers */
        new_screen->handle = screen->handle;
        new_screen->ctk_event = screen->ctk_event;
        new_screen->scrnum = screen->scrnum;
        screen->handle = NULL;
        screen->ctk_event = NULL;
        screen->scrnum = scrnum -1;

        /* Can't apply creation of new screens */
        ctk_object->apply_possible = FALSE;
    }


    /* Translate mode positional relationships to screen relationships */
    for (display = gpu->displays; display; display = display->next) {
        
        if (!display->screen) continue;

        for (mode = display->modes; mode; mode = mode->next) {
            if (mode->relative_to &&
                (mode->relative_to->gpu == mode->display->gpu)) {
                display->screen->position_type = mode->position_type;
                display->screen->relative_to = mode->relative_to->screen;
            }
            mode->position_type = CONF_ADJ_ABSOLUTE;
            mode->relative_to = NULL;
        }
    }

} /* do_configure_display_for_xscreen() */



/** do_configure_display_for_twinview() ******************************
 *
 * Configures the display's GPU for TwinView
 *
 **/

static void do_configure_display_for_twinview(CtkDisplayConfig *ctk_object,
                                              nvDisplayPtr display)
{
    nvScreenPtr screen;
    nvMetaModePtr metamode;
    nvModePtr mode;
    nvModePtr last_mode;
    nvGpuPtr gpu = display->gpu;
    int m;


    /* We need at least one screen to activate TwinView */
    if (!gpu || !gpu->screens) return;


    /* Delete implicit metamodes on all X Screens driven by the GPU */
    prepare_gpu_for_twinview(ctk_object, gpu);


    /* Make sure the screen has enough metamodes */
    screen = gpu->screens;
    for (display = gpu->displays; display; display = display->next) {

        /* Only add enabled displays to TwinView setup */
        if (!display->screen) continue;

        /* Make sure the screen has enough metamodes */
        for (m = display->num_modes - screen->num_metamodes; m > 0; m--) {

            metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
            if (!metamode) break; // XXX Sigh.
            
            metamode->source = METAMODE_SOURCE_NVCONTROL;
            
            /* Add the metamode at the end of the screen's metamode list */
            xconfigAddListItem((GenericListPtr *)(&screen->metamodes),
                               (GenericListPtr)metamode);
            screen->num_metamodes++;
        }
    }


    /* Make sure each display has the right number of modes and that
     * the modes point to the right metamode in the screen.
     */
    for (display = gpu->displays; display; display = display->next) {

        /* Only add enabled displays to TwinView setup */
        if (!display->screen) continue;

        /* Make the display mode point to the right metamode */
        metamode = screen->metamodes;
        mode = display->modes;
        last_mode = NULL;
        while (metamode && mode) {

            if (metamode == screen->cur_metamode) {
                display->cur_mode = mode;
            }

            /* Keep the relationship between displays if possible. If
             * this display's screen is relative to another screen
             * on the same gpu, translate the relationship to the
             * display's mode.
             */
            if (display->screen->relative_to &&
                (display->screen->relative_to->gpu == gpu)) {

                nvDisplayPtr other;

                /* Make the display relative to the other display */
                for (other = gpu->displays; other; other = other->next) {
                    if (other->screen == display->screen->relative_to) {
                        mode->position_type = display->screen->position_type;
                        mode->relative_to = other;
                        break;
                    }
                }
            }
            mode->metamode = metamode;

            if (mode && !mode->next) {
                last_mode = mode;
            }
            metamode = metamode->next;
            mode = mode->next;
        }

        /* Add dummy modes */
        while (metamode) {

            mode = mode_parse(display, "NULL");
            mode->dummy = 1;
            mode->metamode = metamode;

            if (metamode == screen->cur_metamode) {
                display->cur_mode = mode;
            }
            
            /* Duplicate position information of the last mode */
            if (last_mode) {
                mode->dim[X] = last_mode->dim[X];
                mode->dim[Y] = last_mode->dim[Y];
                mode->pan[X] = last_mode->pan[X];
                mode->pan[Y] = last_mode->pan[Y];
                mode->position_type = last_mode->position_type;
                mode->relative_to = last_mode->relative_to;
            }
            
            /* Add the mode at the end of display's mode list */
            xconfigAddListItem((GenericListPtr *)(&display->modes),
                               (GenericListPtr)mode);
            display->num_modes++;
            metamode = metamode->next;
        }
    }


    /* Make the displays part of the screen */
    for (display = gpu->displays; display; display = display->next) {

        /* Only add enabled displays to TwinView setup */
        if (!display->screen) continue;

        if (display->screen != screen) {
            display->screen = screen;
            screen->displays_mask |= display->device_mask;
            screen->num_displays++;
        }
    }


    /* Delete extra screens on the GPU */
    while (gpu->screens->next) {
        nvScreenPtr other;

        /* Delete screens that come before 'screen' */
        if (gpu->screens != screen) {
            other = gpu->screens;
            gpu->screens = other->next;

        /* Delete screens that comes after 'screen' */
        } else {
            other = gpu->screens->next;
            gpu->screens->next = other->next;
        }

        /* Handle positional relationships going away */
        if (screen->relative_to == other) {
            screen->position_type = CONF_ADJ_ABSOLUTE;
            screen->relative_to = NULL;
        }

        /* Clean up memory used by the screen */
        while (other->metamodes) {
            nvMetaModePtr metamode = other->metamodes;
            other->metamodes = metamode->next;
            free(metamode);
        }

        /* Keep the lowest screen number */
        if (other->scrnum < screen->scrnum) {
            if (screen->handle) {
                NvCtrlAttributeClose(screen->handle);
            }
            screen->scrnum = other->scrnum;
            screen->handle = other->handle;
            screen->ctk_event = other->ctk_event;
        } else {
            if (other->handle) {
                NvCtrlAttributeClose(other->handle);
            }
        }

        free(other);
        gpu->num_screens--;
    }

    /* Make sure screen numbering is consistent */
    renumber_xscreens(ctk_object->layout);

} /* do_configure_display_for_twinview() */



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

void do_disable_display(CtkDisplayConfig *ctk_object, nvDisplayPtr display)
{
    nvGpuPtr gpu = display->gpu;
    gchar *str;
    gchar *type = display_get_type_str(display->device_mask, 0);


    /* Setup the remove display dialog */
    if (ctk_object->advanced_mode) {
        str = g_strdup_printf("Disable the display device %s (%s) "
                              "on GPU-%d (%s)?",
                              display->name, type,
                              NvCtrlGetTargetId(gpu->handle), gpu->name);
    } else {
        str = g_strdup_printf("Disable the display device %s (%s)?",
                              display->name, type);
    }
    g_free(type);

    gtk_label_set_text
        (GTK_LABEL(ctk_object->txt_display_disable), str);
    g_free(str);
        
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_display_disable_off),
                         "Disable");
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_display_disable_cancel),
                         "Cancel");

     
    /* Confirm with user before disabling */
    if (do_query_remove_display(ctk_object, display)) {
        ctk_display_layout_disable_display(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                           display);
    }

} /* do_disable_display() */



/** display_config_clicked() *****************************************
 *
 * Called when user clicks on the display configuration button.
 *
 **/

static void display_config_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gint result;
    gboolean update = FALSE;
    nvGpuPtr gpu;
    int num_screens;


    if (!display) return;


    /* Don't allow disabling the last display device */
    num_screens = 0;
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        num_screens += gpu->num_screens;
    }
    if (num_screens == 1 && display->screen &&
        display->screen->num_displays == 1) {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_disabled,
                                 FALSE);
    } else {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_disabled,
                                 TRUE);
    }


    /* We can only enable as many X screens as the GPU supports */
    if (!display->screen &&
        (display->gpu->num_screens >= display->gpu->max_displays)) {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_xscreen,
                                 FALSE);
    } else {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_xscreen,
                                 TRUE);
    }


    /* We can't setup TwinView if there is only one display connected,
     * there are no existing X screens on the GPU, or this display is
     * the only enabled device on the GPU, or when SLI is enabled.
     */
    if (display->gpu->num_displays == 1 || !display->gpu->num_screens ||
        display->gpu->screens->sli ||
        (display->gpu->num_screens == 1 &&
         display->gpu->screens->num_displays == 1 &&
         display->screen == display->gpu->screens)) {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_twinview,
                                 FALSE);
    } else {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_twinview,
                                 TRUE);
    }


    /* Setup the button state */
    if (!display->screen) {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_disabled),
             TRUE);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_disabled),
             "Disabled");
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_xscreen),
             "Separate X screen (requires X restart)");
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_twinview),
             "TwinView");

    } else if (display->screen->num_displays > 1) {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_twinview),
             TRUE);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_disabled),
             "Disabled");
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_xscreen),
             "Separate X screen (requires X restart)");  
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_twinview),
             "TwinView");

    } else {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_xscreen),
             TRUE);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_disabled),
             "Disabled (requires X restart)");
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_xscreen),
             "Separate X screen");  
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_twinview),
             "TwinView (requires X restart)");
    }


    /* Show the display config dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_display_config),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_show_all(ctk_object->dlg_display_config);
    gtk_widget_grab_focus(ctk_object->btn_display_config_cancel);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_display_config));
    gtk_widget_hide(ctk_object->dlg_display_config);
    
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        /* OK */
        break;
        
    case GTK_RESPONSE_CANCEL:
    default:
        /* Cancel */
        return;
    }



    if (gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_disabled))) {
        if (display->screen) {
            do_disable_display(ctk_object, display);
            update = TRUE;
        }
    } else {

        /* Make sure the display has modelines */
        if (!display->modelines) {
            char *tokens;
            gchar *err_str = NULL;
            NvCtrlStringOperation(display->gpu->handle, display->device_mask,
                                  NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,
                                  "", &tokens);
            update = TRUE;
            if (!display_add_modelines_from_server(display, &err_str)) {
                nv_warning_msg(err_str);
                g_free(err_str);
                return;
            }
        }
        if (!display->modelines) return;

        if (!display->screen) {

            /* Enable display as a separate X screen */
            if (gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_xscreen))) {
                do_enable_display_for_xscreen(ctk_object, display);
                update = TRUE;
            }
            
            /* Enable display in TwinView with an existing screen */
            if (gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_twinview))) {
                do_enable_display_for_twinview(ctk_object, display);
                update = TRUE;
            }

        } else {

            /* Move display to a new X screen */
            if (display->screen->num_displays > 1 &&
                gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_xscreen))) {
                do_configure_display_for_xscreen(ctk_object, display);
                update = TRUE;
            }
            
            /* Setup TwinView on the first X screen */
            if (display->screen->num_displays == 1 &&
                gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_twinview))) {
                do_configure_display_for_twinview(ctk_object, display);
                update = TRUE;
            }
        }
    }


    /* Sync the GUI */
    if (update) {

        /* Update the z-order */
        ctk_display_layout_update_zorder(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

        /* Make sure the display is still selected */
        ctk_display_layout_select_display(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                          display);

        /* Recalculate */
        ctk_display_layout_update(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

        /* Auto fix all screens on the gpu */
        {
            nvGpuPtr gpu = display->gpu;
            nvScreenPtr screen;

            for (screen = gpu->screens; screen; screen = screen->next) {
                validation_auto_fix_screen(ctk_object, screen);
            }
        }

        /* Redraw */
        ctk_display_layout_redraw(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
        
        /* Update GUI */
        setup_layout_frame(ctk_object);
        setup_display_page(ctk_object);
        setup_screen_page(ctk_object);

        user_changed_attributes(ctk_object);
    }

} /* display_config_clicked() */



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


    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    modeline = ctk_object->refresh_table[idx];
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


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
         display->cur_mode, modeline);


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
    nvModeLinePtr modeline;
    nvDisplayPtr display;


    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    modeline = ctk_object->resolution_table[idx];
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* cache the selected index */
    last_idx = ctk_object->last_resolution_idx;
    ctk_object->last_resolution_idx = idx;
    
    /* Ignore selecting same resolution */
    if (idx == last_idx) {
        return;
    }


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
    

    /* Select the new modeline for its resolution */
    ctk_display_layout_set_mode_modeline
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display->cur_mode, modeline);


    /* Regenerate the refresh menu */
    setup_display_refresh_dropdown(ctk_object);


    /* Sync the display position */
    setup_display_position(ctk_object);


    /* Sync the panning domain */
    setup_display_panning(ctk_object);


    user_changed_attributes(ctk_object);

} /* display_resolution_changed() */



/** display_modelname_changed() *****************************
 *
 * Called when user selectes a new display from display modelname dropdown.
 *
 **/
static void display_modelname_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvDisplayPtr display;
    gint idx;

    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    display = ctk_object->display_model_table[idx];
    ctk_display_layout_select_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), display);
    ctk_display_layout_redraw(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    /* Reconfigure GUI to display information about the selected display. */
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

} /* display_modelname_changed() */



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
        gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx =
        gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->display_position_table_len) {

        relative_to =
            ctk_object->display_position_table[relative_to_idx];
        
        /* Update the layout */
        ctk_display_layout_set_display_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             display, position_type, relative_to,
             display->cur_mode->dim[X],
             display->cur_mode->dim[Y]);
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
    position_idx = gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx = gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_position_relative));

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
    x += display->cur_mode->metamode->edim[X];
    y += display->cur_mode->metamode->edim[Y];


    /* Update the absolute position */
    ctk_display_layout_set_display_position
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
         display, CONF_ADJ_ABSOLUTE, NULL, x, y);

    user_changed_attributes(ctk_object);

} /* display_position_offset_activate() */



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



/** display_panning_focus_out() **************************************
 *
 * Called when user leaves the panning entry
 *
 **/

static gboolean display_panning_focus_out(GtkWidget *widget, GdkEvent *event,
                                          gpointer user_data)
{
    display_panning_activate(widget, user_data);

    return FALSE;

} /* display_panning_focus_out() */



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



/** screen_virtual_size_focus_out() **********************************
 *
 * Called when user leaves the screen virtual size entry
 *
 **/

static gboolean screen_virtual_size_focus_out(GtkWidget *widget,
                                              GdkEvent *event,
                                              gpointer user_data)
{
    screen_virtual_size_activate(widget, user_data);

    return FALSE;

} /* screen_virtual_size_focus_out() */



/** screen_depth_changed() *******************************************
 *
 * Called when user selects a new color depth for a screen.
 *
 **/

static void screen_depth_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    int depth;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!screen) return;

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
        gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx =
        gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_relative));

    if (relative_to_idx >= 0 &&
        relative_to_idx < ctk_object->screen_position_table_len) {
        
        relative_to =
            ctk_object->screen_position_table[relative_to_idx];

        /* Update the layout */
        ctk_display_layout_set_screen_position
            (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
             screen, position_type, relative_to,
             screen->dim[X],
             screen->dim[Y]);
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
    position_idx = gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_type));

    position_type = __position_table[position_idx];

    relative_to_idx = gtk_option_menu_get_history
        (GTK_OPTION_MENU(ctk_object->mnu_screen_position_relative));

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
        tmp = screen_get_metamode_str(screen, i, 1);
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
         screen, screen->cur_metamode_idx);

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
 * Switches to the current screen metamode using XRandR
 *
 **/

static Bool switch_to_current_metamode(CtkDisplayConfig *ctk_object,
                                       nvScreenPtr screen)
{
    ReturnStatus ret;
    gint result;
    nvMetaModePtr metamode;
    int new_width;
    int new_height;
    int new_rate;
    int old_width;
    int old_height;
    int old_rate;
    static SwitchModeCallbackInfo info;
    GtkWidget *dlg;
    GtkWidget *parent;
    gchar *msg;


    if (!screen->handle || !screen->cur_metamode) goto fail;

    metamode = screen->cur_metamode;

    new_width = metamode->edim[W];
    new_height = metamode->edim[H];
    new_rate = metamode->id;
    
    
    /* Find the parent window for displaying dialogs */

    parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));
    if (!parent) goto fail;


    /* XRandR must be available to do mode switching */

    if (!NvCtrlGetXrandrEventBase(screen->handle)) {
        dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_INFO,
             GTK_BUTTONS_CLOSE,
             "The XRandR X extension was not found.  This extension must "
             "be supported by the X server and enabled for display "
             "configuration settings to be dynamically applicable.");

        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);

        nv_warning_msg("XRandR X extension not enabled, "
                       "cannot apply settings!");
        goto fail;
    }


    /* Get the current mode so we can fall back on that if the
     * mode switch fails, or the user does not confirm.
     */

    ret = NvCtrlXrandrGetScreenMode(screen->handle, &old_width, &old_height,
                                    &old_rate);
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to get current (fallback) mode for "
                       "display device!");
        goto fail;
    }

    nv_info_msg(TAB, "Current mode: %dx%d (id: %d)",
                old_width, old_height, old_rate);
    

    /* Switch to the new mode */

    nv_info_msg(TAB, "Switching to mode: %dx%d (id: %d)...",
                new_width, new_height, new_rate);

    ret = NvCtrlXrandrSetScreenMode(screen->handle, new_width, new_height,
                                    new_rate);
    if (ret != NvCtrlSuccess) {

        nv_warning_msg("Failed to set MetaMode (%d) '%s' "
                       "(Mode: %dx%d, id: %d) on X screen %d!",
                       screen->cur_metamode_idx+1, metamode->string, new_width,
                       new_height, new_rate,
                       NvCtrlGetTargetId(screen->handle));

        if (screen->num_metamodes > 1) {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X screen %d\n\n"
                                  "Would you like to remove this MetaMode?",
                                  screen->cur_metamode_idx+1, metamode->string,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->handle));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_YES_NO,
                 msg);
        } else {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X screen %d.",
                                  screen->cur_metamode_idx+1, metamode->string,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->handle));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_WARNING,
                 GTK_BUTTONS_OK,
                 msg);
        }

        result = gtk_dialog_run(GTK_DIALOG(dlg));

        switch (result) {
        case GTK_RESPONSE_YES:
            ctk_display_layout_delete_screen_metamode
                (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                 screen, screen->cur_metamode_idx);

            nv_info_msg(TAB, "Removed MetaMode %d on Screen %d.\n",
                        screen->cur_metamode_idx+1,
                        NvCtrlGetTargetId(screen));

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
    info.screen = NvCtrlGetTargetId(screen->handle);

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
        nv_info_msg(TAB, "Switching back to mode: %dx%d (id: %d)...",
                    old_width, old_height, old_rate);

        NvCtrlXrandrSetScreenMode(screen->handle, old_width, old_height,
                                  old_rate);
        /* Good luck! */
        goto fail;
    }

    return TRUE;

 fail:
    return FALSE;

} /* switch_to_current_metamode() */



/** find_metamode_string() *******************************************
 *
 * Finds "metamode_str" in the list of strings in metamode_strs.
 * If metamode_str is found, a pointer to the full metamode string
 * is returned (including tokens, if any.)
 *
 * if "metamode_str" is not found in "metamode_strs", NULL is
 * returned.
 *
 **/

static char *find_metamode_string(char *metamode_str, char *metamode_strs)
{
    char *m;
    char *str;

    for (m = metamode_strs; m && strlen(m); m += strlen(m) +1) {

        /* Skip tokens if any */
        str = strstr(m, "::");
        if (str) {
            str = (char *)parse_skip_whitespace(str +2);
        } else {
            str = m;
        }

        /* See if metamode strings match */
        if (!strcmp(str, metamode_str)) return m;
    }

    return NULL;

} /* find_metamode_string() */



/** preprocess_metamodes() *******************************************
 *
 * Does preprocess work to the metamode strings:
 *
 * - Generates the metamode strings for the screen's metamodes
 *   that will be used for creating the metamode list on the X
 *   Server.
 *
 * - Whites out each string in the metamode_strs list that should
 *   not be deleted (it has a matching metamode in "screen".)
 *
 * - Adds new metamodes to the X server screen that are specified
 *   in "screen".
 *
 **/

static void preprocess_metamodes(nvScreenPtr screen, char *metamode_strs)
{
    nvMetaModePtr metamode;
    ReturnStatus ret;
    char *str = NULL;
    char *tokens;
    int metamode_idx;


    for (metamode = screen->metamodes, metamode_idx = 0;
         metamode;
         metamode = metamode->next, metamode_idx++) {

        /* Generate the metamode's string */
        free(metamode->string);
        metamode->string = screen_get_metamode_str(screen, metamode_idx, 0);
        if (!metamode->string) continue;
        
        /* Look for the metamode string in metamode_strs */
        str = find_metamode_string(metamode->string, metamode_strs);
        if (str) {

            /* Grab the metamode id from the tokens */
            tokens = strdup(str);
            if (tokens) {
                char *tmp = strstr(tokens, "::");
                if (tmp) {
                    *tmp = '\0';
                    parse_token_value_pairs(tokens, apply_metamode_token,
                                            metamode);
                }
                free(tokens);
            }

            /* The metamode was found, white out the metamode string
             * so it does not get deleted and continue.
             */
            while (*str) {
                *str = ' ';
                str++;
            }
            continue;
        }
        
        /* The metamode was not found, so add it to the X screen's list */
        tokens = NULL;
        ret = NvCtrlStringOperation(screen->handle, 0,
                                    NV_CTRL_STRING_OPERATION_ADD_METAMODE,
                                    metamode->string, &tokens);

        /* Grab the metamode ID from the returned tokens */
        if (ret == NvCtrlSuccess) {
            if (tokens) {
                parse_token_value_pairs(tokens, apply_metamode_token,
                                        metamode);
                free(tokens);
            }
            nv_info_msg(TAB, "Added   > %s", metamode->string);
        }
    }

} /* preprocess_metamodes() */



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
    char *metamode_str;
    char *update_str;
    int len;
    ReturnStatus ret;


    for (metamode = screen->metamodes, metamode_idx = 0;
         metamode;
         metamode = metamode->next, metamode_idx++) {

        metamode_str = screen_get_metamode_str(screen, metamode_idx,
                                               0);
        if (!metamode_str) continue;
        
        /* Append the index we want */
        len = 24 + strlen(metamode_str);
        update_str = malloc(len);
        snprintf(update_str, len, "index=%d :: %s", metamode_idx,
                 metamode_str);
        
        ret = NvCtrlSetStringAttribute(screen->handle,
                                       NV_CTRL_STRING_MOVE_METAMODE,
                                       update_str, NULL);
        if (ret == NvCtrlSuccess) {
            nv_info_msg(TAB, "Moved   > %s", metamode_str);
        }
        free(metamode_str);
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
    char *metamode_str;
    char *str;
    ReturnStatus ret;


    /* Delete metamodes that were not cleared out from the metamode_strs */
    for (metamode_str = metamode_strs;
         metamode_str && strlen(metamode_str);
         metamode_str += strlen(metamode_str) +1) {

        /* Skip tokens */
        str = strstr(metamode_str, "::");
        if (!str) continue;

        str = (char *)parse_skip_whitespace(str +2);


        /* Delete the metamode */
        ret = NvCtrlSetStringAttribute(screen->handle,
                                       NV_CTRL_STRING_DELETE_METAMODE,
                                       str, NULL);
        if (ret == NvCtrlSuccess) {
            nv_info_msg(TAB, "Removed > %s", str);
        }
    }

    /* Reorder the list of metamodes */
    order_metamodes(screen);

} /* postprocess_metamodes() */



/** update_screen_metamodes() ****************************************
 *
 * Updates the screen's metamode list.
 *
 **/

static int update_screen_metamodes(CtkDisplayConfig *ctk_object,
                                   nvScreenPtr screen)
{
    char *metamode_strs = NULL;
    char *cur_metamode_str = NULL;
    char *metamode_str;
    int len;

    int clear_apply = 0; /* Set if we should clear the apply button */
    ReturnStatus ret;


    /* Make sure the screen has a valid handle to make the updates */
    if (!screen->handle) {
        return 1;
    }

    nv_info_msg("", "Updating Screen %d's MetaModes:",
                NvCtrlGetTargetId(screen->handle));

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
     **/
  
    /* Get the list of the current metamodes */

    ret = NvCtrlGetBinaryAttribute(screen->handle,
                                   0,
                                   NV_CTRL_BINARY_DATA_METAMODES,
                                   (unsigned char **)&metamode_strs,
                                   &len);
    if (ret != NvCtrlSuccess) goto done;

    /* Get the current metamode for the screen */

    ret = NvCtrlGetStringAttribute(screen->handle,
                                   NV_CTRL_STRING_CURRENT_METAMODE,
                                   &cur_metamode_str);
    if (ret != NvCtrlSuccess) goto done;

    /* Skip tokens */
    metamode_str = strstr(cur_metamode_str, "::");
    if (metamode_str) {
        metamode_str = (char *)parse_skip_whitespace(metamode_str +2);
    } else {
        metamode_str = cur_metamode_str;
    }

    /* Preprocess the new metamodes list */

    preprocess_metamodes(screen, metamode_strs);
    
    /* If we need to switch metamodes, do so now */

    if (strcmp(screen->cur_metamode->string, metamode_str)) {

        if (switch_to_current_metamode(ctk_object, screen)) {

            ctk_config_statusbar_message(ctk_object->ctk_config,
                                         "Switched to MetaMode %dx%d.",
                                         screen->cur_metamode->edim[W],
                                         screen->cur_metamode->edim[H]);
            
            nv_info_msg(TAB, "Using   > %s", screen->cur_metamode->string);

            clear_apply = 1;
        }
    }

    /* Post process the metamodes list */

    postprocess_metamodes(screen, metamode_strs);

 done:

    XFree(metamode_strs);
    XFree(cur_metamode_str);

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
    nvGpuPtr gpu;
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

    /* Update all GPUs */
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        nvScreenPtr screen;

        /* Update all X screens */
        for (screen = gpu->screens; screen; screen = screen->next) {

            if (!screen->handle) continue;
            if (screen->no_scanout) continue;

            if (!update_screen_metamodes(ctk_object, screen)) {
                clear_apply = FALSE;
            } else {
                ReturnStatus ret;

                ret = NvCtrlSetDisplayAttributeWithReply
                    (screen->handle, 0,
                     NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
                     screen->displays_mask);
                
                if (ret != NvCtrlSuccess) {
                    nv_error_msg("Failed to set screen %d's association mask "
                                 "to: 0x%08x",
                                 screen->scrnum, screen->displays_mask);
                } else {
                    /* Make sure other parts of nvidia-settings get updated */
                    ctk_event_emit(screen->ctk_event, 0,
                                   NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
                                   screen->displays_mask);
                }
            }
            
            if (screen->primaryDisplay) {
                gchar *primary_str =
                    display_get_type_str(screen->primaryDisplay->device_mask,
                                         0);

                ret = NvCtrlSetStringAttribute(screen->handle,
                                 NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER,
                                 primary_str, NULL);
                g_free(primary_str);

                if (ret != NvCtrlSuccess) {
                    nv_error_msg("Failed to set primary display"
                                 "for screen %d (GPU:%s)", screen->scrnum,
                                 screen->gpu->name);
                } else {
                    /* Make sure other parts of nvidia-settings get updated */
                    ctk_event_emit_string(screen->ctk_event, 0,
                                   NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER);
                }
            }
        }
    }

    /* Clear the apply button if all went well */
    if (clear_apply) {
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
    
    monitor->identifier = (char *)malloc(32);
    snprintf(monitor->identifier, 32, "Monitor%d", monitor_id);
    monitor->vendor = xconfigStrdup("Unknown");  /* XXX */

    /* Copy the model name string, stripping any '"' characters */

    len = strlen(display->name);
    monitor->modelname = (char *)malloc(len + 1);
    for (i = 0, j = 0; i < len; i++, j++) {
        if (display->name[i] == '\"') {
            if (++i >= len)
                break;
        }
        monitor->modelname[j] = display->name[i];
    }
    monitor->modelname[j] = '\0';

    /* Get the Horizontal Sync ranges from nv-control */

    ret = NvCtrlGetStringDisplayAttribute
        (display->gpu->handle,
         display->device_mask,
         NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES,
         &range_str);
    if (ret != NvCtrlSuccess) {
        nv_error_msg("Unable to determine valid horizontal sync ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->name, display->gpu->name);
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
                     display->name, display->gpu->name);
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

    ret = NvCtrlGetStringDisplayAttribute
        (display->gpu->handle,
         display->device_mask,
         NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES,
         &range_str);
    if (ret != NvCtrlSuccess) {
        nv_error_msg("Unable to determine valid vertical refresh ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->name, display->gpu->name);
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
                     display->name, display->gpu->name);
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
    device->identifier = (char *)malloc(32);
    snprintf(device->identifier, 32, "Device%d", device_id);

    device->driver = xconfigStrdup("nvidia");
    device->vendor = xconfigStrdup("NVIDIA Corporation");
    device->board = xconfigStrdup(gpu->name);

    if (print_bus_id) {
        device->busid = (char *)malloc(32);
        snprintf(device->busid, 32, "PCI:%d:%d:0",
                 gpu->pci_bus, gpu->pci_device);
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
        
        conf_display->virtualX = screen->dim[W];
        conf_display->virtualY = screen->dim[H];
    }

    /* XXX Don't do any further tweaking to the display subsection.
     *     All mode configuration should be done through the 'MetaModes"
     *     X Option.  The modes generated by xconfigAddDisplay() will
     *     be used as a fallack.
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
    conf_screen->identifier = (char *)malloc(32);
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
        for (display = screen->gpu->displays; display; display = display->next) {
            if (display->screen == screen) {
                break;
            }
        }
        if (!display) {
            nv_error_msg("Unable to find a display device for screen %d!",
                         screen->scrnum);
            goto fail;
        }

        
        /* Create the screen's only Monitor section from the first display */
        if (!add_monitor_to_xconfig(display, config, screen->scrnum)) {
            nv_error_msg("Failed to add display device '%s' to screen %d!",
                         display->name, screen->scrnum);
            goto fail;
        }


        /* Tie the screen to the monitor section */
        conf_screen->monitor_name =
            xconfigStrdup(display->conf_monitor->identifier);
        conf_screen->monitor = display->conf_monitor;


        /* Add the modelines of all other connected displays to the monitor */
        for (other = screen->gpu->displays; other; other = other->next) {
            if (other->screen != screen) continue;
            if (other == display) continue;
            
            /* Add modelines used by this display */
            add_modelines_to_monitor(display->conf_monitor, other->modes);
        }

        /* Set the TwinView option */
        xconfigAddNewOption(&conf_screen->options, "TwinView",
                            ((screen->num_displays > 1) ? "1" : "0" ));
        
        /* Set the TwinviewXineramaInfoOrder option */
        
        if (screen->primaryDisplay) {
            gchar *primary_str =
                display_get_type_str(screen->primaryDisplay->device_mask, 0);
            
            xconfigAddNewOption(&conf_screen->options, "TwinViewXineramaInfoOrder",
                                primary_str);
            g_free(primary_str);
        }

        /* Create the "metamode" option string. */
        ret = generate_xconf_metamode_str(ctk_object, screen, &metamode_strs);
        if (ret != XCONFIG_GEN_OK) goto bail;
        
        /* If no user specified metamodes were found, add
         * whatever the currently selected metamode is
         */
        if (!metamode_strs) {
            metamode_strs = screen_get_metamode_str(screen,
                                                    screen->cur_metamode_idx, 1);
        }
        
        if (metamode_strs) {
            xconfigAddNewOption(&conf_screen->options, "metamodes", metamode_strs);
            free(metamode_strs);
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
    nvScreenPtr other;
    int device_screen_id;

    /* Go through the GPU's screens and figure out what the
     * GPU-relative screen number should be for the given
     * screen's device section.
     */

    /* If there is one screen, the device section shouldn't
     * specify a "Screen #"
     */

    if (gpu->num_screens < 2) return -1;


    /* Count the number of screens that have a screen number
     * that is lower than the given screen, and that's the
     * relative position of this screen wrt the GPU.
     */

    device_screen_id = 0;
    for (other = gpu->screens; other; other = other->next) {
        if (other == screen) continue;
        if (screen->scrnum > other->scrnum) {
            device_screen_id++;
        }
    }

    return device_screen_id;

} /* get_device_screen_id() */



/*
 * add_screens_to_xconfig() - Adds all the X screens in the given
 * layout to the X configuration structure.
 */

static int add_screens_to_xconfig(CtkDisplayConfig *ctk_object,
                                  nvLayoutPtr layout, XConfigPtr config)
{
    nvGpuPtr gpu;
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
    if (layout->num_gpus == 1 &&
        layout->gpus->num_screens == 1) {
        print_bus_ids = 0;
    } else {
        print_bus_ids = 1;
    }

    /* Generate the Device sections and Screen sections */

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        
        for (screen = gpu->screens; screen; screen = screen->next) {

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
        adj->x = screen->dim[X];
        adj->y = screen->dim[Y];
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
    nvGpuPtr gpu;
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
        screen = NULL;
        for (gpu = layout->gpus; gpu; gpu = gpu->next) {
            for (screen = gpu->screens; screen; screen = screen->next) {
                if (screen->scrnum == scrnum) break;
            }
            if (screen) {
                if (!add_adjacency_to_xconfig(screen, config)) goto fail;
                break;
            }
        }
        scrnum++;
    } while (screen);


    /* Setup for Xinerama */
    if (!config->flags) {
        config->flags = (XConfigFlagsPtr) calloc(1, sizeof(XConfigFlagsRec));
        if (!config->flags) goto fail;
    }
    xconfigAddNewOption(&config->flags->options, "Xinerama",
                        (layout->xinerama_enabled ? "1" : "0"));

    layout->conf_layout = conf_layout;
    return TRUE;

 fail:
    return FALSE;

} /* add_layout_to_xconfig() */



/*
 * get_default_project_root() - scan some common directories for the X
 * project root
 *
 * Users of this information should be careful to account for the
 * modular layout.
 */

char *get_default_project_root(void)
{
    char *paths[] = { "/usr/X11R6", "/usr/X11", NULL };
    struct stat stat_buf;
    int i;
        
    for (i = 0; paths[i]; i++) {
        
        if (stat(paths[i], &stat_buf) == -1) {
            continue;
        }
    
        if (S_ISDIR(stat_buf.st_mode)) {
            return paths[i];
        }
    }
    
    /* default to "/usr/X11R6", I guess */

    return paths[0];

} /* get_default_project_root() */



/*
 * generateXConfig() - Generates an X config structure based
 * on the layout given.
 */

static int generateXConfig(CtkDisplayConfig *ctk_object, XConfigPtr *pConfig)
{
    nvLayoutPtr layout = ctk_object->layout;
    XConfigPtr config = NULL;
    GenerateOptions go;
    char *server_vendor;
    int ret;


    if (!pConfig) goto fail;


    /* Query server X.Org/XFree86 */
    server_vendor = NvCtrlGetServerVendor(layout->handle);
    if (server_vendor && g_strrstr(server_vendor, "X.Org")) {
        go.xserver = X_IS_XORG;
    } else {
        go.xserver = X_IS_XF86;
    }


    /* XXX Assume we are creating an X config file for the local system */
    go.x_project_root = get_default_project_root();
    go.keyboard = NULL;
    go.mouse = NULL;
    go.keyboard_driver = NULL;


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
    run_save_xconfig_dialog(ctk_object->save_xconfig_dlg);

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
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

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
    unsigned int mask;
    nvGpuPtr gpu;
    nvDisplayPtr display;
    nvDisplayPtr selected_display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    ReturnStatus ret;
    gchar *type;
    gchar *str;

    /* Probe each GPU for display changes */
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {

        if (!gpu->handle) continue;

        g_signal_handlers_block_by_func
            (G_OBJECT(gpu->ctk_event),
             G_CALLBACK(display_config_attribute_changed),
             (gpointer) ctk_object);

        /* Do the probe */
        ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_PROBE_DISPLAYS,
                                 (int *)&probed_displays);
        if (ret != NvCtrlSuccess) {
            nv_error_msg("Failed to probe for display devices on GPU-%d '%s'.",
                         NvCtrlGetTargetId(gpu->handle), gpu->name);

            g_signal_handlers_unblock_by_func
                (G_OBJECT(gpu->ctk_event),
                 G_CALLBACK(display_config_attribute_changed),
                 (gpointer) ctk_object);

            continue;
        }

        /* Make sure other parts of nvidia-settings get updated */
        ctk_event_emit(gpu->ctk_event, 0,
                       NV_CTRL_PROBE_DISPLAYS, probed_displays);
        
        /* Go through the probed displays */
        for (mask = 1; mask; mask <<= 1) {

            /* Ask users about removing old displays */
            if ((gpu->connected_displays & mask) &&
                !(probed_displays & mask)) {

                display = gpu_get_display(gpu, mask);
                if (!display) continue; /* XXX ack. */

                /* The selected display is being removed */
                if (display == selected_display) {
                    selected_display = NULL;
                }
                    
                /* Setup the remove display dialog */
                type = display_get_type_str(display->device_mask, 0);
                str = g_strdup_printf("The display device %s (%s) on GPU-%d "
                                      "(%s) has been\nunplugged.  Would you "
                                      "like to remove this display from the "
                                      "layout?",
                                      display->name, type,
                                      NvCtrlGetTargetId(gpu->handle),
                                      gpu->name);
                g_free(type);
                gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_disable),
                                   str);
                g_free(str);
                
                gtk_button_set_label
                    (GTK_BUTTON(ctk_object->btn_display_disable_off),
                     "Remove");

                gtk_button_set_label
                    (GTK_BUTTON(ctk_object->btn_display_disable_cancel),
                     "Ignore");
                 
                /* Ask the user if they want to remove the display */
                if (do_query_remove_display(ctk_object, display)) {
                    
                    /* Remove display from the GPU */
                    gpu_remove_and_free_display(display);
                    
                    /* Let display layout widget know about change */
                    ctk_display_layout_update_display_count
                        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), NULL);

                    user_changed_attributes(ctk_object);
                }

            /* Add new displays as 'disabled' */
            } else if (!(gpu->connected_displays & mask) &&
                       (probed_displays & mask)) {
                gchar *err_str = NULL;
                display = gpu_add_display_from_server(gpu, mask, &err_str);
                if (err_str) {
                    nv_warning_msg(err_str);
                    g_free(err_str);
                }
                gpu_add_screenless_modes_to_displays(gpu);
                ctk_display_layout_update_display_count
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     selected_display);
            }
        }

        g_signal_handlers_unblock_by_func
            (G_OBJECT(gpu->ctk_event),
             G_CALLBACK(display_config_attribute_changed),
             (gpointer) ctk_object);
    }


    /* Sync the GUI */
    ctk_display_layout_redraw(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    
    setup_display_page(ctk_object);
    
    setup_screen_page(ctk_object);

} /* probe_clicked() */



/** reset_layout() *************************************************
 *
 * Load current X server settings.
 *
 **/

static void reset_layout(CtkDisplayConfig *ctk_object) 
{
    gchar *err_str = NULL;
    nvLayoutPtr layout;
    /* Load the current layout */
    layout = layout_load_from_server(ctk_object->handle, &err_str);


    /* Handle errors loading the new layout */
    if (!layout || err_str) {
        nv_error_msg(err_str);
        g_free(err_str);
        return;
    }


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


    /* Setup the GUI */
    setup_layout_frame(ctk_object);
    setup_display_page(ctk_object);
    setup_screen_page(ctk_object);

    /* Get new position */
    get_cur_screen_pos(ctk_object);

    /* Clear the apply button */
    ctk_object->apply_possible = TRUE;
    gtk_widget_set_sensitive(ctk_object->btn_apply, FALSE);

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
        gtk_widget_set_sensitive(ctk_object->btn_apply, False);
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
 * unresgister itself by returning FALSE.
 *
 **/

static void display_config_attribute_changed(GtkObject *object, gpointer arg1,
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
        !(GTK_WIDGET_VISIBLE(ctk_object->box_validation_override_details));

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
