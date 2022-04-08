/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004,2012 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include "NVCtrlLib.h"

#include "msg.h"

#include "parse.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h> /* pow(3) */

#include <sys/utsname.h>



/*
 * NvCtrlAttributeInit() - XXX not sure how to handle errors
 */

NvCtrlAttributeHandle *NvCtrlAttributeInit(Display *dpy, int target_type,
                                           int target_id,
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
    h->target_type = target_type;
    h->target_id = target_id;

    /* initialize the NV-CONTROL attributes; give up if this fails */

    if (subsystems & NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM) {
        h->nv = NvCtrlInitNvControlAttributes(h);
        if (!h->nv) goto failed;
    }

    /*
     * initialize X Screen specific attributes for X Screen
     * target types.
     */

    if (target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {

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
            h->xv = NvCtrlInitXvAttributes(h);
        }
        
        /*
         * initialize the GLX extension and attributes; it is OK if
         * this fails
         */
        
        if (subsystems & NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM) {
            h->glx = NvCtrlInitGlxAttributes(h);
        }
    } /* X Screen target type attribute subsystems */

    /*
     * initialize the XRandR extension and attributes; this does not
     * require an X screen and it is OK if this fails
     */
    if (subsystems & NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM) {
        h->xrandr = NvCtrlInitXrandrAttributes(h);
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

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) {
        /* Return the display name and # without a screen number */
        return nv_standardize_screen_name(display_name, -2);
    }
    
    return nv_standardize_screen_name(display_name, h->target_id);
    
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

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;
    
    return h->target_id;

} /* NvCtrlGetScreen() */


/*
 * NvCtrlGetTargetType() - returns the target type associated with this
 * NvCtrlAttributeHandle.
 */

int NvCtrlGetTargetType(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    return h->target_type;

} /* NvCtrlGetTargetType() */


/*
 * NvCtrlGetTargetId() - returns the target id number associated with this
 * NvCtrlAttributeHandle.
 */

int NvCtrlGetTargetId(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    return h->target_id;

} /* NvCtrlGetTargetId() */


/*
 * NvCtrlGetScreenWidth() - return the width of the screen associated
 * with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenWidth(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;

    return DisplayWidth(h->dpy, h->target_id);
    
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

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;

    return DisplayHeight(h->dpy, h->target_id);
    
} /* NvCtrlGetScreenHeight() */


int NvCtrlGetEventBase(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->nv) return -1;
    return (h->nv->event_base);
    
} /* NvCtrlGetEventBase() */


int NvCtrlGetXrandrEventBase(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->xrandr) return -1;
    return (h->xrandr->event_base);
    
} /* NvCtrlGetXrandrEventBase() */


/*
 * NvCtrlGetServerVendor() - return the server vendor
 * information string associated with this
 * NvCtrlAttributeHandle.
 */

char *NvCtrlGetServerVendor(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return NULL;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->dpy) return NULL;
    return ServerVendor(h->dpy);

} /* NvCtrlGetServerVendor() */


/*
 * NvCtrlGetVendorRelease() - return the server vendor
 * release number associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetVendorRelease(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->dpy) return -1;
    return VendorRelease(h->dpy);

} /* NvCtrlGetVendorRelease() */


/*
 * NvCtrlGetScreenCount() - return the number of (logical)
 * X Screens associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenCount(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->dpy) return -1;
    return ScreenCount(h->dpy);

} /* NvCtrlGetScreenCount() */


/*
 * NvCtrlGetProtocolVersion() - Returns the majoy version
 * number of the X protocol (server).
 */

int NvCtrlGetProtocolVersion(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->dpy) return -1;
    return ProtocolVersion(h->dpy);

} /* NvCtrlGetProtocolVersion() */


/*
 * NvCtrlGetProtocolRevision() - Returns the revision number
 * of the X protocol (server).
 */

int NvCtrlGetProtocolRevision(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->dpy) return -1;
    return ProtocolRevision(h->dpy);

} /* NvCtrlGetProtocolRevision() */


/*
 * NvCtrlGetScreenWidthMM() - return the width (in Millimeters) of the
 * screen associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenWidthMM(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;

    return DisplayWidthMM(h->dpy, h->target_id);
    
} /* NvCtrlGetScreenWidthMM() */


/*
 * NvCtrlGetScreenHeightMM() - return the height (in Millimeters) of the
 * screen associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenHeightMM(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;

    return DisplayHeightMM(h->dpy, h->target_id);
    
} /* NvCtrlGetScreenHeightMM() */


/*
 * NvCtrlGetScreenPlanes() - return the number of planes (the depth)
 * of the screen associated with this NvCtrlAttributeHandle.
 */

