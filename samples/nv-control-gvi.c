/*
 * Copyright (c) 2006-2007 NVIDIA, Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/*
 * nv-control-gvi.c - NV-CONTROL client that demonstrates how to
 * interact with the GVI capabilities on an X Server.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"


/*
 * Decode SDI input value returned.
 */
char *SyncTypeName(int value)
{
    switch (value) {
    case NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_HD:
        return "NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_HD";
        break;
    case NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_SD:
        return "NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_SD";
        break;
    case NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE:
        return "NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE";
        break;
    default:
        return "Invalid Value";
        break;
    }
}

/*
 * Decode provided signal format.
 */
char *VideoFormatName(int value)
{
    switch(value) {
    case NV_CTRL_GVO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC:
        return "NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL:
        return "NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296:
        return "NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274:
        return "NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048P_30_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048P_30_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048P_29_97_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048P_29_97_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048I_60_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048I_60_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048I_59_94_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048I_59_94_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048P_25_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048P_25_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048I_50_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048I_50_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048P_24_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048P_24_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048P_23_98_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048P_23_98_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048I_48_00_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048I_48_00_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_2048I_47_96_SMPTE372:
        return "NV_CTRL_GVO_VIDEO_FORMAT_2048I_47_96_SMPTE372";
        break;
    case NV_CTRL_GVO_VIDEO_FORMAT_NONE:
        return "NV_CTRL_GVO_VIDEO_FORMAT_NONE";
        break;
    default:
        return "Invalid Value";
        break;
    }
}



/*
 * do_help()
 *
 * Prints some help on how to use this app.
 *
 */
static void do_help(void)
{
    printf("usage:\n");
    printf("-q: query system GVI information.\n");
    printf("-c <TOPOLOGY>: configure default GVI system topology.\n");
    printf("-g #: Operate on specific GVI device.\n");
    printf("\n");

} /* do_help()*/



/*
 * do_query()
 *
 * Prints information for all GVI devices found on
 * the given X server.
 *
 */
