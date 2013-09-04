/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2012 NVIDIA Corporation.
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
 *
 *
 * The VDPAU page is based on vdpauinfo 0.0.6:
 *
 *  http://cgit.freedesktop.org/~aplattner/vdpauinfo/
 *  http://people.freedesktop.org/~aplattner/vdpau/vdpauinfo-0.0.6.tar.gz
 *
 * which has the following copyright and license:
 *
 * Copyright (c) 2008 Wladimir J. van der Laan
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <dlfcn.h>

#include <gtk/gtk.h>

#include "ctkutils.h"
#include "ctkhelp.h"
#include "ctkvdpau.h"
#include "ctkbanner.h"

const gchar* __vdpau_information_label_help =
"This page shows information about the Video Decode and Presentation API for "
"Unix-like systems (VDPAU) library.";

const gchar* __base_information_help =
"This tab shows the VDPAU API version and supported codecs.";

const gchar* __vdpau_api_version_help =
"This shows the VDPAU API version.";

const gchar* __supported_codecs_help =
"This shows the supported codecs.";

const gchar* __surface_limits_help =
"This tab shows the maximum supported resolution and formats for video, "
"bitmap and output surfaces.";

const gchar* __video_surface_help =
"This shows the maximum supported resolution and formats for video surfaces.";

const gchar* __bitmap_surface_help =
"This shows the maximum supported resolution and formats for bitmap surfaces.";

const gchar* __ouput_surface_help =
"This shows the maximum supported resolution and formats for output surfaces.";

const gchar* __decoder_limits_help =
"This tab shows the maximum level, number of macroblocks and resolution for "
"each supported VDPAU decoder.";

const gchar* __video_mixer_help =
"This tab shows the capabilities of the VDPAU video mixer: the features, "
"parameters, and attributes.";

const gchar* __video_mixer_feature_help =
"This shows the features supported by the video mixer.";

const gchar* __video_mixer_parameter_help =
"This shows the video mixer parameters and any applicable ranges.";

const gchar* __video_mixer_attribute_help =
"This shows the video mixer attributes and any applicable ranges.";

static int queryOutputSurface(CtkVDPAU *ctk_vdpau, VdpDevice device,
                              VdpGetProcAddress *getProcAddress);

static int queryBitmapSurface(CtkVDPAU *ctk_vdpau, VdpDevice device,
                              VdpGetProcAddress *getProcAddress);

#define GETADDR(device, function_id, function_pointer) \
    getProcAddress(device, function_id, function_pointer)

static void getAddressVDPAUDeviceFunctions(VdpDevice device,
                                           VdpGetProcAddress *getProcAddress)
{
    GETADDR(device, VDP_FUNC_ID_GET_ERROR_STRING,
            (void**)&VDPAUDeviceFunctions.GetErrorString);
    GETADDR(device, VDP_FUNC_ID_GET_PROC_ADDRESS,
            (void**)&VDPAUDeviceFunctions.GetProcAddress);
    GETADDR(device, VDP_FUNC_ID_GET_API_VERSION,
            (void**)&VDPAUDeviceFunctions.GetApiVersion);
    GETADDR(device, VDP_FUNC_ID_GET_INFORMATION_STRING,
            (void**)&VDPAUDeviceFunctions.GetInformationString);
    GETADDR(device, VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.VideoSurfaceQueryCapabilities);
    GETADDR(device, VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.VideoSurfaceQueryGetPutBitsYCbCrCapabilities);
    GETADDR(device, VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.OutputSurfaceQueryCapabilities);
    GETADDR(device, VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.OutputSurfaceQueryGetPutBitsNativeCapabilities);
    GETADDR(device, VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.OutputSurfaceQueryPutBitsYCbCrCapabilities);
    GETADDR(device, VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.BitmapSurfaceQueryCapabilities);
    GETADDR(device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,
            (void**)&VDPAUDeviceFunctions.DecoderQueryCapabilities);
    GETADDR(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT,
            (void**)&VDPAUDeviceFunctions.VideoMixerQueryFeatureSupport);
    GETADDR(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,
            (void**)&VDPAUDeviceFunctions.VideoMixerQueryParameterSupport);
    GETADDR(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT,
            (void**)&VDPAUDeviceFunctions.VideoMixerQueryAttributeSupport);
    GETADDR(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE,
            (void**)&VDPAUDeviceFunctions.VideoMixerQueryParameterValueRange);
    GETADDR(device, VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE,
            (void**)&VDPAUDeviceFunctions.VideoMixerQueryAttributeValueRange);

}
#undef GETADDR



/*
 * queryBaseInfo() - Query basic VDPAU information
 */

