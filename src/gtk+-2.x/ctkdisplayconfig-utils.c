/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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
#include "common-utils.h"

#include "ctkdisplayconfig-utils.h"
#include "ctkutils.h"
#include "ctkgpu.h"


static void xconfig_update_buffer(GtkWidget *widget, gpointer user_data);
static gchar *display_pick_config_name(nvDisplayPtr display, int be_generic);
static Bool screen_check_metamodes(nvScreenPtr screen);



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

    /* Source */
    } else if (!strcasecmp("source", token)) {
        if (!value || !strlen(value)) {
            nv_warning_msg("MetaMode 'source' token requires a value!");
        } else if (!strcasecmp("xconfig", value)) {
            metamode->source = METAMODE_SOURCE_XCONFIG;
        } else if (!strcasecmp("implicit", value)) {
            metamode->source = METAMODE_SOURCE_IMPLICIT;
        } else if (!strcasecmp("nv-control", value)) {
            metamode->source = METAMODE_SOURCE_NVCONTROL;
        } else if (!strcasecmp("randr", value)) {
            metamode->source = METAMODE_SOURCE_RANDR;
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
 * Modifies the GdkRectangle structure (pointed to by data) with
 * information from the token-value pair given.  Currently accepts
 * position and width/height data.
 *
 **/
void apply_screen_info_token(char *token, char *value, void *data)
{
    GdkRectangle *screen_info = (GdkRectangle *)data;

    if (!screen_info || !token || !strlen(token)) {
        return;
    }

    if (!strcasecmp("x", token)) {
        screen_info->x = atoi(value);

    } else if (!strcasecmp("y", token)) {
        screen_info->y = atoi(value);

    } else if (!strcasecmp("width", token)) {
        screen_info->width = atoi(value);

    } else if (!strcasecmp("height", token)) {
        screen_info->height = atoi(value);

    /* Unknown token */
    } else {
        nv_warning_msg("Unknown screen info token value pair: %s=%s",
                       token, value);
    }
}




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
                                    nvGpuPtr gpu,
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
    if (display->is_sdi && gpu->num_gvo_modes) {
        /* Fetch the SDI refresh rate of the mode from the gvo mode table */
        int i;
        for (i = 0; i < gpu->num_gvo_modes; i++) {
            if (gpu->gvo_mode_data[i].id &&
                gpu->gvo_mode_data[i].name &&
                !strcmp(gpu->gvo_mode_data[i].name,
                        modeline->data.identifier)) {
                modeline->refresh_rate = gpu->gvo_mode_data[i].rate;
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


/*!
 * Clamps the given dimensions to be no smaller than the mode's viewPortIn
 *
 * \param[in, out]  rect  The GdkRectangle to clamp.
 * \param[in]       mode  The mode to clamp against.
 */
void clamp_rect_to_viewportin(GdkRectangle *rect, const nvMode *mode)
{
    if (rect->width < mode->viewPortIn.width) {
        rect->width = mode->viewPortIn.width;
    }
    if (rect->height < mode->viewPortIn.height) {
        rect->height = mode->viewPortIn.height;
    }
}



/*!
 * Clamps the mode's panning domain to the mode's viewPortIn dimensions
 *
 * \param[in, out]  mode  The mode who's panning to clamp.
 */
void clamp_mode_panning(nvModePtr mode)
{
    clamp_rect_to_viewportin(&(mode->pan), mode);
}



/*!
 * Fills a rectangle struct with both position and size information of the
 * given mode's viewPortIn.
 *
 * \param[in]       mode  The mode to return information for.
 * \param[in, out]  rect  The GdkRectangle structure to populate.
 */
void get_viewportin_rect(const nvMode *mode, GdkRectangle *rect)
{
    rect->x = mode->pan.x;
    rect->y = mode->pan.y;
    rect->width = mode->viewPortIn.width;
    rect->height = mode->viewPortIn.height;
}



void mode_set_modeline(nvModePtr mode,
                       nvModeLinePtr modeline,
                       const nvSize *providedViewPortIn,
                       const GdkRectangle *providedViewPortOut)
{
    int width;
    int height;
    Bool panning_modified;

    /* Figure out what dimensions to use */
    if (providedViewPortIn) {
        width = providedViewPortIn->width;
        height = providedViewPortIn->height;
    } else if (modeline) {
        width = modeline->data.hdisplay;
        height = modeline->data.vdisplay;
    } else {
        /* NULL modeline given (display is being turned off), use a default
         * resolution to show the display.
         */
        if (mode->display->modelines) {
            // XXX assumes that the first modeline in the display's list is the
            //     default (nvidia-auto-select).
            width = mode->display->modelines->data.hdisplay;
            height = mode->display->modelines->data.vdisplay;
        } else {
            /* display has no modelines, 800x600 seems reasonable */
            width = 800;
            height = 600;
        }
    }

    /* Reset the viewPortOut to match the full visible size of the modeline */
    // XXX Only do this if viewport out has not been tweaked?
    // XXX - Should we do any clamping?
    if (providedViewPortOut) {
        mode->viewPortOut = *providedViewPortOut;
    } else {
        mode->viewPortOut.x = 0;
        mode->viewPortOut.y = 0;
        mode->viewPortOut.width = width;
        mode->viewPortOut.height = height;
    }

    /* Oriented the dimensions to use for the viewPortIn and Panning */
    if ((mode->rotation == ROTATION_90) ||
        (mode->rotation == ROTATION_270)) {
        int temp = width;
        width = height;
        height = temp;
    }

    /* XXX Later, keep a flag in nvModePtr to track if the panning has
     * been modified */
    panning_modified =
        (mode->pan.width != mode->viewPortIn.width) ||
        (mode->pan.height != mode->viewPortIn.height);

    /* XXX Only set this if the user has not modified viewPortIn... */
    {
        mode->viewPortIn.width = width;
        mode->viewPortIn.height = height;
        /* Panning domain must include viewPortIn */
        clamp_mode_panning(mode);
    }

    /* Only set this if the user has not modified panning... */
    if (!panning_modified) {
        mode->pan.width = width;
        mode->pan.height = height;
    }

    mode->modeline = modeline;
}


/*!
 * Sets the mode to have the specified rotation
 *
 * \param[in]  mode      The mode to modify
 * \param[in]  rotation  The rotation to set
 *
 * \return  TRUE if a new rotation was set, FALSE if the mode was already
 *          set to the rotation given.
 */
Bool mode_set_rotation(nvModePtr mode, Rotation rotation)
{
    Bool old_is_horiz;
    Bool new_is_horiz;
    int tmp;

    if (mode->rotation == rotation) {
        return FALSE;
    }

    /* Set the new rotation orientation and swap if we need to*/
    old_is_horiz = ((mode->rotation == ROTATION_0) ||
                    (mode->rotation == ROTATION_180)) ? TRUE : FALSE;

    new_is_horiz = ((rotation == ROTATION_0) ||
                    (rotation == ROTATION_180)) ? TRUE : FALSE;

    mode->rotation = rotation;

    if (old_is_horiz != new_is_horiz) {
        tmp = mode->viewPortIn.width;
        mode->viewPortIn.width = mode->viewPortIn.height;
        mode->viewPortIn.height = tmp;

        tmp = mode->pan.width;
        mode->pan.width = mode->pan.height;
        mode->pan.height = tmp;
    }

    /* Mark mode as being modified */
    if (mode->metamode) {
        mode->metamode->source = METAMODE_SOURCE_NVCONTROL;
    }

    return TRUE;
}



/** apply_mode_attribute_token() *************************************
 *
 * Modifies the nvMode structure (pointed to by data) with
 * information from the token-value pair given.  Currently accepts
 * stereo (mode) data.
 *
 * Unknown token and/or values are silently ignored.
 *
 **/
static void apply_mode_attribute_token(char *token, char *value, void *data)
{
    nvModePtr mode = (nvModePtr)data;

    if (!mode || !token || !strlen(token)) {
        return;
    }

    /* stereo */
    if (!strcasecmp("stereo", token)) {
        if (!strcasecmp("PassiveLeft", value)) {
            mode->passive_stereo_eye = PASSIVE_STEREO_EYE_LEFT;
        } else if (!strcasecmp("PassiveRight", value)) {
            mode->passive_stereo_eye = PASSIVE_STEREO_EYE_RIGHT;
        }

    /* ViewPortIn */
    } else if (!strcasecmp("viewportin", token)) {
        parse_read_integer_pair(value, 'x',
                                &(mode->viewPortIn.width),
                                &(mode->viewPortIn.height));

    /* ViewPortOut */
    } else if (!strcasecmp("viewportout", token)) {
        const char *str;

        str = parse_read_integer_pair(value, 'x',
                                      &(mode->viewPortOut.width),
                                      &(mode->viewPortOut.height));

        str = parse_read_integer_pair(str, 0,
                                      &(mode->viewPortOut.x),
                                      &(mode->viewPortOut.y));
    }

    /* Rotation */
    if (!strcasecmp("rotation", token)) {
        if (!strcasecmp("left", value) ||
            !strcasecmp("CCW", value) ||
            !strcasecmp("90", value)) {
            mode->rotation = ROTATION_90;
        } else if (!strcasecmp("invert", value) ||
                   !strcasecmp("inverted", value) ||
                   !strcasecmp("180", value)) {
            mode->rotation = ROTATION_180;
        } else if (!strcasecmp("right", value) ||
                   !strcasecmp("CW", value) ||
                   !strcasecmp("270", value)) {
            mode->rotation = ROTATION_270;
        }
    }

    /* Reflection */
    if (!strcasecmp("reflection", token)) {
        if (!strcasecmp("x", value)) {
            mode->reflection = REFLECTION_X;
        } else if (!strcasecmp("y", value)) {
            mode->reflection = REFLECTION_Y;
        } else if (!strcasecmp("xy", value)) {
            mode->reflection = REFLECTION_XY;
        }
    }

} /* apply_mode_attribute_token() */



/** mode_parse() *****************************************************
 *
 * Converts a mode string (dpy specific part of a metamode) to a
 * mode structure that the display configuration page can use.
 *
 * Mode strings have the following format:
 *
 *   "mode_name +X+Y @WxH {token=value, ...}"
 *
 **/

nvModePtr mode_parse(nvDisplayPtr display, const char *mode_str)
{
    nvModePtr   mode;
    char       *mode_name; /* Modeline reference name */
    const char *str = mode_str;
    nvModeLinePtr modeline;



    if (!str || !display) return NULL;


    /* Allocate a Mode structure */
    mode = (nvModePtr)calloc(1, sizeof(nvMode));
    if (!mode) return NULL;

    mode->display = display;

    /* Set default values */
    mode->rotation = ROTATION_0;
    mode->reflection = REFLECTION_NONE;
    mode->passive_stereo_eye = PASSIVE_STEREO_EYE_NONE;
    mode->position_type = CONF_ADJ_ABSOLUTE;


    /* Read the mode name */
    str = parse_read_name(str, &mode_name, 0);
    if (!str || !mode_name) goto fail;


    /* Find the display's modeline that matches the given mode name */
    modeline = display->modelines;
    while (modeline) {
        if (!strcmp(mode_name, modeline->data.identifier)) {
            break;
        }
        modeline = modeline->next;
    }

    /* If we can't find a matching modeline, set the NULL mode. */
    if (!modeline) {
        if (strcmp(mode_str, "NULL")) {
            nv_warning_msg("Mode name '%s' does not match any modelines for "
                           "display device '%s' in modeline '%s'.",
                           mode_name, display->logName, mode_str);
        }
        free(mode_name);

        mode_set_modeline(mode,
                          NULL /* modeline */,
                          NULL /* viewPortIn */,
                          NULL /* viewPortOut */);

        return mode;
    }
    free(mode_name);

    /* Don't call mode_set_modeline() here since we want to apply the values
     * from the string we're parsing, so just link the modeline
     */
    mode->modeline = modeline;


    /* Read mode information */
    while (*str) {

        /* Read panning */
        if (*str == '@') {
            str++;
            str = parse_read_integer_pair(str, 'x',
                                          &(mode->pan.width),
                                          &(mode->pan.height));
        }

        /* Read position */
        else if (*str == '+') {
            str++;
            str = parse_read_integer_pair(str, 0,
                                          &(mode->pan.x),
                                          &(mode->pan.y));
        }

        /* Read extra params */
        else if (*str == '{') {
            const char *end;
            char *tmp;
            str++;

            end = strchr(str, '}');
            if (!end) goto fail;

            /* Dupe the string so we can parse it properly */
            tmp = nvstrndup(str, (size_t)(end-str));
            if (!tmp) goto fail;

            parse_token_value_pairs(tmp, apply_mode_attribute_token, mode);
            free(tmp);
            if (end && (*end == '}')) {
                str = ++end;
            }
        }

        /* Mode parse error - Ack! */
        else {
            nv_error_msg("Unknown mode token: %s", str);
            str = NULL;
        }

        /* Catch errors */
        if (!str) goto fail;
    }

    /* Initialize defaults for the viewports if unspecified */
    if ((mode->viewPortOut.width == 0) || (mode->viewPortOut.height == 0)) {
        mode->viewPortOut.width = mode->modeline->data.hdisplay;
        mode->viewPortOut.height = mode->modeline->data.vdisplay;
    }
    if ((mode->viewPortIn.width == 0) || (mode->viewPortIn.height == 0)) {
        mode->viewPortIn.width = mode->viewPortOut.width;
        mode->viewPortIn.height = mode->viewPortOut.height;
    }

    /* If rotation is specified, swap W/H if they are still set to the
     * modeline's unrotated dimentions.  Panning should not be rotated
     * here since it is returned rotated by the X driver.
     */
    if (((mode->rotation == ROTATION_90) ||
         (mode->rotation == ROTATION_270)) &&
        (mode->viewPortIn.width == mode->viewPortOut.width) &&
        (mode->viewPortIn.height == mode->viewPortOut.height)) {
        int tmp = mode->viewPortIn.width;
        mode->viewPortIn.width = mode->viewPortIn.height;
        mode->viewPortIn.height = tmp;
    }

    /* Clamp the panning domain */
    clamp_mode_panning(mode);

    return mode;


    /* Handle failures */
 fail:
    if (mode) {
        free(mode);
    }

    return NULL;

} /* mode_parse() */



/** apply_underscan_to_viewportout() **********************************
 *
 * Modifies the given ViewPortOut with an underscan border of the given size in
 * horizontal pixels.  The original aspect ratio is preserved.
 *
 */
void apply_underscan_to_viewportout(const nvSize raster_size,
                                    const int hpixel_value,
                                    GdkRectangle *viewPortOut)
{
    float scale_factor, x_offset, y_offset;

    /* Preserve aspect ratio */
    scale_factor = (float) raster_size.width / raster_size.height;

    x_offset = (float) hpixel_value;
    y_offset = x_offset / scale_factor;

    viewPortOut->x = (gint) x_offset;
    viewPortOut->y = (gint) y_offset;
    viewPortOut->width  = (gint) (raster_size.width - (2 * x_offset));
    viewPortOut->height = (gint) (raster_size.height - (2 * y_offset));

    /* Limit ViewPortOut to a minimum size */
    viewPortOut->width  = MAX(viewPortOut->width, 10);
    viewPortOut->height = MAX(viewPortOut->height, 10);
}



/** get_underscan_percent_from_viewportout() **************************
 *
 * Retrieve underscan from the current ViewPortOut.
 *
 * We could just try to revert the formula, i.e. check that:
 *   viewPortOut.width  + (2 * viewPortOut.x) == raster.width &&
 *   viewPortOut.height + (2 * viewPortOut.y) == raster.height
 *
 * But this doesn't match the case where the initial ViewPortOut
 * computation had rounding issues.
 *
 * Instead, we compute a new ViewPortOut from the current x offset
 * and check that the result matches the current ViewPortOut.
 *
 * Returns the underscan value in percentage.  Defaults to -1 if no
 * underscan could be found.
 *
 */
void get_underscan_settings_from_viewportout(const nvSize raster_size,
                                             const GdkRectangle viewPortOut,
                                             gfloat *percent_value,
                                             gint *pixel_value)
{
    GdkRectangle dummyViewPortOut;

    if (!percent_value || !pixel_value) {
        return;
    }

    apply_underscan_to_viewportout(raster_size,
                                   (int) viewPortOut.x,
                                   &dummyViewPortOut);

    if (!memcmp(&viewPortOut, &dummyViewPortOut, sizeof(GdkRectangle))) {
        *percent_value = (gfloat) viewPortOut.x / raster_size.width * 100;
        *pixel_value = viewPortOut.x;
    } else {
        *percent_value = -1;
        *pixel_value = -1;
    }
}



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
    gchar *flags_str;
    nvDisplayPtr display = mode->display;
    nvScreenPtr screen;
    nvGpuPtr gpu;

    /* Make sure the mode has everything it needs to be displayed */
    if (!mode || !mode->metamode || !display) {
        return NULL;
    }

    /* Don't display dummy modes */
    if (be_generic && mode->dummy && !mode->modeline) {
        return NULL;
    }

    screen = display->screen;
    gpu = display->gpu;
    if (!screen || !gpu) {
        return NULL;
    }

    /* Pick a suitable display name qualifier */
    mode_str = display_pick_config_name(display, be_generic);
    if (mode_str[0] != '\0') {
        tmp = mode_str;
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
    if (!be_generic || (mode->pan.width != mode->viewPortIn.width ||
                        mode->pan.height != mode->viewPortIn.height)) {
        tmp = g_strdup_printf("%s @%dx%d",
                              mode_str, mode->pan.width, mode->pan.height);
        g_free(mode_str);
        mode_str = tmp;
    }


    /* Offset */

    /*
     * XXX Later, we'll want to allow the user to select how
     *     the metamodes are generated:
     *
     *   Programability:
     *     make mode->viewPortIn relative to screen->dim
     *
     *   Coherency:
     *     make mode->viewPortIn relative to mode->metamode->edim
     *
     *
     * XXX Also, we may want to take in consideration the
     *     TwinViewOrientation when writing out position
     *     information.
     */

    tmp = g_strdup_printf("%s +%d+%d",
                          mode_str,
                          /* Make mode position relative */
                          mode->pan.x - mode->metamode->edim.x,
                          mode->pan.y - mode->metamode->edim.y);
    g_free(mode_str);
    mode_str = tmp;


    /* Mode Flags */
    flags_str = NULL;

    /* Passive Stereo Eye */
    if (screen->stereo_supported &&
        (screen->stereo == NV_CTRL_STEREO_PASSIVE_EYE_PER_DPY)) {
        const char *str = NULL;

        switch (mode->passive_stereo_eye) {
        case PASSIVE_STEREO_EYE_LEFT:
            str = "PassiveLeft";
            break;
        case PASSIVE_STEREO_EYE_RIGHT:
            str = "PassiveRight";
            break;
        case 0:
        default:
            str = NULL;
            break;
        }

        if (str) {
            tmp = g_strdup_printf("%s, stereo=%s", (flags_str ? flags_str : ""),
                                  str);
            g_free(flags_str);
            flags_str = tmp;
        }
    }

    /* Rotation */
    if (mode->rotation != ROTATION_0) {
        const char *str = NULL;

        switch (mode->rotation) {
        case ROTATION_90:
            str = "90";
            break;
        case ROTATION_180:
            str = "180";
            break;
        case ROTATION_270:
            str = "270";
            break;
        default:
            break;
        }

        if (str) {
            tmp = g_strdup_printf("%s, rotation=%s",
                                  (flags_str ? flags_str : ""), str);
            g_free(flags_str);
            flags_str = tmp;
        }
    }

    /* Reflection */
    if (mode->reflection != REFLECTION_NONE) {
        const char *str = NULL;

        switch (mode->reflection) {
        case REFLECTION_X:
            str = "X";
            break;
        case REFLECTION_Y:
            str = "Y";
            break;
        case REFLECTION_XY:
            str = "XY";
            break;
        default:
            break;
        }

        if (str) {
            tmp = g_strdup_printf("%s, reflection=%s",
                                  (flags_str ? flags_str : ""), str);
            g_free(flags_str);
            flags_str = tmp;
        }
    }

    /* ViewPortIn */
    {
        int width;
        int height;

        /* Only write out the ViewPortIn if it is specified and differes from
         * the ViewPortOut.
         */
        if ((mode->rotation == ROTATION_90) ||
            (mode->rotation == ROTATION_270)) {
            width = mode->viewPortOut.height;
            height = mode->viewPortOut.width;
        } else {
            width = mode->viewPortOut.width;
            height = mode->viewPortOut.height;
        }

        if (mode->viewPortIn.width && mode->viewPortIn.height &&
            ((mode->viewPortIn.width != width) ||
             (mode->viewPortIn.height != height))) {
            tmp = g_strdup_printf("%s, viewportin=%dx%d",
                                  (flags_str ? flags_str : ""),
                                  mode->viewPortIn.width,
                                  mode->viewPortIn.height);
            g_free(flags_str);
            flags_str = tmp;
        }
    }

    /* ViewPortOut */
    if (mode->viewPortOut.x ||
        mode->viewPortOut.y ||
        (mode->viewPortOut.width && mode->viewPortOut.height &&
         ((mode->viewPortOut.width != mode->modeline->data.hdisplay) ||
          (mode->viewPortOut.height != mode->modeline->data.vdisplay)))) {
        tmp = g_strdup_printf("%s, viewportout=%dx%d%+d%+d",
                              (flags_str ? flags_str : ""),
                              mode->viewPortOut.width, mode->viewPortOut.height,
                              mode->viewPortOut.x, mode->viewPortOut.y);
        g_free(flags_str);
        flags_str = tmp;
    }

    if (flags_str) {
        tmp = g_strdup_printf("%s {%s}",
                              mode_str,
                              flags_str+2 // Skip the first comma and whitespace
                              );
        g_free(mode_str);
        mode_str = tmp;
    }

    return mode_str;

} /* mode_get_str() */




/*****************************************************************************/
/** DISPLAY FUNCTIONS ********************************************************/
/*****************************************************************************/


/** display_names_match() ********************************************
 *
 * Determines if two (display) names are the same.  Returns FALSE if
 * either name is NULL.
 *
 **/

static Bool display_names_match(const char *name1, const char *name2)
{
    if (!name1 || !name2) {
        return FALSE;
    }

    return (strcasecmp(name1, name2) == 0) ? TRUE : FALSE;
}



/** display_pick_config_name() ***************************************
 *
 * Returns one of the display's names to be used for writing
 * configuration.
 *
 * If 'generic' is TRUE, then the most generic name possible is
 * returned.  This depends on the current existence of other display
 * devices, and the name returned here will not collide with the name
 * of other display devices.
 *
 **/
static gchar *display_pick_config_name(nvDisplayPtr display, int be_generic)
{
    nvDisplayPtr other;

    /* Be specific */
    if (!be_generic) {
        goto return_specific;
    }

    /* Only one display, so no need for a qualifier */
    if (display->gpu->num_displays == 1) {
        return g_strdup("");
    }

    /* Find the best generic name possible.  If any display connected to the
     * GPU has the same typeBaseName, then return the typeIdName instead
     */
    for (other = display->gpu->displays; other; other = other->next_on_gpu) {
        if (other == display) continue;
        if (strcmp(other->typeBaseName, display->typeBaseName) == 0) {
            goto return_specific;
        }
    }

    /* No other display device on the GPU shares the same type basename,
     * so we can use it
     */
    return g_strdup(display->typeBaseName);

 return_specific:
    return g_strdup(display->typeIdName);
}



/** display_find_closest_mode_matching_modeline() ********************
 *
 * Helper function that returns the mode index of the display's mode
 * that best matches the given modeline.
 *
 * A best match is:
 *
 * - The modelines match in width & height.
 * - Then, the modelines match the ViewPortIn.
 * - Then, the modelines match the ViewPortOut.
 *
 **/
int display_find_closest_mode_matching_modeline(nvDisplayPtr display,
                                                nvModeLinePtr modeline)
{
    const int targetWidth = modeline->data.hdisplay;
    const int targetHeight = modeline->data.vdisplay;

    nvModePtr mode, best_mode = NULL;
    int mode_idx;
    int best_idx = -1;

    mode_idx = 0;
    for (mode = display->modes; mode; mode = mode->next) {
        if (!mode->modeline) {
            continue;
        } else if (mode->modeline->data.hdisplay == targetWidth &&
                   mode->modeline->data.vdisplay == targetHeight) {
            nvModePtr tmp_mode = mode;
            int tmp_idx = mode_idx;

            /* We already have a match.  Let's figure out if the
             * currently considered mode is the closer to what we
             * want.
             */
            if (best_mode) {
                Bool current_match_vpin =
                    (mode->viewPortIn.width == targetWidth &&
                     mode->viewPortIn.height == targetHeight);
                Bool best_match_vpin =
                    (best_mode->viewPortIn.width == targetWidth &&
                     best_mode->viewPortIn.height == targetHeight);
                Bool best_match_vpout =
                    (best_mode->viewPortOut.width == targetWidth &&
                     best_mode->viewPortOut.height == targetHeight);

                /* Try to find reasons why we should prefer the
                 * previous match over the currently considered
                 * mode.
                 *
                 * We first check which one has a matching ViewPortIn
                 * If it's the case for both of them, then we compare
                 * ViewPortOut.
                 *
                 * If both are equally close, we keep our previous
                 * match.
                 */
                if ((!current_match_vpin && best_match_vpin) ||
                    (current_match_vpin && best_match_vpin &&
                     best_match_vpout)) {
                    tmp_mode = best_mode;
                    tmp_idx = best_idx;
                }
                /* Fallthrough. */
            }
            best_mode = tmp_mode;
            best_idx = tmp_idx;
        }
        mode_idx++;
    }

    return best_idx;

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



/** viewport_in_match() **********************************************
 * 
 * Helper function that returns TRUE of FALSE based on whether
 * the ViewPortIn arguments match each other.
 *
 **/
Bool viewports_in_match(const nvSize viewPortIn1,
                        const nvSize viewPortIn2)
{
    return ((viewPortIn1.width == viewPortIn2.width) &&
            (viewPortIn1.height == viewPortIn2.height));
}



/** viewport_out_match() *********************************************
 * 
 * Helper function that returns TRUE of FALSE based on whether
 * the ViewPortOut arguments match each other.
 *
 **/
Bool viewports_out_match(const GdkRectangle viewPortOut1,
                         const GdkRectangle viewPortOut2)
{
    return ((viewPortOut1.x == viewPortOut2.x) &&
            (viewPortOut1.y == viewPortOut2.y) &&
            (viewPortOut1.width == viewPortOut2.width) &&
            (viewPortOut1.height == viewPortOut2.height));
}



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
Bool display_add_modelines_from_server(nvDisplayPtr display, nvGpuPtr gpu,
                                       gchar **err_str)
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

    ret = NvCtrlGetAttribute(display->handle,
                             NV_CTRL_ATTR_NV_MAJOR_VERSION, &major);
    ret1 = NvCtrlGetAttribute(display->handle,
                              NV_CTRL_ATTR_NV_MINOR_VERSION, &minor);

    if ((ret == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        ((major > 1) || ((major == 1) && (minor > 13)))) {
        broken_doublescan_modelines = 0;
    }


    /* Free any old mode lines */
    display_remove_modelines(display);


    /* Get the validated modelines for the display */
    ret = NvCtrlGetBinaryAttribute(display->handle, 0,
                                   NV_CTRL_BINARY_DATA_MODELINES,
                                   (unsigned char **)&modeline_strs, &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query modelines of display "
                                  "device %d '%s'.",
                                   NvCtrlGetTargetId(display->handle),
                                   display->logName);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Parse each modeline */
    str = modeline_strs;
    while (strlen(str)) {

        modeline = modeline_parse(display, gpu, str,
                                  broken_doublescan_modelines);
        if (!modeline) {
            *err_str = g_strdup_printf("Failed to parse the following "
                                       "modeline of display device\n"
                                      "%d '%s' :\n\n%s",
                                       NvCtrlGetTargetId(display->handle),
                                       display->logName,
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



/*!
 * Sets all the modes on the display to the specified rotation
 *
 * \param[in]  mode      The display who's modes are to be modified
 * \param[in]  rotation  The rotation to set
 *
 * \return  TRUE if a new rotation was set for at least one mode, FALSE if all
 *          of the modes on the display were already set to the rotation given.
 */
Bool display_set_modes_rotation(nvDisplayPtr display, Rotation rotation)
{
    nvModePtr mode;
    Bool modified = FALSE;

    for (mode = display->modes; mode; mode = mode->next) {
        if (mode_set_rotation(mode, rotation)) {
            modified = TRUE;
        }
    }

    return modified;
}



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
        XFree(display->logName);
        XFree(display->typeBaseName);
        XFree(display->typeIdName);
        XFree(display->dpGuidName);
        XFree(display->edidHashName);
        XFree(display->targetIdName);
        XFree(display->randrName);
        free(display);
    }

} /* display_free() */




/*****************************************************************************/
/** SCREEN FUNCTIONS *********************************************************/
/*****************************************************************************/


/*!
 * Clamps the given (screen) dimensions to the minimum allowed screen size.
 *
 * \param[in, out]  rect  The dimensions to clamp
 */
void clamp_screen_size_rect(GdkRectangle *rect)
{
    if (rect->width < 304) {
        rect->width = 304;
    }
    if (rect->height < 200) {
        rect->height = 200;
    }
}



/** screen_find_named_display() **************************************
 *
 * Finds a display named 'display_name' in the list of displays on the
 * given screen, or NULL if no display matched 'display_name'.
 *
 **/

static nvDisplayPtr screen_find_named_display(nvScreenPtr screen,
                                              char *display_name)
{
    nvDisplayPtr display;
    nvDisplayPtr possible_display = NULL;

    if (!display_name) {
        return NULL;
    }

    /* Look for exact matches */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        /* Look for an exact match */
        if (display_names_match(display->typeIdName, display_name)) {
            return display;
        }
        if (display_names_match(display->dpGuidName, display_name)) {
            return display;
        }
        if (display_names_match(display->targetIdName, display_name)) {
            return display;
        }
        if (display_names_match(display->randrName, display_name)) {
            return display;
        }

        /* Allow matching to generic names, but only return these
         * if no other name matched
         */
        if (!possible_display) {
            if (display_names_match(display->typeBaseName, display_name)) {
                possible_display = display;
            }
            if (display_names_match(display->edidHashName, display_name)) {
                possible_display = display;
            }
        }
    }

    return possible_display;
}



/** renumber_xscreens() **********************************************
 *
 * Ensures that the screens are numbered from 0 to (n-1).
 *
 **/

void renumber_xscreens(nvLayoutPtr layout)
{
    nvScreenPtr screen;
    nvScreenPtr lowest;
    int scrnum;

    scrnum = 0;
    do {

        /* Find screen w/ lowest # >= current screen index being assigned */
        lowest = NULL;
        for (screen = layout->screens;
             screen;
             screen = screen->next_in_layout) {

            if ((screen->scrnum >= scrnum) &&
                (!lowest || (lowest->scrnum > screen->scrnum))) {
                lowest = screen;
            }
        }

        if (lowest) {
            lowest->scrnum = scrnum;
        }

        /* Assign next screen number */
        scrnum++;
    } while (lowest);

} /* renumber_xscreens() */



/** screen_link_display() ********************************************
 *
 * Makes the given display part of the screen
 *
 **/
void screen_link_display(nvScreenPtr screen, nvDisplayPtr display)
{
    if (!display || !screen || (display->screen == screen)) return;

    display->screen = screen;
    display->next_in_screen = NULL;

    /* Add the display at the end of the screen's display list */
    if (!screen->displays) {
        screen->displays = display;
    } else {
        nvDisplayPtr last;
        for (last = screen->displays; last; last = last->next_in_screen) {
            if (!last->next_in_screen) {
                last->next_in_screen = display;
                break;
            }
        }
    }
    screen->num_displays++;

} /* screen_link_display() */



/** screen_unlink_display() ******************************************
 *
 * Removes the display from the screen's list of displays
 *
 **/
void screen_unlink_display(nvDisplayPtr display)
{
    nvScreenPtr screen;

    if (!display || !display->screen) return;

    screen = display->screen;

    /* Remove the display from the screen */
    if (screen->displays == display) {
        screen->displays = display->next_in_screen;
    } else {
        nvDisplayPtr cur = screen->displays;
        while (cur) {
            if (cur->next_in_screen == display) {
                cur->next_in_screen = display->next_in_screen;
                break;
            }
            cur = cur->next_in_screen;
        }
    }
    screen->num_displays--;

    display->screen = NULL;

} /* screen_unlink_display() */


static void screen_link_displays(nvScreenPtr screen)
{
    ReturnStatus ret;
    int *pData;
    int len;
    int i;

    ret = NvCtrlGetBinaryAttribute
        (screen->handle, 0, NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN,
         (unsigned char **)(&pData), &len);

    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query list of displays assigned to X screen "
                       " %d.",
                        NvCtrlGetTargetId(screen->handle));
        return;
    }

    // For each id in pData
    for (i = 0; i < pData[0]; i++) {
        nvDisplayPtr display;

        display = layout_get_display(screen->layout, pData[i+1]);
        if (!display) {
            nv_warning_msg("Failed to find display %d assigned to X screen "
                           " %d.",
                           pData[i+1],
                           NvCtrlGetTargetId(screen->handle));
            continue;
        }

        screen_link_display(screen, display);
    }

    XFree(pData);
}




/** screen_remove_display() ******************************************
 *
 * Removes a display device from the screen
 *
 **/
void screen_remove_display(nvDisplayPtr display)
{
    nvScreenPtr screen;
    nvDisplayPtr other;
    nvModePtr mode;

    if (!display || !display->screen) return;


    screen = display->screen;

    /* Make any display relative to this one use absolute position */
    for (other = screen->displays; other; other = other->next_in_screen) {

        if (other == display) continue;

        for (mode = other->modes; mode; mode = mode->next) {
            if (mode->relative_to == display) {
                mode->position_type = CONF_ADJ_ABSOLUTE;
                mode->relative_to = NULL;
            }
        }
    }

    /* Remove the display from the screen */
    screen_unlink_display(display);

    /* Clean up old references */
    if (screen->primaryDisplay == display) {
        screen->primaryDisplay = NULL;
    }

    display_remove_modes(display);

} /* screen_remove_display() */



/** screen_remove_displays() *****************************************
 *
 * Removes all displays currently pointing at this screen, also
 * freeing any memory used.
 *
 **/
static void screen_remove_displays(nvScreenPtr screen)
{
    if (!screen) return;

    while (screen->displays) {
        screen_remove_display(screen->displays);
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

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

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
    nvDisplayPtr display;
    nvMetaModePtr metamode;

    if (!screen) return;

    /* Remove the modes from this screen's displays */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        display_remove_modes(display);
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

} /* screen_remove_metamodes() */



 /** mode_strtok() ***************************************************
 *
 * Special strtok function for parsing modes.  This function ignores
 * anything between curly braces, including commas when parsing tokens
 * deliminated by commas.
 *
 **/
static char *mode_strtok(char *str)
{
    static char *intStr = NULL;
    char *start;

    if (str) {
        intStr = str;
    }

    if (!intStr || *intStr == '\0') {
        return NULL;
    }

    /* Mark off the next token value */
    start = intStr;
    while (*intStr != '\0') {
        if (*intStr == '{') {
            while (*intStr != '}' && *intStr != '\0') {
                intStr++;
            }
        }
        if (*intStr == ',') {
            *intStr = '\0';
            intStr++;
            break;
        }
        intStr++;
    }

    return start;
}



/** screen_add_metamode() ********************************************
 *
 * Parses a metamode string and adds the appropriate modes to the
 * screen's display devices (at the end of the list)
 *
 **/
static Bool screen_add_metamode(nvScreenPtr screen, const char *metamode_str,
                                gchar **err_str)
{
    char *mode_str_itr;
    const char *tokens_end;
    const char *metamode_modes;
    char *metamode_copy = NULL;
    nvMetaModePtr metamode = NULL;
    int mode_count = 0;


    if (!screen || !screen->gpu || !metamode_str) goto fail;


    metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
    if (!metamode) goto fail;


    /* Read the MetaMode ID (along with any metamode tokens) */
    tokens_end = strstr(metamode_str, "::");
    if (tokens_end) {
        char *tokens = strdup(metamode_str);

        if (!tokens) goto fail;

        tokens[tokens_end-metamode_str] = '\0';
        parse_token_value_pairs(tokens, apply_metamode_token, (void *)metamode);

        free(tokens);
        metamode_modes = tokens_end + 2;
    } else {
        /* No tokens?  Try the old "ID: METAMODE_STR" syntax */
        metamode_modes = parse_read_integer(metamode_str, &(metamode->id));
        metamode->source = METAMODE_SOURCE_NVCONTROL;
        if (*metamode_modes == ':') {
            metamode_modes++;

        }
    }

    /* Process each mode in the metamode string */
    metamode_copy = strdup(metamode_modes);
    if (!metamode_copy) goto fail;

    for (mode_str_itr = mode_strtok(metamode_copy);
         mode_str_itr;
         mode_str_itr = mode_strtok(NULL)) {

        nvModePtr     mode;
        nvDisplayPtr  display;
        unsigned int  display_id;
        const char *orig_mode_str = parse_skip_whitespace(mode_str_itr);
        const char *mode_str;

        /* Parse the display device (NV-CONTROL target) id from the name */
        mode_str = parse_read_display_id(mode_str_itr, &display_id);
        if (!mode_str) {
            nv_warning_msg("Failed to read a display device name on screen %d "
                           "while parsing metamode:\n\n'%s'",
                           screen->scrnum,

                           orig_mode_str);
            continue;
        }

        /* Match device id to an existing display */
        display = layout_get_display(screen->layout, display_id);
        if (!display) {
            nv_warning_msg("Failed to find display device %d on screen %d "
                           "while parsing metamode:\n\n'%s'",
                           display_id,
                           screen->scrnum,

                           orig_mode_str);
            continue;
        }

        /* Parse the mode */
        mode = mode_parse(display, mode_str);
        if (!mode) {
            nv_warning_msg("Failed to parse mode '%s'\non screen %d\n"
                           "from metamode:\n\n'%s'",
                           mode_str,
                           screen->scrnum,
                           orig_mode_str);
            continue;
        }

        /* Make the mode part of the metamode */
        mode->metamode = metamode;

        /* On older X driver NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN
         * attribute is Not Available so we are unable to link displays to
         * the screen implicitly.
         * To avoid display->cur_mode = NULL link displays explicitly.
         */ 
        screen_link_display(screen, display);
        /* Make sure each display has the right number of (NULL) modes */
        screen_check_metamodes(screen);

        /* Add the mode at the end of the display's mode list */
        xconfigAddListItem((GenericListPtr *)(&display->modes),
                           (GenericListPtr)mode);
        display->num_modes++;
        mode_count++;
    }

    free(metamode_copy);
    metamode_copy = NULL;

    /* Make sure something was added */
    if (mode_count == 0) {
        nv_warning_msg("Failed to find any display on screen %d (on GPU-%d)\n"
                       "while parsing metamode:\n\n'%s'",
                       screen->scrnum,
                       NvCtrlGetTargetId(screen->gpu->handle),
                       metamode_str);
        goto fail;
    }

    /* Add the metamode to the end of the screen's metamode list */
    xconfigAddListItem((GenericListPtr *)(&screen->metamodes),
                       (GenericListPtr)metamode);

    return TRUE;

 fail:

    /* Cleanup */
    if (metamode) {
        free(metamode);
    }
    if (metamode_copy) {
        free(metamode_copy);
    }

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


    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

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
                mode->pan.x = last_mode->pan.x;
                mode->pan.y = last_mode->pan.y;
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
static void screen_assign_dummy_metamode_positions(nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvModePtr ok_mode;
    nvModePtr mode;


    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        /* Get the first non-dummy mode */
        for (ok_mode = display->modes; ok_mode; ok_mode = ok_mode->next) {
            if (!ok_mode->dummy) break;
        }

        if (ok_mode) {
            for (mode = display->modes; mode; mode = mode->next) {
                if (!mode->dummy) continue;
                mode->pan.x = ok_mode->pan.x;
                mode->pan.y = ok_mode->pan.y;
            }
        }
    }

} /* screen_assign_dummy_metamode_positions() */



/** screen_add_metamodes() *******************************************
 *
 * Adds all the appropriate modes on all display devices of this
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
                                   NV_CTRL_BINARY_DATA_METAMODES_VERSION_2,
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
                                   NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
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
    for (str = metamode_strs;
         (str && strlen(str));
          str += strlen(str) +1) {

        /* Add the individual metamodes to the screen,
         * This populates the display device's mode list.
         */
        if (!screen_add_metamode(screen, str, err_str)) {
            nv_warning_msg("Failed to add metamode '%s' to screen %d (on "
                           "GPU-%d).",
                           str, screen->scrnum,
                           NvCtrlGetTargetId(screen->gpu->handle));
            continue;
        }

        /* Keep track of the current metamode */
        if (!strcmp(str, cur_metamode_str)) {
            screen->cur_metamode_idx = screen->num_metamodes;
        }

        /* Keep count of the metamode */
        screen->num_metamodes++;

        /* Make sure each display device gets a mode */
        screen_check_metamodes(screen);
    }
    XFree(metamode_strs);
    metamode_strs = NULL;

    if (!screen->metamodes) {
        nv_warning_msg("Failed to add any metamode to screen %d (on GPU-%d).",
                       screen->scrnum,
                       NvCtrlGetTargetId(screen->gpu->handle));
        goto fail;
    }

    /* Assign the top left position of dummy modes */
    screen_assign_dummy_metamode_positions(screen);


    /* Make the screen point at the current metamode */
    screen->cur_metamode = screen->metamodes;
    for (i = 0; i < screen->cur_metamode_idx; i++) {
        screen->cur_metamode = screen->cur_metamode->next;
    }


    /* Make each display within the screen point to the current mode */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

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


/** gpu_remove_and_free_display() ************************************
 *
 * Removes a display from the GPU and frees it.
 *
 **/
void gpu_remove_and_free_display(nvDisplayPtr display)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;

    if (!display || !display->gpu) return;

    gpu = display->gpu;
    screen = display->screen;

    /* Remove the display from the X screen */
    if (screen) {
        screen_remove_display(display);

        if (!screen->num_displays) {
            layout_remove_and_free_screen(screen);
        }
    }

    /* Remove the display from the GPU */
    if (gpu->displays == display) {
        gpu->displays = display->next_on_gpu;
    } else {
        nvDisplayPtr cur;
        for (cur = gpu->displays; cur; cur = cur->next_on_gpu) {
            if (cur->next_on_gpu == display) {
                cur->next_on_gpu = display->next_on_gpu;
                break;
            }
        }
    }
    gpu->num_displays--;

    display_free(display);

} /* gpu_remove_and_free_display() */



/** gpu_remove_displays() ********************************************
 *
 * Removes all displays from the gpu
 *
 **/
static void gpu_remove_displays(nvGpuPtr gpu)
{
    if (!gpu) return;

    while (gpu->displays) {
        gpu_remove_and_free_display(gpu->displays);
    }

} /* gpu_remove_displays() */



/** gpu_add_display() ************************************************
 *
 * Adds the display to (the end of) the GPU display list.
 *
 **/
static void gpu_add_display(nvGpuPtr gpu, nvDisplayPtr display)
{

    if (!display || !gpu || (display->gpu == gpu)) return;

    display->gpu = gpu;
    display->next_on_gpu = NULL;

    /* Add the display at the end of the GPU's display list */
    if (!gpu->displays) {
        gpu->displays = display;
    } else {
        nvDisplayPtr last;
        for (last = gpu->displays; last; last = last->next_on_gpu) {
            if (!last->next_on_gpu) {
                last->next_on_gpu = display;
                break;
            }
        }
    }
    gpu->num_displays++;

} /* gpu_add_display() */



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



/** display_add_name_from_server() ***********************************
 *
 *  Queries and adds the NV-CONTROL name to the display device.
 *
 **/

static const struct DisplayNameInfoRec {
    int attr;
    Bool canBeNull;
    const char *nameDescription;
    size_t offset;
} DisplayNamesTable[] = {
    { NV_CTRL_STRING_DISPLAY_DEVICE_NAME,        FALSE, "Log Name",
      offsetof(nvDisplay, logName) },
    { NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME, FALSE, "Type Base Name",
      offsetof(nvDisplay, typeBaseName) },
    { NV_CTRL_STRING_DISPLAY_NAME_TYPE_ID,       FALSE, "Type ID",
      offsetof(nvDisplay, typeIdName) },
    { NV_CTRL_STRING_DISPLAY_NAME_DP_GUID,       TRUE,  "DP GUID Name",
      offsetof(nvDisplay, dpGuidName) },
    { NV_CTRL_STRING_DISPLAY_NAME_EDID_HASH,     TRUE,  "EDID Hash Name",
      offsetof(nvDisplay, edidHashName) },
    { NV_CTRL_STRING_DISPLAY_NAME_TARGET_INDEX,  FALSE, "Target Index Name",
      offsetof(nvDisplay, targetIdName) },
    { NV_CTRL_STRING_DISPLAY_NAME_RANDR,         FALSE, "RandR Name",
      offsetof(nvDisplay, randrName) },
};

static Bool display_add_name_from_server(nvDisplayPtr display,
                                         const struct DisplayNameInfoRec *displayNameInfo,
                                         gchar **err_str)
{
    ReturnStatus ret;
    char *str;

    ret = NvCtrlGetStringAttribute(display->handle,
                                   displayNameInfo->attr,
                                   &str);
    if (ret == NvCtrlSuccess) {
        *((char **)(((char *)display) + displayNameInfo->offset)) = str;

    } else if (!displayNameInfo->canBeNull) {
        *err_str = g_strdup_printf("Failed to query name '%s' of display "
                                   "device DPY-%d.",
                                   displayNameInfo->nameDescription,
                                   NvCtrlGetTargetId(display->handle));
        nv_error_msg(*err_str);
        return FALSE;
    }

    return TRUE;
}



/** gpu_add_display_from_server() ************************************
 *
 *  Adds the display with the device id given to the GPU structure.
 *
 **/
nvDisplayPtr gpu_add_display_from_server(nvGpuPtr gpu,
                                         unsigned int display_id,
                                         gchar **err_str)
{
    ReturnStatus ret;
    nvDisplayPtr display;
    int i;


    /* Create the display structure */
    display = (nvDisplayPtr)calloc(1, sizeof(nvDisplay));
    if (!display) goto fail;


    /* Make an NV-CONTROL handle to talk to the display */
    display->handle =
        NvCtrlAttributeInit(NvCtrlGetDisplayPtr(gpu->handle),
                            NV_CTRL_TARGET_TYPE_DISPLAY,
                            display_id,
                            NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);
    if (!display->handle) {
        *err_str = g_strdup_printf("Failed to create NV-CONTROL handle for\n"
                                   "display %d (on GPU-%d).",
                                   display_id,
                                   NvCtrlGetTargetId(gpu->handle));
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Query the display information */
    for (i = 0; i < ARRAY_LEN(DisplayNamesTable); i++) {
        if (!display_add_name_from_server(display,
                                          DisplayNamesTable + i, err_str)) {
            goto fail;
        }
    }


    /* Query if this display is an SDI display */
    ret = NvCtrlGetAttribute(display->handle,
                             NV_CTRL_IS_GVO_DISPLAY,
                             &(display->is_sdi));
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query if display device\n"
                       "%d connected to GPU-%d '%s' is an\n"
                       "SDI device.",
                       display_id, NvCtrlGetTargetId(gpu->handle),
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
            gpu->gvo_mode_data = calloc(gpu->num_gvo_modes, sizeof(GvoModeData));
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
    if (!display_add_modelines_from_server(display, gpu, err_str)) {
        nv_warning_msg("Failed to add modelines to display device %d "
                       "'%s'\nconnected to GPU-%d '%s'.",
                       display_id, display->logName,
                       NvCtrlGetTargetId(gpu->handle), gpu->name);
        goto fail;
    }

    /* Add the display at the end of gpu's display list */
    gpu_add_display(gpu, display);

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
    ReturnStatus ret;
    int *pData;
    int len;
    int i;


    /* Clean up the GPU list */
    gpu_remove_displays(gpu);

    /* Get list of displays connected to this GPU */
    ret = NvCtrlGetBinaryAttribute(gpu->handle, 0,
                                   NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                                   (unsigned char **)(&pData), &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query list of displays \n"
                                   "connected to GPU-%d '%s'.",
                                   NvCtrlGetTargetId(gpu->handle), gpu->name);
        nv_error_msg(*err_str);
        goto fail;
    }

    /* Add each connected display */
    for (i = 0; i < pData[0]; i++) {
        if (!gpu_add_display_from_server(gpu, pData[i+1], err_str)) {
            nv_warning_msg("Failed to add display device %d to GPU-%d "





                           "'%s'.",
                           pData[i+1], NvCtrlGetTargetId(gpu->handle),
                           gpu->name);
            XFree(pData);
            goto fail;
        }
    }

    XFree(pData);
    return TRUE;

 fail:
    gpu_remove_displays(gpu);
    return FALSE;

} /* gpu_add_displays_from_server() */



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

    for (display = gpu->displays; display; display = display->next_on_gpu) {
        if (display->screen) continue;
        if (display->modes) continue;

        /* Create a fake mode */
        mode = (nvModePtr)calloc(1, sizeof(nvMode));
        if (!mode) return FALSE;

        mode->display = display;
        mode->dummy = 1;

        mode_set_modeline(mode,
                          NULL /* modeline */,
                          NULL /* viewPortIn */,
                          NULL /* viewPortOut */);

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
        gpu_remove_displays(gpu);

        XFree(gpu->name);
        XFree(gpu->flags_memory);
        g_free(gpu->pci_bus_id);
        free(gpu->gvo_mode_data);

        if (gpu->handle) {
            NvCtrlAttributeClose(gpu->handle);
        }
        free(gpu);
    }

} /* gpu_free() */




/*****************************************************************************/
/** LAYOUT FUNCTIONS *********************************************************/
/*****************************************************************************/



/** layout_add_gpu() *************************************************
 *
 * Adds a GPU to the (end of the) layout's list of GPUs.
 *
 **/
static void layout_add_gpu(nvLayoutPtr layout, nvGpuPtr gpu)
{
    gpu->layout = layout;
    gpu->next_in_layout = NULL;

    if (!layout->gpus) {
        layout->gpus = gpu;
    } else {
        nvGpuPtr last;
        for (last = layout->gpus; last; last = last->next_in_layout) {
            if (!last->next_in_layout) {
                last->next_in_layout = gpu;
                break;
            }
        }
    }

    layout->num_gpus++;

} /* layout_add_gpu() */



/** layout_add_screen() **********************************************
 *
 * Adds a screen to the (end of the) layout's list of screens.
 *
 **/
void layout_add_screen(nvLayoutPtr layout, nvScreenPtr screen)
{
    screen->layout = layout;
    screen->next_in_layout = NULL;

    if (!layout->screens) {
        layout->screens = screen;
    } else {
        nvScreenPtr last;
        for (last = layout->screens; last; last = last->next_in_layout) {
            if (!last->next_in_layout) {
                last->next_in_layout = screen;
                break;
            }
        }
    }

    layout->num_screens++;

} /* layout_add_screen() */



/** layout_remove_and_free_screen() **********************************
 *
 * Removes a screen from the layout and frees it.
 *
 **/

void layout_remove_and_free_screen(nvScreenPtr screen)
{
    nvLayoutPtr layout = screen->layout;
    nvScreenPtr other;

    if (!screen) return;


    /* Make sure other screens in the layout aren't relative
     * to this screen
     */
    for (other = layout->screens;
         other;
         other = other->next_in_layout) {

        if (other->relative_to == screen) {
            other->position_type = CONF_ADJ_ABSOLUTE;
            other->relative_to = NULL;
        }
    }

    /* Remove the screen from the layout */
    if (layout->screens == screen) {
        layout->screens = screen->next_in_layout;
    } else {
        nvScreenPtr cur;
        for (cur = layout->screens; cur; cur = cur->next_in_layout) {
            if (cur->next_in_layout == screen) {
                cur->next_in_layout = screen->next_in_layout;
                break;
            }
        }
    }
    layout->num_screens--;

    screen_free(screen);

} /* layout_remove_and_free_screen() */



/** layout_remove_gpus() *********************************************
 *
 * Removes all GPUs from the layout structure.
 *
 **/
static void layout_remove_gpus(nvLayoutPtr layout)
{
    nvGpuPtr gpu;

    if (!layout) return;

    while (layout->gpus) {
        gpu = layout->gpus;
        layout->gpus = gpu->next_in_layout;
        gpu_free(gpu);
    }
    layout->num_gpus = 0;

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
    unsigned int *pData;
    int len;


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

    get_bus_id_str(gpu->handle, &(gpu->pci_bus_id));

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

    ret = NvCtrlGetBinaryAttribute(gpu->handle,
                                   0,
                                   NV_CTRL_BINARY_DATA_GPU_FLAGS,
                                   (unsigned char **)&pData,
                                   &len);
    if (ret != NvCtrlSuccess) {
        gpu->num_flags = 0;
        gpu->flags_memory = NULL;
        gpu->flags = NULL;
    } else {
        gpu->flags_memory = pData;
        gpu->num_flags = pData[0];
        gpu->flags = &pData[1];
    }

    /* Add the display devices to the GPU */
    if (!gpu_add_displays_from_server(gpu, err_str)) {
        nv_warning_msg("Failed to add displays to GPU-%d '%s'.",
                       gpu_id, gpu->name);
        goto fail;
    }

    /* Add the GPU at the end of the layout's GPU list */
    layout_add_gpu(layout, gpu);

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



/** layout_remove_screens() ******************************************
 *
 * Removes all X screens from the layout structure.
 *
 **/
static void layout_remove_screens(nvLayoutPtr layout)
{
    if (!layout) return;

    while (layout->screens) {
        layout_remove_and_free_screen(layout->screens);
    }

} /* layout_remove_screens() */



/** link_screen_to_gpu() *********************************************
 *
 * Finds the GPU driving the screen and links the two.
 *
 **/

static Bool link_screen_to_gpu(nvLayoutPtr layout, nvScreenPtr screen)
{
    int val;
    ReturnStatus ret;
    nvGpuPtr gpu;

    /* Link the screen to the display owner GPU.  If there is no display owner,
     * which is the case when SLI Mosaic Mode is configured, link screen
     * to the first GPU we find.
     */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_MULTIGPU_DISPLAY_OWNER,
                             &val);
    if (ret != NvCtrlSuccess) {
        int *pData = NULL;
        int len;

        ret = NvCtrlGetBinaryAttribute(screen->handle,
                                       0,
                                       NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                       (unsigned char **)(&pData),
                                       &len);
        if (ret != NvCtrlSuccess || !pData) {
            return FALSE;
        }
        if (pData[0] < 1) {
            XFree(pData);
            return FALSE;
        }

        /* Pick the first GPU */
        val = pData[1];
        XFree(pData);
    }

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        if (val == NvCtrlGetTargetId(gpu->handle)) {
            screen->gpu = gpu;
            return TRUE;
        }
    }

    return FALSE;

} /* link_screen_to_gpu() */



/** layout_add_screen_from_server() **********************************
 *
 * Adds an X screen to the layout structure.
 *
 **/
static Bool layout_add_screen_from_server(nvLayoutPtr layout,
                                          unsigned int screen_id,
                                          gchar **err_str)
{
    Display *display;
    nvScreenPtr screen;
    int val, tmp;
    ReturnStatus ret;
    gchar *primary_str = NULL;


    screen = (nvScreenPtr)calloc(1, sizeof(nvScreen));
    if (!screen) goto fail;

    screen->scrnum = screen_id;


    /* Make an NV-CONTROL handle to talk to the screen (use the
     * first gpu's display)
     */
    display = NvCtrlGetDisplayPtr(layout->gpus->handle);
    screen->handle =
        NvCtrlAttributeInit(display,
                            NV_CTRL_TARGET_TYPE_X_SCREEN,
                            screen_id,
                            NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM);

    if (!screen->handle) {
        *err_str = g_strdup_printf("Failed to create NV-CONTROL handle for\n"
                                   "screen %d.",
                                   screen_id);
        nv_error_msg(*err_str);
        goto fail;
    }


    /* Query the current stereo mode */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_STEREO, &val);
    if (ret == NvCtrlSuccess) {
        screen->stereo_supported = TRUE;
        screen->stereo = val;

        /* XXX For now, if stereo is off, don't show configuration options
         * until we work out interactions with composite.
         */
        if (val == NV_CTRL_STEREO_OFF) {
            screen->stereo_supported = FALSE;
        }
    } else {
        screen->stereo_supported = FALSE;
    }

    /* Query the current overlay state */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_OVERLAY, &val);
    if (ret == NvCtrlSuccess) {
        screen->overlay = val;
    } else {
        screen->overlay = NV_CTRL_OVERLAY_OFF;
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
    screen->dynamic_twinview = !!val;


    /* See if the screen is set to not scanout */
    ret = NvCtrlGetAttribute(screen->handle, NV_CTRL_NO_SCANOUT, &val);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query NoScanout for "
                                   "screen %d.",
                                   screen_id);
        nv_warning_msg(*err_str);
        goto fail;
    }
    screen->no_scanout = (val == NV_CTRL_NO_SCANOUT_ENABLED);


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

    /* Link screen to the GPU driving it */
    if (!link_screen_to_gpu(layout, screen)) {
        *err_str = g_strdup_printf("Failed to find GPU that drives screen %d.",
                                   screen_id);
        nv_warning_msg(*err_str);
        goto fail;
    }

    /* Query SLI status */
    ret = NvCtrlGetAttribute(screen->handle,
                             NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,
                             &tmp);

    screen->sli = (ret == NvCtrlSuccess);


    /* Listen to NV-CONTROL events on this screen handle */
    screen->ctk_event = CTK_EVENT(ctk_event_new(screen->handle));


    /* Query the depth of the screen */
    screen->depth = NvCtrlGetScreenPlanes(screen->handle);

    /* Initialize the virtual X screen size */
    screen->dim.width = NvCtrlGetScreenWidth(screen->handle);
    screen->dim.height = NvCtrlGetScreenHeight(screen->handle);

    /* Add the screen to the layout */
    layout_add_screen(layout, screen);

    /* Link displays to the screen */
    screen_link_displays(screen);

    /* Parse the screen's metamodes (ties displays on the gpu to the screen) */
    if (!screen->no_scanout) {
        if (!screen_add_metamodes(screen, err_str)) {
            nv_warning_msg("Failed to add metamodes to screen %d.",
                           screen_id);
            goto fail;
        }

        /* Query & parse the screen's primary display */
        screen->primaryDisplay = NULL;
        ret = NvCtrlGetStringAttribute(screen->handle,
                                       NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER,
                                       &primary_str);

        if (ret == NvCtrlSuccess && primary_str) {
            char *str;

            /* The TwinView Xinerana Info Order string may be a comma-separated
             * list of display device names, though we could add full support
             * for ordering these, just keep track of a single display here.
             */
            str = strchr(primary_str, ',');
            if (!str) {
                str = nvstrdup(primary_str);
            } else {
                str = nvstrndup(primary_str, str-primary_str);
            }
            XFree(primary_str);

            screen->primaryDisplay = screen_find_named_display(screen, str);
            nvfree(str);
        }
    }

    return TRUE;

 fail:
    if (screen) {
        if (screen->layout) {
            layout_remove_and_free_screen(screen);
        } else {
            screen_free(screen);
        }
    }

    return FALSE;

} /* layout_add_screen_from_server() */