static void do_query(Display *dpy, int use_gvi)
{
    Bool ret;
    int num_gvi;
    int gvi;
    int last_gvi;

    int value;
    int num_jacks;
    int jack;
    int max_channels_per_jack;
    int channel;

    char *pOut;


    /* Query the number of GVI devices on the server */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GVI,
                                  &num_gvi);
    if (!ret) {
        printf("Failed to query number of GVI devices!\n");
        return;
    }
    printf("Found %d GVI device(s) on server.\n", num_gvi);
    if ( !num_gvi ) {
        return;
    }

    if (use_gvi >= 0 && use_gvi < num_gvi) {
        /* Only display information about 1 GVI device/ */
        printf("Querying GVI device %d...\n", use_gvi);
        gvi = use_gvi;
        last_gvi = use_gvi;
    } else {
        /* Display all GVI devices */
        printf("Querying all GVI devices...\n");
        gvi = 0;
        last_gvi = num_gvi-1;
    }

    /* Display information about the GVI(s) */

    for (; gvi <= last_gvi; gvi++) {
        
        printf("\n");
        printf("- GVI Board %d :\n", gvi);

        ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                NV_CTRL_TARGET_TYPE_GVI,
                                                gvi, // target_id
                                                0, // display_mask
                                                NV_CTRL_STRING_GVIO_FIRMWARE_VERSION,
                                                &pOut);
        if (!ret) {
            printf("  - Failed to query firmware version of GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Firmware Version: %s\n", pOut);
        XFree(pOut);
        pOut = NULL;
        /*
        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,
                                          &value);
        if (!ret) {
            printf("  - Failed to query requested video format of "
                   "GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Requested video format: %d\n", value);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT,
                                          &value);
        if (!ret) {
            printf("  - Failed to query detected video format of "
                   "GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Detected video format: %d\n", value);
        */

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_GLOBAL_IDENTIFIER,
                                          &value);
        if (!ret) {
            printf("  - Failed to query global ID of GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Global ID: %d\n", value);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_NUM_CAPTURE_SURFACES,
                                          &value);
        if (!ret) {
            printf("  - Failed to number of capture surfaces of GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Number of capture surfaces: %d\n", value);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_MAX_STREAMS,
                                          &value);
        if (!ret) {
            printf("  - Failed to query max number of streams on "
                   "GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Max number of configurable streams: %d\n", value);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_MAX_LINKS_PER_STREAM,
                                          &value);
        if (!ret) {
            printf("  - Failed to query max number of links per stream on "
                   "GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Max number of links per stream: %d\n", value);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_NUM_JACKS,
                                          &num_jacks);
        if (!ret) {
            printf("  - Failed to query number of input jacks on GVI "
                   "%d.\n", gvi);
            continue;
        } 
        printf("  - Number of input jacks on device: %d\n", num_jacks);

        ret = XNVCTRLQueryTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GVI,
                                          gvi, // target_id
                                          0, // display_mask
                                          NV_CTRL_GVI_MAX_CHANNELS_PER_JACK,
                                          &max_channels_per_jack);
        if (!ret) {
            printf("  - Failed to query maximum number of channels per "
                   "jack on GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Maximum number of channels per jack on device: %d\n",
               max_channels_per_jack);

        /* Display per-jack/channel information */

        for (jack = 0; jack < num_jacks; jack++) {
            printf("    - Jack %d\n", jack);

            for (channel = 0; channel < max_channels_per_jack; channel++) {
                printf("      - Channel %d\n", channel);

                unsigned int link_definition = ((channel & 0xFFFF)<<16);
                link_definition |= (jack & 0xFFFF);

                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected video format "
                           "on jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected Video Format: %d (%s)\n", value,
                       VideoFormatName(value)
                       );
            
                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVI_DETECTED_CHANNEL_SMPTE352_IDENTIFIER,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected SMPTE352 "
                           "Identifier on jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected SMPTE352 Identifier: 0x%08x\n",
                       value);
                
                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVI_DETECTED_CHANNEL_BITS_PER_COMPONENT,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected bits per "
                           "component on jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected bits per component: ");
                switch (value) {
                case NV_CTRL_GVI_BITS_PER_COMPONENT_8:  printf("8");  break;
                case NV_CTRL_GVI_BITS_PER_COMPONENT_10: printf("10"); break;
                case NV_CTRL_GVI_BITS_PER_COMPONENT_12: printf("12"); break;
                case NV_CTRL_GVI_BITS_PER_COMPONENT_UNKNOWN: /* Fall Through */
                default:
                    printf("Unknown");
                    break;
                }
                printf("\n");
                
                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVI_DETECTED_CHANNEL_COMPONENT_SAMPLING,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected component "
                           "sampling on jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected component sampling: ");
                switch (value) {
                case NV_CTRL_GVI_COMPONENT_SAMPLING_4444: printf("4:4:4:4"); break;
                case NV_CTRL_GVI_COMPONENT_SAMPLING_4224: printf("4:2:2:4"); break;
                case NV_CTRL_GVI_COMPONENT_SAMPLING_444:  printf("4:4:4");   break;
                case NV_CTRL_GVI_COMPONENT_SAMPLING_422:  printf("4:2:2");   break;
                case NV_CTRL_GVI_COMPONENT_SAMPLING_420:  printf("4:2:0");   break;
                case NV_CTRL_GVI_COMPONENT_SAMPLING_UNKNOWN: /* Fall Through */
                default:
                    printf("Unknown");
                    break;
                }
                printf("\n");
                
                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVI_DETECTED_CHANNEL_COLOR_SPACE,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected color space on "
                           "jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected color space: ");
                switch (value) {
                case NV_CTRL_GVI_COLOR_SPACE_GBR:    printf("GBR");   break;
                case NV_CTRL_GVI_COLOR_SPACE_GBRA:   printf("GBRA");   break;
                case NV_CTRL_GVI_COLOR_SPACE_GBRD:   printf("GBRD");   break;
                case NV_CTRL_GVI_COLOR_SPACE_YCBCR:  printf("YCbCr"); break;
                case NV_CTRL_GVI_COLOR_SPACE_YCBCRA: printf("YCbCrA"); break;
                case NV_CTRL_GVI_COLOR_SPACE_YCBCRD: printf("YCbCrD"); break;
                case NV_CTRL_GVI_COLOR_SPACE_UNKNOWN: /* Fall Through */
                default:
                    printf("Unknown");
                    break;
                }
                printf("\n");
                
                ret = XNVCTRLQueryTargetAttribute(dpy,
                                                  NV_CTRL_TARGET_TYPE_GVI,
                                                  gvi, // target_id
                                                  link_definition, // display_mask
                                                  NV_CTRL_GVI_DETECTED_CHANNEL_LINK_ID,
                                                  &value);
                if (!ret) {
                    printf("        - Failed to query detected link ID on "
                           "jack %d, channel %d of GVI %d.\n",
                           jack, channel, gvi);
                    continue;
                } 
                printf("        - Detected Link ID: Link %d", value);
                if (value == NV_CTRL_GVI_LINK_ID_UNKNOWN) {
                    printf(" (Unknown)");
                } else if (value < 26) {
                    printf(" (Link%c)", (int)('A')+value);
                }
                printf("\n");

            } /* Done querying per-channel information */
        } /* Done querying per-jack information */


        /* Query stream (link to jack+channel) topology */
        ret = XNVCTRLStringOperation(dpy,
                                     NV_CTRL_TARGET_TYPE_GVI,
                                     gvi, // target_id
                                     0, // display_mask
                                     NV_CTRL_STRING_OPERATION_GVI_CONFIGURE_STREAMS,
                                     NULL, // pIn
                                     &pOut);
        if (!ret || !pOut) {
            printf("  - Failed to query stream topology configuration of "
                   "GVI %d.\n", gvi);
            continue;
        } 
        printf("  - Topology:\n");
        printf("\n      %s\n\n", pOut ? pOut : "No streams are configured.");
        XFree(pOut);
        pOut = NULL;

    } /* Done Querying information about GVI devices */

} /* do_query() */



