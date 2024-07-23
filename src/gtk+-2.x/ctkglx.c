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
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "glxinfo.h" /* xxx_abbrev functions */

#include "ctkbanner.h"
#include "ctkglx.h"
#include "ctkutils.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkconstants.h"

#include <GL/glx.h> /* GLX #defines */


/* Number of FBConfigs attributes reported in gui */
#define NUM_FBCONFIG_ATTRIBS  32
#define NUM_EGL_FBCONFIG_ATTRIBS  32

/* Indent size of Vulkan Info */
#define INDENT_SIZE 28

/* FBConfig tooltips */
static const char * __show_fbc_help =
  "Show the GLX Frame Buffer Configurations table in a new window.";
static const char * __show_egl_fbc_help =
  "Show the EGL Frame Buffer Configurations table in a new window.";
static const char * __fid_help  =
  "fid (Frame buffer ID) - Frame Buffer Configuration ID.";
static const char * __vid_help  =
  "vid (XVisual ID) -  ID of the associated X Visual.";
static const char * __vt_help  =
  "vt (XVisual Type) -  Type of the associated X Visual.  "
  "Possible X visual types are 'tc', 'dc', 'pc', 'sc', 'gs', 'sg' and '.' "
  "which mean TrueColor, DirectColor, PseudoColor, StaticColor, GrayScale, "
  "StaticGray and None, respectively.";
static const char * __bfs_help =
  "bfs (buffer size) - Number of bits per color in the color buffer.";
static const char * __lvl_help =
  "lvl (level) - Frame buffer level.  Level zero is the default frame "
  "buffer.  Positive levels are the overlay frame buffers (on top of the "
  "default frame buffer).  Negative levels are the underlay frame buffers "
  "(under the default frame buffer).";
static const char * __bf_help =
  "bf (Buffer format) - Color buffer format.  'rgb' means each element of the "
  "pixel buffer holds red, green, blue, and alpha values.  'ci' means each "
  "element of the pixel buffer holds a color index value, where the actual "
  "color is defined by a color map.";
static const char * __db_help =
  "db (Double buffer) - 'y' if the configuration has front and back color "
  "buffers that are swappable.  '-' if this is not supported.";
static const char * __st_help =
  "st (Stereo buffer) - 'y' if the configuration has left and right color "
  "buffers that are rendered to in stereo.  '-' if this is not supported.";
static const char * __rs_help =
  "rs (Red size) - Number of bits per color used for red.  "
  "Undefined for configurations that use color indexing.";
static const char * __gs_help =
  "gs (Green size) - Number of bits per color used for green.  "
  "Undefined for configurations that use color indexing.";
static const char * __bs_help =
  "bs (Blue size) - Number of bits per color used for blue.  "
  "Undefined for configurations that use color indexing.";
static const char * __as_help =
  "as (Alpha size) - Number of bits per color used for alpha.  "
  "Undefined for configurations that use color indexing.";
static const char * __aux_help =
  "aux (Auxiliary buffers) - Number of available auxiliary color buffers.";
static const char * __dpt_help =
  "dpt (Depth buffer size) - Number of bits per color in the depth buffer.";
static const char * __stn_help =
  "stn (Stencil size) - Number of bits per element in the stencil buffer.";
static const char * __acr_help =
  "acr (Accumulator red size) - Number of bits per color used for red "
  "in the accumulator buffer.";
static const char * __acg_help =
  "acg (Accumulator green size) - Number of bits per color used for green "
  "in the accumulator buffer.";
static const char * __acb_help =
  "acb (Accumulator blue size) - Number of bits per color used for blue "
  "in the accumulator buffer.";
static const char * __aca_help =
  "aca (Accumulator alpha size) - Number of bits per color used for alpha "
  "in the accumulator buffer.";
static const char * __mvs_help =
  "mvs (Multisample coverage samples) - Number of coverage samples per multisample.";
static const char * __mcs_help =
  "mcs (Multisample color samples) - Number of color samples per multisample.";
static const char * __mb_help =
  "mb (Multisample buffer count) - Number of multisample buffers.";
static const char * __cav_help =
  "cav (Caveats) - Caveats for this configuration.  A frame buffer "
  "configuration may have the following caveats: 'NonC' if it supports "
  "any non-conformant visual extension.  'Slow' if it has reduced "
  "performance.  '-' if it has no caveats.";
static const char * __pbw_help =
  "pbw (Pbuffer width) - Width of pbuffer (in hexadecimal).";
static const char * __pbh_help =
  "pbh (Pbuffer height) - Height of pbuffer (in hexadecimal).";
static const char * __pbp_help =
  "pbp (Pbuffer max pixels) - Max number of pixels in pbuffer (in "
  "hexadecimal).";
static const char * __trt_help =
  "trt (Transparency type) - Type of transparency (RGBA or Index).";
static const char * __trr_help =
  "trr (Transparency red value) - Red value considered transparent.";
static const char * __trg_help =
  "trg (Transparency green value) - Green value considered transparent.";
static const char * __trb_help =
  "trb (Transparency blue value) - Blue value considered transparent.";
static const char * __tra_help =
  "tra (Transparency alpha value) - Alpha value considered transparent.";
static const char * __tri_help =
  "tri (Transparency index value) - Color index value considered transparent.";


static const char * __egl_as_help =
  "as (Alpha size) - Number of bits of alpha stored in the color buffer.";
static const char * __egl_ams_help =
  "ams (Alpha mask size) - Number of bits in the alpha mask buffer.";
static const char * __egl_bt_help =
  "bt (Bind to Texture RGB) - 'y' if color buffers can be bound "
  "to an RGB texture, '.' otherwise.";
static const char * __egl_bta_help =
  "bta (Bind to Texture RGBA) - 'y' if color buffers can be bound "
  "to an RGBA texture, '.' otherwise.";
static const char * __egl_bs_help =
  "bs (Blue size) - Number of bits of blue stored in the color buffer.";
static const char * __egl_bfs_help =
  "bfs (Buffer size) - Depth of the color buffer. It is the sum of 'rs', 'gs', "
  "'bs', and 'as'.";
static const char * __egl_cbt_help =
  "cbt (Color buffer type) - Type of the color buffer. Possible types are "
  "'rgb' for RGB color buffer and 'lum' for Luminance.";
static const char * __egl_cav_help =
  "cav (Config caveat) - Caveats for the frame buffer configuration. Possible "
  "caveat values are 'slo' for Slow Config, 'NoC' for a non-conformant "
  "config, and '.' otherwise.";
static const char * __egl_id_help =
  "id (Config ID) - ID of the frame buffer configuration.";
static const char * __egl_cfm_help =
  "cfm (Conformant) - Bitmask indicating which client API contexts created "
  "with respect to this config are conformant.";
static const char * __egl_dpt_help =
  "dpt (Depth size) - Number of bits in the depth buffer.";
static const char * __egl_gs_help =
  "gs (Green size) - Number of bits of green stored in the color buffer.";
static const char * __egl_lvl_help =
  "lvl (Frame buffer level) - Level zero is the default frame buffer. Positive "
  "levels correspond to frame buffers that overlay the default buffer and "
  "negative levels correspond to frame buffers that underlay the default "
  "buffer.";
static const char * __egl_lum_help =
  "lum (Luminance size) - Number of bits of luminance stored in the luminance "
  "buffer.";
static const char * __egl_pbw_help =
  "pbw (Pbuffer max width) - Maximum width of a pixel buffer surface in "
  "pixels.";
static const char * __egl_pbh_help =
  "pdh (Pbuffer max height) - Maximum height of a pixel buffer surface in "
  "pixels.";
static const char * __egl_pbp_help =
  "pbp (Pbuffer max pixels) - Maximum size of a pixel buffer surface in "
  "pixels.";
static const char * __egl_six_help =
  "six (Swap interval max) - Maximum value that can be passed to "
  "eglSwapInterval.";
static const char * __egl_sin_help =
  "sin (Swap interval min) - Minimum value that can be passed to "
  "eglSwapInterval.";
static const char * __egl_nrd_help =
  "nrd (Native renderable) - 'y' if native rendering APIs can "
  "render into the surface, '.' otherwise.";
static const char * __egl_vid_help =
  "vid (Native visual ID) - ID of the associated native visual.";
static const char * __egl_nvt_help =
  "nvt (Native visual type) - Type of the associated native visual.";
static const char * __egl_rs_help =
  "rs (Red size) - Number of bits of red stored in the color buffer.";
static const char * __egl_rdt_help =
  "rdt (Renderable type) - Bitmask indicating the types of supported client "
  "API contexts.";
static const char * __egl_spb_help =
  "spb (Sample buffers) - Number of multisample buffers.";
static const char * __egl_smp_help =
  "smp (Samples) - Number of samples per pixel.";
static const char * __egl_stn_help =
  "stn (Stencil size) - Number of bits in the stencil buffer.";
static const char * __egl_sur_help =
  "sur (Surface type) - Bitmask indicating the types of supported EGL "
  "surfaces.";
static const char * __egl_tpt_help =
  "tpt (Transparent type) - Type of supported transparency. Possible "
  "transparency values are: 'rgb' for Transparent RGB and '.' otherwise.";
static const char * __egl_trv_help = "trv (Transparent red value)";
static const char * __egl_tgv_help = "tgv (Transparent green value)";
static const char * __egl_tbv_help = "tbv (Transparent blue value)";


GType ctk_glx_get_type(void)
{
    static GType ctk_glx_type = 0;

    if (!ctk_glx_type) {
        static const GTypeInfo ctk_glx_info = {
            sizeof (CtkGLXClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGLX),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_glx_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkGLX", &ctk_glx_info, 0);
    }

    return ctk_glx_type;
} /* ctk_glx_get_type() */



/*
 * show_fbc_toggled() - called when the show GLX Frame Buffer Configurations
 * button has been toggled.
 */

static void show_fbc_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkGLX *ctk_glx = user_data;
    gboolean enabled;

    /* get the enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {
        gtk_widget_show_all(ctk_glx->fbc_window);
    } else {
        gtk_widget_hide(ctk_glx->fbc_window);
    }

    ctk_config_statusbar_message(ctk_glx->ctk_config,
                                 "Show GLX Frame Buffer Configurations "
                                 "button %s.",
                                 enabled ? "enabled" : "disabled");

} /* show_fbc_toggled() */


/*
 * show_egl_fbc_toggled() - called when the show EGL Frame Buffer Configurations
 * button has been toggled.
 */

static void show_egl_fbc_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkGLX *ctk_glx = user_data;
    gboolean enabled;

    /* get the enabled state */

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {
        gtk_widget_show_all(ctk_glx->egl_fbc_window);
    } else {
        gtk_widget_hide(ctk_glx->egl_fbc_window);
    }

    ctk_config_statusbar_message(ctk_glx->ctk_config,
                                 "Show EGL Frame Buffer Configurations "
                                 "button %s.",
                                 enabled ? "enabled" : "disabled");

} /* show_egl_fbc_toggled() */


