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

static char *remove_whitespace(char *str);
static char *mode_strtok(char *str);
static Bool parse_mode_string(char *modeString, char **modeName,
                              int *dpyId);
static char *find_modeline(char *modeString, char *pModeLines,
                           int ModeLineLen);

static void print_display_name(Display *dpy, int target_id, int attr,
                               char *name)
{
    Bool ret;
    char *str;

    ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                            NV_CTRL_TARGET_TYPE_DISPLAY,
                                            target_id, 0,
                                            attr,
                                            &str);
    if (!ret) {
        printf("    %18s : N/A\n", name);
        return;
    }

    printf("    %18s : %s\n", name, str);
    XFree(str);
}

static void print_display_id_and_name(Display *dpy, int target_id,
                                      const char *tab)
{
    char name_str[64];
    int len;

    len = snprintf(name_str, sizeof(name_str), "%sDPY-%d", tab, target_id);

    if ((len < 0) || (len >= sizeof(name_str))) {
        return;
    }

    print_display_name(dpy, target_id, NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                       name_str);
}

int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int screen, major, minor, len, i, j;
    char *str, *start, *str0, *str1;
    int *enabledDpyIds;


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

    printf("\nUsing NV-CONTROL extension %d.%d on %s\n\n",
           major, minor, XDisplayName(NULL));


    /*
     * query the enabled display devices on this X screen and print basic
     * information about each X screen.
     */

    ret = XNVCTRLQueryTargetBinaryData(dpy,
                                       NV_CTRL_TARGET_TYPE_X_SCREEN,
                                       screen,
                                       0,
                                       NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN,
                                       (unsigned char **) &enabledDpyIds,
                                       &len);
    if (!ret || (len < sizeof(enabledDpyIds[0]))) {
        fprintf(stderr, "Failed to query the enabled Display Devices.\n\n");
        return 1;
    }

    printf("Enabled Display Devices:\n");

    for (i = 0; i < enabledDpyIds[0]; i++) {
        int dpyId = enabledDpyIds[i+1];

        print_display_id_and_name(dpy, dpyId, "  ");
    }

    printf("\n");
    
    
    /*
     * perform the requested action, based on the specified
     * commandline option
     */
    
    if (argc <= 1) goto printHelp;


    /*
     * for each enabled display device on this X screen, query the list of
     * modelines in the mode pool using NV_CTRL_BINARY_DATA_MODELINES, then
     * print the results.
     */

    if (strcmp(argv[1], "--print-modelines") == 0) {

        for (i = 0; i < enabledDpyIds[0]; i++) {
            int dpyId = enabledDpyIds[i+1];

            ret = XNVCTRLQueryTargetBinaryData(dpy,
                                               NV_CTRL_TARGET_TYPE_DISPLAY,
                                               dpyId,
                                               0,
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

            printf("Modelines for DPY-%d:\n", dpyId);

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
     * for each enabled display device on this X screen, query the current
     * modeline using NV_CTRL_STRING_CURRENT_MODELINE.
     */

    else if (strcmp(argv[1], "--print-current-modeline") == 0) {

       for (i = 0; i < enabledDpyIds[0]; i++) {
            int dpyId = enabledDpyIds[i+1];

            ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_DISPLAY,
                                                    dpyId,
                                                    0,
                                                    NV_CTRL_STRING_CURRENT_MODELINE,
                                                    &str);
            if (!ret) {
                fprintf(stderr, "Failed to query current ModeLine.\n\n");
                return 1;
            }

            printf("Current Modeline for DPY-%d:\n", dpyId);
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
        
        int dpyId = strtol(argv[2], NULL, 0);

        ret = XNVCTRLSetTargetStringAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_DISPLAY,
                                              dpyId,
                                              0,
                                              NV_CTRL_STRING_ADD_MODELINE,
                                              argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to add the modeline \"%s\" to DPY-%d's "
                    "mode pool.\n\n", argv[3], dpyId);
            return 1;
        }
        
        printf("Added modeline \"%s\" to DPY-%d's mode pool.\n\n",
               argv[3], dpyId);
    }

    
    /*
     * delete the specified modeline from the mode pool for the
     * specified display device, using NV_CTRL_STRING_DELETE_MODELINE
     */
    
    else if ((strcmp(argv[1], "--delete-modeline") == 0) &&
             argv[2] && argv[3]) {
        
        int dpyId = strtol(argv[2], NULL, 0);
    
        ret = XNVCTRLSetTargetStringAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_DISPLAY,
                                              dpyId,
                                              0,
                                              NV_CTRL_STRING_DELETE_MODELINE,
                                              argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to delete the mode \"%s\" from DPY-%d's "
                    "mode pool.\n\n", argv[3], dpyId);
            return 1;
        }
        
        printf("Deleted modeline \"%s\" from DPY-%d's mode pool.\n\n",
               argv[3], dpyId);
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
     * query the MetaModes for the X screen, using
     * NV_CTRL_BINARY_DATA_METAMODES_VERSION_2.
     */

    else if (strcmp(argv[1], "--print-metamodes-version2") == 0) {

        /* get list of metamodes */

        ret = XNVCTRLQueryBinaryData(dpy, screen, 0, // n/a
                                     NV_CTRL_BINARY_DATA_METAMODES_VERSION_2,
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
        
        ret = XNVCTRLQueryStringAttribute(dpy, screen, 0,
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
     * query the currently in use MetaMode.  Note that an alternative
     * way to accomplish this is to use XRandR to query the current
     * mode's refresh rate, and then match the refresh rate to the id
     * reported in the returned NV_CTRL_BINARY_DATA_METAMODES_VERSION_2 data.
     */

    else if (strcmp(argv[1], "--print-current-metamode-version2") == 0) {

        ret = XNVCTRLQueryStringAttribute(dpy, screen, 0,
                                          NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
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
     * Enabled Display Devices:
     *   DPY-0 : EIZO F931
     *   DPY-1 : ViewSonic P815-4
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

        for (i = 0; i < enabledDpyIds[0]; i++) {
            int dpyId = enabledDpyIds[i+1];

            ret = XNVCTRLQueryTargetStringAttribute
                (dpy, NV_CTRL_TARGET_TYPE_DISPLAY, dpyId, 0,
                 NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES,
                 &str0);

            if (!ret) {
                fprintf(stderr, "Failed to query HorizSync for DPY-%d.\n\n",
                        dpyId);
                return 1;
            }

            ret = XNVCTRLQueryTargetStringAttribute
                (dpy, NV_CTRL_TARGET_TYPE_DISPLAY, dpyId, 0,
                 NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES,
                 &str1);

            if (!ret) {
                fprintf(stderr, "Failed to query VertRefresh for DPY-%d.\n\n",
                        dpyId);
                XFree(str0);
                return 1;
            }

            printf("frequency information for DPY-%d:\n", dpyId);
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

        for (i = 0; i < enabledDpyIds[0]; i++) {
            int dpyId = enabledDpyIds[i+1];

            ret = XNVCTRLStringOperation
                (dpy,
                 NV_CTRL_TARGET_TYPE_DISPLAY,
                 dpyId,
                 0,
                 NV_CTRL_STRING_OPERATION_BUILD_MODEPOOL,
                 argv[2],
                 &str0);

            if (!ret) {
                fprintf(stderr, "Failed to build modepool for DPY-%d (it most "
                        "likely already has a modepool).\n\n", dpyId);
            } else {
                printf("Built modepool for DPY-%d.\n\n", dpyId);
            }
        }
    }

    
    /*
     * query the assigned display devices on this X screen; these are the
     * display devices that are available to the X screen for use by MetaModes.
     */

    else if (strcmp(argv[1], "--get-assigned-dpys") == 0) {

        int *pData = NULL;
        int len;

        ret = XNVCTRLQueryTargetBinaryData(dpy,
                                           NV_CTRL_TARGET_TYPE_X_SCREEN,
                                           screen,
                                           0,
                                           NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN,
                                           (unsigned char **) &pData,
                                           &len);
        if (!ret || (len < sizeof(pData[0]))) {
            fprintf(stderr, "failed to query the assigned display "
                    "devices.\n\n");
            return 1;
        }

        printf("Assigned display devices:\n");

        for (i = 0; i < pData[0]; i++) {
            int dpyId = pData[i+1];

            printf("  DPY-%d\n", dpyId);
        }

        printf("\n");
        XFree(pData);
    }

    /*
     * query information about the GPUs in the system
     */
    
    else if (strcmp(argv[1], "--query-gpus") == 0) {

        int num_gpus, num_screens, i;
        int *pData;

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
            
            if (!ret || (len < sizeof(pData[0]))) {
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
            
            if (!ret || (len < sizeof(pData[0]))) {
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
            int deprecated;
            int *pData;
            
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
                                              &deprecated);
            
            if (!ret) {
                fprintf(stderr, "Failed to probe the enabled Display "
                        "Devices on GPU-%d (%s).\n\n", i, str);
                return 1;
            }
            
            printf("  display devices on GPU-%d (%s):\n", i, str);
            XFree(str);
        
            /* Report results */

            ret = XNVCTRLQueryTargetBinaryData(dpy,
                                               NV_CTRL_TARGET_TYPE_GPU, i,
                                               0,
                                               NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU,
                                               (unsigned char **) &pData,
                                               &len);
            if (!ret || (len < sizeof(pData[0]))) {
                fprintf(stderr, "Failed to query the connected Display Devices.\n\n");
                return 1;
            }

            for (j = 0; j < pData[0]; j++) {
                int dpyId = pData[j+1];

                print_display_id_and_name(dpy, dpyId, "    ");
            }

            printf("\n");
        }
        
        printf("\n");
    }
    
    
    /*
     * query the nvidiaXineramaInfoOrder
     */
    
    else if (strcmp(argv[1], "--query-nvidia-xinerama-info-order") == 0) {
        
        ret = XNVCTRLQueryTargetStringAttribute
            (dpy, NV_CTRL_TARGET_TYPE_X_SCREEN, screen, 0,
             NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER, &str);
        
        if (!ret) {
            fprintf(stderr, "Failed to query nvidiaXineramaInfoOrder.\n\n");
            return 1;
        }
        
        printf("nvidiaXineramaInfoOrder: %s\n\n", str);
    }
    
    
    /*
     * assign the nvidiaXineramaInfoOrder
     */
    
    else if ((strcmp(argv[1], "--assign-nvidia-xinerama-info-order")== 0)
             && argv[2]) {
        
        ret = XNVCTRLSetStringAttribute
            (dpy,
             screen,
             0,
             NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER,
             argv[2]);
        
        if (!ret) {
            fprintf(stderr, "Failed to assign "
                    "nvidiaXineramaInfoOrder = \"%s\".\n\n", argv[2]);
            return 1;
        }
        
        printf("assigned nvidiaXineramaInfoOrder: \"%s\"\n\n",
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
        int MetaModeLen, ModeLineLen[8], ModeLineDpyId[8];
        int dpyId;

        /* first, we query the MetaModes on this X screen */
        
        XNVCTRLQueryBinaryData(dpy, screen, 0,
                               NV_CTRL_BINARY_DATA_METAMODES_VERSION_2,
                               (void *) &pMetaModes, &MetaModeLen);
        
        /*
         * then, we query the ModeLines for each display device on
         * this X screen; we'll need these later
         */

        for (i = 0; i < enabledDpyIds[0]; i++) {
            dpyId = enabledDpyIds[i+1];

            XNVCTRLQueryTargetBinaryData(dpy, NV_CTRL_TARGET_TYPE_DISPLAY,
                                         dpyId,
                                         0,
                                         NV_CTRL_BINARY_DATA_MODELINES,
                                         (void *) &str, &len);

            pModeLines[i] = str;
            ModeLineLen[i] = len;
            ModeLineDpyId[i] = dpyId;
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

                /* Parse each mode from the metamode */

                for (modeString = mode_strtok(tmp);
                     modeString;
                     modeString = mode_strtok(NULL)) {

                    /*
                     * retrieve the modeName and display device id
                     * for this segment of the Metamode
                     */

                    if (!parse_mode_string(modeString, &modeName, &dpyId)) {
                        fprintf(stderr, "  Failed to parse mode string '%s'."
                                "\n\n",
                                modeString);
                        continue;
                    }

                    /* lookup the modeline that matches */

                    for (i = 0; i < enabledDpyIds[0]; i++) {
                        if (ModeLineDpyId[i] == dpyId) {
                            break;
                        }
                    }
                    if ( i >= enabledDpyIds[0] ) {
                        fprintf(stderr, "  Failed to find modelines for "
                                "DPY-%d.\n\n",
                                dpyId);
                        continue;
                    }

                    modeLine = find_modeline(modeName,
                                             pModeLines[i],
                                             ModeLineLen[i]);

                    printf("  DPY-%d: %s\n", dpyId, modeLine);
                }

                printf("\n");

                free(noWhiteSpace);
                
                /* move to the next MetaMode */
                
                start = &str[j+1];
            }
        }
    }


    /* Display all names each display device goes by
     */
    else if (strcmp(argv[1], "--print-display-names") == 0) {
        int *pData;
        int len, i;

        printf("Display Device Information:\n");

        ret = XNVCTRLQueryTargetBinaryData(dpy,
                                           NV_CTRL_TARGET_TYPE_GPU,
                                           0,
                                           0,
                                           NV_CTRL_BINARY_DATA_DISPLAY_TARGETS,
                                           (unsigned char **) &pData,
                                           &len);
        if (!ret || (len < sizeof(pData[0]))) {
            fprintf(stderr, "Failed to query number of display devices.\n\n");
            return 1;
        }

        printf("  number of display devices: %d\n", pData[0]);

        for (i = 1; i <= pData[0]; i++) {

            printf("\n  Display Device: %d\n", pData[i]);

            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME,
                               "Type Basename");
            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_TYPE_ID,
                               "Type ID");
            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_DP_GUID,
                               "DP GUID");
            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_EDID_HASH,
                               "EDID HASH");
            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_TARGET_INDEX,
                               "Target Index");
            print_display_name(dpy, pData[i],
                               NV_CTRL_STRING_DISPLAY_NAME_RANDR,
                               "RANDR");
        }
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

        printf("  --add-modeline [dpy id] [modeline]: "
               "add new modeline.\n\n");
        
        printf("  --delete-modeline [dpy id] [modename]: "
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

        printf("  --print-metamodes-version2: print the current MetaModes for "
               "the X screen with extended information\n\n");

        printf("  --add-metamode [metamode]: add the specified "
               "MetaMode to the X screen's list of MetaModes.\n\n");
        
        printf("  --delete-metamode [metamode]: delete the specified MetaMode "
               "from the X screen's list of MetaModes.\n\n");

        printf("  --print-current-metamode: print the current MetaMode.\n\n");

        printf("  --print-current-metamode-version2: print the current "
               "MetaMode with extended information.\n\n");


        printf(" Misc options:\n\n");
        
        printf("  --get-valid-freq-ranges: query the valid frequency "
               "information for each display device.\n\n");
        
        printf("  --build-modepool: build a modepool for any display device "
               "that does not already have one.\n\n");
                
        printf("  --get-assigned-dpys: query the assigned display device for "
               "this X screen\n\n");
        
        printf("  --query-gpus: print GPU information and relationship to "
               "X screens.\n\n");
        
        printf("  --probe-dpys: probe GPUs for new display devices\n\n");
        
        printf("  --query-nvidia-xinerama-info-order: query the "
               "nvidiaXineramaInfoOrder.\n\n");
        
        printf("  --assign-nvidia-xinerama-info-order [order]: assign the "
               "nvidiaXineramaInfoOrder.\n\n");

        printf("  --max-screen-size: query the maximum screen size "
               "on all GPUs in the system\n\n");

        printf("  --print-used-modelines: print the modeline for each display "
               "device for each MetaMode on the X screen.\n\n");

        printf("  --print-display-names: print all the names associated with "
               "each display device on the server\n\n");
    }

    return 0;

}