static int queryBaseInfo(CtkVDPAU *ctk_vdpau, VdpDevice device,
                         VdpGetProcAddress *getProcAddress)
{
    static const Desc decoder_list[] = {
        {"MPEG1",              VDP_DECODER_PROFILE_MPEG1, 0x01},
        {"MPEG2",       VDP_DECODER_PROFILE_MPEG2_SIMPLE, 0x02},
        {"MPEG2",         VDP_DECODER_PROFILE_MPEG2_MAIN, 0x02},
        {"H264",       VDP_DECODER_PROFILE_H264_BASELINE, 0x04},
        {"H264",           VDP_DECODER_PROFILE_H264_MAIN, 0x04},
        {"H264",           VDP_DECODER_PROFILE_H264_HIGH, 0x04},
        {"VC1",           VDP_DECODER_PROFILE_VC1_SIMPLE, 0x08},
        {"VC1"  ,           VDP_DECODER_PROFILE_VC1_MAIN, 0x08},
        {"VC1",         VDP_DECODER_PROFILE_VC1_ADVANCED, 0x08},
        {"MPEG4",     VDP_DECODER_PROFILE_MPEG4_PART2_SP, 0x10},
        {"MPEG4",    VDP_DECODER_PROFILE_MPEG4_PART2_ASP, 0x10},
        {"DIVX4",      VDP_DECODER_PROFILE_DIVX4_QMOBILE, 0x20},
        {"DIVX4",       VDP_DECODER_PROFILE_DIVX4_MOBILE, 0x20},
        {"DIVX4", VDP_DECODER_PROFILE_DIVX4_HOME_THEATER, 0x20},
        {"DIVX4",     VDP_DECODER_PROFILE_DIVX4_HD_1080P, 0x20},
        {"DIVX5",      VDP_DECODER_PROFILE_DIVX5_QMOBILE, 0x40},
        {"DIVX5",       VDP_DECODER_PROFILE_DIVX5_MOBILE, 0x40},
        {"DIVX5", VDP_DECODER_PROFILE_DIVX5_HOME_THEATER, 0x40},
        {"DIVX5",     VDP_DECODER_PROFILE_DIVX5_HD_1080P, 0x40},
    };
    const size_t decoder_list_count = sizeof(decoder_list)/sizeof(Desc);

    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *event;
    uint32_t api;
    GtkWidget *eventbox;
    int x, count = 0;
    uint32_t decoder_mask = 0;

    if (VDPAUDeviceFunctions.GetApiVersion &&
        (VDPAUDeviceFunctions.GetApiVersion(&api) != VDP_STATUS_OK)) {
        return -1;
    }

    /* Add base information */

    vbox = gtk_vbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), vbox);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));

    gtk_notebook_append_page(GTK_NOTEBOOK(ctk_vdpau->notebook), eventbox,
                             gtk_label_new("Base Information"));

    hbox  = gtk_hbox_new(FALSE, 0);
    table = gtk_table_new(2, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 10);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    add_table_row_with_help_text(table, ctk_vdpau->ctk_config,
                                 __vdpau_api_version_help,
                                 0, 0,
                                 0, 0, "API version:",
                                 0, 0,  g_strdup_printf("%i", api));

    label = gtk_label_new("Supported Codecs:");
    event = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event), label);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL,
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL,
                         &(event->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, event,
                           __supported_codecs_help);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), event, 0, 1, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);


    for (x = 0; x < decoder_list_count; x++) {
        VdpBool is_supported = FALSE;
        VdpStatus ret;
        uint32_t max_level, max_macroblocks, max_width, max_height;

        ret = VDPAUDeviceFunctions.DecoderQueryCapabilities(device,
                                                            decoder_list[x].id,
                                                            &is_supported,
                                                            &max_level,
                                                            &max_macroblocks,
                                                            &max_width,
                                                            &max_height);
        if (ret == VDP_STATUS_OK && is_supported) {
            gchar *str;

            if (decoder_mask & decoder_list[x].aux) {
                continue;
            }
            gtk_table_resize(GTK_TABLE(table), 2+count, 2);
            str = g_strdup_printf("%s", decoder_list[x].name);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 0, 0);
            count++;
            decoder_mask |= decoder_list[x].aux;
        }
    }
    ctk_vdpau->baseInfoVbox = vbox;

    return 0;
} /* queryBaseInfo() */



/**************** Video surface ************/
static const Desc ycbcr_types[] = {
    {"NV12", VDP_YCBCR_FORMAT_NV12, 0},
    {"YV12", VDP_YCBCR_FORMAT_YV12, 0},
    {"UYVY", VDP_YCBCR_FORMAT_UYVY, 0},
    {"YUYV", VDP_YCBCR_FORMAT_YUYV, 0},
    {"Y8U8V8A8", VDP_YCBCR_FORMAT_Y8U8V8A8, 0},
    {"V8U8Y8A8", VDP_YCBCR_FORMAT_V8U8Y8A8, 0},
};
static const size_t ycbcr_type_count = sizeof(ycbcr_types)/sizeof(Desc);