/*
 * fbc_window_destroy() - called when the window displaying the
 * GLX Frame Buffer Configurations table is closed.
 */
static gboolean
fbc_window_destroy(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    CtkGLX *ctk_glx = user_data;

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_glx->show_fbc_button),
         FALSE);

    return TRUE;

} /* fbc_window_destroy() */


/*
 * egl_fbc_window_destroy() - called when the window displaying the
 * EGL Frame Buffer Configurations table is closed.
 */
static gboolean
egl_fbc_window_destroy(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    CtkGLX *ctk_glx = user_data;

    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_glx->show_egl_fbc_button),
         FALSE);

    return TRUE;

} /* egl_fbc_window_destroy() */


/*
 * create_fbconfig_model() - called to create and populate the model for
 * the GLX Frame Buffer Configurations table.
 */
static GtkTreeModel *create_fbconfig_model(GLXFBConfigAttr *fbconfig_attribs)
{
    GtkListStore *model;
    GtkTreeIter iter;
    int i;
    GValue v = G_VALUE_INIT;

    if (!fbconfig_attribs) {
        return NULL;
    }

    g_value_init(&v, G_TYPE_STRING);

    model = gtk_list_store_new(NUM_FBCONFIG_ATTRIBS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /* Populate FBConfig table */
    i = 0;
    while ( fbconfig_attribs[i].fbconfig_id != 0 ) {
        char str[NUM_FBCONFIG_ATTRIBS + 1][16];
        int  cell = 0; /* Used for putting information into cells */

        if ( fbconfig_attribs[i].fbconfig_id )  {
            snprintf((char *) (&(str[cell++])), 16, "0x%02X",
                     fbconfig_attribs[i].fbconfig_id);
        } else {
            sprintf((char *) (&(str[cell++])),".");
        }

        if ( fbconfig_attribs[i].visual_id )  {
            snprintf((char *) (&(str[cell++])), 16, "0x%02X",
                     fbconfig_attribs[i].visual_id);
        } else {
            sprintf((char *) (&(str[cell++])),".");
        }
        snprintf((char *) (&(str[cell++])), 16, "%s",
                 x_visual_type_abbrev(fbconfig_attribs[i].x_visual_type));
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].buffer_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].level);
        snprintf((char *) (&(str[cell++])), 16, "%s",
                 render_type_abbrev(fbconfig_attribs[i].render_type) );
        snprintf((char *) (&(str[cell++])), 16, "%c",
                 fbconfig_attribs[i].doublebuffer ? 'y' : '.');
        snprintf((char *) (&(str[cell++])), 16, "%c",
                 fbconfig_attribs[i].stereo ? 'y' : '.');
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].red_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].green_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].blue_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].alpha_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].aux_buffers);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].depth_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].stencil_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].accum_red_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].accum_green_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].accum_blue_size);
        snprintf((char *) (&(str[cell++])), 16, "%2d",
                 fbconfig_attribs[i].accum_alpha_size);
        if (fbconfig_attribs[i].multi_sample_valid) {
            snprintf((char *) (&(str[cell++])), 16, "%2d",
                     fbconfig_attribs[i].multi_samples);
            if (fbconfig_attribs[i].multi_sample_coverage_valid) {
                snprintf((char *) (&(str[cell++])), 16, "%2d",
                         fbconfig_attribs[i].multi_samples_color);
            } else {
                snprintf((char *) (&(str[cell++])), 16, "%2d",
                         fbconfig_attribs[i].multi_samples);
            }
        } else {
            snprintf((char *) (&(str[cell++])), 16, " 0");
            snprintf((char *) (&(str[cell++])), 16, " 0");
        }
        snprintf((char *) (&(str[cell++])), 16, "%1d",
                 fbconfig_attribs[i].multi_sample_buffers);
        snprintf((char *) (&(str[cell++])), 16, "%s",
                 caveat_abbrev( fbconfig_attribs[i].config_caveat) );
        snprintf((char *) (&(str[cell++])), 16, "0x%04X",
                 fbconfig_attribs[i].pbuffer_width);
        snprintf((char *) (&(str[cell++])), 16, "0x%04X",
                 fbconfig_attribs[i].pbuffer_height);
        snprintf((char *) (&(str[cell++])), 16, "0x%07X",
                 fbconfig_attribs[i].pbuffer_max);
        snprintf((char *) (&(str[cell++])), 16, "%s",
                 transparent_type_abbrev(fbconfig_attribs[i].transparent_type));
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].transparent_red_value);
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].transparent_green_value);
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].transparent_blue_value);
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].transparent_alpha_value);
        snprintf((char *) (&(str[cell++])), 16, "%3d",
                 fbconfig_attribs[i].transparent_index_value);
        str[NUM_FBCONFIG_ATTRIBS][0] = '\0';

        /* Populate cells for this row */
        gtk_list_store_append (model, &iter);
        for (cell=0; cell<NUM_FBCONFIG_ATTRIBS; cell++) {
            g_value_set_string(&v, str[cell]);
            gtk_list_store_set_value(model, &iter, cell, &v);
        }

        i++;

    } /* Done - Populating FBconfig table */

    return GTK_TREE_MODEL(model);
}


/*
 * create_egl_fbconfig_model() - called to create and populate the model for
 * the EGL Frame Buffer Configurations table.
 */
static GtkTreeModel*
create_egl_fbconfig_model(EGLConfigAttr *egl_fbconfig_attribs)
{
    GtkListStore *model;
    GtkTreeIter iter;
    int i;
    GValue v = G_VALUE_INIT;

    if (!egl_fbconfig_attribs) {
        return NULL;
    }

    g_value_init(&v, G_TYPE_STRING);

    model = gtk_list_store_new(NUM_EGL_FBCONFIG_ATTRIBS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /* Populate FBConfig table */
    i = 0;
    while ( egl_fbconfig_attribs[i].config_id != 0 ) {
        char str[NUM_EGL_FBCONFIG_ATTRIBS + 1][16];
        int  cell = 0;

        snprintf((char*) (&(str[cell++])), 16, "0x%02X",
            egl_fbconfig_attribs[i].config_id);
        snprintf((char*) (&(str[cell++])), 16, "0x%02X",
            egl_fbconfig_attribs[i].native_visual_id);

        snprintf((char*) (&(str[cell++])), 16, "0x%X",
            egl_fbconfig_attribs[i].native_visual_type);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].buffer_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].level);
        snprintf((char*) (&(str[cell++])), 16, "%s",
            egl_color_buffer_type_abbrev(
                egl_fbconfig_attribs[i].color_buffer_type));
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].red_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].green_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].blue_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].alpha_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].alpha_mask_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].luminance_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].depth_size);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].stencil_size);
        snprintf((char*) (&(str[cell++])), 16, "%c",
            egl_fbconfig_attribs[i].bind_to_texture_rgb ? 'y' : '.');
        snprintf((char*) (&(str[cell++])), 16, "%c",
            egl_fbconfig_attribs[i].bind_to_texture_rgba ? 'y' : '.');
        snprintf((char*) (&(str[cell++])), 16, "0x%X",
            egl_fbconfig_attribs[i].conformant);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].sample_buffers);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].samples);
        snprintf((char*) (&(str[cell++])), 16, "%s",
            egl_config_caveat_abbrev(egl_fbconfig_attribs[i].config_caveat));
        snprintf((char*) (&(str[cell++])), 16, "0x%04X",
            egl_fbconfig_attribs[i].max_pbuffer_width);
        snprintf((char*) (&(str[cell++])), 16, "0x%04X",
            egl_fbconfig_attribs[i].max_pbuffer_height);
        snprintf((char*) (&(str[cell++])), 16, "0x%07X",
            egl_fbconfig_attribs[i].max_pbuffer_pixels);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].max_swap_interval);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].min_swap_interval);
        snprintf((char*) (&(str[cell++])), 16, "%c",
            egl_fbconfig_attribs[i].native_renderable ? 'y' : '.');
        snprintf((char*) (&(str[cell++])), 16, "0x%X",
            egl_fbconfig_attribs[i].renderable_type);
        snprintf((char*) (&(str[cell++])), 16, "0x%X",
            egl_fbconfig_attribs[i].surface_type);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].transparent_type);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].transparent_red_value);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].transparent_green_value);
        snprintf((char*) (&(str[cell++])), 16, "%d",
            egl_fbconfig_attribs[i].transparent_blue_value);


        str[NUM_EGL_FBCONFIG_ATTRIBS][0] = '\0';

        /* Populate cells for this row */
        gtk_list_store_append (model, &iter);
        for (cell=0; cell<NUM_EGL_FBCONFIG_ATTRIBS; cell++) {
            g_value_set_string(&v, str[cell]);
            gtk_list_store_set_value(model, &iter, cell, &v);
        }

        i++;

    } /* Done - Populating FBconfig table */

    return GTK_TREE_MODEL(model);
}

/* Creates the GLX information widget
 * 
 * NOTE: The Graphics information other than the FBConfigs will
 *       be setup when this page is hooked up and the "parent-set"
 *       signal is thrown.  This will result in calling the
 *       ctk_glx_probe_info() function.
 */

typedef struct WidgetSizeRec {
    GtkWidget *widget;
    int width;
} WidgetSize;

