/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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

#include <gtk/gtk.h>

#include "msg.h"
#include "parse.h"

#include "ctkdisplayconfig-utils.h"




/*****************************************************************************/
/** TOKEN PARSING FUNCTIONS **************************************************/
/*****************************************************************************/


/** apply_modeline_token() *******************************************
 *
 * Modifies the modeline structure given with the token/value pair
 * given.
 *
 **/
void apply_modeline_token(char *token, char *value, void *data)
{
    nvModeLinePtr modeline = (nvModeLinePtr) data;

    if (!modeline || !token || !strlen(token)) {
        return;
    }

    /* Modeline source */
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

    /* X config name */
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
void apply_metamode_token(char *token, char *value, void *data)
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

} /* apply_metamode_token() */



/** apply_monitor_token() ********************************************
 *
 * Reads the source of a refresh/sync range value
 *
 **/
void apply_monitor_token(char *token, char *value, void *data)
{
    char **source = (char **)data;
    
    if (!source || !token || !strlen(token)) {
        return;
    }

    /* Vert sync or horiz refresh source */
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
void apply_screen_info_token(char *token, char *value, void *data)
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




/*****************************************************************************/
/** MODELINE FUNCTIONS *******************************************************/
/*****************************************************************************/


/** modeline_parse() *************************************************
 *
 * Converts a modeline string to an modeline structure that the
 * display configuration page can use
 *
 * Modeline strings have the following format:
 *
 *   "mode_name"  dot_clock  timings  flags
 *
 **/
static nvModeLinePtr modeline_parse(const char *modeline_str)
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
        parse_token_value_pairs(tokens, apply_modeline_token,
                                (void *)modeline);
        free(tokens);
    }

    /* Read the mode name */
    str = parse_skip_whitespace(str);
    if (!str || *str != '"') goto fail;
    str++;
    str = parse_read_name(str, &(modeline->data.identifier), '"');
    if (!str) goto fail;

    /* Read dot clock */
    {
        int digits = 100;

        str = parse_read_integer(str, &(modeline->data.clock));
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
        str = parse_skip_whitespace(str);
    }

    str = parse_read_integer(str, &(modeline->data.hdisplay));
    str = parse_read_integer(str, &(modeline->data.hsyncstart));
    str = parse_read_integer(str, &(modeline->data.hsyncend));
    str = parse_read_integer(str, &(modeline->data.htotal));
    str = parse_read_integer(str, &(modeline->data.vdisplay));
    str = parse_read_integer(str, &(modeline->data.vsyncstart));
    str = parse_read_integer(str, &(modeline->data.vsyncend));
    str = parse_read_integer(str, &(modeline->data.vtotal));


    /* Parse modeline flags */
    while ((str = parse_read_name(str, &tmp, ' ')) && strlen(tmp)) {
        
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
            str = parse_read_integer(str, &(modeline->data.hskew));
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
            str = parse_read_integer(str, &(modeline->data.vscan));
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

} /* modeline_parse() */




/*****************************************************************************/
/** MODE FUNCTIONS ***********************************************************/
/*****************************************************************************/


/** mode_parse() *****************************************************
 *
 * Converts a mode string (dpy specific part of a metamode) to a
 * mode structure that the display configuration page can use.
 *
 * Mode strings have the following format:
 *
 *   "mode_name +X+Y @WxH"
 *
 **/
nvModePtr mode_parse(nvDisplayPtr display, const char *mode_str)
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
    str = parse_read_name(str, &mode_name, ' ');
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
            str = parse_read_integer_pair(str, 'x',
                                          &(mode->pan[W]), &(mode->pan[H]));
        }

        /* Read position */
        else if (*str == '+') {
            str++;
            str = parse_read_integer_pair(str, 0,
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

} /* mode_parse() */



/** mode_get_str() ***************************************************
 *
 * Returns the mode string of the given mode in the following format:
 *
 * "mode_name @WxH +X+Y"
 *
 **/
static gchar *mode_get_str(nvModePtr mode, int be_generic)
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
        tmp = display_get_type_str(mode->display->device_mask, generic);
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

} /* mode_get_str() */




/*****************************************************************************/
/** DISPLAY FUNCTIONS ********************************************************/
/*****************************************************************************/


/** display_get_type_str() *******************************************
 *
 * Returns the type name of a display (CRT, CRT-1, DFP ..)
 *
 * If 'generic' is set to 1, then a generic version of the name is
 * returned.
 *
 **/
