/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
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
#include <string.h>

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "msg.h"

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkgvo.h"
#include "ctkgvi.h"
#include "ctkgpu.h"
#include "ctkbanner.h"

#define DEFAULT_UPDATE_VIDEO_FORMAT_INFO_TIME_INTERVAL 1000

static gboolean update_sdi_input_info(gpointer);

GType ctk_gvi_get_type(void)
{
    static GType ctk_gvi_type = 0;

    if (!ctk_gvi_type) {
        static const GTypeInfo ctk_gvi_info = {
            sizeof (CtkGviClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGvi),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_gvi_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkGvi",
                                   &ctk_gvi_info, 0);
    }

    return ctk_gvi_type;

} /* ctk_gvi_get_type() */

static const GvioFormatName samplingFormatNames[] = {
    { NV_CTRL_GVI_COMPONENT_SAMPLING_4444, "4:4:4:4"},
    { NV_CTRL_GVI_COMPONENT_SAMPLING_4224, "4:2:2:4"},
    { NV_CTRL_GVI_COMPONENT_SAMPLING_444,  "4:4:4"  },
    { NV_CTRL_GVI_COMPONENT_SAMPLING_422,  "4:2:2"  },
    { NV_CTRL_GVI_COMPONENT_SAMPLING_420,  "4:2:0"  },
    { -1, NULL },
};

static const GvioFormatName bitFormatNames[] = {
    { NV_CTRL_GVI_BITS_PER_COMPONENT_8,  "8 bpc" },
    { NV_CTRL_GVI_BITS_PER_COMPONENT_10, "10 bpc"},
    { NV_CTRL_GVI_BITS_PER_COMPONENT_12, "12 bpc"},
    { -1, NULL },
};

static const GvioFormatName colorSpaceFormatNames[] = {
    { NV_CTRL_GVI_COLOR_SPACE_GBR,    "GBR"   },
    { NV_CTRL_GVI_COLOR_SPACE_GBRA,   "GBRA"  },
    { NV_CTRL_GVI_COLOR_SPACE_GBRD,   "GBRD"  },
    { NV_CTRL_GVI_COLOR_SPACE_YCBCR,  "YCbCr" },
    { NV_CTRL_GVI_COLOR_SPACE_YCBCRA, "YCbCrA"},
    { NV_CTRL_GVI_COLOR_SPACE_YCBCRD, "YCbCrD"},
    { -1, NULL },
};

extern const GvioFormatName videoFormatNames[];


static gboolean update_sdi_input_info(gpointer user_data);

/*
 * ctk_gvio_get_format_name() - retrun name of format.
 */
static const char *ctk_gvio_get_format_name(const GvioFormatName *formatTable,
                                            const gint format)
{
    int i;
    for (i = 0; formatTable[i].name; i++) {
        if (formatTable[i].format == format) {
            return formatTable[i].name;
        }
    }
    return "Unknown";
}



/* 
 * update_sdi_input_info() - Update SDI input information.
 */

typedef struct {
    int video_format;
    int component_sampling;
    int color_space;
    int bpc;
    int link_id;
    int smpte352_id;
} ChannelInfo;


static void query_channel_info(CtkGvi *ctk_gvi, int jack, int channel, ChannelInfo *channel_info)
{
    gint ret;
    unsigned int jack_channel = ((channel & 0xFFFF) << 16);
    jack_channel |= (jack & 0xFFFF);

            
    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT,
                                    &(channel_info->video_format));
    if (ret != NvCtrlSuccess) {
        channel_info->video_format = NV_CTRL_GVIO_VIDEO_FORMAT_NONE;
    }
    
    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVI_DETECTED_CHANNEL_COMPONENT_SAMPLING,
                                    &(channel_info->component_sampling));
    if (ret != NvCtrlSuccess) {
        channel_info->component_sampling =
            NV_CTRL_GVI_COMPONENT_SAMPLING_UNKNOWN;
    }
    
    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVI_DETECTED_CHANNEL_COLOR_SPACE,
                                    &(channel_info->color_space));
    if (ret != NvCtrlSuccess) {
        channel_info->color_space = NV_CTRL_GVI_COLOR_SPACE_UNKNOWN;
    }
    
    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVI_DETECTED_CHANNEL_BITS_PER_COMPONENT,
                                    &(channel_info->bpc));
    if (ret != NvCtrlSuccess) {
        channel_info->bpc = NV_CTRL_GVI_BITS_PER_COMPONENT_UNKNOWN;
    }
    
    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVI_DETECTED_CHANNEL_LINK_ID,
                                    &(channel_info->link_id));
    if (ret != NvCtrlSuccess) {
        channel_info->link_id = NV_CTRL_GVI_LINK_ID_UNKNOWN;
    }

    ret = NvCtrlGetDisplayAttribute(ctk_gvi->handle,
                                    jack_channel,
                                    NV_CTRL_GVI_DETECTED_CHANNEL_SMPTE352_IDENTIFIER,
                                    &(channel_info->smpte352_id));
    if (ret != NvCtrlSuccess) {
        channel_info->smpte352_id = 0x0;
    }
}