static const Desc rgb_types[] = {
    {"B8G8R8A8", VDP_RGBA_FORMAT_B8G8R8A8, 0},
    {"R8G8B8A8", VDP_RGBA_FORMAT_R8G8B8A8, 0},
    {"R10G10B10A2", VDP_RGBA_FORMAT_R10G10B10A2, 0},
    {"B10G10R10A2", VDP_RGBA_FORMAT_B10G10R10A2, 0},
    {"A8", VDP_RGBA_FORMAT_A8, 0},
};
static const size_t rgb_type_count = sizeof(rgb_types)/sizeof(Desc);

/*
 * queryVideoSurface() - Query Video surface limits.
 *
 */

static int queryVideoSurface(CtkVDPAU *ctk_vdpau, VdpDevice device,
                             VdpGetProcAddress *getProcAddress)
{
    static const Desc chroma_types[] = {
        {"420", VDP_CHROMA_TYPE_420, 0},
        {"422", VDP_CHROMA_TYPE_422, 0},
        {"444", VDP_CHROMA_TYPE_444, 0},
    };

    const size_t chroma_type_count = sizeof(chroma_types)/sizeof(Desc);

    VdpStatus ret;
    int x;
    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *hseparator, *scrollWin;
    GtkWidget *eventbox, *event;
    GString *str1 = g_string_new("");
    int count = 0;

    if ((VDPAUDeviceFunctions.VideoSurfaceQueryGetPutBitsYCbCrCapabilities ==
         NULL) ||
        (VDPAUDeviceFunctions.VideoSurfaceQueryGetPutBitsYCbCrCapabilities ==
         NULL)) {
        return -1;
    }

    /* Add Video surface limits */

    vbox     = gtk_vbox_new(FALSE, 0);
    hbox     = gtk_hbox_new(FALSE, 0);
    label    = gtk_label_new("Video Surface:");
    eventbox = gtk_event_box_new();
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    event = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event), label);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL,
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL,
                         &(event->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, event,
                           __video_surface_help);
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), event, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(eventbox), hbox);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          eventbox);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);

    /* Add tab to notebook */
    gtk_notebook_append_page(GTK_NOTEBOOK(ctk_vdpau->notebook), scrollWin,
                             gtk_label_new("Surface Limits"));

    ctk_vdpau->surfaceVbox = vbox;

    /* Generate a new table */

    table = gtk_table_new(1, 4, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    label = gtk_label_new("Name");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Width");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Height");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Types");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* fill data to the table */

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    for (x = 0; x < chroma_type_count; x++) {
        VdpBool is_supported = FALSE;
        uint32_t max_width, max_height;

        ret = VDPAUDeviceFunctions.VideoSurfaceQueryCapabilities(device,
                                         chroma_types[x].id,
                                         &is_supported,
                                         &max_width, &max_height);
        if (ret == VDP_STATUS_OK && is_supported) {
            int y;
            gchar *str = NULL;


            gtk_table_resize(GTK_TABLE(table), count+2, 4);
            str = g_strdup_printf("%s", chroma_types[x].name);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_width);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_height);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 2, 3, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);


            /* Find out supported formats */
            str1 = g_string_erase (str1, 0, -1);
            for (y = 0; y < ycbcr_type_count; y++) {
                is_supported = FALSE;

                ret =
                    VDPAUDeviceFunctions.VideoSurfaceQueryGetPutBitsYCbCrCapabilities
                    (device, chroma_types[x].id, ycbcr_types[y].id,
                     &is_supported);
                if (ret == VDP_STATUS_OK && is_supported) {
                    const gchar* s = g_strdup_printf("%s ",
                                                     ycbcr_types[y].name);
                    str1 = g_string_append(str1, s);
                }
            }
            label = gtk_label_new(g_strdup_printf("%s", str1->str));
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 3, 4, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }
        count++;
    }
    g_string_free(str1, TRUE);

    queryOutputSurface(ctk_vdpau, device, getProcAddress);
    queryBitmapSurface(ctk_vdpau, device, getProcAddress);

    return 0;
} /* queryVideoSurface() */



/******************* Decoder ****************/

/*
 * queryDecoderCaps() - Query decoder capabilities.
 */

