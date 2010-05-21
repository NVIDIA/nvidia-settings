/*
 * Copyright (c) 2006-2008 NVIDIA, Corporation
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
 * nv-control-dpy.c - This sample NV-CONTROL client demonstrates how
 * to configure display devices using NV-CONTROL.  This client
 * demonstrates many different pieces of display configuration
 * functionality, controlled through commandline options.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

#include "nv-control-screen.h"

static char *display_device_name(int mask);
static unsigned int display_device_mask(char *str);
static char *remove_whitespace(char *str);
static int count_bits(unsigned int mask);
static void parse_mode_string(char *modeString, char **modeName, int *mask);
static char *find_modeline(char *modeString, char *pModeLines,
                           int ModeLineLen);




int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int screen, display_devices, mask, major, minor, len, j;
    char *str, *start, *str0, *str1;
    char *displayDeviceNames[8];
    int nDisplayDevice;
    
    
    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display '%s'.\n\n", XDisplayName(NULL));
        return 1;
    }
    
    screen = GetNvXScreen(dpy);

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist "
                "on '%s'.\n\n", XDisplayName(NULL));
        return 1;
    }

    printf("\nUsing NV-CONTROL extension %d.%d on %s\n",
           major, minor, XDisplayName(NULL));
    
    
    /*
     * query the connected display devices on this X screen and print
     * basic information about each X screen
     */

    ret = XNVCTRLQueryAttribute(dpy, screen, 0,
                                NV_CTRL_CONNECTED_DISPLAYS, &display_devices);

    if (!ret) {
        fprintf(stderr, "Failed to query the enabled Display Devices.\n\n");
        return 1;
    }

    printf("Connected Display Devices:\n");
    
    nDisplayDevice = 0;
    for (mask = 1; mask < (1 << 24); mask <<= 1) {
        
        if (display_devices & mask) {
            
            XNVCTRLQueryStringAttribute(dpy, screen, mask,
                                        NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                        &str);
            
            displayDeviceNames[nDisplayDevice++] = str;
            
            printf("  %s (0x%08x): %s\n",
                   display_device_name(mask), mask, str);
        }
    }
    printf("\n");
    
    
    /*
     * perform the requested action, based on the specified
     * commandline option
     */
    
    if (argc <= 1) goto printHelp;


    /*
     * for each connected display device on this X screen, query the
     * list of modelines in the mode pool using
     * NV_CTRL_BINARY_DATA_MODELINES, then print the results
     */
    
    if (strcmp(argv[1], "--print-modelines") == 0) {

        nDisplayDevice = 0;
        
        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (!(display_devices & mask)) continue;
                
            ret = XNVCTRLQueryBinaryData(dpy, screen, mask,
                                         NV_CTRL_BINARY_DATA_MODELINES,
                                         (void *) &str, &len);
                
            if (!ret) {
                fprintf(stderr, "Failed to query ModeLines.\n\n");
                return 1;
            }

            /*
             * the returned data is in the form:
             *
             *  "ModeLine 1\0ModeLine 2\0ModeLine 3\0Last ModeLine\0\0"
             *
             * so walk from one "\0" to the next to print each ModeLine.
             */
            
            printf("Modelines for %s:\n",
                   displayDeviceNames[nDisplayDevice++]);
                
            start = str;
            for (j = 0; j < len; j++) {
                if (str[j] == '\0') {
                    printf("  %s\n", start);
                    start = &str[j+1];
                    }
            }
            
            XFree(str);
        }
    }
    

    /*
     * for each connected display device on this X screen, query the
     * current modeline using NV_CTRL_STRING_CURRENT_MODELINE
     */
    
    else if (strcmp(argv[1], "--print-current-modeline") == 0) {

        nDisplayDevice = 0;

        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (!(display_devices & mask)) continue;

            ret =
                XNVCTRLQueryStringAttribute(dpy, screen, mask,
                                            NV_CTRL_STRING_CURRENT_MODELINE,
                                            &str);
                
            if (!ret) {
                fprintf(stderr, "Failed to query current ModeLine.\n\n");
                return 1;
            }
                
            printf("Current Modeline for %s:\n",
                   displayDeviceNames[nDisplayDevice++]);
            printf("  %s\n\n", str);

            XFree(str);
        }
    }
    

    /*
     * add the specified modeline to the mode pool for the specified
     * display device, using NV_CTRL_STRING_ADD_MODELINE
     */
    
    else if ((strcmp(argv[1], "--add-modeline") == 0) &&
             argv[2] && argv[3]) {
        
        mask = strtol(argv[2], NULL, 0);

        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        mask,
                                        NV_CTRL_STRING_ADD_MODELINE,
                                        argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to add the modeline \"%s\" to %s's "
                    "mode pool.\n\n", argv[3], display_device_name(mask));
            return 1;
        }
        
        printf("Added modeline \"%s\" to %s's mode pool.\n\n",
               argv[3], display_device_name(mask));
    }

    
    /*
     * delete the specified modeline from the mode pool for the
     * specified display device, using NV_CTRL_STRING_DELETE_MODELINE
     */
    
    else if ((strcmp(argv[1], "--delete-modeline") == 0) &&
             argv[2] && argv[3]) {
        
        mask = strtol(argv[2], NULL, 0);
    
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        mask,
                                        NV_CTRL_STRING_DELETE_MODELINE,
                                        argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to delete the mode \"%s\" from %s's "
                    "mode pool.\n\n", argv[3], display_device_name(mask));
            return 1;
        }
        
        printf("Deleted modeline \"%s\" from %s's mode pool.\n\n",
               argv[3], display_device_name(mask));
    }
    
    
    /*
     * generate a GTF modeline using NV_CTRL_STRING_OPERATION_GTF_MODELINE
     */
    
    else if ((strcmp(argv[1], "--generate-gtf-modeline") == 0) &&
             argv[2] && argv[3] && argv[4]) {
        
        char pGtfString[128];
        char *pOut;
        
        snprintf(pGtfString, 128, "width=%s, height=%s, refreshrate=%s",
                 argv[2], argv[3], argv[4]);

        ret = XNVCTRLStringOperation(dpy,
                                     NV_CTRL_TARGET_TYPE_X_SCREEN,
                                     screen,
                                     0,
                                     NV_CTRL_STRING_OPERATION_GTF_MODELINE,
                                     pGtfString,
                                     &pOut);

        if (!ret) {
            fprintf(stderr, "Failed to generate GTF ModeLine from "
                    "\"%s\".\n\n", pGtfString);
            return 1;
        }
        
        printf("GTF ModeLine from \"%s\": %s\n\n", pGtfString, pOut);
    }
    
    
    /*
     * generate a CVT modeline using NV_CTRL_STRING_OPERATION_CVT_MODELINE
     */

    else if ((strcmp(argv[1], "--generate-cvt-modeline") == 0) &&
             argv[2] && argv[3] && argv[4] && argv[5]) {

        char pCvtString[128];
        char *pOut;
        
        snprintf(pCvtString, 128, "width=%s, height=%s, refreshrate=%s, "
                 "reduced-blanking=%s",
                 argv[2], argv[3], argv[4], argv[5]);
        
        ret = XNVCTRLStringOperation(dpy,
                                     NV_CTRL_TARGET_TYPE_X_SCREEN,
                                     screen,
                                     0,
                                     NV_CTRL_STRING_OPERATION_CVT_MODELINE,
                                     pCvtString,
                                     &pOut);

        if (!ret) {
            fprintf(stderr, "Failed to generate CVT ModeLine from "
                    "\"%s\".\n\n", pCvtString);
            return 1;
        }

        printf("CVT ModeLine from \"%s\": %s\n\n", pCvtString, pOut);
    }

    
    /*
     * query the MetaModes for the X screen, using
     * NV_CTRL_BINARY_DATA_METAMODES.
     */
    
    else if (strcmp(argv[1], "--print-metamodes") == 0) {

        /* get list of metamodes */
        
        ret = XNVCTRLQueryBinaryData(dpy, screen, 0, // n/a
                                     NV_CTRL_BINARY_DATA_METAMODES,
                                     (void *) &str, &len);
        
        if (!ret) {
            fprintf(stderr, "Failed to query MetaModes.\n\n");
            return 1;
        }
        
        /*
         * the returned data is in the form:
         *
         *   "MetaMode 1\0MetaMode 2\0MetaMode 3\0Last MetaMode\0\0"
         *
         * so walk from one "\0" to the next to print each MetaMode.
         */
        
        printf("MetaModes:\n");
        
        start = str;
        for (j = 0; j < len; j++) {
            if (str[j] == '\0') {
                printf("  %s\n", start);
                start = &str[j+1];
            }
        }
        
        XFree(str);
    }
    
    
    /*
     * query the currently in use MetaMode.  Note that an alternative
     * way to accomplish this is to use XRandR to query the current
     * mode's refresh rate, and then match the refresh rate to the id
     * reported in the returned NV_CTRL_BINARY_DATA_METAMODES data.
     */
    
    else if (strcmp(argv[1], "--print-current-metamode") == 0) {
        
        ret = XNVCTRLQueryStringAttribute(dpy, screen, mask,
                                          NV_CTRL_STRING_CURRENT_METAMODE,
                                          &str);
        
        if (!ret) {
            fprintf(stderr, "Failed to query the current MetaMode.\n\n");
            return 1;
        }
        
        printf("current metamode: \"%s\"\n\n", str);

        XFree(str);
    }
    
    
    /*
     * add the given MetaMode to X screen's list of MetaModes, using
     * NV_CTRL_STRING_OPERATION_ADD_METAMODE; example MetaMode string:
     *
     * "nvidia-auto-select, nvidia-auto-select"
     *
     * The output string will contain "id=#" which indicates the
     * unique identifier for this MetaMode.  You can then use XRandR
     * to switch to this mode by matching the identifier with the
     * refresh rate reported via XRandR.
     *
     * For example:
     *
     * $ ./nv-control-dpy --add-metamode \
     *                         "nvidia-auto-select, nvidia-auto-select"
     *
     * Using NV-CONTROL extension 1.12 on :0
     * Connected Display Devices:
     *   CRT-0 (0x00000001): EIZO F931
     *   CRT-1 (0x00000002): ViewSonic P815-4
     *
     * Added MetaMode "nvidia-auto-select, nvidia-auto-select"; 
     * pOut: "id=52"
     *
     * $ xrandr -q
     * SZ:    Pixels          Physical       Refresh
     *  0   3200 x 1200   ( 821mm x 302mm )   51   52  
     * *1   1600 x 600    ( 821mm x 302mm )  *50  
     * Current rotation - normal
     * Current reflection - none
     * Rotations possible - normal 
     * Reflections possible - none
     *
     * $ xrandr -s 0 -r 52
     */
    
    else if ((strcmp(argv[1], "--add-metamode") == 0) && (argv[2])) {
        
        char *pOut;
        
        ret = XNVCTRLStringOperation(dpy,
                                     NV_CTRL_TARGET_TYPE_X_SCREEN,
                                     screen,
                                     0,
                                     NV_CTRL_STRING_OPERATION_ADD_METAMODE,
                                     argv[2],
                                     &pOut);

        if (!ret) {
            fprintf(stderr, "Failed to add the MetaMode \"%s\".\n\n",
                    argv[2]);
            return 1;
        }

        printf("Added MetaMode \"%s\"; pOut: \"%s\"\n\n", argv[2], pOut);
        
        XFree(pOut);
    }
    
    
    /*
     * delete the given MetaMode from the X screen's list of
     * MetaModes, using NV_CTRL_STRING_DELETE_METAMODE
     */

    else if ((strcmp(argv[1], "--delete-metamode") == 0) && (argv[1])) {
        
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        0,
                                        NV_CTRL_STRING_DELETE_METAMODE,
                                        argv[2]);

        if (!ret) {
            fprintf(stderr, "Failed to delete the MetaMode.\n\n");
            return 1;
        }
        
        printf("Deleted MetaMode \"%s\".\n\n", argv[2]);
    }
    
    
    /*
     * query the valid frequency ranges for each display device, using
     * NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES and
     * NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES
     */
    
    else if (strcmp(argv[1], "--get-valid-freq-ranges") == 0) {
        
        nDisplayDevice = 0;

        for (mask = 1; mask < (1 << 24); mask <<= 1) {

            if (!(display_devices & mask)) continue;
            
            ret = XNVCTRLQueryStringAttribute
                (dpy, screen, mask,
                 NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES,
                 &str0);
                
            if (!ret) {
                fprintf(stderr, "Failed to query HorizSync for %s.\n\n",
                        display_device_name(mask));
                return 1;
            }
                    

            ret = XNVCTRLQueryStringAttribute
                (dpy, screen, mask,
                 NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES,
                 &str1);
                
            if (!ret) {
                fprintf(stderr, "Failed to query VertRefresh for %s.\n\n",
                        display_device_name(mask));
                XFree(str0);
                return 1;
            }
                
            printf("frequency information for %s:\n",
                   displayDeviceNames[nDisplayDevice++]);
            printf("  HorizSync   : \"%s\"\n", str0);
            printf("  VertRefresh : \"%s\"\n\n", str1);
            
            XFree(str0);
            XFree(str1);
        }
    }
    
    
    /*
     * attempt to build the modepool for each display device; this
     * will fail for any display device that already has a modepool
     */
    
    else if (strcmp(argv[1], "--build-modepool") == 0) {
        
        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (!(display_devices & mask)) continue;
            
            ret = XNVCTRLStringOperation
                (dpy,
                 NV_CTRL_TARGET_TYPE_X_SCREEN,
                 screen,
                 mask,
                 NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,
                 argv[2],
                 &str0);
            
            if (!ret) {
                fprintf(stderr, "Failed to build modepool for %s (it most "
                        "likely already has a modepool).\n\n",
                        display_device_name(mask));
            } else {
                printf("Built modepool for %s.\n\n",
                       display_device_name(mask));
            }
        }
    }

    
    /*
     * query the associated display devices on this X screen; these
     * are the display devices that are available to the X screen for
     * use by MetaModes.
     */
    
    else if (strcmp(argv[1], "--get-associated-dpys") == 0) {
        
        ret = XNVCTRLQueryAttribute(dpy,
                                    screen,
                                    0,
                                    NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
                                    &mask);
        
        if (ret) {
            printf("associated display device mask: 0x%08x\n\n", mask);
        } else {
            fprintf(stderr, "failed to query the associated display device "
                    "mask.\n\n");
            return 1;
        }
        
    }
    
    
    /*
     * assign the associated display device mask for this X screen;
     * these are the display devices that are available to the X
     * screen for use by MetaModes.
     */
    
    else if ((strcmp(argv[1], "--set-associated-dpys") == 0) && (argv[2])) {

        mask = strtol(argv[2], NULL, 0);

        ret = XNVCTRLSetAttributeAndGetStatus
            (dpy,
             screen,
             0,
             NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
             mask);
        
        if (ret) {
            printf("set the associated display device mask to 0x%08x\n\n",
                   mask);
        } else {
            fprintf(stderr, "failed to set the associated display device "
                    "mask to 0x%08x\n\n", mask);
            return 1;
        }
    }

    
    /*
     * query information about the GPUs in the system
     */
    
    else if (strcmp(argv[1], "--query-gpus") == 0) {

        int num_gpus, num_screens, i;
        unsigned int *pData;

        printf("GPU Information:\n");

        /* Get the number of gpus in the system */
        
        ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU,
                                      &num_gpus);
        if (!ret) {
            fprintf(stderr, "Failed to query number of gpus.\n\n");
            return 1;
        }

        printf("  number of GPUs: %d\n", num_gpus);

        /* List the X screen number of all X screens driven by each gpu */
        
        for (i = 0; i < num_gpus; i++) {
            
            ret = XNVCTRLQueryTargetBinaryData
                (dpy,
                 NV_CTRL_TARGET_TYPE_GPU,
                 i, // target_id
                 0,
                 NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU,
                 (unsigned char **) &pData,
                 &len);
            
            if (!ret) {
                fprintf(stderr, "Failed to query list of X Screens\n");
                return 1;
            }
            
            printf("  number of X screens using GPU %d: %d\n", i, pData[0]);
            
            /* List X Screen number of all X Screens driven by this GPU. */
            
            printf("    Indices of X screens using GPU %d: ", i);
            
            for (j = 1; j <= pData[0]; j++) {
                printf(" %d", pData[j]);
            }
            printf("\n");
            XFree(pData);
        }
        

        /* Get the number of X Screens in the system 
         *
         * NOTE: If Xinerama is enabled, ScreenCount(dpy) will return 1,
         *       where as querying the screen count information from
         *       NV-CONTROL will return the number of underlying X Screens.
         */
        
        ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_X_SCREEN,
                                      &num_screens);
        if (!ret) {
            fprintf(stderr, "Failed to query number of X Screens\n\n");
            return 1;
        }

        printf("\n");
        printf("  number of X screens (ScreenCount): %d\n",
               ScreenCount(dpy));
        printf("  number of X screens (NV-CONTROL): %d\n\n",
               num_screens);

        for (i = 0; i < num_screens; i++) {
            
            ret = XNVCTRLQueryTargetBinaryData
                (dpy,
                 NV_CTRL_TARGET_TYPE_X_SCREEN,
                 i, // target_id
                 0,
                 NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN,
                 (unsigned char **) &pData,
                 &len);
            
            if (!ret) {
                fprintf(stderr, "Failed to query list of gpus\n\n");
                return 1;
            }
            
            printf("  number of GPUs used by X screen %d: %d\n", i,
                   pData[0]);

            /* List gpu number of all gpus driven by this X screen */

            printf("    Indices of GPUs used by X screen %d: ", i);
            for (j = 1; j <= pData[0]; j++) {
                printf(" %d", pData[j]);
            }
            printf("\n");
            XFree(pData);
        }
        
        printf("\n");

    }
    
    
    /*
     * probe for any newly connected display devices
     */
    
    else if (strcmp(argv[1], "--probe-dpys") == 0) {

        int num_gpus, i;

        printf("Display Device Probed Information:\n\n");

        /* Get the number of gpus in the system */
        
        ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU,
                                      &num_gpus);
        
        if (!ret) {
            fprintf(stderr, "Failed to query number of gpus\n\n");
            return 1;
        }
        
        printf("  number of GPUs: %d\n", num_gpus);

        /* Probe and list the Display devices */
        
        for (i = 0; i < num_gpus; i++) {
            
            /* Get the gpu name */
            
            ret = XNVCTRLQueryTargetStringAttribute
                (dpy, NV_CTRL_TARGET_TYPE_GPU, i, 0,
                 NV_CTRL_STRING_PRODUCT_NAME, &str);
            
            if (!ret) {
                fprintf(stderr, "Failed to query gpu name\n\n");
                return 1;
            }

            /* Probe the GPU for new/old display devices */
            
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU, i,
                                              0,
                                              NV_CTRL_PROBE_DISPLAYS,
                                              &display_devices);
            
            if (!ret) {
                fprintf(stderr, "Failed to probe the enabled Display "
                        "Devices on GPU-%d (%s).\n\n", i, str);
                return 1;
            }
            
            printf("  display devices on GPU-%d (%s):\n", i, str);
            XFree(str);
        
            /* Report results */
            
            for (mask = 1; mask < (1 << 24); mask <<= 1) {
                
                if (display_devices & mask) {
                    
                    XNVCTRLQueryTargetStringAttribute
                        (dpy, NV_CTRL_TARGET_TYPE_GPU, i, mask,
                         NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                         &str);
                    
                    printf("    %s (0x%08x): %s\n",
                           display_device_name(mask), mask, str);
                }
            }
            
            printf("\n");
        }
        
        printf("\n");
    }
    
    
    /*
     * query the TwinViewXineramaInfoOrder
     */
    
    else if (strcmp(argv[1], "--query-twinview-xinerama-info-order") == 0) {
        
        ret = XNVCTRLQueryTargetStringAttribute
            (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, screen, 0,
             NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER, &str);
        
        if (!ret) {
            fprintf(stderr, "Failed to query "
                    "TwinViewXineramaInfoOrder.\n\n");
            return 1;
        }
        
        printf("TwinViewXineramaInfoOrder: %s\n\n", str);
    }
    
    
    /*
     * assign the TwinViewXineramaInfoOrder
     */
    
    else if ((strcmp(argv[1], "--assign-twinview-xinerama-info-order")== 0)
             && argv[2]) {
        
        ret = XNVCTRLSetStringAttribute
            (dpy,
             screen,
             0,
             NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER,
             argv[2]);
        
        if (!ret) {
            fprintf(stderr, "Failed to assign "
                    "TwinViewXineramaInfoOrder = \"%s\".\n\n", argv[2]);
            return 1;
        }
        
        printf("assigned TwinViewXineramaInfoOrder: \"%s\"\n\n",
               argv[2]);
    }


    /*
     * use NV_CTRL_MAX_SCREEN_WIDTH and NV_CTRL_MAX_SCREEN_HEIGHT to
     * query the maximum screen dimensions on each GPU in the system
     */
    
    else if (strcmp(argv[1], "--max-screen-size") == 0) {

        int num_gpus, i, width, height;
        
        /* Get the number of gpus in the system */
        
        ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU,
                                      &num_gpus);
        if (!ret) {
            fprintf(stderr, "Failed to query number of gpus.\n\n");
            return 1;
        }
        
        for (i = 0; i < num_gpus; i++) {
            
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              i,
                                              0,
                                              NV_CTRL_MAX_SCREEN_WIDTH,
                                              &width);

            if (!ret) {
                fprintf(stderr, "Failed to query the maximum screen "
                        "width on GPU-%d\n\n", i);
                return 1;
            }
            
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              i,
                                              0,
                                              NV_CTRL_MAX_SCREEN_HEIGHT,
                                              &height);

            if (!ret) {
                fprintf(stderr, "Failed to query the maximum screen "
                        "height on GPU-%d.\n\n", i);
                return 1;
            }
            
            printf("GPU-%d: maximum X screen size: %d x %d.\n\n",
                   i, width, height);
        }
    }


    /*
     * demonstrate how to use NV-CONTROL to query what modelines are
     * used by the MetaModes of the X screen: we first query all the
     * MetaModes, parse out the display device names and mode names,
     * and then lookup the modelines associated with those mode names
     * on those display devices
     *
     * this could be implemented much more efficiently, but
     * demonstrates the general idea
     */
    
    else if (strcmp(argv[1], "--print-used-modelines") == 0) {

        char *pMetaModes, *pModeLines[8], *tmp, *modeString;
        char *modeLine, *modeName, *noWhiteSpace;
        int MetaModeLen, ModeLineLen[8];
        unsigned int thisMask;
        
        /* first, we query the MetaModes on this X screen */
        
        XNVCTRLQueryBinaryData(dpy, screen, 0, // n/a
                               NV_CTRL_BINARY_DATA_METAMODES,
                               (void *) &pMetaModes, &MetaModeLen);
        
        /*
         * then, we query the ModeLines for each display device on
         * this X screen; we'll need these later
         */
        
        nDisplayDevice = 0;

        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (!(display_devices & mask)) continue;
                
            XNVCTRLQueryBinaryData(dpy, screen, mask,
                                   NV_CTRL_BINARY_DATA_MODELINES,
                                   (void *) &str, &len);
                
            pModeLines[nDisplayDevice] = str;
            ModeLineLen[nDisplayDevice] = len;

            nDisplayDevice++;
        }
        
        /* now, parse each MetaMode */
        
        str = start = pMetaModes;
        
        for (j = 0; j < MetaModeLen - 1; j++) {

            /*
             * if we found the end of a line, treat the string from
             * start to str[j] as a MetaMode
             */

            if ((str[j] == '\0') && (str[j+1] != '\0')) {

                printf("MetaMode: %s\n", start);
                
                /*
                 * remove any white space from the string to make
                 * parsing easier
                 */
                
                noWhiteSpace = remove_whitespace(start);

                /*
                 * the MetaMode may be preceded with "token=value"
                 * pairs, separated by the main MetaMode with "::"; if
                 * "::" exists in the string, skip past it
                 */
                
                tmp = strstr(noWhiteSpace, "::");
                if (tmp) {
                    tmp += 2;
                } else {
                    tmp = noWhiteSpace;
                }

                /* split the MetaMode string by comma */

                for (modeString = strtok(tmp, ",");
                     modeString;
                     modeString = strtok(NULL, ",")) {

                    /*
                     * retrieve the modeName and display device mask
                     * for this segment of the Metamode
                     */

                    parse_mode_string(modeString, &modeName, &thisMask);

                    /* lookup the modeline that matches */
                    
                    nDisplayDevice = 0;
                    
                    if (thisMask) {
                        for (mask = 1; mask < (1 << 24); mask <<= 1) {
                            if (!(display_devices & mask)) continue;
                            if (thisMask & mask) break;
                            nDisplayDevice++;
                        }
                    }


                    modeLine = find_modeline(modeName,
                                             pModeLines[nDisplayDevice],
                                             ModeLineLen[nDisplayDevice]);
                    
                    printf("  %s: %s\n",
                           display_device_name(thisMask), modeLine);
                }
                
                printf("\n");

                free(noWhiteSpace);
                
                /* move to the next MetaMode */
                
                start = &str[j+1];
            }
        }
    }
    

    /*
     * demonstrate how to programmatically transition into TwinView
     * using NV-CONTROL and XRandR; the process is roughly:
     *
     * - probe for new display devices
     *
     * - if we found any new display devices, create a mode pool for
     *   the new display devices
     *
     * - associate any new display devices to the X screen, so that
     *   they are available for MetaMode assignment.
     *
     * - add a new MetaMode 
     *
     * We have skipped error checking, and checking for things like if
     * we are already using multiple display devices, but this gives
     * the general idea.
     */

    else if (strcmp(argv[1], "--dynamic-twinview") == 0) {
        
        char *pOut;
        int id;

        /*
         * first, probe for new display devices; while
         * NV_CTRL_CONNECTED_DISPLAYS reports what the NVIDIA X driver
         * believes is currently connected to the GPU,
         * NV_CTRL_PROBE_DISPLAYS forces the driver to redetect what
         * is connected.
         */
        
        XNVCTRLQueryAttribute(dpy,
                              screen,
                              0,
                              NV_CTRL_PROBE_DISPLAYS,
                              &display_devices);
        
        /*
         * if we don't have atleast two display devices, there is
         * nothing to do
         */

        printf("probed display device mask: 0x%08x\n\n", display_devices);
        
        if (count_bits(display_devices) < 2) {
            printf("only one display device found; cannot enable "
                   "TwinView.\n\n");
            return 1;
        }
        
        
        /*
         * next, make sure all display devices have a modepool; a more
         * sophisticated client could use
         * NV_CTRL_BINARY_DATA_MODELINES to query if a pool of
         * modelines already exist on each display device we care
         * about.
         */
        
        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (!(display_devices & mask)) continue;
            
            XNVCTRLStringOperation(dpy,
                                   NV_CTRL_TARGET_TYPE_X_SCREEN,
                                   screen,
                                   mask,
                                   NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,
                                   NULL,
                                   NULL);
        }
        
        printf("all display devices should now have a mode pool.\n\n");
        

        /*
         * associate all the probed display devices to the X screen;
         * this makes the display devices available for MetaMode
         * assignment
         */
        
        ret =
            XNVCTRLSetAttributeAndGetStatus(dpy,
                                            screen,
                                            0,
                                            NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,
                                            display_devices);
        
        if (!ret) {
            printf("unable to assign the associated display devices to "
                   "0x%08x.\n\n", display_devices);
            return 1;
        }

        printf("associated display devices 0x%08x.\n\n", display_devices);


        /*
         * next, we add a new MetaMode; a more sophisticated client
         * would actually select a modeline from
         * NV_CTRL_BINARY_DATA_MODELINES on each display device, but
         * for demonstration purposes, we assume that
         * "nvidia-auto-select" is a valid mode on each display
         * device.
         */
        
        pOut = NULL;

        ret = XNVCTRLStringOperation(dpy,
                                     NV_CTRL_TARGET_TYPE_X_SCREEN,
                                     screen,
                                     0,
                                     NV_CTRL_STRING_OPERATION_ADD_METAMODE,
                                     "nvidia-auto-select, nvidia-auto-select",
                                     &pOut);
        
        if (!ret || !pOut) {
            printf("failed to add MetaMode; this may be because the MetaMode "
                   "already exists for this X screen.\n\n");
            return 1;
        }
        
        /*
         * retrieve the id of the added MetaMode from the return
         * string; a more sophisticated client should do actual
         * parsing of the returned string, which is defined to be a
         * comma-separated list of "token=value" pairs as output.
         * Currently, the only output token is "id", which indicates
         * the id that was assigned to the MetaMode.
         */

        sscanf(pOut, "id=%d", &id);
        
        XFree(pOut);


        /*
         * we have added a new MetaMode for this X screen, and we know
         * it's id.  The last step is to use the XRandR extension to
         * switch to this mode; see the Xrandr(3) manpage for details.
         *
         * For demonstration purposes, just use the xrandr commandline
         * utility to switch to the mode with the refreshrate that
         * matches the id.
         */
        
        printf("The id of the new MetaMode is %d; use xrandr to "
               "switch to it.\n\n", id);
    }
    

    /*
     * print help information
     */

    else {
        
    printHelp:
        
        printf("\nnv-control-dpy [options]:\n\n");
        
        
        printf(" ModeLine options:\n\n");

        printf("  --print-modelines: print the modelines in the mode pool "
               "for each Display Device.\n\n");
        
        printf("  --print-current-modeline: print the current modeline "
               "for each Display Device.\n\n");

        printf("  --add-modeline [dpy mask] [modeline]: "
               "add new modeline.\n\n");
        
        printf("  --delete-modeline [dpy mask] [modename]: "
               "delete modeline with modename.\n\n");
        
        printf("  --generate-gtf-modeline [width] [height] [refreshrate]:"
               " use the GTF formula"
               " to generate a modeline for the specified parameters.\n\n");
        
        printf("  --generate-cvt-modeline [width] [height] [refreshrate]"
               " [reduced-blanking]: use the CVT formula"
               " to generate a modeline for the specified parameters.\n\n");
                

        printf(" MetaMode options:\n\n");

        printf("  --print-metamodes: print the current MetaModes for the "
               "X screen\n\n");

        printf("  --add-metamode [metamode]: add the specified "
               "MetaMode to the X screen's list of MetaModes.\n\n");
        
        printf("  --delete-metamode [metamode]: delete the specified MetaMode "
               "from the X screen's list of MetaModes.\n\n");

        printf("  --print-current-metamode: print the current MetaMode.\n\n");

        
        printf(" Misc options:\n\n");
        
        printf("  --get-valid-freq-ranges: query the valid frequency "
               "information for each display device.\n\n");
        
        printf("  --build-modepool: build a modepool for any display device "
               "that does not already have one.\n\n");
                
        printf("  --get-associated-dpys: query the associated display device "
               "mask for this X screen\n\n");
        
        printf("  --set-associated-dpys [mask]: set the associated display "
               "device mask for this X screen\n\n");
        
        printf("  --query-gpus: print GPU information and relationship to "
               "X screens.\n\n");
        
        printf("  --probe-dpys: probe GPUs for new display devices\n\n");
        
        printf("  --query-twinview-xinerama-info-order: query the "
               "TwinViewXineramaInfoOrder.\n\n");
        
        printf("  --assign-twinview-xinerama-info-order [order]: assign the "
               "TwinViewXineramaInfoOrder.\n\n");

        printf("  --max-screen-size: query the maximum screen size "
               "on all GPUs in the system\n\n");

        printf("  --print-used-modelines: print the modeline for each display "
               "device for each MetaMode on the X screen.\n\n");

        printf("  --dynamic-twinview: demonstrates the process of "
               "dynamically transitioning into TwinView.\n\n");
    }

    return 0;

}



