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


#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

static const char *attr2str(int n);
static const char *target2str(int n);

int main(void)
{
    Display *dpy;
    Bool ret;
    int event_base, error_base;
    int num_screens, num_gpus, num_framelocks, num_vcs, i;
    int sources;
    XEvent event;
    XNVCtrlAttributeChangedEvent *nvevent;
    XNVCtrlAttributeChangedEventTarget *nveventtarget;

    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    /*
     * check if the NV-CONTROL X extension is present on this X server
     */
    
    ret = XNVCTRLQueryExtension(dpy, &event_base, &error_base);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /* Query number of X Screens */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                  &num_screens);
    if (ret != True) {
        fprintf(stderr, "Failed to query the number of X Screens on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /* Query number of GPUs */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU,
                                  &num_gpus);
    if (ret != True) {
        fprintf(stderr, "Failed to query the number of GPUs on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /* Query number of Frame Lock (G-Sync) devices */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                  &num_framelocks);
    if (ret != True) {
        fprintf(stderr, "Failed to query the number of G-Sync devices on "
                "'%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /* Query number of VCS (Visual Computing System) devices */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_VCSC,
                                  &num_vcs);
    if (ret != True) {
        fprintf(stderr, "Failed to query the number of Visual Computing "
                "System devices on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /*
     * register to receive NV-CONTROL events: whenever any NV-CONTROL
     * attribute is changed by an NV-CONTROL client, any other client
     * can receive notification of the change.
     */

    sources = 0;

    /* 
     * - Register to receive ATTRIBUTE_CHANGE_EVENT events.  These events
     *   are specific to attributes set on X Screens.
     */

    printf("Registering to receive ATTRIBUTE_CHANGED_EVENT events...\n");
    fflush(stdout);

    for (i = 0; i < num_screens; i++ ) {

        /* Only register to receive events if this screen is controlled by
         * the NVIDIA driver.
         */
        if (!XNVCTRLIsNvScreen(dpy, i)) {
            printf("- The NV-CONTROL X not available on X screen "
                   "%d of '%s'.\n", i, XDisplayName(NULL));
            continue;
        }
        
        ret = XNVCtrlSelectNotify(dpy, i, ATTRIBUTE_CHANGED_EVENT, True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL events on '%s'."
                   "\n", XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to ATTRIBUTE_CHANGE_EVENTs on X screen %d.\n", i);
        sources++;
    }
    printf("\n");

    /* 
     * - Register to receive TARGET_ATTRIBUTE_CHANGED_EVENT events.  These
     *   events are specific to attributes set on various devices and
     *   structures controlled by the NVIDIA driver.  Some possible targets
     *   include X Screens, GPUs, and Frame Lock boards.
     */

    printf("Registering to receive TARGET_ATTRIBUTE_CHANGED_EVENT "
           "events...\n");
    fflush(stdout);

    /* Register to receive on all X Screen targets */

    for (i = 0; i < num_screens; i++ ) {

        /* Only register to receive events if this screen is controlled by
         * the NVIDIA driver.
         */
        if (!XNVCTRLIsNvScreen(dpy, i)) {
            printf("- The NV-CONTROL X not available on X screen "
                   "%d of '%s'.\n", i, XDisplayName(NULL));
            continue;
        }

        ret = XNVCtrlSelectTargetNotify(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                        i, TARGET_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL X Screen "
                   "target events for screen %d on '%s'.\n",
                   i, XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to TARGET_ATTRIBUTE_CHANGE_EVENTs on X screen "
               "%d.\n", i);
        sources++;
    }

    /* Register to receive on all GPU targets */

    for (i = 0; i < num_gpus; i++ ) {

        ret = XNVCtrlSelectTargetNotify(dpy, NV_CTRL_TARGET_TYPE_GPU,
                                        i, TARGET_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL GPU "
                   "target events for GPU %d on '%s'.\n",
                   i, XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to TARGET_ATTRIBUTE_CHANGE_EVENTs on GPU "
               "%d.\n", i);
        sources++;
    }

    /* Register to receive on all Frame Lock (G-Sync) targets */

    for (i = 0; i < num_framelocks; i++ ) {

        ret = XNVCtrlSelectTargetNotify(dpy, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                        i, TARGET_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL GPU "
                   "target events for Frame Lock %d on '%s'.\n",
                   i, XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to TARGET_ATTRIBUTE_CHANGE_EVENTs on Frame Lock "
               "%d.\n", i);
        sources++;
    }

    /* Register to receive on all VCS targets */

    for (i = 0; i < num_vcs; i++ ) {

        ret = XNVCtrlSelectTargetNotify(dpy, NV_CTRL_TARGET_TYPE_VCSC,
                                        i, TARGET_ATTRIBUTE_CHANGED_EVENT,
                                        True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL VCS "
                   "target events for VCS %d on '%s'.\n",
                   i, XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to TARGET_ATTRIBUTE_CHANGE_EVENTs on VCS "
               "%d.\n", i);
        sources++;
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
        char target_str[256];

        /* block for the next event */

        XNextEvent(dpy, &event);

        /* if this is not one of our events, then bail out of this iteration */

        if ((event.type != (event_base + ATTRIBUTE_CHANGED_EVENT)) &&
            (event.type != (event_base + TARGET_ATTRIBUTE_CHANGED_EVENT)))
            continue;
        
        /* Handle ATTRIBUTE_CHANGED_EVENTS */
        if (event.type == (event_base + ATTRIBUTE_CHANGED_EVENT)) {

            /* cast the X event as an XNVCtrlAttributeChangedEvent */
            
            nvevent = (XNVCtrlAttributeChangedEvent *) &event;
            
            /* print out the event information */
            snprintf(target_str, 256, "%s-%-3d", 
                     target2str(NV_CTRL_TARGET_TYPE_X_SCREEN),
                     nvevent->screen);

            printf("ATTRIBUTE_CHANGED_EVENTS:         Target: %15s   "
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
            
            nveventtarget = (XNVCtrlAttributeChangedEventTarget *) &event;
            
            /* print out the event information */
            snprintf(target_str, 256, "%s-%-3d",
                     target2str(nveventtarget->target_type),
                     nveventtarget->target_id);

            printf("TARGET_ATTRIBUTE_CHANGED_EVENTS:  Target: %15s   "
                   "Display Mask: 0x%08x   "
                   "Attribute: (%3d) %-32s   Value: %d (0x%08x)\n",
                   target_str,
                   nveventtarget->display_mask,
                   nveventtarget->attribute,
                   attr2str(nveventtarget->attribute),
                   nveventtarget->value,
                   nveventtarget->value
                   );
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
    case NV_CTRL_TARGET_TYPE_X_SCREEN:  return "X Screen"; break;
    case NV_CTRL_TARGET_TYPE_GPU:       return "GPU"; break;
    case NV_CTRL_TARGET_TYPE_FRAMELOCK: return "Frame Lock"; break;
    case NV_CTRL_TARGET_TYPE_VCSC:      return "VCS"; break;
    default:
        snprintf(unknown, 24, "Unknown (%d)", n);
        return unknown;
    }
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
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_ALPHA),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_RED),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_GREEN),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_BLUE),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_X_OFFSET),
    MAKE_ENTRY(NV_CTRL_CURSOR_SHADOW_Y_OFFSET),
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
    MAKE_ENTRY(NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVO_INPUT_VIDEO_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVO_DATA_FORMAT),
    MAKE_ENTRY(NV_CTRL_GVO_DISPLAY_X_SCREEN),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED),
    MAKE_ENTRY(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE),
    MAKE_ENTRY(NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED),
    MAKE_ENTRY(NV_CTRL_GVO_VIDEO_OUTPUTS),
    MAKE_ENTRY(NV_CTRL_GVO_FIRMWARE_VERSION),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_DELAY_PIXELS),
    MAKE_ENTRY(NV_CTRL_GVO_SYNC_DELAY_LINES),
    MAKE_ENTRY(NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE),
    MAKE_ENTRY(NV_CTRL_GVO_GLX_LOCKED),
    MAKE_ENTRY(NV_CTRL_GVO_VIDEO_FORMAT_WIDTH),
    MAKE_ENTRY(NV_CTRL_GVO_VIDEO_FORMAT_HEIGHT),
    MAKE_ENTRY(NV_CTRL_GVO_VIDEO_FORMAT_REFRESH_RATE),
    MAKE_ENTRY(NV_CTRL_GVO_X_SCREEN_PAN_X),
    MAKE_ENTRY(NV_CTRL_GVO_X_SCREEN_PAN_Y),
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
    MAKE_ENTRY(NV_CTRL_SHOW_SLI_HUD),
    MAKE_ENTRY(NV_CTRL_XV_SYNC_TO_DISPLAY),
    MAKE_ENTRY(NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT2),
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
    MAKE_ENTRY(NV_CTRL_GPU_SCALING),
    MAKE_ENTRY(NV_CTRL_FRONTEND_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_BACKEND_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_NATIVE_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION),
    MAKE_ENTRY(NV_CTRL_GPU_SCALING_ACTIVE),
    MAKE_ENTRY(NV_CTRL_DFP_SCALING_ACTIVE),
    MAKE_ENTRY(NV_CTRL_FSAA_APPLICATION_ENHANCED),
    MAKE_ENTRY(NV_CTRL_FRAMELOCK_SYNC_RATE_4),
    MAKE_ENTRY(NV_CTRL_GVO_LOCK_OWNER),
    MAKE_ENTRY(NV_CTRL_HWOVERLAY),
    MAKE_ENTRY(NV_CTRL_NUM_GPU_ERRORS_RECOVERED),
    MAKE_ENTRY(NV_CTRL_REFRESH_RATE_3),
    MAKE_ENTRY(NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS),
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
    MAKE_ENTRY(NV_CTRL_DISPLAYPORT_LINK_RATE),
    MAKE_ENTRY(NV_CTRL_STEREO_EYES_EXCHANGE),
    { -1, NULL, NULL }
};