/** layout_add_screens_from_server() *********************************
 *
 * Adds the screens found on the server to the layout structure.
 *
 **/
static int layout_add_screens_from_server(nvLayoutPtr layout, gchar **err_str)
{
    ReturnStatus ret;
    int i, nscreens;


    layout_remove_screens(layout);


    ret = NvCtrlQueryTargetCount(layout->handle, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                 &nscreens);
    if (ret != NvCtrlSuccess || !nscreens) {
        *err_str = g_strdup("Failed to query number of X screens (or no X "
                            "screens found) in the system.");
        nv_error_msg(*err_str);
        nscreens = 0;
        goto fail;
    }

    for (i = 0; i < nscreens; i++) {
        if (!layout_add_screen_from_server(layout, i, err_str)) {
            nv_warning_msg("Failed to add X screen %d to layout.", i);
            g_free(*err_str);
            *err_str = NULL;
        }
    }

    return nscreens;


 fail:
    layout_remove_screens(layout);
    return 0;

} /* layout_add_screens_from_server() */



/** layout_add_screenless_modes_to_displays()*************************
 *
 * Adds fake modes to display devices that are currently not
 * associated with an X screen so we can see them on the layout.
 *
 **/
static Bool layout_add_screenless_modes_to_displays(nvLayoutPtr layout)
{
    nvGpuPtr gpu;

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {

        /* Add fake modes to screenless display devices */
        if (!gpu_add_screenless_modes_to_displays(gpu)) {
            nv_warning_msg("Failed to add screenless modes to GPU-%d '%s'.",
                           NvCtrlGetTargetId(gpu->handle), gpu->name);
            return FALSE;
        }
    }

    return TRUE;

} /* layout_add_screenless_modes_to_displays() */



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
    int tmp;

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

    /* does the driver know about NV_CTRL_CURRENT_METAMODE_ID? */
    ret = NvCtrlGetAttribute(handle, NV_CTRL_CURRENT_METAMODE_ID, &tmp);
    if (ret != NvCtrlSuccess) {
        char *displayName = NvCtrlGetDisplayName(handle);
        *err_str = g_strdup_printf("The NVIDIA X driver on %s is not new\n"
                                   "enough to support the nvidia-settings "
                                   "Display Configuration page.",
                                   displayName ? displayName : "this X server");
        free(displayName);
        nv_warning_msg(*err_str);
        goto fail;
    }

    if (!layout_add_gpus_from_server(layout, err_str)) {
        nv_warning_msg("Failed to add GPU(s) to layout for display "
                       "configuration page.");
        goto fail;
    }

    if (!layout_add_screens_from_server(layout, err_str)) {
        nv_warning_msg("Failed to add screens(s) to layout for display "
                       "configuration page.");
        goto fail;
    }

    if (!layout_add_screenless_modes_to_displays(layout)) {
        nv_warning_msg("Failed to add screenless modes to layout for "
                       "display configuration page.");
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
    nvScreenPtr screen = NULL;
    nvScreenPtr cur;

    if (!layout || !layout->screens) return NULL;

    screen = layout->screens;
    for (cur = screen->next_in_layout; cur; cur = cur->next_in_layout) {
        if (cur->gpu == preferred_gpu) {
            if (screen->gpu != preferred_gpu) {
                screen = cur;
                continue;
            }
        }
        if (screen->scrnum > cur->scrnum) {
            screen = cur;
        }
    }

    return screen;

} /* layout_get_a_screen() */



/** layout_get_display() *********************************************
 *
 * Returns the display with the matching display id or NULL if not
 * found.
 *
 **/
nvDisplayPtr layout_get_display(const nvLayoutPtr layout,
                                const unsigned int display_id)
{
    nvGpuPtr gpu;
    nvDisplayPtr display;

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {

            if (NvCtrlGetTargetId(display->handle) == display_id) {
                return display;
            }
        }
    }

    return NULL;

} /* layout_get_display() */




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
    gchar *err_msg = NULL;
    struct stat st;

    int ret = 0;


    if (!buf || !filename) goto done;

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
                             dlg->merge_toggleable && mergeable);


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

    dlg = malloc(sizeof(SaveXConfDlg));
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
