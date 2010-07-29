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
 
#include <stdlib.h> /* malloc */
#include <string.h> /* strlen,  strdup */
#include <unistd.h> /* lseek, close */
#include <errno.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include <gtk/gtk.h>

#include "msg.h"
#include "parse.h"
#include "command-line.h"

#include "ctkdisplayconfig-utils.h"
#include "ctkutils.h"
#include "ctkgpu.h"


static void xconfig_update_buffer(GtkWidget *widget, gpointer user_data);




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
static nvModeLinePtr modeline_parse(nvDisplayPtr display,
                                    const char *modeline_str,
                                    const int broken_doublescan_modelines)
{
    nvModeLinePtr modeline = NULL;
    const char *str = modeline_str;
    char *tmp;
    char *tokens, *nptr;
    double htotal, vtotal, factor;
    gdouble pclk;

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
    str = parse_read_name(str, &(modeline->data.clock), 0);
    if (!str) goto fail;

    /* Read the mode timings */
    str = parse_read_integer(str, &(modeline->data.hdisplay));
    str = parse_read_integer(str, &(modeline->data.hsyncstart));
    str = parse_read_integer(str, &(modeline->data.hsyncend));
    str = parse_read_integer(str, &(modeline->data.htotal));
    str = parse_read_integer(str, &(modeline->data.vdisplay));
    str = parse_read_integer(str, &(modeline->data.vsyncstart));
    str = parse_read_integer(str, &(modeline->data.vsyncend));
    str = parse_read_integer(str, &(modeline->data.vtotal));


    /* Parse modeline flags */
    while ((str = parse_read_name(str, &tmp, 0)) && strlen(tmp)) {
        
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

    modeline->refresh_rate = 0;
    if (display->is_sdi && display->gpu->num_gvo_modes) {
        /* Fetch the SDI refresh rate of the mode from the gvo mode table */
        int i;
        for (i = 0; i < display->gpu->num_gvo_modes; i++) {
            if (display->gpu->gvo_mode_data[i].id &&
                display->gpu->gvo_mode_data[i].name &&
                !strcmp(display->gpu->gvo_mode_data[i].name,
                        modeline->data.identifier)) {
                modeline->refresh_rate = display->gpu->gvo_mode_data[i].rate;
                modeline->refresh_rate /= 1000.0;
                break;
            }
        }
    }

    if (modeline->refresh_rate == 0) {
        /*
         * Calculate the vertical refresh rate of the modeline in Hz;
         * divide by two for double scan modes (if the double scan
         * modeline isn't broken; i.e., already has a correct vtotal), and
         * multiply by two for interlaced modes (so that we report the
         * field rate, rather than the frame rate)
         */

        htotal = (double) modeline->data.htotal;
        vtotal = (double) modeline->data.vtotal;

        /*
         * Use g_ascii_strtod(), so that we do not have to change the locale
         * to "C".
         */
        pclk = g_ascii_strtod((const gchar *)modeline->data.clock,
                              (gchar **)&nptr);

        if ((pclk == 0.0) || !nptr || *nptr != '\0' || ((htotal * vtotal) == 0)) {
            nv_warning_msg("Failed to compute the refresh rate "
                           "for the modeline '%s'", str);
            goto fail;
        }

        modeline->refresh_rate = (pclk * 1000000.0) / (htotal * vtotal);

        factor = 1.0;

        if ((modeline->data.flags & V_DBLSCAN) && !broken_doublescan_modelines) {
            factor *= 0.5;
        }

        if (modeline->data.flags & V_INTERLACE) {
            factor *= 2.0;
        }

        modeline->refresh_rate *= factor;
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
    str = parse_read_name(str, &mode_name, 0);
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



/** modeline_free() *************************************
 *
 * Helper function that frees an nvModeLinePtr and
 * associated memory.
 *
 **/
void modeline_free(nvModeLinePtr m)
{
    if (m->xconfig_name) {
        free(m->xconfig_name);
    }

    if (m->data.identifier) {
        free(m->data.identifier);
    }

    if (m->data.comment) {
        free(m->data.comment);
    }

    if (m->data.clock) {
        free(m->data.clock);
    }

    free(m);
}



/** modelines_match() *************************************
 *
 * Helper function that returns True or False based on whether
 * the modeline arguments match each other.
 *
 **/
Bool modelines_match(nvModeLinePtr modeline1,
                     nvModeLinePtr modeline2)
{
    if (!modeline1 || !modeline2) {
        return FALSE;
    }

    if (!g_ascii_strcasecmp(modeline1->data.clock, modeline2->data.clock) &&
        modeline1->data.hdisplay == modeline2->data.hdisplay &&
        modeline1->data.hsyncstart == modeline2->data.hsyncstart &&
        modeline1->data.hsyncend == modeline2->data.hsyncend &&
        modeline1->data.htotal == modeline2->data.htotal &&
        modeline1->data.vdisplay == modeline2->data.vdisplay &&
        modeline1->data.vsyncstart == modeline2->data.vsyncstart &&
        modeline1->data.vsyncend == modeline2->data.vsyncend &&
        modeline1->data.vtotal == modeline2->data.vtotal &&
        modeline1->data.vscan == modeline2->data.vscan &&
        modeline1->data.flags == modeline2->data.flags &&
        modeline1->data.hskew == modeline2->data.hskew &&
        !g_ascii_strcasecmp(modeline1->data.identifier, 
                            modeline2->data.identifier)) {
            return TRUE;
    } else {
        return FALSE;
    }
} /* modelines_match() */



/** display_has_modeline() *******************************************
 *
 * Helper function that returns TRUE or FALSE based on whether
 * the display passed as argument supports the given modeline.
 *
 **/
Bool display_has_modeline(nvDisplayPtr display,
                          nvModeLinePtr modeline)
{
    nvModeLinePtr m;

    for (m = display->modelines; m; m = m->next) {
         if (modelines_match(m, modeline)) {
            return TRUE;
        }
    }

    return FALSE;

} /* display_has_modeline() */



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
            modeline_free(modeline);
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
    ReturnStatus ret, ret1;
    int major = 0, minor = 0;
    int broken_doublescan_modelines;

    /*
     * check the version of the NV-CONTROL protocol -- versions <=
     * 1.13 had a bug in how they reported double scan modelines
     * (vsyncstart, vsyncend, and vtotal were doubled); determine
     * if this X server has this bug, so that we can use
     * broken_doublescan_modelines to correctly compute the
     * refresh rate.
     */
    broken_doublescan_modelines = 1;

    ret = NvCtrlGetAttribute(display->gpu->handle,
                             NV_CTRL_ATTR_NV_MAJOR_VERSION, &major);
    ret1 = NvCtrlGetAttribute(display->gpu->handle,
                              NV_CTRL_ATTR_NV_MINOR_VERSION, &minor);

    if ((ret == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        ((major > 1) || ((major == 1) && (minor > 13)))) {
        broken_doublescan_modelines = 0;
    }


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

        modeline = modeline_parse(display, str, broken_doublescan_modelines);
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
        xconfigAddListItem((GenericListPtr *)(&display->modelines),
                           (GenericListPtr)modeline);
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


/** renumber_xscreens() **********************************************
 *
 * Ensures that the screens are numbered from 0 to (n-1).
 *
 **/

void renumber_xscreens(nvLayoutPtr layout)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvScreenPtr lowest;
    int scrnum;

    scrnum = 0;
    do {

        /* Find screen w/ lowest # >= current screen index being assigned */
        lowest = NULL;
        for (gpu = layout->gpus; gpu; gpu = gpu->next) {
            for (screen = gpu->screens; screen; screen = screen->next) {
                if ((screen->scrnum >= scrnum) &&
                    (!lowest || (lowest->scrnum > screen->scrnum))) {
                    lowest = screen;
                }
            }
        }

        if (lowest) {
            lowest->scrnum = scrnum;
        }

        /* Assign next screen number */ 
        scrnum++;
    } while (lowest);

} /* renumber_xscreens() */



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

        if (screen->primaryDisplay == display) {
            screen->primaryDisplay = NULL;
        }

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
static Bool screen_add_metamode(nvScreenPtr screen, const char *metamode_str,
                                gchar **err_str)
{
    char *mode_str_itr;
    char *str = NULL;
    const char *tmp;
    char *tokens;
    char *metamode_copy;
    nvMetaModePtr metamode;
    int str_offset = 0;


    if (!screen || !screen->gpu || !metamode_str) goto fail;


    metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
    if (!metamode) goto fail;


    /* Read the MetaMode ID */
    tmp = strstr(metamode_str, "::");
    if (tmp) {

        tokens = strdup(metamode_str);
        if (!tokens) goto fail;

        tokens[ tmp-metamode_str ] = '\0';
        tmp += 2;
        parse_token_value_pairs(tokens, apply_metamode_token,
                                (void *)metamode);
        free(tokens);
        str_offset = tmp - metamode_str;
    } else {
        /* No tokens?  Try the old "ID: METAMODE_STR" syntax */
        const char *read_offset;
        read_offset = parse_read_integer(metamode_str, &(metamode->id));
        metamode->source = METAMODE_SOURCE_NVCONTROL;
        if (*read_offset == ':') {
            read_offset++;
            str_offset = read_offset - metamode_str;
        }
    }

    /* Copy the string so we can split it up */
    metamode_copy = strdup(metamode_str + str_offset);
    if (!metamode_copy) goto fail;

    /* Add the metamode at the end of the screen's metamode list */
    xconfigAddListItem((GenericListPtr *)(&screen->metamodes),
                       (GenericListPtr)metamode);


    /* Split up the metamode into separate modes */
    for (mode_str_itr = strtok(metamode_copy, ",");
         mode_str_itr;
         mode_str_itr = strtok(NULL, ",")) {
        
        nvModePtr     mode;
        unsigned int  device_mask;
        nvDisplayPtr  display;
        const char *orig_mode_str = parse_skip_whitespace(mode_str_itr);
        const char *mode_str;

        /* Parse the display device bitmask from the name */
        mode_str = parse_read_display_name(mode_str_itr, &device_mask);
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
        xconfigAddListItem((GenericListPtr *)(&display->modes),
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
            xconfigAddListItem((GenericListPtr *)(&display->modes),
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
        xconfigRemoveListItem((GenericListPtr *)(&gpu->displays),
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



/** gpu_query_gvo_mode_info() ****************************************
 *
 * Adds GVO mode information to the GPU's gvo mode data table at
 * the given table index.
 *
 **/
static Bool gpu_query_gvo_mode_info(nvGpuPtr gpu, int mode_id, int table_idx)
{
    ReturnStatus ret1, ret2;
    GvoModeData *data;
    int rate;
    char *name;


    if (!gpu || table_idx >= gpu->num_gvo_modes) {
        return FALSE;
    }

    data = &(gpu->gvo_mode_data[table_idx]);

    ret1 = NvCtrlGetDisplayAttribute(gpu->handle,
                                     mode_id,
                                     NV_CTRL_GVIO_VIDEO_FORMAT_REFRESH_RATE,
                                     &(rate));
    
    ret2 = NvCtrlGetStringDisplayAttribute(gpu->handle,
                                           mode_id,
                                           NV_CTRL_STRING_GVIO_VIDEO_FORMAT_NAME,
                                           &(name));

    if ((ret1 == NvCtrlSuccess) && (ret2 == NvCtrlSuccess)) {
        data->id = mode_id;
        data->rate = rate;
        data->name = name;
        return TRUE;
    }

    XFree(name);
    return FALSE;
}


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


    /* Query if this display is an SDI display */
    ret = NvCtrlGetDisplayAttribute(gpu->handle, device_mask,
                                    NV_CTRL_IS_GVO_DISPLAY,
                                    &(display->is_sdi));
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query if display device\n"
                       "0x%08x connected to GPU-%d '%s' is an\n"
                       "SDI device.",
                       device_mask, NvCtrlGetTargetId(gpu->handle),
                       gpu->name);
        display->is_sdi = FALSE;
    }


    /* Load the SDI mode table so we can report accurate refresh rates. */
    if (display->is_sdi && !gpu->gvo_mode_data) {
        unsigned int valid1 = 0;
        unsigned int valid2 = 0;
        unsigned int valid3 = 0;
        NVCTRLAttributeValidValuesRec valid;

        ret = NvCtrlGetValidAttributeValues(gpu->handle,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
            valid1 = 0;
        } else {
            valid1 = valid.u.bits.ints;
        }
        
        ret = NvCtrlGetValidAttributeValues(gpu->handle,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT2,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
            valid2 = 0;
        } else {
            valid2 = valid.u.bits.ints;
        }

        ret = NvCtrlGetValidAttributeValues(gpu->handle,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT3,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
            valid3 = 0;
        } else {
            valid3 = valid.u.bits.ints;
        }

        /* Count the number of valid modes there are */
        gpu->num_gvo_modes = count_number_of_bits(valid1);
        gpu->num_gvo_modes += count_number_of_bits(valid2);
        gpu->num_gvo_modes += count_number_of_bits(valid3);
        if (gpu->num_gvo_modes > 0) {
            gpu->gvo_mode_data = (GvoModeData *)calloc(gpu->num_gvo_modes,
                                                       sizeof(GvoModeData));
        }
        if (!gpu->gvo_mode_data) {
            gpu->num_gvo_modes = 0;
        } else {
            // Gather all the bits and dump them into the array
            int idx = 0; // Index into gvo_mode_data.
            int id = 0;  // Mode ID
            while (valid1) {
                if (valid1 & 1) {
                    if (gpu_query_gvo_mode_info(gpu, id, idx)) {
                        idx++;
                    }
                }
                valid1 >>= 1;
                id++;
            }
            while (valid2) {
                if (valid2 & 1) {
                    if (gpu_query_gvo_mode_info(gpu, id, idx)) {
                        idx++;
                    }
                }
                valid2 >>= 1;
                id++;
            }
            while (valid3) {
                if (valid3 & 1) {
                    if (gpu_query_gvo_mode_info(gpu, id, idx)) {
                        idx++;
                    }
                }
                valid3 >>= 1;
                id++;
            }
        }
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
    xconfigAddListItem((GenericListPtr *)(&gpu->displays),
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
        
        xconfigRemoveListItem((GenericListPtr *)(&gpu->screens),
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
    gchar *primary_str = NULL;

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


    /* See if the screen supports dynamic twinview */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_DYNAMIC_TWINVIEW, &val);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query Dynamic TwinView for "
                                   "screen %d.",
                                   screen_id);
        nv_warning_msg(*err_str);
        goto fail;
    }
    screen->dynamic_twinview = val ? TRUE : FALSE;


    /* See if the screen is set to not scanout */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_NO_SCANOUT, &val);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query NoScanout for "
                                   "screen %d.",
                                   screen_id);
        nv_warning_msg(*err_str);
        goto fail;
    }
    screen->no_scanout = (val == NV_CTRL_NO_SCANOUT_ENABLED) ? TRUE : FALSE;


    /* XXX Currently there is no support for screens that are scanning
     *     out but have TwinView disabled.
     */
    if (!screen->dynamic_twinview && !screen->no_scanout) {
        *err_str = g_strdup_printf("nvidia-settings currently does not "
                                   "support scanout screens (%d) that have "
                                   "dynamic twinview disabled.",
                                   screen_id);
        nv_warning_msg(*err_str);
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

    /* Initialize the virtual X screen size */
    screen->dim[W] = NvCtrlGetScreenWidth(screen->handle);
    screen->dim[H] = NvCtrlGetScreenHeight(screen->handle);


    /* Parse the screen's metamodes (ties displays on the gpu to the screen) */
    if (!screen->no_scanout) {
        if (!screen_add_metamodes(screen, err_str)) {
            nv_warning_msg("Failed to add metamodes to screen %d (on GPU-%d).",
                           screen_id, NvCtrlGetTargetId(gpu->handle));
            goto fail;
        }
    
        /* Query & parse the screen's primary display */
        screen->primaryDisplay = NULL;
        ret = NvCtrlGetStringDisplayAttribute
            (screen->handle,
             0,
             NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER,
             &primary_str);
        
        if (ret == NvCtrlSuccess) {
            nvDisplayPtr d;
            unsigned int  device_mask;
            
            /* Parse the device mask */
            parse_read_display_name(primary_str, &device_mask);
            
            /* Find the matching primary display */
            for (d = gpu->displays; d; d = d->next) {
                if (d->screen == screen &&
                    d->device_mask & device_mask) {
                    screen->primaryDisplay = d;
                    break;
                }
            }
        }
    }


    /* Add the screen at the end of the gpu's screen list */
    xconfigAddListItem((GenericListPtr *)(&gpu->screens),
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
            XFree(pData);
            goto fail;
        }
    }

    XFree(pData);
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
        g_free(gpu->pci_bus_id);
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

    get_bus_related_info(gpu->handle, NULL, &(gpu->pci_bus_id));


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

    ret = NvCtrlGetAttribute(gpu->handle, NV_CTRL_DEPTH_30_ALLOWED,
                             &(gpu->allow_depth_30));

    if (ret != NvCtrlSuccess) {
        gpu->allow_depth_30 = FALSE;
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
    xconfigAddListItem((GenericListPtr *)(&layout->gpus),
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




/*****************************************************************************/
/** XCONFIG FUNCTIONS ********************************************************/
/*****************************************************************************/


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

static int save_xconfig_file(SaveXConfDlg *dlg,
                             gchar *filename, char *buf, mode_t mode)
{
    gchar *backup_filename = NULL;
    FILE *fp = NULL;
    size_t size;
    gchar *err_msg = NULL;
    struct stat st;

    int ret = 0;


    if (!buf || !filename) goto done;

    size = strlen(buf) ;

    /* Backup any existing file */
    if ((access(filename, F_OK) == 0)) {

        /* Verify the file-write permission */
        if ((access(filename, W_OK) != 0)) {
            err_msg =
              g_strdup_printf("You do not have adequate permission to"
              " open the existing X configuration file '%s' for writing.",
              filename);

            /* Verify the user permissions */
            if (stat(filename, &st) == 0) {
                if ((getuid() != 0) && (st.st_uid == 0) &&
                    !(st.st_mode & (S_IWGRP | S_IWOTH)))
                    err_msg = g_strconcat(err_msg, " You must be 'root'"
                    " to modify the file.", NULL);
            }
            goto done;
        }

        backup_filename = g_strdup_printf("%s.backup", filename);
        nv_info_msg("", "X configuration file '%s' already exists, "
                    "backing up file as '%s'", filename,
                    backup_filename);
        
        /* Delete any existing backup file */
        if (access(backup_filename, F_OK) == 0) {

            if (unlink(backup_filename) != 0) {
                err_msg =
                    g_strdup_printf("Unable to remove old X config backup "
                                    "file '%s'.",
                                    backup_filename);
                goto done;
            }
        }

        /* Make the current x config file the backup */
        if (rename(filename, backup_filename)) {
                err_msg =
                    g_strdup_printf("Unable to create new X config backup "
                                    "file '%s'.",
                                    backup_filename);
            goto done;            
        }
    }

    /* Write out the X config file */
    fp = fopen(filename, "w");
    if (!fp) {
        err_msg =
            g_strdup_printf("Unable to open X config file '%s' for writing.",
                            filename);
        goto done;
    }
    fprintf(fp, "%s", buf);

    ret = 1;
    
 done:
    /* Display any errors that might have occured */
    if (err_msg) {
        ctk_display_error_msg(ctk_get_parent_window(GTK_WIDGET(dlg->parent)),
                              err_msg);
        g_free(err_msg);
    }

    if (fp) fclose(fp);
    g_free(backup_filename);
    return ret;
    
} /* save_xconfig_file() */



/** get_non_regular_file_type_description() **************************
 *
 * Returns a string that describes the mode type of a file.
 *
 */
static const char *get_non_regular_file_type_description(mode_t mode)
{
    if (S_ISDIR(mode)) {
        return "directory";
    } else if (S_ISCHR(mode)) {
        return "character device file";
    } else if (S_ISBLK(mode)) {
        return "block device file";
    } else if (S_ISFIFO(mode)) {
        return "FIFO";
    } else if (S_ISLNK(mode)) {
        return "symbolic link";
    } else if (S_ISSOCK(mode)) {
        return "socket";
    } else if (!S_ISREG(mode)) {
        return "non-regular file";
    }

    return NULL;

} /* get_non_regular_file_type_description() */



/**  update_xconfig_save_buffer() ************************************
 *
 * Updates the "preview" buffer to hold the right contents based on
 * how the user wants the X config file to be generated (and what is
 * possible.)
 *
 * Also updates the state of the "Merge" checkbox in the case where
 * the named file can/cannot be parsed as a valid X config file.
 *
 */
static void update_xconfig_save_buffer(SaveXConfDlg *dlg)
{
    const gchar *filename;

    XConfigPtr xconfCur = NULL;
    XConfigPtr xconfGen = NULL;
    XConfigError xconfErr;

    char *tmp_filename;
    int tmp_fd;
    struct stat st;
    void *buf;
    GtkTextIter buf_start, buf_end;

    gboolean merge;
    gboolean merged;
    gboolean mergeable = FALSE;

    gchar *err_msg = NULL;



    /* Get how the user wants to generate the X config file */
    merge = gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(dlg->btn_xconfig_merge));

    filename = gtk_entry_get_text(GTK_ENTRY(dlg->txt_xconfig_file));


    /* Assume we can save until we find out otherwise */
    gtk_dialog_set_response_sensitive(GTK_DIALOG(dlg->dlg_xconfig_save),
                                      GTK_RESPONSE_ACCEPT,
                                      TRUE);


    /* Find out if the file is mergable */
    if (filename && (stat(filename, &st) == 0)) {
        const char *non_regular_file_type_description =
            get_non_regular_file_type_description(st.st_mode);
        const char *test_filename;

        /* Make sure this is a regular file */
        if (non_regular_file_type_description) {
            err_msg = g_strdup_printf("Invalid file '%s': File exits but is a "
                                      "%s!",
                                      filename,
                                      non_regular_file_type_description);
            gtk_widget_set_sensitive(dlg->btn_xconfig_merge, FALSE);
            gtk_dialog_set_response_sensitive(GTK_DIALOG(dlg->dlg_xconfig_save),
                                              GTK_RESPONSE_ACCEPT,
                                              FALSE);
            goto fail;
        }

        /* Must be able to open the file */
        test_filename = xconfigOpenConfigFile(filename, NULL);
        if (!test_filename || strcmp(test_filename, filename)) {
            xconfigCloseConfigFile();

        } else {
            GenerateOptions gop;

            /* Must be able to parse the file as an X config file */
            xconfErr = xconfigReadConfigFile(&xconfCur);
            xconfigCloseConfigFile();
            if ((xconfErr != XCONFIG_RETURN_SUCCESS) || !xconfCur) {
                /* If we failed to parse the config file, we should not
                 * allow a merge.
                 */
                err_msg = g_strdup_printf("Failed to parse existing X "
                                          "config file '%s'!",
                                          filename);
                ctk_display_warning_msg
                    (ctk_get_parent_window(GTK_WIDGET(dlg->parent)), err_msg);

                xconfCur = NULL;

            } else {

                /* Sanitize the X config file */
                xconfigGenerateLoadDefaultOptions(&gop);
                xconfigGetXServerInUse(&gop);
                
                if (!xconfigSanitizeConfig(xconfCur, NULL, &gop)) {
                    err_msg = g_strdup_printf("Failed to sanitize existing X "
                                              "config file '%s'!",
                                              filename);
                    ctk_display_warning_msg
                        (ctk_get_parent_window(GTK_WIDGET(dlg->parent)),
                         err_msg);
                    
                    xconfigFreeConfig(&xconfCur);
                    xconfCur = NULL;
                } else {
                    mergeable = TRUE;
                }
            }

            /* If we're not actualy doing a merge, close the file */
            if (!merge && xconfCur) {
                xconfigFreeConfig(&xconfCur);
            }
        }
    }


    /* If we have to merge but we cannot, prevent user from saving */
    if (merge && !xconfCur && !dlg->merge_toggleable) {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(dlg->dlg_xconfig_save),
                                          GTK_RESPONSE_ACCEPT,
                                          FALSE);
        goto fail;
    }

    merge = (merge && xconfCur);


    /* Generate the X config file */
    xconfGen = dlg->xconf_gen_func(xconfCur, merge, &merged,
                                   dlg->callback_data);
    if (!xconfGen) {
        err_msg = g_strdup_printf("Failed to generate X config file!");
        goto fail;
    }

    /* Update merge status */
    g_signal_handlers_block_by_func
        (G_OBJECT(dlg->btn_xconfig_merge),
         G_CALLBACK(xconfig_update_buffer), (gpointer) dlg);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(dlg->btn_xconfig_merge), merged);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(dlg->btn_xconfig_merge),
         G_CALLBACK(xconfig_update_buffer), (gpointer) dlg);
    
    gtk_widget_set_sensitive(dlg->btn_xconfig_merge,
                             (dlg->merge_toggleable && mergeable) ?
                             TRUE : FALSE);
    

    /* We're done with the user's X config, so do some cleanup,
     * but make sure to handle the case where the generation
     * function modifies the user's X config structure instead
     * of generating a new one.
     */
    if (xconfGen == xconfCur) {
        xconfCur = NULL;
    } else {
        xconfigFreeConfig(&xconfCur);
    }


    /* Update the X config banner */
    update_banner(xconfGen);


    /* Setup the X config file preview buffer by writing to a temp file */
    tmp_filename = g_strdup_printf("/tmp/.xconfig.tmp.XXXXXX");
    tmp_fd = mkstemp(tmp_filename);
    if (!tmp_fd) {
        err_msg = g_strdup_printf("Failed to create temp X config file '%s' "
                                  "for display.",
                                  tmp_filename);
        g_free(tmp_filename);
        goto fail;
    }
    xconfigWriteConfigFile(tmp_filename, xconfGen);
    xconfigFreeConfig(&xconfGen);

    lseek(tmp_fd, 0, SEEK_SET);
    fstat(tmp_fd, &st);
    buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, tmp_fd, 0);

    /* Clear the GTK buffer */
    gtk_text_buffer_get_bounds
        (GTK_TEXT_BUFFER(dlg->buf_xconfig_save), &buf_start,
         &buf_end);
    gtk_text_buffer_delete
        (GTK_TEXT_BUFFER(dlg->buf_xconfig_save), &buf_start,
         &buf_end);

    /* Set the new GTK buffer contents */
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(dlg->buf_xconfig_save),
                             buf, st.st_size);
    munmap(buf, st.st_size);
    close(tmp_fd);
    remove(tmp_filename);
    g_free(tmp_filename);

    return;


 fail:
    /* Clear the GTK buffer */
    gtk_text_buffer_get_bounds
        (GTK_TEXT_BUFFER(dlg->buf_xconfig_save), &buf_start,
         &buf_end);
    gtk_text_buffer_delete
        (GTK_TEXT_BUFFER(dlg->buf_xconfig_save), &buf_start,
         &buf_end);

    if (err_msg) {
        ctk_display_warning_msg(ctk_get_parent_window(GTK_WIDGET(dlg->parent)),
                                err_msg);
        g_free(err_msg);
    }

    if (xconfGen) {
        xconfigFreeConfig(&xconfGen);
    }

    if (xconfCur) {
        xconfigFreeConfig(&xconfCur);
    }

    return;

} /* update_xconfig_save_buffer() */



