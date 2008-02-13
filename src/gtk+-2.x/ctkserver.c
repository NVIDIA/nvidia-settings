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
#include <stdio.h> /* snprintf */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "ctkimage.h"

#include "ctkserver.h"
#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkutils.h"


GType ctk_server_get_type(void)
{
    static GType ctk_server_type = 0;

    if (!ctk_server_type) {
        static const GTypeInfo ctk_server_info = {
            sizeof (CtkServerClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkServer),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_server_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkServer", &ctk_server_info, 0);
    }

    return ctk_server_type;
}



/*
 * Code taken and modified from xdpyinfo.c
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
static gchar * get_server_vendor_version(NvCtrlAttributeHandle *handle)
{
    int vendrel = NvCtrlGetVendorRelease(handle);
    char *vendstr = NvCtrlGetServerVendor(handle);
    gchar *version = NULL;
    gchar *tmp;

    if (vendrel < 0 || !vendstr) return NULL;

    /* XFree86 */

    if (g_strrstr(vendstr, "XFree86")) {

        if (vendrel < 336) {
            /*
             * vendrel was set incorrectly for 3.3.4 and 3.3.5, so handle
             * those cases here.
             */
            version = g_strdup_printf("%d.%d.%d",
                                       vendrel / 100,
                                      (vendrel / 10) % 10,
                                       vendrel       % 10);
        } else if (vendrel < 3900) {
            /* 3.3.x versions, other than the exceptions handled above */
            if (((vendrel / 10) % 10) || (vendrel % 10)) {
                if (vendrel % 10) {
                    version = g_strdup_printf("%d.%d.%d.%d",
                                               vendrel / 1000,
                                              (vendrel /  100) % 10,
                                              (vendrel / 10) % 10,
                                               vendrel % 10);
                } else {
                    version = g_strdup_printf("%d.%d.%d",
                                               vendrel / 1000,
                                              (vendrel /  100) % 10,
                                              (vendrel / 10) % 10);
                }
            } else {
                version = g_strdup_printf("%d.%d",
                                           vendrel / 1000,
                                          (vendrel /  100) % 10);
            }
        } else if (vendrel < 40000000) {
            /* 4.0.x versions */
            if (vendrel % 10) {
                version = g_strdup_printf("%d.%d.%d",
                                           vendrel / 1000,
                                          (vendrel /   10) % 10,
                                           vendrel % 10);
            } else {
                version = g_strdup_printf("%d.%d",
                                           vendrel / 1000,
                                          (vendrel /   10) % 10);
            }
        } else {
            /* post-4.0.x */
            if (vendrel % 1000) {
                version = g_strdup_printf("%d.%d.%d.%d",
                                           vendrel / 10000000,
                                          (vendrel /   100000) % 100,
                                          (vendrel /     1000) % 100,
                                           vendrel %     1000);
            } else {
                version = g_strdup_printf("%d.%d.%d",
                                           vendrel / 10000000,
                                          (vendrel /   100000) % 100,
                                          (vendrel /     1000) % 100);
            }
        }
    }

    /* X.Org */

    if (g_strrstr(vendstr, "X.Org")) {
        tmp = g_strdup_printf("%d.%d.%d",
                               vendrel / 10000000,
                              (vendrel /   100000) % 100,
                              (vendrel /     1000) % 100);
        if (vendrel % 1000) {
            version = g_strdup_printf("%s.%d", tmp, vendrel % 1000); 
        } else {
            version = g_strdup(tmp);
        }

        g_free(tmp);
    }

    /* DMX */

    if (g_strrstr(vendstr, "DMX")) {
        int major, minor, year, month, day;

        major    = vendrel / 100000000;
        vendrel -= major   * 100000000;
        minor    = vendrel /   1000000;
        vendrel -= minor   *   1000000;
        year     = vendrel /     10000;
        vendrel -= year    *     10000;
        month    = vendrel /       100;
        vendrel -= month   *       100;
        day      = vendrel;

                                /* Add other epoch tests here */
        if (major > 0 && minor > 0) year += 2000;

                                /* Do some sanity tests in case there is
                                 * another server with the same vendor
                                 * string.  That server could easily use
                                 * values < 100000000, which would have
                                 * the effect of keeping our major
                                 * number 0. */
        if (major > 0 && major <= 20
            && minor >= 0 && minor <= 99
            && year >= 2000
            && month >= 1 && month <= 12
            && day >= 1 && day <= 31)
            version = g_strdup_printf("%d.%d.%04d%02d%02d\n",
                                      major, minor, year, month, day);
    }

    /* Add the vendor release number */
    
    if (version) {
        tmp = g_strdup_printf("%s (%d)", version, vendrel);
    } else {
        tmp = g_strdup_printf("%d", vendrel);
    }
    g_free(version);
    version = tmp;

    return version;
}



