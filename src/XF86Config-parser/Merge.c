/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"



/*
 * xconfigAddRemovedOptionComment() - Makes a note in the comment
 * string "existing_comments" that a particular option has been
 * removed.
 *
 */
static void xconfigAddRemovedOptionComment(char **existing_comments,
                                           XConfigOptionPtr option)
{
    int len;
    char *str;
    char *name, *value;

    if (!option || !existing_comments)
        return;

    name = xconfigOptionName(option);
    value = xconfigOptionValue(option);

    if (!name) return;

    if (value) {
        len = 32 + strlen(name) + strlen(value);
        str = malloc(len);
        if (!str) return;
        snprintf(str, len, "# Removed Option \"%s\" \"%s\"", name, value);
    } else {
        len = 32 + strlen(name);
        str = malloc(len);
        if (!str) return;
        snprintf(str, len, "# Removed Option \"%s\"", name);
    }

    *existing_comments = xconfigAddComment(*existing_comments, str);

} /* xconfigAddRemovedOptionComment() */



/*
 * xconfigRemoveNamedOption() - Removes the named option from an option
 * list and (if specified) adds a comment to an existing comments string
 *
 */
void xconfigRemoveNamedOption(XConfigOptionPtr *pHead, const char *name,
                              char **comments)
{
    XConfigOptionPtr option;

    option = xconfigFindOption(*pHead, name);
    if (option) {
        if (comments) {
            xconfigAddRemovedOptionComment(comments, option);
        }
        xconfigRemoveOption(pHead, option);
    }

} /* xconfigRemoveNamedOption() */



/*
 * xconfigOptionValuesDiffer() - return '1' if the option values for
 * option0 and option1 are different; return '0' if the option values
 * are the same.
 */

static int xconfigOptionValuesDiffer(XConfigOptionPtr option0,
                                     XConfigOptionPtr option1)
{
    char *value0, *value1;

    value0 = value1 = NULL;

    if (!option0 && !option1) return 0;
    if (!option0 &&  option1) return 1;
    if ( option0 && !option1) return 1;

    value0 = xconfigOptionValue(option0);
    value1 = xconfigOptionValue(option1);

    if (!value0 && !value1) return 0;
    if (!value0 &&  value1) return 1;
    if ( value0 && !value1) return 1;

    return (strcmp(value0, value1) != 0);

} /* xconfigOptionValuesDiffer() */



/*
 * xconfigMergeOption() - Merge option "name" from option source
 * list "srcHead" to option destination list "dstHead".
 *
 * Merging here means:
 *
 * If the option is not in the source config, do nothing to the
 * destination.  Otherwise, either add or update the option in
 * the dest.  If the option is modified, and a comment is given,
 * then the old option will be commented out instead of being
 * simply removed/replaced.
 */
static void xconfigMergeOption(XConfigOptionPtr *dstHead,
                               XConfigOptionPtr *srcHead,
                               const char *name, char **comments)
{
    XConfigOptionPtr srcOption = xconfigFindOption(*srcHead, name);
    XConfigOptionPtr dstOption = xconfigFindOption(*dstHead, name);

    char *srcValue = NULL;

    if (!srcOption) {
        /* Option does not exist in src, do nothing to dst. */
        return;
    }

    srcValue = xconfigOptionValue(srcOption);

    if (srcOption && !dstOption) {

        /* option exists in src but not in dst: add to dst */
        xconfigAddNewOption(dstHead, name, srcValue);

    } else if (srcOption && dstOption) {

        /*
         * option exists in src and in dst; if the option values are
         * different, replace the dst's option value with src's option
         * value; note that xconfigAddNewOption() will remove the old
         * option first, if necessary
         */

        if (xconfigOptionValuesDiffer(srcOption, dstOption)) {
            if (comments) {
                xconfigAddRemovedOptionComment(comments, dstOption);
            }
            xconfigAddNewOption(dstHead, name, srcValue);
        }
    }

} /* xconfigMergeOption() */



/*
 * xconfigMergeFlags() - Updates the destination's list of server flag
 * options with the options found in the source config.
 *
 * Optons in the destination are either added or updated.  Options that
 * are found in the destination config and not in the source config are
 * not modified.
 *
 * Returns 1 if the merge was successful and 0 if not.
 */
