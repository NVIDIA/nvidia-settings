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

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "NVCtrlLib.h"

#include "msg.h"

#include "parse.h"

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xvlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/utsname.h>



/*
 * NvCtrlAttributeInit() - XXX not sure how to handle errors
 */

NvCtrlAttributeHandle *NvCtrlAttributeInit(Display *dpy, int screen,
                                           unsigned int subsystems)
{
    NvCtrlAttributePrivateHandle *h = NULL;

    h = calloc(1, sizeof (NvCtrlAttributePrivateHandle));
    if (!h) {
        nv_error_msg("Memory allocation failure!");
        goto failed;
    }

    /* initialize the display and screen to the parameter values */

    h->dpy = dpy;
    h->screen = screen;
       
    /* initialize the NV-CONTROL attributes; give up if this fails */

    if (subsystems & NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM) {
        h->nv = NvCtrlInitNvControlAttributes(h);
        if (!h->nv) goto failed;
    }

    /*
     * initialize the XF86VidMode attributes; it is OK if this
     * fails
     */

    if (subsystems & NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM) {
        h->vm = NvCtrlInitVidModeAttributes(h);
    }

    /*
     * initialize the XVideo extension and attributes; it is OK if
     * this fails
     */

    if (subsystems & NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM) {
        NvCtrlInitXvOverlayAttributes(h);
    }
    
    return (NvCtrlAttributeHandle *) h;

 failed:
    if (h) free (h);
    return NULL;

} /* NvCtrlAttributeInit() */


/*
 * NvCtrlGetDisplayName() - return a string of the form:
 * 
 * [host]:[display].[screen]
 *
 * that describes the X screen associated with this
 * NvCtrlAttributeHandle.  This is done by getting the string that
 * describes the display connection, and then substituting the correct
 * screen number.  If no hostname is present in the display string,
 * uname.nodename is prepended.  Returns NULL if any error occors.
 */

char *NvCtrlGetDisplayName(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;
    char *display_name;
        
    if (!handle) return NULL;
    
    h = (NvCtrlAttributePrivateHandle *) handle;
    
    display_name = DisplayString(h->dpy);
    
    return nv_standardize_screen_name(display_name, h->screen);
    
} /* NvCtrlGetDisplayName() */


/*
 * NvCtrlDisplayPtr() - returns the Display pointer associated with
 * this NvCtrlAttributeHandle.
 */
  
Display *NvCtrlGetDisplayPtr(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return NULL;

    h = (NvCtrlAttributePrivateHandle *) handle;
    
    return h->dpy;

} /* NvCtrlDisplayPtr() */


/*
 * NvCtrlGetScreen() - returns the screen number associated with this
 * NvCtrlAttributeHandle.
 */

int NvCtrlGetScreen(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;
    
    return h->screen;

} /* NvCtrlGetScreen() */


/*
 * NvCtrlGetScreenWidth() - return the width of the screen associated
 * with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenWidth(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    return DisplayWidth(h->dpy, h->screen);
    
} /* NvCtrlGetScreenWidth() */


/*
 * NvCtrlGetScreenHeight() - return the height of the screen
 * associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenHeight(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    return DisplayHeight(h->dpy, h->screen);
    
} /* NvCtrlGetScreenHeight() */


int NvCtrlGetEventBase(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return 0;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->nv) return 0;
    return (h->nv->event_base);
    
} /* NvCtrlGetEventBase() */


ReturnStatus NvCtrlGetAttribute(NvCtrlAttributeHandle *handle,
                                int attr, int *val)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlGetDisplayAttribute(handle, 0, attr, val);
    
} /* NvCtrlGetAttribute() */


ReturnStatus NvCtrlSetAttribute(NvCtrlAttributeHandle *handle,
                                int attr, int val)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlSetDisplayAttribute(handle, 0, attr, val);

} /* NvCtrlSetAttribute() */


ReturnStatus NvCtrlGetValidAttributeValues(NvCtrlAttributeHandle *handle,
                                           int attr,
                                           NVCTRLAttributeValidValuesRec *val)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlGetValidDisplayAttributeValues(handle, 0, attr, val);
    
} /* NvCtrlGetValidAttributeValues() */


ReturnStatus NvCtrlGetStringAttribute(NvCtrlAttributeHandle *handle,
                                      int attr, char **ptr)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlGetStringDisplayAttribute(handle, 0, attr, ptr);

} /* NvCtrlGetStringAttribute() */