/*
 * CTK Server widget creation
 *
 */
GtkWidget* ctk_server_new(NvCtrlAttributeHandle *handle,
                          CtkConfig *ctk_config)
{
    GObject *object;
    CtkServer *ctk_object;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;

    gchar *os;
    gchar *arch;
    gchar *driver_version;

    gchar *dname = NvCtrlGetDisplayName(handle);
    gchar *display_name;
    gchar *server_version;
    gchar *vendor_str;
    gchar *vendor_ver;
    gchar *num_screens;

    ReturnStatus ret;
    int tmp, os_val;
    int xinerama_enabled;

    /*
     * get the data that we will display below
     *
     */
    
    /* NV_CTRL_XINERAMA */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_XINERAMA, &xinerama_enabled);
    if (ret != NvCtrlSuccess) {
        xinerama_enabled = FALSE;
    }

    /* NV_CTRL_OPERATING_SYSTEM */

    os_val = NV_CTRL_OPERATING_SYSTEM_LINUX;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_OPERATING_SYSTEM, &os_val);
    os = NULL;
    if (ret == NvCtrlSuccess) {
        if      (os_val == NV_CTRL_OPERATING_SYSTEM_LINUX) os = "Linux";
        else if (os_val == NV_CTRL_OPERATING_SYSTEM_FREEBSD) os = "FreeBSD";
        else if (os_val == NV_CTRL_OPERATING_SYSTEM_SUNOS) os = "SunOS";
    }
    if (!os) os = "Unknown";

    /* NV_CTRL_ARCHITECTURE */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ARCHITECTURE, &tmp);
    arch = NULL;
    if (ret == NvCtrlSuccess) {
        if      (tmp == NV_CTRL_ARCHITECTURE_X86) arch = "x86";
        else if (tmp == NV_CTRL_ARCHITECTURE_X86_64) arch = "x86_64";
        else if (tmp == NV_CTRL_ARCHITECTURE_IA64) arch = "ia64";
    }
    if (!arch) arch = "Unknown";
    os = g_strdup_printf("%s-%s", os, arch);

    /* NV_CTRL_STRING_NVIDIA_DRIVER_VERSION */

    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                   &driver_version);
    if (ret != NvCtrlSuccess) driver_version = NULL;

    /* Display Name */

    display_name = nv_standardize_screen_name(dname, -2);

    /* X Server Version */

    server_version = g_strdup_printf("%d.%d",
                                     NvCtrlGetProtocolVersion(handle),
                                     NvCtrlGetProtocolRevision(handle));

    /* Server Vendor String */
    
    vendor_str = g_strdup(NvCtrlGetServerVendor(handle));

    /* Server Vendor Version */

    vendor_ver = get_server_vendor_version(handle);

    /* # Logical X Screens */
    
    if (xinerama_enabled) {
        num_screens = g_strdup_printf("%d (Xinerama)",
                                      NvCtrlGetScreenCount(handle));
    } else {
        num_screens = g_strdup_printf("%d", NvCtrlGetScreenCount(handle));
    }

   
    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_SERVER, NULL);
    ctk_object = CTK_SERVER(object);

    /* cache the attribute handle */

    ctk_object->handle = handle;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_object), 10);

    /* banner */

    if (os_val == NV_CTRL_OPERATING_SYSTEM_LINUX) {
        banner = ctk_banner_image_new(BANNER_ARTWORK_PENGUIN);
    } else if (os_val == NV_CTRL_OPERATING_SYSTEM_FREEBSD) {
        banner = ctk_banner_image_new(BANNER_ARTWORK_BSD);
    } else if (os_val == NV_CTRL_OPERATING_SYSTEM_SUNOS) {
        banner = ctk_banner_image_new(BANNER_ARTWORK_SOLARIS);
    } else {
        banner = ctk_banner_image_new(BANNER_ARTWORK_PENGUIN);
    }
    gtk_box_pack_start(GTK_BOX(ctk_object), banner, FALSE, FALSE, 0);

    /*
     * This displays basic System information, including
     * display name, Operating system type and the NVIDIA driver version.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("System Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Operating System:",      0, 0.5, os);
    add_table_row(table, 1,
                  0, 0.5, "NVIDIA Driver Version:", 0, 0.5, driver_version);

    /*
     * This displays basic X Server information, including
     * version number, vendor information and the number of
     * X Screens.
     */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("X Server Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(11, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Display Name:",          0, 0.5, display_name);
    /* separator */
    add_table_row(table, 4,
                  0, 0.5, "Server Version Number:", 0, 0.5, server_version);
    add_table_row(table, 5,
                  0, 0.5, "Server Vendor String:",  0, 0.5, vendor_str);
    add_table_row(table, 6,
                  0, 0.5, "Server Vendor Version:", 0, 0.5, vendor_ver);
    /* separator */
    add_table_row(table, 10,
                  0, 0,   "X Screens:",             0, 0, num_screens);


    /* print special trademark text for FreeBSD */
    
    if (os_val == NV_CTRL_OPERATING_SYSTEM_FREEBSD) {

        hseparator = gtk_hseparator_new();
        gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);
        
        label = gtk_label_new(NULL);

        gtk_label_set_markup(GTK_LABEL(label),
                             "<span style=\"italic\" size=\"small\">"
                             "\n"
                             "The mark FreeBSD is a registered trademark "
                             "of The FreeBSD Foundation and is used by "
                             "NVIDIA with the permission of The FreeBSD "
                             "Foundation."
                             "\n\n"
                             "The FreeBSD Logo is a trademark of The "
                             "FreeBSD Foundation and is used by NVIDIA "
                             "with the permission of The FreeBSD "
                             "Foundation."
                             "\n"
                             "</span>");
        
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        
        hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    }
    

    g_free(display_name);
    g_free(os);
    XFree(driver_version);
    
    g_free(server_version);
    g_free(vendor_str);
    g_free(vendor_ver);
    g_free(num_screens);

    gtk_widget_show_all(GTK_WIDGET(object));
    
    return GTK_WIDGET(object);
}



