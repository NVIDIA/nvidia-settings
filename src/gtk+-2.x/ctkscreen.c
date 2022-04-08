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

static const _CtkStereoMode stereoMode[] = {
    { NV_CTRL_STEREO_OFF,                          "Stereo Disabled" },
    { NV_CTRL_STEREO_DDC,                          "DDC Stereo" },
    { NV_CTRL_STEREO_BLUELINE,                     "Blueline Stereo" },
    { NV_CTRL_STEREO_DIN,                          "Onboard DIN Stereo" },
    { NV_CTRL_STEREO_PASSIVE_EYE_PER_DPY,          "Passive One-Eye-per-Display Stereo" },
    { NV_CTRL_STEREO_VERTICAL_INTERLACED,          "Vertical Interlaced Stereo" },
    { NV_CTRL_STEREO_COLOR_INTERLACED,             "Color Interleaved Stereo" },
    { NV_CTRL_STEREO_HORIZONTAL_INTERLACED,        "Horizontal Interlaced Stereo" },
    { NV_CTRL_STEREO_CHECKERBOARD_PATTERN,         "Checkerboard Pattern Stereo" },
    { NV_CTRL_STEREO_INVERSE_CHECKERBOARD_PATTERN, "Inverse Checkerboard Stereo" },
    { NV_CTRL_STEREO_3D_VISION,                    "NVIDIA 3D Vision Stereo" },
    { NV_CTRL_STEREO_3D_VISION_PRO,                "NVIDIA 3D Vision Pro Stereo" },
    { -1, NULL},
};

void ctk_screen_event_handler(GtkWidget *widget,
                              XRRScreenChangeNotifyEvent *ev,
                              gpointer data);

static void associated_displays_received(GtkObject *object, gpointer arg1,
                                         gpointer user_data);

static void info_update_gpu_error(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static const char *get_stereo_mode_string(int stereo_mode)
{
    int i;
    for (i = 0; stereoMode[i].name; i++) {
        if (stereoMode[i].stereo_mode == stereo_mode) {
            return stereoMode[i].name;
        }
    }
    return "Unknown";
}

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
 * given as "handle".
 */
static gchar *make_display_device_list(NvCtrlAttributeHandle *handle)
{
    return create_display_name_list_string(handle,
                                           NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN);
} /* make_display_device_list() */



/*
 * Calculations of the screen dimensions and resolution are based on
 * the dxpyinfo utility code.
 *
 * Copyright Information for xdpyinfo:
 *
 ***********************************************************************
 * 
 * xdpyinfo - print information about X display connecton
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

GtkWidget* ctk_screen_new(NvCtrlAttributeHandle *handle,
                          CtkEvent *ctk_event)
{
    GObject *object;
    CtkScreen *ctk_screen;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;

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

    screen_number = g_strdup_printf("%d", NvCtrlGetTargetId(handle));
    
    display_name = NvCtrlGetDisplayName(handle);
    
    dimensions = g_strdup_printf("%dx%d pixels (%dx%d millimeters)",
                                 NvCtrlGetScreenWidth(handle),
                                 NvCtrlGetScreenHeight(handle),
                                 NvCtrlGetScreenWidthMM(handle),
                                 NvCtrlGetScreenHeightMM(handle));
    
    /*
     * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
     *
     *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
     *         = N pixels / (M inch / 25.4)
     *         = N * 25.4 pixels / M inch
     */
    
    xres = (((double) NvCtrlGetScreenWidth(handle)) * 25.4) / 
        ((double) NvCtrlGetScreenWidthMM(handle));

    yres = (((double) NvCtrlGetScreenHeight(handle)) * 25.4) / 
        ((double) NvCtrlGetScreenHeightMM(handle));

    resolution = g_strdup_printf("%dx%d dots per inch", 
                                 (int) (xres + 0.5),
                                 (int) (yres + 0.5));
    
    depth = g_strdup_printf("%d", NvCtrlGetScreenPlanes(handle));
    
    /* get the list of GPUs driving this (logical) X screen */

    gpus = NULL;
    ret = NvCtrlGetBinaryAttribute(handle,
                                   0,
                                   NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN,
                                   (unsigned char **)(&pData),
                                   &len);
    if (ret == NvCtrlSuccess) {
        for (i = 1; i <= pData[0]; i++) {
            gchar *tmp_str;
            gchar *gpu_name;
            Bool valid;

            valid =
                XNVCTRLQueryTargetStringAttribute(NvCtrlGetDisplayPtr(handle),
                                                  NV_CTRL_TARGET_TYPE_GPU,
                                                  pData[i],
                                                  0,
                                                  NV_CTRL_STRING_PRODUCT_NAME,
                                                  &gpu_name);
            if (!valid) {
                gpu_name = "Unknown";
            }
            if (gpus) {
                tmp_str = g_strdup_printf("%s,\n%s (GPU %d)",
                                          gpus, gpu_name, pData[i]);
            } else {
                tmp_str = g_strdup_printf("%s (GPU %d)", gpu_name, pData[i]);
            }
            if (valid) {
                XFree(gpu_name);
            }
            g_free(gpus);
            gpus = tmp_str;
        }
        if (!gpus) {
            gpus = g_strdup("None");
        }
        XFree(pData);
    }

    /* get the list of Display Devices displaying this X screen */

    displays = make_display_device_list(handle);

    /* get the number of gpu errors occurred */

    gpu_errors = 0;
    ret = NvCtrlGetAttribute(handle,
                             NV_CTRL_NUM_GPU_ERRORS_RECOVERED,
                             (int *)&gpu_errors);

    snprintf(tmp, 16, "%d", gpu_errors);

    /* get the stereo mode set for this X screen */
    ret = NvCtrlGetAttribute(handle, NV_CTRL_STEREO, (int *)&stereo_mode);
    if (ret != NvCtrlSuccess) {
        stereo_mode = -1;
    }

    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_SCREEN, NULL);
    ctk_screen = CTK_SCREEN(object);

    /* cache the attribute handle */

    ctk_screen->handle = handle;

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
    add_table_row(table, 20, 0, 0, "Stereo Mode:", 0, 0,
                  get_stereo_mode_string(stereo_mode));

    g_free(screen_number);
    free(display_name);
    g_free(dimensions);
    g_free(resolution);
    g_free(depth);
    g_free(gpus);
    g_free(displays);
    
    /* Handle updates to the list of associated display devices */
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ASSOCIATED_DISPLAY_DEVICES),
                     G_CALLBACK(associated_displays_received),
                     (gpointer) ctk_screen);

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
                                      const gchar *screen_name)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "X Screen Information Help");

    ctk_help_para(b, &i, "This page in the NVIDIA "
                  "X Server Control Panel describes basic "
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

    ctk_help_heading(b, &i, "Stereo Mode");
    ctk_help_para(b, &i, "This is the stereo mode set for the X screen.");

    ctk_help_finish(b);

    return b;
}



