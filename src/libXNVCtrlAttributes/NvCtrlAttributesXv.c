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

#include <X11/extensions/Xvlib.h>

#include <stdlib.h>
#include <string.h>

static NvCtrlXvAttribute *getXvAttribute (Display *, XvPortID, const char *);

static Bool checkAdaptor(NvCtrlAttributePrivateHandle *h,
                         unsigned int attribute);

static unsigned int getXvPort(NvCtrlAttributePrivateHandle *h,
                              unsigned int attribute);

static NvCtrlXvAttribute *getXvAttributePtr(NvCtrlAttributePrivateHandle *h,
                                            unsigned int attribute);


/*
 * NvCtrlInitXvAttributes() - scan through the list of Xv adaptors on
 * the given Display for the video overlay adaptor.  Get the
 * attributes "XV_CONTRAST", "XV_BRIGHTNESS", "XV_SATURATION",
 * "XV_HUE" and "XV_SET_DEFAULTS".  Returns a malloced and initialized
 * NvCtrlXvOverlayAttributes struct if an nv10 or nv17 video overlay
 * adaptor was found with all the needed attributes, or NULL
 * otherwise.
 */

void NvCtrlInitXvOverlayAttributes(NvCtrlAttributePrivateHandle *h)
{
    XvAdaptorInfo *ainfo;
    unsigned int ver, rev, req, event, error, nadaptors;
    int ret, i;
    Display *dpy = h->dpy;
    
    /* clear the adaptor pointers */

    h->xv_overlay = NULL;
    h->xv_texture = NULL;
    h->xv_blitter = NULL;

    ret = XvQueryExtension(dpy, &ver, &rev, &req, &event, &error);
    if (ret != Success) return;
    
    /* XXX do we have a minimum Xv version? */
    
    ret = XvQueryAdaptors(dpy, RootWindow(dpy, h->screen),
                          &nadaptors, &ainfo);

    if (ret != Success || !nadaptors || !ainfo) return;
    
    for (i = 0; i < nadaptors; i++) {
        
        if ((strcmp(ainfo[i].name, "NV17 Video Overlay") == 0) ||
            (strcmp(ainfo[i].name, "NV10 Video Overlay") == 0)) {
        
            NvCtrlXvOverlayAttributes *attrs;
            
            attrs = (NvCtrlXvOverlayAttributes *)
                malloc(sizeof(NvCtrlXvOverlayAttributes));
        
            attrs->port = ainfo[i].base_id;
            attrs->saturation = getXvAttribute(dpy, attrs->port,
                                               "XV_SATURATION");
            attrs->contrast   = getXvAttribute(dpy, attrs->port,
                                               "XV_CONTRAST");
            attrs->brightness = getXvAttribute(dpy, attrs->port,
                                               "XV_BRIGHTNESS");
            attrs->hue        = getXvAttribute(dpy, attrs->port,
                                               "XV_HUE");
            attrs->defaults   = getXvAttribute(dpy, attrs->port,
                                               "XV_SET_DEFAULTS");
        
            if (!attrs->saturation ||
                !attrs->contrast ||
                !attrs->brightness ||
                !attrs->hue ||
                !attrs->defaults) {

                if (attrs->saturation) free(attrs->saturation);
                if (attrs->contrast)   free(attrs->contrast);
                if (attrs->brightness) free(attrs->brightness);
                if (attrs->hue)        free(attrs->hue);
                if (attrs->defaults)   free(attrs->defaults);
                
                free(attrs);
                attrs = NULL;
                
            } else {
                h->xv_overlay = attrs;
            }
        }

        if (strcmp(ainfo[i].name, "NV17 Video Texture") == 0) {
            
            NvCtrlXvTextureAttributes *attrs;

            attrs = (NvCtrlXvTextureAttributes *)
                malloc(sizeof(NvCtrlXvTextureAttributes));
            
            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(dpy, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->defaults       = getXvAttribute(dpy, attrs->port,
                                                   "XV_SET_DEFAULTS");
            if (!attrs->sync_to_vblank ||
                !attrs->defaults) {
                
                if (attrs->sync_to_vblank) free(attrs->sync_to_vblank);
                if (attrs->defaults)       free(attrs->defaults);
                
                free(attrs);
                attrs = NULL;
                
            } else {
                h->xv_texture = attrs;
            }
        }

        if (strcmp(ainfo[i].name, "NV05 Video Blitter") == 0) {
            
            NvCtrlXvBlitterAttributes *attrs;

            attrs = (NvCtrlXvBlitterAttributes *)
                malloc(sizeof(NvCtrlXvBlitterAttributes));
            
            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(dpy, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->defaults       = getXvAttribute(dpy, attrs->port,
                                                   "XV_SET_DEFAULTS");
            if (!attrs->sync_to_vblank ||
                !attrs->defaults) {
                
                if (attrs->sync_to_vblank) free(attrs->sync_to_vblank);
                if (attrs->defaults)       free(attrs->defaults);
                
                free(attrs);
                attrs = NULL;
                
            } else {
                h->xv_blitter = attrs;
            }
        }
    }
} /* NvCtrlInitXvOverlayAttributes() */

                      


ReturnStatus NvCtrlXvGetAttribute(NvCtrlAttributePrivateHandle *h,
                                  int attr, int *val)
{
    NvCtrlXvAttribute *a = NULL;
    unsigned int port;

    /* first, check that we have the necessary adaptor */
    
    if (!checkAdaptor(h, attr)) return NvCtrlAttributeNotAvailable;

    /* get the attribute pointer */

    a = getXvAttributePtr(h, attr);
    if (!a) return NvCtrlAttributeNotAvailable;
    
    if (! (a->range.permissions & ATTRIBUTE_TYPE_READ))
        return NvCtrlWriteOnlyAttribute;
    
    /* get the port */
    
    port = getXvPort(h, attr);
    if (!port) return NvCtrlAttributeNotAvailable;

    /* finally, query the value */
    
    if (XvGetPortAttribute(h->dpy, port, a->atom, val) != Success) {
        return NvCtrlError;
    }
    
    return NvCtrlSuccess;
    
} /* NvCtrlXvGetAttribute() */



ReturnStatus NvCtrlXvSetAttribute(NvCtrlAttributePrivateHandle *h,
                                  int attr, int val)
{
    NvCtrlXvAttribute *a = NULL;
    unsigned int port;

    /* first, check that we have the necessary adaptor */
    
    if (!checkAdaptor(h, attr)) return NvCtrlAttributeNotAvailable;
    
    /* get the attribute pointer */
    
    a = getXvAttributePtr(h, attr);
    if (!a) return NvCtrlAttributeNotAvailable;

    /* get the port */
    
    port = getXvPort(h, attr);
    if (!port) return NvCtrlAttributeNotAvailable;
    
    /* finally, set the value */
    
    if (XvSetPortAttribute(h->dpy, port, a->atom, val) != Success) {
        return NvCtrlError;
    }
    
    XFlush(h->dpy);

    return NvCtrlSuccess; 
    
} /* NvCtrlXvSetAttribute() */



ReturnStatus
NvCtrlXvGetValidAttributeValues(NvCtrlAttributePrivateHandle *h, int attr,
                                NVCTRLAttributeValidValuesRec *val)
{
    NvCtrlXvAttribute *a;

    /* first, check that we have the necessary adaptor */
    
    if (!checkAdaptor(h, attr)) return NvCtrlAttributeNotAvailable;
    
    /* get the attribute pointer */
    
    a = getXvAttributePtr(h, attr);
    if (!a) return NvCtrlAttributeNotAvailable;
    
    /* XXX should we force _SET_DEFAULTS to bool? */
    
    /* assign range and return */
    
    *val = a->range;

    return NvCtrlSuccess;

} /* NvCtrlXvGetValidAttributeValues() */



/*
 * getXvAttribute() - loop through the attributes for the given port,
 * looking for the attribute specified by 'name'.  If the named
 * attribute is found, get its minimum and maximum values, and (if
 * possible) get its current value.  Return a malloced and initialized
 * NvCtrlXvAttribute struct if successful, otherwise return NULL.
 */

static NvCtrlXvAttribute *getXvAttribute(Display *dpy, XvPortID port,
                                         const char *name)
{
    NvCtrlXvAttribute *attr = NULL;
    XvAttribute *attributes = NULL;
    int i, n;
    
    attributes = XvQueryPortAttributes(dpy, port, &n);

    if (!attributes || !n) goto failed;
    
    for (i = 0; i < n; i++) {
        if (strcmp(attributes[i].name, name) != 0) continue;

        attr = (NvCtrlXvAttribute *) malloc(sizeof (NvCtrlXvAttribute));
        attr->range.type = ATTRIBUTE_TYPE_RANGE;
        attr->range.u.range.min = attributes[i].min_value;
        attr->range.u.range.max = attributes[i].max_value;
        attr->atom = XInternAtom(dpy, name, True);
        if (attr->atom == None) goto failed;
        
        if (! (attributes[i].flags & XvSettable)) goto failed;
        
        attr->range.permissions = ATTRIBUTE_TYPE_WRITE;
        
        if (attributes[i].flags & XvGettable) {
            attr->range.permissions |= ATTRIBUTE_TYPE_READ;
        }
        
        break;
    }

    if (attributes) XFree(attributes);

    return attr;

 failed:
    if (attr) free(attr);
    if (attributes) XFree(attributes);
    return NULL;

} /* getXvAttribute() */



/*
 * checkAdaptor() - check that the handle has data for the adaptor
 * needed for the specified attribute.
 */

static Bool checkAdaptor(NvCtrlAttributePrivateHandle *h,
                         unsigned int attribute)
{
    switch(attribute) {
        
    case NV_CTRL_ATTR_XV_OVERLAY_SATURATION:
    case NV_CTRL_ATTR_XV_OVERLAY_CONTRAST:
    case NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS:
    case NV_CTRL_ATTR_XV_OVERLAY_HUE:
    case NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS:
        if (h && h->xv_overlay) return True;
        else return False;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        if (h && h->xv_texture) return True;
        else return False;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        if (h && h->xv_blitter) return True;
        else return False;
        
    default:
        return False;
    }
} /* checkAdaptor() */



/*
 * look up the port number for the adaptor for the attribute
 * specified; returns 0 if the attribute/adaptor is not available.
 */

static unsigned int getXvPort(NvCtrlAttributePrivateHandle *h,
                              unsigned int attribute)
{
    if (!checkAdaptor(h, attribute)) return 0;

    switch(attribute) {
        
    case NV_CTRL_ATTR_XV_OVERLAY_SATURATION:
    case NV_CTRL_ATTR_XV_OVERLAY_CONTRAST:
    case NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS:
    case NV_CTRL_ATTR_XV_OVERLAY_HUE:
    case NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS:
        return h->xv_overlay->port;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        return h->xv_texture->port;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        return h->xv_blitter->port;
        
    default:
        return 0;
    }
} /* getXvPort() */



/*
 * getXvAttributePtr() - return the NvCtrlXvAttribute pointer for the
 * specified attribute; or NULL if that attribute is not available.
 */

static NvCtrlXvAttribute *getXvAttributePtr(NvCtrlAttributePrivateHandle *h,
                                            unsigned int attribute)
{
    if (!checkAdaptor(h, attribute)) return NULL;
    
    switch (attribute) {
        
    case NV_CTRL_ATTR_XV_OVERLAY_SATURATION:
        return h->xv_overlay->saturation;
        
    case NV_CTRL_ATTR_XV_OVERLAY_CONTRAST:
        return h->xv_overlay->contrast;
        
    case NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS:
        return h->xv_overlay->brightness;
        
    case NV_CTRL_ATTR_XV_OVERLAY_HUE:
        return h->xv_overlay->hue;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
        return h->xv_texture->sync_to_vblank;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
        return h->xv_blitter->sync_to_vblank;
        
    case NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS:
        return h->xv_overlay->defaults;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        return h->xv_texture->defaults;

    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        return h->xv_blitter->defaults;

    default:
        return NULL;
    }
} /* getXvAttributePtr() */