/*
 * Server Information help screen
 */
GtkTextBuffer *ctk_server_create_help(GtkTextTagTable *table,
                                      CtkServer *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "X Server Information Help");

    ctk_help_heading(b, &i, "Operating System");
    ctk_help_para(b, &i, "This is the operating system on which the NVIDIA "
                  "X driver is running; possible values are "
                  "'Linux', 'FreeBSD', and 'SunOS'.  This also specifies the "
                  "platform on which the operating system is running, such "
                  "as x86, x86_64, or ia64");
    
    ctk_help_heading(b, &i, "NVIDIA Driver Version");
    ctk_help_para(b, &i, "This is the version of the NVIDIA Accelerated "
                  "Graphics Driver currently in use.");

    ctk_help_heading(b, &i, "Display Name");
    ctk_help_para(b, &i, "This is the display connection string used to "
                  "communicate with the X Server.");

    ctk_help_heading(b, &i, "X Server Version");
    ctk_help_para(b, &i, "This is the version number of the X Server.");

    ctk_help_heading(b, &i, "X Server Vendor String");
    ctk_help_para(b, &i, "This is the X Server vendor information string.");

    ctk_help_heading(b, &i, "X Server Vendor Version");
    ctk_help_para(b, &i, "This is the vertsion number of the X Server "
                  "vendor.");

    ctk_help_heading(b, &i, "X Screens");
    ctk_help_para(b, &i, "This is the number of X Screens on the "
                  "display.  (When Xinerama is enabled this is always 1).");

    ctk_help_finish(b);

    return b;
}