static int xconfigMergeFlags(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    if (srcConfig->flags) {
        XConfigOptionPtr option;
        
        /* Flag section was not found, create a new one */
        if (!dstConfig->flags) {
            dstConfig->flags =
                (XConfigFlagsPtr) calloc(1, sizeof(XConfigFlagsRec));
            if (!dstConfig->flags) return 0;
        }
        
        option = srcConfig->flags->options;
        while (option) {
            xconfigMergeOption(&(dstConfig->flags->options),
                               &(srcConfig->flags->options),
                               xconfigOptionName(option),
                               &(dstConfig->flags->comment));
            option = option->next;
        }
    }
    
    return 1;

} /* xconfigMergeFlags() */



/*
 * xconfigMergeMonitors() - Updates information in the destination monitor
 * with that of the source monitor.
 *
 */
static void xconfigMergeMonitors(XConfigMonitorPtr dstMonitor,
                                 XConfigMonitorPtr srcMonitor)
{
    int i;


    /* Update vendor */
    
    free(dstMonitor->vendor);
    dstMonitor->vendor = xconfigStrdup(srcMonitor->vendor);
    
    /* Update modelname */
    
    free(dstMonitor->modelname);
    dstMonitor->modelname = xconfigStrdup(srcMonitor->modelname);
    
    /* Update horizontal sync */
    
    dstMonitor->n_hsync = srcMonitor->n_hsync;
    for (i = 0; i < srcMonitor->n_hsync; i++) {
        dstMonitor->hsync[i].lo = srcMonitor->hsync[i].lo;
        dstMonitor->hsync[i].hi = srcMonitor->hsync[i].hi;
    }
    
    /* Update vertical sync */
    
    dstMonitor->n_vrefresh = srcMonitor->n_vrefresh;
    for (i = 0; i < srcMonitor->n_hsync; i++) {
        dstMonitor->vrefresh[i].lo = srcMonitor->vrefresh[i].lo;
        dstMonitor->vrefresh[i].hi = srcMonitor->vrefresh[i].hi;
    }
    
    /* XXX Remove the destination monitor's "UseModes" references to
     *     avoid having the wrong modelines tied to the new monitor.
     */
    xconfigFreeModesLinkList(&dstMonitor->modes_sections);

} /* xconfigMergeMonitors() */



/*
 * xconfigMergeAllMonitors() - This function ensures that all monitors in
 * the source config appear in the destination config by adding and/or
 * updating the "appropriate" destination monitor sections.
 *
 */
static int xconfigMergeAllMonitors(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    XConfigMonitorPtr dstMonitor;
    XConfigMonitorPtr srcMonitor;


    /* Make sure all monitors in the src config are also in the dst config */

    for (srcMonitor = srcConfig->monitors;
         srcMonitor;
         srcMonitor = srcMonitor->next) {

        dstMonitor =
            xconfigFindMonitor(srcMonitor->identifier, dstConfig->monitors);

        /* Monitor section was not found, create a new one and add it */
        if (!dstMonitor) {
            dstMonitor =
                (XConfigMonitorPtr) calloc(1, sizeof(XConfigMonitorRec));
            if (!dstMonitor) return 0;

            dstMonitor->identifier = xconfigStrdup(srcMonitor->identifier);

            xconfigAddListItem((GenericListPtr *)(&dstConfig->monitors),
                               (GenericListPtr)dstMonitor);
        }

        /* Do the merge */
        xconfigMergeMonitors(dstMonitor, srcMonitor);
    }

    return 1;

} /* xconfigMergeAllMonitors() */



/*
 * xconfigMergeDevices() - Updates information in the destination device
 * with that of the source device.
 *
 */