ReturnStatus
NvCtrlGetDisplayAttribute(NvCtrlAttributeHandle *handle,
                          unsigned int display_mask, int attr, int *val)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if ((attr >= NV_CTRL_ATTR_EXT_BASE) &&
        (attr <= NV_CTRL_ATTR_EXT_LAST_ATTRIBUTE)) {
        switch (attr) {
          case NV_CTRL_ATTR_EXT_NV_PRESENT:
            *val = (h->nv) ? True : False; break;
          case NV_CTRL_ATTR_EXT_VM_PRESENT:
            *val = (h->vm) ? True : False; break;
          case NV_CTRL_ATTR_EXT_XV_OVERLAY_PRESENT:
            *val = (h->xv_overlay) ? True : False; break;
          case NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT:
            *val = (h->xv_texture) ? True : False; break;
          case NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT:
            *val = (h->xv_blitter) ? True : False; break;
          default:
            return NvCtrlNoAttribute;
        }
        return NvCtrlSuccess;
    }
    
    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetAttribute(h, display_mask, attr, val);
    }

    if ((attr >= NV_CTRL_ATTR_XV_BASE) &&
        (attr <= NV_CTRL_ATTR_XV_LAST_ATTRIBUTE)) {
        
        return NvCtrlXvGetAttribute(h, attr, val);
    }
    return NvCtrlNoAttribute;
    
} /* NvCtrlGetDisplayAttribute() */


ReturnStatus
NvCtrlSetDisplayAttribute(NvCtrlAttributeHandle *handle,
                          unsigned int display_mask, int attr, int val)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;
    
    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlSetAttribute(h, display_mask, attr, val);
    }

    if ((attr >= NV_CTRL_ATTR_XV_BASE) &&
        (attr <= NV_CTRL_ATTR_XV_LAST_ATTRIBUTE)) {
        
        return NvCtrlXvSetAttribute(h, attr, val);
    }
    
    return NvCtrlNoAttribute;

} /* NvCtrlSetDisplayAttribute() */


ReturnStatus
NvCtrlGetValidDisplayAttributeValues(NvCtrlAttributeHandle *handle,
                                     unsigned int display_mask, int attr,
                                     NVCTRLAttributeValidValuesRec *val)
{
    NvCtrlAttributePrivateHandle *h;
    
    h = (NvCtrlAttributePrivateHandle *) handle;
    
    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetValidAttributeValues(h, display_mask,
                                                      attr, val);
    }

    if ((attr >= NV_CTRL_ATTR_XV_BASE) &&
        (attr <= NV_CTRL_ATTR_XV_LAST_ATTRIBUTE)) {

        return NvCtrlXvGetValidAttributeValues(h, attr, val);
    }
    
    return NvCtrlNoAttribute;
    
} /* NvCtrlGetValidDisplayAttributeValues() */


ReturnStatus
NvCtrlGetStringDisplayAttribute(NvCtrlAttributeHandle *handle,
                                unsigned int display_mask,
                                int attr, char **ptr)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetStringAttribute(h, display_mask, attr, ptr);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetStringDisplayAttribute() */


char *NvCtrlAttributesStrError(ReturnStatus status)
{
    switch (status) {
      case NvCtrlSuccess:
          return "Success"; break;
      case NvCtrlBadArgument:
          return "Bad argument"; break;
      case NvCtrlBadHandle:
          return "Bad handle"; break;
      case NvCtrlNoAttribute:
          return "No such attribute"; break;
      case NvCtrlMissingExtension:
          return "Missing Extension"; break;
      case NvCtrlReadOnlyAttribute:
          return "Read only attribute"; break;
      case NvCtrlWriteOnlyAttribute:
          return "Write only attribute"; break;
      case NvCtrlAttributeNotAvailable:
          return "Attribute not available"; break;
      case NvCtrlError: /* fall through to default */
      default:
        return "Unknown Error"; break;
    }
} /* NvCtrlAttributesStrError() */


void NvCtrlAttributeClose(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;
    
    if (!handle) return;
    
    h = (NvCtrlAttributePrivateHandle *) handle;

    /*
     * XXX should free any additional resources allocated by each
     * subsystem
     */

    free(h);

} /* NvCtrlAttributeClose() */
