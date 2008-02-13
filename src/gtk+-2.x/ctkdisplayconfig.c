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

#include <stdlib.h> /* malloc */
#include <unistd.h> /* lseek, close */
#include <string.h> /* strlen,  strdup */
#include <errno.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <NvCtrlAttributes.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include "msg.h"

#include "blank_banner.h"

#include "ctkimage.h"
#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkdisplayconfig.h"
#include "ctkdisplaylayout.h"


void layout_selected_callback(nvLayoutPtr layout, void *data);
void layout_modified_callback(nvLayoutPtr layout, void *data);

static void setup_layout_frame(CtkDisplayConfig *ctk_object);

static void setup_display_frame(CtkDisplayConfig *ctk_object);

static void display_config_clicked(GtkWidget *widget, gpointer user_data);

static void display_resolution_changed(GtkWidget *widget, gpointer user_data);
static void display_refresh_changed(GtkWidget *widget, gpointer user_data);

static void display_position_type_changed(GtkWidget *widget, gpointer user_data);
static void display_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void display_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void display_panning_activate(GtkWidget *widget, gpointer user_data);

static void setup_screen_frame(CtkDisplayConfig *ctk_object);

static void screen_depth_changed(GtkWidget *widget, gpointer user_data);

static void screen_position_type_changed(GtkWidget *widget, gpointer user_data);
static void screen_position_offset_activate(GtkWidget *widget, gpointer user_data);
static void screen_position_relative_changed(GtkWidget *widget, gpointer user_data);

static void screen_metamode_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_activate(GtkWidget *widget, gpointer user_data);
static void screen_metamode_add_clicked(GtkWidget *widget, gpointer user_data);
static void screen_metamode_delete_clicked(GtkWidget *widget, gpointer user_data);

static void xconfig_preview_clicked(GtkWidget *widget, gpointer user_data);
static void xconfig_file_clicked(GtkWidget *widget, gpointer user_data);

static void xinerama_state_toggled(GtkWidget *widget, gpointer user_data);
static void apply_clicked(GtkWidget *widget, gpointer user_data);
static void save_clicked(GtkWidget *widget, gpointer user_data);
static void probe_clicked(GtkWidget *widget, gpointer user_data);
static void advanced_clicked(GtkWidget *widget, gpointer user_data);
static void reset_clicked(GtkWidget *widget, gpointer user_data);
static void validation_details_clicked(GtkWidget *widget, gpointer user_data);




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

typedef void (* apply_token_func)(char *token, char *value,
                                  void *data);

typedef struct SwitchModeCallbackInfoRec {
    CtkDisplayConfig *ctk_object;
    int screen;
} SwitchModeCallbackInfo;




/*** G L O B A L S ***********************************************************/

static int __position_table[] = { CONF_ADJ_ABSOLUTE,
                                  CONF_ADJ_RIGHTOF,
                                  CONF_ADJ_LEFTOF,
                                  CONF_ADJ_ABOVE,
                                  CONF_ADJ_BELOW,
                                  CONF_ADJ_RELATIVE };


/* Layout tooltips */

static const char * __layout_xinerama_button_help =
"The Enable Xinerama checkbox enables the Xinerama X extension;  Changing "
"this option will require restarting your X server.";


/* Display tooltips */

static const char * __dpy_resolution_mnu_help =
"The Resolution drop-down allows you to select a desired resolution "
"for the currently selected display device.";

static const char * __dpy_refresh_mnu_help =
"The Refresh drop-down allows you to select a desired refresh rate "
"for the currently selected display device.  Note that the selected "
"resolution may restrict the available refresh rates.";

static const char * __dpy_position_type_help =
"The Position Type drop-down allows you to set how the selected display "
"device is placed within the X Screen.";

static const char * __dpy_position_relative_help =
"The Position Relative drop-down allows you to set which other display "
"device (within the X Screen) the selected display device should be "
"relative to.";

static const char * __dpy_position_offset_help =
"The Position Offset identifies the top left of the display device "
"as an offset from the top left of the X Screen position.";

static const char * __dpy_panning_help =
"The Panning Domain sets the total width/height that the display "
"device may pan within.";


/* Screen tooltips */

static const char * __screen_depth_help =
"The Depth drop-down allows setting of the color quality for the selected "
"screen;  Changing this option will require restarting your X server.";

static const char * __screen_position_type_help =
"The Position Type drop-down allows you to set how the selected screen "
"is placed within the X Server layout;  Changing this option will require "
"restarting your X server.";

static const char * __screen_position_relative_help =
"The Position Relative drop-down allows you to set which other Screen "
"the selected screen should be relative to;  Changing this option will "
"require restarting your X server.";

static const char * __screen_position_offset_help =
"The Position Offset identifies the top left of the selected Screen as "
"an offset from the top left of the X Server layout in absolute coordinates;  "
"Changing this option will require restarting your X server.";

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

static const char * __probe_button_help =
"The probe button allows you to probe for new display devices that may "
"have been hotplugged.";

static const char * __advanced_button_help =
"The Advanced/Basic button toggles between a basic view, and an advanced view "
"with extra configuration options.";

static const char * __reset_button_help =
"The Reset button will re-probe the X Server for current configuration.  Any "
"alterations you may have made (and not applied) will be lost.";

static const char * __save_button_help =
"The Save to X Configuration File button allows you to save the current "
"X Server configuration settings to an X Configuration file.";




/*** F U N C T I O N S *******************************************************/


/** get_display_type_str() *******************************************
 *
 * Returns the type name of a display (CRT, CRT-1, DFP ..)
 *
 * If 'generic' is set to 1, then a generic version of the name is
 * returned.
 *
 **/

static gchar *get_display_type_str(unsigned int device_mask, int be_generic)
{
    unsigned int  bit = 0;
    int           num;
    gchar        *name = NULL;
    gchar        *type_name;


    /* Get the generic type name of the display */
    if (device_mask & 0x000000FF) {
        name = g_strdup("CRT");
        bit  = (device_mask & 0x000000FF);

    } else if (device_mask & 0x0000FF00) {
        name = g_strdup("TV");
        bit  = (device_mask & 0x0000FF00) >> 8;

    } else if (device_mask & 0x00FF0000) {
        name = g_strdup("DFP");
        bit  = (device_mask & 0x00FF0000) >> 16;
    }

    if (be_generic || !name) {
        return name;
    }


    /* Add the specific display number to the name */
    num = 0;
    while (bit) {
        num++;
        bit >>= 1;
    }
    if (num) {
        num--;
    }

    type_name = g_strdup_printf("%s-%d", name, num);
    g_free(name);

    return type_name;

} /* get_display_type_str() */



/** get_mode_str() ***************************************************
 *
 * Returns the mode string of a mode:
 *
 * "mode_name @WxH +X+Y"
 *
 **/

static gchar * get_mode_str(nvModePtr mode, int be_generic)
{
    gchar *mode_str;
    gchar *tmp;

    
    /* Make sure the mode has everything it needs to be displayed */
    if (!mode || !mode->display || !mode->display->gpu || !mode->metamode) {
        return NULL;
    }


    /* Don't display dummy modes */
    if (be_generic && mode->dummy && !mode->modeline) {
        return NULL;
    }


    /* Only one display, be very generic (no 'CRT:' in metamode) */
    if (be_generic && mode->display->gpu->num_displays == 1) {
        mode_str = g_strdup("");

    /* If there's more than one CRT/DFP/TV, we can't be generic. */
    } else {
        int generic = be_generic;

        if ((mode->display->device_mask & 0x000000FF) &&
            (mode->display->device_mask !=
             (mode->display->gpu->connected_displays & 0x000000FF))) {
            generic = 0;
        }
        if ((mode->display->device_mask & 0x0000FF00) &&
            (mode->display->device_mask !=
             (mode->display->gpu->connected_displays & 0x0000FF00))) {
            generic = 0;
        }
        if ((mode->display->device_mask & 0x00FF0000) &&
            (mode->display->device_mask !=
             (mode->display->gpu->connected_displays & 0x00FF0000))) {
            generic = 0;
        }

        /* Get the display type */
        tmp = get_display_type_str(mode->display->device_mask, generic);
        mode_str = g_strconcat(tmp, ": ", NULL);
        g_free(tmp);
    }


    /* NULL mode */
    if (!mode->modeline) {
        tmp = g_strconcat(mode_str, "NULL", NULL);
        g_free(mode_str);
        return tmp;
    }


    /* Mode name */
    tmp = g_strconcat(mode_str, mode->modeline->data.identifier, NULL);
    g_free(mode_str);
    mode_str = tmp;


    /* Panning domain */
    if (!be_generic || (mode->pan[W] != mode->dim[W] ||
                        mode->pan[H] != mode->dim[H])) {
        tmp = g_strdup_printf("%s @%dx%d",
                              mode_str, mode->pan[W], mode->pan[H]);
        g_free(mode_str);
        mode_str = tmp;
    }


    /* Offset */

    /*
     * XXX Later, we'll want to allow the user to select how
     *     the metamodes are generated:
     * 
     *   Programability:
     *     make mode->dim relative to screen->dim
     *
     *   Coherency:
     *     make mode->dim relative to mode->metamode->edim
     *
     *
     * XXX Also, we may want to take in consideration the
     *     TwinViewOrientation when writing out position
     *     information.
     */

    tmp = g_strdup_printf("%s +%d+%d",
                          mode_str,
                          /* Make mode position relative */
                          mode->dim[X] - mode->metamode->edim[X],
                          mode->dim[Y] - mode->metamode->edim[Y]);
    g_free(mode_str);
    mode_str = tmp;


    return mode_str;

} /* get_mode_str() */



/** get_display_from_gpu() *******************************************
 *
 * Returns the display with the matching device_mask
 *
 **/
 
static nvDisplayPtr get_display_from_gpu(nvGpuPtr gpu,
                                         unsigned int device_mask)
{
    nvDisplayPtr display;

    for (display = gpu->displays; display; display = display->next) {
        if (display->device_mask == device_mask) return display;
    }
    
    return NULL;

} /* get_display_from_gpu() */



/** get_cur_screen_pos() *********************************************
 *
 * Grabs a copy of the currently selected screen position.
 *
 **/

static void get_cur_screen_pos(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    
    if (!display || !display->screen) return;

    ctk_object->cur_screen_pos[X] = display->screen->dim[X];
    ctk_object->cur_screen_pos[Y] = display->screen->dim[Y];

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


/** find_closest_mode_with_modeline() ********************************
 *
 * Helper function that returns the mode index of the display's mode
 * that best matches the given modeline.
 *
 * A best match is:
 *
 * - The modelines are the same.
 * - The modelines match in width & height.
 *
 **/

static int find_closest_mode_with_modeline(nvDisplayPtr display,
                                           nvModeLinePtr modeline)
{
    nvModePtr mode;
    int mode_idx;
    int match_idx = -1;
    
    mode_idx = 0;
    for (mode = display->modes; mode; mode = mode->next) {
        if (mode->modeline->data.vdisplay == modeline->data.vdisplay &&
            mode->modeline->data.hdisplay == modeline->data.hdisplay) {
            match_idx = mode_idx;
        }
        if (mode->modeline == modeline) break;
        mode_idx++;
    }

    return match_idx;

} /* find_closest_mode_with_modeline() */



/** Parsing Tools ****************************************************
 *
 * Some tools for parsing strings
 *
 */

static const char *skip_whitespace(const char *str)
{
    while (*str &&
           (*str == ' '  || *str == '\t' ||
            *str == '\n' || *str == '\r')) {
        str++;
    }
    return str;
}

static void chop_whitespace(char *str)
{
    char *tmp = str + strlen(str) -1;
    
    while (tmp >= str &&
           (*tmp == ' '  || *tmp == '\t' ||
            *tmp == '\n' || *tmp == '\r')) {
        *tmp = '\0';
        tmp++;
    }
}

static const char *skip_integer(const char *str)
{
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str && *str >= '0' && *str <= '9') {
        str++;
    }
    return str;
}

static const char *read_integer(const char *str, int *num)
{
    str = skip_whitespace(str);
    *num = atoi(str);
    str = skip_integer(str);
    return skip_whitespace(str);
}

static const char *read_pair(const char *str, char separator, int *a, int *b)
{
    str = read_integer(str, a);
    if (!str) return NULL;
    if (separator) {
        if (*str != separator) return NULL;
        str++;
    }
    return read_integer(str, b);
}

static const char *read_name(const char *str, char **name, char term)
{
    const char *tmp;

    str = skip_whitespace(str);
    tmp = str;
    while (*str && *str != term) {
        str++;
    }
    *name = (char *)calloc(1, str -tmp +1);
    if (!(*name)) {
        return NULL;
    }
    strncpy(*name, tmp, str -tmp);
    if (*str == term) {
        str++;
    }
    return skip_whitespace(str);
}

/* Convert 'CRT-1' display device type names into a device_mask
 * '0x00000002' bitmask
 */
static const char *read_display_name(const char *str, unsigned int *bit)
{
    if (!str || !bit) {
        return NULL;
    }

    str = skip_whitespace(str);
    if (!strncmp(str, "CRT-", 4)) {
        *bit = 1 << (atoi(str+4));

    } else if (!strncmp(str, "TV-", 3)) {
        *bit = (1 << (atoi(str+3))) << 8;

    } else if (!strncmp(str, "DFP-", 4)) {
        *bit = (1 << (atoi(str+4))) << 16;

    } else {
        return NULL;
    }

    while (*str && *str != ':') {
        str++;
    }
    if (*str == ':') {
        str++;
    }

    return skip_whitespace(str);
}

static int read_float_range(char *str, float *min, float *max)
{
    if (!str) return 0;

    str = (char *)skip_whitespace(str);
    *min = atof(str);
    str = strstr(str, "-");
    if (!str) return 0;
    str++;
    *max = atof(str);
    
    return 1;
}



/** apply_modeline_token() *******************************************
 *
 * Modifies the modeline structure given with the token/value pair
 * given.
 *
 **/

static void apply_modeline_token(char *token, char *value, void *data)
{
    nvModeLinePtr modeline = (nvModeLinePtr) data;

    if (!modeline || !token || !strlen(token)) {
        return;
    }

    /* Modeline Source */
    if (!strcasecmp("source", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("Modeline 'source' token requires a value!");
        } else if (!strcasecmp("xserver", value)) {
            modeline->source |=  MODELINE_SOURCE_XSERVER;
        } else if (!strcasecmp("xconfig", value)) {
            modeline->source |=  MODELINE_SOURCE_XCONFIG;
        } else if (!strcasecmp("builtin", value)) {
            modeline->source |=  MODELINE_SOURCE_BUILTIN;
        } else if (!strcasecmp("vesa", value)) {
            modeline->source |=  MODELINE_SOURCE_VESA;
        } else if (!strcasecmp("edid", value)) {
            modeline->source |=  MODELINE_SOURCE_EDID;
        } else if (!strcasecmp("nv-control", value)) {
            modeline->source |=  MODELINE_SOURCE_NVCONTROL;
        } else {
            nv_warning_msg("Unknown modeline source '%s'", value);
        }

    /* X Config name */
    } else if (!strcasecmp("xconfig-name", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("Modeline 'xconfig-name' token requires a value!");
        } else {
            if (modeline->xconfig_name) {
                free(modeline->xconfig_name);
            }
            modeline->xconfig_name = g_strdup(value);
        }

    /* Unknown token */
    } else {
        nv_warning_msg("Unknown modeline token value pair: %s=%s",
                       token, value);
    }

} /* apply_modeline_token() */



