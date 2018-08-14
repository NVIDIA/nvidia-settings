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
#include <libintl.h>

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

#define _(STRING) gettext(STRING)

static void xconfig_update_buffer(GtkWidget *widget, gpointer user_data);
static gchar *display_pick_config_name(nvDisplayPtr display,
                                       int force_target_id_name);
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
    }
}



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
    }
}



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
            free(tmp);
            goto fail;
        }
        free(tmp);
    }
    free(tmp);

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
 * information from the token-value pair given.
 *
 * Unknown tokens and/or values are silently ignored.
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

    /* Rotation */
    } else if (!strcasecmp("rotation", token)) {
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

    /* Reflection */
    } else if (!strcasecmp("reflection", token)) {
        if (!strcasecmp("x", value)) {
            mode->reflection = REFLECTION_X;
        } else if (!strcasecmp("y", value)) {
            mode->reflection = REFLECTION_Y;
        } else if (!strcasecmp("xy", value)) {
            mode->reflection = REFLECTION_XY;
        }

    /* Pixelshift */
    } else if (!strcasecmp("PixelShiftMode", token)) {
        if (!strcasecmp("4kTopLeft", value)) {
            mode->pixelshift = PIXELSHIFT_4K_TOP_LEFT;
        } else if (!strcasecmp("4kBottomRight", value)) {
            mode->pixelshift = PIXELSHIFT_4K_BOTTOM_RIGHT;
        } else if (!strcasecmp("8k", value)) {
            mode->pixelshift = PIXELSHIFT_8K;
        }

    /* ForceCompositionPipeline */
    } else if (!strcasecmp("forcecompositionpipeline", token)) {
        if (!strcasecmp("on", value)) {
            mode->forceCompositionPipeline = True;
        }

    /* ForceFullCompositionPipeline */
    } else if (!strcasecmp("forcefullcompositionpipeline", token)) {
        if (!strcasecmp("on", value)) {
            mode->forceFullCompositionPipeline = True;
        }

    /* AllowGSYNC */
    } else if (!strcasecmp("allowgsync", token)) {
        if (!strcasecmp("off", value)) {
            mode->allowGSYNC = False;
        }
    }

}



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
    mode->allowGSYNC = True;


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
     * modeline's unrotated dimensions.  Panning should not be rotated
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
    viewPortOut->width  = NV_MAX(viewPortOut->width, 10);
    viewPortOut->height = NV_MAX(viewPortOut->height, 10);
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
static gchar *mode_get_str(nvLayoutPtr layout,
                           nvModePtr mode,
                           int force_target_id_name)
{
    gchar *mode_str;
    gchar *tmp;
    gchar *flags_str;
    nvDisplayPtr display;
    nvScreenPtr screen;
    nvGpuPtr gpu;

    /* Make sure the mode has everything it needs to be displayed */
    if (!mode || !mode->metamode || !mode->display) {
        return NULL;
    }

    display = mode->display;

    /* Don't include dummy modes */
    if (mode->dummy && !mode->modeline) {
        return NULL;
    }

    screen = display->screen;
    gpu = display->gpu;
    if (!screen || !gpu) {
        return NULL;
    }

    /* Pick a suitable display name qualifier */
    mode_str = display_pick_config_name(display, force_target_id_name);
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
    if ((mode->pan.width != mode->viewPortIn.width) ||
        (mode->pan.height != mode->viewPortIn.height)) {
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
     *   Programmability:
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

    if (layout->num_prime_displays > 0) {
        /* Do not reposition the mode if we have PRIME displays */
        tmp = g_strdup_printf("%s +%d+%d", mode_str, mode->pan.x, mode->pan.y);
    } else {
        tmp = g_strdup_printf("%s +%d+%d",
                              mode_str,
                              /* Make mode position relative */
                              mode->pan.x - mode->metamode->edim.x,
                              mode->pan.y - mode->metamode->edim.y);
    }

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
            str = "left";
            break;
        case ROTATION_180:
            str = "invert";
            break;
        case ROTATION_270:
            str = "right";
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

    /* Pixelshift */
    if (mode->pixelshift != PIXELSHIFT_NONE) {
        const char *str = NULL;

        switch (mode->pixelshift) {
        case PIXELSHIFT_4K_TOP_LEFT:
            str = "4kTopLeft";
            break;
        case PIXELSHIFT_4K_BOTTOM_RIGHT:
            str = "4kBottomRight";
            break;
        case PIXELSHIFT_8K:
            str = "8k";
            break;
        default:
            break;
        }

        if (str) {
            tmp = g_strdup_printf("%s, PixelShiftMode=%s",
                                  (flags_str ? flags_str : ""), str);
            g_free(flags_str);
            flags_str = tmp;
        }

    /* ViewPortIn */
    } else {
        int width;
        int height;

        /* Only write out the ViewPortIn if it is specified and differs from the
         * ViewPortOut.  Skip writing ViewPortIn if pixelshift mode was
         * requested, as pixelshift mode overrides any specified ViewPortIn.
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

    /* ForceCompositionPipeline */
    if (mode->forceCompositionPipeline) {
        tmp = g_strdup_printf("%s, ForceCompositionPipeline=On",
                              (flags_str ? flags_str : ""));
        g_free(flags_str);
        flags_str = tmp;
    }

    /* ForceFullCompositionPipeline */
    if (mode->forceFullCompositionPipeline) {
        tmp = g_strdup_printf("%s, ForceFullCompositionPipeline=On",
                              (flags_str ? flags_str : ""));
        g_free(flags_str);
        flags_str = tmp;
    }

    /* AllowGSYNC */
    if (!mode->allowGSYNC) {
        tmp = g_strdup_printf("%s, AllowGSYNC=Off",
                              (flags_str ? flags_str : ""));
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
static gchar *display_pick_config_name(nvDisplayPtr display,
                                       int force_target_id_name)
{
    nvScreenPtr screen;
    nvGpuPtr gpu;

    /* Use target ID name for talking to X server */
    if (force_target_id_name) {
        return g_strdup(display->targetIdName);
    }

    screen = display->screen;
    gpu = display->gpu;


    /* If one of the Mosaic modes is configured, and the X server supports
     * GPU UUIDs, qualify the display device with the GPU UUID.
     */
    if (screen->num_gpus >= 1 &&
        gpu->mosaic_enabled &&
        gpu->uuid) {
        return g_strconcat(gpu->uuid, ".", display->randrName, NULL);
    }

    /* If the X screen is driven by a single display on a single GPU, omit the
     * display name qualifier, so the configuration will be portable.
     */
    if (screen->num_displays == 1 && gpu->num_displays == 1) {
        return g_strdup("");
    }

    /* Otherwise, use the RandR based name */
    return g_strdup(display->randrName);
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
    CtrlTarget *ctrl_target = display->ctrl_target;

    /*
     * check the version of the NV-CONTROL protocol -- versions <=
     * 1.13 had a bug in how they reported double scan modelines
     * (vsyncstart, vsyncend, and vtotal were doubled); determine
     * if this X server has this bug, so that we can use
     * broken_doublescan_modelines to correctly compute the
     * refresh rate.
     */
    broken_doublescan_modelines = 1;

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_ATTR_NV_MAJOR_VERSION, &major);
    ret1 = NvCtrlGetAttribute(ctrl_target,
                              NV_CTRL_ATTR_NV_MINOR_VERSION, &minor);

    if ((ret == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        ((major > 1) || ((major == 1) && (minor > 13)))) {
        broken_doublescan_modelines = 0;
    }


    /* Free any old mode lines */
    display_remove_modelines(display);


    /* Get the validated modelines for the display */
    ret = NvCtrlGetBinaryAttribute(ctrl_target, 0,
                                   NV_CTRL_BINARY_DATA_MODELINES,
                                   (unsigned char **)&modeline_strs, &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query modelines of display "
                                  "device %d '%s'.",
                                   NvCtrlGetTargetId(ctrl_target),
                                   display->logName);
        nv_error_msg("%s", *err_str);
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
                                       NvCtrlGetTargetId(ctrl_target),
                                       display->logName,
                                       str);
            nv_error_msg("%s", *err_str);
            goto fail;
        }

        /* Add the modeline at the end of the display's modeline list */
        xconfigAddListItem((GenericListPtr *)(&display->modelines),
                           (GenericListPtr)modeline);
        display->num_modelines++;

        /* Get next modeline string */
        str += strlen(str) +1;
    }

    free(modeline_strs);
    return TRUE;


    /* Handle the failure case */
 fail:
    display_remove_modelines(display);
    free(modeline_strs);
    return FALSE;

} /* display_add_modelines_from_server() */



/** display_get_mode_str() *******************************************
 *
 * Returns the mode string of the display's 'mode_idx''s
 * mode.
 *
 **/
static gchar *display_get_mode_str(nvLayoutPtr layout, nvDisplayPtr display,
                                   int mode_idx, int force_target_id_name)
{
    nvModePtr mode = display->modes;

    while (mode && mode_idx) {
        mode = mode->next;
        mode_idx--;
    }

    if (mode) {
        return mode_get_str(layout, mode, force_target_id_name);
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
        free(display->logName);
        free(display->typeBaseName);
        free(display->typeIdName);
        free(display->dpGuidName);
        free(display->edidHashName);
        free(display->targetIdName);
        free(display->randrName);
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



/** get_screen_max_displays () ***************************************
 *
 * Returns the maximum number of allowable enabled displays for the X screen.
 * This is based on the screen's driving GPU's max number of enabled displays,
 * in conjunction with whether or not Mosaic is enabled and which type.
 * Surround (Base Mosaic) only supports up to 3 enabled display devices,
 * while other modes (Base Mosaic and SLI Mosaic) support unlimited displays.
 *
 **/

int get_screen_max_displays(nvScreenPtr screen)
{
    nvGpuPtr gpu = screen->display_owner_gpu;

    /* If mosaic is enabled, check the type so we can properly limit the number
     * of display devices
     */
    if (gpu->mosaic_enabled) {
        if (gpu->mosaic_type == MOSAIC_TYPE_BASE_MOSAIC_LIMITED) {
            return 3;
        }
        return -1; /* Not limited */
    }

    return screen->max_displays;
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
    CtrlTargetNode *node;

    for (node = screen->ctrl_target->relations; node; node = node->next) {
        CtrlTarget *ctrl_target = node->t;
        nvDisplayPtr display;

        if (NvCtrlGetTargetType(ctrl_target) != DISPLAY_TARGET) {
            continue;
        }

        display = layout_get_display(screen->layout,
                                     NvCtrlGetTargetId(ctrl_target));
        if (!display) {
            nv_warning_msg("Failed to find display %d assigned to X screen "
                           " %d.",
                           NvCtrlGetTargetId(ctrl_target),
                           NvCtrlGetTargetId(screen->ctrl_target));
            continue;
        }

        screen_link_display(screen, display);
    }
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
                               int force_target_id_name)
{
    nvDisplayPtr display;

    gchar *metamode_str = NULL;
    gchar *mode_str;
    gchar *tmp;

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        mode_str = display_get_mode_str(screen->layout, display, metamode_idx,
                                        force_target_id_name);
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

    if (!metamode_str) {
        metamode_str = strdup("NULL");
    }

    return metamode_str;

} /* screen_get_metamode_str() */



/** cleanup_metamode() ***********************************************
 *
 * Frees any internal memory used by the metamode.
 *
 **/

void cleanup_metamode(nvMetaModePtr metamode)
{
    free(metamode->cpl_str);
    metamode->cpl_str = NULL;

    free(metamode->x_str);
    metamode->x_str = NULL;

    metamode->x_str_entry = NULL;
}



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
        cleanup_metamode(metamode);
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
 * delimited by commas.
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
    nvMetaModePtr metamode = NULL;
    int mode_count = 0;

    if (!screen || !metamode_str) {
        goto fail;
    }


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

    metamode_modes = parse_skip_whitespace(metamode_modes);

    if (strcmp(metamode_modes, "NULL")) {
        /* Process each mode in the metamode string */
        char *metamode_copy = strdup(metamode_modes);
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
                nv_warning_msg("Failed to read a display device name on screen "
                               "%d while parsing metamode:\n\n'%s'",
                               screen->scrnum, orig_mode_str);
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

        /* Make sure something was added */
        if (mode_count == 0) {
            nv_warning_msg("Failed to find any display on screen %d\n"
                           "while parsing metamode:\n\n'%s'",
                           screen->scrnum, metamode_str);
            goto fail;
        }
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

        /* Each display must have as many modes as its screen has metamodes */
        while (metamode) {

            /* Create a dummy mode */
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
    ret = NvCtrlGetBinaryAttribute(screen->ctrl_target, 0,
                                   NV_CTRL_BINARY_DATA_METAMODES_VERSION_2,
                                   (unsigned char **)&metamode_strs,
                                   &len);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query list of metamodes on\n"
                                   "screen %d.", screen->scrnum);
        nv_error_msg("%s", *err_str);
        goto fail;
    }


    /* Get the current metamode for the screen */
    ret = NvCtrlGetStringAttribute(screen->ctrl_target,
                                   NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                                   &cur_metamode_str);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query current metamode of\n"
                                   "screen %d.", screen->scrnum);
        nv_error_msg("%s", *err_str);
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
            nv_warning_msg("Failed to add metamode '%s' to screen %d.",
                           str, screen->scrnum);
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
    free(metamode_strs);
    metamode_strs = NULL;

    if (!screen->metamodes) {
        nv_warning_msg("Failed to add any metamode to screen %d.",
                       screen->scrnum);
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

    free(metamode_strs);
    return FALSE;

} /* screen_add_metamodes() */



/** screen_remove_prime_displays() ***********************************
 *
 * Removes all prime displays currently associated with this screen.
 *
 **/
static void screen_remove_prime_displays(nvScreenPtr screen)
{
    if (!screen) return;

    while (screen->prime_displays) {
        screen_unlink_prime_display(screen->prime_displays);
    }

} /* screen_remove_prime_displays() */



/** screen_free() ****************************************************
 *
 * Frees memory used by a screen structure
 *
 **/
static void screen_free(nvScreenPtr screen)
{
    if (screen) {
        ctk_event_destroy(G_OBJECT(screen->ctk_event));

        screen_remove_metamodes(screen);
        screen_remove_displays(screen);
        screen_remove_prime_displays(screen);

        nvfree(screen->gpus);
        screen->num_gpus = 0;

        free(screen->sli_mode);
        free(screen->multigpu_mode);
        free(screen);
    }

} /* screen_free() */



/** link_screen_to_gpu() *********************************************
 *
 * Updates the X screen to track the given GPU as a driver.
 *
 **/
void link_screen_to_gpu(nvScreenPtr screen, nvGpuPtr gpu)
{
    screen->num_gpus++;
    screen->gpus = nvrealloc(screen->gpus,
                             screen->num_gpus * sizeof(nvGpuPtr));

    screen->gpus[screen->num_gpus -1] = gpu;

    /* Consolidate screen's capabilities based on all GPUs involved */
    if (screen->num_gpus == 1) {
        screen->max_width = gpu->max_width;
        screen->max_height = gpu->max_height;
        screen->max_displays = gpu->max_displays;
        screen->allow_depth_30 = gpu->allow_depth_30;
        screen->display_owner_gpu = gpu;
        return;
    }

    screen->max_width = MIN(screen->max_width, gpu->max_width);
    screen->max_height = MIN(screen->max_height, gpu->max_height);
    screen->allow_depth_30 = screen->allow_depth_30 && gpu->allow_depth_30;

    if (screen->max_displays <= 0) {
        screen->max_displays = gpu->max_displays;
    } else if (gpu->max_displays > 0) {
        screen->max_displays = MIN(screen->max_displays, gpu->max_displays);
    }

    /* Set the display owner GPU. */
    if (screen->display_owner_gpu_id >= 0) {
        /* Link to the multi GPU display owner, if it is specified */
        if (screen->display_owner_gpu_id == NvCtrlGetTargetId(gpu->ctrl_target)) {
            screen->display_owner_gpu = gpu;
        }
    } else if (gpu->multigpu_master_possible &&
               !screen->display_owner_gpu->multigpu_master_possible) {
        /* Pick the best GPU to be the display owner.  This is the first
         * GPU that can be a multigpu master, or the first linked GPU,
         * if none of the GPU(s) can be set as master.
         */
        screen->display_owner_gpu = gpu;
    }
}



/** screen_has_gpu() *************************************************
 *
 * Returns whether or not the screen is driven by the given GPU.
 *
 **/

Bool screen_has_gpu(nvScreenPtr screen, nvGpuPtr gpu)
{
    int i;

    if (!gpu) {
        return FALSE;
    }

    for (i = 0; i < screen->num_gpus; i++) {
        if (gpu == screen->gpus[i]) {
            return TRUE;
        }
    }

    return FALSE;
}



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

    ret1 = NvCtrlGetDisplayAttribute(gpu->ctrl_target,
                                     mode_id,
                                     NV_CTRL_GVIO_VIDEO_FORMAT_REFRESH_RATE,
                                     &rate);

    ret2 = NvCtrlGetStringDisplayAttribute(gpu->ctrl_target,
                                           mode_id,
                                           NV_CTRL_STRING_GVIO_VIDEO_FORMAT_NAME,
                                           &name);

    if ((ret1 == NvCtrlSuccess) && (ret2 == NvCtrlSuccess)) {
        data->id = mode_id;
        data->rate = rate;
        data->name = name;
        return TRUE;
    }

    free(name);
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

    ret = NvCtrlGetStringAttribute(display->ctrl_target,
                                   displayNameInfo->attr,
                                   &str);
    if (ret == NvCtrlSuccess) {
        *((char **)(((char *)display) + displayNameInfo->offset)) = str;

    } else if (!displayNameInfo->canBeNull) {
        *err_str = g_strdup_printf("Failed to query name '%s' of display "
                                   "device DPY-%d.",
                                   displayNameInfo->nameDescription,
                                   NvCtrlGetTargetId(display->ctrl_target));
        nv_error_msg("%s", *err_str);
        return FALSE;
    }

    return TRUE;
}



/** gpu_add_display_from_server() ************************************
 *
 *  Adds the display with the device id given to the GPU structure.
 *
 **/
static nvDisplayPtr gpu_add_display_from_server(nvGpuPtr gpu,
                                                CtrlTarget *ctrl_target,
                                                gchar **err_str)
{
    ReturnStatus ret;
    nvDisplayPtr display;
    int i;


    /* Create the display structure */
    display = (nvDisplayPtr)calloc(1, sizeof(nvDisplay));
    if (!display) goto fail;

    display->ctrl_target = ctrl_target;


    /* Query the display information */
    for (i = 0; i < ARRAY_LEN(DisplayNamesTable); i++) {
        if (!display_add_name_from_server(display,
                                          DisplayNamesTable + i, err_str)) {
            goto fail;
        }
    }


    /* Query if this display is an SDI display */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_IS_GVO_DISPLAY,
                             &(display->is_sdi));
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query if display device\n"
                       "%d connected to GPU-%d '%s' is an\n"
                       "SDI device.",
                       NvCtrlGetTargetId(ctrl_target),
                       NvCtrlGetTargetId(gpu->ctrl_target),
                       gpu->name);
        display->is_sdi = FALSE;
    }


    /* Load the SDI mode table so we can report accurate refresh rates. */
    if (display->is_sdi && !gpu->gvo_mode_data) {
        unsigned int valid1 = 0;
        unsigned int valid2 = 0;
        unsigned int valid3 = 0;
        CtrlAttributeValidValues valid;

        ret = NvCtrlGetValidAttributeValues(gpu->ctrl_target,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
            valid1 = 0;
        } else {
            valid1 = valid.allowed_ints;
        }

        ret = NvCtrlGetValidAttributeValues(gpu->ctrl_target,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT2,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
            valid2 = 0;
        } else {
            valid2 = valid.allowed_ints;
        }

        ret = NvCtrlGetValidAttributeValues(gpu->ctrl_target,
                                            NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT3,
                                            &valid);
        if ((ret != NvCtrlSuccess) ||
            (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
            valid3 = 0;
        } else {
            valid3 = valid.allowed_ints;
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
                       NvCtrlGetTargetId(ctrl_target), display->logName,
                       NvCtrlGetTargetId(gpu->ctrl_target), gpu->name);
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
    CtrlTargetNode *node;

    gpu_remove_displays(gpu);

    for (node = gpu->ctrl_target->relations; node; node = node->next) {
        CtrlTarget *ctrl_target = node->t;

        if (NvCtrlGetTargetType(ctrl_target) != DISPLAY_TARGET ||
            !(ctrl_target->display.connected)) {
            continue;
        }

        if (!gpu_add_display_from_server(gpu, ctrl_target, err_str)) {
            nv_warning_msg("Failed to add display device %d to GPU-%d "
                           "'%s'.",
                           NvCtrlGetTargetId(ctrl_target),
                           NvCtrlGetTargetId(gpu->ctrl_target),
                           gpu->name);
            goto fail;
        }
    }
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

        ctk_event_destroy(G_OBJECT(gpu->ctk_event));

        free(gpu->name);
        free(gpu->uuid);
        free(gpu->flags_memory);
        g_free(gpu->pci_bus_id);
        free(gpu->gvo_mode_data);
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



/** layout_add_prime_display() **********************************************
 *
 * Adds a prime display to the (end of the) layout's list of prime displays.
 *
 **/
static void layout_add_prime_display(nvLayoutPtr layout,
                                     nvPrimeDisplayPtr prime)
{
    prime->layout = layout;
    prime->next_in_layout = NULL;

    if (!layout->prime_displays) {
        layout->prime_displays = prime;
        layout->num_prime_displays++;
    } else {
        nvPrimeDisplayPtr last;
        for (last = layout->prime_displays; last; last = last->next_in_layout) {
            if (!last->next_in_layout) {
                last->next_in_layout = prime;
                layout->num_prime_displays++;
                break;
            }
        }
    }

} /* layout_add_prime_display() */



/** layout_remove_and_free_screen() **********************************
 *
 * Removes a screen from the layout and frees it.
 *
 **/

void layout_remove_and_free_screen(nvScreenPtr screen)
{
    nvLayoutPtr layout;
    nvScreenPtr other;

    if (!screen) return;

    layout = screen->layout;


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



/** layout_remove_prime_displays() ************************************
 *
 * Removes all PRIME displays from the layout structure.
 *
 **/
static void layout_remove_prime_displays(nvLayoutPtr layout)
{
    nvPrimeDisplayPtr prime;

    if (!layout) return;

    while (layout->prime_displays) {
        prime = layout->prime_displays;
        layout->prime_displays = prime->next_in_layout;

        free(prime->label);
        free(prime);
    }
    layout->num_prime_displays = 0;
}



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




/** screen_link_prime_display() **************************************
 *
 * Makes the given PRIME display part of the screen
 *
 **/
void screen_link_prime_display(nvScreenPtr screen,
                               nvPrimeDisplayPtr prime)
{

    if (!prime || !screen || (prime->screen == screen)) return;

    prime->screen = screen;
    prime->next_in_screen = NULL;

    /* Add the prime display at the end of the screen's prime display list */
    if (!screen->prime_displays) {
        screen->prime_displays = prime;
    } else {
        nvPrimeDisplayPtr cur;
        for (cur = screen->prime_displays; cur; cur = cur->next_in_screen) {
            if (!cur->next_in_screen) {
                cur->next_in_screen = prime;
                break;
            }
        }
    }
    screen->num_prime_displays++;

} /* screen_link_prime_display() */



/** screen_unlink_prime_display() ************************************
 *
 * Removes the PRIME display from the screen's list of PRIME displays
 *
 **/
void screen_unlink_prime_display(nvPrimeDisplayPtr prime)
{
    nvScreenPtr screen;

    if (!prime|| !prime->screen) return;

    screen = prime->screen;

    /* Remove the prime display from the screen */
    if (screen->prime_displays == prime) {
        screen->prime_displays = prime->next_in_screen;
    } else {
        nvPrimeDisplayPtr cur = screen->prime_displays;
        while (cur) {
            if (cur->next_in_screen == prime) {
                cur->next_in_screen = prime->next_in_screen;
                break;
            }
            cur = cur->next_in_screen;
        }
    }
    screen->num_prime_displays--;

    prime->screen = NULL;

}





/** add_prime_display_from_server() **************************
 *
 *  Adds all PRIME displays associated with a screen to the screen and layout.
 *
 **/
static
nvPrimeDisplayPtr add_prime_display_from_server(nvScreenPtr screen,
                                                char* src_info_str)
{
    nvLayoutPtr layout = screen->layout;
    char * tok;
    nvPrimeDisplayPtr prime;
    char *info_str = g_strdup(src_info_str);

    prime = (nvPrimeDisplayPtr)calloc(1, sizeof(nvPrimeDisplay));
    if (!prime) goto fail;

    prime->screen_num = -1;

    tok = strtok(info_str, ",");
    while (tok) {
        char *value = strchr(tok, '=');
        if (value && strlen(value) >= 2 ) {
            *value = '\0';
            value++;

            while (*tok == ' ') {
                tok++;
            }

            if (!strcmp(tok, "width")) {         // required
                prime->rect.width = atoi(value);
            } else if (!strcmp(tok, "height")) { // required
                prime->rect.height = atoi(value);
            } else if (!strcmp(tok, "xpos")) {   // required
                prime->rect.x = atoi(value);
            } else if (!strcmp(tok, "ypos")) {   // required
                prime->rect.y = atoi(value);
            } else if (!strcmp(tok, "screen")) {
                prime->screen_num = atoi(value);
            } else if (!strcmp(tok, "name")) {
                prime->label = g_strdup(value);
            }
        }
        tok = strtok(NULL, ",");
    }

    layout_add_prime_display(layout, prime);
    screen_link_prime_display(screen, prime);

    g_free(info_str);
    return prime;

 fail:
    free(prime);

    g_free(info_str);
    return NULL;
}


/** layout_add_prime_displays_from_server() **************************
 *
 * Adds PRIME displays to the layout for all screens.
 *
 **/
static void layout_add_prime_displays_from_server(nvLayoutPtr layout)
{
    char *prime_outputs_data_str = NULL;
    nvScreenPtr screen;
    ReturnStatus ret;
    char *start;
    char *end;

    layout_remove_prime_displays(layout);

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        ret = NvCtrlGetStringAttribute(screen->ctrl_target,
                                       NV_CTRL_STRING_PRIME_OUTPUTS_DATA,
                                       &prime_outputs_data_str);

        if (ret != NvCtrlSuccess) {
            continue;
        }

        start = prime_outputs_data_str;
        end = strchr(start, ';');
        while (start && *start && end && start < end) {
            *end = '\0';
            add_prime_display_from_server(screen, start);

            start = end + 1;
            end = strchr(start, ';');
        }
        free(prime_outputs_data_str);
    }
}

/** layout_add_gpu_from_server() *************************************
 *
 * Adds a GPU to the layout structure.
 *
 **/
static Bool layout_add_gpu_from_server(nvLayoutPtr layout,
                                       CtrlTarget *ctrl_target,
                                       gchar **err_str)
{
    ReturnStatus ret;
    nvGpuPtr gpu = NULL;
    unsigned int *pData;
    int len;
    int val;


    /* Create the GPU structure */
    gpu = (nvGpuPtr)calloc(1, sizeof(nvGpu));
    if (!gpu) goto fail;

    gpu->layout = layout;
    gpu->ctrl_target = ctrl_target;
    gpu->ctk_event = CTK_EVENT(ctk_event_new(ctrl_target));


    /* Query the GPU information */
    ret = NvCtrlGetStringAttribute(ctrl_target, NV_CTRL_STRING_PRODUCT_NAME,
                                   &gpu->name);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query GPU name of GPU-%d.",
                                   NvCtrlGetTargetId(ctrl_target));
        nv_error_msg("%s", *err_str);
        goto fail;
    }

    ret = NvCtrlGetStringAttribute(ctrl_target, NV_CTRL_STRING_GPU_UUID,
                                   &gpu->uuid);
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query GPU UUID of GPU-%d '%s'.  GPU UUID "
                       "qualifiers will not be used.",
                       NvCtrlGetTargetId(ctrl_target),
                       gpu->name);
        gpu->uuid = NULL;
    }

    gpu->pci_bus_id = get_bus_id_str(ctrl_target);

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_MAX_SCREEN_WIDTH,
                             (int *)&(gpu->max_width));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX SCREEN WIDTH on "
                                   "GPU-%d '%s'.",
                                   NvCtrlGetTargetId(ctrl_target),
                                   gpu->name);
        nv_error_msg("%s", *err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_MAX_SCREEN_HEIGHT,
                             (int *)&(gpu->max_height));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX SCREEN HEIGHT on "
                                   "GPU-%d '%s'.",
                                   NvCtrlGetTargetId(ctrl_target),
                                   gpu->name);
        nv_error_msg("%s", *err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_MAX_DISPLAYS,
                             (int *)&(gpu->max_displays));
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup_printf("Failed to query MAX DISPLAYS on "
                                   "GPU-%d '%s'.",
                                   NvCtrlGetTargetId(ctrl_target),
                                   gpu->name);
        nv_error_msg("%s", *err_str);
        goto fail;
    }

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_DEPTH_30_ALLOWED,
                             &(gpu->allow_depth_30));

    if (ret != NvCtrlSuccess) {
        gpu->allow_depth_30 = FALSE;
    }

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_MULTIGPU_MASTER_POSSIBLE,
                             &(gpu->multigpu_master_possible));
    if (ret != NvCtrlSuccess) {
        gpu->multigpu_master_possible = FALSE;
    }

    ret = NvCtrlGetBinaryAttribute(ctrl_target,
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


    /* Determine available and current Mosaic configuration */

    gpu->mosaic_type = MOSAIC_TYPE_UNSUPPORTED;
    gpu->mosaic_enabled = FALSE;

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE,
                             &(val));

    if ((ret == NvCtrlSuccess) &&
        (val == NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE_TRUE)) {
        char *sli_str;

        gpu->mosaic_type = MOSAIC_TYPE_SLI_MOSAIC;

        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_SLI_MODE,
                                       &sli_str);
        if ((ret == NvCtrlSuccess) && sli_str) {
            if (!strcasecmp(sli_str, "Mosaic")) {
                gpu->mosaic_enabled = TRUE;
            }
            free(sli_str);
        }

    } else {
        CtrlAttributeValidValues valid;

        ret = NvCtrlGetValidAttributeValues(ctrl_target,
                                            NV_CTRL_BASE_MOSAIC,
                                            &valid);
        if ((ret == NvCtrlSuccess) &&
            (valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {

            if (valid.allowed_ints & (1 << NV_CTRL_BASE_MOSAIC_FULL)) {
                gpu->mosaic_type = MOSAIC_TYPE_BASE_MOSAIC;
            } else if (valid.allowed_ints &
                       (1 << NV_CTRL_BASE_MOSAIC_LIMITED)) {
                gpu->mosaic_type = MOSAIC_TYPE_BASE_MOSAIC_LIMITED;
            }

            if (gpu->mosaic_type != MOSAIC_TYPE_UNSUPPORTED) {
                ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_BASE_MOSAIC,
                                         &(val));
                if ((ret == NvCtrlSuccess) &&
                    (val == NV_CTRL_BASE_MOSAIC_FULL ||
                     val == NV_CTRL_BASE_MOSAIC_LIMITED)) {
                    gpu->mosaic_enabled = TRUE;
                }
            }
        }
    }

    /* Add the display devices to the GPU */
    if (!gpu_add_displays_from_server(gpu, err_str)) {
        nv_warning_msg("Failed to add displays to GPU-%d '%s'.",
                       NvCtrlGetTargetId(ctrl_target),
                       gpu->name);
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
    CtrlTargetNode *node;

    layout_remove_gpus(layout);

    for (node = layout->system->targets[GPU_TARGET]; node; node = node->next) {
        CtrlTarget *ctrl_target = node->t;

        if (!layout_add_gpu_from_server(layout, ctrl_target, err_str)) {
            nv_warning_msg("Failed to add GPU-%d to layout.",
                           NvCtrlGetTargetId(ctrl_target));
            goto fail;
        }
    }

    return layout->num_gpus;


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



/** link_screen_to_gpus() ********************************************
 *
 * Finds the GPU(s) driving the screen and tracks the link(s).
 *
 **/

static Bool link_screen_to_gpus(nvLayoutPtr layout, nvScreenPtr screen)
{
    Bool status = FALSE;
    ReturnStatus ret;
    int *pData = NULL;
    int len;
    int i;
    int scrnum = NvCtrlGetTargetId(screen->ctrl_target);

    /* Link the screen to the display owner GPU.  If there is no display owner,
     * which is the case when SLI Mosaic is configured, link the screen to the
     * first (multi gpu master possible) GPU we find.
     */
    ret = NvCtrlGetAttribute(screen->ctrl_target,
                             NV_CTRL_MULTIGPU_DISPLAY_OWNER,
                             &(screen->display_owner_gpu_id));
    if (ret != NvCtrlSuccess) {
        screen->display_owner_gpu_id = -1;
    }

    ret = NvCtrlGetBinaryAttribute(screen->ctrl_target,
                                   0,
                                   NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                                   (unsigned char **)(&pData),
                                   &len);
    if ((ret != NvCtrlSuccess) || !pData || (pData[0] < 1)) {
        goto done;
    }

    /* Point to all the gpus */

    for (i = 0; i < pData[0]; i++) {
        nvGpuPtr gpu;

        for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
            int gpuid = NvCtrlGetTargetId(gpu->ctrl_target);

            if (gpuid != pData[1+i]) {
                continue;
            }

            link_screen_to_gpu(screen, gpu);
        }
    }

    /* Make sure a display owner was picked */
    if (screen->num_gpus <= 0) {
        nv_error_msg("Failed to link X screen %d to any GPU.", scrnum);
        goto done;
    }

    status = TRUE;

 done:
    free(pData);

    return status;
}



/** layout_add_screen_from_server() **********************************
 *
 * Adds an X screen to the layout structure.
 *
 **/
static Bool layout_add_screen_from_server(nvLayoutPtr layout,
                                          CtrlTarget *ctrl_target,
                                          gchar **err_str)
{
    nvScreenPtr screen;
    int val, tmp;
    ReturnStatus ret;
    gchar *primary_str = NULL;
    gchar *screen_info = NULL;
    GdkRectangle screen_parsed_info;


    screen = (nvScreenPtr)calloc(1, sizeof(nvScreen));
    if (!screen) goto fail;

    screen->ctrl_target = ctrl_target;
    screen->scrnum = NvCtrlGetTargetId(ctrl_target);


    /* Query the current stereo mode */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_STEREO, &val);
    if (ret == NvCtrlSuccess) {
        screen->stereo_supported = TRUE;
        screen->stereo = val;
    } else {
        screen->stereo_supported = FALSE;
    }

    /* Query the current overlay state */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_OVERLAY, &val);
    if (ret == NvCtrlSuccess) {
        screen->overlay = val;
    } else {
        screen->overlay = NV_CTRL_OVERLAY_OFF;
    }

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_HWOVERLAY, &val);
    if (ret == NvCtrlSuccess) {
        screen->hw_overlay = val;
    } else {
        screen->hw_overlay = NV_CTRL_HWOVERLAY_FALSE;
    }

    /* Query the current UBB state */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_UBB, &val);
    if (ret == NvCtrlSuccess) {
        screen->ubb = val;
    } else {
        screen->ubb = NV_CTRL_UBB_OFF;
    }

    /* See if the screen is set to not scanout */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_NO_SCANOUT, &val);
    if (ret != NvCtrlSuccess) {
        /* Don't make it a fatal error if NV_CTRL_NO_SCANOUT can't be
         * queried, since some drivers may not support this attribute. */

        val = NV_CTRL_NO_SCANOUT_DISABLED;

        *err_str = g_strdup_printf("Failed to query NoScanout for "
                                   "screen %d.",
                                   NvCtrlGetTargetId(ctrl_target));
        nv_warning_msg("%s", *err_str);

        g_free(*err_str);
        *err_str = NULL;
    }
    screen->no_scanout = (val == NV_CTRL_NO_SCANOUT_ENABLED);


    /* Link screen to the GPUs driving it */
    if (!link_screen_to_gpus(layout, screen)) {
        *err_str = g_strdup_printf("Failed to find GPU that drives screen %d.",
                                   NvCtrlGetTargetId(ctrl_target));
        nv_warning_msg("%s", *err_str);
        goto fail;
    }

    /* Query SLI status */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,
                             &tmp);

    screen->sli = (ret == NvCtrlSuccess);

    /* Query SLI mode */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_SLI_MODE,
                                   &screen->sli_mode);
    if (ret != NvCtrlSuccess) {
        screen->sli_mode = NULL;
    }

    /* Query MULTIGPU mode */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_MULTIGPU_MODE,
                                   &screen->multigpu_mode);
    if (ret != NvCtrlSuccess) {
        screen->multigpu_mode = NULL;
    }

    /* Listen to NV-CONTROL events on this screen handle */
    screen->ctk_event = CTK_EVENT(ctk_event_new(ctrl_target));

    /* Query the depth of the screen */
    screen->depth = NvCtrlGetScreenPlanes(ctrl_target);

    /* Initialize the virtual X screen size */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_SCREEN_RECTANGLE,
                                   &screen_info);
    if (ret != NvCtrlSuccess) {
        screen_info = NULL;
    }

    if (screen_info) {

        /* Parse the positioning information */
        screen_parsed_info.width = -1;
        screen_parsed_info.height = -1;

        parse_token_value_pairs(screen_info, apply_screen_info_token,
                                &screen_parsed_info);

        if (screen_parsed_info.width >= 0 &&
            screen_parsed_info.height >= 0) {

            screen->dim.width = screen_parsed_info.width;
            screen->dim.height = screen_parsed_info.height;
        }
        free(screen_info);
    }

    /* Add the screen to the layout */
    layout_add_screen(layout, screen);

    /* Link displays to the screen */
    screen_link_displays(screen);

    /* Parse the screen's metamodes (ties displays on the gpu to the screen) */
    if (!screen->no_scanout) {
        if (!screen_add_metamodes(screen, err_str)) {
            nv_warning_msg("Failed to add metamodes to screen %d.",
                           NvCtrlGetTargetId(ctrl_target));
            goto fail;
        }

        /* Query & parse the screen's primary display */
        screen->primaryDisplay = NULL;
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER,
                                       &primary_str);

        if (ret == NvCtrlSuccess && primary_str) {
            char *str;

            /* The TwinView Xinerama Info Order string may be a comma-separated
             * list of display device names, though we could add full support
             * for ordering these, just keep track of a single display here.
             */
            str = strchr(primary_str, ',');
            if (!str) {
                str = nvstrdup(primary_str);
            } else {
                str = nvstrndup(primary_str, str-primary_str);
            }
            free(primary_str);

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
    CtrlTargetNode *node;

    layout_remove_screens(layout);

    for (node = layout->system->physical_screens;
         node;
         node = node->next) {
        CtrlTarget *ctrl_target = node->t;

        if (!layout_add_screen_from_server(layout, ctrl_target, err_str)) {
            nv_warning_msg("Failed to add X screen %d to layout.",
                           NvCtrlGetTargetId(ctrl_target));
            g_free(*err_str);
            *err_str = NULL;
        }
    }

    return layout->num_screens;

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
                           NvCtrlGetTargetId(gpu->ctrl_target), gpu->name);
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
        layout_remove_screens(layout);
        layout_remove_gpus(layout);
        layout_remove_prime_displays(layout);
        NvCtrlFreeAllSystems(&layout->systems);
        free(layout);
    }

} /* layout_free() */



