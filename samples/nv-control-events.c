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

/*
 * nv-control-events.c - trivial sample NV-CONTROL client that
 * demonstrates how to handle NV-CONTROL events.
 */

#include <stdio.h>

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
    int num_screens, num_gpus, num_framelocks, i;
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
            printf("- The NV-CONTROL X not available on screen "
                   "%d of '%s'.\n", i, XDisplayName(NULL));
            continue;
        }
        
        ret = XNVCtrlSelectNotify(dpy, i, ATTRIBUTE_CHANGED_EVENT, True);
        if (ret != True) {
            printf("- Unable to register to receive NV-CONTROL events on '%s'."
                   "\n", XDisplayName(NULL));
            continue;
        }
        
        printf("+ Listening to ATTRIBUTE_CHANGE_EVENTs on screen %d.\n", i);
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
            printf("- The NV-CONTROL X not available on screen "
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
        
        printf("+ Listening to TARGET_ATTRIBUTE_CHANGE_EVENTs on X Screen "
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
            
            printf("received NV-CONTROL event [attribute: %d (%s)  "
                   "value: %d]\n",
                   nvevent->attribute,
                   attr2str(nvevent->attribute),
                   nvevent->value);

        /* Handle TARGET_ATTRIBUTE_CHANGED_EVENTS */
        } else if (event.type ==
                   (event_base + TARGET_ATTRIBUTE_CHANGED_EVENT)) {
            /* cast the X event as an XNVCtrlAttributeChangedEventTarget */
            
            nveventtarget = (XNVCtrlAttributeChangedEventTarget *) &event;
            
            /* print out the event information */
            
            printf("received NV-CONTROL target event [target: %d (%s)  "
                   "id: %d ] [attribute: %d (%s)  value: %d]\n",
                   nveventtarget->target_type,
                   target2str(nveventtarget->target_type),
                   nveventtarget->target_id,
                   nveventtarget->attribute,
                   attr2str(nveventtarget->attribute),
                   nveventtarget->value);
        }
    }

    return 0;
}



/*
 * target2str() - translate a target type into a string
 */
static const char *target2str(int n)
{
    switch (n) {
    case NV_CTRL_TARGET_TYPE_X_SCREEN:  return "X Screen"; break;
    case NV_CTRL_TARGET_TYPE_GPU:       return "GPU"; break;
    case NV_CTRL_TARGET_TYPE_FRAMELOCK: return "Frame Lock"; break;
    default:
        return "Unknown";
    }
}




/*
 * attr2str() - translate an attribute integer into a string
 */