/** apply_metamode_token() *******************************************
 *
 * Modifies the metamode structure given with the token/value pair
 * given.
 *
 **/

static void apply_metamode_token(char *token, char *value, void *data)
{
    nvMetaModePtr metamode = (nvMetaModePtr) data;
    
    if (!metamode || !token || !strlen(token)) {
        return;
    }

    /* Metamode ID */
    if (!strcasecmp("id", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("MetaMode 'id' token requires a value!");
        } else {
            metamode->id = atoi(value);
        }

    /* Modeline Source */
    } else if (!strcasecmp("source", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("MetaMode 'source' token requires a value!");
        } else if (!strcasecmp("xconfig", value)) {
            metamode->source |= METAMODE_SOURCE_XCONFIG;
        } else if (!strcasecmp("implicit", value)) {
            metamode->source |= METAMODE_SOURCE_IMPLICIT;
        } else if (!strcasecmp("nv-control", value)) {
            metamode->source |= METAMODE_SOURCE_NVCONTROL;
        } else {
            nv_warning_msg("Unknown MetaMode source '%s'", value);
        }

    /* Switchable */
    } else if (!strcasecmp("switchable", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("MetaMode 'switchable' token requires a value!");
        } else {
            if (!strcasecmp(value, "yes")) {
                metamode->switchable = TRUE;
            } else {
                metamode->switchable = FALSE;
            }
        }

    /* Unknown token */
    } else {
        nv_warning_msg("Unknown MetaMode token value pair: %s=%s",
                       token, value);
    }

} /* apply_metamode_token */



/** apply_monitor_token() ********************************************
 *
 * Returns the source string of a refresh/sync range.
 *
 **/

static void apply_monitor_token(char *token, char *value, void *data)
{
    char **source = (char **)data;
    
    if (!source || !token || !strlen(token)) {
        return;
    }

    /* Metamode ID */
    if (!strcasecmp("source", token)) {
        if (*source) free(*source);
        *source = strdup(value);

    /* Unknown token */
    } else {
        nv_warning_msg("Unknown monitor range token value pair: %s=%s",
                       token, value);
    }

} /* apply_monitor_token() */



/** apply_screen_info_token() ****************************************
 *
 * Modifies the ScreenInfo structure (pointed to by data) with
 * information from the token-value pair given.  Currently accepts
 * position and width/height data.
 *
 **/

typedef struct _ScreenInfo {
    int x;
    int y;
    int width;
    int height;
} ScreenInfo;

static void apply_screen_info_token(char *token, char *value, void *data)
{
    ScreenInfo *screen_info = (ScreenInfo *)data;
    
    if (!screen_info || !token || !strlen(token)) {
        return;
    }

    /* X */
    if (!strcasecmp("x", token)) {
        screen_info->x = atoi(value);

    /* Y */
    } else if (!strcasecmp("y", token)) {
        screen_info->y = atoi(value);

    /* Width */
    } else if (!strcasecmp("width", token)) {
        screen_info->width = atoi(value);

    /* Height */
    } else if (!strcasecmp("height", token)) {
        screen_info->height = atoi(value);

    /* Unknown token */
    } else {
        nv_warning_msg("Unknown screen info token value pair: %s=%s",
                       token, value);
    }

} /* apply_screen_info_token() */



/** parse_tokens() ***************************************************
 *
 * Parses the given tring for "token=value, token=value, ..." pairs
 * and dispatches the handeling of tokens to the given function with
 * the given data as an extra argument.
 *
 **/

static Bool parse_tokens(const char *str, apply_token_func func, void *data)
{
    char *token;
    char *value;


    if (str) {

        /* Parse each token */
        while (*str) {
            
            /* Read the token */
            str = read_name(str, &token, '=');
            if (!str) return FALSE;
            
            /* Read the value */
            str = read_name(str, &value, ',');
            if (!str) return FALSE;
            
            /* Remove trailing whitespace */
            chop_whitespace(token);
            chop_whitespace(value);
            
            func(token, value, data);
            
            free(token);
            free(value);
        }
    }

    return TRUE;

} /* parse_tokens() */



/** parse_modeline() *************************************************
 *
 * Converts a modeline string to an X config modeline structure that
 * the XF86Parser backend can read.
 *
 *
 * Modeline strings have this format:
 *
 *   "mode_name"  dot_clock  timings  flags
 *
 **/

nvModeLinePtr parse_modeline(const char *modeline_str)
{
    nvModeLinePtr modeline = NULL;
    const char *str = modeline_str;
    char *tmp;
    char *tokens;


    if (!str) return NULL;

    modeline = (nvModeLinePtr)calloc(1, sizeof(nvModeLine));
    if (!modeline) return NULL;

    /* Parse the modeline tokens */
    tmp = strstr(str, "::");
    if (tmp) {
        tokens = strdup(str);
        tokens[ tmp-str ] = '\0';
        str = tmp +2;
        parse_tokens(tokens, apply_modeline_token, (void *)modeline);
        free(tokens);
    }

    /* Read the mode name */
    str = skip_whitespace(str);
    if (!str || *str != '"') goto fail;
    str++;
    str = read_name(str, &(modeline->data.identifier), '"');
    if (!str) goto fail;

    /* Read dot clock */
    {
        int digits = 100;

        str = read_integer(str, &(modeline->data.clock));
        modeline->data.clock *= 1000;
        if (*str == '.') {
            str++;
            while (digits &&
                   *str &&
                   *str != ' '  && *str != '\t' &&
                   *str != '\n' && *str != '\r') {

                modeline->data.clock += digits * (*str - '0');
                digits /= 10;
                str++;
            }
        }
        str = skip_whitespace(str);
    }

    str = read_integer(str, &(modeline->data.hdisplay));
    str = read_integer(str, &(modeline->data.hsyncstart));
    str = read_integer(str, &(modeline->data.hsyncend));
    str = read_integer(str, &(modeline->data.htotal));
    str = read_integer(str, &(modeline->data.vdisplay));
    str = read_integer(str, &(modeline->data.vsyncstart));
    str = read_integer(str, &(modeline->data.vsyncend));
    str = read_integer(str, &(modeline->data.vtotal));


    /* Parse modeline flags */
    while ((str = read_name(str, &tmp, ' ')) && strlen(tmp)) {
        
        if (!xconfigNameCompare(tmp, "+hsync")) {
            modeline->data.flags |= XCONFIG_MODE_PHSYNC;
        }
        else if (!xconfigNameCompare(tmp, "-hsync")) {
            modeline->data.flags |= XCONFIG_MODE_NHSYNC;
        }
        else if (!xconfigNameCompare(tmp, "+vsync")) {
            modeline->data.flags |= XCONFIG_MODE_PVSYNC;
        }
        else if (!xconfigNameCompare(tmp, "-vsync")) {
            modeline->data.flags |= XCONFIG_MODE_NVSYNC;
        }
        else if (!xconfigNameCompare(tmp, "interlace")) {
            modeline->data.flags |= XCONFIG_MODE_INTERLACE;
        }
        else if (!xconfigNameCompare(tmp, "doublescan")) {
            modeline->data.flags |= XCONFIG_MODE_DBLSCAN;
        }
        else if (!xconfigNameCompare(tmp, "composite")) {
            modeline->data.flags |= XCONFIG_MODE_CSYNC;
        }
        else if (!xconfigNameCompare(tmp, "+csync")) {
            modeline->data.flags |= XCONFIG_MODE_PCSYNC;
        }
        else if (!xconfigNameCompare(tmp, "-csync")) {
            modeline->data.flags |= XCONFIG_MODE_NCSYNC;
        }
        else if (!xconfigNameCompare(tmp, "hskew")) {
            str = read_integer(str, &(modeline->data.hskew));
            if (!str) {
                free(tmp);
                goto fail;
            }
            modeline->data.flags |= XCONFIG_MODE_HSKEW;
        }
        else if (!xconfigNameCompare(tmp, "bcast")) {
            modeline->data.flags |= XCONFIG_MODE_BCAST;
        }
        else if (!xconfigNameCompare(tmp, "CUSTOM")) {
            modeline->data.flags |= XCONFIG_MODE_CUSTOM;
        }
        else if (!xconfigNameCompare(tmp, "vscan")) {
            str = read_integer(str, &(modeline->data.vscan));
            if (!str) {
                free(tmp);
                goto fail;
            }
            modeline->data.flags |= XCONFIG_MODE_VSCAN;
        }
        else {
            nv_warning_msg("Invalid modeline keyword '%s' in modeline '%s'",
                           tmp, modeline_str);
            goto fail;
        }
        free(tmp);
    }

    return modeline;


    /* Handle failures */
 fail:
    free(modeline);
    return NULL;

} /* parse_modeline() */



/** parse_mode() *****************************************************
 *
 * Makes a mode structure from the mode string given.
 *
 *
 * Currently supported mode syntax:
 *
 *   "mode_name" +X+Y @WxH
 *
 **/

nvModePtr parse_mode(nvDisplayPtr display, const char *mode_str)
{
    nvModePtr   mode;
    char       *mode_name; /* Modeline reference name */
    const char *str = mode_str;



    if (!str || !display || !display->modelines) return NULL;


    /* Allocate a Mode structure */
    mode = (nvModePtr)calloc(1, sizeof(nvMode));
    if (!mode) return NULL;

    mode->display = display;


    /* Read the mode name */
    str = read_name(str, &mode_name, ' ');
    if (!str || !mode_name) goto fail;


    /* Match the mode name to one of the modelines */
    mode->modeline = display->modelines;
    while (mode->modeline) {
        if (!strcmp(mode_name, mode->modeline->data.identifier)) {
            break;
        }
        mode->modeline = mode->modeline->next;
    }
    free(mode_name);


    /* If we can't find a matching modeline, show device as off
     * using the width & height of whatever the first modeline is.
     * XXX Hopefully this is the default width/height.
     */
    if (!mode->modeline) {
        if (strcmp(mode_str, "NULL")) {
            nv_warning_msg("Mode name '%s' does not match any modelines for "
                           "display device '%s' in modeline '%s'.",
                           mode_name, display->name, mode_str);
        }
        mode->dim[W] = display->modelines->data.hdisplay;
        mode->dim[H] = display->modelines->data.vdisplay;
        mode->pan[W] = mode->dim[W];
        mode->pan[H] = mode->dim[H];
        return mode;
    }


    /* Setup default size and panning of display */
    mode->dim[W] = mode->modeline->data.hdisplay;
    mode->dim[H] = mode->modeline->data.vdisplay;
    mode->pan[W] = mode->dim[W];
    mode->pan[H] = mode->dim[H];


    /* Read mode information */
    while (*str) {

        /* Read panning */
        if (*str == '@') {
            str++;
            str = read_pair(str, 'x',
                            &(mode->pan[W]), &(mode->pan[H]));
        }

        /* Read position */
        else if (*str == '+') {
            str++;
            str = read_pair(str, 0,
                            &(mode->dim[X]), &(mode->dim[Y]));
        }
        
        /* Mode parse error - Ack! */
        else {
            str = NULL;
        }

        /* Catch errors */
        if (!str) goto fail;
    }
    
        
    /* These are the same for now */
    mode->pan[X] = mode->dim[X];
    mode->pan[Y] = mode->dim[Y];


    /* Panning can't be smaller than dimensions */
    if (mode->pan[W] < mode->dim[W]) {
        mode->pan[W] = mode->dim[W];
    }
    if (mode->pan[W] < mode->dim[W]) {
        mode->pan[W] = mode->dim[W];
    }

    return mode;


    /* Handle failures */
 fail:
    if (mode) {
        free(mode);
    }
    
    return NULL;

} /* parse_mode() */



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



/* Layout save/write functions ***************************************/


/** get_display_mode_str() *******************************************
 *
 * Returns the mode string of the display's 'mode_idx''s
 * mode.
 *
 **/
static gchar *get_display_mode_str(nvDisplayPtr display, int mode_idx,
                                   int be_generic)
{
    nvModePtr mode = display->modes;

    while (mode && mode_idx) {
        mode = mode->next;
        mode_idx--;
    }
    
    if (mode) {
        return get_mode_str(mode, be_generic);
    }

    return NULL;

} /* get_display_mode_str() */



/** get_screen_metamode_str() ****************************************
 *
 * Returns a screen's metamode string for the given metamode index
 * as:
 *
 * "mode1_1, mode1_2, mode1_3 ... "
 *
 **/

static gchar *get_screen_metamode_str(nvScreenPtr screen, int metamode_idx,
                                     int be_generic)
{
    nvDisplayPtr display;

    gchar *metamode_str = NULL;
    gchar *mode_str;
    gchar *tmp;

    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue; /* Display not in screen */
    
        mode_str = get_display_mode_str(display, metamode_idx, be_generic);
        if (!mode_str) continue;

        if (!metamode_str) {
            metamode_str = mode_str;
        } else {
            tmp = g_strdup_printf("%s, %s", metamode_str, mode_str);
            g_free(mode_str);
            g_free(metamode_str);
            metamode_str = tmp;
        }
    }

    return metamode_str;

} /* get_screen_metamode_str() */



/** get_screen_metamode_strs() ***************************************
 *
 * Returns the metamode strings of a screen:
 *
 * "mode1_1, mode1_2, mode1_3 ... ; mode 2_1, mode 2_2, mode 2_3 ... ; ..."
 *
 **/

static gchar *get_screen_metamode_strs(nvScreenPtr screen, int be_generic,
                                       int cur_mode_first)
{
    gchar *metamode_strs = NULL;
    gchar *metamode_str;
    gchar *tmp;
    int metamode_idx;
    nvMetaModePtr metamode;


    /* The current mode should appear first in the list */
    if (cur_mode_first) {
        metamode_strs = get_screen_metamode_str(screen,
                                                screen->cur_metamode_idx,
                                                be_generic);
    }

    for (metamode_idx = 0, metamode = screen->metamodes;
         (metamode_idx < screen->num_metamodes) && metamode;
         metamode_idx++, metamode = metamode->next) {

        /* Only write out metamodes that were specified by the user */
        if (!(metamode->source & METAMODE_SOURCE_USER)) continue;

        /* The current mode was already included */
        if (cur_mode_first && (metamode_idx == screen->cur_metamode_idx))
            continue;

        metamode_str = get_screen_metamode_str(screen, metamode_idx,
                                               be_generic);
        if (!metamode_str) continue;

        if (!metamode_strs) {
            metamode_strs = metamode_str;
        } else {
            tmp = g_strconcat(metamode_strs, "; ", metamode_str, NULL);
            g_free(metamode_str);
            g_free(metamode_strs);
            metamode_strs = tmp;
        }
    }

    return metamode_strs;

} /* get_screen_metamode_strs() */




/* Metamode functions ************************************************/


/** remove_modes_from_display() **************************************
 *
 * Removes all modes currently referenced by this screen, also
 * freeing any memory used.
 *
 **/

