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
#include <string.h>

#include "msg.h"
#include "parse.h"

#include "ctkbanner.h"

#include "ctkgpu.h"
#include "ctkhelp.h"
#include "ctkutils.h"

#include "XF86Config-parser/xf86Parser.h"

static void probe_displays_received(GtkObject *object, gpointer arg1,
                                    gpointer user_data);
static gboolean update_gpu_usage(gpointer);

#define ARRAY_ELEMENTS 16
#define DEFAULT_UPDATE_GPU_INFO_TIME_INTERVAL 3000

typedef struct {
    gint graphics;
    gint video;
    gint pcie;
} utilizationEntry, * utilizationEntryPtr;

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
            NULL  /* value_table */
        };

        ctk_gpu_type =
            g_type_register_static(GTK_TYPE_VBOX,
                                   "CtkGpu", &info_ctk_gpu, 0);
    }
    
    return ctk_gpu_type;
}


static gchar *make_display_device_list(NvCtrlAttributeHandle *handle)
{
    return create_display_name_list_string(handle,
                                           NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU);

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

    /* NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH */

    bus_rate = NULL;
    if (bus_type == NV_CTRL_BUS_TYPE_AGP ||
        bus_type == NV_CTRL_BUS_TYPE_PCI_EXPRESS) {
        ret = NvCtrlGetAttribute(handle, NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH, &tmp);
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
        pcie_gen = get_pcie_generation_string(handle);
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

gchar *get_bus_id_str(NvCtrlAttributeHandle *handle)
{
    int ret;
    int pci_domain, pci_bus, pci_device, pci_func;
    gchar *bus_id;

    /* NV_CTRL_PCI_DOMAIN & NV_CTRL_PCI_BUS &
     * NV_CTRL_PCI_DEVICE & NV__CTRL_PCI_FUNCTION
     */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_DOMAIN, &pci_domain);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_BUS, &pci_bus);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_DEVICE, &pci_device);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_FUNCTION, &pci_func);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    bus_id = g_malloc(32);
    if (!bus_id) {
        return NULL;
    }
    xconfigFormatPciBusString(bus_id, 32, pci_domain, pci_bus,
                              pci_device, pci_func);
    return bus_id;
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

    char *product_name, *vbios_version, *video_ram, *gpu_memory_text, *irq;
    char *gpu_uuid;
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
    gchar *bus = NULL;
    gchar *link_speed_str = NULL;
    gchar *link_width_str = NULL;
    gchar *pcie_gen_str = NULL;

    int xinerama_enabled;
    int *pData;
    int len;
    int i;
    int row = 0;
    int total_rows = 21;
    int gpu_memory;


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
    if (ret != NvCtrlSuccess) {
        product_name = NULL;
    }

    ret = NvCtrlGetStringAttribute(handle, NV_CTRL_STRING_GPU_UUID,
                                   &gpu_uuid);
    if (ret != NvCtrlSuccess) {
        gpu_uuid = NULL;
    }

    /* Get Bus related information */

    pci_bus_id = get_bus_id_str(handle);

    /* NV_CTRL_PCI_ID */

    memset(&pci_device_id, 0, sizeof(pci_device_id));
    memset(&pci_vendor_id, 0, sizeof(pci_vendor_id));

    ret = NvCtrlGetAttribute(handle, NV_CTRL_PCI_ID, &pci_id);

    if (ret == NvCtrlSuccess) {
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

    /* NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY, 
                             &gpu_memory);
    if (ret != NvCtrlSuccess) {
        gpu_memory_text = NULL;
    } else {
        gpu_memory_text = g_strdup_printf("%d MB", gpu_memory);
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

    /* now, create the object */

    object = g_object_new(CTK_TYPE_GPU, NULL);
    ctk_gpu = CTK_GPU(object);

    /* cache the attribute handle */

    ctk_gpu->handle = handle;
    ctk_gpu->gpu_cores = (gpu_cores != NULL) ? 1 : 0;
    ctk_gpu->gpu_uuid = (gpu_uuid != NULL) ? 1 : 0;
    ctk_gpu->memory_interface = (memory_interface != NULL) ? 1 : 0;
    ctk_gpu->ctk_config = ctk_config;
    ctk_gpu->ctk_event = ctk_event;
    ctk_gpu->pcie_gen_queriable = FALSE;
    ctk_gpu->gpu_memory = gpu_memory;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_gpu), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GPU);
    gtk_box_pack_start(GTK_BOX(ctk_gpu), banner, FALSE, FALSE, 0);

    /* PCIe link information */
    get_bus_type_str(handle, &bus);
    pcie_gen_str = get_pcie_generation_string(handle);
    if (pcie_gen_str) {
        ctk_gpu->pcie_gen_queriable = TRUE;
        link_speed_str =
            get_pcie_link_speed_string(ctk_gpu->handle,
                                       NV_CTRL_GPU_PCIE_MAX_LINK_SPEED);
        link_width_str = get_pcie_link_width_string(ctk_gpu->handle,
                                                    NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH);
    }

    /*
     * GPU information: TOP->MIDDLE - LEFT->RIGHT
     *
     * This displays basic display adapter information, including
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
    if ( ctk_gpu->gpu_uuid ) {
        add_table_row(table, row++,
                      0, 0.5, "GPU UUID:",
                      0, 0.5, gpu_uuid);
    }
    if ( ctk_gpu->gpu_cores ) {
        gtk_table_resize(GTK_TABLE(table), ++total_rows, 2);
        add_table_row(table, row++,
                      0, 0.5, "CUDA Cores:",
                      0, 0.5, gpu_cores);
    }
    if ( vbios_version ) {
        add_table_row(table, row++,
                      0, 0.5, "VBIOS Version:",
                      0, 0.5, vbios_version);
    }
    add_table_row(table, row++,
                  0, 0.5, "Total Memory:",
                  0, 0.5, video_ram);
    add_table_row(table, row++,
                  0, 0.5, "Total Dedicated Memory:",
                  0, 0.5, gpu_memory_text);
    ctk_gpu->gpu_memory_used_label = 
        add_table_row(table, row++,
                      0, 0.5, "Used Dedicated Memory:",
                      0, 0.5, NULL);
    if ( ctk_gpu->memory_interface ) {
        gtk_table_resize(GTK_TABLE(table), ++total_rows, 2);
        add_table_row(table, row++,
                      0, 0.5, "Memory Interface:",
                      0, 0.5, memory_interface);
    }
    ctk_gpu->gpu_utilization_label =
        add_table_row(table, row++,
                      0, 0.5, "GPU Utilization:",
                      0, 0.5, NULL);
    ctk_gpu->video_utilization_label =
        add_table_row(table, row++,
                      0, 0.5, "Video Engine Utilization:",
                      0, 0.5, NULL);
    /* spacing */
    row += 3;
    add_table_row(table, row++,
                  0, 0.5, "Bus Type:",
                  0, 0.5, bus);
    if ( pci_bus_id ) {
        add_table_row(table, row++,
                      0, 0.5, "Bus ID:",
                      0, 0.5, pci_bus_id);
    }
    if ( pci_device_id[0] ) {
        add_table_row(table, row++,
                      0, 0.5, "PCI Device ID:",
                      0, 0.5, pci_device_id);
    }
    if (pci_vendor_id[0] ) {
        add_table_row(table, row++,
                      0, 0.5, "PCI Vendor ID:",
                      0, 0.5, pci_vendor_id);
    }
    if ( irq ) {
        add_table_row(table, row++,
                      0, 0.5, "IRQ:",
                      0, 0.5, irq);
    }
    if (ctk_gpu->pcie_gen_queriable) { 
        /* spacing */
        row += 3;
        add_table_row(table, row++,
                      0, 0.5, "PCIe Generation:",
                      0, 0.5, pcie_gen_str);
        add_table_row(table, row++,
                      0, 0.5, "Maximum PCIe Link Width:",
                      0, 0.5, link_width_str);
        add_table_row(table, row++,
                      0, 0.5, "Maximum PCIe Link Speed:",
                      0, 0.5, link_speed_str);
        ctk_gpu->pcie_utilization_label =
            add_table_row(table, row++,
                          0, 0.5, "PCIe Bandwidth Utilization:",
                          0, 0.5, NULL);

        g_free(link_speed_str);
        g_free(link_width_str);
        g_free(pcie_gen_str);
        row++;
    }

    update_gpu_usage(ctk_gpu);
    /* spacing */
    row += 3;
    add_table_row(table, row++,
                  0, 0, "X Screens:",
                  0, 0, screens);
    /* spacing */
    displays = make_display_device_list(handle);

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
    g_free(bus);
    g_free(gpu_memory_text);

    gtk_widget_show_all(GTK_WIDGET(object));
    
    /* Handle events */
    
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_PROBE_DISPLAYS),
                     G_CALLBACK(probe_displays_received),
                     (gpointer) ctk_gpu);

    tmp_str = g_strdup_printf("Memory Used (GPU %d)",
                              NvCtrlGetTargetId(handle));

    ctk_config_add_timer(ctk_gpu->ctk_config,
                         DEFAULT_UPDATE_GPU_INFO_TIME_INTERVAL,
                         tmp_str,
                         (GSourceFunc) update_gpu_usage,
                         (gpointer) ctk_gpu);
    g_free(tmp_str);

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
    
    if (ctk_gpu->gpu_uuid) {
        ctk_help_heading(b, &i, "GPU UUID");
        ctk_help_para(b, &i, "This is the global unique identifier "
                      "of the GPU.");
    }
    
    if (ctk_gpu->gpu_cores) {
        ctk_help_heading(b, &i, "CUDA Cores");
        ctk_help_para(b, &i, "This is the number of CUDA cores supported by "
                      "the graphics pipeline.");
    }
    
    ctk_help_heading(b, &i, "VBIOS Version");
    ctk_help_para(b, &i, "This is the Video BIOS version.");
    
    ctk_help_heading(b, &i, "Total Memory"); 
    ctk_help_para(b, &i, "This is the overall amount of memory "
                  "available to your GPU.  With TurboCache(TM) GPUs, "
                  "this value may exceed the amount of video "
                  "memory installed on the graphics card.  With "
                  "integrated GPUs, the value may exceed the amount of "
                  "dedicated system memory set aside by the system "
                  "BIOS for use by the integrated GPU.");

    ctk_help_heading(b, &i, "Total Dedicated Memory"); 
    ctk_help_para(b, &i, "This is the amount of memory dedicated "
                  "exclusively to your GPU.");

    ctk_help_heading(b, &i, "Used Dedicated Memory"); 
    ctk_help_para(b, &i, "This is the amount of dedicated memory used "
                  "by your GPU.");

    if (ctk_gpu->memory_interface) {
        ctk_help_heading(b, &i, "Memory Interface");
        ctk_help_para(b, &i, "This is the bus bandwidth of the GPU's "
                      "memory interface.");
    }

    ctk_help_heading(b, &i, "GPU Utilization");
    ctk_help_para(b, &i, "This is the percentage usage of graphics engine.");

    ctk_help_heading(b, &i, "Video Engine Utilization");
    ctk_help_para(b, &i, "This is the percentage usage of video engine");

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
                  "that all values are in decimal (as opposed to hexadecimal, "
                  "which is how `lspci` formats its BusID values).");

    ctk_help_heading(b, &i, "PCI Device ID");
    ctk_help_para(b, &i, "This is the PCI Device ID of the GPU.");
    
    ctk_help_heading(b, &i, "PCI Vendor ID");
    ctk_help_para(b, &i, "This is the PCI Vendor ID of the GPU.");
    
    ctk_help_heading(b, &i, "IRQ");
    ctk_help_para(b, &i, "This is the interrupt request line assigned to "
                  "this GPU.");

    if (ctk_gpu->pcie_gen_queriable) {
        ctk_help_heading(b, &i, "PCIe Generation");
        ctk_help_para(b, &i, "This is the PCIe generation that this GPU, in "
                      "this system, is compliant with.");

        ctk_help_heading(b, &i, "Maximum PCIe Link Width");
        ctk_help_para(b, &i, "This is the maximum width that the PCIe link "
                      "between the GPU and the system may be trained to.  This "
                      "is expressed in number of lanes.  The trained link "
                      "width may vary dynamically and possibly be narrower "
                      "based on the GPU's utilization and performance "
                      "settings.");

        ctk_help_heading(b, &i, "Maximum PCIe Link Speed");
        ctk_help_para(b, &i, "This is the maximum speed that the PCIe link "
                      "between the GPU and the system may be trained to.  "
                      "This is expressed in gigatransfers per second "
                      "(GT/s).  The link may be dynamically trained to a "
                      "slower speed, based on the GPU's utilization and "
                      "performance settings.");

        ctk_help_heading(b, &i, "PCIe Bandwidth Utilization");
        ctk_help_para(b, &i, "This is the percentage usage of "
                      "PCIe bandwidth.");

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
    CtkGpu *ctk_object = CTK_GPU(user_data);
    gchar *str;

    str = make_display_device_list(ctk_object->handle);

    gtk_label_set_text(GTK_LABEL(ctk_object->displays), str);

    g_free(str);
}