/** xconfig_preview_clicked() ****************************************
 *
 * Called when the user clicks on the "Preview" button of the
 * X config save dialog.
 *
 **/

static void xconfig_preview_clicked(GtkWidget *widget, gpointer user_data)
{
    SaveXConfDlg *dlg = (SaveXConfDlg *)user_data;
    gboolean show = !GTK_WIDGET_VISIBLE(dlg->box_xconfig_save);

    if (show) {
        gtk_widget_show_all(dlg->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_xconfig_save),
                                 TRUE);
        gtk_widget_set_size_request(dlg->txt_xconfig_save, 450, 350);
        gtk_button_set_label(GTK_BUTTON(dlg->btn_xconfig_preview),
                             "Hide Preview...");
    } else {
        gtk_widget_hide(dlg->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_xconfig_save), FALSE);
        gtk_button_set_label(GTK_BUTTON(dlg->btn_xconfig_preview),
                             "Show Preview...");
    }

} /* xconfig_preview_clicked() */



/** xconfig_update_buffer() ******************************************
 *
 * Called when the user selects a new X config filename.
 *
 **/

static void xconfig_update_buffer(GtkWidget *widget, gpointer user_data)
{
    SaveXConfDlg *dlg = (SaveXConfDlg *)user_data;

    update_xconfig_save_buffer(dlg);

} /* xconfig_update_buffer() */



