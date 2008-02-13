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
 * nv-control-dpy.c - NV-CONTROL client that demonstrates how to
 * configure display devices using NV-CONTROL.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"




/*
 * display_device_name() - return the display device name correspoding
 * to the display device mask.
 */

char *display_device_name(int mask)
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
    int screen, display_devices, mask, major, minor, len, j;
    char *str, *start;
    char *displayDeviceNames[8];
    int nDisplayDevice;
    
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

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        fprintf(stderr, "The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }
    
    ret = XNVCTRLQueryAttribute(dpy, screen, 0,
                                NV_CTRL_ENABLED_DISPLAYS, &display_devices);

    if (!ret) {
        fprintf(stderr, "Failed to query the enabled Display Devices.\n");
        return 1;
    }

    /* print basic information about what display devices are present */


    printf("\n");
    printf("Using NV-CONTROL extension %d.%d on %s\n",
           major, minor, XDisplayName(NULL));
    printf("Display Devices:\n");
    
    nDisplayDevice = 0;
    for (mask = 1; mask < (1 << 24); mask <<= 1) {

        if (display_devices & mask) {
            
            XNVCTRLQueryStringAttribute
                (dpy, screen, mask,
                 NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                 &str);
            
            displayDeviceNames[nDisplayDevice++] = str;
            
            printf("  %s (0x%08x): %s\n",
                   display_device_name(mask), mask, str);
        }
    }
    
    printf("\n");
    
    /* now, parse the commandline and perform the requested action */


    if (argc <= 1) goto printHelp;
    
    if (strcmp(argv[1], "-l") == 0) {

        /* get list of modelines, per display device */

        nDisplayDevice = 0;
        
        for (mask = 1; mask < (1 << 24); mask <<= 1) {
            
            if (display_devices & mask) {
            
                XNVCTRLQueryBinaryData(dpy, screen, mask,
                                       NV_CTRL_BINARY_DATA_MODELINES,
                                       (void *) &str, &len);
                
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
        
    } else if (strcmp(argv[1], "-m") == 0) {

        /* get list of metamodes */
        
        XNVCTRLQueryBinaryData(dpy, screen, 0, // n/a
                               NV_CTRL_BINARY_DATA_METAMODES,
                               (void *) &str, &len);
        
        printf("MetaModes:\n");
        
        start = str;
        for (j = 0; j < len; j++) {
            if (str[j] == '\0') {
                printf("  %s\n", start);
                start = &str[j+1];
            }
        }
        
        XFree(str);

    } else if ((strcmp(argv[1], "-M") == 0) && (argv[2])) {
        
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        0,
                                        NV_CTRL_STRING_ADD_METAMODE,
                                        argv[2]);

        if (!ret) {
            fprintf(stderr, "Failed to add the MetaMode.\n");
            return 1;
        }

        printf("Uploaded MetaMode \"%s\"; magicID: %d\n",
               argv[2], ret);

    } else if ((strcmp(argv[1], "-D") == 0) && (argv[1])) {
        
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        0,
                                        NV_CTRL_STRING_DELETE_METAMODE,
                                        argv[2]);

        if (!ret) {
            fprintf(stderr, "Failed to delete the MetaMode.\n");
            return 1;
        }
        
    } else if (strcmp(argv[1], "-c") == 0) {
        
        ret = XNVCTRLQueryStringAttribute
            (dpy, screen, mask,
             NV_CTRL_STRING_CURRENT_METAMODE,
             &str);

        if (!ret) {
            fprintf(stderr, "Failed to query the current MetaMode.\n");
            return 1;
        }
        
        printf("current metamode: \"%s\"\n", str);

    } else if ((strcmp(argv[1], "-L") == 0) && argv[2] && argv[3]) {
        
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        strtol(argv[2], NULL, 0),
                                        NV_CTRL_STRING_ADD_MODELINE,
                                        argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to add the modeline \"%s\".\n", argv[3]);
            return 1;
        }

    } else if ((strcmp(argv[1], "-d") == 0) && argv[2] && argv[3]) {
        
        ret = XNVCTRLSetStringAttribute(dpy,
                                        screen,
                                        strtol(argv[2], NULL, 0),
                                        NV_CTRL_STRING_DELETE_MODELINE,
                                        argv[3]);
        
        if (!ret) {
            fprintf(stderr, "Failed to delete the mode \"%s\".\n", argv[3]);
            return 1;
        }
        
    } else if (strcmp(argv[1], "-g") == 0) {

        int num_gpus, num_screens, i;
        unsigned int *pData;

        printf("GPU Information:\n");

        /* Get the number of gpus in the system */
        ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_GPU, &num_gpus);
        if (!ret) {
            fprintf(stderr, "Failed to query number of gpus\n");
            return 1;
        }

        printf("\n");
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

            printf("    X screens using GPU %d: ", i);
            
            for (j = 1; j <= pData[0]; j++) {
                printf(" %d", pData[j]);
            }
            printf("\n");
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
            fprintf(stderr, "Failed to query number of X Screens\n");
            return 1;
        }

        printf("\n");
        printf("  number of X screens (ScreenCount): %d\n", ScreenCount(dpy));
        printf("\n");
        printf("  number of X screens (NV-CONTROL): %d\n", num_screens);

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
                fprintf(stderr, "Failed to query list of gpus\n");
                return 1;
            }
            
            printf("  number of GPUs used by X screen %d: %d\n", i, pData[0]);

            /* List gpu number of all gpus driven by this X screen */

            printf("    GPUs used by X screen %d: ", i);
            for (j = 1; j <= pData[0]; j++) {
                printf(" %d", pData[j]);
            }
            printf("\n");
        }
        
        printf("\n");
 
    } else {
        
    printHelp:
        
        printf("usage:\n");
        printf("-l: print the modelines in the ModePool "
               "for each Display Device.\n");
        printf("-L [dpy mask] [modeline]: add new modeline.\n");
        printf("-d [dpy mask] [modename]: delete modeline with modename\n");
        printf("-m: print the current MetaModes\n");
        printf("-M [metamode]: add the specified MetaMode to the "
               "server's list of MetaModes.\n");
        printf("-D [metamode]: delete the specified MEtaMode from the "
               "server's list of MetaModes.\n");
        printf("-c: print the current MetaMode\n");
        printf("-g: print GPU information\n");
        printf("\n");
    }

    return 0;

}