static void xconfigMergeDevices(XConfigDevicePtr dstDevice,
                                XConfigDevicePtr srcDevice)
{
    // XXX Zero out the device section?

    /* Update driver */
    
    free(dstDevice->driver);
    dstDevice->driver = xconfigStrdup(srcDevice->driver);
    
    /* Update vendor */
    
    free(dstDevice->vendor);
    dstDevice->vendor = xconfigStrdup(srcDevice->vendor);
    
    /* Update bus ID */
    
    free(dstDevice->busid);
    dstDevice->busid = xconfigStrdup(srcDevice->busid);
    
    /* Update board */
    
    free(dstDevice->board);
    dstDevice->board = xconfigStrdup(srcDevice->board);
    
    /* Update chip info */
    
    dstDevice->chipid = srcDevice->chipid;
    dstDevice->chiprev = srcDevice->chiprev;

    /* Update IRQ */

    dstDevice->irq = srcDevice->irq;
    
    /* Update screen */
    
    dstDevice->screen = srcDevice->screen;

} /* xconfigMergeDevices() */



/*
 * xconfigMergeAllDevices() - This function ensures that all devices in
 * the source config appear in the destination config by adding and/or
 * updating the "appropriate" destination device sections.
 *
 */
static int xconfigMergeAllDevices(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    XConfigDevicePtr dstDevice;
    XConfigDevicePtr srcDevice;


    /* Make sure all monitors in the src config are also in the dst config */

    for (srcDevice = srcConfig->devices;
         srcDevice;
         srcDevice = srcDevice->next) {

        dstDevice =
            xconfigFindDevice(srcDevice->identifier, dstConfig->devices);
        
        /* Device section was not found, create a new one and add it */
        if (!dstDevice) {
            dstDevice =
                (XConfigDevicePtr) calloc(1, sizeof(XConfigDeviceRec));
            if (!dstDevice) return 0;

            dstDevice->identifier = xconfigStrdup(srcDevice->identifier);

            xconfigAddListItem((GenericListPtr *)(&dstConfig->devices),
                               (GenericListPtr)dstDevice);
        }

        /* Do the merge */
        xconfigMergeDevices(dstDevice, srcDevice);
    }

    return 1;

} /* xconfigMergeAllDevices() */



/*
 * xconfigMergeDriverOptions() - Update the (Screen) driver options
 * of the destination config with information from the source config.
 *
 * - Assumes the source options are all found in the srcScreen->options.
 * - Updates only those options listed in the srcScreen->options.
 *
 */
static int xconfigMergeDriverOptions(XConfigScreenPtr dstScreen,
                                     XConfigScreenPtr srcScreen)
{
    XConfigOptionPtr option;
    XConfigDisplayPtr display;

    option = srcScreen->options;
    while (option) {
        char *name = xconfigOptionName(option);

        /* Remove the option from all non-screen option lists */
        
        if (dstScreen->device) {
            xconfigRemoveNamedOption(&(dstScreen->device->options), name,
                                     &(dstScreen->device->comment));
        }
        if (dstScreen->monitor) {
            xconfigRemoveNamedOption(&(dstScreen->monitor->options), name,
                                     &(dstScreen->monitor->comment));
        }       
        for (display = dstScreen->displays; display; display = display->next) {
            xconfigRemoveNamedOption(&(display->options), name,
                                     &(display->comment));
        }

        /* Update/Add the option to the screen's option list */
        {
            // XXX Only add a comment if the value changed.
            XConfigOptionPtr old =
                xconfigFindOption(dstScreen->options, name);

            if (old && xconfigOptionValuesDiffer(option, old)) {
                xconfigRemoveNamedOption(&(dstScreen->options), name,
                                         &(dstScreen->comment));
            } else {
                xconfigRemoveNamedOption(&(dstScreen->options), name,
                                         NULL);
            }
        }

        /* Add the option to the screen->options list */

        xconfigAddNewOption(&dstScreen->options,
                            name, xconfigOptionValue(option));
        
        option = option->next;
    }

    return 1;

} /* xconfigMergeDriverOptions() */



/*
 * xconfigMergeDisplays() - Duplicates display information from the
 * source screen to the destination screen.
 *
 */