static void update_sdi_input_info_simple(CtkGvi *ctk_gvi)
{
    GtkBox *vbox = GTK_BOX(ctk_gvi->input_info_vbox);
    GtkWidget *label;
    gchar *label_str;
    gint jack;
    gint channel;
    const char *vidfmt_str;
    GtkWidget *box = NULL;
    

    /* If not showing detailed information,
     * Show single entry for active jack/channel pairs as:
     *
     *   Jack #, Channel #: VIDEO FORMAT
     */

    for (jack = 0; jack < ctk_gvi->num_jacks; jack++) {
        ChannelInfo channel_infos[ctk_gvi->max_channels_per_jack];
        ChannelInfo *channel_info;
        int num_active_channels = 0;
        int show_channel = 0; /* When 0 or 1 active channel detected */

        /* Get information for each channel in the jack. */
        for (channel = 0; channel < ctk_gvi->max_channels_per_jack;
             channel++) {

            channel_info = channel_infos + channel;
            query_channel_info(ctk_gvi, jack, channel, channel_info);
            if (channel_info->video_format != NV_CTRL_GVIO_VIDEO_FORMAT_NONE) {
                show_channel = channel;
                num_active_channels++;
            }
        }

        /* Populate the info table */
        
        if (num_active_channels > 1) {
            box = gtk_vbox_new(FALSE, 0);
            gtk_box_pack_start(vbox, box, FALSE, FALSE, 0);

            label_str = g_strdup_printf("Jack %d:", jack+1);
            label = gtk_label_new(label_str);
            g_free(label_str);
            gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
            gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        }

        for (channel = 0; channel < ctk_gvi->max_channels_per_jack;
             channel++) {
            channel_info = channel_infos + channel;

            vidfmt_str = ctk_gvio_get_format_name(videoFormatNames,
                                                  channel_info->video_format);

            if (num_active_channels <= 1) {
                if (channel != show_channel) continue;
                label_str = g_strdup_printf("Jack %d: %s", jack+1, vidfmt_str);
                label = gtk_label_new(label_str);
                g_free(label_str);
                gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
                gtk_box_pack_start(vbox, label, FALSE, FALSE, 0);

            } else {
                label_str = g_strdup_printf("Channel %d: %s",
                                            channel+1, vidfmt_str);
                label = gtk_label_new(label_str);
                g_free(label_str);
                gtk_misc_set_padding(GTK_MISC(label), 5, 0);
                gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
                gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
            }
        }
    }
}


static void jack_channel_changed(GtkOptionMenu *optionmenu,
                                 gpointer user_data)
{
    CtkGvi *ctk_gvi = CTK_GVI(user_data);
    GtkWidget *menu;
    GtkWidget *menu_item;

    /* Track new selection */
    menu = gtk_option_menu_get_menu(optionmenu);
    menu_item = gtk_menu_get_active(GTK_MENU(menu));

    ctk_gvi->cur_jack_channel =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menu_item),
                                           "JACK_CHANNEL"));

    update_sdi_input_info(ctk_gvi);
}


