/*
 * Copyright (c) 2004-2008 NVIDIA, Corporation
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
 * nv-control-events.c - trivial sample NV-CONTROL client that
 * demonstrates how to handle NV-CONTROL events.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

#define EVENT_TYPE_START TARGET_ATTRIBUTE_CHANGED_EVENT
#define EVENT_TYPE_END TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT


static const char *attr2str(int n);
static const char *target2str(int n);
static const char *targetTypeAndId2Str(int targetType, int targetId);

struct target_info {
    int type;
    int count;
    unsigned int *pIds; // If Non-NULL, is list of target ids.
};

static void print_usage(char **argv)
{
    printf("Usage:\n");
    printf("%s [-d <dpy>] [-a] [-c] [-b] [-s]\n", argv[0]);
    printf("\n");
    printf("-d <dpy>: X server display to connect to\n");
    printf("-a: Listen for attribute availability events\n");
    printf("-c: Listen for attribute changed events\n");
    printf("-b: Listen for binary attribute changed events\n");
    printf("-s: Listen for string attribute changed events\n");
    printf("\n");
    printf("By default (i.e., if none of -a, -c, -b, or -s are requested),\n"
           "all event types are enabled.\n");
}

int main(int argc, char **argv)
{
    Display *dpy;
    Bool ret;
    int event_base, error_base;
    int i, j, k;
    int sources = 0;
    struct target_info info[] = {
        { .type = NV_CTRL_TARGET_TYPE_X_SCREEN },
        { .type = NV_CTRL_TARGET_TYPE_GPU },
        { .type = NV_CTRL_TARGET_TYPE_DISPLAY },
        { .type = NV_CTRL_TARGET_TYPE_FRAMELOCK },
        { .type = NV_CTRL_TARGET_TYPE_VCSC },
        { .type = NV_CTRL_TARGET_TYPE_GVI },
        { .type = NV_CTRL_TARGET_TYPE_COOLER },
        { .type = NV_CTRL_TARGET_TYPE_THERMAL_SENSOR },
        { .type = NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER },
    };
    static const int num_target_types = sizeof(info) / sizeof(*info);

    int c;
    char *dpy_name = NULL;
    Bool anythingEnabled;

#define EVENT_TYPE_ENTRY(_x) [_x] = { False, #_x }

    struct {
        Bool enabled;
        char *description;
    } eventTypes[] = {
        EVENT_TYPE_ENTRY(TARGET_ATTRIBUTE_CHANGED_EVENT),
        EVENT_TYPE_ENTRY(TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT),
        EVENT_TYPE_ENTRY(TARGET_STRING_ATTRIBUTE_CHANGED_EVENT),
        EVENT_TYPE_ENTRY(TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT),
    };

    while ((c = getopt(argc, argv, "d:acbsh")) >= 0) {
        switch (c) {
        case 'd':
            dpy_name = optarg;
            break;
        case 'a':
            eventTypes[TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT].enabled = True;
            break;
        case 'c':
            eventTypes[TARGET_ATTRIBUTE_CHANGED_EVENT].enabled = True;
            break;
        case 'b':
            eventTypes[TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT].enabled = True;
            break;
        case 's':
            eventTypes[TARGET_STRING_ATTRIBUTE_CHANGED_EVENT].enabled = True;
            break;
        case '?':
            fprintf(stderr, "%s: Unknown argument '%c'\n", argv[0], optopt);
            /* fallthrough */
        case 'h':
            print_usage(argv);
            return 1;
        }
    }

    anythingEnabled = False;
    for (i = EVENT_TYPE_START; i <= EVENT_TYPE_END; i++) {
        if (eventTypes[i].enabled) {
            anythingEnabled = True;
            break;
        }
    }

    if (!anythingEnabled) {
        for (i = EVENT_TYPE_START; i <= EVENT_TYPE_END; i++) {
            eventTypes[i].enabled = True;
        }
    }

    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */

    dpy = XOpenDisplay(dpy_name);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(dpy_name));
        return 1;
    }
    
    /*
     * check if the NV-CONTROL X extension is present on this X server
     */
    
    ret = XNVCTRLQueryExtension(dpy, &event_base, &error_base);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(dpy_name));
        return 1;
    }

    /* Query target counts */
    for (i = 0; i < num_target_types; i++) {

        struct target_info *tinfo = &info[i];


        if (tinfo->type == NV_CTRL_TARGET_TYPE_DISPLAY) {
            ret = XNVCTRLQueryTargetBinaryData(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                               0, 0,
                                               NV_CTRL_BINARY_DATA_DISPLAY_TARGETS,
                                               (unsigned char **)&(tinfo->pIds),
                                               &(tinfo->count));
            if (ret != True) {
                fprintf(stderr, "Failed to query %s target count on '%s'.\n",
                        target2str(tinfo->type), XDisplayName(dpy_name));
                return 1;
            }
            tinfo->count = tinfo->pIds[0];
        } else {
            ret = XNVCTRLQueryTargetCount(dpy, tinfo->type, &tinfo->count);
            if (ret != True) {
                fprintf(stderr, "Failed to query %s target count on '%s'.\n",
                        target2str(tinfo->type), XDisplayName(dpy_name));
                return 1;
            }
        }
    }

    printf("Registering to receive events...\n");
    fflush(stdout);

    /* Register to receive events on all targets */

    for (i = 0; i < num_target_types; i++) {
        struct target_info *tinfo = &info[i];

        for (j = 0; j < tinfo->count; j++) {
            int target_id;

            if (tinfo->pIds) {
                target_id = tinfo->pIds[1+j];
            } else {
                target_id = j;
            }

            for (k = EVENT_TYPE_START; k <= EVENT_TYPE_END; k++) {
                if (!eventTypes[k].enabled) {
                    continue;
                }

                if ((k == TARGET_ATTRIBUTE_CHANGED_EVENT) &&
                    (tinfo->type == NV_CTRL_TARGET_TYPE_X_SCREEN)) {

                    /*
                     * Only register to receive events if this screen is
                     * controlled by the NVIDIA driver.
                     */
                    if (!XNVCTRLIsNvScreen(dpy, target_id)) {
                        printf("- The NV-CONTROL X not available on X screen "
                               "%d of '%s'.\n", i, XDisplayName(dpy_name));
                        continue;
                    }

                    /*
                     * - Register to receive ATTRIBUTE_CHANGE_EVENT events.
                     *   These events are specific to attributes set on X
                     *   Screens.
                     */


                    ret = XNVCtrlSelectNotify(dpy, target_id, ATTRIBUTE_CHANGED_EVENT,
                                              True);
                    if (ret != True) {
                        printf("- Unable to register to receive NV-CONTROL"
                               "events on '%s'.\n", XDisplayName(dpy_name));
                        continue;
                    }

                    printf("+ Listening on X screen %d for "
                           "ATTRIBUTE_CHANGED_EVENTs.\n", target_id);
                    sources++;
                }

                /*
                 * - Register to receive TARGET_ATTRIBUTE_CHANGED_EVENT events.
                 *   These events are specific to attributes set on various
                 *   devices and structures controlled by the NVIDIA driver.
                 *   Some possible targets include X Screens, GPUs, and Frame
                 *   Lock boards.
                 */

                ret = XNVCtrlSelectTargetNotify(dpy,
                                                tinfo->type, /* target type */
                                                target_id,   /* target ID */
                                                k,           /* eventType */
                                                True);
                if (ret != True) {
                    printf("- Unable to register on %s %d for %ss.\n",
                           target2str(tinfo->type), target_id,
                           eventTypes[k].description);
                    continue;
                }

                printf("+ Listening on %s %d for %ss.\n",
                       target2str(tinfo->type), target_id, eventTypes[k].description);

                sources++;
            }
        }
    }

    /* 
     * Report the number of sources (things that we have registered to
     * listen for NV-CONTROL X Events on.)
     */

    printf("\n");
    printf("Listening on %d sources for NV-CONTROL X Events...\n", sources);
    

    /*
     * Loop forever, processing events
     */

    while (True) {
        XEvent event;
        const char *target_str;

        /* block for the next event */

        XNextEvent(dpy, &event);

        /* Handle ATTRIBUTE_CHANGED_EVENTS */
        if (event.type == (event_base + ATTRIBUTE_CHANGED_EVENT)) {

            /* cast the X event as an XNVCtrlAttributeChangedEvent */
            XNVCtrlAttributeChangedEvent *nvevent =
                (XNVCtrlAttributeChangedEvent *) &event;

            target_str = targetTypeAndId2Str(NV_CTRL_TARGET_TYPE_X_SCREEN,
                                             nvevent->screen);

            /* print out the event information */
            printf("ATTRIBUTE_CHANGED_EVENTS:                    Target: %15s  "
                   "Display Mask: 0x%08x   "
                   "Attribute: (%3d) %-32s   Value: %d (0x%08x)\n",
                   target_str,
                   nvevent->display_mask,
                   nvevent->attribute,
                   attr2str(nvevent->attribute),
                   nvevent->value,
                   nvevent->value
                   );

        /* Handle TARGET_ATTRIBUTE_CHANGED_EVENTS */
        } else if (event.type ==
                   (event_base + TARGET_ATTRIBUTE_CHANGED_EVENT)) {

            /* cast the X event as an XNVCtrlAttributeChangedEventTarget */
            XNVCtrlAttributeChangedEventTarget *nveventtarget =
                (XNVCtrlAttributeChangedEventTarget *) &event;

            target_str = targetTypeAndId2Str(nveventtarget->target_type,
                                             nveventtarget->target_id);

            /* print out the event information */
            printf("TARGET_ATTRIBUTE_CHANGED_EVENT:              Target: %15s  "
                   "Display Mask: 0x%08x   "
                   "Attribute: (%3d) %-32s   Value: %d (0x%08x)\n",
                   target_str,
                   nveventtarget->display_mask,
                   nveventtarget->attribute,
                   attr2str(nveventtarget->attribute),
                   nveventtarget->value,
                   nveventtarget->value
                   );

        /* Handle TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENTS */
        } else if (event.type ==
                   (event_base + TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT)) {

            /* cast the X event as an XNVCtrlAttributeChangedEventTargetAvailability */
            XNVCtrlAttributeChangedEventTargetAvailability *nveventavail =
                (XNVCtrlAttributeChangedEventTargetAvailability *) &event;

            target_str = targetTypeAndId2Str(nveventavail->target_type,
                                             nveventavail->target_id);

            /* print out the event information */
            printf("TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT: Target: %15s  "
                   "Display Mask: 0x%08x   "
                   "Attribute: (%3d) %-32s   Available: %s\n",
                   target_str,
                   nveventavail->display_mask,
                   nveventavail->attribute,
                   attr2str(nveventavail->attribute),
                   nveventavail->availability ? "Yes" : "No"
                   );
        } else if (event.type ==
                   (event_base + TARGET_STRING_ATTRIBUTE_CHANGED_EVENT)) {

            XNVCtrlStringAttributeChangedEventTarget *nveventstring =
                (XNVCtrlStringAttributeChangedEventTarget*) &event;

            target_str = targetTypeAndId2Str(nveventstring->target_type,
                                             nveventstring->target_id);

            /* print out the event information */
            printf("TARGET_STRING_ATTRIBUTE_CHANGED_EVENT:       Target: %15s  "
                   "Display Mask: 0x%08x   "
                   "Attribute: %3d\n",
                   target_str,
                   nveventstring->display_mask,
                   nveventstring->attribute
                   );

        } else if (event.type ==
                   (event_base + TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT)) {

            XNVCtrlBinaryAttributeChangedEventTarget *nveventbinary =
                (XNVCtrlBinaryAttributeChangedEventTarget *) &event;

            target_str = targetTypeAndId2Str(nveventbinary->target_type,
                                             nveventbinary->target_id);

            /* print out the event information */
            printf("TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT:       Target: %15s  "
                   "Display Mask: 0x%08x   "
                   "Attribute: %3d\n",
                   target_str,
                   nveventbinary->display_mask,
                   nveventbinary->attribute
                   );

        } else {
            printf("ERROR: unrecognized event type %d\n", event.type);
        }
    }

    return 0;
}