/*****************************************************************************/
/* utility functions */
/*****************************************************************************/


/*
 * display_device_name() - return the display device name correspoding
 * to the specified display device mask.
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



/*
 * display_device_mask() - given a display device name, translate to
 * the display device mask
 */

static unsigned int display_device_mask(char *str)
{
    if (strcmp(str, "CRT-0") == 0) return (1 <<  0);
    if (strcmp(str, "CRT-1") == 0) return (1 <<  1);
    if (strcmp(str, "CRT-2") == 0) return (1 <<  2);
    if (strcmp(str, "CRT-3") == 0) return (1 <<  3);
    if (strcmp(str, "CRT-4") == 0) return (1 <<  4);
    if (strcmp(str, "CRT-5") == 0) return (1 <<  5);
    if (strcmp(str, "CRT-6") == 0) return (1 <<  6);
    if (strcmp(str, "CRT-7") == 0) return (1 <<  7);

    if (strcmp(str, "TV-0") == 0)  return (1 <<  8);
    if (strcmp(str, "TV-1") == 0)  return (1 <<  9);
    if (strcmp(str, "TV-2") == 0)  return (1 << 10);
    if (strcmp(str, "TV-3") == 0)  return (1 << 11);
    if (strcmp(str, "TV-4") == 0)  return (1 << 12);
    if (strcmp(str, "TV-5") == 0)  return (1 << 13);
    if (strcmp(str, "TV-6") == 0)  return (1 << 14);
    if (strcmp(str, "TV-7") == 0)  return (1 << 15);
    
    if (strcmp(str, "DFP-0") == 0) return (1 << 16);
    if (strcmp(str, "DFP-1") == 0) return (1 << 17);
    if (strcmp(str, "DFP-2") == 0) return (1 << 18);
    if (strcmp(str, "DFP-3") == 0) return (1 << 19);
    if (strcmp(str, "DFP-4") == 0) return (1 << 20);
    if (strcmp(str, "DFP-5") == 0) return (1 << 21);
    if (strcmp(str, "DFP-6") == 0) return (1 << 22);
    if (strcmp(str, "DFP-7") == 0) return (1 << 23);
    
    return 0;

} /* display_device_mask() */