gchar *display_get_type_str(unsigned int device_mask, int be_generic)
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

} /* display_get_type_str() */



/** display_find_closest_mode_matching_modeline() ********************
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
int display_find_closest_mode_matching_modeline(nvDisplayPtr display,
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

} /* display_find_closest_mode_matching_modeline() */



/** display_remove_modelines() ***************************************
 *
 * Clears the display device's modeline list.
 *
 **/
static void display_remove_modelines(nvDisplayPtr display)
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

} /* display_remove_modelines() */



/** display_add_modelines_from_server() ******************************
 *
 * Queries the display's current modepool (modelines list).
 *
 **/
Bool display_add_modelines_from_server(nvDisplayPtr display, gchar **err_str)
{
    nvModeLinePtr modeline;
    char *modeline_strs = NULL;
    char *str;
    int len;
    ReturnStatus ret;


    /* Free any old mode lines */
    display_remove_modelines(display);


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

        modeline = modeline_parse(str);
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
    display_remove_modelines(display);
    XFree(modeline_strs);
    return FALSE;

} /* display_add_modelines_from_server() */



/** display_get_mode_str() *******************************************
 *
 * Returns the mode string of the display's 'mode_idx''s
 * mode.
 *
 **/
static gchar *display_get_mode_str(nvDisplayPtr display, int mode_idx,
                                   int be_generic)
{
    nvModePtr mode = display->modes;

    while (mode && mode_idx) {
        mode = mode->next;
        mode_idx--;
    }
    
    if (mode) {
        return mode_get_str(mode, be_generic);
    }

    return NULL;

} /* display_get_mode_str() */



/** display_remove_modes() *******************************************
 *
 * Removes all modes currently referenced by this screen, also
 * freeing any memory used.
 *
 **/
void display_remove_modes(nvDisplayPtr display)
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

} /* display_remove_modes() */



/** display_free() ***************************************************
 *
 * Frees memory used by a display
 *
 **/
static void display_free(nvDisplayPtr display)
{
    if (display) {
        display_remove_modes(display);
        display_remove_modelines(display);
        XFree(display->name);
        free(display);
    }

} /* display_free() */




/*****************************************************************************/
/** METAMODE FUNCTIONS *******************************************************/
/*****************************************************************************/




/*****************************************************************************/
/** SCREEN FUNCTIONS *********************************************************/
/*****************************************************************************/


/** screen_remove_display() ******************************************
 *
 * Removes a display device from the screen
 *
 **/
void screen_remove_display(nvDisplayPtr display)
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
        display_remove_modes(display);
        display->screen = NULL;
    }

} /* screen_remove_display() */



/** screen_remove_displays() *****************************************
 *
 * Removes all displays currently pointing at this screen, also
 * freeing any memory used.
 *
 **/
static void screen_remove_displays(nvScreenPtr screen)
{
    nvGpuPtr gpu;
    nvDisplayPtr display;

    if (screen && screen->gpu) {
        gpu = screen->gpu;
        
        for (display = gpu->displays; display; display = display->next) {
            
            if (display->screen != screen) continue;
            
            screen_remove_display(display);
        }
    }

} /* screen_remove_displays() */



/** screen_get_metamode_str() ****************************************
 *
 * Returns a screen's metamode string for the given metamode index
 * as:
 *
 * "mode1_1, mode1_2, mode1_3 ... "
 *
 **/
gchar *screen_get_metamode_str(nvScreenPtr screen, int metamode_idx,
                               int be_generic)
{
    nvDisplayPtr display;

    gchar *metamode_str = NULL;
    gchar *mode_str;
    gchar *tmp;

    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue; /* Display not in screen */
    
        mode_str = display_get_mode_str(display, metamode_idx, be_generic);
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

} /* screen_get_metamode_str() */



/** screen_remove_metamodes() ****************************************
 *
 * Removes all metamodes currently referenced by this screen, also
 * freeing any memory used.
 *
 **/
static void screen_remove_metamodes(nvScreenPtr screen)
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

                display_remove_modes(display);
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
        
} /* screen_remove_metamodes() */



/** screen_add_metamode() ********************************************
 *
 * Parses a metamode string and adds the appropreate modes to the
 * screen's display devices (at the end of the list)
 *
 **/
