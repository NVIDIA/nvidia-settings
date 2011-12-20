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

#include <stdio.h>
#include <stdlib.h>

#include "parse.h"

#include "ctkbanner.h"

#include "ctkgpu.h"
#include "ctkhelp.h"
#include "ctkutils.h"

#include "XF86Config-parser/xf86Parser.h"

static void probe_displays_received(GtkObject *object, gpointer arg1,
                                    gpointer user_data);
#define ARRAY_ELEMENTS 16
#define DEFAULT_UPDATE_PCIE_INFO_TIME_INTERVAL 1000

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

void get_bus_type_str(NvCtrlAttributeHandle *handle,
                      gchar **bus)
{
    int tmp, ret, bus_type;
    gchar *bus_type_str, *bus_rate, *pcie_gen;

    bus_type = 0xffffffff;
    bus_type_str = "Unknown";
    ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_TYPE, &bus_type);
    if (ret == NvCtrlSuccess) {
        if      (bus_type == NV_CTRL_BUS_TYPE_AGP)
            bus_type_str = "AGP";
        else if (bus_type == NV_CTRL_BUS_TYPE_PCI)
            bus_type_str = "PCI";
        else if (bus_type == NV_CTRL_BUS_TYPE_PCI_EXPRESS)
            bus_type_str = "PCI Express";
        else if (bus_type == NV_CTRL_BUS_TYPE_INTEGRATED)
            bus_type_str = "Integrated";
    }

    /* NV_CTRL_BUS_RATE */

    bus_rate = NULL;
    if (bus_type == NV_CTRL_BUS_TYPE_AGP ||
        bus_type == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
        ret = NvCtrlGetAttribute(handle, NV_CTRL_BUS_RATE, &tmp);
        if (ret == NvCtrlSuccess) {
            if (bus_type == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
                bus_rate = g_strdup_printf("x%u", tmp);
            } else {
                bus_rate = g_strdup_printf("%uX", tmp);
            }
        }
    }

    /* NV_CTRL_GPU_PCIE_GENERATION */

    pcie_gen = NULL;
    if (bus_type == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
        ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_PCIE_GENERATION, &tmp);
        if (ret == NvCtrlSuccess)
            pcie_gen = g_strdup_printf("Gen%u", tmp);
    }

    /* concatenate all the available bus related information */

    if (bus_rate || pcie_gen) {
        *bus = g_strdup_printf("%s %s%s%s", bus_type_str,
                              bus_rate ? bus_rate : "",
                              bus_rate ? " " : "",
                              pcie_gen ? pcie_gen : "");
        g_free(bus_rate);
        g_free(pcie_gen);
    } else {
        *bus = g_strdup(bus_type_str);
    }
}

void get_bus_id_str(NvCtrlAttributeHandle *handle,
                    gchar **pci_bus_id)
{
    int ret;
    int pci_domain, pci_bus, pci_device, pci_func;
    gchar *bus_id;
    const gchar *__pci_bus_id_unknown = "?@?:?:?";

    /* NV_CTRL_PCI_DOMAIN & NV_CTRL_PCI_BUS &
     * NV_CTRL_PCI_DEVICE & NV__CTRL_PCI_FUNCTION
     */

    bus_id = NULL;
    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_DOMAIN, &pci_domain);
    if (ret != NvCtrlSuccess) goto bus_id_fallback;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_BUS, &pci_bus);
    if (ret != NvCtrlSuccess) goto bus_id_fallback;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_DEVICE, &pci_device);
    if (ret != NvCtrlSuccess) goto bus_id_fallback;

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_FUNCTION, &pci_func);
    if (ret != NvCtrlSuccess) goto bus_id_fallback;

    bus_id = malloc(32);
    if (bus_id) {
        xconfigFormatPciBusString(bus_id, 32, pci_domain, pci_bus,
                                  pci_device, pci_func);
    }

 bus_id_fallback:

    if (!bus_id) {
        bus_id = g_strdup(__pci_bus_id_unknown);
    }
    
    *pci_bus_id = bus_id;
}

static gboolean update_pcie_info(gpointer user_data)
{
    gchar *bus;
    gint link_speed;
    gchar *link_speed_str;
    ReturnStatus ret;

    CtkGpu *ctk_gpu = (CtkGpu *) user_data;
    get_bus_type_str(ctk_gpu->handle, &bus);
    gtk_label_set_text(GTK_LABEL(ctk_gpu->bus_label), bus);
    
    if (ctk_gpu->pcie_gen_queriable) {
        ret = NvCtrlGetAttribute(ctk_gpu->handle, NV_CTRL_GPU_PCIE_MAX_LINK_SPEED,
                                 &link_speed);
        if (ret != NvCtrlSuccess) {
            link_speed_str = g_strdup_printf("Unknown");
        }
        else {
            link_speed_str = g_strdup_printf("%d", link_speed);
        }
        gtk_label_set_text(GTK_LABEL(ctk_gpu->link_speed_label), link_speed_str);
        g_free(link_speed_str);
    }
    g_free(bus);
    return TRUE;
}