static GtkWidget *create_jack_channel_menu(CtkGvi *ctk_gvi)
{
    GtkWidget *omenu;
    GtkWidget *menu;
    GtkWidget *menu_item;
    gint idx;
    gchar *label_str;
    gint jack;
    gint channel;
    unsigned int jack_channel;
    gint selected_idx = 0;

    /* Create the menu */

    omenu = gtk_option_menu_new();

    menu = gtk_menu_new();
    
    /* Just show all jack/channel pairs in dropdown */

    idx = 0;
    for (jack = 0; jack < ctk_gvi->num_jacks; jack++) {
        for (channel = 0; channel < ctk_gvi->max_channels_per_jack;
             channel++) {

            jack_channel = ((channel & 0xFFFF) << 16);
            jack_channel |= (jack & 0xFFFF);


            label_str = g_strdup_printf("Jack %d, Channel %d",
                                        jack+1, channel+1);
            menu_item = gtk_menu_item_new_with_label(label_str);
            g_free(label_str);

            g_object_set_data(G_OBJECT(menu_item),
                              "JACK_CHANNEL",
                              GUINT_TO_POINTER(jack_channel));

            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            gtk_widget_show(menu_item);

            if (jack_channel == ctk_gvi->cur_jack_channel) {
                selected_idx = idx;
            }

            idx++;
        }
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);

    gtk_option_menu_set_history(GTK_OPTION_MENU(omenu), selected_idx);

    g_signal_connect(G_OBJECT(omenu), "changed",
                     G_CALLBACK(jack_channel_changed),
                     (gpointer) ctk_gvi);

    return omenu;
}


static void update_sdi_input_info_all(CtkGvi *ctk_gvi)
{
    GtkBox *vbox = GTK_BOX(ctk_gvi->input_info_vbox);
    GtkWidget *box;
    GtkWidget *label;
    gchar *label_str;
    GtkWidget *table;
    gint jack;
    gint channel;
    const char *str;

    ChannelInfo channel_info;

    jack = ctk_gvi->cur_jack_channel & 0xFFFF;
    channel = (ctk_gvi->cur_jack_channel >> 16) & 0xFFFF;

    query_channel_info(ctk_gvi, jack, channel, &channel_info);

    box = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

    table = gtk_table_new(6, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 5);

    gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 0);

    /* Show channel's information in table format */

    label = gtk_label_new("Video Format:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
    
    str = ctk_gvio_get_format_name(videoFormatNames,
                                   channel_info.video_format);
    label_str = g_strdup_printf("%s", str);
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 0, 1);
    g_free(label_str);


    label = gtk_label_new("Component Sampling:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);

    str = ctk_gvio_get_format_name(samplingFormatNames,
                                   channel_info.component_sampling);
    label_str = g_strdup_printf("%s", str);
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);
    g_free(label_str);


    label = gtk_label_new("Color Space:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

    str = ctk_gvio_get_format_name(colorSpaceFormatNames,
                                   channel_info.color_space);
    label_str = g_strdup_printf("%s", str);
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 2, 3);
    g_free(label_str);


    label = gtk_label_new("Bits Per Component:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

    str = ctk_gvio_get_format_name(bitFormatNames,
                                   channel_info.bpc);
    label_str = g_strdup_printf("%s", str);
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 3, 4);
    g_free(label_str);
    
    
    label = gtk_label_new("Link ID:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 4, 5);
                                   
    if (channel_info.link_id == NV_CTRL_GVI_LINK_ID_UNKNOWN) {
        label_str = g_strdup_printf("Unknown");
    } else {
        label_str = g_strdup_printf("%d", channel_info.link_id);
    }
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 4, 5);
    g_free(label_str);

    label = gtk_label_new("SMPTE 352 Payload Identifier:");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 5, 6);

    label_str = g_strdup_printf("0x%08x",
                                (unsigned int) channel_info.smpte352_id);
    label = gtk_label_new(label_str);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 5, 6);
    g_free(label_str);
}


static gboolean update_sdi_input_info(gpointer user_data)
{
    CtkGvi *ctk_gvi = CTK_GVI(user_data);
    gboolean show_detailed_info;

    show_detailed_info =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON
                                     (ctk_gvi->show_detailed_info_btn));

    /* Dump out the old list */

    ctk_empty_container(ctk_gvi->input_info_vbox);

    if (!show_detailed_info) {
        gtk_widget_hide_all(GTK_WIDGET(ctk_gvi->jack_channel_omenu));
        update_sdi_input_info_simple(ctk_gvi);
    } else {
        gtk_widget_show_all(GTK_WIDGET(ctk_gvi->jack_channel_omenu));
        update_sdi_input_info_all(ctk_gvi);
    }

    gtk_widget_show_all(ctk_gvi->input_info_vbox);
    return TRUE;
}


static void show_detailed_info_button_toggled(GtkWidget *button,
                                              gpointer user_data)
{
    CtkGvi *ctk_gvi = CTK_GVI(user_data);
    gboolean active;

    active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    if (active) {
        gtk_button_set_label(GTK_BUTTON(button), "Show Condensed Input Info");
    } else {
        gtk_button_set_label(GTK_BUTTON(button), "Show Detailed Input Info");
    }

    update_sdi_input_info(ctk_gvi);
}