static int queryDecoderCaps(CtkVDPAU *ctk_vdpau, VdpDevice device,
                            VdpGetProcAddress *getProcAddress)
{
    static const Desc decoder_profiles[] = {
        {"MPEG1",              VDP_DECODER_PROFILE_MPEG1,              0},
        {"MPEG2 Simple",       VDP_DECODER_PROFILE_MPEG2_SIMPLE,       0},
        {"MPEG2 Main",         VDP_DECODER_PROFILE_MPEG2_MAIN,         0},
        {"H264 Baseline",      VDP_DECODER_PROFILE_H264_BASELINE,      0},
        {"H264 Main",          VDP_DECODER_PROFILE_H264_MAIN,          0},
        {"H264 High",          VDP_DECODER_PROFILE_H264_HIGH,          0},
        {"VC1 Simple",         VDP_DECODER_PROFILE_VC1_SIMPLE,         0},
        {"VC1 Main",           VDP_DECODER_PROFILE_VC1_MAIN,           0},
        {"VC1 Advanced",       VDP_DECODER_PROFILE_VC1_ADVANCED,       0},
        {"MPEG4 part 2 simple profile",
                               VDP_DECODER_PROFILE_MPEG4_PART2_SP,     0},
        {"MPEG4 part 2 advanced simple profile",
                               VDP_DECODER_PROFILE_MPEG4_PART2_ASP,    0},
        {"DIVX4 QMobile",      VDP_DECODER_PROFILE_DIVX4_QMOBILE,      0},
        {"DIVX4 Mobile",       VDP_DECODER_PROFILE_DIVX4_MOBILE,       0},
        {"DIVX4 Home Theater", VDP_DECODER_PROFILE_DIVX4_HOME_THEATER, 0},
        {"DIVX4 HD 1080P",     VDP_DECODER_PROFILE_DIVX4_HD_1080P,     0},
        {"DIVX5 QMobile",      VDP_DECODER_PROFILE_DIVX5_QMOBILE,      0},
        {"DIVX5 Mobile",       VDP_DECODER_PROFILE_DIVX5_MOBILE,       0},
        {"DIVX5 Home Theater", VDP_DECODER_PROFILE_DIVX5_HOME_THEATER, 0},
        {"DIVX5 HD 1080P",     VDP_DECODER_PROFILE_DIVX5_HD_1080P,     0},
    };
    const size_t decoder_profile_count = sizeof(decoder_profiles)/sizeof(Desc);

    VdpStatus ret;
    int x, count = 0;
    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *hseparator;
    GtkWidget *eventbox;

    if (VDPAUDeviceFunctions.DecoderQueryCapabilities == NULL) {
        return -1;
    }

    /* Add Decoder capabilities */

    vbox      = gtk_vbox_new(FALSE, 0);
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), vbox);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));

    /* Add tab to notebook */

    gtk_notebook_append_page(GTK_NOTEBOOK(ctk_vdpau->notebook), eventbox,
                             gtk_label_new("Decoder Limits"));

    /* Generate a new table */

    table = gtk_table_new(2, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    gtk_widget_modify_fg(table, GTK_STATE_NORMAL,
                         &(table->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(table, GTK_STATE_NORMAL,
                         &(table->style->base[GTK_STATE_NORMAL]));

    label = gtk_label_new("Name");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Level");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Macroblocks");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Width");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Height");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 4, 5, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* separator between heading and data */

    hseparator = gtk_hseparator_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 5, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* Enter the data values */

    for (x = 0; x < decoder_profile_count; x++) {
        VdpBool is_supported = FALSE;
        uint32_t max_level, max_macroblocks, max_width, max_height;

        ret = VDPAUDeviceFunctions.DecoderQueryCapabilities(device,
                                                decoder_profiles[x].id,
                                                &is_supported, &max_level,
                                                &max_macroblocks,
                                                &max_width, &max_height);
        if (ret == VDP_STATUS_OK && is_supported) {
            gchar *str = NULL;

            gtk_table_resize(GTK_TABLE(table), count+4, 5);
            str = g_strdup_printf("%s", decoder_profiles[x].name);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+3, count+4,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_level);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+3, count+4,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_macroblocks);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 2, 3, count+3, count+4,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_width);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 3, 4, count+3, count+4,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_height);
            label = gtk_label_new(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            g_free(str);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 4, 5, count+3, count+4,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }
        count++;
    }
    return 0;
} /* queryDecoderCaps() */



/*
 * queryOutputSurface() - Query Output surface information
 */

static int queryOutputSurface(CtkVDPAU *ctk_vdpau, VdpDevice device,
                              VdpGetProcAddress *getProcAddress)
{
    VdpStatus ret;
    int x, y, count = 0;
    GString *str1 = g_string_new("");
    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *hseparator;
    GtkWidget *eventbox;

    if ((VDPAUDeviceFunctions.OutputSurfaceQueryCapabilities == NULL) ||
        (VDPAUDeviceFunctions.OutputSurfaceQueryGetPutBitsNativeCapabilities
         == NULL)) {
        return -1;
    }

    /* Add Output surface information */

    vbox      = ctk_vdpau->surfaceVbox;
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("Output Surface:");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __ouput_surface_help);
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    /* Generate a new table */

    table = gtk_table_new(1, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    label = gtk_label_new("Name");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Width");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Height");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Native");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Types");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 4, 5, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* fill output surface data */

    for (x = 0; x < rgb_type_count; x++) {
        VdpBool is_supported, native=FALSE;
        uint32_t max_width, max_height;

        ret = VDPAUDeviceFunctions.OutputSurfaceQueryCapabilities(device,
                                                     rgb_types[x].id,
                                                     &is_supported, &max_width,
                                                     &max_height);
        VDPAUDeviceFunctions.OutputSurfaceQueryGetPutBitsNativeCapabilities
            (device, rgb_types[x].id, &native);
        if (ret == VDP_STATUS_OK && is_supported) {
            gchar *str = NULL;

            gtk_table_resize(GTK_TABLE(table), count+2, 5);
            str = g_strdup_printf("%s", rgb_types[x].name);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_width);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_height);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 2, 3, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%c", native?'y':'-');
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 3, 4, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str1 = g_string_erase (str1, 0, -1);

            /* Find out supported formats */

            for (y = 0; y < ycbcr_type_count; y++) {
                is_supported = FALSE;

                ret =
                    VDPAUDeviceFunctions.OutputSurfaceQueryPutBitsYCbCrCapabilities
                    (device, rgb_types[x].id, ycbcr_types[y].id,
                     &is_supported);
                if (ret == VDP_STATUS_OK && is_supported) {
                    gchar* s = g_strdup_printf("%s ", ycbcr_types[y].name);
                    str1 = g_string_append(str1, s);
                }
            }
            label = gtk_label_new(g_strdup_printf("%s", str1->str));
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 4, 5, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }
        count++;
    }
    return 0;
} /* queryOutputSurface() */