GtkWidget* ctk_glx_new(CtrlTarget *ctrl_target,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkGLX *ctk_glx;
    GtkWidget *banner;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *event;    /* For setting the background color to white */
    GtkWidget *window;

    GtkWidget *fbc_scroll_win;
    GtkWidget *fbc_view;
    GtkTreeModel *fbc_model;
    GtkWidget *show_fbc_button;

    GtkWidget *egl_fbc_scroll_win;
    GtkWidget *egl_fbc_view;
    GtkTreeModel *egl_fbc_model;
    GtkWidget *show_egl_fbc_button;

    ReturnStatus ret;

    GLXFBConfigAttr *fbconfig_attribs = NULL;   /* FBConfig data */
    EGLConfigAttr *egl_fbconfig_attribs = NULL; /* EGL Configs data */
    int i;                                      /* Iterator */
    int num_fbconfigs = 0;

    gchar *fbconfig_titles[NUM_FBCONFIG_ATTRIBS] = {
        "fid",  "vid",  "vt", "bfs",  "lvl",
        "bf",   "db",   "st",
        "rs",   "gs",   "bs",   "as",
        "aux",  "dpt",  "stn",
        "acr",  "acg",  "acb",  "aca",
        "mvs",  "mcs",  "mb",
        "cav",
        "pbw",  "pbh",  "pbp",
        "trt",  "trr",  "trg",  "trb",  "tra",  "tri"
    };

    const char *fbconfig_tooltips[NUM_FBCONFIG_ATTRIBS] = {
        __fid_help, __vid_help, __vt_help, __bfs_help, __lvl_help,
        __bf_help,  __db_help,  __st_help,
        __rs_help,  __gs_help,  __bs_help,  __as_help,
        __aux_help, __dpt_help, __stn_help,
        __acr_help, __acg_help, __acb_help, __aca_help,
        __mvs_help, __mcs_help, __mb_help,
        __cav_help,
        __pbw_help, __pbh_help, __pbp_help,
        __trt_help, __trr_help, __trg_help,
        __trb_help, __tra_help, __tri_help
    };

    gchar *egl_fbconfig_titles[NUM_EGL_FBCONFIG_ATTRIBS] = {
        "id",  "vid",
        "nvt", "bfs", "lvl", "cbt",
        "rs",  "gs",  "bs",  "as",
        "ams", "lum", "dpt", "stn",
        "bt",  "bta", "cfm",
        "spb", "smp", "cav",
        "pbw", "pbh", "pbp",
        "six", "sin",
        "nrd", "rdt", "sur",
        "tpt", "trv", "tgv", "tbv"
    };

    const char *egl_fbconfig_tooltips[NUM_EGL_FBCONFIG_ATTRIBS] = {
        __egl_id_help, __egl_vid_help,
        __egl_nvt_help, __egl_bfs_help, __egl_lvl_help, __egl_cbt_help,
        __egl_rs_help, __egl_gs_help, __egl_bs_help, __egl_as_help,
        __egl_ams_help, __egl_lum_help, __egl_dpt_help, __egl_stn_help,
        __egl_bt_help, __egl_bta_help, __egl_cfm_help,
        __egl_spb_help, __egl_smp_help, __egl_cav_help,
        __egl_pbw_help, __egl_pbh_help, __egl_pbp_help,
        __egl_six_help, __egl_sin_help,
        __egl_nrd_help, __egl_rdt_help, __egl_sur_help,
        __egl_tpt_help, __egl_trv_help, __egl_tgv_help, __egl_tbv_help
    };


    /* Create the ctk glx object */
    object = g_object_new(CTK_TYPE_GLX, NULL);
    ctk_glx = CTK_GLX(object);


    /* Cache the target */
    ctk_glx->ctrl_target = ctrl_target;

    /* Set container properties of the object */
    ctk_glx->ctk_config = ctk_config;
    gtk_box_set_spacing(GTK_BOX(ctk_glx), 10);

    /* Image banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GRAPHICS);
    gtk_box_pack_start(GTK_BOX(ctk_glx), banner, FALSE, FALSE, 0);


    /* Information Scroll Box */
    hbox = gtk_hbox_new(FALSE, 0);
    vbox = gtk_vbox_new(FALSE, 5);
    event = gtk_event_box_new();
    ctk_force_text_colors_on_widget(event);
    gtk_container_add(GTK_CONTAINER(event), hbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    ctk_glx->glxinfo_vpane = vbox;


    /* GLX 1.3 supports frame buffer configurations */
#ifdef GLX_VERSION_1_3

    ctk_glx->glx_fbconfigs_available = TRUE;

    /* Grab the FBConfigs */
    ret = NvCtrlGetVoidAttribute(ctrl_target,
                                 NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS,
                                 (void *)(&fbconfig_attribs));
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query list of GLX frame buffer "
                       "configurations.");
        ctk_glx->glx_fbconfigs_available = FALSE;
    } else {
        /* Count the number of fbconfigs */
        if (fbconfig_attribs) {
            for (num_fbconfigs = 0;
                 fbconfig_attribs[num_fbconfigs].fbconfig_id != 0;
                 num_fbconfigs++);
        }
        if (num_fbconfigs == 0) {
            nv_warning_msg("No frame buffer configurations found.");
            ctk_glx->glx_fbconfigs_available = FALSE;
        }
    }

    if (ctk_glx->glx_fbconfigs_available) {
        show_fbc_button = gtk_toggle_button_new_with_label(
                              "Show GLX Frame Buffer Configurations");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_fbc_button), FALSE);
        ctk_config_set_tooltip(ctk_config, show_fbc_button, __show_fbc_help);
        g_signal_connect(G_OBJECT(show_fbc_button),
                         "clicked", G_CALLBACK(show_fbc_toggled),
                         (gpointer) ctk_glx);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "GLX Frame Buffer "
                                                 "Configurations");
        gtk_container_set_border_width(GTK_CONTAINER(window), CTK_WINDOW_PAD);
        gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
        g_signal_connect(G_OBJECT(window), "destroy-event",
                         G_CALLBACK(fbc_window_destroy),
                         (gpointer) ctk_glx);
        g_signal_connect(G_OBJECT(window), "delete-event",
                         G_CALLBACK(fbc_window_destroy),
                         (gpointer) ctk_glx);

        ctk_glx->fbc_window = window;
        ctk_glx->show_fbc_button = show_fbc_button;

        hbox      = gtk_hbox_new(FALSE, 0);
        vbox      = gtk_vbox_new(FALSE, 10);


        /* Create fbconfig window */
        fbc_view = gtk_tree_view_new();

        /* Create columns and column headers with tooltips */
        for ( i = 0; i < NUM_FBCONFIG_ATTRIBS; i++ ) {
            GtkWidget *label;
            GtkCellRenderer *renderer;
            GtkTreeViewColumn *col;

            renderer = gtk_cell_renderer_text_new();
            ctk_cell_renderer_set_alignment(renderer, 0.5, 0.5);

            col = gtk_tree_view_column_new_with_attributes(fbconfig_titles[i],
                                                           renderer,
                                                           "text", i,
                                                           NULL);

            label = gtk_label_new(fbconfig_titles[i]);
            ctk_config_set_tooltip(ctk_config, label, fbconfig_tooltips[i]);
            gtk_widget_show(label);

            gtk_tree_view_column_set_widget(col, label);
            gtk_tree_view_insert_column(GTK_TREE_VIEW(fbc_view), col, -1);
        }

        /* Create data model and add view to the window */
        fbc_model = create_fbconfig_model(fbconfig_attribs);
        free(fbconfig_attribs);

        gtk_tree_view_set_model(GTK_TREE_VIEW(fbc_view), fbc_model);
        g_object_unref(fbc_model);

        fbc_scroll_win = gtk_scrolled_window_new(NULL, NULL);

        gtk_container_add (GTK_CONTAINER (fbc_scroll_win), fbc_view);
        gtk_container_add (GTK_CONTAINER (window), fbc_scroll_win);

    } else {
        free(fbconfig_attribs);
    }

#endif /* GLX_VERSION_1_3 */



    /* EGL Configurations */

    ctk_glx->egl_fbconfigs_available = TRUE;
    ret = NvCtrlGetVoidAttribute(ctrl_target,
                                 NV_CTRL_ATTR_EGL_CONFIG_ATTRIBS,
                                 (void *)(&egl_fbconfig_attribs));
    if (ret != NvCtrlSuccess) {
        nv_warning_msg("Failed to query list of EGL configurations.");
        ctk_glx->egl_fbconfigs_available = FALSE;
    } else {
        num_fbconfigs = 0;
        if (egl_fbconfig_attribs) {
            while (egl_fbconfig_attribs[num_fbconfigs].config_id != 0) {
                 num_fbconfigs++;
            }
        }

        if (ctk_glx->egl_fbconfigs_available && num_fbconfigs == 0) {
            nv_warning_msg("No EGL frame buffer configurations found.");
            ctk_glx->egl_fbconfigs_available = FALSE;
        }
    }

    if (ctk_glx->egl_fbconfigs_available) {

        show_egl_fbc_button = gtk_toggle_button_new_with_label(
                                  "Show EGL Frame Buffer Configurations");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_egl_fbc_button),
                                     FALSE);
        ctk_config_set_tooltip(ctk_config, show_egl_fbc_button,
                               __show_egl_fbc_help);
        g_signal_connect(G_OBJECT(show_egl_fbc_button),
                         "clicked", G_CALLBACK(show_egl_fbc_toggled),
                         (gpointer) ctk_glx);

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window),
                             "EGL Frame Buffer Configurations");
        gtk_container_set_border_width(GTK_CONTAINER(window), CTK_WINDOW_PAD);
        gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
        g_signal_connect(G_OBJECT(window), "destroy-event",
                         G_CALLBACK(egl_fbc_window_destroy),
                         (gpointer) ctk_glx);
        g_signal_connect(G_OBJECT(window), "delete-event",
                         G_CALLBACK(egl_fbc_window_destroy),
                         (gpointer) ctk_glx);

        ctk_glx->egl_fbc_window = window;
        ctk_glx->show_egl_fbc_button = show_egl_fbc_button;

        hbox = gtk_hbox_new(FALSE, 0);
        vbox = gtk_vbox_new(FALSE, 10);


        /* Create fbconfig window */
        egl_fbc_view = gtk_tree_view_new();

        /* Create columns and column headers with tooltips */
        for ( i = 0; i < NUM_EGL_FBCONFIG_ATTRIBS; i++ ) {
            GtkWidget *label;
            GtkCellRenderer *renderer;
            GtkTreeViewColumn *col;

            renderer = gtk_cell_renderer_text_new();
            ctk_cell_renderer_set_alignment(renderer, 0.5, 0.5);

            col =
                gtk_tree_view_column_new_with_attributes(egl_fbconfig_titles[i],
                                                         renderer,
                                                         "text", i,
                                                         NULL);

            label = gtk_label_new(egl_fbconfig_titles[i]);
            ctk_config_set_tooltip(ctk_config, label, egl_fbconfig_tooltips[i]);
            gtk_widget_show(label);

            gtk_tree_view_column_set_widget(col, label);
            gtk_tree_view_insert_column(GTK_TREE_VIEW(egl_fbc_view), col, -1);
        }

        /* Create data model and add view to the window */
        egl_fbc_model = create_egl_fbconfig_model(egl_fbconfig_attribs);
        free(egl_fbconfig_attribs);

        gtk_tree_view_set_model(GTK_TREE_VIEW(egl_fbc_view), egl_fbc_model);
        g_object_unref(egl_fbc_model);

        egl_fbc_scroll_win = gtk_scrolled_window_new(NULL, NULL);

        gtk_container_add(GTK_CONTAINER(egl_fbc_scroll_win), egl_fbc_view);
        gtk_container_add(GTK_CONTAINER(window), egl_fbc_scroll_win);

    } else {
        free(egl_fbconfig_attribs);
    }

    /* Set main page layout */

    gtk_box_pack_start(GTK_BOX(ctk_glx), event, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(object));
    return GTK_WIDGET(object);

} /* ctk_glx_new */



