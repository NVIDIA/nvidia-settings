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
#include <stdlib.h>

#include "parse.h"

#include "ctkbanner.h"

#include "ctkgpu.h"
#include "ctkhelp.h"
#include "ctkutils.h"


static void probe_displays_received(GtkObject *object, gpointer arg1,
                                    gpointer user_data);

GType ctk_gpu_get_type(
    void
)
{
    static GType ctk_gpu_type = 0;

    if (!ctk_gpu_type) {
        static const GTypeInfo info_ctk_gpu = {
            sizeof (CtkGpuClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGpu),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_gpu_type =
            g_type_register_static(GTK_TYPE_VBOX,
                                   "CtkGpu", &info_ctk_gpu, 0);
    }
    
    return ctk_gpu_type;
}


static gchar *make_display_device_list(NvCtrlAttributeHandle *handle,
                                       unsigned int display_devices)
{
    gchar *displays = NULL;
    gchar *type;
    gchar *name;
    gchar *tmp_str;
    unsigned int mask;
    ReturnStatus ret;
    

    /* List of Display Device connected on GPU */

    for (mask = 1; mask; mask <<= 1) {
        
        if (!(mask & display_devices)) continue;
        
        type = display_device_mask_to_display_device_name(mask);
        name = NULL;
        
        ret =
            NvCtrlGetStringDisplayAttribute(handle,
                                            mask,
                                            NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                            &name);
        if (ret != NvCtrlSuccess) {
            tmp_str = g_strdup_printf("Unknown (%s)", type);
        } else {
            tmp_str = g_strdup_printf("%s (%s)", name, type);
            XFree(name);
        }
        free(type);
        
        if (displays) {
            name = g_strdup_printf("%s,\n%s", tmp_str, displays);
            g_free(displays);
            g_free(tmp_str);
        } else {
            name = tmp_str;
        }
        displays = name;
    }

    if (!displays) {
        displays = g_strdup("None");
    }

    return displays;

} /* make_display_device_list() */



GtkWidget* ctk_gpu_new(
    NvCtrlAttributeHandle *handle,
    CtrlHandleTarget *t,
    CtkEvent *ctk_event
)
{
    GObject *object;
    CtkGpu *ctk_gpu;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;

    char *product_name, *vbios_version, *video_ram, *irq;
    gchar *bus_type, *bus_rate, *bus;
    int pci_bus;
    int pci_device;
    int pci_func;
    gchar *pci_bus_id;

    gchar *__pci_bus_id_unknown = "?:?:?";

    int tmp;
    ReturnStatus ret;

    gchar *screens;
    gchar *displays;

    unsigned int display_devices;
    int xinerama_enabled;
    int *pData;
    int len;
    int i;


    /*
     * get the data that we will display below
     * 
     * XXX should be able to update any of this if an attribute
     * changes.
     */

    /* NV_CTRL_XINERAMA */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_XINERAMA, &xinerama_enabled);
    if (ret != NvCtrlSuccess) {
        xinerama_enabled = FALSE;
    }

    /* NV_CTRL_STRING_PRODUCT_NAME */

    ret = NvCtrlGetStringAttribute(handle, NV_CTRL_STRING_PRODUCT_NAME,
                                   &product_name);
    if (ret != NvCtrlSuccess) product_name = NULL;
    
    /* NV_CTRL_BUS_TYPE */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_TYPE, &tmp);
    bus_type = NULL;
    if (ret == NvCtrlSuccess) {
        if      (tmp == NV_CTRL_BUS_TYPE_AGP) bus_type = "AGP";
        else if (tmp == NV_CTRL_BUS_TYPE_PCI) bus_type = "PCI";
        else if (tmp == NV_CTRL_BUS_TYPE_PCI_EXPRESS) bus_type = "PCI Express";
        else if (tmp == NV_CTRL_BUS_TYPE_INTEGRATED) bus_type = "Integrated";
    }

    /* NV_CTRL_BUS_RATE */

    bus_rate = NULL;
    if (tmp == NV_CTRL_BUS_TYPE_AGP ||
        tmp == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
        ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_RATE, &tmp);
        if (ret == NvCtrlSuccess) {
            bus_rate = g_strdup_printf("%dX", tmp);
        }
    }

    if (bus_rate) {
        bus = g_strdup_printf("%s %s", bus_type, bus_rate);
        g_free(bus_rate);
    } else {
        bus = g_strdup(bus_type);
    }

    /* NV_CTRL_PCI_BUS & NV_CTRL_PCI_DEVICE & NV__CTRL_PCI_FUNCTION */

    pci_bus_id = NULL;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_BUS, &pci_bus);
    if (ret != NvCtrlSuccess) pci_bus_id = __pci_bus_id_unknown;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_DEVICE, &pci_device);
    if (ret != NvCtrlSuccess) pci_bus_id = __pci_bus_id_unknown;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_FUNCTION, &pci_func);
    if (ret != NvCtrlSuccess) pci_bus_id = __pci_bus_id_unknown;

    if (!pci_bus_id) {
        pci_bus_id = g_strdup_printf("%d:%d:%d",
                                     pci_bus, pci_device, pci_func);
    } else {
        pci_bus_id = g_strdup(__pci_bus_id_unknown);
    }

    /* NV_CTRL_STRING_VBIOS_VERSION */

    ret = NvCtrlGetStringAttribute(handle, NV_CTRL_STRING_VBIOS_VERSION,
                                   &vbios_version);
    if (ret != NvCtrlSuccess) vbios_version = NULL;
    
    /* NV_CTRL_VIDEO_RAM */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_VIDEO_RAM, &tmp);
    if (ret != NvCtrlSuccess) {
        video_ram = NULL;
    } else {
        video_ram = g_strdup_printf("%d MB", tmp >> 10);
    }

    /* NV_CTRL_IRQ */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_IRQ, &tmp);
    if (ret != NvCtrlSuccess) {
        irq = NULL;
    } else {
        irq = g_strdup_printf("%d", tmp);
    }
    
    /* List of X Screens using GPU */

    if (xinerama_enabled) {

        /* In Xinerama, there is only one logical X screen */

        screens = g_strdup("Screen 0 (Xinerama)");

    } else {
        gchar *tmp_str;
        screens = NULL;

        ret = NvCtrlGetBinaryAttribute(handle,
                                       0,
                                       NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                                       (unsigned char **)(&pData),
                                       &len);
        if (ret == NvCtrlSuccess) {
            for (i = 1; i <= pData[0]; i++) {
                
                if (screens) {
                    tmp_str = g_strdup_printf("%s,\nScreen %d",
                                              screens, pData[i]);
                } else {
                    tmp_str = g_strdup_printf("Screen %d", pData[i]);
                }
                g_free(screens);
                screens = tmp_str;
            }
            if (!screens) {
                screens = g_strdup("None");

            } else if (pData[0] > 0) {

                ret = NvCtrlGetAttribute(t[pData[1]].h,
                                         NV_CTRL_SHOW_SLI_HUD,
                                         &tmp);

                if (ret == NvCtrlSuccess) {
                    tmp_str = g_strdup_printf("%s (SLI)", screens);
                    g_free(screens);
                    screens = tmp_str;
                }
            }
            XFree(pData);
        }
    }

    /* List of Display Device connected on GPU */

    displays = NULL;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_CONNECTED_DISPLAYS,
                             (int *)&display_devices);
    if (ret == NvCtrlSuccess) {
        displays = make_display_device_list(handle, display_devices);
    }
    
    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_GPU, NULL);
    ctk_gpu = CTK_GPU(object);

    /* cache the attribute handle */

    ctk_gpu->handle = handle;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_gpu), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(ctk_gpu), banner, FALSE, FALSE, 0);
        
    /*
     * GPU information: TOP->MIDDLE - LEFT->RIGHT
     *
     * This displays basic display adatper information, including
     * product name, bios version, bus type, video ram and interrupt
     * line.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_gpu), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Graphics Card Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(17, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Graphics Processor:",
                  0, 0.5, product_name);
    add_table_row(table, 1,
                  0, 0.5, "VBIOS Version:",
                  0, 0.5, vbios_version);
    add_table_row(table, 2,
                  0, 0.5, "Memory:",
                  0, 0.5, video_ram);
    /* spacing */
    add_table_row(table, 6,
                  0, 0.5, "Bus Type:",
                  0, 0.5, bus);
    add_table_row(table, 7,
                  0, 0.5, "Bus ID:",
                  0, 0.5, pci_bus_id);
    add_table_row(table, 8,
                  0, 0.5, "IRQ:",
                  0, 0.5, irq);
    /* spacing */
    add_table_row(table, 12,
                  0, 0, "X Screens:",
                  0, 0, screens);
    /* spacing */
    ctk_gpu->displays =
        add_table_row(table, 16,
                      0, 0, "Display Devices:",
                      0, 0, displays);

    XFree(product_name);
    XFree(vbios_version);
    g_free(video_ram);
    g_free(bus);
    g_free(pci_bus_id);
    g_free(irq);
    g_free(screens);
    g_free(displays);

    gtk_widget_show_all(GTK_WIDGET(object));

    /* Handle events */
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                     G_CALLBACK(probe_displays_received),
                     (gpointer) ctk_gpu);

    return GTK_WIDGET(object);
}

    
GtkTextBuffer *ctk_gpu_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Graphics Card Information Help");

    ctk_help_para(b, &i, "This page in the NVIDIA "
                  "X Server Control Panel describes basic "
                  "information about the Graphics Processing Unit "
                  "(GPU).");
    
    ctk_help_heading(b, &i, "Graphics Processor");
    ctk_help_para(b, &i, "This is the product name of the GPU.");
    
    ctk_help_heading(b, &i, "VBIOS Version");
    ctk_help_para(b, &i, "This is the Video BIOS version.");
    
    ctk_help_heading(b, &i, "Memory"); 
    ctk_help_para(b, &i, "This is the overall amount of memory "
                  "available to your GPU.  With TurboCache(TM) GPUs, "
                  "this value may exceed the amount of video "
                  "memory installed on the graphics card.  With "
                  "integrated GPUs, the value may exceed the amount of "
                  "dedicated system memory set aside by the system "
                  "BIOS for use by the integrated GPU.");

    ctk_help_heading(b, &i, "Bus Type");
    ctk_help_para(b, &i, "This is the bus type which is "
                  "used to connect the NVIDIA GPU to the rest of "
                  "your computer; possible values are AGP, PCI, "
                  "PCI Express and Integrated.");
    
    ctk_help_heading(b, &i, "Bus ID");
    ctk_help_para(b, &i, "This is the GPU's PCI identification string, "
                  "reported in the form 'bus:device:function'. It uniquely "
                  "identifies the GPU's location in the host system. "
                  "This string can be used as-is with the 'BusID' X "
                  "configuration file option to unambiguously associate "
                  "Device sections with this GPU.");
    
    ctk_help_heading(b, &i, "IRQ");
    ctk_help_para(b, &i, "This is the interrupt request line assigned to "
                  "this GPU.");
    
    ctk_help_heading(b, &i, "X Screens");
    ctk_help_para(b, &i, "This is the list of X Screens driven by this GPU.");

    ctk_help_heading(b, &i, "Display Devices");
    ctk_help_para(b, &i, "This is the list of Display Devices (CRTs, TVs etc) "
                  "enabled on this GPU.");

    ctk_help_finish(b);

    return b;
}



static void probe_displays_received(GtkObject *object, gpointer arg1,
                                   gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkGpu *ctk_object = CTK_GPU(user_data);
    unsigned int probed_displays = event_struct->value;
    gchar *str;

    
    str = make_display_device_list(ctk_object->handle, probed_displays);

    gtk_label_set_text(GTK_LABEL(ctk_object->displays), str);

    g_free(str);
}