static Bool screen_add_metamode(nvScreenPtr screen, char *metamode_str,
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
        parse_token_value_pairs(tokens, apply_metamode_token,
                                (void *)metamode);
        free(tokens);
    } else {
        /* No tokens?  Try the old "ID: METAMODE_STR" syntax */
        tmp = (char *)parse_read_integer(str, &(metamode->id));
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
        const char *orig_mode_str = parse_skip_whitespace(mode_str);


        /* Parse the display device bitmask from the name */
        mode_str = (char *)parse_read_display_name(mode_str, &device_mask);
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
        display = gpu_get_display(screen->gpu, device_mask);
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
        mode = mode_parse(display, mode_str);
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

} /* screen_add_metamode() */



/** screen_check_metamodes() *****************************************
 *
 * Makes sure all displays associated with the screen have the right
 * number of mode entries.
 *
 **/
static Bool screen_check_metamodes(nvScreenPtr screen)
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
            mode = mode_parse(display, "NULL");
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

} /* screen_check_metamodes() */



/** screen_assign_dummy_metamode_positions() *************************
 *
 * Assign the initial (top left) position of dummy modes to
 * match the top left of the first non-dummy mode
 *
 **/
void screen_assign_dummy_metamode_positions(nvScreenPtr screen)
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
    
} /* screen_assign_dummy_metamode_positions() */



/** screen_add_metamodes() *******************************************
 *
 * Adds all the appropreate modes on all display devices of this
 * screen by parsing all the metamode strings.
 *
 **/
static Bool screen_add_metamodes(nvScreenPtr screen, gchar **err_str)
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
    screen_remove_metamodes(screen);


    /* Parse each mode in the metamode strings */
    str = metamode_strs;
    while (str && strlen(str)) {

        /* Add the individual metamodes to the screen,
         * This populates the display device's mode list.
         */
        if (!screen_add_metamode(screen, str, err_str)) {
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
        screen_check_metamodes(screen);

        /* Go to the next metamode */
        str += strlen(str) +1;
    }
    XFree(metamode_strs);


    /* Assign the top left position of dummy modes */
    screen_assign_dummy_metamode_positions(screen);


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
    screen_remove_metamodes(screen);

    XFree(metamode_strs);
    return FALSE;    

} /* screen_add_metamodes() */



/** screen_free() ****************************************************
 *
 * Frees memory used by a screen structure
 *
 **/
static void screen_free(nvScreenPtr screen)
{
    if (screen) {

        screen_remove_metamodes(screen);
        screen_remove_displays(screen);

        if (screen->handle) {
            NvCtrlAttributeClose(screen->handle);
        }

        free(screen);
    }

} /* screen_free() */




/*****************************************************************************/
/** GPU FUNCTIONS ************************************************************/
/*****************************************************************************/


/** gpu_get_display() ************************************************
 *
 * Returns the display with the matching device_mask or NULL if not
 * found.
 *
 **/
nvDisplayPtr gpu_get_display(nvGpuPtr gpu, unsigned int device_mask)
{
    nvDisplayPtr display;

    for (display = gpu->displays; display; display = display->next) {
        if (display->device_mask == device_mask) return display;
    }
    
    return NULL;

} /* gpu_get_display() */



/** gpu_remove_and_free_display() ************************************
 *
 * Removes a display from the GPU and frees it.
 *
 **/
void gpu_remove_and_free_display(nvDisplayPtr display)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;


    if (display && display->gpu) {
        gpu = display->gpu;
        screen = display->screen;

        /* Remove the display from the screen it may be in */
        if (screen) {
            screen_remove_display(display);

            /* If the screen is empty, remove it too */
            if (!screen->num_displays) {
                gpu_remove_and_free_screen(screen);
            }
        }

        /* Remove the display from the GPU */
        gpu->displays =
            (nvDisplayPtr)xconfigRemoveListItem((GenericListPtr)gpu->displays,
                                                (GenericListPtr)display);
        gpu->connected_displays &= ~(display->device_mask);
        gpu->num_displays--;
    }

    display_free(display);

} /* gpu_remove_and_free_display() */



/** gpu_remove_displays() ********************************************
 *
 * Removes all displays from the gpu
 *
 **/
static void gpu_remove_displays(nvGpuPtr gpu)
{
    nvDisplayPtr display;

    if (gpu) {
        while (gpu->displays) {
            display = gpu->displays;
            screen_remove_display(display);
            gpu->displays = display->next;
            display_free(display);
        }
        //gpu->connected_displays = 0;
        gpu->num_displays = 0;
    }

} /* gpu_remove_displays() */



/** gpu_add_display_from_server() ************************************
 *
 *  Adds the display with the device mask given to the GPU structure.
 *
 **/