/*
 * target2str() - translate a target type into a string
 */
static const char *target2str(int n)
{
    static char unknown[24];

    switch (n) {
    case NV_CTRL_TARGET_TYPE_X_SCREEN:
        return "X Screen";
    case NV_CTRL_TARGET_TYPE_GPU:
        return "GPU";
    case NV_CTRL_TARGET_TYPE_DISPLAY:
        return "Display";
    case NV_CTRL_TARGET_TYPE_FRAMELOCK:
        return "Frame Lock";
    case NV_CTRL_TARGET_TYPE_VCSC:
        return "VCS";
    case NV_CTRL_TARGET_TYPE_GVI:
        return "GVI";
    case NV_CTRL_TARGET_TYPE_COOLER:
        return "Cooler";
    case NV_CTRL_TARGET_TYPE_THERMAL_SENSOR:
        return "Thermal Sensor";
    case NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER:
        return "3D Vision Pro Transceiver";
    default:
        snprintf(unknown, 24, "Unknown (%d)", n);
        return unknown;
    }
}

static const char *targetTypeAndId2Str(int targetType, int targetId)
{
    static char tmp[256];

    snprintf(tmp, sizeof(tmp), "%s-%-3d", target2str(targetType), targetId);

    return tmp;
}


// Used to convert the NV-CONTROL #defines to human readable text.
#define MAKE_ENTRY(ATTRIBUTE) { ATTRIBUTE, #ATTRIBUTE, NULL }