static void apply_gpu_utilization_token(char *token, char *value, void *data)
{
    utilizationEntryPtr pEntry = (utilizationEntryPtr) data;

    if (!strcasecmp("graphics", token)) {
        pEntry->graphics = atoi(value);
    } else if (!strcasecmp("video", token)) {
        pEntry->video = atoi(value);
    } else if (!strcasecmp("pcie", token)) {
        pEntry->pcie = atoi(value);
    } else {
        nv_warning_msg("Unknown GPU utilization token value pair: %s=%s",
                       token, value);
    }
}



static gboolean update_gpu_usage(gpointer user_data)
{
    CtkGpu *ctk_gpu;
    gchar *memory_text;
    gchar *utilization_text = NULL;
    ReturnStatus ret;
    gchar *utilizationStr = NULL;
    gint value = 0;
    utilizationEntry entry;

    ctk_gpu = CTK_GPU(user_data);

    ret = NvCtrlGetAttribute(ctk_gpu->handle, 
                             NV_CTRL_USED_DEDICATED_GPU_MEMORY, 
                             &value);
    if (ret != NvCtrlSuccess || value > ctk_gpu->gpu_memory || value < 0) {
        gtk_label_set_text(GTK_LABEL(ctk_gpu->gpu_memory_used_label), "Unknown");
        return FALSE;
    } else {
        if (ctk_gpu->gpu_memory > 0) {
            memory_text = g_strdup_printf("%d MB (%.0f%%)", 
                                          value, 
                                          100.0 * (double) value / 
                                              (double) ctk_gpu->gpu_memory);
        } else {
            memory_text = g_strdup_printf("%d MB", value);
        }

        gtk_label_set_text(GTK_LABEL(ctk_gpu->gpu_memory_used_label), memory_text);
        g_free(memory_text);
    }

    /* GPU utilization */
    ret = NvCtrlGetStringAttribute(ctk_gpu->handle,
                                   NV_CTRL_STRING_GPU_UTILIZATION,
                                   &utilizationStr);
    if (ret != NvCtrlSuccess) {
        gtk_label_set_text(GTK_LABEL(ctk_gpu->gpu_utilization_label), "Unknown");
        gtk_label_set_text(GTK_LABEL(ctk_gpu->video_utilization_label),
                           "Unknown");
        if (ctk_gpu->pcie_utilization_label) {
            gtk_label_set_text(GTK_LABEL(ctk_gpu->pcie_utilization_label),
                               "Unknown");
        }
        return FALSE;
    } else {
        memset(&entry, -1, sizeof(&entry));
        parse_token_value_pairs(utilizationStr, apply_gpu_utilization_token,
                                &entry);
        if (entry.graphics != -1) {
            utilization_text = g_strdup_printf("%d %%",
                                               entry.graphics);                                     

            gtk_label_set_text(GTK_LABEL(ctk_gpu->gpu_utilization_label),
                               utilization_text);
        }
        if (entry.video != -1) {
            utilization_text = g_strdup_printf("%d %%",
                                               entry.video);

            gtk_label_set_text(GTK_LABEL(ctk_gpu->video_utilization_label),
                               utilization_text);
        }
        if ((entry.pcie != -1) &&
            (ctk_gpu->pcie_utilization_label)) {
            utilization_text = g_strdup_printf("%d %%",
                                               entry.pcie);

            gtk_label_set_text(GTK_LABEL(ctk_gpu->pcie_utilization_label),
                               utilization_text);
        }
        g_free(utilization_text);
    }
    return TRUE;
}

void ctk_gpu_page_select(GtkWidget *widget)
{
    CtkGpu *ctk_gpu = CTK_GPU(widget);

    /* Update GPU usage */

    update_gpu_usage(ctk_gpu);

    /* Start the gpu timer */

    ctk_config_start_timer(ctk_gpu->ctk_config,
                           (GSourceFunc) update_gpu_usage,
                           (gpointer) ctk_gpu);
}

void ctk_gpu_page_unselect(GtkWidget *widget)
{
    CtkGpu *ctk_gpu = CTK_GPU(widget);

    /* Stop the gpu timer */

    ctk_config_stop_timer(ctk_gpu->ctk_config,
                          (GSourceFunc) update_gpu_usage,
                          (gpointer) ctk_gpu);
}

