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

#include <gtk/gtk.h>
#include "NvCtrlAttributes.h"

#include <stdio.h>

#include "big_banner_penguin.h"
#include "big_banner_bsd.h"
#include "big_banner_sun.h"

#include "image.h"

#include "ctkdevice.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#define N_GDK_PIXBUFS 45


GType ctk_device_get_type(
    void
)
{
    static GType ctk_device_type = 0;

    if (!ctk_device_type) {
        static const GTypeInfo info_ctk_device = {
            sizeof (CtkDeviceClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkDevice),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_device_type =
            g_type_register_static(GTK_TYPE_VBOX,
                                   "CtkDevice", &info_ctk_device, 0);
    }
    
    return ctk_device_type;
}



GtkWidget* ctk_device_new(
    NvCtrlAttributeHandle *handle
)
{
    GObject *object;
    CtkDevice *ctk_device;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *frame;
    GtkWidget *image;
    GtkWidget *hseparator;
    GtkWidget *table;
    GtkWidget *alignment;

    guint8 *image_buffer = NULL;
    const nv_image_t *img;

    char *product_name, *bus_type, *vbios_version, *video_ram, *irq;
    char *os, *arch, *version, *bus_rate, *bus;
    ReturnStatus ret;
    gint tmp, os_val;

    gchar *__unknown = "Unknown";

    /*
     * get the data that we will display below
     * 
     * XXX should be able to update any of this if an attribute
     * changes.
     */

    /* NV_CTRL_STRING_PRODUCT_NAME */

    ret = NvCtrlGetStringAttribute(handle, NV_CTRL_STRING_PRODUCT_NAME,
                                   &product_name);
    if (ret != NvCtrlSuccess) product_name = "Unknown GPU";
    
    /* NV_CTRL_BUS_TYPE */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_TYPE, &tmp);
    bus_type = NULL;
    if (ret == NvCtrlSuccess) {
        if (tmp == NV_CTRL_BUS_TYPE_AGP) bus_type = "AGP";
        if (tmp == NV_CTRL_BUS_TYPE_PCI) bus_type = "PCI";
        if (tmp == NV_CTRL_BUS_TYPE_PCI_EXPRESS) bus_type = "PCI Express";
    }
    if (!bus_type) bus_type = __unknown;

    /* NV_CTRL_BUS_RATE */

    bus_rate = NULL;
    if (tmp == NV_CTRL_BUS_TYPE_AGP ||
            tmp == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
        ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_RATE, &tmp);
        if (ret == NvCtrlSuccess) {
            bus_rate = g_strdup_printf("%dX", tmp);
        }
    }

    if (bus_rate != NULL) {
        bus = g_strdup_printf("%s %s", bus_type, bus_rate);
        g_free(bus_rate);
    } else {
        bus = g_strdup(bus_type);
    }

    /* NV_CTRL_STRING_VBIOS_VERSION */

    ret = NvCtrlGetStringAttribute(handle, NV_CTRL_STRING_VBIOS_VERSION,
                                   &vbios_version);
    if (ret != NvCtrlSuccess) vbios_version = __unknown;
    
    /* NV_CTRL_VIDEO_RAM */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_VIDEO_RAM, &tmp);
    if (ret != NvCtrlSuccess) tmp = 0;
    video_ram = g_strdup_printf("%d MB", tmp >> 10);
        
    /* NV_CTRL_IRQ */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_IRQ, &tmp);
    if (ret != NvCtrlSuccess) tmp = 0;
    irq = g_strdup_printf("%d", tmp);
    
    /* NV_CTRL_OPERATING_SYSTEM */

    os_val = NV_CTRL_OPERATING_SYSTEM_LINUX;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_OPERATING_SYSTEM, &os_val);
    os = NULL;
    if (ret == NvCtrlSuccess) {
        if (os_val == NV_CTRL_OPERATING_SYSTEM_LINUX) os = "Linux";
        else if (os_val == NV_CTRL_OPERATING_SYSTEM_FREEBSD) os = "FreeBSD";
        else if (os_val == NV_CTRL_OPERATING_SYSTEM_SUNOS) os = "SunOS";
    }
    if (!os) os = __unknown;

    /* NV_CTRL_ARCHITECTURE */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_ARCHITECTURE, &tmp);
    arch = NULL;
    if (ret == NvCtrlSuccess) {
        if (tmp == NV_CTRL_ARCHITECTURE_X86) arch = "x86";
        if (tmp == NV_CTRL_ARCHITECTURE_X86_64) arch = "x86_64";
        if (tmp == NV_CTRL_ARCHITECTURE_IA64) arch = "ia64";
    }
    if (!arch) arch = __unknown;
    os = g_strdup_printf("%s-%s", os, arch);

    /* NV_CTRL_STRING_NVIDIA_DRIVER_VERSION */

    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                   &version);
    if (ret != NvCtrlSuccess) version = __unknown;
    
    
    
    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_DEVICE, NULL);
    ctk_device = CTK_DEVICE(object);

    /* cache the attribute handle */

    ctk_device->handle = handle;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_device), 10);

    /* banner */

    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(ctk_device), alignment, FALSE, FALSE, 0);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(alignment), frame);

    if (os_val == NV_CTRL_OPERATING_SYSTEM_LINUX) {
        img = &big_banner_penguin_image;
    } else if (os_val == NV_CTRL_OPERATING_SYSTEM_FREEBSD) {
        img = &big_banner_bsd_image;
    } else if (os_val == NV_CTRL_OPERATING_SYSTEM_SUNOS) {
        img = &big_banner_sun_image;
    } else {
        img = &big_banner_penguin_image; /* Should never get here */
    }

    image_buffer = decompress_image_data(img);

    image = gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));
    
    gtk_container_add(GTK_CONTAINER(frame), image);
        
    /*
     * Device information: TOP->MIDDLE - LEFT->RIGHT
     *
     * This displays basic display adatper information, including
     * product name, bios version, bus type, video ram and interrupt
     * line.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_device), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Graphics Card Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(7, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    
    add_table_row(table, 0, 0, "Graphics Processor:", product_name);
    add_table_row(table, 1, 0, "Bus Type:", bus);
    add_table_row(table, 2, 0, "VBIOS Version:", vbios_version);
    add_table_row(table, 3, 0, "Video Memory:", video_ram);
    add_table_row(table, 4, 0, "IRQ:", irq);
    add_table_row(table, 5, 0, "Operating System:", os);
    add_table_row(table, 6, 0, "NVIDIA Driver Version:", version);

    g_free(bus);
    g_free(video_ram);
    g_free(irq);
    g_free(os);

    gtk_widget_show_all(GTK_WIDGET(object));
    
    return GTK_WIDGET(object);
}

    
GtkTextBuffer *ctk_device_create_help(GtkTextTagTable *table,
                                      const gchar *screen_name)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Graphics Card Information Help");

    ctk_help_para(b, &i, "This page in the NVIDIA "
                  "X Server Control Panel describes basic "
                  "information about the Graphics Processing Unit "
                  "(GPU) on which the X screen '%s' is running.",
                  screen_name);
    
    ctk_help_heading(b, &i, "Graphics Processor");
    ctk_help_para(b, &i, "This is the product name of the GPU.");
    
    ctk_help_heading(b, &i, "Bus Type");
    ctk_help_para(b, &i, "This is the bus type which is "
                  "used to connect the NVIDIA GPU to the rest of "
                  "your computer; possible values are AGP, PCI, or "
                  "PCI Express.");
    
    ctk_help_heading(b, &i, "VBIOS Version");
    ctk_help_para(b, &i, "This is the Video BIOS version.");
    
    
    ctk_help_heading(b, &i, "Video Memory"); 
    ctk_help_para(b, &i, "This is the amount of video memory on your "
                  "graphics card.");

    ctk_help_heading(b, &i, "IRQ");
    ctk_help_para(b, &i, "This is the interrupt request line assigned to "
                  "this GPU.");
    
    ctk_help_heading(b, &i, "Operating System");
    ctk_help_para(b, &i, "This is the operating system on which the NVIDIA "
                  "X driver is running; possible values are "
                  "'Linux' and 'FreeBSD'.  This also specifies the platform "
                  "on which the operating system is running, such as x86, "
                  "x86_64, or ia64");
    
    ctk_help_heading(b, &i, "NVIDIA Driver Version");
    ctk_help_para(b, &i, "This is the version of the NVIDIA Accelerated "
                  "Graphics Driver currently in use.");
    
    ctk_help_finish(b);

    return b;
}