/*
 * add_string_to_table()
 */
static void add_string_to_table(GtkWidget *table,
                                const gint row,
                                const gint col,
                                const gchar *str)
{
    GtkWidget *label = gtk_label_new(str);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
}



/*
 * add_table_row_3() - add items to a table row with 3 columns
 */
static void add_table_row_3(GtkWidget *table,
                            const gint row,
                            const gchar *value1,
                            const gchar *value2,
                            const gchar *value3)
{
    add_string_to_table(table, row, 0, value1);
    add_string_to_table(table, row, 1, value2);
    add_string_to_table(table, row, 2, value3);
}



/*
 * add_str_const() - Add label and value to a table.
 */
static void add_str_const(GtkWidget *table, int row,
                          const char *str, const char *value)
{
    int i = 0;
    while (isspace(value[i])) { i++; }
    add_table_row(table, row,
                  0, 0, str,
                  0, 0, value + i);
}



/*
 * add_str() - Add label and value to a table.
 *
 * NOTE - this function will free the string pointed to by value. Use
 * add_str_const() to retain the string.
 */
static void add_str(GtkWidget *table, int row, const char *str, char *value)
{
    add_str_const(table, row, str, value);
    nvfree(value);
}



/*
 * populate_vulkan_device_properties() - Process Vulkan Device Properties
 */
static GtkWidget *populate_vulkan_device_properties(VkDeviceAttr *vkdp, int i)
{
    int row = 3;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Device Properties");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);
    char *str = vulkan_get_version_string(
                    vkdp->phy_device_properties[i].apiVersion);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    add_str_const(table, row++, "Device Name",
                  vkdp->phy_device_properties[i].deviceName);
    add_str_const(table, row++, "Device Type",
                  vulkan_get_physical_device_type(
                      vkdp->phy_device_properties[i].deviceType));
    add_str(table, row++, "API Version", str);
    add_str(table, row++, "Driver Version",
            nvasprintf("%#x", vkdp->phy_device_properties[i].driverVersion));
    add_str(table, row++, "Vendor ID",
            nvasprintf("%#x", vkdp->phy_device_properties[i].vendorID));
    add_str(table, row++, "Device ID",
            nvasprintf("%#x", vkdp->phy_device_properties[i].deviceID));
    if (vkdp->phy_device_uuid && vkdp->phy_device_uuid[i]) {
        add_str_const(table, row++, "Device UUID", vkdp->phy_device_uuid[i]);
    }
    return expander;
}



/*
 * populate_vulkan_device_extensions()
 */
static GtkWidget *populate_vulkan_device_extensions(VkDeviceAttr *vkdp, int d)
{
    int row = 2, j;
    char *str;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Device Extensions");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    str = nvasprintf("%d", vkdp->device_extensions_count[d]);
    add_str_const(table, row++, "Count:", str);
    nvfree(str);

    for (j = 0; j < vkdp->device_extensions_count[d]; j++) {
        str = nvasprintf("Version: %d",
                         vkdp->device_extensions[d][j].specVersion);

        add_str_const(table, j+row,
                      vkdp->device_extensions[d][j].extensionName,
                      str);
        nvfree(str);
    }
    return expander;
}



/*
 * populate_vulkan_device_sparse_properties() - Process Vulkan Device Sparse
 * Properties.
 */
static GtkWidget *populate_vulkan_device_sparse_properties(VkDeviceAttr *vkdp,
                                                           int i)
{
    int row = 5;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Sparse Properties");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

#define PRINT_DEVICE_SPARSE_FEATURES(var)                                   \
    add_str(table, row++, #var, nvasprintf("%s",                               \
           vkdp->phy_device_properties[i].sparseProperties.var ? "yes" : "no"));
    PRINT_DEVICE_SPARSE_FEATURES(residencyStandard2DBlockShape)
    PRINT_DEVICE_SPARSE_FEATURES(residencyStandard2DMultisampleBlockShape)
    PRINT_DEVICE_SPARSE_FEATURES(residencyStandard3DBlockShape)
    PRINT_DEVICE_SPARSE_FEATURES(residencyAlignedMipSize)
    PRINT_DEVICE_SPARSE_FEATURES(residencyNonResidentStrict)
#undef PRINT_DEVICE_SPARSE_FEATURES

    return expander;
}



/*
 * populate_vulkan_device_limits() - Process Vulkan Device Limits.
 */
static GtkWidget *populate_vulkan_device_limits(VkDeviceAttr *vkdp, int i)
{
    int row = 5;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Limits");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

#define PRINT_DEVICE_LIMITS_U(var)   \
    add_str(table, row++, #var,      \
            nvasprintf("%u", vkdp->phy_device_properties[i].limits.var));
#define PRINT_DEVICE_LIMITS_S(var)   \
    add_str(table, row++, #var,      \
            nvasprintf("%lu",        \
                (unsigned long)vkdp->phy_device_properties[i].limits.var));
#define PRINT_DEVICE_LIMITS_F(var)   \
    add_str(table, row++, #var,      \
            nvasprintf("%f", vkdp->phy_device_properties[i].limits.var));
#define PRINT_DEVICE_LIMITS_Z(var)   \
    add_str(table, row++, #var,      \
            nvasprintf("%zu", vkdp->phy_device_properties[i].limits.var));

    PRINT_DEVICE_LIMITS_U(maxImageDimension1D)
    PRINT_DEVICE_LIMITS_U(maxImageDimension2D)
    PRINT_DEVICE_LIMITS_U(maxImageDimension3D)
    PRINT_DEVICE_LIMITS_U(maxImageDimensionCube)
    PRINT_DEVICE_LIMITS_U(maxImageArrayLayers)
    PRINT_DEVICE_LIMITS_U(maxTexelBufferElements)
    PRINT_DEVICE_LIMITS_U(maxUniformBufferRange)
    PRINT_DEVICE_LIMITS_U(maxStorageBufferRange)
    PRINT_DEVICE_LIMITS_U(maxPushConstantsSize)
    PRINT_DEVICE_LIMITS_U(maxMemoryAllocationCount)
    PRINT_DEVICE_LIMITS_U(maxSamplerAllocationCount)
    PRINT_DEVICE_LIMITS_S(bufferImageGranularity)
    PRINT_DEVICE_LIMITS_S(sparseAddressSpaceSize)
    PRINT_DEVICE_LIMITS_U(maxBoundDescriptorSets)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorSamplers)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorUniformBuffers)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorStorageBuffers)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorSampledImages)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorStorageImages)
    PRINT_DEVICE_LIMITS_U(maxPerStageDescriptorInputAttachments)
    PRINT_DEVICE_LIMITS_U(maxPerStageResources)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetSamplers)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetUniformBuffers)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetUniformBuffersDynamic)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetStorageBuffers)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetStorageBuffersDynamic)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetSampledImages)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetStorageImages)
    PRINT_DEVICE_LIMITS_U(maxDescriptorSetInputAttachments)
    PRINT_DEVICE_LIMITS_U(maxVertexInputAttributes)
    PRINT_DEVICE_LIMITS_U(maxVertexInputBindings)
    PRINT_DEVICE_LIMITS_U(maxVertexInputAttributeOffset)
    PRINT_DEVICE_LIMITS_U(maxVertexInputBindingStride)
    PRINT_DEVICE_LIMITS_U(maxVertexOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationGenerationLevel)
    PRINT_DEVICE_LIMITS_U(maxTessellationPatchSize)
    PRINT_DEVICE_LIMITS_U(maxTessellationControlPerVertexInputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationControlPerVertexOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationControlPerPatchOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationControlTotalOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationEvaluationInputComponents)
    PRINT_DEVICE_LIMITS_U(maxTessellationEvaluationOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxGeometryShaderInvocations)
    PRINT_DEVICE_LIMITS_U(maxGeometryInputComponents)
    PRINT_DEVICE_LIMITS_U(maxGeometryOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxGeometryOutputVertices)
    PRINT_DEVICE_LIMITS_U(maxGeometryTotalOutputComponents)
    PRINT_DEVICE_LIMITS_U(maxFragmentInputComponents)
    PRINT_DEVICE_LIMITS_U(maxFragmentOutputAttachments)
    PRINT_DEVICE_LIMITS_U(maxFragmentDualSrcAttachments)
    PRINT_DEVICE_LIMITS_U(maxFragmentCombinedOutputResources)
    PRINT_DEVICE_LIMITS_U(maxComputeSharedMemorySize)
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupCount[0])
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupCount[1])
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupCount[2])
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupInvocations)
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupSize[0])
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupSize[1])
    PRINT_DEVICE_LIMITS_U(maxComputeWorkGroupSize[2])
    PRINT_DEVICE_LIMITS_U(subPixelPrecisionBits)
    PRINT_DEVICE_LIMITS_U(subTexelPrecisionBits)
    PRINT_DEVICE_LIMITS_U(mipmapPrecisionBits)
    PRINT_DEVICE_LIMITS_U(maxDrawIndexedIndexValue)
    PRINT_DEVICE_LIMITS_U(maxDrawIndirectCount)
    PRINT_DEVICE_LIMITS_F(maxSamplerLodBias)
    PRINT_DEVICE_LIMITS_F(maxSamplerAnisotropy)
    PRINT_DEVICE_LIMITS_U(maxViewports)
    PRINT_DEVICE_LIMITS_U(maxViewportDimensions[0])
    PRINT_DEVICE_LIMITS_U(maxViewportDimensions[1])
    PRINT_DEVICE_LIMITS_F(viewportBoundsRange[0])
    PRINT_DEVICE_LIMITS_F(viewportBoundsRange[1])
    PRINT_DEVICE_LIMITS_U(viewportSubPixelBits)
    PRINT_DEVICE_LIMITS_Z(minMemoryMapAlignment)
    PRINT_DEVICE_LIMITS_S(minTexelBufferOffsetAlignment)
    PRINT_DEVICE_LIMITS_S(minUniformBufferOffsetAlignment)
    PRINT_DEVICE_LIMITS_S(minStorageBufferOffsetAlignment)
    PRINT_DEVICE_LIMITS_U(minTexelOffset)
    PRINT_DEVICE_LIMITS_U(maxTexelOffset)
    PRINT_DEVICE_LIMITS_U(minTexelGatherOffset)
    PRINT_DEVICE_LIMITS_U(maxTexelGatherOffset)
    PRINT_DEVICE_LIMITS_F(minInterpolationOffset)
    PRINT_DEVICE_LIMITS_F(maxInterpolationOffset)
    PRINT_DEVICE_LIMITS_U(subPixelInterpolationOffsetBits)
    PRINT_DEVICE_LIMITS_U(maxFramebufferWidth)
    PRINT_DEVICE_LIMITS_U(maxFramebufferHeight)
    PRINT_DEVICE_LIMITS_U(maxFramebufferLayers)
    PRINT_DEVICE_LIMITS_U(framebufferColorSampleCounts)
    PRINT_DEVICE_LIMITS_U(framebufferDepthSampleCounts)
    PRINT_DEVICE_LIMITS_U(framebufferStencilSampleCounts)
    PRINT_DEVICE_LIMITS_U(framebufferNoAttachmentsSampleCounts)
    PRINT_DEVICE_LIMITS_U(maxColorAttachments)
    PRINT_DEVICE_LIMITS_U(sampledImageColorSampleCounts)
    PRINT_DEVICE_LIMITS_U(sampledImageIntegerSampleCounts)
    PRINT_DEVICE_LIMITS_U(sampledImageDepthSampleCounts)
    PRINT_DEVICE_LIMITS_U(sampledImageStencilSampleCounts)
    PRINT_DEVICE_LIMITS_U(storageImageSampleCounts)
    PRINT_DEVICE_LIMITS_U(maxSampleMaskWords)
    PRINT_DEVICE_LIMITS_U(timestampComputeAndGraphics)
    PRINT_DEVICE_LIMITS_F(timestampPeriod)
    PRINT_DEVICE_LIMITS_U(maxClipDistances)
    PRINT_DEVICE_LIMITS_U(maxCullDistances)
    PRINT_DEVICE_LIMITS_U(maxCombinedClipAndCullDistances)
    PRINT_DEVICE_LIMITS_U(discreteQueuePriorities)
    PRINT_DEVICE_LIMITS_F(pointSizeRange[0])
    PRINT_DEVICE_LIMITS_F(pointSizeRange[1])
    PRINT_DEVICE_LIMITS_F(lineWidthRange[0])
    PRINT_DEVICE_LIMITS_F(lineWidthRange[1])
    PRINT_DEVICE_LIMITS_F(pointSizeGranularity)
    PRINT_DEVICE_LIMITS_F(lineWidthGranularity)
    PRINT_DEVICE_LIMITS_U(strictLines)
    PRINT_DEVICE_LIMITS_U(standardSampleLocations)
    PRINT_DEVICE_LIMITS_S(optimalBufferCopyOffsetAlignment)
    PRINT_DEVICE_LIMITS_S(optimalBufferCopyRowPitchAlignment)
    PRINT_DEVICE_LIMITS_S(nonCoherentAtomSize)