/** xconfig_file_clicked() *******************************************
 *
 * Called when the user clicks on the "Browse..." button of the
 * X config save dialog.
 *
 **/

static void xconfig_file_clicked(GtkWidget *widget, gpointer user_data)
{
    SaveXConfDlg *dlg = (SaveXConfDlg *)user_data;
    const gchar *filename =
        gtk_entry_get_text(GTK_ENTRY(dlg->txt_xconfig_file));
    gint result;


    /* Ask user for a filename */
    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_xconfig_file),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_file_selection_set_filename
        (GTK_FILE_SELECTION(dlg->dlg_xconfig_file), filename);
    
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_xconfig_file));
    gtk_widget_hide(dlg->dlg_xconfig_file);
    
    switch (result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
        filename = gtk_file_selection_get_filename
            (GTK_FILE_SELECTION(dlg->dlg_xconfig_file));

        gtk_entry_set_text(GTK_ENTRY(dlg->txt_xconfig_file), filename);

        update_xconfig_save_buffer(dlg);
        break;
    default:
        return;
    }

} /* xconfig_file_clicked() */



/** run_save_xconfig_dialog() ****************************************
 *
 * run_save_xconfig_dialog() - Takes care of running the "Save X
 * Configuration File" dialog.  Generates the X config file by
 * calling the registered callback and takes care of keeping
 * track of the requested filename etc.
 *
 **/

