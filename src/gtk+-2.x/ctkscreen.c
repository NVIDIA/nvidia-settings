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

#include <gtk/gtk.h>
#include "NvCtrlAttributes.h"
#include "NVCtrlLib.h"

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "parse.h"

#include "ctkscreen.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#include "ctkbanner.h"

#include "ctkglwidget.h"
#include "ctkglstereo.h"

void ctk_screen_event_handler(GtkWidget *widget,
                              CtrlEvent *event,
                              gpointer data);

static void info_update_gpu_error(GObject *object, gpointer arg1,
                                      gpointer user_data);

GType ctk_screen_get_type(
    void
)
{
    static GType ctk_screen_type = 0;

    if (!ctk_screen_type) {
        static const GTypeInfo info_ctk_screen = {
            sizeof (CtkScreenClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkScreen),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_screen_type =
            g_type_register_static(GTK_TYPE_VBOX,
                                   "CtkScreen", &info_ctk_screen, 0);
    }
    
    return ctk_screen_type;
}



/* Generates a list of display devices for the logical X screen
 * given as CtrlTarget.
 */
static gchar *make_display_device_list(CtrlTarget *ctrl_target)
{
    return create_display_name_list_string(ctrl_target,
                                           NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN);
} /* make_display_device_list() */



/*
 * Calculations of the screen dimensions and resolution are based on
 * the xdpyinfo utility code.
 *
 * Copyright Information for xdpyinfo:
 *
 ***********************************************************************
 * 
 * xdpyinfo - print information about X display connection
 *
 * 
Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Jim Fulton, MIT X Consortium
 *
 ***********************************************************************
 *
 */

GtkWidget* ctk_screen_new(CtrlTarget *ctrl_target, CtkEvent *ctk_event)
{
    GObject *object;
    CtkScreen *ctk_screen;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;
    GtkWidget *ctk_glstereo;

    ReturnStatus ret;

    gchar *screen_number;
    gchar *display_name;
    gchar *dimensions;
    gchar *resolution;
    gchar *depth;
    gchar *gpus;
    gchar *displays;
    gint  gpu_errors;
    gint  stereo_mode;

    char tmp[16];

    double xres, yres;

    int *pData;
    int len;
    int i;

    /*
     * get the data that we will display below
     */

    screen_number = g_strdup_printf("%d", NvCtrlGetTargetId(ctrl_target));

    display_name = NvCtrlGetDisplayName(ctrl_target);

    dimensions = g_strdup_printf("%dx%d pixels (%dx%d millimeters)",
                                 NvCtrlGetScreenWidth(ctrl_target),
                                 NvCtrlGetScreenHeight(ctrl_target),
                                 NvCtrlGetScreenWidthMM(ctrl_target),
                                 NvCtrlGetScreenHeightMM(ctrl_target));
    
    /*
     * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
     *
     *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
     *         = N pixels / (M inch / 25.4)
     *         = N * 25.4 pixels / M inch
     */
    
    xres = (((double) NvCtrlGetScreenWidth(ctrl_target)) * 25.4) / 
        ((double) NvCtrlGetScreenWidthMM(ctrl_target));

    yres = (((double) NvCtrlGetScreenHeight(ctrl_target)) * 25.4) / 
        ((double) NvCtrlGetScreenHeightMM(ctrl_target));

    resolution = g_strdup_printf("%dx%d dots per inch", 
                                 (int) (xres + 0.5),
                                 (int) (yres + 0.5));
    
    depth = g_strdup_printf("%d", NvCtrlGetScreenPlanes(ctrl_target));
    
    /* get the list of GPUs driving this (logical) X screen */

    gpus = NULL;
    ret = NvCtrlGetBinaryAttribute(ctrl_target,
                                   0,
                                   NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN,
                                   (unsigned char **)(&pData),
                                   &len);
    if (ret == NvCtrlSuccess) {
        for (i = 1; i <= pData[0]; i++) {
            CtrlTarget *gpu_target;
            gchar *tmp_str;
            gchar *gpu_name;

            gpu_target = NvCtrlGetTarget(ctrl_target->system,
                                         GPU_TARGET, pData[i]);
            if (gpu_target == NULL) {
                continue;
            }

            ret = NvCtrlGetStringAttribute(gpu_target,
                                           NV_CTRL_STRING_PRODUCT_NAME,
                                           &gpu_name);
            if (ret != NvCtrlSuccess) {
                gpu_name = "Unknown";
            }
            if (gpus) {
                tmp_str = g_strdup_printf("%s,\n%s (GPU %d)",
                                          gpus, gpu_name, pData[i]);
            } else {
                tmp_str = g_strdup_printf("%s (GPU %d)", gpu_name, pData[i]);
            }
            if (ret == NvCtrlSuccess) {
                free(gpu_name);
            }
            g_free(gpus);
            gpus = tmp_str;
        }
        if (!gpus) {
            gpus = g_strdup("None");
        }
        free(pData);
    }

    /* get the list of Display Devices displaying this X screen */

    displays = make_display_device_list(ctrl_target);

    /* get the number of gpu errors occurred */

    gpu_errors = 0;
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_NUM_GPU_ERRORS_RECOVERED,
                             (int *)&gpu_errors);

    snprintf(tmp, 16, "%d", gpu_errors);

    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_SCREEN, NULL);
    ctk_screen = CTK_SCREEN(object);

    /* cache the control target */

    ctk_screen->ctrl_target = ctrl_target;

    /* get the stereo mode set for this X screen */
    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_STEREO, (int *)&stereo_mode);
    ctk_screen->stereo_available = (ret == NvCtrlSuccess);

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_screen), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_X);
    gtk_box_pack_start(GTK_BOX(ctk_screen), banner, FALSE, FALSE, 0);
        
    /*
     * Screen information: TOP->MIDDLE - LEFT->RIGHT
     *
     * This displays basic X Screen information, including
     * the X Screen number, the display connection used to
     * talk to the X Screen, dimensions, resolution, depth (planes)
     * the list of GPUs driving the X Screen and the list of
     * display devices displaying the X Screen.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_screen), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("X Screen Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(20, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0, 0, 0.5, "Screen Number:", 0, 0.5, screen_number);
    add_table_row(table, 1, 0, 0.5, "Display Name:",  0, 0.5, display_name);
    /* spacing */
    ctk_screen->dimensions = 
        add_table_row(table, 5, 0, 0.5, "Dimensions:",    0, 0.5, dimensions);
    add_table_row(table, 6, 0, 0.5, "Resolution:",    0, 0.5, resolution);
    add_table_row(table, 7, 0, 0.5, "Depth:",         0, 0.5, depth);
    /* spacing */
    add_table_row(table, 11, 0, 0,   "GPUs:",          0, 0, gpus);
    /* spacing */
    ctk_screen->displays =
        add_table_row(table, 15, 0, 0,   "Displays:",      0, 0, displays);
    /* gpu errors */
    ctk_screen->gpu_errors =
        add_table_row(table, 19, 0, 0, "Recovered GPU Errors:", 0, 0, tmp);
    if (ctk_screen->stereo_available) {
        add_table_row(table, 20, 0, 0, "Stereo Mode:", 0, 0,
                      NvCtrlGetStereoModeName(stereo_mode));

        if (stereo_mode != NV_CTRL_STEREO_OFF) {
            ctk_glstereo = ctk_glstereo_new();
            if (ctk_glstereo) {
                hbox = gtk_hbox_new(FALSE, 0);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hbox), ctk_glstereo,
                                   FALSE, FALSE, 0);
            }
        }
    }

    g_free(screen_number);
    free(display_name);
    g_free(dimensions);
    g_free(resolution);
    g_free(depth);
    g_free(gpus);
    g_free(displays);

    /* Setup widget to handle XRRScreenChangeNotify events */
    g_signal_connect(G_OBJECT(ctk_event), "CTK_EVENT_RRScreenChangeNotify",
                     G_CALLBACK(ctk_screen_event_handler),
                     (gpointer) ctk_screen);

    /* Setup widget to reflect the latest number of gpu errors */
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_NUM_GPU_ERRORS_RECOVERED),
                     G_CALLBACK(info_update_gpu_error),
                     (gpointer) ctk_screen);


    gtk_widget_show_all(GTK_WIDGET(object));    

    return GTK_WIDGET(object);
}

    
GtkTextBuffer *ctk_screen_create_help(GtkTextTagTable *table,
                                      CtkScreen *ctk_screen,
                                      const gchar *screen_name)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "X Screen Information Help");

    ctk_help_para(b, &i, "This page in the NVIDIA "
                  "Settings Control Panel describes basic "
                  "information about the X Screen '%s'.",
                  screen_name);
    
    ctk_help_heading(b, &i, "Screen Number");
    ctk_help_para(b, &i, "This is the X Screen number.");
    
    ctk_help_heading(b, &i, "Display Name");
    ctk_help_para(b, &i, "This is the display connection string used to "
                  "communicate with the X Screen on the X Server.");
    
    ctk_help_heading(b, &i, "Dimensions");
    ctk_help_para(b, &i, "This displays the X Screen's horizontal and "
                  "vertical dimensions in pixels and millimeters.");
    
    ctk_help_heading(b, &i, "Resolution");
    ctk_help_para(b, &i, "This is the resolution (in dots per inch) of the "
                  "X Screen.");

    ctk_help_heading(b, &i, "Depth");
    ctk_help_para(b, &i, "This is the number of planes (depth) the X Screen "
                  "has available.");

    ctk_help_heading(b, &i, "GPUs");
    ctk_help_para(b, &i, "This is the list of GPUs that drive this X Screen.");

    ctk_help_heading(b, &i, "Display Devices");
    ctk_help_para(b, &i, "This is the list of Display Devices (CRTs, TVs etc) "
                  "enabled on this X Screen.");

    ctk_help_heading(b, &i, "Recovered GPU Errors");
    ctk_help_para(b, &i, "The GPU can encounter errors, either due to bugs in "
                  "the NVIDIA driver, or due to corruption of the command  "
                  "stream as it is sent from the NVIDIA X driver to the GPU.  "
                  "When the GPU encounters one of these errors, it reports it "
                  "to the NVIDIA X driver and the NVIDIA X driver attempts to "
                  "recover from the error.  This reports how many errors the "
                  "GPU received and the NVIDIA X driver successfully recovered "
                  "from.");

    if (ctk_screen->stereo_available) {
        ctk_help_heading(b, &i, "Stereo Mode");
        ctk_help_para(b, &i, "This is the stereo mode set for the X screen.");
    }

    ctk_help_finish(b);

    return b;
}



