/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
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
 * nv-control-framelock.c - NV-CONTROL client that demonstrates how to
 * interact with the frame lock capabilities on an X Server.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"


/*
 * do_help()
 *
 * Prints some help on how to use this app.
 *
 */
static void do_help(void)
{
    printf("usage:\n");
    printf("-q: query system frame lock information.\n");
    printf("-e: enable frame lock on system.\n");
    printf("-d: disable frame lock on system.\n");
    printf("\n");

} /* do_help()*/



/*
 * do_query()
 *
 * Prints information for all frame lock (g-sync) devices found on
 * the given X server.
 *
 */
static void do_query(Display *dpy)
{
    Bool ret;
    int num_framelocks;
    int framelock;
    int gpu;
    int mask;
    char *name;
    
    int *data;
    int len;
    int i;

    int enabled;


    /* Query the number of frame lock devices on the server */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                  &num_framelocks);
    if (!ret) {
        printf("Failed to query number of frame lock devices!\n");
        return;
    }
    printf("Found %d frame lock device(s) on server.\n", num_framelocks);
    if ( !num_framelocks ) {
        return;
    }

    /* Display information for all frame lock devices found */

    for (framelock = 0; framelock < num_framelocks; framelock++) {
        
        printf("\n");
        printf("- Frame Lock Board %d :\n", framelock);

        /* Query the GPUs connected to this frame lock device */

        ret = XNVCTRLQueryTargetBinaryData
            (dpy,
             NV_CTRL_TARGET_TYPE_FRAMELOCK,
             framelock, // target_id
             0, // display_mask
             NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
             (unsigned char **) &data,
             &len);
        if (!ret) {
            printf("  - Failed to query list of GPUs!\n");
            continue;
        }

        /* Display information for all GPUs connected to frame lock device */

        if ( !data[0] ) {
            printf("  - No GPUs found!\n");
        } else {
            printf("  - Found %d GPU(s).\n", data[0]);
        }

        for (i = 1; i <= data[0]; i++) {
            gpu = data[i];
        
            /* Query GPU product name */
            
            ret = XNVCTRLQueryTargetStringAttribute(dpy,
                                                    NV_CTRL_TARGET_TYPE_GPU,
                                                    gpu, // target_id
                                                    0, // display_mask
                                                    NV_CTRL_STRING_PRODUCT_NAME,
                                                    &name);
            if (!ret) {
                printf("  - Failed to query GPU %d product name.\n", gpu);
                continue;
            } 
            printf("  - GPU %d (%s) :\n", gpu, name);

            /* Query GPU sync state */

            printf("    - Sync          : ");
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_FRAMELOCK_SYNC,
                                              &enabled);
            if (!ret) {
                printf("Failed to query sync state.\n");
            } else {
                printf("%sabled\n", enabled ? "En" : "Dis");
            }

            /* Query GPU displays */

            printf("    - Displays Mask : ");
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_ENABLED_DISPLAYS,
                                              &mask);
            if (!ret) {
                printf("Failed to query enabled displays.\n");
            } else {
                printf("0x%08u\n", mask);
            }

            /* Query GPU server (master) */

            printf("    - Server Mask   : ");
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_FRAMELOCK_MASTER,
                                              &mask);
            if (!ret) {
                printf("Failed to query server mask.\n");
            } else {
                printf("0x%08u\n", mask);
            }

            /* Query GPU clients (slaves) */

            printf("    - Clients Mask  : ");
            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_FRAMELOCK_SLAVES,
                                              &mask);
            if (!ret) {
                printf("Failed to query clients mask.\n");
            } else {
                printf("0x%08u\n", mask);
            }

        } /* Done disabling GPUs */

        XFree(data);

    } /* Done disabling Frame Lock Devices */

} /* do_query() */



/*
 * do_enable()
 *
 * Enables frame lock on the X Server by setting the first capable/available
 * display device as the frame lock server (master) and setting all other
 * display devices as clients (slaves).
 *
 * NOTE: It is up to the user to ensure that each display device is set with
 *       the same refresh rate (mode timings).
 *
 */