#undef PRINT_DEVICE_LIMITS_U
#undef PRINT_DEVICE_LIMITS_S
#undef PRINT_DEVICE_LIMITS_F
#undef PRINT_DEVICE_LIMITS_Z

    return expander;
}



/*
 * populate_vulkan_device_features() - Process Vulkan Device Features.
 */
static GtkWidget *populate_vulkan_device_features(VkDeviceAttr *vkdp, int i)
{
    int row = 5;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Features");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

#define PRINT_DEVICE_FEATURE(var)  \
    add_str_const(table, row++, #var, \
                  (vkdp->features[i].var? "yes" : "no"));
    PRINT_DEVICE_FEATURE(robustBufferAccess)
    PRINT_DEVICE_FEATURE(fullDrawIndexUint32)
    PRINT_DEVICE_FEATURE(imageCubeArray)
    PRINT_DEVICE_FEATURE(independentBlend)
    PRINT_DEVICE_FEATURE(geometryShader)
    PRINT_DEVICE_FEATURE(tessellationShader)
    PRINT_DEVICE_FEATURE(sampleRateShading)
    PRINT_DEVICE_FEATURE(dualSrcBlend)
    PRINT_DEVICE_FEATURE(logicOp)
    PRINT_DEVICE_FEATURE(multiDrawIndirect)
    PRINT_DEVICE_FEATURE(drawIndirectFirstInstance)
    PRINT_DEVICE_FEATURE(depthClamp)
    PRINT_DEVICE_FEATURE(depthBiasClamp)
    PRINT_DEVICE_FEATURE(fillModeNonSolid)
    PRINT_DEVICE_FEATURE(depthBounds)
    PRINT_DEVICE_FEATURE(wideLines)
    PRINT_DEVICE_FEATURE(largePoints)
    PRINT_DEVICE_FEATURE(alphaToOne)
    PRINT_DEVICE_FEATURE(multiViewport)
    PRINT_DEVICE_FEATURE(samplerAnisotropy)
    PRINT_DEVICE_FEATURE(textureCompressionETC2)
    PRINT_DEVICE_FEATURE(textureCompressionASTC_LDR)
    PRINT_DEVICE_FEATURE(textureCompressionBC)
    PRINT_DEVICE_FEATURE(occlusionQueryPrecise)
    PRINT_DEVICE_FEATURE(pipelineStatisticsQuery)
    PRINT_DEVICE_FEATURE(vertexPipelineStoresAndAtomics)
    PRINT_DEVICE_FEATURE(fragmentStoresAndAtomics)
    PRINT_DEVICE_FEATURE(shaderTessellationAndGeometryPointSize)
    PRINT_DEVICE_FEATURE(shaderImageGatherExtended)
    PRINT_DEVICE_FEATURE(shaderStorageImageExtendedFormats)
    PRINT_DEVICE_FEATURE(shaderStorageImageMultisample)
    PRINT_DEVICE_FEATURE(shaderStorageImageReadWithoutFormat)
    PRINT_DEVICE_FEATURE(shaderStorageImageWriteWithoutFormat)
    PRINT_DEVICE_FEATURE(shaderUniformBufferArrayDynamicIndexing)
    PRINT_DEVICE_FEATURE(shaderSampledImageArrayDynamicIndexing)
    PRINT_DEVICE_FEATURE(shaderStorageBufferArrayDynamicIndexing)
    PRINT_DEVICE_FEATURE(shaderStorageImageArrayDynamicIndexing)
    PRINT_DEVICE_FEATURE(shaderClipDistance)
    PRINT_DEVICE_FEATURE(shaderCullDistance)
    PRINT_DEVICE_FEATURE(shaderFloat64)
    PRINT_DEVICE_FEATURE(shaderInt64)
    PRINT_DEVICE_FEATURE(shaderInt16)
    PRINT_DEVICE_FEATURE(shaderResourceResidency)
    PRINT_DEVICE_FEATURE(shaderResourceMinLod)
    PRINT_DEVICE_FEATURE(sparseBinding)
    PRINT_DEVICE_FEATURE(sparseResidencyBuffer)
    PRINT_DEVICE_FEATURE(sparseResidencyImage2D)
    PRINT_DEVICE_FEATURE(sparseResidencyImage3D)
    PRINT_DEVICE_FEATURE(sparseResidency2Samples)
    PRINT_DEVICE_FEATURE(sparseResidency4Samples)
    PRINT_DEVICE_FEATURE(sparseResidency8Samples)
    PRINT_DEVICE_FEATURE(sparseResidency16Samples)
    PRINT_DEVICE_FEATURE(sparseResidencyAliased)
    PRINT_DEVICE_FEATURE(variableMultisampleRate)
    PRINT_DEVICE_FEATURE(inheritedQueries)
#undef PRINT_DEVICE_FEATURE

    return expander;
}



/*
 * populate_vulkan_device_queue_properties() - Process Vulkan Device Queue
 * Properties.
 */
static GtkWidget *populate_vulkan_device_queue_properties(VkDeviceAttr *vkdp,
                                                          int i)
{
    int j;
    int row = 5;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Queue Properties");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    for (j = 0; j < vkdp->queue_properties_count[i]; j++) {
        VkExtent3D e = vkdp->queue_properties[i][j].minImageTransferGranularity;
        char *qstr = vulkan_get_queue_family_flags(
                         vkdp->queue_properties[i][j].queueFlags);
        add_str(table, row++, "Queue Number", nvasprintf("%d", j));

        add_str(table, row++, "Flags", qstr);
        add_str(table, row++, "Count",
                nvasprintf("%d", vkdp->queue_properties[i][j].queueCount));
        add_str(table, row++, "Min Image Transfer Granularity",
                nvasprintf("%dx%dx%d (WxHxD)", e.width, e.height, e.depth));
        add_str_const(table, row++, "", "");
    }
    return expander;
}


/*
 * populate_vulkan_device_memory_type_properties() - Process Vulkan Device
 * Memory Type Properties.
 */
static GtkWidget *populate_vulkan_device_memory_type_properties(
                     VkDeviceAttr *vkdp, int i)
{
    int j;
    int row = 2;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Memory Type Properties");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    for (j = 0; j < vkdp->memory_properties[i].memoryTypeCount; j++) {
        add_str(table, row++, "Index of Memory Type", nvasprintf("%d", j));
        add_str(table, row++, "Heap Index", nvasprintf("%d",
                vkdp->memory_properties[i].memoryTypes[j].heapIndex));
        add_str(table, row++, "Flags",
                vulkan_get_memory_property_flags(
                    vkdp->memory_properties[i].memoryTypes[j].propertyFlags));
        add_str_const(table, row++, "", "");
    }
    return expander;
}