static void remove_modes_from_display(nvDisplayPtr display)
{
    nvModePtr mode;

    if (display) {
        while (display->modes) {
            mode = display->modes;
            display->modes = mode->next;
            free(mode);
        }
        display->num_modes = 0;
        display->cur_mode = NULL;
    }

} /* remove_modes_from_display() */



/** add_metamode_to_screen() *****************************************
 *
 * Parses a metamode string and adds the appropreate modes to the
 * screen's display devices (at the end of the list)
 *
 */

static Bool add_metamode_to_screen(nvScreenPtr screen, char *metamode_str,
                                   gchar **err_str)
{
    char *mode_str;
    char *str = NULL;
    char *tmp;
    char *tokens;
    nvMetaModePtr metamode;


    if (!screen || !screen->gpu || !metamode_str) goto fail;


    metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
    if (!metamode) goto fail;


    /* Copy the string so we can split it up */
    str = strdup(metamode_str);
    if (!str) goto fail;
    

    /* Read the MetaMode ID */
    tmp = strstr(str, "::");
    if (tmp) {
        tokens = strdup(str);
        tokens[ tmp-str ] = '\0';
        tmp += 2;
        parse_tokens(tokens, apply_metamode_token, (void *)metamode);
        free(tokens);
    } else {
        /* No tokens?  Try the old "ID: METAMODE_STR" syntax */
        tmp = (char *)read_integer(str, &(metamode->id));
        metamode->source = METAMODE_SOURCE_NVCONTROL;
        if (*tmp == ':') {
            tmp++;
        }
    }


    /* Add the metamode at the end of the screen's metamode list */
    screen->metamodes =
        (nvMetaModePtr)xconfigAddListItem((GenericListPtr)screen->metamodes,
                                          (GenericListPtr)metamode);


    /* Split up the metamode into separate modes */
    for (mode_str = strtok(tmp, ",");
         mode_str;
         mode_str = strtok(NULL, ",")) {
        
        nvModePtr     mode;
        unsigned int  device_mask;
        nvDisplayPtr  display;
        const char *orig_mode_str = skip_whitespace(mode_str);


        /* Parse the display device bitmask from the name */
        mode_str = (char *)read_display_name(mode_str, &device_mask);
        if (!mode_str) {
            *err_str = g_strdup_printf("Failed to read a display device name "
                                       "on screen %d (on GPU-%d)\nwhile "
                                       "parsing metamode:\n\n'%s'",
                                       screen->scrnum,
                                       NvCtrlGetTargetId(screen->gpu->handle),
                                       orig_mode_str);
            nv_error_msg(*err_str);
            goto fail;
        }


        /* Match device bitmask to an existing display */
        display = get_display_from_gpu(screen->gpu, device_mask);
        if (!display) {
            *err_str = g_strdup_printf("Failed to find display device 0x%08x "
                                       "on screen %d (on GPU-%d)\nwhile "
                                       "parsing metamode:\n\n'%s'",
                                       device_mask,
                                       screen->scrnum,
                                       NvCtrlGetTargetId(screen->gpu->handle),
                                       orig_mode_str);
            nv_error_msg(*err_str);
            goto fail;
        }


        /* Parse the mode */
        mode = parse_mode(display, mode_str);
        if (!mode) {
            *err_str = g_strdup_printf("Failed to parse mode '%s'\non "
                                       "screen %d (on GPU-%d)\nfrom "
                                       "metamode:\n\n'%s'",
                                       mode_str, screen->scrnum,
                                       NvCtrlGetTargetId(screen->gpu->handle),
                                       orig_mode_str);
            nv_error_msg(*err_str);
            goto fail;
        }


        /* Make the mode part of the metamode */
        mode->metamode = metamode;


        /* Make the display part of the screen */
        display->screen = screen;


        /* Set the panning offset */
        mode->pan[X] = mode->dim[X];
        mode->pan[Y] = mode->dim[Y];
        

        /* Add the mode at the end of the display's mode list */
        display->modes =
            (nvModePtr)xconfigAddListItem((GenericListPtr)display->modes,
                                          (GenericListPtr)mode);
        display->num_modes++;
        
    } /* Done parsing a single metamode */

    free(str);
    return TRUE;


    /* Failure case */
 fail:
    
    /* XXX We should probably track which modes were added and remove
     *     them at this point.  For now, just assume the caller will
     *     remove all the modes and bail.
     */

    free(str);
    return FALSE;

} /* add_metamode_to_screen() */



/** check_screen_metamodes() *****************************************
 *
 * Makes sure all displays associated with the screen have the right
 * number of mode entries.
 *
 **/

static Bool check_screen_metamodes(nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvMetaModePtr metamode;
    nvModePtr mode;
    nvModePtr last_mode = NULL;


    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue;

        if (display->num_modes == screen->num_metamodes) continue;

        mode = display->modes;
        metamode = screen->metamodes;
        while (mode && metamode) {
            mode = mode->next;
            metamode = metamode->next;
            if (mode) {
                last_mode = mode;
            }
        }

        /* Each display must have as many modes as it's screen has metamodes */
        while (metamode) {

            /* Create a dumy mode */
            mode = parse_mode(display, "NULL");
            mode->dummy = 1;
            mode->metamode = metamode;

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
            display->modes =
                (nvModePtr)xconfigAddListItem((GenericListPtr)display->modes,
                                              (GenericListPtr)mode);
            display->num_modes++;

            metamode = metamode->next;
        }

        /* XXX Shouldn't need to remove extra modes.
        while (mode) {
        }
        */
    }

    return TRUE;

} /* check_screen_metamodes() */



/** assign_dummy_metamode_positions() ********************************
 *
 * Assign the initial (top left) position of dummy modes to
 * match the top left of the first non-dummy mode
 *
 */

static void assign_dummy_metamode_positions(nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvModePtr ok_mode;
    nvModePtr mode;
    

    for (display = screen->gpu->displays; display; display = display->next) {
        if (display->screen != screen) continue;
        
        /* Get the first non-dummy mode */
        for (ok_mode = display->modes; ok_mode; ok_mode = ok_mode->next) {
            if (!ok_mode->dummy) break;
        }

        if (ok_mode) {
            for (mode = display->modes; mode; mode = mode->next) {
                if (!mode->dummy) continue;
                mode->dim[X] = ok_mode->dim[X];
                mode->pan[X] = ok_mode->dim[X];
                mode->dim[Y] = ok_mode->dim[Y];
                mode->pan[Y] = ok_mode->dim[Y];
            }
        }
    }
    
} /* assign_dummy_metamode_positions() */



/** remove_metamodes_from_screen() ***********************************
 *
 * Removes all metamodes currently referenced by this screen, also
 * freeing any memory used.
 *
 **/

static void remove_metamodes_from_screen(nvScreenPtr screen)
{
    nvGpuPtr gpu;
    nvDisplayPtr display;
    nvMetaModePtr metamode;

    if (screen) {
        gpu = screen->gpu;

        /* Remove the modes from this screen's displays */
        if (gpu) {
            for (display = gpu->displays; display; display = display->next) {

                if (display->screen != screen) continue;

                remove_modes_from_display(display);
            }
        }

        /* Clear the screen's metamode list */
        while (screen->metamodes) {
            metamode = screen->metamodes;
            screen->metamodes = metamode->next;
            free(metamode->string);
            free(metamode);
        }
        screen->num_metamodes = 0;
        screen->cur_metamode = NULL;
        screen->cur_metamode_idx = -1;
    }
        
} /* remove_metamodes_from_screen() */



/** add_metamodes_to_screen() ****************************************
 *
 * Adds all the appropreate modes on all display devices of this
 * screen by parsing all the metamode strings.
 *
 */

static Bool add_metamodes_to_screen(nvScreenPtr screen, gchar **err_str)
{
    nvDisplayPtr display;

    char *metamode_strs = NULL;  /* Screen's list metamode strings */
    char *cur_metamode_str;      /* Current metamode */

    char *str;                   /* Temp pointer for parsing */
    int len;
    ReturnStatus ret;
    int i;



    /* Get the list of metamodes for the screen */
    ret = NvCtrlGetBinaryAttribute(screen->handle, 0,
                                   NV_CTRL_BINARY_DATA_METAMODES,
                                   (unsigned char **)&metamode_strs,
                                   &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query list of metamodes on\n"
                                   "screen %d (on GPU-%d).",
                                   screen->scrnum,
                                   NvCtrlGetTargetId(screen->gpu->handle));
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Get the current metamode for the screen */
    ret = NvCtrlGetStringAttribute(screen->handle,
                                   NV_CTRL_STRING_CURRENT_METAMODE,
                                   &cur_metamode_str);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query current metamode of\n"
                                   "screen %d (on GPU-%d).",
                                   screen->scrnum,
                                   NvCtrlGetTargetId(screen->gpu->handle));
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Remove any existing modes on all displays */
    remove_metamodes_from_screen(screen);


    /* Parse each mode in the metamode strings */
    str = metamode_strs;
    while (str && strlen(str)) {

        /* Add the individual metamodes to the screen,
         * This populates the display device's mode list.
         */
        if (!add_metamode_to_screen(screen, str, err_str)) {
            nv_warning_msg("Failed to add metamode '%s' to screen %d (on "
                           "GPU-%d).",
                           str, screen->scrnum,
                           NvCtrlGetTargetId(screen->gpu->handle));
            goto fail;
        }
        
        /* Keep track of the current metamode */
        if (!strcmp(str, cur_metamode_str)) {
            screen->cur_metamode_idx = screen->num_metamodes;
        }

        /* Keep count of the metamode */
        screen->num_metamodes++;

        /* Make sure each display device gets a mode */
        check_screen_metamodes(screen);

        /* Go to the next metamode */
        str += strlen(str) +1;
    }
    XFree(metamode_strs);


    /* Assign the top left position of dummy modes */
    assign_dummy_metamode_positions(screen);


    /* Make the screen point at the current metamode */
    screen->cur_metamode = screen->metamodes;
    for (i = 0; i < screen->cur_metamode_idx; i++) {
        screen->cur_metamode = screen->cur_metamode->next;
    }
    

    /* Make each display within the screen point to the current mode.
     * Also, count the number of displays on the screen
     */
    screen->num_displays = 0;
    for (display = screen->gpu->displays; display; display = display->next) {
        
        if (display->screen != screen) continue; /* Display not in screen */

        screen->num_displays++;
        screen->displays_mask |= display->device_mask;

        display->cur_mode = display->modes;
        for (i = 0; i < screen->cur_metamode_idx; i++) {
            display->cur_mode = display->cur_mode->next;
        }
    }

    return TRUE;


    /* Failure case */
 fail:
    
    /* Remove modes we may have added */
    remove_metamodes_from_screen(screen);

    XFree(metamode_strs);
    return FALSE;    

} /* add_metamodes_to_screen() */



/* Screen functions **************************************************/


/** remove_display_from_screen() *************************************
 *
 * Removes a display device from the screen
 *
 */
static void remove_display_from_screen(nvDisplayPtr display)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvDisplayPtr other;
    nvModePtr mode;


    if (display && display->screen) {
        screen = display->screen;
        gpu = display->gpu;

        /* Make any display relative to this one use absolute position */
        for (other = gpu->displays; other; other = other->next) {

            if (other == display) continue;
            if (other->screen != screen) continue;

            for (mode = other->modes; mode; mode = mode->next) {
                if (mode->relative_to == display) {
                    mode->position_type = CONF_ADJ_ABSOLUTE;
                    mode->relative_to = NULL;
                }
            }
        }

        /* Remove the display from the screen */
        screen->displays_mask &= ~(display->device_mask);
        screen->num_displays--;

        /* Clean up old references to the screen in the display */
        remove_modes_from_display(display);
        display->screen = NULL;
    }

} /* remove_display_from_screen() */



/** remove_displays_from_screen() ************************************
 *
 * Removes all displays currently pointing at this screen, also
 * freeing any memory used.
 *
 **/

void remove_displays_from_screen(nvScreenPtr screen)
{
    nvGpuPtr gpu;
    nvDisplayPtr display;

    if (screen && screen->gpu) {
        gpu = screen->gpu;
        
        for (display = gpu->displays; display; display = display->next) {
            
            if (display->screen != screen) continue;
            
            remove_display_from_screen(display);
        }
    }

} /* remove_displays_from_screen() */



/** free_screen() ****************************************************
 *
 * Frees memory used by a screen structure
 *
 */
static void free_screen(nvScreenPtr screen)
{
    if (screen) {

        remove_metamodes_from_screen(screen);
        remove_displays_from_screen(screen);

        if (screen->handle) {
            NvCtrlAttributeClose(screen->handle);
        }

        free(screen);
    }

} /* free_screens() */



/** add_screen_to_gpu() **********************************************
 *
 * Adds screen 'screen_id' that is connected to the gpu.
 *
 */
static int add_screen_to_gpu(nvGpuPtr gpu, int screen_id, gchar **err_str)
{
    Display *display;
    nvScreenPtr screen;
    int val;
    ReturnStatus ret;


    /* Create the screen structure */
    screen = (nvScreenPtr)calloc(1, sizeof(nvScreen));
    if (!screen) goto fail;

    screen->gpu = gpu;
    screen->scrnum = screen_id;


    /* Make an NV-CONTROL handle to talk to the screen */
    display = NvCtrlGetDisplayPtr(gpu->handle);
    screen->handle =
        NvCtrlAttributeInit(display,
                            NV_CTRL_TARGET_TYPE_X_SCREEN,
                            screen_id,
                            NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM |
                            NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM);
    if (!screen->handle) {
        *err_str = g_strdup_printf("Failed to create NV-CONTROL handle for\n"
                                   "screen %d (on GPU-%d).",
                                   screen_id, NvCtrlGetTargetId(gpu->handle));
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Make sure this screen supports dynamic twinview */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_DYNAMIC_TWINVIEW,
                             &val);
    if (ret != NvCtrlSuccess || !val) {
        *err_str = g_strdup_printf("Dynamic TwinView is disabled on "
                                   "screen %d.",
                                   screen_id);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* The display owner GPU gets the screen(s) */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_MULTIGPU_DISPLAY_OWNER,
                             &val);
    if (ret != NvCtrlSuccess || val != NvCtrlGetTargetId(gpu->handle)) {
        free_screen(screen);
        return TRUE;
    }


    /* Listen to NV-CONTROL events on this screen handle */
    screen->ctk_event = CTK_EVENT(ctk_event_new(screen->handle));


    /* Query the depth of the screen */
    screen->depth = NvCtrlGetScreenPlanes(screen->handle);


    /* Parse the screen's metamodes (ties displays on the gpu to the screen) */
    if (!add_metamodes_to_screen(screen, err_str)) {
        nv_warning_msg("Failed to add metamodes to screen %d (on GPU-%d).",
                       screen_id, NvCtrlGetTargetId(gpu->handle));
        goto fail;
    }


    /* Add the screen at the end of the gpu's screen list */
    gpu->screens =
        (nvScreenPtr)xconfigAddListItem((GenericListPtr)gpu->screens,
                                        (GenericListPtr)screen);
    gpu->num_screens++;
    return TRUE;


 fail:
    free_screen(screen);
    return FALSE;

} /* add_screen_to_gpu() */