/*
 * queryBitmapSurface() - Query Bitmap surface limits
 */

static int queryBitmapSurface(CtkVDPAU *ctk_vdpau, VdpDevice device,
                              VdpGetProcAddress *getProcAddress)
{
    VdpStatus ret;
    int x, count = 0;
    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *hseparator;
    GtkWidget *eventbox;

    if (VDPAUDeviceFunctions.BitmapSurfaceQueryCapabilities == NULL) {
        return -1;
    }

    /* Add Bitmap surface information */

    vbox      = ctk_vdpau->surfaceVbox;
    hbox       = gtk_hbox_new(FALSE, 0);
    label      = gtk_label_new("Bitmap Surface:");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __bitmap_surface_help);
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), eventbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    /* Generate a new table */

    table = gtk_table_new(1, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    label = gtk_label_new("Name");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Width");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Height");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* fill the Bitmap surface data */

    for (x = 0; x < rgb_type_count; x++) {
        VdpBool is_supported;
        uint32_t max_width, max_height;

        ret = VDPAUDeviceFunctions.BitmapSurfaceQueryCapabilities(device,
                                                      rgb_types[x].id,
                                                      &is_supported,
                                                      &max_width,
                                                      &max_height);
        if (ret == VDP_STATUS_OK && is_supported) {
            gchar *str = NULL;

            gtk_table_resize(GTK_TABLE(table), count+2, 5);
            str = g_strdup_printf("%s", rgb_types[x].name);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_width);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

            str = g_strdup_printf("%i", max_height);
            label = gtk_label_new(str);
            g_free(str);
            gtk_label_set_selectable(GTK_LABEL(label), TRUE);
            gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
            gtk_table_attach(GTK_TABLE(table), label, 2, 3, count+1, count+2,
                             GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        }
        count++;
    }
    return 0;
} /* queryBitmapSurface() */



/******************* Video mixer ****************/

/* Type for value ranges */
enum DataType
{
    DT_NONE,
    DT_INT,
    DT_UINT,
    DT_FLOAT
};

/*
 * display_range() - Print the range
 */

static void display_range(GtkTable *table, gint x, uint32_t aux,
                          uint32_t minval, uint32_t maxval)
{
    gchar *str1 = NULL, *str2 = NULL;
    GtkWidget *label;

    switch(aux) {
    case DT_INT:
        {
            str1 = g_strdup_printf("%i", minval);
            str2 = g_strdup_printf("%i", maxval);
            break;
        }
    case DT_UINT:
        {
            str1 = g_strdup_printf("%u", minval);
            str2 = g_strdup_printf("%u", maxval);
            break;
        }
    case DT_FLOAT:
        {
            str1 = g_strdup_printf("%.2f",*((float*)&minval));
            str2 = g_strdup_printf("%.2f", *((float*)&maxval));
            break;
        }
    default: /* Ignore value which we don't know how to display */;
    }
    label = gtk_label_new(str1);
    g_free(str1);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, x+3, x+4,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(str2);
    g_free(str2);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, x+3, x+4,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
} /* display_range() */



/*
 * queryVideoMixer() - Query Video mixer information
 */