/*
 * populate_vulkan_device_memory_heap_properties() - Process Vulkan Device
 * Memory Heap Properties.
 */
static GtkWidget *populate_vulkan_device_memory_heap_properties(
                     VkDeviceAttr *vkdp, int i)
{
    int j;
    int row = 1;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Memory Heap Properties");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    for (j = 0; j < vkdp->memory_properties[i].memoryHeapCount; j++) {
        add_str(table, row++, "Index of Memory Heap", nvasprintf("%d", j));
        add_str(table, row++, "Size",
                nvasprintf("%" PRIu64,
                           vkdp->memory_properties[i].memoryHeaps[j].size));
        add_str(table, row++, "Flags",
                vulkan_get_memory_heap_flags(
                    vkdp->memory_properties[i].memoryHeaps[j].flags));
        add_str_const(table, row++, "", "");
    }
    return expander;
}



/*
 * setup_vulkan_format_feature_string()
 */
static char *setup_vulkan_format_feature_string(VkFormatFeatureFlags flags)
{
    char *str = vulkan_get_format_feature_flags(flags);
    int i;

    for (i = 0; i < strlen(str); i++)
    {
        if (str[i] == ' ') {
            str[i] = '\n';
        }
    }

    return str;
}



/*
 * populate_vulkan_format()
 */