static int xconfigMergeDisplays(XConfigScreenPtr dstScreen,
                                XConfigScreenPtr srcScreen)
{
    XConfigDisplayPtr dstDisplay;
    XConfigDisplayPtr srcDisplay;
    XConfigModePtr srcMode, dstMode, lastDstMode;

    /* Free all the displays in the destination screen */

    xconfigFreeDisplayList(&dstScreen->displays);

    /* Copy all te displays */
    
    for (srcDisplay = srcScreen->displays;
         srcDisplay;
         srcDisplay = srcDisplay->next) {

        /* Create a new display */

        dstDisplay = xconfigAlloc(sizeof(XConfigDisplayRec));
        if (!dstDisplay) return 0;

        /* Copy display fields */

        dstDisplay->frameX0 = srcDisplay->frameX0;
        dstDisplay->frameY0 = srcDisplay->frameY0;
        dstDisplay->virtualX = srcDisplay->virtualX;
        dstDisplay->virtualY = srcDisplay->virtualY;
        dstDisplay->depth = srcDisplay->depth;
        dstDisplay->bpp = srcDisplay->bpp;
        dstDisplay->visual = xconfigStrdup(srcDisplay->visual);
        dstDisplay->weight = srcDisplay->weight;
        dstDisplay->black = srcDisplay->black;
        dstDisplay->white = srcDisplay->white;
        dstDisplay->comment = xconfigStrdup(srcDisplay->comment);

        /* Copy options over */

        dstDisplay->options = xconfigOptionListDup(srcDisplay->options);

        /* Copy modes over */

        lastDstMode = NULL;
        srcMode = srcDisplay->modes;
        dstMode = NULL;
        while (srcMode) {

            /* Copy the mode */
            
            xconfigAddMode(&dstMode, srcMode->mode_name);

            /* Add mode at the end of the list */

            if ( !lastDstMode ) {
                dstDisplay->modes = dstMode;
            } else {
                lastDstMode->next = dstMode;
            }
            lastDstMode = dstMode;

            srcMode = srcMode->next;
        }

        xconfigAddListItem((GenericListPtr *)(&dstScreen->displays),
                           (GenericListPtr)dstDisplay);
    }

    return 1;

} /* xconfigMergeDisplays() */



/*
 * xconfigMergeScreens() - Updates information in the destination screen
 * with that of the source screen.
 *
 * NOTE: This assumes the Monitor and Device sections have already been
 *       merged.
 *
 */
static void xconfigMergeScreens(XConfigScreenPtr dstScreen,
                                XConfigPtr dstConfig,
                                XConfigScreenPtr srcScreen,
                                XConfigPtr srcConfig)
{
    /* Use the right device */
    
    free(dstScreen->device_name);
    dstScreen->device_name = xconfigStrdup(srcScreen->device_name);
    dstScreen->device =
        xconfigFindDevice(dstScreen->device_name, dstConfig->devices);
    

    /* Use the right monitor */
    
    free(dstScreen->monitor_name);
    dstScreen->monitor_name = xconfigStrdup(srcScreen->monitor_name);
    dstScreen->monitor =
        xconfigFindMonitor(dstScreen->monitor_name, dstConfig->monitors);
    

    /* Update the right default depth */
    
    dstScreen->defaultdepth = srcScreen->defaultdepth;
    

    /* Copy over the display section */
    
    xconfigMergeDisplays(dstScreen, srcScreen);
   

    /* Update the screen's driver options */

    xconfigMergeDriverOptions(dstScreen, srcScreen);

} /* xconfigMergeScreens() */



/*
 * xconfigMergeAllScreens() - This function ensures that all screens in
 * the source config appear in the destination config by adding and/or
 * updating the "appropriate" destination screen sections.
 *
 */
static int xconfigMergeAllScreens(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    XConfigScreenPtr srcScreen;
    XConfigScreenPtr dstScreen;


    /* Make sure all src screens are in the dst config */

    for (srcScreen = srcConfig->screens;
         srcScreen;
         srcScreen = srcScreen->next) {

        dstScreen =
            xconfigFindScreen(srcScreen->identifier, dstConfig->screens);

        /* Screen section was not found, create a new one and add it */
        if (!dstScreen) {
            dstScreen =
                (XConfigScreenPtr) calloc(1, sizeof(XConfigScreenRec));
            if (!dstScreen) return 0;

            dstScreen->identifier = xconfigStrdup(srcScreen->identifier);

            xconfigAddListItem((GenericListPtr *)(&dstConfig->screens),
                               (GenericListPtr)dstScreen);
        }

        /* Do the merge */
        xconfigMergeScreens(dstScreen, dstConfig, srcScreen, srcConfig);
    }

    return 1;

} /* xconfigMergeAllScreens() */



