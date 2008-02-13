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

#include <stdlib.h>

/*
 * NvCtrlInitNvControlAttributes() - check for the NV-CONTROL
 * extension and make sure we have an adequate version.  Returns a
 * malloced and initialized NvCtrlNvControlAttributes structure if
 * successful, or NULL otherwise.
 */

NvCtrlNvControlAttributes *
NvCtrlInitNvControlAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlNvControlAttributes *nv;
    int ret, major, minor, event, error;
    
    ret = XNVCTRLQueryExtension (h->dpy, &event, &error);
    if (ret != True) {
        nv_error_msg("NV-CONTROL extension not found on this Display.");
        return NULL;
    }
    
    ret = XNVCTRLQueryVersion (h->dpy, &major, &minor);
    if (ret != True) {
        nv_error_msg("Failed to query NV-CONTROL extension version.");
        return NULL;
    }

    if (major < NV_MINMAJOR || (major == NV_MINMAJOR && minor < NV_MINMINOR)) {
        nv_error_msg("NV-CONTROL extension version %d.%d is too old; "
                     "the minimimum required version is %d.%d.",
                     major, minor, NV_MINMAJOR, NV_MINMINOR);
        return NULL;
    }
    
    ret = XNVCTRLIsNvScreen (h->dpy, h->screen);
    if (ret != True) {
        nv_warning_msg("NV-CONTROL extension not present on screen %d "
                       "of this Display.", h->screen);
        return NULL;
    }
    
    nv = (NvCtrlNvControlAttributes *)
        calloc(1, sizeof (NvCtrlNvControlAttributes));

    ret = XNVCtrlSelectNotify(h->dpy, h->screen,
                              ATTRIBUTE_CHANGED_EVENT, True);
    if (ret != True) {
        nv_warning_msg("Unable to select attribute changed NV-CONTROL "
                       "events.");
    }

    nv->event_base = event;
    nv->error_base = error;
    nv->major_version = major;
    nv->minor_version = minor;

    return (nv);

} /* NvCtrlInitNvControlAttributes() */


ReturnStatus NvCtrlNvControlGetAttribute (NvCtrlAttributePrivateHandle *h,
                                          unsigned int display_mask,
                                          int attr, int *val)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        if (XNVCTRLQueryAttribute (h->dpy, h->screen, display_mask,
                                   attr, val)) {
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }
     
    return NvCtrlNoAttribute;
    
} /* NvCtrlGetAttribute() */


ReturnStatus NvCtrlNvControlSetAttribute (NvCtrlAttributePrivateHandle *h,
                                          unsigned int display_mask,
                                          int attr, int val)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        XNVCTRLSetAttribute (h->dpy, h->screen, display_mask, attr, val);
        XFlush (h->dpy);
        return NvCtrlSuccess;
    }
      
    return NvCtrlNoAttribute;

} /* NvCtrlGetAttribute() */


ReturnStatus NvCtrlNvControlGetValidAttributeValues
                              (NvCtrlAttributePrivateHandle *h,
                               unsigned int display_mask,
                               int attr, NVCTRLAttributeValidValuesRec *val)
{
    if (attr <= NV_CTRL_LAST_ATTRIBUTE) {
        if (XNVCTRLQueryValidAttributeValues (h->dpy, h->screen,
                                              display_mask, attr, val)) {
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlNvControlGetValidAttributeValues() */


ReturnStatus
NvCtrlNvControlGetStringAttribute (NvCtrlAttributePrivateHandle *h,
                                   unsigned int display_mask,
                                   int attr, char **ptr)
{
    if (attr <= NV_CTRL_STRING_LAST_ATTRIBUTE) {
        if (XNVCTRLQueryStringAttribute (h->dpy, h->screen,
                                         display_mask, attr, ptr)) {
            return NvCtrlSuccess;
        } else {
            return NvCtrlAttributeNotAvailable;
        }
    }

    return NvCtrlNoAttribute;

} /* NvCtrlGetStringAttribute() */


ReturnStatus
NvCtrlNvControlGetBinaryAttribute(NvCtrlAttributePrivateHandle *h,
                                  unsigned int display_mask, int attr,
                                  unsigned char **data, int *len)
{
    Bool bret;
    
    if (!h->nv) return NvCtrlMissingExtension;
    
    /* the X_nvCtrlQueryBinaryData opcode was added in 1.7 */
    
    if ((h->nv->major_version < 1) ||
        ((h->nv->major_version == 1) && (h->nv->minor_version < 7))) {
        return NvCtrlNoAttribute;
    }
    
    bret = XNVCTRLQueryBinaryData(h->dpy, h->screen, display_mask,
                                  attr, data, len);
    
    if (!bret) {
        return NvCtrlError;
    } else {
        return NvCtrlSuccess;
    }
    
} /* NvCtrlNvControlGetBinaryAttribute() */