static GtkWidget *populate_vulkan_formats(VkDeviceAttr *vkdp, int i)
{
    int j;
    int row = 5;
    char *str;
    GtkWidget *table = gtk_table_new(row, 2, FALSE);
    GtkWidget *expander = gtk_expander_new("Formats");
    GtkWidget *ibox = gtk_hbox_new(FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
    gtk_container_add(GTK_CONTAINER(expander), ibox);

    for (j = 0; j < vkdp->formats_count[i]; j++) {
        add_str(table, row++, "Index", nvasprintf("%d", j));
        str = setup_vulkan_format_feature_string(
                  vkdp->formats[i][j].linearTilingFeatures);
        add_str(table, row++, "Linear", str);
        str = setup_vulkan_format_feature_string(
                  vkdp->formats[i][j].bufferFeatures);
        add_str(table, row++, "Buffer", str);
        str = setup_vulkan_format_feature_string(
                  vkdp->formats[i][j].optimalTilingFeatures);
        add_str(table, row++, "Optimal", str);

        add_str_const(table, row++, "", "");
    }
    return expander;
}


/*
 * compare_gpu_uuids() - compare two uuid strings skipping the possible "GPU"
 * prefix and any '-' characters in the first string.
 */
static int compare_gpu_uuids(const char* gpu_uuid, const char* vk_gpu_uuid)
{
    int i = 3, j = 3;

    if (!gpu_uuid || !vk_gpu_uuid) {
        return 0;
    }

    while (gpu_uuid[i] && vk_gpu_uuid[j]) {
        if (gpu_uuid[i] == '-') {
            i++;
        }

        if (vk_gpu_uuid[j] == '-') {
            j++;
        }

        if (gpu_uuid[i] != vk_gpu_uuid[j])
        {
            return 0;
        }

        i++;
        j++;
    }

    if (gpu_uuid[i] == 0 && vk_gpu_uuid[j] == 0) {
        return 1;
    } else {
        return 0;
    }

}



/*
 * Check to see if device number matches a valid device UUID. If
 * any data is missing to make a comparison, assume a match.
 */
static gboolean check_associated_device(VkDeviceAttr *vkdp, int device_num,
                                        char **assoc_gpu_uuid)
{
    int i;
    if (assoc_gpu_uuid == NULL ||
        vkdp->phy_device_uuid == NULL ||
        vkdp->phy_device_uuid[device_num] == NULL) {
        return TRUE;
    }

    for (i = 0; assoc_gpu_uuid[i]; i++) {
        if (compare_gpu_uuids(assoc_gpu_uuid[i],
                              vkdp->phy_device_uuid[device_num]))
        {
            return TRUE;
        }
    }

    return FALSE;
}



/*
 * Probes for Graphics information and sets up the results
 * in the Graphics notebook widget.
 */
void ctk_glx_probe_info(GtkWidget *widget)
{
    CtkGLX *ctk_glx = CTK_GLX(widget);
    CtrlTarget *ctrl_target = ctk_glx->ctrl_target;

    ReturnStatus ret;

    char *direct_rendering  = NULL;
    char *glx_extensions    = NULL;
    char *server_vendor     = NULL;
    char *server_version    = NULL;
    char *server_extensions = NULL;
    char *client_vendor     = NULL;
    char *client_version    = NULL;
    char *client_extensions = NULL;
    char *opengl_vendor     = NULL;
    char *opengl_renderer   = NULL;
    char *opengl_version    = NULL;
    char *opengl_extensions = NULL;
    char *egl_vendor        = NULL;
    char *egl_version       = NULL;
    char *egl_extensions    = NULL;
    char *vk_api_version    = NULL;
    VkLayerAttr *vklp = nvalloc(sizeof(VkLayerAttr));
    VkDeviceAttr *vkdp = nvalloc(sizeof(VkDeviceAttr));
    int *pData;
    int len;
    int i;
    char *ptr;
    int device_num;
    int row;

    GtkWidget *vbox, *vbox2;
    GtkWidget *table;
    GtkWidget *expander;
    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *notebook_label;
    GtkWidget *scroll_win;
    int notebook_padding = 8;

    CtrlTarget *gpu_target = NULL;
    gchar **assoc_gpu_uuid = NULL;

    /* Make sure the widget was initialized and that glx information
     * has not yet been initialized.
     */
    if ( !ctk_glx || !ctk_glx->glxinfo_vpane ||
         ctk_glx->glxinfo_initialized ) {
        return;
    }

    ctk_glx->glx_available = TRUE;
    ctk_glx->egl_available = TRUE;
    ctk_glx->vulkan_available = TRUE;


    if (ctrl_target->targetTypeInfo->nvctrl == X_SCREEN_TARGET) {
        /* Get GLX information */
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_DIRECT_RENDERING,
                                       &direct_rendering);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_GLX_EXTENSIONS,
                                       &glx_extensions);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }


        /* Get Server GLX information */
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_SERVER_VENDOR,
                                       &server_vendor);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_SERVER_VERSION,
                                       &server_version);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_SERVER_EXTENSIONS,
                                       &server_extensions);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }


        /* Get Client GLX information */
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_CLIENT_VENDOR,
                                       &client_vendor);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_CLIENT_VERSION,
                                       &client_version);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS,
                                       &client_extensions);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }


        /* Get OpenGL information */
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_OPENGL_VENDOR,
                                       &opengl_vendor);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_OPENGL_RENDERER,
                                       &opengl_renderer);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_OPENGL_VERSION,
                                       &opengl_version);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
        ret = NvCtrlGetStringAttribute(ctrl_target,
                                       NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS,
                                       &opengl_extensions);
        if ( ret != NvCtrlSuccess ) { ctk_glx->glx_available = FALSE; }
    } else {
        ctk_glx->glx_available = FALSE;
    }


    /* Get EGL information */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_EGL_VENDOR,
                                   &egl_vendor);
    if ( ret != NvCtrlSuccess ) { ctk_glx->egl_available = FALSE; }
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_EGL_VERSION,
                                   &egl_version);
    if ( ret != NvCtrlSuccess ) { ctk_glx->egl_available = FALSE; }
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_EGL_EXTENSIONS,
                                   &egl_extensions);
    if ( ret != NvCtrlSuccess ) { ctk_glx->egl_available = FALSE; }


    /* Get Vulkan information */

    /*
     * This query will gather the UUIDs of GPUs used by the screen this page
     * is assocatied with. Later when we display the data, we will compare these
     * UUIDs against the UUIDs we get from Vulkan. Earlier versions of Vulkan do
     * not make these UUIDs available so in that case we will simply display
     * available data from all GPUs.
     */
    ret = NvCtrlGetBinaryAttribute(ctrl_target,
              0,
              NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN,
              (unsigned char **)(&pData),
              &len);
    if (ret == NvCtrlSuccess) {
        int missing_any_uuid = 0;
        int num_gpus = pData[0];
        assoc_gpu_uuid = nvalloc((num_gpus + 1) * sizeof(gchar*));

        for (i = 0; i < num_gpus; i++) {
            gpu_target = NvCtrlGetTarget(ctrl_target->system,
                                         GPU_TARGET, pData[i+1]);
            if (gpu_target == NULL) {
                continue;
            }

            ret = NvCtrlGetStringAttribute(gpu_target, NV_CTRL_STRING_GPU_UUID,
                                           &assoc_gpu_uuid[i]);
            if (ret != NvCtrlSuccess) {
                missing_any_uuid++;
                break;
            }
        }

        if (missing_any_uuid) {
            for (i = 0; i < num_gpus; i++) {
                nvfree(assoc_gpu_uuid[i]);
            }
            nvfree(assoc_gpu_uuid);
            assoc_gpu_uuid = NULL;
        }

    }

    ret = NvCtrlGetStringAttribute(gpu_target,
                                   NV_CTRL_STRING_VK_API_VERSION,
                                   &vk_api_version);
    if ( ret != NvCtrlSuccess ) { ctk_glx->vulkan_available = FALSE; }

    ret = NvCtrlGetVoidAttribute(gpu_target,
                                 NV_CTRL_ATTR_VK_LAYER_INFO,
                                 (void*) vklp);
    if ( ret != NvCtrlSuccess ) { ctk_glx->vulkan_available = FALSE; }

    ret = NvCtrlGetVoidAttribute(gpu_target,
                                 NV_CTRL_ATTR_VK_DEVICE_INFO,
                                 (void*) vkdp);
    if ( ret != NvCtrlSuccess ) { ctk_glx->vulkan_available = FALSE; }


    if ( !ctk_glx->vulkan_available ) {
        nv_warning_msg("Vulkan Library Information unavailable.");
    }


    if (!ctk_glx->glx_available &&
        !ctk_glx->egl_available &&
        !ctk_glx->vulkan_available) {
        goto done;
    }


    /* Modify extension lists so they show only one name per line */
    for ( ptr = glx_extensions; ptr != NULL && ptr[0] != '\0'; ptr++ ) {
        if ( ptr[0] == ' ' ) ptr[0] = '\n';
    }
    for ( ptr = server_extensions; ptr != NULL && ptr[0] != '\0'; ptr++ ) {
        if ( ptr[0] == ' ' ) ptr[0] = '\n';
    }
    for ( ptr = client_extensions; ptr != NULL && ptr[0] != '\0'; ptr++ ) {
        if ( ptr[0] == ' ' ) ptr[0] = '\n';
    }
    for ( ptr = opengl_extensions; ptr != NULL && ptr[0] != '\0'; ptr++ ) {
        if ( ptr[0] == ' ' ) ptr[0] = '\n';
    }
    for ( ptr = egl_extensions; ptr != NULL && ptr[0] != '\0'; ptr++ ) {
        if ( ptr[0] == ' ' ) ptr[0] = '\n';
    }


    vbox = ctk_glx->glxinfo_vpane;
    gtk_widget_set_size_request(notebook, -1, 250);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    if (ctk_glx->glx_available) {

        /* Add (Shared) GLX information to widget */
        notebook_label = gtk_label_new("GLX");
        vbox2 = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        table = gtk_table_new(2, 2, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        add_table_row(table, 0,
                      0, 0, "Direct Rendering:",
                      0, 0,  direct_rendering);
        add_table_row(table, 1,
                      0, 0, "GLX Extensions:",
                      0, 0,  glx_extensions);

        if (ctk_glx->show_fbc_button) {
            GtkWidget *button_box = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(vbox2), button_box, FALSE, FALSE, 5);
            gtk_box_pack_start(GTK_BOX(button_box), ctk_glx->show_fbc_button,
                               FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);
        gtk_widget_show(GTK_WIDGET(scroll_win));


        /* Add server GLX information to widget */
        notebook_label = gtk_label_new("Server GLX");
        vbox2 = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        table = gtk_table_new(3, 2, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        add_table_row(table, 0,
                      0, 0, "Vendor:",
                      0, 0, server_vendor);
        add_table_row(table, 1,
                      0, 0, "Version:",
                      0, 0, server_version);
        add_table_row(table, 2,
                      0, 0, "Extensions:",
                      0, 0, server_extensions);

        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);


        /* Add client GLX information to widget */
        notebook_label = gtk_label_new("Client GLX");
        vbox2 = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        table = gtk_table_new(3, 2, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        add_table_row(table, 0,
                      0, 0, "Vendor:",
                      0, 0, client_vendor);
        add_table_row(table, 1,
                      0, 0, "Version:",
                      0, 0, client_version);
        add_table_row(table, 2,
                      0, 0, "Extensions:",
                      0, 0, client_extensions);

        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);


        /* Add OpenGL information to widget */
        notebook_label = gtk_label_new("OpenGL");
        vbox2 = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        table = gtk_table_new(4, 2, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        add_table_row(table, 0,
                      0, 0, "Vendor:",
                      0, 0, opengl_vendor);
        add_table_row(table, 1,
                      0, 0, "Renderer:",
                      0, 0, opengl_renderer);
        add_table_row(table, 2,
                      0, 0, "Version:",
                      0, 0, opengl_version);
        add_table_row(table, 3,
                      0, 0, "Extensions:",
                      0, 0, opengl_extensions);

        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);
    }


    /* Add EGL information to widget */
    if (ctk_glx->egl_available) {
        notebook_label = gtk_label_new("EGL");
        vbox2 = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

        table = gtk_table_new(3, 2, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);
        add_table_row(table, 0,
                      0, 0, "Vendor:",
                      0, 0, egl_vendor);
        add_table_row(table, 1,
                      0, 0, "Version:",
                      0, 0, egl_version);
        add_table_row(table, 2,
                      0, 0, "Extensions:",
                      0, 0, egl_extensions);

        if (ctk_glx->show_egl_fbc_button) {
            GtkWidget *button_box = gtk_hbox_new(FALSE, 5);
            gtk_box_pack_start(GTK_BOX(vbox2), button_box, FALSE, FALSE, 5);
            gtk_box_pack_start(GTK_BOX(button_box),
                               ctk_glx->show_egl_fbc_button,
                               FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(vbox2), table, FALSE, FALSE, 0);

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);
    }

    if (ctk_glx->vulkan_available) {
        char *str;
        GtkWidget *ibox;
        GtkWidget *general_frame = gtk_frame_new("General");
        GtkWidget *gbox = gtk_vbox_new(FALSE, 0);

        /* Add Vulkan information to widget */
        notebook_label = gtk_label_new("Vulkan");
        vbox2 = gtk_vbox_new(FALSE, 3);
        gtk_container_set_border_width(GTK_CONTAINER(vbox2), notebook_padding);
        scroll_win = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);

        /* Instance Extensions */
        expander = gtk_expander_new("Instance Information");
        ibox = gtk_hbox_new(FALSE, 0);

        table = gtk_table_new(2 + vklp->inst_extensions_count, 3, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);

        row = 0;
        add_table_row_3(table, row++, "API Version:", vk_api_version, "");

        if (vklp->instance_version) {
            add_table_row_3(table, row++, "Instance Version:",
                            vklp->instance_version, "");
        }

        str = nvasprintf("%d Extensions", vklp->inst_extensions_count);
        add_table_row_3(table, row++, "Instance Extensions:", str, "");
        nvfree(str);

        for (i = 0; i < vklp->inst_extensions_count; i++) {
            char *str = nvasprintf("Version: %d",
                                   vklp->inst_extensions[i].specVersion);

            add_table_row_3(table, i+row,
                            "",
                            vklp->inst_extensions[i].extensionName,
                            str);
            nvfree(str);
        }
        gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
        gtk_container_add(GTK_CONTAINER(expander), ibox);
        gtk_box_pack_start(GTK_BOX(gbox), expander, FALSE, FALSE, 0);


        /* Layers and Layer Extensions */
        expander = gtk_expander_new("Layers");
        ibox = gtk_hbox_new(FALSE, 0);
        table = gtk_table_new(2, 3, FALSE);
        gtk_table_set_row_spacings(GTK_TABLE(table), 3);
        gtk_table_set_col_spacings(GTK_TABLE(table), 15);


        str = nvasprintf("%d Layer(s)", vklp->inst_layer_properties_count);
        add_table_row_3(table, 0, "Layer Properties", str, "");
        nvfree(str);

        row = 1;
        for (i = 0; i < vklp->inst_layer_properties_count; i++) {
            int j;
            char *vstr = vulkan_get_version_string(
                             vklp->inst_layer_properties[i].specVersion);
            char *fstr = nvasprintf("%s - %d", vstr,
                             vklp->inst_layer_properties[i].implementationVersion);

            add_table_row_3(table, row++, "", "Name",
                            vklp->inst_layer_properties[i].layerName);
            add_table_row_3(table, row++, "", "Description",
                            vklp->inst_layer_properties[i].description);
            add_table_row_3(table, row++, "", "Version - Implementation", fstr);

            if (vklp->layer_extensions_count[i] == 0) {
                add_table_row_3(table, row++, "", "Layer Extensions", "None");
            } else {
                char *lstr = nvasprintf("Layer Extensions: %d",
                    vklp->layer_extensions_count[i]);
                GtkWidget *ext_expander = gtk_expander_new(lstr);
                GtkWidget *ext_ibox = gtk_hbox_new(FALSE, 0);
                GtkWidget *ext_table = gtk_table_new(1, 2, FALSE);
                gtk_table_set_row_spacings(GTK_TABLE(ext_table), 3);
                gtk_table_set_col_spacings(GTK_TABLE(ext_table), 15);
                nvfree(lstr);

                add_string_to_table(table, row, 0, "");
                gtk_table_attach(GTK_TABLE(table), ext_expander,
                                 1, 3, row, row + 1,
                                 GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                gtk_container_add(GTK_CONTAINER(ext_expander), ext_ibox);
                gtk_box_pack_start(GTK_BOX(ext_ibox), ext_table,
                                   FALSE, FALSE, INDENT_SIZE);
                row++;

                for (j = 0; j < vklp->layer_extensions_count[i]; j++) {
                    char *estr;
                    add_string_to_table(ext_table, j, 0,
                        vklp->layer_extensions[i][j].extensionName);
                    estr = nvasprintf("Version: %d",
                        vklp->layer_extensions[i][j].specVersion);
                    add_string_to_table(ext_table, j, 1, estr);
                }
            }

            for (device_num = 0;
                 device_num < vklp->phy_devices_count;
                 device_num++) {

                char *dstr;
                char *device_name_str;

                /*
                 * If we have UUIDs to compare between associated devices and
                 * Vulkan devices, look for matches and print only that data. If
                 * we have nothing to compare, display all device data.
                 */
                if (FALSE == check_associated_device(vkdp, device_num,
                                                     assoc_gpu_uuid)) {
                    continue;
                }

                if (vkdp->phy_device_properties[device_num].deviceName) {
                    device_name_str = nvasprintf("%s",
                        vkdp->phy_device_properties[device_num].deviceName);
                } else {
                    device_name_str = nvasprintf("Unknown");
                }
                dstr = nvasprintf("Physical Device %d", device_num);
                add_table_row_3(table, row++, "", dstr, device_name_str);
                nvfree(device_name_str);
                nvfree(dstr);

                if (vklp->layer_device_extensions_count[device_num] &&
                    vklp->layer_device_extensions_count[device_num][i] > 0) {
                    char *lstr = nvasprintf("Layer-Device Extensions: %d",
                        vklp->layer_device_extensions_count[device_num][i]);
                    GtkWidget *ext_expander = gtk_expander_new(lstr);
                    GtkWidget *ext_ibox = gtk_hbox_new(FALSE, 0);
                    GtkWidget *ext_table = gtk_table_new(1, 2, FALSE);
                    gtk_table_set_row_spacings(GTK_TABLE(ext_table), 3);
                    gtk_table_set_col_spacings(GTK_TABLE(ext_table), 15);
                    nvfree(lstr);

                    add_string_to_table(table, row, 0, "");
                    gtk_table_attach(GTK_TABLE(table), ext_expander,
                                     1, 3, row, row + 1,
                                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
                    gtk_box_pack_start(GTK_BOX(ext_ibox), ext_table,
                                       FALSE, FALSE, INDENT_SIZE);
                    gtk_container_add(GTK_CONTAINER(ext_expander), ext_ibox);

                    for (j = 0;
                         j < vklp->layer_device_extensions_count[device_num][i];
                         j++) {
                        char *estr = nvasprintf("Version: %d",
                            vklp->layer_device_extensions[device_num][i][j].specVersion);
                        add_string_to_table(ext_table, j, 0,
                            vklp->layer_device_extensions[device_num][i][j].extensionName);
                        add_string_to_table(ext_table, j, 1, estr);

                        nvfree(estr);
                    }

                } else {
                    add_table_row_3(table, row++, "",
                                    "Layer-Device Extensions:", "None");
                }

            }


            add_table_row_3(table, row++, "", "", "");
            nvfree(fstr);
            nvfree(vstr);




        }
        gtk_box_pack_start(GTK_BOX(ibox), table, FALSE, FALSE, INDENT_SIZE);
        gtk_container_add(GTK_CONTAINER(expander), ibox);
        gtk_box_pack_start(GTK_BOX(gbox), expander, FALSE, FALSE, 0);


        gtk_container_add(GTK_CONTAINER(general_frame), gbox);
        gtk_box_pack_start(GTK_BOX(vbox2), general_frame, FALSE, FALSE, 0);

        /*
         * Vulkan Device Information
         */
        for (device_num = 0;
             device_num < vkdp->phy_devices_count;
             device_num++) {

            GtkWidget *device_box = gtk_vbox_new(FALSE, 0);
            GtkWidget *device_frame;
            char *dstr;
            char *device_name_str;

            /*
             * If we have UUIDs to compare between associated devices and
             * Vulkan devices, look for matches and print only that data. If we
             * have nothing to compare, display all device data.
             */
            if (FALSE == check_associated_device(vkdp, device_num,
                                                 assoc_gpu_uuid)) {
                continue;
            }


            if (vkdp->phy_device_properties[device_num].deviceName) {
                device_name_str = nvasprintf(" - %s",
                    vkdp->phy_device_properties[device_num].deviceName);
            } else {
                device_name_str = nvasprintf("");
            }

            dstr = nvasprintf("Physical Device %d%s",
                              device_num, device_name_str);

            device_frame = gtk_frame_new(dstr);
            gtk_box_pack_start(GTK_BOX(vbox2),
                               device_frame,
                               FALSE, FALSE, 0);
            gtk_container_add(GTK_CONTAINER(device_frame), device_box);

            nvfree(device_name_str);
            nvfree(dstr);

            /* Device Properties */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_properties(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Device Extensions */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_extensions(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Sparse Properties */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_sparse_properties(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Limits */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_limits(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Device Features */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_features(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Queue Properties */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_queue_properties(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Memory Properties */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_memory_type_properties(vkdp, device_num),
                FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_device_memory_heap_properties(vkdp, device_num),
                FALSE, FALSE, 0);

            /* Formats */
            gtk_box_pack_start(GTK_BOX(device_box),
                populate_vulkan_formats(vkdp, device_num),
                FALSE, FALSE, 0);
        }

        ctk_scrolled_window_add(GTK_SCROLLED_WINDOW(scroll_win), vbox2);
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll_win,
                                 notebook_label);
    }


    /* Show the information */
    gtk_widget_show_all(GTK_WIDGET(ctk_glx));

    ctk_glx->glxinfo_initialized = True;

    /* Fall through */
 done:

    NvCtrlFreeVkLayerAttr(vklp);
    NvCtrlFreeVkDeviceAttr(vkdp);
    nvfree(vklp);
    nvfree(vkdp);

    /* Free temp strings */
    free(direct_rendering);
    free(glx_extensions);
    free(server_vendor);
    free(server_version);
    free(server_extensions);
    free(client_vendor);
    free(client_version);
    free(client_extensions);
    free(opengl_vendor);
    free(opengl_renderer);
    free(opengl_version);
    free(opengl_extensions);
    free(egl_vendor);
    free(egl_version);
    free(egl_extensions);
    free(vk_api_version);

} /* ctk_glx_probe_info() */



GtkTextBuffer *ctk_glx_create_help(GtkTextTagTable *table,
                                   CtkGLX *ctk_glx)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Graphics Information Help");
    ctk_help_para(b, &i,
                  "This page in the NVIDIA Settings Control Panel describes "
                  "information about graphics libraries available on this X "
                  "screen."
                  );

    if (ctk_glx->glx_fbconfigs_available) {
        ctk_help_heading(b, &i, "Show GLX Frame Buffer Configurations");
        ctk_help_para(b, &i, "%s", __show_fbc_help);
    }

    if (ctk_glx->glx_available) {

        ctk_help_heading(b, &i, "Direct Rendering");
        ctk_help_para(b, &i,
                      "This will tell you if direct rendering is available.  If "
                      "direct rendering is available, then a program running on "
                      "the same computer that the control panel is running on "
                      "will be able to bypass the X Server and take advantage of "
                      "faster rendering.  If direct rendering is not available, "
                      "then indirect rendering will be used and all rendering "
                      "will happen through the X Server."
                      );
        ctk_help_heading(b, &i, "GLX Extensions");
        ctk_help_para(b, &i,
                      "This is the list of GLX extensions that are supported by "
                      "both the client (libraries) and server (GLX extension to "
                      "the X Server)."
                      );

        ctk_help_heading(b, &i, "Server GLX Vendor String");
        ctk_help_para(b, &i,
                      "This is the vendor supplying the GLX extension running on "
                      "the X Server."
                      );
        ctk_help_heading(b, &i, "Server GLX Version String");
        ctk_help_para(b, &i,
                      "This is the version of the GLX extension running on the X "
                      "Server."
                      );
        ctk_help_heading(b, &i, "Server GLX Extensions");
        ctk_help_para(b, &i,
                      "This is the list of extensions supported by the GLX "
                      "extension running on the X Server."
                      );

        ctk_help_heading(b, &i, "Client GLX Vendor String");
        ctk_help_para(b, &i,
                      "This is the vendor supplying the GLX libraries."
                      );
        ctk_help_heading(b, &i, "Client GLX Version String");
        ctk_help_para(b, &i,
                      "This is the version of the GLX libraries."
                      );
        ctk_help_heading(b, &i, "Client GLX Extensions");
        ctk_help_para(b, &i,
                      "This is the list of extensions supported by the GLX "
                      "libraries."
                      );

        ctk_help_heading(b, &i, "OpenGL Vendor String");
        ctk_help_para(b, &i,
                      "This is the name of the vendor providing the OpenGL "
                      "implementation."
                     );
        ctk_help_heading(b, &i, "OpenGL Renderer String");
        ctk_help_para(b, &i,
                      "This shows the details of the graphics card on which "
                      "OpenGL is running."
                     );
        ctk_help_heading(b, &i, "OpenGL Version String");
        ctk_help_para(b, &i,
                      "This is the version of the OpenGL implementation."
                     );
        ctk_help_heading(b, &i, "OpenGL Extensions");
        ctk_help_para(b, &i,
                      "This is the list of OpenGL extensions that are supported "
                      "by this driver."
                     );
    }

    if (ctk_glx->egl_available) {
        ctk_help_heading(b, &i, "EGL Vendor String");
        ctk_help_para(b, &i,
                      "This is the vendor supplying the EGL implementation."
                     );
        ctk_help_heading(b, &i, "EGL Version String");
        ctk_help_para(b, &i,
                      "This is the version of the EGL implementation."
                     );
        ctk_help_heading(b, &i, "EGL Extensions");
        ctk_help_para(b, &i,
                      "This is the list of EGL extensions that are supported "
                      "by this driver."
                     );
    }

    if (ctk_glx->glx_fbconfigs_available) {
        ctk_help_heading(b, &i, "Show EGL Frame Buffer Configurations");
        ctk_help_para(b, &i, "%s", __show_egl_fbc_help);

        ctk_help_heading(b, &i, "GLX Frame Buffer Configurations");
        ctk_help_para(b, &i, "This table lists the supported GLX frame buffer "
                      "configurations for the display.");
        ctk_help_para(b, &i,
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"

                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"

                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n",

                      __fid_help,
                      __vid_help,
                      __vt_help,
                      __bfs_help,
                      __lvl_help,
                      __bf_help,
                      __db_help,
                      __st_help,
                      __rs_help,
                      __gs_help,
                      __bs_help,

                      __as_help,
                      __aux_help,
                      __dpt_help,
                      __stn_help,
                      __acr_help,
                      __acg_help,
                      __acb_help,
                      __aca_help,
                      __mvs_help,
                      __mcs_help,
                      __mb_help,

                      __cav_help,
                      __pbw_help,
                      __pbh_help,
                      __pbp_help,
                      __trt_help,
                      __trr_help,
                      __trg_help,
                      __trb_help,
                      __tra_help,
                      __tri_help
                     );
    }

    if (ctk_glx->egl_fbconfigs_available) {
        ctk_help_heading(b, &i, "EGL Frame Buffer Configurations");
        ctk_help_para(b, &i, "This table lists the supported EGL frame buffer "
                      "configurations for the display.");
        ctk_help_para(b, &i,
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"

                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"

                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"

                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n"
                      "\t%s\n\n",

                      __egl_id_help,
                      __egl_vid_help,
                      __egl_nvt_help,
                      __egl_bfs_help,
                      __egl_lvl_help,
                      __egl_cbt_help,
                      __egl_rs_help,
                      __egl_gs_help,

                      __egl_bs_help,
                      __egl_as_help,
                      __egl_ams_help,
                      __egl_lum_help,
                      __egl_dpt_help,
                      __egl_stn_help,
                      __egl_bt_help,
                      __egl_bta_help,

                      __egl_cfm_help,
                      __egl_spb_help,
                      __egl_smp_help,
                      __egl_cav_help,
                      __egl_pbw_help,
                      __egl_pbh_help,
                      __egl_pbp_help,
                      __egl_six_help,

                      __egl_sin_help,
                      __egl_nrd_help,
                      __egl_rdt_help,
                      __egl_sur_help,
                      __egl_tpt_help,
                      __egl_trv_help,
                      __egl_tgv_help,
                      __egl_tbv_help
                     );
    }

    ctk_help_finish(b);

    return b;

} /* ctk_glx_create_help() */