static int queryVideoMixer(CtkVDPAU *ctk_vdpau, VdpDevice device,
                           VdpGetProcAddress *getProcAddress)
{
    static const Desc mixer_features[] = {
        {"DEINTERLACE_TEMPORAL",
         VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,    0},
        {"DEINTERLACE_TEMPORAL_SPATIAL",
         VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL, 0},
        {"INVERSE_TELECINE",
         VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE,        0},
        {"NOISE_REDUCTION",
         VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,         0},
        {"SHARPNESS",
         VDP_VIDEO_MIXER_FEATURE_SHARPNESS,               0},
        {"LUMA_KEY",
         VDP_VIDEO_MIXER_FEATURE_LUMA_KEY,                0},
        {"HIGH QUALITY SCALING - L1",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1, 0},
        {"HIGH QUALITY SCALING - L2",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2, 0},
        {"HIGH QUALITY SCALING - L3",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3, 0},
        {"HIGH QUALITY SCALING - L4",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4, 0},
        {"HIGH QUALITY SCALING - L5",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5, 0},
        {"HIGH QUALITY SCALING - L6",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6, 0},
        {"HIGH QUALITY SCALING - L7",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7, 0},
        {"HIGH QUALITY SCALING - L8",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8, 0},
        {"HIGH QUALITY SCALING - L9",
         VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9, 0},
    };
    static const size_t mixer_features_count =
        sizeof(mixer_features)/sizeof(Desc);

    static const Desc mixer_parameters[] = {
        {"VIDEO_SURFACE_WIDTH",
         VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,DT_UINT},
        {"VIDEO_SURFACE_HEIGHT",
         VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,DT_UINT},
        {"CHROMA_TYPE",VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE,DT_NONE},
        {"LAYERS",VDP_VIDEO_MIXER_PARAMETER_LAYERS,DT_UINT},
    };
    static const size_t mixer_parameters_count =
        sizeof(mixer_parameters)/sizeof(Desc);

    static const Desc mixer_attributes[] = {
        {"BACKGROUND_COLOR",
         VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR,DT_NONE},
        {"CSC_MATRIX",
         VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX,DT_NONE},
        {"NOISE_REDUCTION_LEVEL",
         VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL,DT_FLOAT},
        {"SHARPNESS_LEVEL",
         VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL,DT_FLOAT},
        {"LUMA_KEY_MIN_LUMA",
         VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA,DT_NONE},
        {"LUMA_KEY_MAX_LUMA",
         VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA,DT_NONE},
    };
    static const size_t mixer_attributes_count =
        sizeof(mixer_attributes)/sizeof(Desc);

    VdpStatus ret;
    int x, count = 0;
    GtkWidget *vbox, *hbox;
    GtkWidget *table;
    GtkWidget *label, *hseparator;
    GtkWidget *eventbox;
    GtkWidget *scrollWin, *event;

    if (VDPAUDeviceFunctions.VideoMixerQueryFeatureSupport == NULL) {
        return -1;
    }

    /* Add Video mixer information */

    vbox      = gtk_vbox_new(FALSE, 0);
    label      = gtk_label_new("Video Mixer:");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __video_mixer_help);

    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox = gtk_hbox_new(FALSE, 0);
    event = gtk_event_box_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL,
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL,
                         &(event->style->base[GTK_STATE_NORMAL]));
    gtk_container_add(GTK_CONTAINER(event), hbox);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          event);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    gtk_widget_set_size_request(scrollWin, -1, 50);

    /* Add tab to notebook */

    gtk_notebook_append_page(GTK_NOTEBOOK(ctk_vdpau->notebook), scrollWin,
                             gtk_label_new("Video Mixer"));

    /* Generate a new table */

    table = gtk_table_new(2, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_widget_modify_fg(table, GTK_STATE_NORMAL,
                         &(table->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(table, GTK_STATE_NORMAL,
                         &(table->style->base[GTK_STATE_NORMAL]));

    label = gtk_label_new("Feature Name");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), eventbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __video_mixer_feature_help);

    label = gtk_label_new("Supported");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __video_mixer_attribute_help);

    /* separator between heading and data */

    hseparator = gtk_hseparator_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 5, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* fill Mixer feature data */

    for (x = 0; x < mixer_features_count; x++) {
        gchar *str = NULL;
        /* There seems to be a bug in VideoMixerQueryFeatureSupport,
         * is_supported is only set if the feature is not supported
         */
        VdpBool is_supported = TRUE;
        ret = VDPAUDeviceFunctions.VideoMixerQueryFeatureSupport
            (device, mixer_features[x].id, &is_supported);
        is_supported = (ret == VDP_STATUS_OK && is_supported);

        gtk_table_resize(GTK_TABLE(table), count+4, 5);
        str = g_strdup_printf("%s", mixer_features[x].name);
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        str = g_strdup_printf("%c", is_supported?'y':'-');
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
        count++;

    }

    if (VDPAUDeviceFunctions.VideoMixerQueryParameterSupport == NULL) {
        return -1;
    }

    /* Generate a new table */

    count = 0;
    table = gtk_table_new(2, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    label = gtk_label_new("Parameter Name");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), eventbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __video_mixer_parameter_help);

    label = gtk_label_new("Supported");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Min");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Max");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* separator between heading and data */

    hseparator = gtk_hseparator_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 5, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* fill the Mixer parameter data */

    for (x = 0; x < mixer_parameters_count; x++) {
        uint32_t minval, maxval;
        VdpBool is_supported = FALSE;
        gchar *str = NULL;

        ret = VDPAUDeviceFunctions.VideoMixerQueryParameterSupport
            (device, mixer_parameters[x].id, &is_supported);
        is_supported = (ret == VDP_STATUS_OK && is_supported);

        gtk_table_resize(GTK_TABLE(table), count+4, 5);
        str = g_strdup_printf("%s", mixer_parameters[x].name);
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        str = g_strdup_printf("%c", is_supported?'y':'-');
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        count++;
        if (is_supported && mixer_parameters[x].aux != DT_NONE) {
            ret = VDPAUDeviceFunctions.VideoMixerQueryParameterValueRange
                (device, mixer_parameters[x].id, (void*)&minval,
                 (void*)&maxval);
            display_range(GTK_TABLE(table), count-1,
                          mixer_parameters[x].aux,
                          minval, maxval);
        }
    }

    if (VDPAUDeviceFunctions.VideoMixerQueryAttributeSupport == NULL) {
        return -1;
    }

    /* Generate a new table */

    count = 0;
    table = gtk_table_new(2, 5, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    label = gtk_label_new("Attribute Name");
    eventbox = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(eventbox), label);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), eventbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_widget_modify_fg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(eventbox, GTK_STATE_NORMAL,
                         &(eventbox->style->base[GTK_STATE_NORMAL]));
    ctk_config_set_tooltip(ctk_vdpau->ctk_config, eventbox,
                           __video_mixer_attribute_help);

    label = gtk_label_new("Supported");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Min");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new("Max");
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_table_attach(GTK_TABLE(table), label, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* separator between heading and data */

    hseparator = gtk_hseparator_new();
    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 5, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    hbox  = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);

    /* fill the Attributes data */

    for (x = 0; x < mixer_attributes_count; x++) {
        VdpBool is_supported = FALSE;
        gchar *str = NULL;
        uint32_t minval, maxval;

        ret = VDPAUDeviceFunctions.VideoMixerQueryAttributeSupport
            (device, mixer_attributes[x].id, &is_supported);
        is_supported = (ret == VDP_STATUS_OK && is_supported);

        gtk_table_resize(GTK_TABLE(table), count+4, 5);
        str = g_strdup_printf("%s", mixer_attributes[x].name);
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 0, 1, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        str = g_strdup_printf("%c", is_supported?'y':'-');
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, count+3, count+4,
                         GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

        count++;
        if (is_supported && mixer_attributes[x].aux != DT_NONE) {
            ret = VDPAUDeviceFunctions.VideoMixerQueryAttributeValueRange
                (device, mixer_attributes[x].id,
                 (void*)&minval, (void*)&maxval);
            display_range(GTK_TABLE(table), count-1, mixer_attributes[x].aux,
                          minval, maxval);
        }
    }
    return 0;
} /* queryVideoMixer() */



