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
#include <stdio.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "glxinfo.h" /* xxx_abbrev functions */

#include "ctkbanner.h"
#include "ctkglx.h"
#include "ctkutils.h"
#include "ctkconfig.h"
#include "ctkhelp.h"

#include <GL/glx.h> /* GLX #defines */


/* Number of FBConfigs attributes reported in gui */
#define NUM_FBCONFIG_ATTRIBS  32


/* FBConfig tooltips */
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
  "preformance.  '-' if it has no caveats.";
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
  "trr (Transparency red value) - Red value consided transparent.";
static const char * __trg_help =
  "trg (Transparency green value) - Green value consided transparent.";
static const char * __trb_help =
  "trb (Transparency blue value) - Blue value consided transparent.";
static const char * __tra_help =
  "tra (Transparency alpha value) - Alpha value consided transparent.";
static const char * __tri_help =
  "tri (Transparency index value) - Color index value consided transparent.";


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
        };

        ctk_glx_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkGLX", &ctk_glx_info, 0);
    }

    return ctk_glx_type;
} /* ctk_glx_get_type() */


static void dummy_button_signal(GtkWidget *widget,
                                 gpointer user_data)
{
    /* This is a dummy function so tooltips are enabled
     * for the fbconfig table column titles
     */
}


/* Creates the GLX information widget
 * 
 * NOTE: The GLX information other than the FBConfigs will
 *       be setup when this page is hooked up and the "parent-set"
 *       signal is thrown.  This will result in calling the
 *       ctk_glx_probe_info() function.
 */

typedef struct WidgetSizeRec {
    GtkWidget *widget;
    int width;
} WidgetSize;

GtkWidget* ctk_glx_new(NvCtrlAttributeHandle *handle,
                       CtkConfig *ctk_config, CtkEvent *ctk_event)
{
    GObject *object;
    CtkGLX *ctk_glx;
    GtkWidget *label;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *hbox, *hbox1;
    GtkWidget *vbox, *vbox2;
    GtkWidget *scrollWin;
    GtkWidget *event;    /* For setting the background color to white */
    GtkWidget *data_table, *header_table;
    GtkWidget *data_viewport, *full_viewport;
    GtkWidget *vscrollbar, *hscrollbar, *vpan;
    GtkRequisition req;
    ReturnStatus ret;

    char * glx_info_str = NULL;               /* Test if GLX supported */
    GLXFBConfigAttr *fbconfig_attribs = NULL; /* FBConfig data */
    int i;                                    /* Iterator */
    int num_fbconfigs = 0;
    char *err_str = NULL;

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

    WidgetSize fbconfig_header_sizes[NUM_FBCONFIG_ATTRIBS];

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


    /* Create the ctk glx object */
    object = g_object_new(CTK_TYPE_GLX, NULL);
    ctk_glx = CTK_GLX(object);


    /* Cache the attribute handle */
    ctk_glx->handle = handle;

    /* Set container properties of the object */
    ctk_glx->ctk_config = ctk_config;
    gtk_box_set_spacing(GTK_BOX(ctk_glx), 10);

    /* Image banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GLX);
    gtk_box_pack_start(GTK_BOX(ctk_glx), banner, FALSE, FALSE, 0);

    /* Determine if GLX is supported */
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_SERVER_VENDOR,
                                   &glx_info_str);
    free(glx_info_str);
    if ( ret != NvCtrlSuccess ) {
        err_str = "Fail to query the GLX server vendor.";
        goto fail;
    }


    /* Information Scroll Box */
    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox = gtk_hbox_new(FALSE, 0);
    vbox = gtk_vbox_new(FALSE, 5);
    event = gtk_event_box_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL, &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL, &(event->style->base[GTK_STATE_NORMAL]));
    gtk_container_add(GTK_CONTAINER(event), hbox);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          event);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    ctk_glx->glxinfo_vpane = vbox;
    gtk_widget_set_size_request(scrollWin, -1, 50);


    /* GLX 1.3 supports frame buffer configurations */