static const char *attr2str(int n)
{
    switch (n) {
    case NV_CTRL_FLATPANEL_SCALING:                return "flatpanel scaling"; break;
    case NV_CTRL_FLATPANEL_DITHERING:              return "flatpanel dithering"; break;
    case NV_CTRL_DIGITAL_VIBRANCE:                 return "digital vibrance"; break;
    case NV_CTRL_SYNC_TO_VBLANK:                   return "sync to vblank"; break;
    case NV_CTRL_LOG_ANISO:                        return "log aniso"; break;
    case NV_CTRL_FSAA_MODE:                        return "fsaa mode"; break;
    case NV_CTRL_TEXTURE_SHARPEN:                  return "texture sharpen"; break;
    case NV_CTRL_EMULATE:                          return "OpenGL software emulation"; break;
    case NV_CTRL_FRAMELOCK_MASTER:                 return "frame lock master"; break;
    case NV_CTRL_FRAMELOCK_POLARITY:               return "frame lock sync edge"; break;
    case NV_CTRL_FRAMELOCK_SYNC_DELAY:             return "frame lock sync delay"; break;
    case NV_CTRL_FRAMELOCK_SYNC_INTERVAL:          return "frame lock sync interval"; break;
    case NV_CTRL_FRAMELOCK_SYNC:                   return "frame lock sync"; break;
    case NV_CTRL_FRAMELOCK_TEST_SIGNAL:            return "frame lock test signal"; break;
    case NV_CTRL_FRAMELOCK_VIDEO_MODE:             return "frame lock video mode"; break;
    case NV_CTRL_FORCE_GENERIC_CPU:                return "force generic cpu"; break;
    case NV_CTRL_OPENGL_AA_LINE_GAMMA:             return "opengl aa line gamma"; break;
    case NV_CTRL_FLIPPING_ALLOWED:                 return "flipping allowed"; break;
    case NV_CTRL_TEXTURE_CLAMPING:                 return "texture clamping"; break;
    case NV_CTRL_CURSOR_SHADOW:                    return "cursor shadow"; break;
    case NV_CTRL_CURSOR_SHADOW_ALPHA:              return "cursor shadow alpha"; break;
    case NV_CTRL_CURSOR_SHADOW_RED:                return "cursor shadow red"; break;
    case NV_CTRL_CURSOR_SHADOW_GREEN:              return "cursor shadow green"; break;
    case NV_CTRL_CURSOR_SHADOW_BLUE:               return "cursor shadow blue"; break;
    case NV_CTRL_CURSOR_SHADOW_X_OFFSET:           return "cursor shadow x offset"; break;
    case NV_CTRL_CURSOR_SHADOW_Y_OFFSET:           return "cursor shadow y offset"; break;
    case NV_CTRL_FSAA_APPLICATION_CONTROLLED:      return "fsaa application controlled"; break;
    case NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED: return "log aniso application controlled"; break;
    case NV_CTRL_IMAGE_SHARPENING:                 return "image sharpening"; break;
    case NV_CTRL_TV_OVERSCAN:                      return "tv overscan"; break;
    case NV_CTRL_TV_FLICKER_FILTER:                return "tv flicker filter"; break;
    case NV_CTRL_TV_BRIGHTNESS:                    return "tv brightness"; break;
    case NV_CTRL_TV_HUE:                           return "tv hue"; break;
    case NV_CTRL_TV_CONTRAST:                      return "tv contrast"; break;
    case NV_CTRL_TV_SATURATION:                    return "tv saturation"; break;
    case NV_CTRL_TV_RESET_SETTINGS:                return "tv reset settings"; break;
    case NV_CTRL_GPU_CORE_TEMPERATURE:             return "gpu core temperature"; break;
    case NV_CTRL_GPU_CORE_THRESHOLD:               return "gpu core threshold"; break;
    case NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD:       return "gpu default core threshold"; break;
    case NV_CTRL_GPU_MAX_CORE_THRESHOLD:           return "gpu max core_threshold"; break;
    case NV_CTRL_AMBIENT_TEMPERATURE:              return "ambient temperature"; break;
    case NV_CTRL_PBUFFER_SCANOUT_XID:              return "scanout pbuffer xid"; break;

        /* XXX GVO stuff */
        
    case NV_CTRL_GPU_OVERCLOCKING_STATE:           return "overclocking state"; break;
    case NV_CTRL_GPU_2D_CLOCK_FREQS:               return "gpu 2d clock frequencies"; break;
    case NV_CTRL_GPU_3D_CLOCK_FREQS:               return "gpu 3d clock frequencies"; break;
    case NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS:          return "gpu optimal clock frequencies"; break;
    case NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE: return "gpu optimal clock frequency detection state"; break;
        
        /* XXX DDCCI stuff */

    case NV_CTRL_USE_HOUSE_SYNC:                   return "use house sync"; break;
    case NV_CTRL_FORCE_STEREO:                     return "force stereo"; break;
    case NV_CTRL_IMAGE_SETTINGS:                   return "image settings"; break;
    case NV_CTRL_XINERAMA:                         return "xinerama"; break;
    case NV_CTRL_XINERAMA_STEREO:                  return "xinerama stereo"; break;
    case NV_CTRL_SHOW_SLI_HUD:                     return "show sli hud"; break;
    case NV_CTRL_XV_SYNC_TO_DISPLAY:               return "xv sync to display"; break;

        /* XXX More GVO stuff */

    case NV_CTRL_ASSOCIATED_DISPLAY_DEVICES:       return "associated_display_devices"; break;
    case NV_CTRL_FRAMELOCK_SLAVES:                 return "frame lock slaves"; break;
    case NV_CTRL_PROBE_DISPLAYS:                   return "probed displays"; break;

    default:
        return "Unknown";
    }
} /* attr2str() */
