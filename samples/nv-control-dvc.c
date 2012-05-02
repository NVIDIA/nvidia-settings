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
 * nv-control-dvc.c - trivial sample NV-CONTROL client that
 * demonstrates how to query and set integer attributes.
 *
 * The attribute NV_CTRL_DIGITAL_VIBRANCE ("Digital Vibrance Control")
 * is used as an example.  This attribute is interesting because it
 * can be controlled on a per-display device basis.
 *
 * Please see the section "DISPLAY DEVICES" in NV-CONTROL-API.txt for
 * an explanation of display devices.
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

#include "nv-control-screen.h"


/*
 * display_device_name() - return the display device name correspoding
 * to the display device mask.
 */

static char *display_device_name(int mask)
{
    switch (mask) {
    case (1 <<  0): return "CRT-0"; break;
    case (1 <<  1): return "CRT-1"; break;
    case (1 <<  2): return "CRT-2"; break;
    case (1 <<  3): return "CRT-3"; break;
    case (1 <<  4): return "CRT-4"; break;
    case (1 <<  5): return "CRT-5"; break;
    case (1 <<  6): return "CRT-6"; break;
    case (1 <<  7): return "CRT-7"; break;

    case (1 <<  8): return "TV-0"; break;
    case (1 <<  9): return "TV-1"; break;
    case (1 << 10): return "TV-2"; break;
    case (1 << 11): return "TV-3"; break;
    case (1 << 12): return "TV-4"; break;
    case (1 << 13): return "TV-5"; break;
    case (1 << 14): return "TV-6"; break;
    case (1 << 15): return "TV-7"; break;

    case (1 << 16): return "DFP-0"; break;
    case (1 << 17): return "DFP-1"; break;
    case (1 << 18): return "DFP-2"; break;
    case (1 << 19): return "DFP-3"; break;
    case (1 << 20): return "DFP-4"; break;
    case (1 << 21): return "DFP-5"; break;
    case (1 << 22): return "DFP-6"; break;
    case (1 << 23): return "DFP-7"; break;
    default: return "Unknown";
    }
} /* display_device_name() */



int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int screen, retval, setval = -1;
    int display_devices, mask;
    NVCTRLAttributeValidValuesRec valid_values;

    /*
     * If there is a commandline argument, interpret it as the value
     * to use to set DVC.
     */
    
    if (argc == 2) {
        setval = atoi(argv[1]);
    }


    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    screen = GetNvXScreen(dpy);


    /*
     * Get the bitmask of enabled display devices
     */

    ret = XNVCTRLQueryAttribute(dpy,
                                screen,
                                0,
                                NV_CTRL_ENABLED_DISPLAYS,
                                &display_devices);
    if (!ret) {
        fprintf(stderr, "Unable to determine enabled display devices for "
                "screen %d of '%s'\n", screen, XDisplayName(NULL));
        return 1;
    }
    
    
    /*
     * loop over each enabled display device
     */

    for (mask = 1; mask < (1<<24); mask <<= 1) {

        if (!(mask & display_devices)) continue;

        /*
         * Query the valid values for NV_CTRL_DIGITAL_VIBRANCE
         */

        ret = XNVCTRLQueryValidAttributeValues(dpy,
                                               screen,
                                               mask,
                                               NV_CTRL_DIGITAL_VIBRANCE,
                                               &valid_values);
        if (!ret) {
            fprintf(stderr, "Unable to query the valid values for "
                    "NV_CTRL_DIGITAL_VIBRANCE on display device %s of "
                    "screen %d of '%s'.\n",
                    display_device_name(mask),
                    screen, XDisplayName(NULL));
            return 1;
        }

        /* we assume that NV_CTRL_DIGITAL_VIBRANCE is a range type */
        
        if (valid_values.type != ATTRIBUTE_TYPE_RANGE) {
            fprintf(stderr, "NV_CTRL_DIGITAL_VIBRANCE is not of "
                    "type RANGE.\n");
            return 1;
        }

        /* print the range of valid values */

        printf("Valid values for NV_CTRL_DIGITAL_VIBRANCE: "
               "(%" PRId64 " - %" PRId64 ").\n",
               valid_values.u.range.min, valid_values.u.range.max);
    
        /*
         * if a value was specified on the commandline, set it;
         * otherwise, query the current value
         */
        
        if (setval != -1) {
        
            XNVCTRLSetAttribute(dpy,
                                screen,
                                mask,
                                NV_CTRL_DIGITAL_VIBRANCE,
                                setval);
            XFlush(dpy);

            printf("Set NV_CTRL_DIGITAL_VIBRANCE to %d on display device "
                   "%s of screen %d of '%s'.\n", setval,
                   display_device_name(mask),
                   screen, XDisplayName(NULL));
        } else {
        
            ret = XNVCTRLQueryAttribute(dpy,
                                        screen,
                                        mask,
                                        NV_CTRL_DIGITAL_VIBRANCE,
                                        &retval);

            printf("The current value of NV_CTRL_DIGITAL_VIBRANCE "
                   "is %d on display device %s of screen %d of '%s'.\n",
                   retval, display_device_name(mask),
                   screen, XDisplayName(NULL));
        }
    }
    
    return 0;
}