GType ctk_vdpau_get_type(void)
{
    static GType ctk_vdpau_type = 0;

    if (!ctk_vdpau_type) {
        static const GTypeInfo ctk_vdpau_info = {
            sizeof (CtkVDPAUClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkVDPAU),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_vdpau_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkVDPAU",
                                   &ctk_vdpau_info, 0);
    }

    return ctk_vdpau_type;

} /* ctk_vdpau_get_type() */



GtkWidget* ctk_vdpau_new(NvCtrlAttributeHandle *handle,
                         CtkConfig *ctk_config,
                         CtkEvent *ctk_event)
{
    GObject *object;
    CtkVDPAU *ctk_vdpau;
    GtkWidget *banner;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *vbox3;
    GtkWidget *notebook;
    GtkWidget *scrollWin;
    GtkWidget *event;    /* For setting the background color to white */

    void *vdpau_handle = NULL;
    VdpDevice device;
    VdpGetProcAddress *getProcAddress = NULL;
    VdpStatus ret;
    VdpDeviceCreateX11 *VDPAUDeviceCreateX11 = NULL;

    /* make sure we have a handle */

    g_return_val_if_fail(handle != NULL, NULL);

    /* Create the ctk vdpau object */
    object = g_object_new(CTK_TYPE_VDPAU, NULL);
    ctk_vdpau = CTK_VDPAU(object);


    /* Cache the attribute handle */
    ctk_vdpau->handle = handle;

    /* Set container properties of the object */
    ctk_vdpau->ctk_config = ctk_config;
    gtk_box_set_spacing(GTK_BOX(ctk_vdpau), 10);

    /* Image banner */
    banner = ctk_banner_image_new(BANNER_ARTWORK_VDPAU);
    gtk_box_pack_start(GTK_BOX(ctk_vdpau), banner, FALSE, FALSE, 0);

    /* open VDPAU library */
    vdpau_handle = dlopen("libvdpau.so.1", RTLD_NOW);
    if (!vdpau_handle) {
        goto fail;
    }

    VDPAUDeviceCreateX11 = dlsym(vdpau_handle, "vdp_device_create_x11");
    if (!VDPAUDeviceCreateX11) {
        goto fail;
    }


    /* get device and ProcAddress */
    ret = VDPAUDeviceCreateX11(NvCtrlGetDisplayPtr(handle),
                               NvCtrlGetScreen(handle),
                               &device, &getProcAddress);

    if ((ret != VDP_STATUS_OK) || !device || !getProcAddress) {
        goto fail;
    }

    getAddressVDPAUDeviceFunctions(device, getProcAddress);

    /* Return early if any function is NULL */
    if (VDPAUDeviceFunctions.GetErrorString == NULL &&
        VDPAUDeviceFunctions.GetProcAddress == NULL &&
        VDPAUDeviceFunctions.GetApiVersion == NULL &&
        VDPAUDeviceFunctions.GetInformationString == NULL &&
        VDPAUDeviceFunctions.VideoSurfaceQueryCapabilities == NULL &&
        VDPAUDeviceFunctions.VideoSurfaceQueryGetPutBitsYCbCrCapabilities ==
        NULL &&
        VDPAUDeviceFunctions.OutputSurfaceQueryCapabilities == NULL &&
        VDPAUDeviceFunctions.OutputSurfaceQueryGetPutBitsNativeCapabilities ==
        NULL &&
        VDPAUDeviceFunctions.OutputSurfaceQueryPutBitsYCbCrCapabilities ==
        NULL &&
        VDPAUDeviceFunctions.BitmapSurfaceQueryCapabilities == NULL &&
        VDPAUDeviceFunctions.DecoderQueryCapabilities == NULL &&
        VDPAUDeviceFunctions.VideoMixerQueryFeatureSupport == NULL &&
        VDPAUDeviceFunctions.VideoMixerQueryParameterSupport == NULL &&
        VDPAUDeviceFunctions.VideoMixerQueryAttributeSupport == NULL &&
        VDPAUDeviceFunctions.VideoMixerQueryParameterValueRange == NULL &&
        VDPAUDeviceFunctions.VideoMixerQueryAttributeValueRange == NULL) {
        goto fail;
    }

    /* Information Scroll Box */
    vbox3 = gtk_vbox_new(FALSE, 5);
    vbox = gtk_vbox_new(FALSE, 5);

    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox = gtk_hbox_new(FALSE, 0);
    event = gtk_event_box_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_fg(event, GTK_STATE_NORMAL,
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL,
                         &(event->style->base[GTK_STATE_NORMAL]));
    gtk_container_add(GTK_CONTAINER(event), hbox);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          event);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 5);
    gtk_widget_set_size_request(scrollWin, -1, 50);

    gtk_box_pack_start(GTK_BOX(vbox3), scrollWin, TRUE, TRUE, 5);

    /* Create tabbed notebook for widget */

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(ctk_vdpau), notebook, TRUE, TRUE, 0);

    /* Create first tab for device info */

    ctk_vdpau->notebook = notebook;

    /* Query and print VDPAU information */
    queryBaseInfo(ctk_vdpau, device, getProcAddress);
    queryVideoSurface(ctk_vdpau, device, getProcAddress);
    queryDecoderCaps(ctk_vdpau, device, getProcAddress);
    queryVideoMixer(ctk_vdpau, device, getProcAddress);

    gtk_widget_show_all(GTK_WIDGET(object));

    /* close the handle */
    if (vdpau_handle) {
        dlclose(vdpau_handle);
    }

    return GTK_WIDGET(object);

 fail:
    if (vdpau_handle) {
        dlclose(vdpau_handle);
    }

    return NULL;
}