static void do_enable(Display *dpy)
{
    Bool ret;
    int num_framelocks;
    int framelock;
    int gpu;
    unsigned int mask;

    int *data;
    int len;
    int i;

    int enabled;
    int masterable;
    int pick_server = 1;
    int server_set = 0;


    /* Query the number of frame lock devices to enable */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                  &num_framelocks);
    if (!ret) {
        printf("Failed to query number of frame lock devices!\n");
        return;
    }
    printf("Found %d frame lock device(s) on server.\n", num_framelocks);
    if ( !num_framelocks ) {
        return;
    }

    /* Enable frame lock on all GPUs connected to each frame lock device */
    
    for (framelock = 0; framelock < num_framelocks; framelock++) {
        
        printf("\n");
        printf("- Frame Lock Board %d :\n", framelock);

        /* Query the GPUs connected to this frame lock device */
        
        ret = XNVCTRLQueryTargetBinaryData
            (dpy,
             NV_CTRL_TARGET_TYPE_FRAMELOCK,
             framelock, // target_id
             0, // display_mask
             NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
             (unsigned char **) &data,
             &len);
        if (!ret) {
            printf("  - Failed to query list of GPUs!\n");
            continue;
        }
        
        /* Enable frame lock on all GPUs connected to the frame lock device */
        
        if ( !data[0] ) {
            printf("  - No GPUs found!\n");
        } else {
            printf("  - Found %d GPU(s).\n", data[0]);
        }

        for (i = 1; i <= data[0]; i++) {
            gpu = data[i];
        
            printf("  - Enabling G-Sync Device %d - GPU %d... ",
                   framelock, gpu);
            
            /* Make sure frame lock is disabled */

            XNVCTRLQueryTargetAttribute(dpy,
                                        NV_CTRL_TARGET_TYPE_GPU,
                                        gpu, // target_id
                                        0, // display_mask
                                        NV_CTRL_FRAMELOCK_SYNC,
                                        &enabled);
            if (enabled != NV_CTRL_FRAMELOCK_SYNC_DISABLE) {
                printf("Frame lock already enabled!\n");
                continue;
            }

            /* Get the list of displays to enable */

            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_ENABLED_DISPLAYS,
                                              &mask);
            if (!ret) {
                printf("Failed to query enabled displays!\n");
                continue;
            }

            /* Query if this GPU can be set as master */

            ret = XNVCTRLQueryTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              mask, // display_mask
                                              NV_CTRL_FRAMELOCK_MASTERABLE,
                                              &masterable);
            if (!ret) {
                printf("Failed to query masterable!\n");
                continue;
            }
            
            /* Clear the master setting if any */
            
            if (masterable == NV_CTRL_FRAMELOCK_MASTERABLE_TRUE) {
                XNVCTRLSetTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GPU,
                                          gpu, // target_id
                                          0, // display_mask
                                          NV_CTRL_FRAMELOCK_MASTER,
                                          0);
            }
            
            /* Clear the slaves setting if any */

            XNVCTRLSetTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      gpu, // target_id
                                      0, // display_mask
                                      NV_CTRL_FRAMELOCK_SLAVES,
                                      0);

            printf("\n");
            
            /* Pick the first available/capable display device as master */

            if (pick_server &&
                masterable == NV_CTRL_FRAMELOCK_MASTERABLE_TRUE) {
                
                /* Just pick the first enabled display */
                
                unsigned int master = (1<<31);
                while (master && !(master & mask)) {
                    master >>= 1;
                }
                
                if (master) {
                    mask &= ~master;

                    /* Make sure we're not using the House Sync signal. */
                    
                    XNVCTRLSetTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                              framelock, // target_id
                                              0, // display_mask
                                              NV_CTRL_USE_HOUSE_SYNC,
                                              NV_CTRL_USE_HOUSE_SYNC_FALSE);
                    
                    /* Set the master */
                    
                    XNVCTRLSetTargetAttribute(dpy,
                                              NV_CTRL_TARGET_TYPE_GPU,
                                              gpu, // target_id
                                              0, // display_mask
                                              NV_CTRL_FRAMELOCK_MASTER,
                                              master);

                    printf("    - Set Server Display    : 0x%08u\n", master);
                    pick_server = 0;
                    server_set = 1;
                }
            }

            /* Set rest of enabled display devices as clients (slaves) */

            if (mask) {
                XNVCTRLSetTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GPU,
                                          gpu, // target_id
                                          0, // display_mask
                                          NV_CTRL_FRAMELOCK_SLAVES,
                                          mask);
                printf("    - Set Client Display(s) : 0x%08u\n", mask);
            }

            /* Enable frame lock */

            XNVCTRLSetTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      gpu, // target_id
                                      0, // display_mask
                                      NV_CTRL_FRAMELOCK_SYNC,
                                      NV_CTRL_FRAMELOCK_SYNC_ENABLE);
            XFlush(dpy);
            printf("    - Frame Lock Sync Enabled.\n");
            
            /* If we just enabled the server, also toggle the test signal
             * to guarentee accuracy of the universal frame count (as
             * returned by the glXQueryFrameCountNV() function in the
             * GLX_NV_swap_group extension).
             */
            if (server_set) {
                XNVCTRLSetTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GPU,
                                          gpu, // target_id
                                          0, // display_mask
                                          NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                                          NV_CTRL_FRAMELOCK_TEST_SIGNAL_ENABLE);
                                          
                XNVCTRLSetTargetAttribute(dpy,
                                          NV_CTRL_TARGET_TYPE_GPU,
                                          gpu, // target_id
                                          0, // display_mask
                                          NV_CTRL_FRAMELOCK_TEST_SIGNAL,
                                          NV_CTRL_FRAMELOCK_TEST_SIGNAL_DISABLE);

                printf("    - Frame Lock Test Signal Toggled.\n");
                server_set = 0;
            }

        } /* Done enabling GPUs */

        XFree(data);

    } /* Done enabling framelocks */
}



