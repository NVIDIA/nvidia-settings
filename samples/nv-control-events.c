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

int main(void)
{
    Display *dpy;
    Bool ret;
    int event_base, error_base, screen;
    XEvent event;
    XNVCtrlAttributeChangedEvent *nvevent;

    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    screen = DefaultScreen(dpy);

    if (!XNVCTRLIsNvScreen(dpy, screen)) {
        fprintf(stderr, "The NV-CONTROL X not available on screen "
                "%d of '%s'.\n", screen, XDisplayName(NULL));
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

    /*
     * register to receive NV-CONTROL events: whenever any NV-CONTROL
     * attribute is changed by an NV-CONTROL client, any other client
     * can receive notification of the change.
     */

    ret = XNVCtrlSelectNotify(dpy, screen, ATTRIBUTE_CHANGED_EVENT, True);
    if (ret != True) {
        fprintf(stderr, "Unable to register to receive NV-CONTROL events "
                "on '%s'.\n", XDisplayName(NULL));
        return 1;
    }

    /*
     * Loop forever, processing events
     */

    while (True) {

        /* block for the next event */

        XNextEvent(dpy, &event);

        /* if this is not one of our events, then bail out of this iteration */

        if (event.type != (event_base + ATTRIBUTE_CHANGED_EVENT)) continue;
        
        /* cast the X event as an XNVCtrlAttributeChangedEvent */

        nvevent = (XNVCtrlAttributeChangedEvent *) &event;

        /* print out the event information */

        printf("received NV-CONTROL event [attribute: %d (%s)  value: %d]\n",
               nvevent->attribute,
               attr2str(nvevent->attribute),
               nvevent->value);
    }
    
    return 0;
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
    case NV_CTRL_USE_HOUSE_SYNC:                   return "use house sync"; break;
    default:
        return "Unknown";
    }
} /* attr2str() */