int NvCtrlGetScreenPlanes(NvCtrlAttributeHandle *handle)
{
    NvCtrlAttributePrivateHandle *h;

    if (!handle) return -1;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) return -1;

    return DisplayPlanes(h->dpy, h->target_id);
    
} /* NvCtrlGetScreenPlanes() */



ReturnStatus NvCtrlQueryTargetCount(NvCtrlAttributeHandle *handle,
                                    int target_type,
                                    int *val)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;
    if (!h) return NvCtrlBadArgument;
    return NvCtrlNvControlQueryTargetCount(handle, target_type, val);

} /* NvCtrlQueryTargetCount() */

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


ReturnStatus NvCtrlGetAttribute64(NvCtrlAttributeHandle *handle,
                                  int attr, int64_t *val)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlGetDisplayAttribute64(handle, 0, attr, val);

} /* NvCtrlGetAttribute64() */


ReturnStatus NvCtrlGetVoidAttribute(NvCtrlAttributeHandle *handle,
                                    int attr, void **ptr)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlGetVoidDisplayAttribute(handle, 0, attr, ptr);
    
} /* NvCtrlGetVoidAttribute() */


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


ReturnStatus NvCtrlSetStringAttribute(NvCtrlAttributeHandle *handle,
                                      int attr, char *ptr, int *ret)
{
    if (!handle) return NvCtrlBadArgument;
    return NvCtrlSetStringDisplayAttribute(handle, 0, attr, ptr, ret);

} /* NvCtrlSetStringAttribute() */


ReturnStatus
NvCtrlGetDisplayAttribute64(NvCtrlAttributeHandle *handle,
                            unsigned int display_mask, int attr, int64_t *val)
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
            *val = h->xv && h->xv->overlay; break;
          case NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT:
            *val = h->xv && h->xv->texture; break;
          case NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT:
            *val = h->xv && h->xv->blitter; break;
          default:
            return NvCtrlNoAttribute;
        }
        return NvCtrlSuccess;
    }

    if (attr >= NV_CTRL_ATTR_RANDR_BASE &&
        attr <= NV_CTRL_ATTR_RANDR_LAST_ATTRIBUTE) {
        return NvCtrlXrandrGetAttribute(h, display_mask, attr, val);
    }

    if (((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) ||
        ((attr >= NV_CTRL_ATTR_NV_BASE) &&
         (attr <= NV_CTRL_ATTR_NV_LAST_ATTRIBUTE))) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetAttribute(h, display_mask, attr, val);
    }

    return NvCtrlNoAttribute;
    
} /* NvCtrlGetDisplayAttribute64() */

ReturnStatus
NvCtrlGetDisplayAttribute(NvCtrlAttributeHandle *handle,
                          unsigned int display_mask, int attr, int *val)
{
    ReturnStatus status;
    int64_t value_64;

    status = NvCtrlGetDisplayAttribute64(handle, display_mask, attr, &value_64);
    *val = value_64;

    return status;

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

    return NvCtrlNoAttribute;

} /* NvCtrlSetDisplayAttribute() */


