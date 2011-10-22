/*
 * Copyright (c) 2011 NVIDIA, Corporation
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
 * nv-control-3dvisionpro.c - Sample application that displays 
 * the details of the glasses currently attached to the transceiver
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#include "NVCtrlLib.h"

int main(int argc, char* argv[])
{
    Display *dpy;
    Bool ret;
    int i, target_id = 0, glass_id, num_of_glasses = 0;
    unsigned int *ptr;
    char *glass_name;
    int len;    

    if(argc > 2) {
        printf("Too many Arguments\n");
        return(0);
    }

    if(argc != 1) {
        target_id = atoi(argv[1]);
    }
    
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
     * query the ids of the glasses connected to the Transceiver 
     */    
    
    ret = XNVCTRLQueryTargetBinaryData(dpy, NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER, target_id,
                                       0, NV_CTRL_BINARY_DATA_GLASSES_PAIRED_TO_3D_VISION_PRO_TRANSCEIVER,
                                       (unsigned char**) &ptr, &len);
    if (ret) {
        num_of_glasses = ptr[0];
        printf("Total no. of glasses paired = %d\n", num_of_glasses);
        
        if (num_of_glasses > 0) {
            printf("\n");
            printf("%-20s", "GlassId");
            printf("%-s\n", "GlassName" );
        }
        
        for (i = 0; i < num_of_glasses; i++) {
            glass_id = ptr[i+1];
            ret = XNVCTRLQueryTargetStringAttribute (dpy, NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER, target_id,
                                                     glass_id, NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME, &glass_name);
            if (ret) {
                printf("%-20d", glass_id);
                printf("%-s\n", glass_name);
            }
            else {
                printf("Error retrieving GlassName for Glassid %d\n", glass_id);
            }
            XFree(glass_name);
        }
        XFree(ptr);
    }

    /*
     * close the display connection
     */

    XCloseDisplay(dpy);
    
    return 0;
}