static gchar* gpu_name_string(gint gpu, CtrlHandles *handle)
{
    gchar *gpu_name;

    if ((gpu < 0) || (gpu >= handle->targets[GPU_TARGET].n)) {
        gpu_name = g_strdup_printf("None");
    } else {
        NvCtrlAttributeHandle *gpu_handle = handle->targets[GPU_TARGET].t[gpu].h;
        gpu_name = create_gpu_name_string(gpu_handle);
    }
    return gpu_name;
}

static void bound_gpu_changed(GtkObject *object, gpointer arg1,
                              gpointer user_data)
{
    CtkGvi *ctk_gvi = (CtkGvi *) user_data;
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    gchar *gpu_name;

    gpu_name = gpu_name_string(event_struct->value, ctk_gvi->ctk_config->pCtrlHandles);
    
    gtk_label_set_label(GTK_LABEL(ctk_gvi->gpu_name), gpu_name);
}

GtkWidget* ctk_gvi_new(NvCtrlAttributeHandle *handle,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkGvi *ctk_gvi;
    GtkWidget *hbox, *vbox, *hsep, *hseparator, *table, *button;
    GtkWidget *banner, *label;
    gchar *bus, *pci_bus_id, *irq, *gpu_name;
    int tmp;
    ReturnStatus ret;
    gchar *firmware_version; 
    gchar *s;
    
    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /*
     * get the static data that we will display below
     */

    /* Firmware Version */
    
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_GVIO_FIRMWARE_VERSION,
                                   &firmware_version);
    if (ret != NvCtrlSuccess) {
        firmware_version = g_strdup("Unable to determine");
    }

    /* Get Bus related information */

    get_bus_type_str(handle, &bus);
    get_bus_id_str(handle, &pci_bus_id);

    /* NV_CTRL_IRQ */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_IRQ, &tmp);
    if (ret != NvCtrlSuccess) {
        irq = NULL;
    } else {
        irq = g_strdup_printf("%d", tmp);
    }
   
    /* NV_CTRL_GVI_BOUND_GPU */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVI_BOUND_GPU, &tmp);
    if (ret != NvCtrlSuccess) {
        tmp = -1;
    }
    gpu_name = gpu_name_string(tmp, ctk_config->pCtrlHandles);

    /* create the CtkGvi object */

    object = g_object_new(CTK_TYPE_GVI, NULL);
    
    ctk_gvi = CTK_GVI(object);
    ctk_gvi->handle = handle;
    ctk_gvi->ctk_config = ctk_config;

    /* Query static GVI properties */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVI_NUM_JACKS,
                             &(ctk_gvi->num_jacks));
    if (ret != NvCtrlSuccess) {
        ctk_gvi->num_jacks = 0;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVI_MAX_CHANNELS_PER_JACK,
                             &(ctk_gvi->max_channels_per_jack));
    if (ret != NvCtrlSuccess) {
        ctk_gvi->max_channels_per_jack = 0;
    }

    /* set container properties for the CtkGvi widget */

    gtk_box_set_spacing(GTK_BOX(ctk_gvi), 5);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_GVI);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);


    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("GVI Device Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(8, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Firmware Version:",
                  0, 0.5, firmware_version);
    /* spacing */
    add_table_row(table, 2,
                  0, 0.5, "Bus Type:",
                  0, 0.5, bus);
    add_table_row(table, 3,
                  0, 0.5, "Bus ID:",
                  0, 0.5, pci_bus_id);
    /* spacing */
    add_table_row(table, 5,
                  0, 0.5, "IRQ:",
                  0, 0.5, irq);
    
    label = gtk_label_new("Bound GPU:");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 7, 8,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    label = gtk_label_new(gpu_name);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 7, 8,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    ctk_gvi->gpu_name = label;
    
    g_free(firmware_version);
    g_free(bus);
    g_free(pci_bus_id);
    g_free(irq);
    g_free(gpu_name);


    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Input Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hsep = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hsep, TRUE, TRUE, 5);

    /* Jack+Channel selection dropdown (hidden in condensed view) */
    
    ctk_gvi->jack_channel_omenu = create_jack_channel_menu(ctk_gvi);
    gtk_box_pack_start(GTK_BOX(vbox),
                       ctk_gvi->jack_channel_omenu, FALSE, FALSE, 0);

    /* Jack input info box */

    ctk_gvi->input_info_vbox = gtk_vbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(ctk_gvi->input_info_vbox), 5);
    gtk_box_pack_start(GTK_BOX(vbox),
                       ctk_gvi->input_info_vbox, FALSE, FALSE, 0);
    
    /* Register a timer callback to update the video format info */
    s = g_strdup_printf("Graphics Video In (GVI %d)",
                        NvCtrlGetTargetId(handle));

    ctk_config_add_timer(ctk_gvi->ctk_config,
                         DEFAULT_UPDATE_VIDEO_FORMAT_INFO_TIME_INTERVAL,
                         s,
                         (GSourceFunc) update_sdi_input_info,
                         (gpointer) ctk_gvi);
    g_free(s); 

    /* Condensed/Detailed view toggle button */

    button = gtk_toggle_button_new_with_label("Show Detailed Input Info");
    ctk_gvi->show_detailed_info_btn = button;

    hbox = gtk_hbox_new(FALSE, 5);

    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(show_detailed_info_button_toggled),
                     GTK_OBJECT(ctk_gvi));

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVI_BOUND_GPU),
                     G_CALLBACK(bound_gpu_changed),
                     (gpointer) ctk_gvi);

    gtk_widget_show_all(GTK_WIDGET(ctk_gvi));

    update_sdi_input_info(ctk_gvi);

    return GTK_WIDGET(ctk_gvi);
}