/** remove_screen_from_gpu() *****************************************
 *
 * Removes a screen from a gpu.
 *
 */
static void remove_screen_from_gpu(nvScreenPtr screen)
{
    nvGpuPtr gpu;
    nvScreenPtr other;

    if (!screen || !screen->gpu) return;

    /* Remove the screen from the GPU */
    gpu = screen->gpu;
    
    gpu->screens =
        (nvScreenPtr)xconfigRemoveListItem((GenericListPtr)gpu->screens,
                                               (GenericListPtr)screen);
    gpu->num_screens--;

    /* Make sure other screens in the layout aren't relative
     * to this screen
     */
    for (gpu = screen->gpu->layout->gpus; gpu; gpu = gpu->next) {
        for (other = gpu->screens; other; other = other->next) {
            if (other->relative_to == screen) {
                other->position_type = CONF_ADJ_ABSOLUTE;
                other->relative_to = NULL;
            }
        }
    }

    screen->gpu = NULL;

    /* XXX May want to remove metamodes here */
    /* XXX May want to remove displays here */

} /* remove_screen_from_gpu() */



/** remove_screens_from_gpu() ****************************************
 *
 * Removes all screens from a gpu.
 *
 */
static void remove_screens_from_gpu(nvGpuPtr gpu)
{
    nvScreenPtr screen;

    if (gpu) {
        while (gpu->screens) {
            screen = gpu->screens;
            gpu->screens = screen->next;
            free_screen(screen);
        }
        gpu->num_screens = 0;
    }

} /* remove_screens_from_gpu() */



/** add_screens_to_gpu() *********************************************
 *
 * Queries the list of screens on the gpu.
 *
 */