/*
 * xconfigMergeLayout() - Updates information in the destination's first
 * layout with that of the source's first layout.
 *
 */
static int xconfigMergeLayout(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    XConfigLayoutPtr srcLayout = srcConfig->layouts;
    XConfigLayoutPtr dstLayout = dstConfig->layouts;

    XConfigAdjacencyPtr srcAdj;
    XConfigAdjacencyPtr dstAdj;
    XConfigAdjacencyPtr lastDstAdj;

    if (!dstLayout || !srcLayout) {
        return 0;
    }

    /* Clear the destination's adjacency list */

    xconfigFreeAdjacencyList(&dstLayout->adjacencies);
    
    /* Copy adjacencies over */
    
    lastDstAdj = NULL;
    srcAdj = srcLayout->adjacencies;
    while (srcAdj) {
        
        /* Copy the adjacency */
        
        dstAdj =
            (XConfigAdjacencyPtr) calloc(1, sizeof(XConfigAdjacencyRec));

        dstAdj->scrnum = srcAdj->scrnum;
        dstAdj->screen_name = xconfigStrdup(srcAdj->screen_name);
        dstAdj->top_name = xconfigStrdup(srcAdj->top_name);
        dstAdj->bottom_name = xconfigStrdup(srcAdj->bottom_name);
        dstAdj->left_name = xconfigStrdup(srcAdj->left_name);
        dstAdj->right_name = xconfigStrdup(srcAdj->right_name);
        dstAdj->where = srcAdj->where;
        dstAdj->x = srcAdj->x;
        dstAdj->y = srcAdj->y;
        dstAdj->refscreen = xconfigStrdup(srcAdj->refscreen);

        dstAdj->screen =
            xconfigFindScreen(dstAdj->screen_name, dstConfig->screens);
        dstAdj->top =
            xconfigFindScreen(dstAdj->top_name, dstConfig->screens);
        dstAdj->bottom =
            xconfigFindScreen(dstAdj->bottom_name, dstConfig->screens);
        dstAdj->left =
            xconfigFindScreen(dstAdj->left_name, dstConfig->screens);
        dstAdj->right =
            xconfigFindScreen(dstAdj->right_name, dstConfig->screens);

        /* Add adjacency at the end of the list */
        
        if (!lastDstAdj) {
            dstLayout->adjacencies = dstAdj;
        } else {
            lastDstAdj->next = dstAdj;
        }
        lastDstAdj = dstAdj;
        
        srcAdj = srcAdj->next;
    }

    /* Merge the options */
    
    if (srcLayout->options) {
        XConfigOptionPtr srcOption;

        srcOption = srcLayout->options;
        while (srcOption) {
            xconfigMergeOption(&(dstLayout->options),
                               &(srcLayout->options),
                               xconfigOptionName(srcOption),
                               &(dstLayout->comment));
            srcOption = srcOption->next;
        }
    }

    return 1;

} /* xconfigMergeLayout() */



/*
 * xconfigMergeConfigs() - Merges the source X configuration with the
 * destination X configuration.
 *
 * NOTE: This function is currently only used for merging X config files
 *       for display configuration reasons.  As such, the merge assumes
 *       that the dst config file is the target config file and that
 *       mostly, only new display configuration information should be
 *       copied from the source X config to the destination X config.
 *
 */
int xconfigMergeConfigs(XConfigPtr dstConfig, XConfigPtr srcConfig)
{
    /* Make sure the X config is valid */
    // make_xconfig_usable(dstConfig);


    /* Merge the server flag (Xinerama) section */

    if (!xconfigMergeFlags(dstConfig, srcConfig)) {
        return 0;
    }


    /* Merge the monitor sections */

    if (!xconfigMergeAllMonitors(dstConfig, srcConfig)) {
        return 0;
    }


    /* Merge the device sections */

    if (!xconfigMergeAllDevices(dstConfig, srcConfig)) {
        return 0;
    }


    /* Merge the screen sections */

    if (!xconfigMergeAllScreens(dstConfig, srcConfig)) {
        return 0;
    }


    /* Merge the first layout */
    
    if (!xconfigMergeLayout(dstConfig, srcConfig)) {
        return 0;
    }

    return 1;

} /* xconfigMergeConfigs() */