GtkTextBuffer *ctk_gvi_create_help(GtkTextTagTable *table,
                                       CtkGvi *ctk_gvi)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "GVI Device Information Help");
    ctk_help_para(b, &i, "This page in the NVIDIA "
                  "X Server Control Panel describes basic "
                  "information about the Graphics Video In "
                  "(GVI) device.");

    ctk_help_heading(b, &i, "Firmware Version");
    ctk_help_para(b, &i, "The Firmware Version reports the version "
                  "of the firmware running on the GVI device."); 

    ctk_help_heading(b, &i, "Bus Type");
    ctk_help_para(b, &i,  "This is the bus type which is "
                  "used to connect the NVIDIA GVI device to the rest of "
                  "your computer; possible values are AGP, PCI, "
                  "PCI Express and Integrated.");

    ctk_help_heading(b, &i, "Bus ID");
    ctk_help_para(b, &i, "This is the GVI device's PCI identification string, "
                  "reported in the form 'bus:device:function'.  It uniquely "
                  "identifies the GVI device's location in the host system.");

    ctk_help_heading(b, &i, "IRQ");
    ctk_help_para(b, &i, "This is the interrupt request line assigned to "
                  "this GVI device.");

    ctk_help_heading(b, &i, "Bound GPU");
    ctk_help_para(b, &i, "An OpenGL application can bind a GVI device to a "
                  "GPU using the GL_NV_video_capture OpenGL extension.  The "
                  "Bound GPU field reports if an OpenGL application has "
                  "currently bound this GVI device to a GPU.");

    ctk_help_heading(b, &i, "Input Information");
    ctk_help_para(b, &i, "This section shows the detected video format(s) on "
                  "each jack of the GVI device.  When condensed mode is "
                  "selected, the detected video format is shown for each "
                  "jack (and channel).  When detailed mode is selected, "
                  "information pertaining to the selected jack is reported.  "
                  "Note that the GVI device can only detect the following "
                  "information if the incoming signal has a non-zero SMPTE "
                  "352 payload identifier, which not all SDI devices provide.");

    ctk_help_para(b, &i, "Video Format:  The detected SMPTE video format.");
    ctk_help_para(b, &i, "Component Sampling: The detected composition of the "
                  "channel.");
    ctk_help_para(b, &i, "Color Space: The detected color space.");
    ctk_help_para(b, &i, "Bites Per Component: The detected number of bits "
                  "per component.");
    ctk_help_para(b, &i, "Link ID: The detected link ID of the channel.");

    ctk_help_finish(b);

    return b;
}

void ctk_gvi_start_timer(GtkWidget *widget)
{
    CtkGvi *ctk_gvi = CTK_GVI(widget);

    /* Start the GVI timer */

    ctk_config_start_timer(ctk_gvi->ctk_config,
                           (GSourceFunc) update_sdi_input_info,
                           (gpointer) ctk_gvi);
}

void ctk_gvi_stop_timer(GtkWidget *widget)
{
    CtkGvi *ctk_gvi = CTK_GVI(widget);

    /* Stop the GVI timer */

    ctk_config_stop_timer(ctk_gvi->ctk_config,
                          (GSourceFunc) update_sdi_input_info,
                          (gpointer) ctk_gvi);
}