GtkWidget* ctk_gpu_new(
    NvCtrlAttributeHandle *handle,
    CtrlHandleTarget *t,
    CtkEvent *ctk_event,
    CtkConfig *ctk_config
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
    gchar *s;
    gchar *pci_bus_id;
    gchar pci_device_id[ARRAY_ELEMENTS];
    gchar pci_vendor_id[ARRAY_ELEMENTS];
    int pci_id;

    int tmp;
    ReturnStatus ret;

    gchar *screens;
    gchar *displays;
    gchar *tmp_str;
    gchar *gpu_cores;
    gchar *memory_interface;

    unsigned int display_devices;
    int xinerama_enabled;
    int *pData;
    int len;
    int i;
    int row = 0;
    int total_rows = 19;


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
    
    /* Get Bus related information */
    
    get_bus_id_str(handle, &pci_bus_id);
    
    /* NV_CTRL_PCI_ID */

    pci_device_id[ARRAY_ELEMENTS-1] = '\0';
    pci_vendor_id[ARRAY_ELEMENTS-1] = '\0';

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_ID, &pci_id);

    if (ret != NvCtrlSuccess) {
        snprintf(pci_device_id, ARRAY_ELEMENTS, "Unknown");
        snprintf(pci_vendor_id, ARRAY_ELEMENTS, "Unknown");
    } else {
        snprintf(pci_device_id, ARRAY_ELEMENTS, "0x%04x", (pci_id & 0xFFFF));
        snprintf(pci_vendor_id, ARRAY_ELEMENTS, "0x%04x", (pci_id >> 16));
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
    
    /* NV_CTRL_GPU_CORES */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_CORES, &tmp);
    if (ret != NvCtrlSuccess) {
        gpu_cores = NULL;
    } else {
        gpu_cores = g_strdup_printf("%d", tmp);
    }

    /* NV_CTRL_GPU_MEMORY_BUS_WIDTH  */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_MEMORY_BUS_WIDTH, &tmp);
    if (ret != NvCtrlSuccess) {
        memory_interface = NULL;
    } else {
        memory_interface = g_strdup_printf("%d-bit", tmp);
    }
    
    /* NV_CTRL_IRQ */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_IRQ, &tmp);
    if (ret != NvCtrlSuccess) {
        irq = NULL;
    } else {
        irq = g_strdup_printf("%d", tmp);
    }
    
    /* List of X Screens using the GPU */

    screens = NULL;
    ret = NvCtrlGetBinaryAttribute(handle,
                                   0,
                                   NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                                   (unsigned char **)(&pData),
                                   &len);
    if (ret == NvCtrlSuccess) {
        if (pData[0] == 0) {
            screens = g_strdup("None");
        } else {
            NvCtrlAttributeHandle *screen_handle;

            if (xinerama_enabled) {
                screens = g_strdup("Screen 0 (Xinerama)");
                /* XXX Use the only screen handle we have.
                 *     This is currently OK since we only
                 *     query xinerama attributes with this
                 *     handle below.  If we needed to query
                 *     a screen-specific attribute below,
                 *     then we would need to get a handle
                 *     for the correct screen instead.
                 */
                screen_handle = t[0].h;
            } else {
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
                screen_handle = t[pData[1]].h;
            }

            ret = NvCtrlGetAttribute(screen_handle,
                                     NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,
                                     &tmp);
            if (ret == NvCtrlSuccess) {
                tmp_str = g_strdup_printf("%s (SLI)", screens);
                g_free(screens);
                screens = tmp_str;
            }
        }
        XFree(pData);
    }
    if (!screens) {
        screens = g_strdup("Unknown");
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
    ctk_gpu->gpu_cores = (gpu_cores != NULL) ? 1 : 0;
    ctk_gpu->memory_interface = (memory_interface != NULL) ? 1 : 0;
    ctk_gpu->ctk_config = ctk_config;
    ctk_gpu->pcie_gen_queriable = FALSE;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_gpu), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(ctk_gpu), banner, FALSE, FALSE, 0);

    /* NV_CTRL_GPU_PCIE_GENERATION */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_PCIE_GENERATION, &tmp);
    if (ret == NvCtrlSuccess)
        ctk_gpu->pcie_gen_queriable = TRUE;

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

    table = gtk_table_new(total_rows, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, row++,
                  0, 0.5, "Graphics Processor:",
                  0, 0.5, product_name);
    if ( ctk_gpu->gpu_cores ) {
        gtk_table_resize(GTK_TABLE(table), ++total_rows, 2);
        add_table_row(table, row++,
                      0, 0.5, "CUDA Cores:",
                      0, 0.5, gpu_cores);
    }
    add_table_row(table, row++,
                  0, 0.5, "VBIOS Version:",
                  0, 0.5, vbios_version);
    add_table_row(table, row++,
                  0, 0.5, "Memory:",
                  0, 0.5, video_ram);
    if ( ctk_gpu->memory_interface ) {
        gtk_table_resize(GTK_TABLE(table), ++total_rows, 2);
        add_table_row(table, row++,
                      0, 0.5, "Memory Interface:",
                      0, 0.5, memory_interface);
    }
    /* spacing */
    row += 3;
    ctk_gpu->bus_label =
        add_table_row(table, row++,
                      0, 0.5, "Bus Type:",
                      0, 0.5, NULL);
    add_table_row(table, row++,
                  0, 0.5, "Bus ID:",
                  0, 0.5, pci_bus_id);
    add_table_row(table, row++,
                  0, 0.5, "PCI Device ID:",
                  0, 0.5, pci_device_id);
    add_table_row(table, row++,
                  0, 0.5, "PCI Vendor ID:",
                  0, 0.5, pci_vendor_id);
    add_table_row(table, row++,
                  0, 0.5, "IRQ:",
                  0, 0.5, irq);
    if (ctk_gpu->pcie_gen_queriable) { 
        /* spacing */
        row += 3;
        ctk_gpu->link_speed_label =
            add_table_row(table, row++,
                          0, 0.5, "PCI-E Max Link Speed:",
                          0, 0.5, NULL);
        row++;
    }

    /* spacing */
    row += 3;
    add_table_row(table, row++,
                  0, 0, "X Screens:",
                  0, 0, screens);
    /* spacing */
    row += 3;
    ctk_gpu->displays =
        add_table_row(table, row,
                      0, 0, "Display Devices:",
                      0, 0, displays);

    XFree(product_name);
    XFree(vbios_version);
    g_free(video_ram);
    g_free(gpu_cores);
    g_free(memory_interface);
    g_free(pci_bus_id);
    g_free(irq);
    g_free(screens);
    g_free(displays);

    gtk_widget_show_all(GTK_WIDGET(object));
    
    /* update PCI-E info */
    update_pcie_info((gpointer) ctk_gpu);

    /* Register a timer callback to update the PCI-E info */
    s = g_strdup_printf("Graphics Card (GPU %d)",
                        NvCtrlGetTargetId(handle));

    ctk_config_add_timer(ctk_config,
                         DEFAULT_UPDATE_PCIE_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_pcie_info,
                         (gpointer) ctk_gpu);
    g_free(s);

    /* Handle events */
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                     G_CALLBACK(probe_displays_received),
                     (gpointer) ctk_gpu);

    return GTK_WIDGET(object);
}

    
GtkTextBuffer *ctk_gpu_create_help(GtkTextTagTable *table, 
                                   CtkGpu *ctk_gpu)
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
    
    if (ctk_gpu->gpu_cores) {
        ctk_help_heading(b, &i, "CUDA Cores");
        ctk_help_para(b, &i, "This is the number of CUDA cores supported by "
                      "the graphics pipeline.");
    }
    
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

    if (ctk_gpu->memory_interface) {
        ctk_help_heading(b, &i, "Memory Interface");
        ctk_help_para(b, &i, "This is the bus bandwidth of the GPU's "
                      "memory interface.");
    }

    ctk_help_heading(b, &i, "Bus Type");
    ctk_help_para(b, &i, "This is the bus type which is "
                  "used to connect the NVIDIA GPU to the rest of "
                  "your computer; possible values are AGP, PCI, "
                  "PCI Express and Integrated.");
    
    ctk_help_heading(b, &i, "Bus ID");
    ctk_help_para(b, &i, "This is the GPU's PCI identification string, "
                  "in X configuration file 'BusID' format: "
                  "\"bus:device:function\", or, if the PCI domain of the GPU "
                  "is non-zero, \"bus@domain:device:function\".  Note "
                  "that all values are in decimal (as opposed to hexidecimal, "
                  "which is how `lspci` formats its BusID values).");

    ctk_help_heading(b, &i, "PCI Device ID");
    ctk_help_para(b, &i, "This is the PCI Device ID of the GPU.");
    
    ctk_help_heading(b, &i, "PCI Vendor ID");
    ctk_help_para(b, &i, "This is the PCI Vendor ID of the GPU.");
    
    ctk_help_heading(b, &i, "IRQ");
    ctk_help_para(b, &i, "This is the interrupt request line assigned to "
                  "this GPU.");
    
    if (ctk_gpu->pcie_gen_queriable) {
        ctk_help_heading(b, &i, "PCI-E Max Link Speed");
        ctk_help_para(b, &i, "This is the maximum PCI-E link speed.");
    }
    
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

void ctk_gpu_start_timer(GtkWidget *widget)
{
    CtkGpu *ctk_gpu = CTK_GPU(widget);

    /* Start the GPU timer */

    ctk_config_start_timer(ctk_gpu->ctk_config,
                           (GSourceFunc) update_pcie_info,
                           (gpointer) ctk_gpu);
}

void ctk_gpu_stop_timer(GtkWidget *widget)
{
    CtkGpu *ctk_gpu = CTK_GPU(widget);

    /* Stop the GPU timer */

    ctk_config_stop_timer(ctk_gpu->ctk_config,
                          (GSourceFunc) update_pcie_info,
                          (gpointer) ctk_gpu);
}