/*****************************************************************************/
/* utility functions */
/*****************************************************************************/


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
 * Special strtok function for parsing modes.  This function ignores
 * anything between curly braces, including commas when parsing tokens
 * delimited by commas.
 */
static char *mode_strtok(char *str)
{
    static char *intStr = NULL;
    char *start;

    if (str) {
        intStr = str;
    }

    if (!intStr || *intStr == '\0') {
        return NULL;
    }

    /* Mark off the next token value */
    start = intStr;
    while (*intStr != '\0') {
        if (*intStr == '{') {
            while (*intStr != '}' && *intStr != '\0') {
                intStr++;
            }
        }
        if (*intStr == ',') {
            *intStr = '\0';
            intStr++;
            break;
        }
        intStr++;
    }

    return start;
}



/*
 * parse_mode_string() - extract the modeName and the display device
 * id for the per-display device MetaMode string in 'modeString'
 */

static Bool parse_mode_string(char *modeString, char **modeName,
                              int *dpyId)
{
    char *colon, *s, tmp;

    colon = strchr(modeString, ':');
    if (!colon) {
        return False;
    }
    *colon = '\0';
    *dpyId = strtol(modeString+4, NULL, 0);
    *colon = ':';

    modeString = colon + 1;

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

    return True;
}



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