/*
 * remove_whitespace() - return an allocated copy of the given string,
 * with any whitespace removed
 */

static char *remove_whitespace(char *str)
{
    int len;
    char *ret, *s, *d;
    
    len = strlen(str);
    
    ret = malloc(len+1);
    
    for (s = str, d = ret; *s; s++) {
        if (!isspace(*s)) {
            *d = *s;
            d++;
        }
    }

    *d = '\0';
    
    return ret;
    
} /* remove_whitespace() */



/*
 * count the number of bits set in the specified mask
 */

static int count_bits(unsigned int mask)
{
    int n = 0;
    
    while (mask) {
        n++;
        mask &= (mask - 1) ;
    }

    return n;
    
} /* count_bits() */



/*
 * parse_mode_string() - extract the modeName and the display device
 * mask for the per-display device MetaMode string in 'modeString'
 */

static void parse_mode_string(char *modeString, char **modeName, int *mask)
{
    char *colon, *s, tmp;

    colon = strchr(modeString, ':');
                    
    if (colon) {
        
        *colon = '\0';
        *mask = display_device_mask(modeString);
        *colon = ':';
        
        modeString = colon + 1;
    } else {
        *mask = 0;
    }
    
    /*
     * find the modename; trim off any panning domain or
     * offsets
     */
    
    for (s = modeString; *s; s++) {
        if (*s == '@') break;
        if ((*s == '+') && isdigit(s[1])) break;
        if ((*s == '-') && isdigit(s[1])) break;
    }
    
    tmp = *s;
    *s = '\0';
    *modeName = strdup(modeString);
    *s = tmp;
    
} /* parse_mode_string() */



/*
 * find_modeline() - search the pModeLines list of ModeLines for the
 * mode named 'modeName'; return a pointer to the matching ModeLine,
 * or NULL if no match is found
 */

static char *find_modeline(char *modeName, char *pModeLines, int ModeLineLen)
{
    char *start, *beginQuote, *endQuote;
    int j, match = 0;
    
    start = pModeLines;
    
    for (j = 0; j < ModeLineLen; j++) {
        if (pModeLines[j] == '\0') {
            
            /*
             * the modeline will contain the modeName in quotes; find
             * the begin and end of the quoted modeName, so that we
             * can compare it to modeName
             */
            
            beginQuote = strchr(start, '"');
            endQuote = beginQuote ? strchr(beginQuote+1, '"') : NULL;
            
            if (beginQuote && endQuote) {
                *endQuote = '\0';
                
                match = (strcmp(modeName, beginQuote+1) == 0);
                
                *endQuote = '"';
                
                /*
                 * if we found a match, return a pointer to the start
                 * of this modeLine
                 */

                if (match) return start;
            }
            
            start = &pModeLines[j+1];
        }
    }
    
    return NULL;
    
} /* find_modeline() */