/*
 * When XConfigureRequest events happens outside of the control panel,
 * they are trapped by this function so the gui can be updated
 * with the new screen information.
 */
void ctk_screen_event_handler(GtkWidget *widget,
                              CtrlEvent *event,
                              gpointer data)
{
    CtkScreen *ctk_screen = (CtkScreen *) data;
    gchar *dimensions;

    if (event->type != CTRL_EVENT_TYPE_SCREEN_CHANGE) {
        return;
    }

    dimensions =  g_strdup_printf("%dx%d pixels (%dx%d millimeters)",
                                  event->screen_change.width,
                                  event->screen_change.height,
                                  event->screen_change.mwidth,
                                  event->screen_change.mheight);

    gtk_label_set_text(GTK_LABEL(ctk_screen->dimensions),
                       dimensions);

    g_free(dimensions);

} /* ctk_screen_event_handler() */



/*
 * When the number of gpu errors occurred changes,
 * update the count showed on the page.
 */

static void info_update_gpu_error(GObject *object, gpointer arg1,
                                         gpointer data)
{
    CtkScreen *ctk_screen = (CtkScreen *) data;
    ReturnStatus ret;
    gint  gpu_errors = 0;
    char tmp[16];

    /* get the number of gpu errors occurred */
    ret = NvCtrlGetAttribute(ctk_screen->ctrl_target,
                             NV_CTRL_NUM_GPU_ERRORS_RECOVERED,
                             (int *)&gpu_errors);
    if (ret == NvCtrlSuccess) {
        snprintf(tmp, 16, "%d", gpu_errors);
        gtk_label_set_text(GTK_LABEL(ctk_screen->gpu_errors), tmp);
    }

} /* info_update_gpu_error() */