nvDisplayPtr gpu_add_display_from_server(nvGpuPtr gpu,
                                         unsigned int device_mask,
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
    if (!display_add_modelines_from_server(display, err_str)) {
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
    display_free(display);
    return NULL;

} /* gpu_add_display_from_server() */



/** gpu_add_displays_from_server() ***********************************
 *
 * Adds the display devices connected on the GPU to the GPU structure
 *
 **/
static Bool gpu_add_displays_from_server(nvGpuPtr gpu, gchar **err_str)
{
    unsigned int mask;


    /* Clean up the GPU list */
    gpu_remove_displays(gpu);


    // XXX Query connected_displays here...


    /* Add each connected display */
    for (mask = 1; mask; mask <<= 1) {

        if (!(mask & (gpu->connected_displays))) continue;
        
        if (!gpu_add_display_from_server(gpu, mask, err_str)) {
            nv_warning_msg("Failed to add display device 0x%08x to GPU-%d "
                           "'%s'.",
                           mask, NvCtrlGetTargetId(gpu->handle), gpu->name);
            goto fail;
        }
    }

    return TRUE;


    /* Failure case */
 fail:
    gpu_remove_displays(gpu);
    return FALSE;

} /* gpu_add_displays_from_server() */



/** gpu_remove_and_free_screen() *************************************
 *
 * Removes a screen from its GPU and frees it.
 *
 **/
void gpu_remove_and_free_screen(nvScreenPtr screen)
{
    nvGpuPtr gpu;
    nvScreenPtr other;

    if (screen && screen->gpu) {

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
    }

    screen_free(screen);

} /* gpu_remove_and_free_screen() */



/** gpu_remove_screens() *********************************************
 *
 * Removes all screens from a gpu and frees them
 *
 **/
static void gpu_remove_screens(nvGpuPtr gpu)
{
    nvScreenPtr screen;

    if (gpu) {
        while (gpu->screens) {
            screen = gpu->screens;
            gpu->screens = screen->next;
            screen_free(screen);
        }
        gpu->num_screens = 0;
    }

} /* gpu_remove_screens() */



/** gpu_add_screen_from_server() *************************************
 *
 * Adds screen 'screen_id' that is connected to the gpu.
 *
 **/
static int gpu_add_screen_from_server(nvGpuPtr gpu, int screen_id,
                                      gchar **err_str)
{
    Display *display;
    nvScreenPtr screen;
    int val, tmp;
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
        screen_free(screen);
        return TRUE;
    }

    ret = NvCtrlGetAttribute(screen->handle,
                             NV_CTRL_SHOW_SLI_HUD,
                             &tmp);

    screen->sli = (ret == NvCtrlSuccess);


    /* Listen to NV-CONTROL events on this screen handle */
    screen->ctk_event = CTK_EVENT(ctk_event_new(screen->handle));


    /* Query the depth of the screen */
    screen->depth = NvCtrlGetScreenPlanes(screen->handle);


    /* Parse the screen's metamodes (ties displays on the gpu to the screen) */
    if (!screen_add_metamodes(screen, err_str)) {
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
    screen_free(screen);
    return FALSE;

} /* gpu_add_screen_from_server() */



/** gpu_add_screens_from_server() ************************************
 *
 * Queries the list of screens on the gpu.
 *
 */
static Bool gpu_add_screens_from_server(nvGpuPtr gpu, gchar **err_str)
{
    ReturnStatus ret;
    int *pData;
    int len;
    int i;


    /* Clean up the GPU list */
    gpu_remove_screens(gpu);


    /* Query the list of X screens this GPU is driving */
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


    /* Add each X screen */
    for (i = 1; i <= pData[0]; i++) {
        if (!gpu_add_screen_from_server(gpu, pData[i], err_str)) {
            nv_warning_msg("Failed to add screen %d to GPU-%d '%s'.",
                           pData[i], NvCtrlGetTargetId(gpu->handle),
                           gpu->name);
            goto fail;
        }
    }

    return TRUE;


    /* Failure case */
 fail:
    gpu_remove_screens(gpu);
    return FALSE;

} /* gpu_add_screens_from_server() */



/** gpu_add_screenless_modes_to_displays() ***************************
 *
 * Adds fake modes to display devices that have no screens so we
 * can show them on the layout page.
 *
 **/
Bool gpu_add_screenless_modes_to_displays(nvGpuPtr gpu)
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

} /* gpu_add_screenless_modes_to_displays() */



/** gpu_free() *******************************************************
 *
 * Frees memory used by the gpu.
 *
 **/