void run_save_xconfig_dialog(SaveXConfDlg *dlg)
{
    void *buf;
    GtkTextIter buf_start, buf_end;
    gchar *filename;
    const gchar *tmp_filename;
    struct stat st;

    gint result;


    /* Generate the X config file save buffer */
    update_xconfig_save_buffer(dlg);


    /* Show the save dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_xconfig_save),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_widget_hide(dlg->box_xconfig_save);
    gtk_window_resize(GTK_WINDOW(dlg->dlg_xconfig_save), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_xconfig_save),
                             FALSE);
    gtk_button_set_label(GTK_BUTTON(dlg->btn_xconfig_preview),
                         "Show preview...");
    gtk_widget_show(dlg->dlg_xconfig_save);
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_xconfig_save));
    gtk_widget_hide(dlg->dlg_xconfig_save);
    
    
    /* Handle user's response */
    switch (result)
    {
    case GTK_RESPONSE_ACCEPT:

        /* Get the filename to write to */
        tmp_filename = gtk_entry_get_text(GTK_ENTRY(dlg->txt_xconfig_file));
        filename = tilde_expansion(tmp_filename);

        if (!filename) {
            nv_error_msg("Failed to get X configuration filename!");
            break;
        }

        /* If the file exists, make sure it is a regular file */
        if (stat(filename, &st) == 0) {
            const char *non_regular_file_type_description =
                get_non_regular_file_type_description(st.st_mode);

            if (non_regular_file_type_description) {
                nv_error_msg("Failed to write X configuration to file '%s': "
                             "File exists but is a %s.",
                             filename,
                             non_regular_file_type_description);
                break;
            }
        }

        /* Get the buffer to write */
        gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(dlg->buf_xconfig_save),
                                   &buf_start, &buf_end);
        buf = (void *) gtk_text_buffer_get_text
            (GTK_TEXT_BUFFER(dlg->buf_xconfig_save), &buf_start,
             &buf_end, FALSE);
        if (!buf) {
            nv_error_msg("Failed to read X configuration buffer!");
            break;
        }

        /* Save the X config file */
        nv_info_msg("", "Writing X config file '%s'", filename);
        save_xconfig_file(dlg, filename, (char *)buf, 0644);
        g_free(buf);
        g_free(filename);
        break;
        
    case GTK_RESPONSE_REJECT:
    default:
        /* do nothing. */
        break;
    }

} /* run_save_xconfig_dialog() */



