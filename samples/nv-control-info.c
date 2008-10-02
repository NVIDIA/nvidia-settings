/*
 * Copyright (c) 2004 NVIDIA, Corporation
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