static Bool add_screens_to_gpu(nvGpuPtr gpu, gchar **err_str)
{
    ReturnStatus ret;
    int *pData;
    int len;
    int i;


    /* Clean up the GPU list */
    remove_screens_from_gpu(gpu);


    /* Query the list of X Screens this GPU is driving */
    ret = NvCtrlGetBinaryAttribute(gpu->handle, 0,
                                   NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                                   (unsigned char **)(&pData), &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query list of screens driven\n"
                                   "by GPU-%d '%s'.",
                                   NvCtrlGetTargetId(gpu->handle), gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Add each X Screen */
    for (i = 1; i <= pData[0]; i++) {
        if (!add_screen_to_gpu(gpu, pData[i], err_str)) {
            nv_warning_msg("Failed to add screen %d to GPU-%d '%s'.",
                           pData[i], NvCtrlGetTargetId(gpu->handle),
                           gpu->name);
            goto fail;
        }
    }

    return TRUE;


    /* Failure case */
 fail:
    remove_screens_from_gpu(gpu);
    return FALSE;

} /* add_screens_to_gpu() */



/* Display device functions ******************************************/


/** remove_modelines_from_display() **********************************
 *
 * Clears the display device's modeline list.
 *
 **/

static void remove_modelines_from_display(nvDisplayPtr display)
{
    nvModeLinePtr modeline;
    
    if (display) {
        while (display->modelines) {
            modeline = display->modelines;
            display->modelines = display->modelines->next;
            free(modeline);
        }
        display->num_modelines = 0;
    }

} /* remove_modelines_from_display() */



/** add_modelines_to_display() ***************************************
 *
 * Queries the display's current modepool (modelines list).
 *
 **/

static Bool add_modelines_to_display(nvDisplayPtr display, gchar **err_str)
{
    nvModeLinePtr modeline;
    char *modeline_strs = NULL;
    char *str;
    int len;
    ReturnStatus ret;


    /* Free any old mode lines */
    remove_modelines_from_display(display);


    /* Get the validated modelines for the display */
    ret = NvCtrlGetBinaryAttribute(display->gpu->handle,
                                   display->device_mask,
                                   NV_CTRL_BINARY_DATA_MODELINES,
                                   (unsigned char **)&modeline_strs, &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query modelines of display "
                                   "device 0x%08x '%s'\nconnected to "
                                   "GPU-%d '%s'.",
                                   display->device_mask, display->name,
                                   NvCtrlGetTargetId(display->gpu->handle),
                                   display->gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Parse each modeline */
    str = modeline_strs;
    while (strlen(str)) {

        modeline = parse_modeline(str);
        if (!modeline) {
            *err_str = g_strdup_printf("Failed to parse the following "
                                       "modeline of display device\n"
                                       "0x%08x '%s' connected to GPU-%d "
                                       "'%s':\n\n%s",
                                       display->device_mask,
                                       display->name,
                                       NvCtrlGetTargetId(display->gpu->handle),
                                       display->gpu->name,
                                       str);
            nv_error_msg(*err_str);
            goto fail;
        }
        
        /* Add the modeline at the end of the display's modeline list */
        display->modelines = (nvModeLinePtr)xconfigAddListItem
            ((GenericListPtr)display->modelines, (GenericListPtr)modeline);
        display->num_modelines++;

        /* Get next modeline string */
        str += strlen(str) +1;
    }

    XFree(modeline_strs);
    return TRUE;


    /* Handle the failure case */
 fail:
    remove_modelines_from_display(display);
    XFree(modeline_strs);
    return FALSE;

} /* add_modelines_to_display() */



/** free_display() ***************************************************
 *
 * Frees memory used by a display
 *
 */
static void free_display(nvDisplayPtr display)
{
    if (display) {
        remove_modes_from_display(display);
        remove_modelines_from_display(display);
        XFree(display->name);
        free(display);
    }

} /* free_display(display) */



/** add_display_to_gpu() *********************************************
 *
 *  Adds the display with the device mask given to the GPU structure.
 *
 */
static nvDisplayPtr add_display_to_gpu(nvGpuPtr gpu, unsigned int device_mask,
                                       gchar **err_str)
{
    ReturnStatus ret;
    nvDisplayPtr display = NULL;

    
    /* Create the display structure */
    display = (nvDisplayPtr)calloc(1, sizeof(nvDisplay));
    if (!display) goto fail;


    /* Init the display structure */
    display->gpu = gpu;
    display->device_mask = device_mask;

    
    /* Query the display information */
    ret = NvCtrlGetStringDisplayAttribute(gpu->handle,
                                          device_mask,
                                          NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                          &(display->name));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query name of display device\n"
                                   "0x%08x connected to GPU-%d '%s'.",
                                   device_mask, NvCtrlGetTargetId(gpu->handle),
                                   gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Query the modelines for the display device */
    if (!add_modelines_to_display(display, err_str)) {
        nv_warning_msg("Failed to add modelines to display device 0x%08x "
                       "'%s'\nconnected to GPU-%d '%s'.",
                       device_mask, display->name,
                       NvCtrlGetTargetId(gpu->handle), gpu->name);
        goto fail;
    }


    /* Add the display at the end of gpu's display list */
    gpu->displays =
        (nvDisplayPtr)xconfigAddListItem((GenericListPtr)gpu->displays,
                                         (GenericListPtr)display);
    gpu->connected_displays |= device_mask;
    gpu->num_displays++;
    return display;


    /* Failure case */
 fail:
    free_display(display);
    return NULL;

} /* add_display_to_gpu() */



/** remove_display_from_gpu() ****************************************
 *
 * Removes a display from the gpu
 *
 */
static void remove_display_from_gpu(nvDisplayPtr display)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    

    if (display && display->gpu) {
        gpu = display->gpu;
        screen = display->screen;

        /* Remove the display from the screen it may be in */
        if (screen) {
            remove_display_from_screen(display);

            /* If the screen is empty, remove it too */
            if (!screen->num_displays) {
                remove_screen_from_gpu(screen);
                free_screen(screen);
            }
        }

        /* Remove the display from the gpu */
        gpu->displays =
            (nvDisplayPtr)xconfigRemoveListItem((GenericListPtr)gpu->displays,
                                                (GenericListPtr)display);
        gpu->connected_displays &= ~(display->device_mask);
        gpu->num_displays--;
    }

} /* remove_display_from_gpu() */



/** remove_displays_from_gpu() ***************************************
 *
 * Removes all displays from the gpu
 *
 */
static void remove_displays_from_gpu(nvGpuPtr gpu)
{
    nvDisplayPtr display;

    if (gpu) {
        while (gpu->displays) {
            display = gpu->displays;
            remove_display_from_screen(display);
            gpu->displays = display->next;
            free_display(display);
        }
        gpu->num_displays = 0;
    }

} /* remove_displays_from_gpu() */



/** add_displays_to_gpu() ********************************************
 *
 * Adds the display devices connected on the GPU to the GPU structure
 *
 */
static Bool add_displays_to_gpu(nvGpuPtr gpu, gchar **err_str)
{
    unsigned int mask;


    /* Clean up the GPU list */
    remove_displays_from_gpu(gpu);


    /* Add each connected display */
    for (mask = 1; mask; mask <<= 1) {

        if (!(mask & (gpu->connected_displays))) continue;
        
        if (!add_display_to_gpu(gpu, mask, err_str)) {
            nv_warning_msg("Failed to add display device 0x%08x to GPU-%d "
                           "'%s'.",
                           mask, NvCtrlGetTargetId(gpu->handle), gpu->name);
            goto fail;
        }
    }

    return TRUE;


    /* Failure case */
 fail:
    remove_displays_from_gpu(gpu);
    return FALSE;

} /* add_displays_to_gpu() */



/* GPU functions *****************************************************/


/** add_screenless_modes_to_displays() *******************************
 *
 * Adds fake modes to display devices that have no screens so we
 * can show them on the layout page.
 *
 */
static Bool add_screenless_modes_to_displays(nvGpuPtr gpu)
{
    nvDisplayPtr display;
    nvModePtr mode;

    for (display = gpu->displays; display; display = display->next) {
        if (display->screen) continue;

        /* Create a fake mode */
        mode = (nvModePtr)calloc(1, sizeof(nvMode));
        if (!mode) return FALSE;

        mode->display = display;
        mode->dummy = 1;

        mode->dim[W] = 800;
        mode->dim[H] = 600;
        mode->pan[W] = mode->dim[W];
        mode->pan[H] = mode->dim[H];

        /* Add the mode to the display */
        display->modes = mode;
        display->cur_mode = mode;
        display->num_modes = 1;
    }

    return TRUE;

} /* add_screenless_modes_to_displays() */



/** free_gpu() *******************************************************
 *
 * Frees memory used by the gpu.
 *
 **/
static void free_gpu(nvGpuPtr gpu)
{
    if (gpu) {
        remove_screens_from_gpu(gpu);
        remove_displays_from_gpu(gpu);
        XFree(gpu->name);
        if (gpu->handle) {
            NvCtrlAttributeClose(gpu->handle);
        }
        free(gpu);
    }

} /* free_gpu() */



/** add_gpu_to_layout() **********************************************
 *
 * Adds a GPU to the layout structure.
 *
 **/
static Bool add_gpu_to_layout(nvLayoutPtr layout, unsigned int gpu_id,
                              gchar **err_str)
{
    ReturnStatus ret;
    Display *dpy;
    nvGpuPtr gpu = NULL;

    
    /* Create the GPU structure */
    gpu = (nvGpuPtr)calloc(1, sizeof(nvGpu));
    if (!gpu) goto fail;

    
    /* Make an NV-CONTROL handle to talk to the GPU */
    dpy = NvCtrlGetDisplayPtr(layout->handle);
    gpu->layout = layout;
    gpu->handle = NvCtrlAttributeInit(dpy, NV_CTRL_TARGET_TYPE_GPU, gpu_id,
                                      NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
    if (!gpu->handle) {
        *err_str = g_strdup_printf("Failed to create NV-CONTROL handle for "
                                   "GPU-%d.", gpu_id);
        nv_error_msg(*err_str);
        goto fail;
    }

    gpu->ctk_event = CTK_EVENT(ctk_event_new(gpu->handle));


    /* Query the GPU information */
    ret = NvCtrlGetStringAttribute(gpu->handle, NV_CTRL_STRING_PRODUCT_NAME,
                                   &gpu->name);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query GPU name of GPU-%d.",
                                   gpu_id);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_CONNECTED_DISPLAYS,
                             (int *)&(gpu->connected_displays));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query connected display "
                                   "devices on GPU-%d '%s'.",
                                   gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_PCI_BUS,
                             (int *)&(gpu->pci_bus));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query PCI BUS on GPU-%d '%s'.",
                                   gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_PCI_DEVICE,
                             (int *)&(gpu->pci_device));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query PCI DEVICE on "
                                   "GPU-%d '%s'.", gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_PCI_FUNCTION,
                             (int *)&(gpu->pci_func));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query PCI FUNCTION on "
                                   "GPU-%d '%s'.", gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_MAX_SCREEN_WIDTH,
                             (int *)&(gpu->max_width));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX SCREEN WIDTH on "
                                   "GPU-%d '%s'.", gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_MAX_SCREEN_HEIGHT,
                             (int *)&(gpu->max_height));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX SCREEN HEIGHT on "
                                   "GPU-%d '%s'.", gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_MAX_DISPLAYS,
                             (int *)&(gpu->max_displays));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX DISPLAYS on "
                                   "GPU-%d '%s'.", gpu_id, gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Add the display devices to the GPU */
    if (!add_displays_to_gpu(gpu, err_str)) {
        nv_warning_msg("Failed to add displays to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }


    /* Add the X Screens to the GPU */
    if (!add_screens_to_gpu(gpu, err_str)) {
        nv_warning_msg("Failed to add screens to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }
    

    /* Add fake modes to screenless display devices */
    if (!add_screenless_modes_to_displays(gpu)) {
        nv_warning_msg("Failed to add screenless modes to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }


    /* Add the GPU at the end of the layout's GPU list */
    layout->gpus = (nvGpuPtr)xconfigAddListItem((GenericListPtr)layout->gpus,
                                                (GenericListPtr)gpu);
    layout->num_gpus++;
    return TRUE;


    /* Failure case */
 fail:
    free_gpu(gpu);
    return FALSE;

} /* add_gpu_to_layout() */



/** remove_gpus_from_layout() ****************************************
 *
 * Removes all GPUs from the layout structure.
 *
 **/
static void remove_gpus_from_layout(nvLayoutPtr layout)
{
    nvGpuPtr gpu;

    if (layout) {
        while (layout->gpus) {
            gpu = layout->gpus;
            layout->gpus = gpu->next;
            free_gpu(gpu);
        }
        layout->num_gpus = 0;
    }

} /* remove_gpus_from_layout() */



/** add_gpus_to_layout() *********************************************
 *
 * Adds the GPUs found on the server to the layout structure.
 *
 **/

static int add_gpus_to_layout(nvLayoutPtr layout, gchar **err_str)
{
    ReturnStatus ret;
    int ngpus;
    int i;


    /* Clean up the GPU list */
    remove_gpus_from_layout(layout);


    /* Query the number of GPUs on the server */
    ret = NvCtrlQueryTargetCount(layout->handle, NV_CTRL_TARGET_TYPE_GPU,
                                 &ngpus);
    if (ret != NvCtrlSuccess || !ngpus) {
        *err_str = g_strdup("Failed to query number of GPUs (or no GPUs "
                            "found) in the system.");
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Add each GPU */
    for (i = 0; i < ngpus; i++) {
        if (!add_gpu_to_layout(layout, i, err_str)) {
            nv_warning_msg("Failed to add GPU-%d to layout.", i);
            goto fail;
        }
    }

    return layout->num_gpus;


    /* Failure case */
 fail:
    remove_gpus_from_layout(layout);
    return 0;

} /* add_gpus_to_layout() */



/* Layout functions **************************************************/


/** assign_screen_positions() ****************************************
 *
 * Assign the initial position of the X Screens.
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

                parse_tokens(screen_info, apply_screen_info_token,
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



/** load_server_layout() *********************************************
 *
 * Loads layout information from the X server.
 *
 **/

nvLayoutPtr load_server_layout(NvCtrlAttributeHandle *handle, gchar **err_str)
{
    nvLayoutPtr layout;
    ReturnStatus ret;


    /* Allocate the layout structure */
    layout = (nvLayoutPtr)calloc(1, sizeof(nvLayout));
    if (!layout) goto fail;


    /* Cache the handle for talking to the X Server */
    layout->handle = handle;
    

    /* Is Xinerma enabled? */
    ret = NvCtrlGetAttribute(handle, NV_CTRL_XINERAMA,
                             &layout->xinerama_enabled);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup("Failed to query status of Xinerama.");
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Add GPUs to the layout */
    if (!add_gpus_to_layout(layout, err_str)) {
        nv_warning_msg("Failed to add GPU(s) to layout for display "
                       "configuration page.");
        goto fail;
    }

    return layout;


    /* Failure case */
 fail:
    if (layout) {
        remove_gpus_from_layout(layout);
        free(layout);
    }
    return NULL;

} /* load_server_layout() */



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
                          "%s The location an X Screens has changed.\n"
                          "%s The location type of an X Screens has changed.\n"
                          "%s The color depth of an X Screen has changed.\n"
                          "%s An X Screen has been added or removed.\n"
                          "%s Xinerama is being enabled/disabled.\n"
                          "\n"
                          "For all the requested settings to take effect,\n"
                          "you must save the configuration to the X Config\n"
                          "file and restart the X Server.",
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
    GtkWidget *hbox;
    GtkWidget *hbox2;
    GtkWidget *vbox;
    GtkWidget *label;

    GtkRequisition req;

    GtkWidget *vpanel;
    GtkWidget *scrolled_window;
    GtkWidget *viewport;

    GtkWidget *menu;
    GtkWidget *menu_item;

    gchar *err_str = NULL;


    /*
     * Create the ctk object
     *
     */

    object = g_object_new(CTK_TYPE_DISPLAY_CONFIG, NULL);
    ctk_object = CTK_DISPLAY_CONFIG(object);

    ctk_object->handle = handle;
    ctk_object->ctk_config = ctk_config;

    ctk_object->apply_possible = TRUE;
    ctk_object->advanced_mode = FALSE;

    /* Set container properties of the object & pack the banner */
    gtk_box_set_spacing(GTK_BOX(ctk_object), 5);

    banner = ctk_banner_image_new(&blank_banner_image);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);



    /*
     * Create the display configuration widgets
     *
     */
    
    /* Load the layout structure from the X server */
    ctk_object->layout = load_server_layout(handle, &err_str);

    /* If we failed to load, tell the user why */
    if (err_str || !ctk_object->layout) {
        gchar *str;

        if (!err_str) {
            str = g_strdup("Failed to load X Server Display Configuration.");
        } else {
            str = g_strdup_printf("Error while loading X Server Display "
                                  "Configuration:\n\n%s", err_str);
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

    /* Create the layout widget */
    ctk_object->obj_layout = ctk_display_layout_new(handle, ctk_config,
                                                    ctk_object->layout,
                                                    300, /* min width */
                                                    225, /* min height */
                                                    layout_selected_callback,
                                                    (void *)ctk_object,
                                                    layout_modified_callback,
                                                    (void *)ctk_object);

    /* Make sure we have some kind of positioning */
    assign_screen_positions(ctk_object);

    /* Grab the current screen position for "apply possible" tracking */
    get_cur_screen_pos(ctk_object);
    

    /*
     * Create the widgets
     *
     */

    /* Xinerama button */

    ctk_object->chk_xinerama_enabled =
        gtk_check_button_new_with_label("Enable Xinerama");
    ctk_config_set_tooltip(ctk_config, ctk_object->chk_xinerama_enabled,
                           __layout_xinerama_button_help);
    g_signal_connect(G_OBJECT(ctk_object->chk_xinerama_enabled), "toggled",
                     G_CALLBACK(xinerama_state_toggled),
                     (gpointer) ctk_object);


    /* Display model name */
    ctk_object->txt_display_model = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ctk_object->txt_display_model), TRUE);
    gtk_misc_set_alignment(GTK_MISC(ctk_object->txt_display_model), 0.0f, 0.5f);
    
    /* Display configuration (Disabled, TwinView, Separate X Screen */
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
         "Separate X Screen");
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


    /* X Screen number */
    ctk_object->txt_screen_num = gtk_label_new("");
    gtk_label_set_selectable(GTK_LABEL(ctk_object->txt_screen_num), TRUE);
    gtk_misc_set_alignment(GTK_MISC(ctk_object->txt_screen_num), 0.0f, 0.5f);

    /* X Screen depth */
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

    /* X Screen metamode */
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
    ctk_object->dlg_xconfig_save = gtk_dialog_new_with_buttons
        ("Save X Configuration",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_object))),
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT |
         GTK_DIALOG_NO_SEPARATOR,
         GTK_STOCK_SAVE,
         GTK_RESPONSE_ACCEPT,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(ctk_object->dlg_xconfig_save),
                                    GTK_RESPONSE_REJECT);

    ctk_object->btn_xconfig_preview = gtk_button_new();
    g_signal_connect(G_OBJECT(ctk_object->btn_xconfig_preview), "clicked",
                     G_CALLBACK(xconfig_preview_clicked),
                     (gpointer) ctk_object);

    ctk_object->txt_xconfig_save = gtk_text_view_new();
    gtk_text_view_set_left_margin
        (GTK_TEXT_VIEW(ctk_object->txt_xconfig_save), 5);

    ctk_object->buf_xconfig_save = gtk_text_buffer_new(NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(ctk_object->txt_xconfig_save),
                             GTK_TEXT_BUFFER(ctk_object->buf_xconfig_save));

    ctk_object->scr_xconfig_save = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type
        (GTK_SCROLLED_WINDOW(ctk_object->scr_xconfig_save), GTK_SHADOW_IN);

    ctk_object->txt_xconfig_file = gtk_entry_new();
    gtk_widget_set_size_request(ctk_object->txt_xconfig_file, 300, -1);

    ctk_object->btn_xconfig_file = gtk_button_new_with_label("Browse...");
    g_signal_connect(G_OBJECT(ctk_object->btn_xconfig_file), "clicked",
                     G_CALLBACK(xconfig_file_clicked),
                     (gpointer) ctk_object);
    ctk_object->dlg_xconfig_file = gtk_file_selection_new
        ("Please select the X configuration file");
    


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
                           __probe_button_help);
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

        frame = gtk_frame_new("Layout");
        hbox = gtk_hbox_new(FALSE, 5); /* main panel */
        vbox = gtk_vbox_new(FALSE, 5); /* layout panel */
        gtk_box_pack_start(GTK_BOX(ctk_object), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 5);

        hbox = gtk_hbox_new(FALSE, 5); /* layout panel */
        gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_add(GTK_CONTAINER(frame), hbox);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

        /* Pack the layout widget */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->obj_layout,
                           FALSE, FALSE, 0);

        /* Xinerama checkbox */
        gtk_box_pack_start(GTK_BOX(vbox), ctk_object->chk_xinerama_enabled,
                           FALSE, FALSE, 0);
    }
    

    /* Scrolled window */
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(ctk_object), scrolled_window, TRUE, TRUE, 0);
    gtk_widget_set_size_request(scrolled_window, -1, 200);

    viewport = gtk_viewport_new(NULL, NULL);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), viewport);


    /* Panel for the display/screen sections */
    vpanel = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(viewport), vpanel);


    { /* Display section */
        GtkWidget *longest_hbox;

        /* Create the display frame */
        frame = gtk_frame_new("Display");
        ctk_object->display_frame = frame;
        hbox  = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vpanel), hbox, FALSE, FALSE, 0);
        
        /* Generate the major vbox for the display section */
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

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
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Model:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_display_model,
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

    } /* Display sub-section */
    

    { /* X Screen */

        /* Create the X screen frame */
        frame = gtk_frame_new("X Screen");
        ctk_object->screen_frame = frame;
        hbox  = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vpanel), hbox, FALSE, FALSE, 5);

        /* Generate the major vbox for the display section */
        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* X Screen number */      
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Screen Number:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_size_request(label, &req);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->txt_screen_num,
                           TRUE, TRUE, 0);

        /* X Screen depth dropdown */
        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        label = gtk_label_new("Color Depth:");
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_widget_set_size_request(label, req.width, -1);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 5); 
        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->mnu_screen_depth,
                           TRUE, TRUE, 0);
        ctk_object->box_screen_depth = hbox;

        /* X Screen positioning */
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

        /* X Screen metamode drop down & buttons */
        hbox = gtk_hbox_new(FALSE, 5);
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

    } /* X Screen sub-section */
    
 
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

        /* X Config Save Dialog */
        gtk_dialog_set_has_separator(GTK_DIALOG(ctk_object->dlg_xconfig_save),
                                     TRUE);

        /* Preview button */
        hbox = gtk_hbox_new(FALSE, 0);
        hbox2 = gtk_hbox_new(FALSE, 0);

        gtk_box_pack_start(GTK_BOX(hbox), ctk_object->btn_xconfig_preview,
                           FALSE, FALSE, 5);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_xconfig_save)->vbox),
             hbox, FALSE, FALSE, 5);

        /* Preview window */
        hbox = gtk_hbox_new(TRUE, 0);

        gtk_container_add(GTK_CONTAINER(ctk_object->scr_xconfig_save),
                          ctk_object->txt_xconfig_save);
        gtk_box_pack_start(GTK_BOX(hbox),
                           ctk_object->scr_xconfig_save,
                           TRUE, TRUE, 5);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_xconfig_save)->vbox),
             hbox,
             TRUE, TRUE, 0);
        ctk_object->box_xconfig_save = hbox;

        /* Filename */
        hbox = gtk_hbox_new(FALSE, 0);
        hbox2 = gtk_hbox_new(FALSE, 5);

        gtk_box_pack_end(GTK_BOX(hbox2), ctk_object->btn_xconfig_file,
                           FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(hbox2), ctk_object->txt_xconfig_file,
                           TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(hbox), hbox2,
                           TRUE, TRUE, 5);
        gtk_box_pack_start
            (GTK_BOX(GTK_DIALOG(ctk_object->dlg_xconfig_save)->vbox),
             hbox,
             FALSE, FALSE, 5);

        gtk_widget_show_all(GTK_DIALOG(ctk_object->dlg_xconfig_save)->vbox);
    }


    /* Show the GUI */
    gtk_widget_show_all(GTK_WIDGET(ctk_object));


    /* Setup the GUI */
    setup_layout_frame(ctk_object);
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);


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
                  "the X Server's display devices.");

    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Layout Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the X server layout.");
    ctk_help_heading(b, &i, "Layout Image");
    ctk_help_para(b, &i, "The layout image shows the geomertic relationship "
                  "that display devices and X screens have to each other.  "
                  "You may drag display devices around to reposition them.  "
                  "When in advanced view, the display's panning domain may "
                  "be resized by holding shift while dragging.");
    ctk_help_heading(b, &i, "Enable Xinerama");
    ctk_help_para(b, &i, "%s.  This setting is only available when multiple "
                  "X screens are present.", __layout_xinerama_button_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Display Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the currently selected display device.");
    ctk_help_heading(b, &i, "Model");
    ctk_help_para(b, &i, "The Model name is the name of the display device.");
    ctk_help_heading(b, &i, "Resolution");
    ctk_help_para(b, &i, __dpy_resolution_mnu_help);
    ctk_help_heading(b, &i, "Refresh");
    ctk_help_para(b, &i, "The Refresh drop-down is to the right of the "
                  "Resolution drop-down.  %s", __dpy_refresh_mnu_help);
    ctk_help_heading(b, &i, "Mode Name");
    ctk_help_para(b, &i, "The Mode name is the name of the modeline that is "
                  "currently chosen for the selected display device.  "
                  "(Advanced view only)");
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, __dpy_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, __dpy_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, __dpy_position_offset_help);
    ctk_help_heading(b, &i, "Panning");
    ctk_help_para(b, &i, "%s.  (Advanced view only)", __dpy_panning_help);


    ctk_help_para(b, &i, "");
    ctk_help_heading(b, &i, "Screen Section");
    ctk_help_para(b, &i, "This section shows information and configuration "
                  "settings for the currently selected X Screen.");
    ctk_help_heading(b, &i, "Screen Depth");
    ctk_help_para(b, &i, __screen_depth_help);
    ctk_help_heading(b, &i, "Position Type");
    ctk_help_para(b, &i, __screen_position_type_help);
    ctk_help_heading(b, &i, "Position Relative");
    ctk_help_para(b, &i, __screen_position_relative_help);
    ctk_help_heading(b, &i, "Position Offset");
    ctk_help_para(b, &i, __screen_position_offset_help);
    ctk_help_heading(b, &i, "MetaMode Selection");
    ctk_help_para(b, &i, "%s.  (Advanced view only)", __screen_metamode_help);
    ctk_help_heading(b, &i, "Add Metamode");
    ctk_help_para(b, &i, "%s.  (Advanced view only)",
                  __screen_metamode_add_button_help);
    ctk_help_heading(b, &i, "Delete Metamode");
    ctk_help_para(b, &i, "%s.  (Advanced view only)",
                  __screen_metamode_delete_button_help);
    
    
    ctk_help_para(b, &i, "");
     ctk_help_heading(b, &i, "Buttons");
    ctk_help_heading(b, &i, "Probe");
    ctk_help_para(b, &i, __apply_button_help);
    ctk_help_heading(b, &i, "Detect Displays");
    ctk_help_para(b, &i, __probe_button_help);
    ctk_help_heading(b, &i, "Advanced/Basic...");
    ctk_help_para(b, &i, "%s.  The Basic view modifies the currently active "
                  "MetaMode for an X screen, while the Advanced view exposes "
                  "all the MetaModes available on an X screen, and lets you "
                  "modify each of them", __advanced_button_help);
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


    /* Only allow Xinerama when there are multiple X Screens */
    num_screens = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        num_screens += gpu->num_screens;
    }

    /* Unselect Xinerama if only one (or no) X Screen */
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
                           "Separate X Screen");
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_config),
                           "TwinView");
    }
    
} /* setup_display_config() */