/** create_save_xconfig_dialog() *************************************
 *
 * Creates the "Save X Configuration" button widget
 *
 **/

SaveXConfDlg *create_save_xconfig_dialog(GtkWidget *parent,
                                         Bool merge_toggleable,
                                         generate_xconfig_callback xconf_gen_func,
                                         void *callback_data)
{
    SaveXConfDlg *dlg;
    GtkWidget *hbox;
    GtkWidget *hbox2;
    gchar *filename;
    const char *tmp_filename;

    dlg = (SaveXConfDlg *) malloc (sizeof(SaveXConfDlg));
    if (!dlg) return NULL;

    dlg->parent = parent;
    dlg->xconf_gen_func = xconf_gen_func;
    dlg->merge_toggleable = merge_toggleable;
    dlg->callback_data = callback_data;

    /* Setup the default filename */
    tmp_filename = xconfigOpenConfigFile(NULL, NULL);
    if (tmp_filename) {
        filename = g_strdup(tmp_filename);
    } else {
        filename = g_strdup("");
    }
    xconfigCloseConfigFile();

    if (!filename) {
        free(dlg);
        return NULL;
    }

    /* Create the dialog */
    dlg->dlg_xconfig_save = gtk_dialog_new_with_buttons
        ("Save X Configuration",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(parent))),
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT |
         GTK_DIALOG_NO_SEPARATOR,
         GTK_STOCK_SAVE,
         GTK_RESPONSE_ACCEPT,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_xconfig_save),
                                    GTK_RESPONSE_REJECT);

    gtk_dialog_set_has_separator(GTK_DIALOG(dlg->dlg_xconfig_save), TRUE);

    /* Create the preview button */
    dlg->btn_xconfig_preview = gtk_button_new();
    g_signal_connect(G_OBJECT(dlg->btn_xconfig_preview), "clicked",
                     G_CALLBACK(xconfig_preview_clicked),
                     (gpointer) dlg);

    /* Create the preview text window & buffer */
    dlg->txt_xconfig_save = gtk_text_view_new();
    gtk_text_view_set_left_margin
        (GTK_TEXT_VIEW(dlg->txt_xconfig_save), 5);

    dlg->buf_xconfig_save = gtk_text_buffer_new(NULL);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW(dlg->txt_xconfig_save),
                             GTK_TEXT_BUFFER(dlg->buf_xconfig_save));

    dlg->scr_xconfig_save = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type
        (GTK_SCROLLED_WINDOW(dlg->scr_xconfig_save), GTK_SHADOW_IN);

    /* Create the filename text entry */
    dlg->txt_xconfig_file = gtk_entry_new();
    gtk_widget_set_size_request(dlg->txt_xconfig_file, 300, -1);
    gtk_entry_set_text(GTK_ENTRY(dlg->txt_xconfig_file), filename);
    g_signal_connect(G_OBJECT(dlg->txt_xconfig_file), "activate",
                     G_CALLBACK(xconfig_update_buffer),
                     (gpointer) dlg);

    /* Create the filename browse button */
    dlg->btn_xconfig_file = gtk_button_new_with_label("Browse...");
    g_signal_connect(G_OBJECT(dlg->btn_xconfig_file), "clicked",
                     G_CALLBACK(xconfig_file_clicked),
                     (gpointer) dlg);
    dlg->dlg_xconfig_file =
        gtk_file_selection_new("Please select the X configuration file");

    /* Create the merge checkbox */
    dlg->btn_xconfig_merge =
        gtk_check_button_new_with_label("Merge with existing file.");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->btn_xconfig_merge),
                                 TRUE);
    gtk_widget_set_sensitive(dlg->btn_xconfig_merge, merge_toggleable);
    g_signal_connect(G_OBJECT(dlg->btn_xconfig_merge), "toggled",
                     G_CALLBACK(xconfig_update_buffer),
                     (gpointer) dlg);


    /* Pack the preview button */
    hbox = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), dlg->btn_xconfig_preview,
                       FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg->dlg_xconfig_save)->vbox),
                       hbox, FALSE, FALSE, 5);

    /* Pack the preview window */
    hbox = gtk_hbox_new(TRUE, 0);

    gtk_container_add(GTK_CONTAINER(dlg->scr_xconfig_save),
                      dlg->txt_xconfig_save);
    gtk_box_pack_start(GTK_BOX(hbox), dlg->scr_xconfig_save, TRUE, TRUE, 5);
    gtk_box_pack_start
        (GTK_BOX(GTK_DIALOG(dlg->dlg_xconfig_save)->vbox),
         hbox,
         TRUE, TRUE, 0);
    dlg->box_xconfig_save = hbox;
    
    /* Pack the filename text entry and browse button */
    hbox = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 5);
    
    gtk_box_pack_end(GTK_BOX(hbox2), dlg->btn_xconfig_file, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox2), dlg->txt_xconfig_file, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), hbox2, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg->dlg_xconfig_save)->vbox),
                       hbox,
                       FALSE, FALSE, 5);
    
    /* Pack the merge checkbox */
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg->dlg_xconfig_save)->vbox),
                       dlg->btn_xconfig_merge,
                       FALSE, FALSE, 5);
    
    gtk_widget_show_all(GTK_DIALOG(dlg->dlg_xconfig_save)->vbox);
    
    return dlg;

} /* create_save_xconfig_button() */