static void gpu_free(nvGpuPtr gpu)
{
    if (gpu) {
        gpu_remove_screens(gpu);
        gpu_remove_displays(gpu);
        XFree(gpu->name);
        if (gpu->handle) {
            NvCtrlAttributeClose(gpu->handle);
        }
        free(gpu);
    }

} /* gpu_free() */




/*****************************************************************************/
/** LAYOUT FUNCTIONS *********************************************************/
/*****************************************************************************/


/** layout_remove_gpus() *********************************************
 *
 * Removes all GPUs from the layout structure.
 *
 **/
static void layout_remove_gpus(nvLayoutPtr layout)
{
    if (layout) {
        while (layout->gpus) {
            nvGpuPtr gpu = layout->gpus;
            layout->gpus = gpu->next;
            gpu_free(gpu);
        }
        layout->num_gpus = 0;
    }

} /* layout_remove_gpus() */



/** layout_add_gpu_from_server() *************************************
 *
 * Adds a GPU to the layout structure.
 *
 **/
static Bool layout_add_gpu_from_server(nvLayoutPtr layout, unsigned int gpu_id,
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
    if (!gpu_add_displays_from_server(gpu, err_str)) {
        nv_warning_msg("Failed to add displays to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }


    /* Add the X screens to the GPU */
    if (!gpu_add_screens_from_server(gpu, err_str)) {
        nv_warning_msg("Failed to add screens to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }
    

    /* Add fake modes to screenless display devices */
    if (!gpu_add_screenless_modes_to_displays(gpu)) {
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
    gpu_free(gpu);
    return FALSE;

} /* layout_add_gpu_from_server() */



/** layout_add_gpus_from_server() ************************************
 *
 * Adds the GPUs found on the server to the layout structure.
 *
 **/
static int layout_add_gpus_from_server(nvLayoutPtr layout, gchar **err_str)
{
    ReturnStatus ret;
    int ngpus;
    int i;


    /* Clean up the GPU list */
    layout_remove_gpus(layout);


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
        if (!layout_add_gpu_from_server(layout, i, err_str)) {
            nv_warning_msg("Failed to add GPU-%d to layout.", i);
            goto fail;
        }
    }

    return layout->num_gpus;


    /* Failure case */
 fail:
    layout_remove_gpus(layout);
    return 0;

} /* layout_add_gpus_from_server() */



/** layout_free() ****************************************************
 *
 * Frees a layout structure.
 *
 **/
void layout_free(nvLayoutPtr layout)
{
    if (layout) {
        layout_remove_gpus(layout);
        free(layout);
    }

} /* layout_free() */



/** layout_load_from_server() ****************************************
 *
 * Loads layout information from the X server.
 *
 **/
nvLayoutPtr layout_load_from_server(NvCtrlAttributeHandle *handle,
                                    gchar **err_str)
{
    nvLayoutPtr layout = NULL;
    ReturnStatus ret;


    /* Allocate the layout structure */
    layout = (nvLayoutPtr)calloc(1, sizeof(nvLayout));
    if (!layout) goto fail;


    /* Cache the handle for talking to the X server */
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
    if (!layout_add_gpus_from_server(layout, err_str)) {
        nv_warning_msg("Failed to add GPU(s) to layout for display "
                       "configuration page.");
        goto fail;
    }

    return layout;


    /* Failure case */
 fail:
    layout_free(layout);
    return NULL;

} /* layout_load_from_server() */



/** layout_get_a_screen() ********************************************
 *
 * Returns a screen from the layout.  if 'preferred_gpu' is set,
 * screens from that gpu are preferred.  The screen with the lowest
 * number is returned.
 *
 **/
nvScreenPtr layout_get_a_screen(nvLayoutPtr layout, nvGpuPtr preferred_gpu)
{
    nvGpuPtr gpu;
    nvScreenPtr screen = NULL;
    nvScreenPtr other;
    
    if (!layout) return NULL;

    if (preferred_gpu && preferred_gpu->screens) {
        gpu = preferred_gpu;
    } else {
        preferred_gpu = NULL;
        gpu = layout->gpus;
    }

    for (; gpu; gpu = gpu->next) {
        for (other = gpu->screens; other; other = other->next) {
            if (!screen || screen->scrnum > other->scrnum) {
                screen = other;
            }
        }

        /* We found a preferred screen */
        if (gpu == preferred_gpu) break;
    }

    return screen;

} /* layout_get_a_screen() */