/** setup_display_refresh_dropdwon() *********************************
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
    cur_rate     = GET_MODELINE_REFRESH_RATE(cur_modeline);


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

        float modeline_rate;
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

        modeline_rate = GET_MODELINE_REFRESH_RATE(modeline);
        is_doublescan = (modeline->data.flags & V_DBLSCAN);
        is_interlaced = (modeline->data.flags & V_INTERLACE);

        name = g_strdup_printf("%.0f Hz", modeline_rate);


        /* Get a unique number for this modeline */
        count_ref = 0; /* # modelines with similar refresh rates */
        num_ref = 0;   /* Modeline # in a group of similar refresh rates */
        for (m = modelines; m; m = m->next) {
            float m_rate = GET_MODELINE_REFRESH_RATE(m);
            gchar *tmp = g_strdup_printf("%.0f Hz", m_rate);
            
            if (!IS_NVIDIA_DEFAULT_MODE(m) &&
                m->data.hdisplay == modeline->data.hdisplay &&
                m->data.vdisplay == modeline->data.vdisplay &&
                !g_ascii_strcasecmp(tmp, name) &&
                m != auto_modeline) {

                count_ref++;
                /* Modelines with similar refresh rates get a unique # (num_ref) */
                if (m == modeline) {
                    num_ref = count_ref; /* This modeline's # */
                }
            }
            g_free(tmp);
        }

        /* Is default refresh rate for resolution */
        if (!ctk_object->refresh_table_len) {
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
                
                float prev_rate = GET_MODELINE_REFRESH_RATE(ctk_object->refresh_table[cur_idx]);
                float rate      = GET_MODELINE_REFRESH_RATE(modeline); 
                
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



/** setup_display_resolution_dropdwon() ******************************
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

    if (!display || !display->screen || !display->cur_mode || !display->gpu) {
        return;
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
        ctk_object->display_position_table_len = 0;
        gtk_widget_hide(ctk_object->mnu_display_position_relative);
        return;
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


    /* Handle cases where the position relative dropdown should be hidden */
    if (display->cur_mode->position_type == CONF_ADJ_ABSOLUTE) {
        gtk_widget_hide(ctk_object->mnu_display_position_relative);
        return;
    }
    gtk_widget_show(ctk_object->mnu_display_position_relative);

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
    tmp_str = g_strdup_printf("%dx%d", mode->pan[W], mode->pan[H]);

    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_display_panning),
                       tmp_str);
    g_free(tmp_str);

} /* setup_display_panning */



/** setup_display_frame() ********************************************
 *
 * Sets up the display frame to reflect the currently selected
 * display. 
 *
 **/

static void setup_display_frame(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gchar *type;
    gchar *str;



    /* If no display is selected or there is no screen, hide the frame */
    if (!display) {
        gtk_widget_set_sensitive(ctk_object->display_frame, False);
        gtk_widget_hide(ctk_object->display_frame);
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->display_frame, True);
    

    /* Setup the display name */
    type = get_display_type_str(display->device_mask, 0);
    str = g_strdup_printf("%s (%s)", display->name, type);
    g_free(type);
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_display_model), str);
    g_free(str);


    /* Setp the seleted mode modename */
    setup_display_config(ctk_object);


    /* Setp the seleted mode modename */
    setup_display_modename(ctk_object);


    /* Setup display resolution menu */
    setup_display_resolution_dropdown(ctk_object);
    

    /* Setup position */
    setup_display_position(ctk_object);


    /* Setup panning */
    setup_display_panning(ctk_object);


    gtk_widget_show(ctk_object->display_frame);

} /* setup_display_frame() */



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
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    
    if (!display || !display->screen) {
        gtk_widget_hide(ctk_object->box_screen_depth);
        return;
    }


    menu      = gtk_menu_new();
    menu_item = gtk_menu_item_new_with_label("Millions of Colors (32 bpp)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    menu_item = gtk_menu_item_new_with_label("Thousands of Colors (16 bpp)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);
    menu_item = gtk_menu_item_new_with_label("256 Colors (8 bpp)");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    gtk_widget_show(menu_item);

    g_signal_handlers_block_by_func(G_OBJECT(ctk_object->mnu_screen_depth),
                                    G_CALLBACK(screen_depth_changed),
                                    (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), menu);

    if (display->screen->depth == 24) {
        gtk_option_menu_set_history
            (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), 0);
    } else if (display->screen->depth == 16) {
        gtk_option_menu_set_history
            (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), 1);
    } else {
        gtk_option_menu_set_history
            (GTK_OPTION_MENU(ctk_object->mnu_screen_depth), 2);
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
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    
    /* Handle cases where the position type should be hidden */
    if (!display || !display->screen) {
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
         display->screen->position_type);

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
    nvDisplayPtr display;
    nvScreenPtr screen;
    nvScreenPtr relative_to;
    nvGpuPtr gpu;
    int idx;
    int selected_idx;
    GtkWidget *menu;
    GtkWidget *menu_item;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

    if (!display || !display->cur_mode || !display->gpu || !display->screen) {
        return;
    }

    screen = display->screen;


    /* Allocate the screen lookup table for the dropdown */
    if (ctk_object->screen_position_table) {
        free(ctk_object->screen_position_table);
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

    ctk_object->screen_position_table =
        (nvScreenPtr *)calloc(1, ctk_object->screen_position_table_len *
                              sizeof(nvScreenPtr));
    
    if (!ctk_object->screen_position_table) {
        ctk_object->screen_position_table_len = 0;
        gtk_widget_hide(ctk_object->mnu_screen_position_relative);
        return;
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

            tmp_str = g_strdup_printf("X Screen %d",
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


    /* Handle cases where the position relative dropdown should be hidden */
    if (display->screen->position_type == CONF_ADJ_ABSOLUTE) {
        gtk_widget_hide(ctk_object->mnu_screen_position_relative);
        return;
    }
    gtk_widget_show(ctk_object->mnu_screen_position_relative);

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
    nvDisplayPtr display;
    nvScreenPtr screen;

    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Handle cases where the position offset should be hidden */
    if (!display || !display->screen ||
        (display->screen->position_type != CONF_ADJ_ABSOLUTE && 
         display->screen->position_type != CONF_ADJ_RELATIVE)) {
        gtk_widget_hide(ctk_object->txt_screen_position_offset);
        return;
    }
    gtk_widget_show(ctk_object->txt_screen_position_offset);


    /* Update the position text */
    screen = display->screen;

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
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    int num_screens;


    /* Count the number of screens */
    num_screens = 0;
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        num_screens += gpu->num_screens;
    }


    /* Hide the position box if there is only one screen */
    if (!display || !display->screen || num_screens <= 1) {
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


    if (!screen || !ctk_object->advanced_mode) {
        gtk_widget_hide(ctk_object->box_screen_metamode);
        return;
    }


    /* Update the meatamode selector button */
    str = g_strdup_printf("%d - ...", screen->cur_metamode_idx +1);
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_screen_metamode), str);
    g_free(str);

    /* Only allow deletes if there are more than 1 metamodes */
    gtk_widget_set_sensitive(ctk_object->btn_screen_metamode_delete,
                             ((screen->num_metamodes > 1) ? True : False));
    
    gtk_widget_show(ctk_object->box_screen_metamode);

} /* setup_screen_metamode() */



/** setup_screen_frame() *********************************************
 *
 * Sets up the screen frame to reflect the currently selected screen.
 *
 **/

static void setup_screen_frame(CtkDisplayConfig *ctk_object)
{
    nvDisplayPtr display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    gchar *tmp;


    /* If there is no display or no screen selected, hide the frame */
    if (!display || !display->screen) {
        gtk_widget_set_sensitive(ctk_object->screen_frame, False);
        gtk_widget_hide(ctk_object->screen_frame);
        return;
    }


    /* Enable display widgets and setup widget information */
    gtk_widget_set_sensitive(ctk_object->screen_frame, True);


    /* Setup the screen number */
    tmp = g_strdup_printf("%d", display->screen->scrnum);
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_screen_num), tmp);
    g_free(tmp);
    

    /* Setup depth menu */
    setup_screen_depth_dropdown(ctk_object);


    /* Setup position */
    setup_screen_position(ctk_object);


    /* Setup metamode menu */
    setup_screen_metamode(ctk_object);


    gtk_widget_show(ctk_object->screen_frame);

} /* setup_screen_frame() */



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

        metamode_str = get_screen_metamode_str(screen, i, 0);
        for (j = 0; j < i; j++) {
            char *tmp;
            tmp = get_screen_metamode_str(screen, j, 0);
            
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
        metamode_str = get_screen_metamode_str(screen, i, 0);
        for (j = 0; j < i; j++) {
            char *tmp = get_screen_metamode_str(screen, j, 0);

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

static int validate_layout(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    gchar *err_strs = NULL;
    gchar *err_str;
    gchar *tmp;
    gint result;


    /* Validate each screen */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            err_str = validate_screen(screen);
            if (err_str) {
                tmp = g_strconcat((err_strs ? err_strs : ""), err_str, NULL);
                g_free(err_strs);
                g_free(err_str);
                err_strs = tmp;
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
        setup_display_frame(ctk_object);
        setup_screen_frame(ctk_object);
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
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);

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

    /* If the positioning of the X Screen changes, we cannot apply */
    check_screen_pos_changed(ctk_object);

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

} /* layout_modified_callback()  */



/* Widget signal handlers ********************************************/


/** do_enable_display_for_xscreen() **********************************
 *
 * Adds the display device to a separate X Screen in the layout.
 *
 **/

void do_enable_display_for_xscreen(CtkDisplayConfig *ctk_object,
                                   nvDisplayPtr display)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvMetaModePtr metamode;
    nvModePtr mode;
    int scrnum;


    /* Get the next available screen number */
    scrnum = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            scrnum++;
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
    screen->depth = 24;
    screen->displays_mask = display->device_mask;
    screen->num_displays = 1;
    screen->metamodes = metamode;
    screen->num_metamodes = 1;
    screen->cur_metamode = metamode;
    screen->cur_metamode_idx = 0;

    /* XXX Should we position it "RightOf" the right-most screen by default? */
    screen->dim[X] = mode->dim[X];
    screen->dim[Y] = mode->dim[Y];
    screen->position_type = CONF_ADJ_ABSOLUTE;


    /* Add the screen at the end of the gpu's screen list */
    gpu = display->gpu;
    gpu->screens =
        (nvScreenPtr)xconfigAddListItem((GenericListPtr)gpu->screens,
                                        (GenericListPtr)screen);
    gpu->num_screens++;


    /* We can't dynamically add new X Screens */
    ctk_object->apply_possible = FALSE;

} /* do_enable_display_for_xscreen() */



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


    /* Make sure a screen exists */
    gpu = display->gpu;
    screen = gpu->screens;

    if (!screen) return;


    /* Associate the display device with the screen */
    new_mask = screen->displays_mask | display->device_mask;
    if (screen->handle) {
        ReturnStatus ret;
        ret = NvCtrlSetAttribute(screen->handle,
                                 NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
                                 new_mask);
        if (ret != NvCtrlSuccess) {
            nv_error_msg("Failed to associate display device '%s' with "
                         "X Screen %d!", display->name, screen->scrnum);
        } else {
            /* Make sure other parts of nvidia-settings get updated */
            ctk_event_emit(screen->ctk_event, 0,
                           NV_CTRL_ASSOCIATED_DISPLAY_DEVICES, new_mask);
        }
    }


    /* Fix up the display's metamode list */
    remove_modes_from_display(display);
    
    for (metamode = screen->metamodes; metamode; metamode = metamode->next) {

        /* Create the nvidia-auto-select mode fo the display */
        mode = parse_mode(display, "nvidia-auto-select");
        mode->metamode = metamode;

        /* Set the currently selected mode */
        if (metamode == screen->cur_metamode) {
            display->cur_mode = mode;
        }

        /* Position the new mode at the top right of the metamode */
        mode->position_type = CONF_ADJ_ABSOLUTE;
        mode->dim[X] = metamode->dim[X] + metamode->dim[W];
        mode->dim[Y] = metamode->dim[Y];
        mode->pan[X] = mode->dim[X];
        mode->pan[Y] = mode->dim[Y];

        /* Add the mode at the end of the display's mode list */
        display->modes =
            (nvModePtr)xconfigAddListItem((GenericListPtr)display->modes,
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
 * Configures the display's GPU for Multiple X Screens
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


    /* Make sure there is just one display device per X Screen */
    for (display = gpu->displays; display; display = display->next) {

        nvScreenPtr new_screen;

        screen = display->screen;
        if (!screen || screen->num_displays == 1) continue;
        
        /* Create a new X Screen for this display */
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
            new_screen->metamodes =
                (nvMetaModePtr)xconfigAddListItem((GenericListPtr)new_screen->metamodes,
                                                  (GenericListPtr)metamode);
            new_screen->num_metamodes++;
        }

        /* Move the display to the new screen */
        screen->num_displays--;
        screen->displays_mask &= ~(display->device_mask);

        display->screen = new_screen;

        /* Append the screen to the gpu */
        gpu->screens =
            (nvScreenPtr)xconfigAddListItem((GenericListPtr)gpu->screens,
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
            screen->metamodes =
                (nvMetaModePtr)xconfigAddListItem((GenericListPtr)screen->metamodes,
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

            mode = parse_mode(display, "NULL");
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
            display->modes =
                (nvModePtr)xconfigAddListItem((GenericListPtr)display->modes,
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
        } else {
            if (other->handle) {
                NvCtrlAttributeClose(other->handle);
            }
        }

        free(other);
        gpu->num_screens--;
    }

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
    nvScreenPtr screen = display->screen;
    gchar *str;
    gchar *type = get_display_type_str(display->device_mask, 0);


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

     
    /* Ask user what to do */
    if (do_query_remove_display(ctk_object, display)) {

        /* Remove display from the X Screen */
        remove_display_from_screen(display);
        
        /* If the screen is empty, remove it */
        if (!screen->num_displays) {
            remove_screen_from_gpu(screen);
            free_screen(screen);
        }

        /* Add the fake mode to the display */
        add_screenless_modes_to_displays(display->gpu);
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


    /* We can only enable as many X Screens as the GPU supports */
    if (!display->screen &&
        (display->gpu->num_screens >= display->gpu->max_displays)) {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_xscreen,
                                 FALSE);
    } else {
        gtk_widget_set_sensitive(ctk_object->rad_display_config_xscreen,
                                 TRUE);
    }


    /* We can't setup TwinView if there is only one display connected,
     * there are no existing X Screens on the GPU, or this display is
     * the only enabled device on the GPU.
     */
    if (display->gpu->num_displays == 1 || !display->gpu->num_screens ||
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
             "Separate X Screen (Requires X restart)");
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
             "Separate X Screen (Requires X restart)");  
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_twinview),
             "TwinView");

    } else {
        gtk_toggle_button_set_active
            (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_xscreen),
             TRUE);
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_disabled),
             "Disabled (Requires X restart)");
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_xscreen),
             "Separate X Screen");  
        gtk_button_set_label
            (GTK_BUTTON(ctk_object->rad_display_config_twinview),
             "TwinView (Requires X restart)");
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
            if (!add_modelines_to_display(display, &err_str)) {
                nv_warning_msg(err_str);
                g_free(err_str);
                return;
            }
        }
        if (!display->modelines) return;

        if (!display->screen) {

            /* Enable display as a separate X Screen */
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

            /* Move display to a new X Screen */
            if (display->screen->num_displays > 1 &&
                gtk_toggle_button_get_active
                (GTK_TOGGLE_BUTTON(ctk_object->rad_display_config_xscreen))) {
                do_configure_display_for_xscreen(ctk_object, display);
                update = TRUE;
            }
            
            /* Setup TwinView on the first X Screen */
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

        /* Recaltulate */
        ctk_display_layout_redraw(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));

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
        setup_display_frame(ctk_object);
        setup_screen_frame(ctk_object);
        
        gtk_widget_set_sensitive(ctk_object->btn_apply, True);
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
        int metamode_idx = find_closest_mode_with_modeline(display, modeline);

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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    nvModeLinePtr modeline;
    nvDisplayPtr display;


    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    modeline = ctk_object->resolution_table[idx];
    display = ctk_display_layout_get_selected_display
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));


    /* Ignore selecting same resolution */
    if (display->cur_mode->modeline == modeline) {
        return;
    }


    /* In Basic view, we assume the user most likely wants
     * to change which metamode is being used.
     */
    if (!ctk_object->advanced_mode && (display->screen->num_displays == 1)) {
        int metamode_idx = find_closest_mode_with_modeline(display, modeline);

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


    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

} /* display_resolution_changed() */



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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    str = read_pair(str, 0, &x, &y);
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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    
    str = read_pair(str, 'x', &x, &y);
    if (!str) {
        /* Reset the display panning */
        setup_display_panning(ctk_object);
        return;
    }

    ctk_display_layout_set_display_panning
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), display, x, y);

} /* display_panning_activate() */