typedef struct {
    int num;
    char *str;
    char *name;
} AttrEntry;

static AttrEntry attr_table[];

static const char *attr2str(int n)
{
    AttrEntry *entry;

    entry = attr_table;
    while (entry->str) {
        if (entry->num == n) {
            if (!entry->name) {
                int len;
                entry->name = strdup(entry->str + 8);
                for (len = 0; len < strlen(entry->name); len++) {
                    entry->name[len] = tolower(entry->name[len]);
                }
            }
            return entry->name;
        }
        entry++;
    }

    return NULL;
}

// Attribute -> String table, generated using:
//
// grep 'define.*\/\*' NVCtrl.h | sed 's/.*define \([^ ]*\).*/    MAKE_ENTRY(\1),/' > DATA | head DATA
//
static AttrEntry attr_table[] = {
    MAKE_ENTRY(NV_CTRL_FLATPANEL_SCALING),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_DITHERING),
    MAKE_ENTRY(NV_CTRL_DITHERING),
    MAKE_ENTRY(NV_CTRL_DIGITAL_VIBRANCE),
    MAKE_ENTRY(NV_CTRL_BUS_TYPE),
    MAKE_ENTRY(NV_CTRL_VIDEO_RAM),
    MAKE_ENTRY(NV_CTRL_IRQ),
    MAKE_ENTRY(NV_CTRL_OPERATING_SYSTEM),
    MAKE_ENTRY(NV_CTRL_SYNC_TO_VBLANK),
    MAKE_ENTRY(NV_CTRL_LOG_ANISO),
    MAKE_ENTRY(NV_CTRL_FSAA_MODE),
    MAKE_ENTRY(NV_CTRL_TEXTURE_SHARPEN),
    MAKE_ENTRY(NV_CTRL_UBB),
    MAKE_ENTRY(NV_CTRL_OVERLAY),
    MAKE_ENTRY(NV_CTRL_STEREO),
    MAKE_ENTRY(NV_CTRL_EMULATE),
    MAKE_ENTRY(NV_CTRL_TWINVIEW),
    MAKE_ENTRY(NV_CTRL_CONNECTED_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_ENABLED_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_MASTER),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_POLARITY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_DELAY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_INTERVAL),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_PORT0_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_PORT1_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_HOUSE_STATUS),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_READY),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_STEREO_SYNC),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_TEST_SIGNAL),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_ETHERNET_DETECTED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_VIDEO_MODE),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_RATE),
    MAKE_ENTRY(NV_CTRL_FORCE_GENERIC_CPU),
    MAKE_ENTRY(NV_CTRL_OPENGL_AA_LINE_GAMMA),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_TIMING),
    MAKE_ENTRY(NV_CTRL_FLIPPING_ALLOWED),
    MAKE_ENTRY(NV_CTRL_ARCHITECTURE),
    MAKE_ENTRY(NV_CTRL_TEXTURE_CLAMPING),
    MAKE_ENTRY(NV_CTRL_FSAA_APPLICATION_CONTROLLED),
    MAKE_ENTRY(NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED),
    MAKE_ENTRY(NV_CTRL_IMAGE_SHARPENING),
    MAKE_ENTRY(NV_CTRL_TV_OVERSCAN),
    MAKE_ENTRY(NV_CTRL_TV_FLICKER_FILTER),
    MAKE_ENTRY(NV_CTRL_TV_BRIGHTNESS),
    MAKE_ENTRY(NV_CTRL_TV_HUE),
    MAKE_ENTRY(NV_CTRL_TV_CONTRAST),
    MAKE_ENTRY(NV_CTRL_TV_SATURATION),
    MAKE_ENTRY(NV_CTRL_TV_RESET_SETTINGS),
    MAKE_ENTRY(NV_CTRL_GPU_CORE_TEMPERATURE),
    MAKE_ENTRY(NV_CTRL_GPU_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_GPU_MAX_CORE_THRESHOLD),
    MAKE_ENTRY(NV_CTRL_AMBIENT_TEMPERATURE),
    MAKE_ENTRY(NV_CTRL_PBUFFER_SCANOUT_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_PBUFFER_SCANOUT_XID),
    MAKE_ENTRY(NV_CTRL_GVO_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_MODE),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_SOURCE),
    MAKE_ENTRY(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVO_DATA_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE),
    MAKE_ENTRY(NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED),
    MAKE_ENTRY(NV_CTRL_GVO_VIDEO_OUTPUTS),
    MAKE_ENTRY(NV_CTRL_GVO_FIRMWARE_VERSION),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_DELAY_PIXELS),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_DELAY_LINES),
    MAKE_ENTRY(NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE),
    MAKE_ENTRY(NV_CTRL_GVO_GLX_LOCKED),
    MAKE_ENTRY(NV_CTRL_GVIO_VIDEO_FORMAT_WIDTH),
    MAKE_ENTRY(NV_CTRL_GVIO_VIDEO_FORMAT_HEIGHT),
    MAKE_ENTRY(NV_CTRL_GVIO_VIDEO_FORMAT_REFRESH_RATE),
    MAKE_ENTRY(NV_CTRL_GPU_OVERCLOCKING_STATE),
    MAKE_ENTRY(NV_CTRL_GPU_2D_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_3D_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION),
    MAKE_ENTRY(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_CHIP_LOCATION),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_LINK),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_SIGNAL),
    MAKE_ENTRY(NV_CTRL_USE_HOUSE_SYNC),
    MAKE_ENTRY(NV_CTRL_EDID_AVAILABLE),
    MAKE_ENTRY(NV_CTRL_FORCE_STEREO),
    MAKE_ENTRY(NV_CTRL_IMAGE_SETTINGS),
    MAKE_ENTRY(NV_CTRL_XINERAMA),
    MAKE_ENTRY(NV_CTRL_XINERAMA_STEREO),
    MAKE_ENTRY(NV_CTRL_BUS_RATE),
    MAKE_ENTRY(NV_CTRL_SHOW_SLI_VISUAL_INDICATOR),
    MAKE_ENTRY(NV_CTRL_XV_SYNC_TO_DISPLAY),
    MAKE_ENTRY(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT2),
    MAKE_ENTRY(NV_CTRL_GVO_OVERRIDE_HW_CSC),
    MAKE_ENTRY(NV_CTRL_GVO_CAPABILITIES),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_TERMINATION),
    MAKE_ENTRY(NV_CTRL_ASSOCIATED_DISPLAY_DEVICES),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SLAVES),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_MASTERABLE),
    MAKE_ENTRY(NV_CTRL_PROBE_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_REFRESH_RATE),
    MAKE_ENTRY(NV_CTRL_GVO_FLIP_QUEUE_SIZE),
    MAKE_ENTRY(NV_CTRL_CURRENT_SCANLINE),
    MAKE_ENTRY(NV_CTRL_INITIAL_PIXMAP_PLACEMENT),
    MAKE_ENTRY(NV_CTRL_PCI_BUS),
    MAKE_ENTRY(NV_CTRL_PCI_DEVICE),
    MAKE_ENTRY(NV_CTRL_PCI_FUNCTION),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_FPGA_REVISION),
    MAKE_ENTRY(NV_CTRL_MAX_SCREEN_WIDTH),
    MAKE_ENTRY(NV_CTRL_MAX_SCREEN_HEIGHT),
    MAKE_ENTRY(NV_CTRL_MAX_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_DYNAMIC_TWINVIEW),
    MAKE_ENTRY(NV_CTRL_MULTIGPU_DISPLAY_OWNER),
    MAKE_ENTRY(NV_CTRL_FSAA_APPLICATION_ENHANCED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_RATE_4),
    MAKE_ENTRY(NV_CTRL_GVO_LOCK_OWNER),
    MAKE_ENTRY(NV_CTRL_HWOVERLAY),
    MAKE_ENTRY(NV_CTRL_NUM_GPU_ERRORS_RECOVERED),
    MAKE_ENTRY(NV_CTRL_REFRESH_RATE_3),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_SOURCE),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_PERFORMANCE_MODE),
    MAKE_ENTRY(NV_CTRL_GLYPH_CACHE),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL),
    MAKE_ENTRY(NV_CTRL_GPU_ADAPTIVE_CLOCK_STATE),
    MAKE_ENTRY(NV_CTRL_GVO_OUTPUT_VIDEO_LOCKED),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_LOCK_STATUS),
    MAKE_ENTRY(NV_CTRL_GVO_ANC_TIME_CODE_GENERATION),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_ALPHA_KEY),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_LUMA_KEY_RANGE),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_CR_KEY_RANGE),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_CB_KEY_RANGE),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_NUM_KEY_RANGES),
    MAKE_ENTRY(NV_CTRL_SWITCH_TO_DISPLAYS),
    MAKE_ENTRY(NV_CTRL_NOTEBOOK_DISPLAY_CHANGE_LID_EVENT),
    MAKE_ENTRY(NV_CTRL_NOTEBOOK_INTERNAL_LCD),
    MAKE_ENTRY(NV_CTRL_DEPTH_30_ALLOWED),
    MAKE_ENTRY(NV_CTRL_MODE_SET_EVENT),
    MAKE_ENTRY(NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE),
    MAKE_ENTRY(NV_CTRL_VCSC_HIGH_PERF_MODE),
    MAKE_ENTRY(NV_CTRL_DISPLAYPORT_LINK_RATE),
    MAKE_ENTRY(NV_CTRL_STEREO_EYES_EXCHANGE),
    MAKE_ENTRY(NV_CTRL_NO_SCANOUT),
    MAKE_ENTRY(NV_CTRL_GVO_CSC_CHANGED_EVENT),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SLAVEABLE ),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_TO_DISPLAY),
    MAKE_ENTRY(NV_CTRL_X_SERVER_UNIQUE_ID),
    MAKE_ENTRY(NV_CTRL_PIXMAP_CACHE),
    MAKE_ENTRY(NV_CTRL_PIXMAP_CACHE_ROUNDING_SIZE_KB),
    MAKE_ENTRY(NV_CTRL_IS_GVO_DISPLAY),
    MAKE_ENTRY(NV_CTRL_PCI_ID),
    MAKE_ENTRY(NV_CTRL_GVO_FULL_RANGE_COLOR),
    MAKE_ENTRY(NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE),
    MAKE_ENTRY(NV_CTRL_GVO_ENABLE_RGB_DATA),
    MAKE_ENTRY(NV_CTRL_IMAGE_SHARPENING_DEFAULT),
    MAKE_ENTRY(NV_CTRL_PCI_DOMAIN),
    MAKE_ENTRY(NV_CTRL_GVI_NUM_JACKS),
    MAKE_ENTRY(NV_CTRL_GVI_MAX_LINKS_PER_STREAM),
    MAKE_ENTRY(NV_CTRL_GVI_DETECTED_CHANNEL_BITS_PER_COMPONENT),
    MAKE_ENTRY(NV_CTRL_GVI_REQUESTED_STREAM_BITS_PER_COMPONENT),
    MAKE_ENTRY(NV_CTRL_GVI_DETECTED_CHANNEL_COMPONENT_SAMPLING),
    MAKE_ENTRY(NV_CTRL_GVI_REQUESTED_STREAM_COMPONENT_SAMPLING),
    MAKE_ENTRY(NV_CTRL_GVI_REQUESTED_STREAM_CHROMA_EXPAND),
    MAKE_ENTRY(NV_CTRL_GVI_DETECTED_CHANNEL_COLOR_SPACE),
    MAKE_ENTRY(NV_CTRL_GVI_DETECTED_CHANNEL_LINK_ID),
    MAKE_ENTRY(NV_CTRL_GVI_DETECTED_CHANNEL_SMPTE352_IDENTIFIER),
    MAKE_ENTRY(NV_CTRL_GVI_GLOBAL_IDENTIFIER),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_GPU_COOLER_MANUAL_CONTROL),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_LEVEL),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_CONTROL_TYPE),
    MAKE_ENTRY(NV_CTRL_THERMAL_COOLER_TARGET),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_STATUS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_CONFIGURATION_SUPPORTED),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_CONFIGURATION),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_DEFAULT_CONFIGURATION),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_SINGLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_DOUBLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_AGGREGATE_SINGLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_AGGREGATE_DOUBLE_BIT_ERRORS),
    MAKE_ENTRY(NV_CTRL_GPU_ECC_RESET_ERROR_STATUS),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_MIZER_MODE),
    MAKE_ENTRY(NV_CTRL_GVI_SYNC_OUTPUT_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVI_MAX_CHANNELS_PER_JACK),
    MAKE_ENTRY(NV_CTRL_GVI_MAX_STREAMS ),
    MAKE_ENTRY(NV_CTRL_GVI_NUM_CAPTURE_SURFACES),
    MAKE_ENTRY(NV_CTRL_OVERSCAN_COMPENSATION),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_GENERATION),
    MAKE_ENTRY(NV_CTRL_GVI_BOUND_GPU),
    MAKE_ENTRY(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT3),
    MAKE_ENTRY(NV_CTRL_ACCELERATE_TRAPEZOIDS),
    MAKE_ENTRY(NV_CTRL_GPU_CORES),
    MAKE_ENTRY(NV_CTRL_GPU_MEMORY_BUS_WIDTH),
    MAKE_ENTRY(NV_CTRL_GVI_TEST_MODE),
    MAKE_ENTRY(NV_CTRL_COLOR_SPACE),
    MAKE_ENTRY(NV_CTRL_COLOR_RANGE),
    MAKE_ENTRY(NV_CTRL_GPU_SCALING_DEFAULT_TARGET),
    MAKE_ENTRY(NV_CTRL_GPU_SCALING_DEFAULT_METHOD),
    MAKE_ENTRY(NV_CTRL_DITHERING_MODE),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING_MODE),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_READING),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_PROVIDER),
    MAKE_ENTRY(NV_CTRL_THERMAL_SENSOR_TARGET),
    MAKE_ENTRY(NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR),
    MAKE_ENTRY(NV_CTRL_GPU_CURRENT_PROCESSOR_CLOCK_FREQS),
    MAKE_ENTRY(NV_CTRL_GVIO_VIDEO_FORMAT_FLAGS),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_MAX_LINK_SPEED),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_RESET_TRANSCEIVER_TO_FACTORY_SETTINGS),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE),
    MAKE_ENTRY(NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES),
    MAKE_ENTRY(NV_CTRL_DITHERING_DEPTH),
    MAKE_ENTRY(NV_CTRL_CURRENT_DITHERING_DEPTH),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_FREQUENCY),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_COUNT),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_PAIR_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_DISCOVER_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_IDENTIFY_GLASSES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_SYNC_CYCLE),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_MISSED_SYNC_CYCLES),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL),
    MAKE_ENTRY(NV_CTRL_GVO_ANC_PARITY_COMPUTATION),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_PAIR_EVENT),
    MAKE_ENTRY(NV_CTRL_3D_VISION_PRO_GLASSES_UNPAIR_EVENT),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH),
    MAKE_ENTRY(NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED),
    MAKE_ENTRY(NV_CTRL_GVO_AUDIO_BLANKING),
    MAKE_ENTRY(NV_CTRL_CURRENT_METAMODE_ID),
    MAKE_ENTRY(NV_CTRL_DISPLAY_ENABLED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_INCOMING_HOUSE_SYNC_RATE),
    MAKE_ENTRY(NV_CTRL_FXAA),
    MAKE_ENTRY(NV_CTRL_DISPLAY_RANDR_OUTPUT_ID),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_DISPLAY_CONFIG),
    MAKE_ENTRY(NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY),
    MAKE_ENTRY(NV_CTRL_USED_DEDICATED_GPU_MEMORY),
    MAKE_ENTRY(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE),
    MAKE_ENTRY(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT),
    MAKE_ENTRY(NV_CTRL_DPY_HDMI_3D),
    MAKE_ENTRY(NV_CTRL_BASE_MOSAIC),
    MAKE_ENTRY(NV_CTRL_MULTIGPU_MASTER_POSSIBLE),
    MAKE_ENTRY(NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE),
    MAKE_ENTRY(NV_CTRL_XV_SYNC_TO_DISPLAY_ID),
    MAKE_ENTRY(NV_CTRL_PALETTE_UPDATE_EVENT),
    MAKE_ENTRY(NV_CTRL_GSYNC_ALLOWED),
    { -1, NULL, NULL }
};