ReturnStatus
NvCtrlSetDisplayAttributeWithReply(NvCtrlAttributeHandle *handle,
                                   unsigned int display_mask,
                                   int attr, int val)
{
    NvCtrlAttributePrivateHandle *h;
    
    h = (NvCtrlAttributePrivateHandle *) handle;
    
    if ((attr >= 0) && (attr <= NV_CTRL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlSetAttributeWithReply(h, display_mask,
                                                    attr, val);
    }
    
    return NvCtrlNoAttribute;
}


ReturnStatus
NvCtrlGetVoidDisplayAttribute(NvCtrlAttributeHandle *handle,
                              unsigned int display_mask, int attr, void **ptr)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if ( attr >= NV_CTRL_ATTR_GLX_BASE &&
         attr >= NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE ) {
        if ( !(h->glx) ) return NvCtrlMissingExtension;
        return NvCtrlGlxGetVoidAttribute(h, display_mask, attr, ptr);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetVoidDisplayAttribute() */


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

    return NvCtrlNoAttribute;
    
} /* NvCtrlGetValidDisplayAttributeValues() */


/*
 * GetValidStringDisplayAttributeValuesExtraAttr() -fill the
 * NVCTRLAttributeValidValuesRec strucure for extra string atrributes i.e.
 * NvCtrlNvControl*, NvCtrlGlx*, NvCtrlXrandr*, NvCtrlVidMode*, or NvCtrlXv*.
 */

static ReturnStatus
GetValidStringDisplayAttributeValuesExtraAttr(NVCTRLAttributeValidValuesRec
                                                    *val)
{
    if (val) {
        memset(val, 0, sizeof(NVCTRLAttributeValidValuesRec));
        val->type = ATTRIBUTE_TYPE_STRING;
        val->permissions = ATTRIBUTE_TYPE_READ | ATTRIBUTE_TYPE_X_SCREEN;
        return NvCtrlSuccess;
    } else {
        return NvCtrlBadArgument;
    }
} /* GetValidStringDisplayAttributeValuesExtraAttr() */


/*
 * NvCtrlGetValidStringDisplayAttributeValues() -fill the
 * NVCTRLAttributeValidValuesRec strucure for String atrributes
 */

ReturnStatus
NvCtrlGetValidStringDisplayAttributeValues(NvCtrlAttributeHandle *handle,
                                           unsigned int display_mask, int attr,
                                           NVCTRLAttributeValidValuesRec *val)
{
    NvCtrlAttributePrivateHandle *h;
    h = (NvCtrlAttributePrivateHandle *) handle;

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetValidStringDisplayAttributeValues(h,
                                                                   display_mask,
                                                                   attr, val);
    }

    /*
     * The below string attributes are only available for X screen target
     * types
     */

    if (h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlAttributeNotAvailable;
    }
    
    if ((attr >= NV_CTRL_STRING_NV_CONTROL_BASE) &&
        (attr <= NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_GLX_BASE) &&
        (attr <= NV_CTRL_STRING_GLX_LAST_ATTRIBUTE)) {
        if (!h->glx) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XRANDR_BASE) &&
        (attr <= NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE)) {
        if (!h->xrandr) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XF86VIDMODE_BASE) &&
        (attr <= NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE)) {
        if (!h->vm) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    if ((attr >= NV_CTRL_STRING_XV_BASE) &&
        (attr <= NV_CTRL_STRING_XV_LAST_ATTRIBUTE)) {
        if (!h->xv) return NvCtrlMissingExtension;
        return GetValidStringDisplayAttributeValuesExtraAttr(val);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetValidStringDisplayAttributeValues() */


ReturnStatus
NvCtrlGetStringDisplayAttribute(NvCtrlAttributeHandle *handle,
                                unsigned int display_mask,
                                int attr, char **ptr)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetStringAttribute(h, display_mask, attr, ptr);
    }

    if ((attr >= NV_CTRL_STRING_NV_CONTROL_BASE) &&
        (attr <= NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlGetStringAttribute(h, display_mask, attr, ptr);
    }

    if ((attr >= NV_CTRL_STRING_GLX_BASE) &&
        (attr <= NV_CTRL_STRING_GLX_LAST_ATTRIBUTE)) {
        if (!h->glx) return NvCtrlMissingExtension;
        return NvCtrlGlxGetStringAttribute(h, display_mask, attr, ptr);
    }

    if ((attr >= NV_CTRL_STRING_XRANDR_BASE) &&
        (attr <= NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE)) {
        if (!h->xrandr) return NvCtrlMissingExtension;
        return NvCtrlXrandrGetStringAttribute(h, display_mask, attr, ptr);
    }

    if ((attr >= NV_CTRL_STRING_XF86VIDMODE_BASE) &&
        (attr <= NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE)) {
        if (!h->vm) return NvCtrlMissingExtension;
        return NvCtrlVidModeGetStringAttribute(h, display_mask, attr, ptr);
    }

    if ((attr >= NV_CTRL_STRING_XV_BASE) &&
        (attr <= NV_CTRL_STRING_XV_LAST_ATTRIBUTE)) {
        if (!h->xv) return NvCtrlMissingExtension;
        return NvCtrlXvGetStringAttribute(h, display_mask, attr, ptr);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetStringDisplayAttribute() */


ReturnStatus
NvCtrlSetStringDisplayAttribute(NvCtrlAttributeHandle *handle,
                                unsigned int display_mask,
                                int attr, char *ptr, int *ret)
{
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    if ((attr >= 0) && (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlSetStringAttribute(h, display_mask, attr,
                                                 ptr, ret);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlSetStringDisplayAttribute() */


ReturnStatus
NvCtrlGetBinaryAttribute(NvCtrlAttributeHandle *handle,
                         unsigned int display_mask, int attr,
                         unsigned char **data, int *len)
{
    NvCtrlAttributePrivateHandle *h;
    
    h = (NvCtrlAttributePrivateHandle *) handle;

    return NvCtrlNvControlGetBinaryAttribute(h, display_mask, attr, data, len);

} /* NvCtrlGetBinaryAttribute() */


ReturnStatus
NvCtrlStringOperation(NvCtrlAttributeHandle *handle,
                      unsigned int display_mask, int attr,
                      char *ptrIn, char **ptrOut)
{
    NvCtrlAttributePrivateHandle *h;
    
    h = (NvCtrlAttributePrivateHandle *) handle;
    
    if ((attr >= 0) && (attr <= NV_CTRL_STRING_OPERATION_LAST_ATTRIBUTE)) {
        if (!h->nv) return NvCtrlMissingExtension;
        return NvCtrlNvControlStringOperation(h, display_mask, attr, ptrIn,
                                              ptrOut);
    }

    return NvCtrlNoAttribute;

} /* NvCtrlStringOperation() */


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

    if ( h->glx ) {
        NvCtrlGlxAttributesClose(h);   
    }
    if ( h->xrandr ) {
        NvCtrlXrandrAttributesClose(h);   
    }
    if ( h->xv ) {
        NvCtrlXvAttributesClose(h);
    }

    free(h);
} /* NvCtrlAttributeClose() */


/*
 * NvCtrlGetMultisampleModeName() - lookup a string desribing the
 * NV-CONTROL constant.
*/

const char *NvCtrlGetMultisampleModeName(int multisample_mode)
{
    static const char *mode_names[] = {

        "Off",                 /* FSAA_MODE_NONE  */
        "2x (2xMS)",           /* FSAA_MODE_2x    */
        "2x Quincunx",         /* FSAA_MODE_2x_5t */
        "1.5 x 1.5",           /* FSAA_MODE_15x15 */
        "2 x 2 Supersampling", /* FSAA_MODE_2x2   */
        "4x (4xMS)",           /* FSAA_MODE_4x    */
        "4x, 9-tap Gaussian",  /* FSAA_MODE_4x_9t */
        "8x (4xMS, 4xCS)",     /* FSAA_MODE_8x    */
        "16x (4xMS, 12xCS)",   /* FSAA_MODE_16x   */
        "8x (4xSS, 2xMS)",     /* FSAA_MODE_8xS   */
        "8x (8xMS)",           /* FSAA_MODE_8xQ   */
        "16x (4xSS, 4xMS)",    /* FSAA_MODE_16xS  */
        "16x (8xMS, 8xCS)",    /* FSAA_MODE_16xQ  */
        "32x (4xSS, 8xMS)",    /* FSAA_MODE_32xS  */
        "32x (8xMS, 24xCS)",   /* FSAA_MODE_32x   */
        "64x (16xSS, 4xMS)",   /* FSAA_MODE_64xS  */
    };

    if ((multisample_mode < NV_CTRL_FSAA_MODE_NONE) ||
        (multisample_mode > NV_CTRL_FSAA_MODE_MAX)) {
        return "Unknown Multisampling";
    }

    return mode_names[multisample_mode];

} /* NvCtrlGetMultisampleModeName  */


ReturnStatus NvCtrlGetColorAttributes(NvCtrlAttributeHandle *handle,
                                      float contrast[3],
                                      float brightness[3],
                                      float gamma[3])
{
    NvCtrlAttributePrivateHandle *h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlVidModeGetColorAttributes(h, contrast, brightness, gamma);
    } else if (h->target_type == NV_CTRL_TARGET_TYPE_DISPLAY) {
        return NvCtrlXrandrGetColorAttributes(h, contrast, brightness, gamma);
    }

    return NvCtrlBadHandle;
}

ReturnStatus NvCtrlSetColorAttributes(NvCtrlAttributeHandle *handle,
                                      float c[3],
                                      float b[3],
                                      float g[3],
                                      unsigned int bitmask)
{
    ReturnStatus status;
    int val = 0;

    NvCtrlAttributePrivateHandle *h = (NvCtrlAttributePrivateHandle *) handle;

    status = NvCtrlGetAttribute(h, NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE, &val);

    if ((status != NvCtrlSuccess || !val) && 
        h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlVidModeSetColorAttributes(h, c, b, g, bitmask);
    } else if (status == NvCtrlSuccess && val &&
               h->target_type == NV_CTRL_TARGET_TYPE_DISPLAY) {
        return NvCtrlXrandrSetColorAttributes(h, c, b, g, bitmask);
    }

    return NvCtrlBadHandle;
}


ReturnStatus NvCtrlGetColorRamp(NvCtrlAttributeHandle *handle,
                                unsigned int channel,
                                uint16_t **lut,
                                int *n)
{
    NvCtrlAttributePrivateHandle *h = (NvCtrlAttributePrivateHandle *) handle;

    if (h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlVidModeGetColorRamp(h, channel, lut, n);
    } else if (h->target_type == NV_CTRL_TARGET_TYPE_DISPLAY) {
        return NvCtrlXrandrGetColorRamp(h, channel, lut, n);
    }

    return NvCtrlBadHandle;
}


/* helper functions private to the libXNVCtrlAttributes backend */

void NvCtrlInitGammaInputStruct(NvCtrlGammaInput *pGammaInput)
{
    int ch;

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {
        pGammaInput->brightness[ch] = BRIGHTNESS_DEFAULT;
        pGammaInput->contrast[ch]   = CONTRAST_DEFAULT;
        pGammaInput->gamma[ch]      = GAMMA_DEFAULT;
    }
}

/*
 * Compute the gammaRamp entry given its index, and the contrast,
 * brightness, and gamma.
 */
static unsigned short ComputeGammaRampVal(int gammaRampSize,
                                          int i,
                                          float contrast,
                                          float brightness,
                                          float gamma)
{
    double j, half, scale;
    int shift, val, num;

    num = gammaRampSize - 1;
    shift = 16 - (ffs(gammaRampSize) - 1);

    scale = (double) num / 3.0; /* how much brightness and contrast
                                   affect the value */
    j = (double) i;

    /* contrast */

    contrast *= scale;

    if (contrast > 0.0) {
        half = ((double) num / 2.0) - 1.0;
        j -= half;
        j *= half / (half - contrast);
        j += half;
    } else {
        half = (double) num / 2.0;
        j -= half;
        j *= (half + contrast) / half;
        j += half;
    }

    if (j < 0.0) j = 0.0;

    /* gamma */

    gamma = 1.0 / (double) gamma;

    if (gamma == 1.0) {
        val = (int) j;
    } else {
        val = (int) (pow (j / (double)num, gamma) * (double)num + 0.5);
    }

    /* brightness */

    brightness *= scale;

    val += (int)brightness;
    if (val > num) val = num;
    if (val < 0) val = 0;

    val <<= shift;
    return (unsigned short) val;
}

void NvCtrlUpdateGammaRamp(const NvCtrlGammaInput *pGammaInput,
                           int gammaRampSize,
                           unsigned short *gammaRamp[3],
                           unsigned int bitmask)
{
    int i, ch;

    /* update the requested channels within the gammaRamp */

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {

        /* only update requested channels */

        if ((bitmask & (1 << ch)) == 0) {
            continue;
        }

        for (i = 0; i < gammaRampSize; i++) {
            gammaRamp[ch][i] =
                ComputeGammaRampVal(gammaRampSize,
                                    i,
                                    pGammaInput->contrast[ch],
                                    pGammaInput->brightness[ch],
                                    pGammaInput->gamma[ch]);
        }
    }
}

void NvCtrlAssignGammaInput(NvCtrlGammaInput *pGammaInput,
                            const float inContrast[3],
                            const float inBrightness[3],
                            const float inGamma[3],
                            const unsigned int bitmask)
{
    int ch;

    /* clamp input, but only the input specified in the bitmask */

    for (ch = FIRST_COLOR_CHANNEL; ch <= LAST_COLOR_CHANNEL; ch++) {

        /* only update requested channels */

        if ((bitmask & (1 << ch)) == 0) {
            continue;
        }

        if (bitmask & CONTRAST_VALUE) {
            if (inContrast[ch] > CONTRAST_MAX) {
                pGammaInput->contrast[ch] = CONTRAST_MAX;
            } else if (inContrast[ch] < CONTRAST_MIN) {
                pGammaInput->contrast[ch] = CONTRAST_MIN;
            } else {
                pGammaInput->contrast[ch] = inContrast[ch];
            }
        }

        if (bitmask & BRIGHTNESS_VALUE) {
            if (inBrightness[ch] > BRIGHTNESS_MAX) {
                pGammaInput->brightness[ch] = BRIGHTNESS_MAX;
            } else if (inBrightness[ch] < BRIGHTNESS_MIN) {
                pGammaInput->brightness[ch] = BRIGHTNESS_MIN;
            } else {
                pGammaInput->brightness[ch] = inBrightness[ch];
            }
        }

        if (bitmask & GAMMA_VALUE) {
            if (inGamma[ch] > GAMMA_MAX) {
                pGammaInput->gamma[ch] = GAMMA_MAX;
            } else if (inGamma[ch] < GAMMA_MIN) {
                pGammaInput->gamma[ch] = GAMMA_MIN;
            } else {
                pGammaInput->gamma[ch] = inGamma[ch];
            }
        }
    }
}