/** screen_depth_changed() *******************************************
 *
 * Called when user selects a new color depth for a screen.
 *
 **/

static void screen_depth_changed(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
    int depth = 8;
    nvScreenPtr screen = ctk_display_layout_get_selected_screen
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
        
    if (!screen) return;

    /* Set the new default depth of the screen */
    if (idx == 0) {
        depth = 24;
    } else if (idx == 1) {
        depth = 16;
    }
    ctk_display_layout_set_screen_depth
        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), screen, depth);

    /* Can't apply screen depth changes */
    ctk_object->apply_possible = FALSE;

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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

    /* Get the new X Screen to be relative to */
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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    str = read_pair(str, 0, &x, &y);
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

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
        tmp = get_screen_metamode_str(screen, i, 1);
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
    setup_display_frame(ctk_object);

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);

    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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

    /* Make the apply button sensitive to user input */
    gtk_widget_set_sensitive(ctk_object->btn_apply, True);

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
    str = g_strdup_printf("The Mode on X Screen %d has been set.\n"
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

    parent = gtk_widget_get_parent(GTK_WIDGET(ctk_object));
    while (!GTK_IS_WINDOW(parent)) {
        GtkWidget *old = parent;
        parent = gtk_widget_get_parent(GTK_WIDGET(old));
        if (!parent || old == parent) {
            /* GTK Error, can't find parent window! */
            goto fail;
        }
    }


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
                       "(mode: %dx%d, id: %d) on X Screen %d!",
                       screen->cur_metamode_idx+1, metamode->string, new_width,
                       new_height, new_rate,
                       NvCtrlGetTargetId(screen->handle));

        if (screen->num_metamodes > 1) {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X Screen %d\n\n"
                                  "Would you like to remove this MetaMode?",
                                  screen->cur_metamode_idx+1, metamode->string,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->handle));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_INFO,
                 GTK_BUTTONS_YES_NO,
                 msg);
        } else {
            msg = g_strdup_printf("Failed to set MetaMode (%d) '%s' "
                                  "(Mode %dx%d, id: %d) on X Screen %d.",
                                  screen->cur_metamode_idx+1, metamode->string,
                                  new_width, new_height, new_rate,
                                  NvCtrlGetTargetId(screen->handle));
            dlg = gtk_message_dialog_new
                (GTK_WINDOW(parent),
                 GTK_DIALOG_DESTROY_WITH_PARENT,
                 GTK_MESSAGE_INFO,
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
            setup_display_frame(ctk_object);
            setup_screen_frame(ctk_object);
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
    
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
        /* Kill the timer */
        g_source_remove(ctk_object->display_confirm_timer);
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
            str = (char *)skip_whitespace(str +2);
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
 * - Adds new metamodes to the X Server screen that are specified
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

        /* Generate the metamore's string */
        free(metamode->string);
        metamode->string = get_screen_metamode_str(screen, metamode_idx, 0);
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
                    parse_tokens(tokens, apply_metamode_token, metamode);
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
        
        /* The metamode was not found, so add it to the X Screen's list */
        tokens = NULL;
        ret = NvCtrlStringOperation(screen->handle, 0,
                                    NV_CTRL_STRING_OPERATION_ADD_METAMODE,
                                    metamode->string, &tokens);

        /* Grab the metamode ID from the returned tokens */
        if (ret == NvCtrlSuccess) {
            if (tokens) {
                parse_tokens(tokens, apply_metamode_token, metamode);
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

        metamode_str = get_screen_metamode_str(screen, metamode_idx,
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

        str = (char *)skip_whitespace(str +2);


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
        metamode_str = (char *)skip_whitespace(metamode_str +2);
    } else {
        metamode_str = cur_metamode_str;
    }

    /* Preprocess the new metamodes list */

    preprocess_metamodes(screen, metamode_strs);
    
    /* If we need to switch metamodes, do so now */

    if (strcmp(screen->cur_metamode->string, metamode_str)) {
        if (switch_to_current_metamode(ctk_object, screen)) {
            ctk_config_statusbar_message(ctk_object->ctk_config,
                                         "Switched to mode %dx%d "
                                         "@ %d Hz.",
                                         screen->cur_metamode->edim[W],
                                         screen->cur_metamode->edim[H],
                                         screen->cur_metamode->id);

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
    int clear_apply = 1;


    /* Make sure we can apply */
    if (!validate_apply(ctk_object)) {
        return;
    }

    /* Make sure the layout is ready to be applied */
    if (!validate_layout(ctk_object)) {
        return;
    }

    /* Update all GPUs */
    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        nvScreenPtr screen;

        /* Update all X Screens */
        for (screen = gpu->screens; screen; screen = screen->next) {

            if (!screen->handle) continue;

            if (!update_screen_metamodes(ctk_object, screen)) {
                clear_apply = FALSE;
            } else {
                ReturnStatus ret;
                ret = NvCtrlSetAttribute(screen->handle,
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
        }
    }

    /* Clear the apply button if all went well */
    if (clear_apply) {
        gtk_widget_set_sensitive(widget, False);
    }

} /* apply_clicked() */



/** xconfig_file_clicked() *******************************************
 *
 * Called when the user clicks on the "Browse..." button of the
 * X Config save dialog.
 *
 **/

static void xconfig_file_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    const gchar *filename =
        gtk_entry_get_text(GTK_ENTRY(ctk_object->txt_xconfig_file));
    gint result;


    /* Ask user for a filename */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_xconfig_file),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));

    gtk_file_selection_set_filename
        (GTK_FILE_SELECTION(ctk_object->dlg_xconfig_file), filename);
    
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_xconfig_file));
    gtk_widget_hide(ctk_object->dlg_xconfig_file);
    
    switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
        
        filename = gtk_file_selection_get_filename
            (GTK_FILE_SELECTION(ctk_object->dlg_xconfig_file));

        gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_xconfig_file),
                           filename);
        break;
    default:
        return;
    }

} /* xconfig_file_clicked() */



/** xconfig_preview_clicked() ****************************************
 *
 * Called when the user clicks on the "Preview" button of the
 * X Config save dialog.
 *
 **/