/** layout_load_from_server() ****************************************
 *
 * Loads layout information from the X server.
 *
 **/
nvLayoutPtr layout_load_from_server(CtrlTarget *ctrl_target,
                                    gchar **err_str)
{
    nvLayoutPtr layout = NULL;
    ReturnStatus ret;
    int tmp;

    /* Allocate the layout structure */
    layout = (nvLayoutPtr)calloc(1, sizeof(nvLayout));
    if (!layout) goto fail;

    /* Duplicate the connection to the system */
    layout->system = NvCtrlConnectToSystem(ctrl_target->system->display,
                                           &(layout->systems));
    if (layout->system == NULL) {
        goto fail;
    }

    /* Is Xinerama enabled? */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_XINERAMA,
                             &layout->xinerama_enabled);
    if (ret != NvCtrlSuccess) {
        *err_str = g_strdup("Failed to query status of Xinerama.");
        nv_error_msg("%s", *err_str);
        goto fail;
    }

    /* does the driver know about NV_CTRL_CURRENT_METAMODE_ID? */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_CURRENT_METAMODE_ID, &tmp);
    if (ret != NvCtrlSuccess) {
        char *displayName = NvCtrlGetDisplayName(ctrl_target);
        *err_str = g_strdup_printf("The NVIDIA X driver on %s is not new\n"
                                   "enough to support the nvidia-settings "
                                   "Display Configuration page.",
                                   displayName ? displayName : "this X server");
        free(displayName);
        nv_warning_msg("%s", *err_str);
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

    layout_add_prime_displays_from_server(layout);

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
    Bool found_preferred_gpu;

    if (!layout || !layout->screens) return NULL;

    screen = layout->screens;
    found_preferred_gpu = screen_has_gpu(screen, preferred_gpu);

    for (cur = screen->next_in_layout; cur; cur = cur->next_in_layout) {
        Bool gpu_match = screen_has_gpu(cur, preferred_gpu);

        /* Pick screens that are driven by the preferred gpu */
        if (gpu_match) {
            if (!found_preferred_gpu) {
                screen = cur;
                continue;
            }
        } else if (found_preferred_gpu) {
            continue;
        }

        /* Pick lower numbered screens */
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

    if (!layout) {
        return NULL;
    }

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {

            if (NvCtrlGetTargetId(display->ctrl_target) == display_id) {
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
              g_strdup_printf(_("You do not have adequate permission to"
              " open the existing X configuration file '%s' for writing."),
              filename);

            /* Verify the user permissions */
            if (stat(filename, &st) == 0) {
                if ((getuid() != 0) && (st.st_uid == 0) &&
                    !(st.st_mode & (S_IWGRP | S_IWOTH)))
                    err_msg = g_strconcat(err_msg, _(" You must be 'root'"
                    " to modify the file."), NULL);
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
                    g_strdup_printf(_("Unable to remove old X config backup "
                                    "file '%s'."),
                                    backup_filename);
                goto done;
            }
        }

        /* Make the current x config file the backup */
        if (rename(filename, backup_filename)) {
                err_msg =
                    g_strdup_printf(_("Unable to create new X config backup "
                                    "file '%s'."),
                                    backup_filename);
            goto done;
        }
    }

    /* Write out the X config file */
    fp = fopen(filename, "w");
    if (!fp) {
        err_msg =
            g_strdup_printf(_("Unable to open X config file '%s' for writing."),
                            filename);
        goto done;
    }
    fprintf(fp, "%s", buf);

    ret = 1;

 done:
    /* Display any errors that might have occurred */
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

    char *tmp_filename = NULL;
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
            err_msg = g_strdup_printf(_("Invalid file '%s': File exits but is a "
                                      "%s!"),
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
                err_msg = g_strdup_printf(_("Failed to parse existing X "
                                          "config file '%s'!"),
                                          filename);
                ctk_display_warning_msg
                    (ctk_get_parent_window(GTK_WIDGET(dlg->parent)), err_msg);

                xconfCur = NULL;

            } else {

                /* Sanitize the X config file */
                xconfigGenerateLoadDefaultOptions(&gop);
                xconfigGetXServerInUse(&gop);

                if (!xconfigSanitizeConfig(xconfCur, NULL, &gop)) {
                    err_msg = g_strdup_printf(_("Failed to sanitize existing X "
                                              "config file '%s'!"),
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

            /* If we're not actually doing a merge, close the file */
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
        err_msg = g_strdup_printf(_("Failed to generate X config file!"));
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
        err_msg = g_strdup_printf(_("Failed to create temp X config file '%s' "
                                  "for preview."),
                                  tmp_filename);
        goto fail;
    }
    xconfigWriteConfigFile(tmp_filename, xconfGen);
    xconfigFreeConfig(&xconfGen);

    if (lseek(tmp_fd, 0, SEEK_SET) == (off_t)-1) {
        err_msg = g_strdup_printf(_("Failed lseek() on temp X config file '%s' "
                                  "for preview."),
                                  tmp_filename);
        goto fail;
    }
    if (fstat(tmp_fd, &st) == -1) {
        err_msg = g_strdup_printf(_("Failed fstat() on temp X config file '%s' "
                                  "for preview."),
                                  tmp_filename);
        goto fail;
    }
    buf = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, tmp_fd, 0);
    if (buf == MAP_FAILED) {
        err_msg = g_strdup_printf(_("Failed mmap() on temp X config file '%s' "
                                  "for preview."),
                                  tmp_filename);
        goto fail;
    }

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

    g_free(tmp_filename);

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
    gboolean show = !ctk_widget_get_visible(dlg->box_xconfig_save);

    if (show) {
        gtk_widget_show_all(dlg->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_xconfig_save),
                                 TRUE);
        gtk_widget_set_size_request(dlg->txt_xconfig_save, 450, 350);
        gtk_button_set_label(GTK_BUTTON(dlg->btn_xconfig_preview),
                             _("Hide Preview..."));
    } else {
        gtk_widget_hide(dlg->box_xconfig_save);
        gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_xconfig_save), FALSE);
        gtk_button_set_label(GTK_BUTTON(dlg->btn_xconfig_preview),
                             _("Show Preview..."));
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
    gchar *selected_filename;

    /* Ask user for a filename */
    selected_filename =
        ctk_get_filename_from_dialog(_("Please select the X configuration file"),
             GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))),
             filename);

    if (selected_filename) {
        gtk_entry_set_text(GTK_ENTRY(dlg->txt_xconfig_file), selected_filename);

        update_xconfig_save_buffer(dlg);

        g_free(selected_filename);
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
    gchar *filename = NULL;
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
                         _("Show preview..."));
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
        break;

    case GTK_RESPONSE_REJECT:
    default:
        /* do nothing. */
        break;
    }

    g_free(filename);

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
        (_("Save X Configuration"),
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(parent))),
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_SAVE,
         GTK_RESPONSE_ACCEPT,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_xconfig_save),
                                    GTK_RESPONSE_REJECT);

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
    dlg->btn_xconfig_file = gtk_button_new_with_label(_("Browse..."));
    g_signal_connect(G_OBJECT(dlg->btn_xconfig_file), "clicked",
                     G_CALLBACK(xconfig_file_clicked),
                     (gpointer) dlg);

    /* Create the merge checkbox */
    dlg->btn_xconfig_merge =
        gtk_check_button_new_with_label(_("Merge with existing file."));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dlg->btn_xconfig_merge),
                                 TRUE);
    gtk_widget_set_sensitive(dlg->btn_xconfig_merge, merge_toggleable);
    g_signal_connect(G_OBJECT(dlg->btn_xconfig_merge), "toggled",
                     G_CALLBACK(xconfig_update_buffer),
                     (gpointer) dlg);


    /* Pack the preview button */
    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), dlg->btn_xconfig_preview,
                       FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_xconfig_save))),
                       hbox, FALSE, FALSE, 5);

    /* Pack the save window */
    hbox = gtk_hbox_new(TRUE, 0);

    gtk_container_add(GTK_CONTAINER(dlg->scr_xconfig_save),
                      dlg->txt_xconfig_save);
    gtk_box_pack_start(GTK_BOX(hbox), dlg->scr_xconfig_save, TRUE, TRUE, 5);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_xconfig_save))),
         hbox,
         TRUE, TRUE, 0);
    dlg->box_xconfig_save = hbox;

    /* Pack the filename text entry and browse button */
    hbox = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 5);

    gtk_box_pack_end(GTK_BOX(hbox2), dlg->btn_xconfig_file, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox2), dlg->txt_xconfig_file, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), hbox2, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_xconfig_save))),
                       hbox,
                       FALSE, FALSE, 5);

    /* Pack the merge checkbox */
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_xconfig_save))),
                       dlg->btn_xconfig_merge,
                       FALSE, FALSE, 5);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_xconfig_save)));

    return dlg;

} /* create_save_xconfig_button() */