#ifdef GLX_VERSION_1_3

    /* Grab the FBConfigs */
    ret = NvCtrlGetVoidAttribute(handle, NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS,
                                 (void *)(&fbconfig_attribs));
    if ( ret != NvCtrlSuccess ) {
        err_str = "Failed to query list of GLX frame buffer configurations.";
        goto fail;
    }

    /* Count the number of fbconfigs */
    if ( fbconfig_attribs ) {
        for (num_fbconfigs = 0;
             fbconfig_attribs[num_fbconfigs].fbconfig_id != 0;
             num_fbconfigs++);
    }
    if ( ! num_fbconfigs ) {
        err_str = "No frame buffer configurations found.";
        
        goto fail;
    }


    /* Create clist in a scroll box */
    hbox1      = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("Frame Buffer Configurations");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), hseparator, TRUE, TRUE, 5);

    hbox      = gtk_hbox_new(FALSE, 0);
    vbox      = gtk_vbox_new(FALSE, 10);
    vbox2     = gtk_vbox_new(FALSE, 10);
    vpan      = gtk_vpaned_new();

    data_viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_size_request(data_viewport, 400, 50);
    vscrollbar = gtk_vscrollbar_new(gtk_viewport_get_vadjustment
                                        (GTK_VIEWPORT(data_viewport)));
    
    full_viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_size_request(full_viewport, 400, 50);
    hscrollbar = gtk_hscrollbar_new(gtk_viewport_get_hadjustment
                                     (GTK_VIEWPORT(full_viewport)));
    /*
     * NODE: Because clists have a hard time displaying tooltips in their
     *       column labels/buttons, we make the fbconfig table using a
     *       table widget.
     */

    /* Create the header table */

    header_table = gtk_table_new(num_fbconfigs, NUM_FBCONFIG_ATTRIBS, FALSE);
    
    for ( i = 0; i < NUM_FBCONFIG_ATTRIBS; i++ ) {
        GtkWidget * btn  = gtk_button_new_with_label(fbconfig_titles[i]);
        g_signal_connect(G_OBJECT(btn), "clicked",
                         G_CALLBACK(dummy_button_signal),
                         (gpointer) ctk_glx);
        ctk_config_set_tooltip(ctk_config, btn, fbconfig_tooltips[i]);
        gtk_table_attach(GTK_TABLE(header_table), btn,  i, i+1, 0, 1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
        
        fbconfig_header_sizes[i].widget = btn;
        gtk_widget_size_request(btn, &req);
        fbconfig_header_sizes[i].width = req.width;
    }
    
    /* Create the data table */

    data_table = gtk_table_new(num_fbconfigs, NUM_FBCONFIG_ATTRIBS, FALSE);
    event = gtk_event_box_new();
    
    gtk_widget_modify_fg(data_table, GTK_STATE_NORMAL, 
                         &(data_table->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(data_table, GTK_STATE_NORMAL, 
                         &(data_table->style->base[GTK_STATE_NORMAL]));
    gtk_container_add (GTK_CONTAINER(event), data_table);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL, 
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL, 
                         &(event->style->base[GTK_STATE_NORMAL]));
    gtk_container_add(GTK_CONTAINER(data_viewport), event);

    /* Pack the fbconfig header and data tables */

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), header_table, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data_viewport, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER(full_viewport), vbox);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), full_viewport, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hscrollbar, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    gtk_paned_pack1 (GTK_PANED (vpan), scrollWin, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (vpan), vbox, TRUE, FALSE);
    gtk_box_pack_start(GTK_BOX(ctk_glx), vpan, TRUE, TRUE, 0);
    
    /* Fill the data table */

    if ( fbconfig_attribs ) {

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
        
            /* Populate row cells */
            for ( cell = 0; cell < NUM_FBCONFIG_ATTRIBS ; cell++) {
                GtkWidget * label = gtk_label_new( str[cell] );
                gtk_label_set_justify( GTK_LABEL(label), GTK_JUSTIFY_CENTER);
                gtk_table_attach(GTK_TABLE(data_table), label,
                                 cell, cell+1, 
                                 i+1, i+2,
                                 GTK_EXPAND, GTK_EXPAND, 0, 0);

                /* Make sure the table headers are the same width
                 * as their table data column
                 */
                gtk_widget_size_request(label, &req);
                
                if ( fbconfig_header_sizes[cell].width > req.width ) {
                    gtk_widget_set_size_request(label,
                                                fbconfig_header_sizes[cell].width,
                                                -1);
                } else if ( fbconfig_header_sizes[cell].width < req.width ) {
                    fbconfig_header_sizes[cell].width = req.width + 6;
                    gtk_widget_set_size_request(fbconfig_header_sizes[cell].widget,
                                                fbconfig_header_sizes[cell].width,
                                                -1);
                }
            }
            i++;

        } /* Done - Populating FBconfig table */


        free(fbconfig_attribs);

    } /* Done - FBConfigs exist */
        