static void xconfig_preview_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gboolean show = !GTK_WIDGET_VISIBLE(ctk_object->box_xconfig_save);

    if (show) {
        gtk_widget_show_all(ctk_object->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_xconfig_save),
                                 TRUE);
        gtk_widget_set_size_request(ctk_object->txt_xconfig_save, 450, 350);
        gtk_button_set_label(GTK_BUTTON(ctk_object->btn_xconfig_preview),
                             "Hide Preview...");
    } else {
        gtk_widget_hide(ctk_object->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_xconfig_save),
                                 FALSE);
        gtk_button_set_label(GTK_BUTTON(ctk_object->btn_xconfig_preview),
                             "Show Preview...");
    }

} /* xconfig_preview_clicked() */



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
        xconf_modeline->identifier = strdup(modeline->xconfig_name);
    } else if (modeline->data.identifier) {
        xconf_modeline->identifier = strdup(modeline->data.identifier);
    }
    
    if (modeline->data.comment) {
        xconf_modeline->comment = strdup(modeline->data.comment);
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

        /* Only add modelines that originated from the X Config
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
        monitor->modelines =
            (XConfigModeLinePtr)xconfigAddListItem((GenericListPtr)monitor->modelines,
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
    
    monitor = (XConfigMonitorPtr)calloc(1, sizeof(XConfigMonitorRec));
    if (!monitor) goto fail;
    
    monitor->identifier = (char *)malloc(32);
    snprintf(monitor->identifier, 32, "Monitor%d", monitor_id);
    monitor->vendor = xconfigStrdup("Unknown");  /* XXX */
    monitor->modelname = xconfigStrdup(display->name);
    
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
    
    if (!read_float_range(tmp, &min, &max)) {
        nv_error_msg("Unable to determine valid horizontal sync ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->name, display->gpu->name);
        goto fail;
    }

    monitor->n_hsync = 1;
    monitor->hsync[0].lo = min;
    monitor->hsync[0].hi = max;
    
    parse_tokens(range_str, apply_monitor_token, (void *)(&h_source));
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
    
    if (!read_float_range(tmp, &min, &max)) {
        nv_error_msg("Unable to determine valid vertical refresh ranges "
                     "for display device '%s' (GPU: %s)!",
                     display->name, display->gpu->name);
        goto fail;
    }

    monitor->n_vrefresh = 1;
    monitor->vrefresh[0].lo = min;
    monitor->vrefresh[0].hi = max;

    parse_tokens(range_str, apply_monitor_token, (void *)(&v_source));
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

    opt = xconfigAddNewOption(opt, xconfigStrdup("DPMS"), NULL);
    monitor->options = opt;

    /* Add modelines used by this display */

    add_modelines_to_monitor(monitor, display->modes);

    /* Append the monitor to the end of the monitor list */

    config->monitors =
        (XConfigMonitorPtr)xconfigAddListItem((GenericListPtr)config->monitors,
                                              (GenericListPtr)monitor);

    display->conf_monitor = monitor;
    return TRUE;


 fail:
    free(range_str);
    free(h_source);
    free(v_source);
    if (monitor) {
        xconfigFreeMonitorList(monitor);
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
    snprintf(device->identifier, 32, "Videocard%d", device_id);

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
    config->devices =
        (XConfigDevicePtr)xconfigAddListItem((GenericListPtr)config->devices,
                                             (GenericListPtr)device);

    return device;

 fail:
    if (device) {
        xconfigFreeDeviceList(device);
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
    /* Clear the display list */
    xconfigFreeDisplayList(conf_screen->displays);
    conf_screen->displays = NULL;


    /* Add a single display subsection for the default depth */
    conf_screen->displays = xconfigAddDisplay(NULL, conf_screen->defaultdepth);
    if (!conf_screen->displays) goto fail;


    /* XXX Don't do any further tweaking to the display subsection.
     *     All mode configuration should be done through the 'MetaModes"
     *     X Option.  The modes generated by xconfigAddDisplay() will
     *     be used as a fallack.
     */

    return TRUE;

 fail:
    xconfigFreeDisplayList(conf_screen->displays);
    conf_screen->displays = NULL;

    return FALSE;
    
} /* add_display_to_screen() */



/*
 * add_screen_to_xconfig() - Adds the given X Screen's information
 * to the X configuration structure.
 */

static Bool add_screen_to_xconfig(CtkDisplayConfig *ctk_object,
                                  nvScreenPtr screen, XConfigPtr config,
                                  int screen_id)
{
    XConfigScreenPtr conf_screen;
    nvDisplayPtr display;
    nvDisplayPtr other;
    char *metamode_strs;

    conf_screen = (XConfigScreenPtr)calloc(1, sizeof(XConfigScreenRec));
    if (!conf_screen) goto fail;


    /* Fill out the screen information */
    conf_screen->identifier = (char *)malloc(32);
    snprintf(conf_screen->identifier, 32, "Screen%d", screen_id);

    
    /* Tie the screen to its device section */
    conf_screen->device_name =
        xconfigStrdup(screen->conf_device->identifier);
    conf_screen->device = screen->conf_device;

    
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
    if (!add_monitor_to_xconfig(display, config, screen_id)) {
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

    /* Add the TwinView option for multi monitor setups */
    if (screen->num_displays > 1) {
        conf_screen->options =
            xconfigAddNewOption(conf_screen->options,
                                xconfigStrdup("TwinView"),
                                xconfigStrdup("1"));
    }

    /* XXX Setup any other twinview options ... */
    
    /* Setup the metamode section.
     *
     * In basic view, always specify the currently selected
     * metamode first in the list so the X server starts
     * in this mode.
     */
    metamode_strs = get_screen_metamode_strs(screen, 1,
                                             !ctk_object->advanced_mode);

    /* If no user specified metamodes were found, add
     * whatever the currently selected metamode is
     */
    if (!metamode_strs) {
        metamode_strs = get_screen_metamode_str(screen,
                                                screen->cur_metamode_idx, 1);
    }

    if (metamode_strs) {
        conf_screen->options =
            xconfigAddNewOption(conf_screen->options,
                                xconfigStrdup("metamodes"),
                                metamode_strs);
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
    config->screens =
        (XConfigScreenPtr)xconfigAddListItem((GenericListPtr)config->screens,
                                             (GenericListPtr)conf_screen);
    
    screen->conf_screen = conf_screen;
    return TRUE;

 fail:
    if (conf_screen) {
        xconfigFreeScreenList(conf_screen);
    }
    return FALSE;

} /* add_screen_to_xconfig() */



/*
 * add_screens_to_xconfig() - Adds all the X Screens in the given
 * layout to the X configuration structure.
 */

static Bool add_screens_to_xconfig(CtkDisplayConfig *ctk_object,
                                   nvLayoutPtr layout, XConfigPtr config)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    int device_id, screen_id;
    int print_bus_ids;


    /* Clear the screen list */
    xconfigFreeMonitorList(config->monitors);
    config->monitors = NULL;
    xconfigFreeDeviceList(config->devices);
    config->devices = NULL;
    xconfigFreeScreenList(config->screens);
    config->screens = NULL;

    /* Don't print the bus ID in the case where we have a single
     * GPU driving a single X Screen
     */
    if (layout->num_gpus == 1 &&
        layout->gpus->num_screens == 1) {
        print_bus_ids = 0;
    } else {
        print_bus_ids = 1;
    }

    /* Generate the Device sections and Screen sections */

    device_id = 0;
    screen_id = 0;

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        int device_screen_id = -1;

        for (screen = gpu->screens; screen; screen = screen->next) {

            /* Only print a screen number if more than 1 screen on gpu */
            if (gpu->num_screens > 1) {
                device_screen_id++;
            }

            /* Each screen needs a unique device section */
            screen->conf_device = add_device_to_xconfig(gpu, config,
                                                        device_id,
                                                        device_screen_id,
                                                        print_bus_ids);
            if (!screen->conf_device) {
                nv_error_msg("Failed to add Device '%s' to X Config.",
                             gpu->name);
                goto fail;
            }

            if (!add_screen_to_xconfig(ctk_object, screen,
                                       config, screen_id)) {
                nv_error_msg("Failed to add X Screen %d to X Config.",
                             screen->scrnum);
                goto fail;
            }

            device_id++;
            screen_id++;
        }
    }
    return TRUE;

 fail:
    xconfigFreeMonitorList(config->monitors);
    config->monitors = NULL;
    xconfigFreeDeviceList(config->devices);
    config->devices = NULL;
    xconfigFreeScreenList(config->screens);
    config->screens = NULL;    
    return FALSE;

} /* add_screens_to_xconfig() */



/*
 * add_adjacency_to_xconfig() - Adds the given X screen's positioning
 * information to an X config structure.
 */

static Bool add_adjacency_to_xconfig(nvScreenPtr screen, XConfigPtr config,
                                     int scrnum)
{
    XConfigAdjacencyPtr adj;
    XConfigLayoutPtr conf_layout = config->layouts;


    adj = (XConfigAdjacencyPtr) calloc(1, sizeof(XConfigAdjacencyRec));
    if (!adj) return FALSE;

    adj->scrnum = scrnum;
    adj->screen = screen->conf_screen;
    adj->screen_name = xconfigStrdup(screen->conf_screen->identifier);
    
    /* Position the X Screen */
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
    conf_layout->adjacencies =
        (XConfigAdjacencyPtr)xconfigAddListItem((GenericListPtr)conf_layout->adjacencies,
                                                (GenericListPtr)adj);

    return TRUE;

} /* add_adjacency_to_xconfig() */



/*
 * add_layout_to_xconfig() - Adds layout (adjacency/X Screen
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
    xconfigFreeAdjacencyList(conf_layout->adjacencies);
    conf_layout->adjacencies = NULL;

    
    /* Assign the adjacencies */
    scrnum = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            if (!add_adjacency_to_xconfig(screen, config, scrnum)) goto fail;
            scrnum++;
        }
    }


    /* Setup for Xinerama */
    if (!config->flags) {
        config->flags = (XConfigFlagsPtr) calloc(1, sizeof(XConfigFlagsRec));
        if (!config->flags) goto fail;
    }
    config->flags->options =
        xconfigAddNewOption(config->flags->options,
                            xconfigStrdup("Xinerama"),
                            xconfigStrdup(layout->xinerama_enabled?"1":"0"));

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

static XConfigPtr generateXConfig(CtkDisplayConfig *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    XConfigPtr config = NULL;
    GenerateOptions go;
    char *server_vendor;


    /* Query server Xorg/XFree86 */
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
    if (!add_screens_to_xconfig(ctk_object, layout, config)) {
        nv_error_msg("Failed to add X Screens to X Config.");
        goto fail;
    }
    if (!add_layout_to_xconfig(layout, config)) {
        nv_error_msg("Failed to add Server Layout to X Config.");
        goto fail;
    }

    return config;

 fail:
    if (config) {
        xconfigFreeConfig(config);
    }
    return NULL;

} /* generateXConfig() */



/*
 * tilde_expansion() - do tilde expansion on the given path name;
 * based loosely on code snippets found in the comp.unix.programmer
 * FAQ.  The tilde expansion rule is: if a tilde ('~') is alone or
 * followed by a '/', then substitute the current user's home
 * directory; if followed by the name of a user, then substitute that
 * user's home directory.
 *
 * Code adapted from nvidia-xconfig
 */

char *tilde_expansion(char *str)
{
    char *prefix = NULL;
    char *replace, *user, *ret;
    struct passwd *pw;
    int len;

    if ((!str) || (str[0] != '~')) return str;
    
    if ((str[1] == '/') || (str[1] == '\0')) {

        /* expand to the current user's home directory */

        prefix = getenv("HOME");
        if (!prefix) {
            
            /* $HOME isn't set; get the home directory from /etc/passwd */
            
            pw = getpwuid(getuid());
            if (pw) prefix = pw->pw_dir;
        }
        
        replace = str + 1;
        
    } else {
    
        /* expand to the specified user's home directory */

        replace = strchr(str, '/');
        if (!replace) replace = str + strlen(str);

        len = replace - str;
        user = malloc(len + 1);
        strncpy(user, str+1, len-1);
        user[len] = '\0';
        pw = getpwnam(user);
        if (pw) prefix = pw->pw_dir;
        free (user);
    }

    if (!prefix) return str;
    
    ret = malloc(strlen(prefix) + strlen(replace) + 1);
    strcpy(ret, prefix);
    strcat(ret, replace);
    
    return ret;

} /* tilde_expansion() */



/*
 * update_banner() - add our banner at the top of the config, but
 * first we need to remove any lines that already include our prefix
 * (because presumably they are a banner from an earlier run of
 * nvidia-settings)
 *
 * Code adapted from nvidia-xconfig
 */

extern const char *pNV_ID;

static void update_banner(XConfigPtr config)
{
    static const char *banner =
        "X configuration file generated by nvidia-settings\n";
    static const char *prefix =
        "# nvidia-settings: ";

    char *s = config->comment;
    char *line, *eol, *tmp;
    
    /* remove all lines that begin with the prefix */
    
    while (s && (line = strstr(s, prefix))) {
        
        eol = strchr(line, '\n'); /* find the end of the line */
        
        if (eol) {
            eol++;
            if (*eol == '\0') eol = NULL;
        }
        
        if (line == s) { /* the line with the prefix is at the start */
            if (eol) {   /* there is more after the prefix line */
                tmp = g_strdup(eol);
                g_free(s);
                s = tmp;
            } else {     /* the prefix line is the only line */
                g_free(s);
                s = NULL;
            }
        } else {         /* prefix line is in the middle or end */
            *line = '\0';
            tmp = g_strconcat(s, eol, NULL);
            g_free(s);
            s = tmp;
        }
    }
    
    /* add our prefix lines at the start of the comment */
    config->comment = g_strconcat(prefix, banner,
                                  "# ", pNV_ID, "\n",
                                  (s ? s : ""),
                                  NULL);
    if (s) g_free(s);
    
} /* update_banner() */



/** save_xconfig_file() **********************************************
 *
 * Saves the X config file text from buf into a file called
 * filename.  If filename already exists, a backup file named
 * 'filename.backup' is created.
 *
 **/

static int save_xconfig_file(gchar *filename, char *buf, mode_t mode)
{
    gchar *backup_filename = NULL;
    FILE *fp = NULL;
    size_t size;

    int ret = 0;


    if (!buf || !filename) goto done;

    size = strlen(buf) ;

    /* Backup any existing file */
    if ((access(filename, F_OK) == 0)) {

        backup_filename = g_strdup_printf("%s.backup", filename);
        nv_info_msg("", "X configuration file '%s' already exists, "
                    "backing up file as '%s'", filename,
                    backup_filename);
        
        /* Delete any existing backup file */
        if (access(backup_filename, F_OK) == 0) {

            if (unlink(backup_filename) != 0) {
                nv_error_msg("Unable to create backup file '%s'.",
                             backup_filename);
                goto done;
            }
        }

        /* Make the current x config file the backup */
        if (rename(filename, backup_filename)) {
            nv_error_msg("Unable to create backup file '%s'.",
                         backup_filename);
            goto done;            
        }
    }

    /* Write out the X Config file */
    fp = fopen(filename, "w");
    if (!fp) {
        nv_error_msg("Unable to open file '%s' for writing.",
                     filename);
        goto done;
    }
    fprintf(fp, "%s", buf);

    ret = 1;
    
 done:

    if (fp) fclose(fp);
    g_free(backup_filename);
    return ret;
    
} /* save_xconfig_file() */



/** save_clicked() ***************************************************
 *
 * Called when the user clicks on the "Save" button.
 *
 **/

static void save_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    gint result;

    gchar *filename;
    XConfigPtr config = NULL;    

    char *tmp_filename;
    int tmp_fd;
    struct stat st;
    void *buf;
    GtkTextIter buf_start, buf_end;


    /* Make sure the layout is ready to be saved */
    if (!validate_layout(ctk_object)) {
        return;
    }


    /* Setup the default X config filename */
    if (!ctk_object->layout->filename) {
        filename = (gchar *) xconfigOpenConfigFile(NULL, NULL);
        if (filename) {
            ctk_object->layout->filename = g_strdup(filename);
            xconfigCloseConfigFile();
            filename = NULL;
        } else {
            ctk_object->layout->filename = g_strdup("");
        }
    }
    gtk_entry_set_text(GTK_ENTRY(ctk_object->txt_xconfig_file),
                       ctk_object->layout->filename);


    /* Generate an X Config file from the layout */
    config = generateXConfig(ctk_object);
    if (!config) {
        nv_error_msg("Failed to generate an X config file!");
        return;
    }

    /* Update the X Config banner */
    update_banner(config);

    /* Setup the X config file preview buffer by writing to a temp file */
    tmp_filename = g_strdup_printf("/tmp/.xconfig.tmp.XXXXXX");
    tmp_fd = mkstemp(tmp_filename);
    if (!tmp_fd) {
        nv_error_msg("Failed to create temp file for displaying X config!");
        g_free(tmp_filename);
        return;
    }
    xconfigWriteConfigFile(tmp_filename, config);
    xconfigFreeConfig(config);

    lseek(tmp_fd, 0, SEEK_SET);
    fstat(tmp_fd, &st);
    buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, tmp_fd, 0);
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(ctk_object->buf_xconfig_save),
                             buf, st.st_size);
    munmap(buf, st.st_size);
    close(tmp_fd);
    remove(tmp_filename);
    g_free(tmp_filename);
    

    /* Confirm the save */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_object->dlg_xconfig_save),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ctk_object))));
    gtk_widget_hide(ctk_object->box_xconfig_save);
    gtk_window_resize(GTK_WINDOW(ctk_object->dlg_xconfig_save), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(ctk_object->dlg_xconfig_save),
                             FALSE);
    gtk_button_set_label(GTK_BUTTON(ctk_object->btn_xconfig_preview),
                         "Show preview...");
    gtk_widget_show(ctk_object->dlg_xconfig_save);
    result = gtk_dialog_run(GTK_DIALOG(ctk_object->dlg_xconfig_save));
    gtk_widget_hide(ctk_object->dlg_xconfig_save);
    
    
    /* Handle user's response */
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:

        /* Get the filename to write to */
        filename =
            (gchar *) gtk_entry_get_text(GTK_ENTRY(ctk_object->txt_xconfig_file));

        g_free(ctk_object->layout->filename);
        ctk_object->layout->filename = tilde_expansion(filename);
        if (ctk_object->layout->filename == filename) {
            ctk_object->layout->filename = g_strdup(filename);
        }
        filename = ctk_object->layout->filename;


        /* Get the buffer to write */
        gtk_text_buffer_get_bounds
            (GTK_TEXT_BUFFER(ctk_object->buf_xconfig_save), &buf_start,
             &buf_end);
        buf = (void *) gtk_text_buffer_get_text
            (GTK_TEXT_BUFFER(ctk_object->buf_xconfig_save), &buf_start,
             &buf_end, FALSE);
        if (!buf) {
            nv_error_msg("Failed to read X configuration buffer!");
            break;
        }

        /* Save the X config file */
        nv_info_msg("", "Writing X Config file '%s'", filename);
        save_xconfig_file(filename, (char *)buf, 0644);
        g_free(buf);
        break;
        
    case GTK_RESPONSE_REJECT:
    default:
        /* do nothing. */
        break;
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
        ctk_display_layout_set_advanced_mode(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                             1);

    /* Show basic display options */
    } else {
        gtk_button_set_label(GTK_BUTTON(widget), "Advanced...");
        ctk_display_layout_set_advanced_mode(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                                             0);
    }

    
    /* Update the GUI to show the right widgets */
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);

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
        

        /* Do the probe */
        ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_PROBE_DISPLAYS,
                                 (int *)&probed_displays);
        if (ret != NvCtrlSuccess) {
            nv_error_msg("Failed to probe for display devices on GPU-%d '%s'.",
                         NvCtrlGetTargetId(gpu->handle), gpu->name);
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

                display = get_display_from_gpu(gpu, mask);
                if (!display) continue; /* XXX ack. */

                /* The selected display is being removed */
                if (display == selected_display) {
                    selected_display = NULL;
                }
                    
                /* Setup the remove display dialog */
                type = get_display_type_str(display->device_mask, 0);
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
                    remove_display_from_gpu(display);
                    free_display(display);
                    
                    /* Let display layout widget know about change */
                    ctk_display_layout_update_display_count
                        (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout), NULL);

                    /* Activate the apply button */
                    gtk_widget_set_sensitive(ctk_object->btn_apply, True);
                }

            /* Add new displays as 'disabled' */
            } else if (!(gpu->connected_displays & mask) &&
                       (probed_displays & mask)) {
                gchar *err_str = NULL;
                display = add_display_to_gpu(gpu, mask, &err_str);
                if (err_str) {
                    nv_warning_msg(err_str);
                    g_free(err_str);
                }
                add_screenless_modes_to_displays(gpu);
                ctk_display_layout_update_display_count
                    (CTK_DISPLAY_LAYOUT(ctk_object->obj_layout),
                     selected_display);
            }
        }
    }


    /* Sync the GUI */
    ctk_display_layout_redraw(CTK_DISPLAY_LAYOUT(ctk_object->obj_layout));
    
    setup_display_frame(ctk_object);
    
    setup_screen_frame(ctk_object);

} /* probe_clicked() */



/** reset_clicked() **************************************************
 *
 * Called when user clicks on the "Reset" button.
 *
 **/

static void reset_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkDisplayConfig *ctk_object = CTK_DISPLAY_CONFIG(user_data);
    nvLayoutPtr layout;
    gint result;
    gchar *err_str = NULL;


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


    /* Load the current layout */
    layout = load_server_layout(ctk_object->handle, &err_str);


    /* Handle errors loading the new layout */
    if (!layout || err_str) {
        nv_error_msg(err_str);
        g_free(err_str);
        return;
    }   


    /* Free the existing layout */
    if (ctk_object->layout) {
        remove_gpus_from_layout(ctk_object->layout);
        free(ctk_object->layout);
    }


    /* Setup the new layout */
    ctk_object->layout = layout;
    ctk_display_layout_set_layout((CtkDisplayLayout *)(ctk_object->obj_layout),
                                  ctk_object->layout);


    /* Make sure X Screens have some kind of position */
    assign_screen_positions(ctk_object);


    /* Setup the GUI */
    setup_layout_frame(ctk_object);
    setup_display_frame(ctk_object);
    setup_screen_frame(ctk_object);

    /* Get new position */
    get_cur_screen_pos(ctk_object);

    /* Clear the apply button */
    ctk_object->apply_possible = TRUE;
    gtk_widget_set_sensitive(ctk_object->btn_apply, FALSE);

} /* reset_clicked() */



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