static void do_disable(Display *dpy)
{
    Bool ret;
    int num_framelocks;
    int framelock;
    int gpu;

    int *data;
    int len;
    int i;

    /* Query the number of frame lock devices to disable */

    ret = XNVCTRLQueryTargetCount(dpy, NV_CTRL_TARGET_TYPE_FRAMELOCK,
                                  &num_framelocks);
    if (!ret) {
        printf("Failed to query number of frame lock devices!\n");
        return;
    }
    printf("Found %d frame lock device(s) on server.\n", num_framelocks);
    if ( !num_framelocks ) {
        return;
    }

    /* Disable frame lock on all GPUs connected to each frame lock device */

    for (framelock = 0; framelock < num_framelocks; framelock++) {
        
        printf("\n");
        printf("- Frame Lock Board %d :\n", framelock);

        /* Query the GPUs connected to this frame lock device */

        ret = XNVCTRLQueryTargetBinaryData
            (dpy,
             NV_CTRL_TARGET_TYPE_FRAMELOCK,
             framelock, // target_id
             0, // display_mask
             NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK,
             (unsigned char **) &data,
             &len);
        if (!ret) {
            printf("  - Failed to query list of GPUs!\n");
            continue;
        }

        /* Disable frame lock on all GPUs connected to the frame lock device */
        
        if ( !data[0] ) {
            printf("  - No GPUs found!\n");
        } else {
            printf("  - Found %d GPU(s).\n", data[0]);
        }

        for (i = 1; i <= data[0]; i++) {
            gpu = data[i];
        
            printf("  - Disabling G-Sync Device %d - GPU %d... ",
                   framelock, gpu);

            XNVCTRLSetTargetAttribute(dpy,
                                      NV_CTRL_TARGET_TYPE_GPU,
                                      gpu, // target_id
                                      0, // display_mask
                                      NV_CTRL_FRAMELOCK_SYNC,
                                      NV_CTRL_FRAMELOCK_SYNC_DISABLE);
            XFlush(dpy);
            printf("Done.\n");

        } /* Done disabling GPUs */

        XFree(data);

    } /* Done disabling Frame Lock Devices */

} /* do_disable() */



int main(int argc, char *argv[])
{
    Display *dpy;
    Bool ret;
    int major, minor;

    /*
     * Open a display connection, and make sure the NV-CONTROL X
     * extension is present on the screen we want to use.
     */
    
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("Cannot open display '%s'.\n", XDisplayName(NULL));
        return 1;
    }
    
    /* Query the NV-CONTROL version */

    ret = XNVCTRLQueryVersion(dpy, &major, &minor);
    if (ret != True) {
        printf("The NV-CONTROL X extension does not exist on '%s'.\n",
                XDisplayName(NULL));
        return 1;
    }
    
    /* Print some information */

    printf("Using NV-CONTROL extension %d.%d on %s\n\n",
           major, minor, XDisplayName(NULL));
    
    /* Do what the user wants */

    if (argc <= 1 || (strcmp(argv[1], "-q") == 0)) {
        do_query(dpy);

    } else if (strcmp(argv[1], "-e") == 0) {
        do_enable(dpy);

    } else if (strcmp(argv[1], "-d") == 0) {
        do_disable(dpy);

    } else {
        do_help();
    }

    return 0;
}