void do_configure(Display *dpy, int use_gvi, char *pIn)
{
    Bool ret;
    char *pOut = NULL;


    if (use_gvi < 0) {
        use_gvi = 0;
    }
        
    printf("Configuring GVI device %d:\n\n", use_gvi);
    printf("Setting ");
    if (!pIn) {
        pIn = "stream=0, link0=jack0.0; "
              "stream=1, link0=jack1.0; "
              "stream=2, link0=jack2.0; "
              "stream=3, link0=jack3.0";
        printf("default");
    } else {
        printf("custom");
    }
    printf(" configuration:\n\n");
    printf("  \"%s\"\n\n", pIn);

    ret = XNVCTRLStringOperation(dpy,
                                 NV_CTRL_TARGET_TYPE_GVI,
                                 use_gvi, // target_id 
                                 0, // display_mask
                                 NV_CTRL_STRING_OPERATION_GVI_CONFIGURE_STREAMS,
                                 pIn,
                                 &pOut);
    if (!ret || !pOut) {
        printf("  - Failed to configure stream topology of GVI %d.\n",
               use_gvi);
        return;
    } 
    printf("Topology:\n\n");
    printf("  %s\n\n", pOut ? pOut : "No streams are configured.");
    XFree(pOut);
    pOut = NULL;
}



int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int major, minor;
    int use_gvi = -1;
    int i;
    char *topology_str = NULL;

    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    /* Query the NV-CONTROL version */

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        printf("The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }
    
    /* Print some information */

    printf("Using NV-CONTROL extension %d.%d on %s\n\n",
           major, minor, XDisplayName(NULL));

    /* See if user wants a specific GVI device */

    for (i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-g") == 0) && ((i+1) < argc)) {
            use_gvi = strtol(argv[i+1], NULL, 10);
        }
        if ((strcmp(argv[i], "-c") == 0) && ((i+1) < argc)) {
            topology_str = argv[i+1];
        }
    }
    
    /* Do what the user wants */

    ret = 0;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-q") == 0) {
            do_query(dpy, use_gvi);
            ret = 1;
            break;
        } else if (strcmp(argv[i], "-c") == 0) {
            do_configure(dpy, use_gvi, topology_str);
            ret = 1;
            break;
        }
    }
    if (!ret) {
        do_help();
    }

    return 0;
}