GtkTextBuffer *ctk_vdpau_create_help(GtkTextTagTable *table,
                                     CtkVDPAU *ctk_vdpau)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "VDPAU Information Help");
    ctk_help_para(b, &i, "%s", __vdpau_information_label_help);

    ctk_help_heading(b, &i, "Base Information");
    ctk_help_para(b, &i, "%s", __base_information_help);

    ctk_help_heading(b, &i, "API Version");
    ctk_help_para(b, &i, "%s", __vdpau_api_version_help);

    ctk_help_heading(b, &i, "Supported Codecs");
    ctk_help_para(b, &i, "%s", __supported_codecs_help);

    ctk_help_heading(b, &i, "Surface Limits");
    ctk_help_para(b, &i, "%s", __surface_limits_help);

    ctk_help_heading(b, &i, "Video Surface");
    ctk_help_para(b, &i, "%s", __video_surface_help);

    ctk_help_heading(b, &i, "Output Surface");
    ctk_help_para(b, &i, "%s", __ouput_surface_help);

    ctk_help_heading(b, &i, "Bitmap Surface");
    ctk_help_para(b, &i, "%s", __bitmap_surface_help);

    ctk_help_heading(b, &i, "Decoder Limits");
    ctk_help_para(b, &i, "%s", __decoder_limits_help);

    ctk_help_heading(b, &i, "Video Mixer");
    ctk_help_para(b, &i, "%s", __video_mixer_help);

    ctk_help_term(b, &i, "Feature");
    ctk_help_para(b, &i, "%s", __video_mixer_feature_help);

    ctk_help_term(b, &i, "Parameter");
    ctk_help_para(b, &i, "%s", __video_mixer_parameter_help);

    ctk_help_term(b, &i, "Attribute");
    ctk_help_para(b, &i, "%s", __video_mixer_attribute_help);

    ctk_help_finish(b);
    return b;
}