#endif /* GLX_VERSION_1_3 */


    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);


    /* Failure (no GLX) */
 fail:
    if (err_str) {
        label = gtk_label_new(err_str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);

        gtk_container_add(GTK_CONTAINER(ctk_glx), label);
    }

    /* Free memory that may have been allocated */
    free(fbconfig_attribs);
    
    gtk_widget_show_all(GTK_WIDGET(object));
    return GTK_WIDGET(object);

} /* ctk_glx_new */




/* Probes for GLX information and sets up the results
 * in the GLX widget.
 */
void ctk_glx_probe_info(GtkWidget *widget)
{
    CtkGLX *ctk_glx = CTK_GLX(widget);

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
    char *ptr;

    GtkWidget *hseparator;
    GtkWidget *hbox, *hbox2;
    GtkWidget *vbox, *vbox2;
    GtkWidget *label;
    GtkWidget *table;


    /* Make sure the widget was initialized and that glx information
     * has not yet been initialized.
     */
    if ( !ctk_glx || !ctk_glx->glxinfo_vpane ||
         ctk_glx->glxinfo_initialized ) {
        return;
    }


    /* Get GLX information */
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_DIRECT_RENDERING,
                                   &direct_rendering);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_GLX_EXTENSIONS,
                                   &glx_extensions);
    if ( ret != NvCtrlSuccess ) { goto done; }


    /* Get Server GLX information */
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_SERVER_VENDOR,
                                   &server_vendor);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_SERVER_VERSION,
                                   &server_version);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_SERVER_EXTENSIONS,
                                   &server_extensions);
    if ( ret != NvCtrlSuccess ) { goto done; }


    /* Get Client GLX information */
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_CLIENT_VENDOR,
                                   &client_vendor);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_CLIENT_VERSION,
                                   &client_version);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS,
                                   &client_extensions);
    if ( ret != NvCtrlSuccess ) { goto done; }


    /* Get OpenGL information */
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_OPENGL_VENDOR,
                                   &opengl_vendor);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_OPENGL_RENDERER,
                                   &opengl_renderer);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_OPENGL_VERSION,
                                   &opengl_version);
    if ( ret != NvCtrlSuccess ) { goto done; }
    ret = NvCtrlGetStringAttribute(ctk_glx->handle,
                                   NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS,
                                   &opengl_extensions);
    if ( ret != NvCtrlSuccess ) { goto done; }


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


    /* Add (Shared) GLX information to widget */
    vbox       = ctk_glx->glxinfo_vpane;
    vbox2      = gtk_vbox_new(FALSE, 0);
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("GLX Information");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    hbox  = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
    table = gtk_table_new(2, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    add_table_row(table, 0,
                  0, 0, "Direct Rendering:",
                  0, 0,  direct_rendering);
    add_table_row(table, 1,
                  0, 0, "GLX Extensions:",
                  0, 0,  glx_extensions);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);


    /* Add server GLX information to widget */   
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("Server GLX Information");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    hbox  = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
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
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);


    /* Add client GLX information to widget */
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("Client GLX Information");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    hbox  = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
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
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
    

    /* Add OpenGL information to widget */
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("OpenGL Information");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    hbox  = gtk_hbox_new(FALSE, 0);
    hbox2 = gtk_hbox_new(FALSE, 0);
    table = gtk_table_new(4, 2, FALSE);
    vbox2 = gtk_vbox_new(FALSE, 0);
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
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), vbox2, TRUE, TRUE, 2);


    /* Show the information */
    gtk_widget_show_all(GTK_WIDGET(ctk_glx));

    ctk_glx->glxinfo_initialized = True;

    /* Fall through */
 done:

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
    
} /* ctk_glx_probe_info() */



GtkTextBuffer *ctk_glx_create_help(GtkTextTagTable *table,
                                   CtkGLX *ctk_glx)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "GLX Help");
    ctk_help_para(b, &i,
                  "This page in the NVIDIA X Server Control Panel describes "
                  "information about the OpenGL extension to the X Server "
                  "(GLX)."
                  );

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


    ctk_help_heading(b, &i, "Frame Buffer Configurations");
    ctk_help_para(b, &i, "This table lists the supported frame buffer "
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

    ctk_help_finish(b);

    return b;

} /* ctk_glx_create_help() */


