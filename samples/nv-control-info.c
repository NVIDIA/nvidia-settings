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
 * nv-control-info.c - trivial sample NV-CONTROL client that
 * demonstrates how to determine if the NV-CONTROL extension is
 * present.
 */

#include <stdio.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

int main(void)
{
    Display *dpy;
    Bool ret;
    int event_base, error_base, major, minor, screens, i;
    char *str;
        
    /*
     * open a connection to the X server indicated by the DISPLAY
     * environment variable
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

    /*
     * query the major and minor extension version
     */

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }

    /*
     * print statistics thus far
     */

    printf("NV-CONTROL X extension present\n");
    printf("  version        : %d.%d\n", major, minor);
    printf("  event base     : %d\n", event_base);
    printf("  error base     : %d\n", error_base);
    
    /*
     * loop over each screen, and determine if each screen is
     * controlled by the NVIDIA X driver (and thus supports the
     * NV-CONTROL X extension); then, query the string attributes on
     * the screen.
     */

    screens = ScreenCount(dpy);
    for (i = 0; i < screens; i++) {
        if (XNVCTRLIsNvScreen(dpy, i)) {
            printf("Screen %d supports the NV-CONTROL X extension\n", i);

            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not curently used */
                                              NV_CTRL_STRING_PRODUCT_NAME,
                                              &str);
            if (ret) {
                printf("  GPU            : %s\n", str);
                XFree(str);
            }
            
            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not curently used */
                                              NV_CTRL_STRING_VBIOS_VERSION,
                                              &str);
            
            if (ret) {
                printf("  VideoBIOS      : %s\n", str);
                XFree(str);
            }

            ret = XNVCTRLQueryStringAttribute(dpy, i,
                                              0, /* XXX not curently used */
                                              NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                              &str);

            if (ret) {
                printf("  Driver version : %s\n", str);
                XFree(str);
            }
        }
    }
    
    /*
     * close the display connection
     */

    XCloseDisplay(dpy);

    return 0;
}