/*
 * When XConfigureRequest events happens outside of the control panel,
 * they are trapped by this function so the gui can be updated
 * with the new screen information.
 */
void ctk_screen_event_handler(GtkWidget *widget,
                              XRRScreenChangeNotifyEvent *ev,
                              gpointer data)
{
    CtkScreen *ctk_screen = (CtkScreen *) data;

    gchar *dimensions =  g_strdup_printf("%dx%d pixels (%dx%d millimeters)",
                                         ev->width, ev->height,
                                         ev->mwidth, ev->mheight);

    gtk_label_set_text(GTK_LABEL(ctk_screen->dimensions),
                       dimensions);

    g_free(dimensions);

} /* ctk_screen_event_handler() */



/*
 * When the list of associated displays on this screen changes, we should
 * update the display device list shown on the page.
 */
static void associated_displays_received(GtkObject *object, gpointer arg1,
                                         gpointer user_data)
{
    CtkScreen *ctk_object = CTK_SCREEN(user_data);
    gchar *str;

    str = make_display_device_list(ctk_object->handle);

    gtk_label_set_text(GTK_LABEL(ctk_object->displays), str);

    g_free(str);

} /* associated_displays_received() */


/*
 * When the  number of gpu errors occured changes,
 * update the count showed on the page.
 */

static void info_update_gpu_error(GtkObject *object, gpointer arg1,
                                         gpointer data)
{
    CtkScreen *ctk_screen = (CtkScreen *) data;
    ReturnStatus ret;
    gint  gpu_errors = 0;
    char tmp[16];

    /* get the number of gpu errors occurred */
    ret = NvCtrlGetAttribute(ctk_screen->handle,
                             NV_CTRL_NUM_GPU_ERRORS_RECOVERED,
                             (int *)&gpu_errors);
    if (ret == NvCtrlSuccess) {
        snprintf(tmp, 16, "%d", gpu_errors);
        gtk_label_set_text(GTK_LABEL(ctk_screen->gpu_errors), tmp);
    }

} /* info_update_gpu_error() */
