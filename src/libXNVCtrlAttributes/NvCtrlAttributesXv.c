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
#include <dlfcn.h>

#include "msg.h"

static NvCtrlXvAttribute *getXvAttribute (NvCtrlXvAttributes *,
                                          XvPortID, const char *);

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

NvCtrlXvAttributes * NvCtrlInitXvAttributes(NvCtrlAttributePrivateHandle *h)
{
    NvCtrlXvAttributes *xv = NULL;
    XvAdaptorInfo *ainfo;
    unsigned int ver, rev, req, event, error, nadaptors;
    int ret, i;
    Display *dpy = h->dpy;
    const char *error_str = NULL;
    const char *warn_str = NULL;
    

    /* Check parameters */
    if (h == NULL || h->dpy == NULL) {
        goto fail;
    }


    /* Allocate the attributes structure */
    xv = (NvCtrlXvAttributes *)
        calloc(1, sizeof (NvCtrlXvAttributes));
    if ( xv == NULL ) {
        error_str = "Out of memory.";
        goto fail;
    }


    /* Link the libXv lib */
    xv->libXv = dlopen("libXv.so.1", RTLD_LAZY);

    if (xv->libXv == NULL) {
        warn_str = "Failed to open libXv.so.1: this library "
            "is not present in your system or is not in your "
            "LD_LIBRARY_PATH.";
        goto fail;
    }


    /* Resolve Xv functions needed */
    xv->XvQueryExtension = NV_DLSYM(xv->libXv, "XvQueryExtension");
    if ((error_str = dlerror()) != NULL) goto fail;
    
    xv->XvQueryAdaptors = NV_DLSYM(xv->libXv, "XvQueryAdaptors");
    if ((error_str = dlerror()) != NULL) goto fail;
    
    xv->XvGetPortAttribute = NV_DLSYM(xv->libXv, "XvGetPortAttribute");
    if ((error_str = dlerror()) != NULL) goto fail;

    xv->XvSetPortAttribute = NV_DLSYM(xv->libXv, "XvSetPortAttribute");
    if ((error_str = dlerror()) != NULL) goto fail;

    xv->XvQueryPortAttributes = NV_DLSYM(xv->libXv, "XvQueryPortAttributes");
    if ((error_str = dlerror()) != NULL) goto fail;


    /* Duplicate the display connection and verify Xv extension exists */
    xv->dpy = XOpenDisplay( XDisplayString(h->dpy) );
    if ( xv->dpy == NULL ) {
        goto fail;
    }
    ret = xv->XvQueryExtension(xv->dpy, &ver, &rev, &req, &event, &error);
    if (ret != Success) goto fail;
    
    /* XXX do we have a minimum Xv version? */


    /* Get the list of adaptors */
    ret = xv->XvQueryAdaptors(xv->dpy, RootWindow(dpy, h->screen),
                              &nadaptors, &ainfo);

    if (ret != Success || !nadaptors || !ainfo) goto fail;
    
    for (i = 0; i < nadaptors; i++) {
        
        if ((strcmp(ainfo[i].name, "NV17 Video Overlay") == 0) ||
            (strcmp(ainfo[i].name, "NV10 Video Overlay") == 0)) {
        
            NvCtrlXvOverlayAttributes *attrs;
            
            attrs = (NvCtrlXvOverlayAttributes *)
                malloc(sizeof(NvCtrlXvOverlayAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }
        
            attrs->port = ainfo[i].base_id;
            attrs->saturation = getXvAttribute(xv, attrs->port,
                                               "XV_SATURATION");
            attrs->contrast   = getXvAttribute(xv, attrs->port,
                                               "XV_CONTRAST");
            attrs->brightness = getXvAttribute(xv, attrs->port,
                                               "XV_BRIGHTNESS");
            attrs->hue        = getXvAttribute(xv, attrs->port,
                                               "XV_HUE");
            attrs->defaults   = getXvAttribute(xv, attrs->port,
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
                xv->overlay = attrs;
            }
        }

        if (strcmp(ainfo[i].name, "NV17 Video Texture") == 0) {
            
            NvCtrlXvTextureAttributes *attrs;

            attrs = (NvCtrlXvTextureAttributes *)
                malloc(sizeof(NvCtrlXvTextureAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }

            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(xv, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->defaults       = getXvAttribute(xv, attrs->port,
                                                   "XV_SET_DEFAULTS");
            if (!attrs->sync_to_vblank ||
                !attrs->defaults) {
                
                if (attrs->sync_to_vblank) free(attrs->sync_to_vblank);
                if (attrs->defaults)       free(attrs->defaults);
                
                free(attrs);
                attrs = NULL;
                
            } else {
                xv->texture = attrs;
            }
        }

        if (strcmp(ainfo[i].name, "NV05 Video Blitter") == 0) {
            
            NvCtrlXvBlitterAttributes *attrs;

            attrs = (NvCtrlXvBlitterAttributes *)
                malloc(sizeof(NvCtrlXvBlitterAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }

            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(xv, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->defaults       = getXvAttribute(xv, attrs->port,
                                                   "XV_SET_DEFAULTS");
            if (!attrs->sync_to_vblank ||
                !attrs->defaults) {
                
                if (attrs->sync_to_vblank) free(attrs->sync_to_vblank);
                if (attrs->defaults)       free(attrs->defaults);
                
                free(attrs);
                attrs = NULL;
                
            } else {
                xv->blitter = attrs;
            }
        }
    }

    return xv;


    /* Handle failures */
 fail:
    if (error_str) {
        nv_error_msg("libXv setup error: %s\n", error_str);
    }
    if (warn_str) {
        nv_warning_msg("libXv setup warning: %s\n", warn_str);
    }
    if (xv != NULL) {
        if ( xv->dpy != NULL ) {
            XCloseDisplay(xv->dpy);
        }
        if (xv->libXv) {
            dlclose(xv->libXv);
        }
        free (xv);
    }

    return NULL;

} /* NvCtrlInitXvAttributes() */

                      


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
    
    if (h->xv->XvGetPortAttribute(h->xv->dpy, port, a->atom, val) != Success) {
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
    
    if (h->xv->XvSetPortAttribute(h->xv->dpy, port, a->atom, val) != Success) {
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

static NvCtrlXvAttribute *getXvAttribute(NvCtrlXvAttributes *xv,
                                         XvPortID port,
                                         const char *name)
{
    NvCtrlXvAttribute *attr = NULL;
    XvAttribute *attributes = NULL;
    unsigned int n;
    int i;
    
    attributes = xv->XvQueryPortAttributes(xv->dpy, port, &n);

    if (!attributes || !n) goto failed;
    
    for (i = 0; i < n; i++) {
        if (strcmp(attributes[i].name, name) != 0) continue;

        attr = (NvCtrlXvAttribute *) malloc(sizeof (NvCtrlXvAttribute));
        attr->range.type = ATTRIBUTE_TYPE_RANGE;
        attr->range.u.range.min = attributes[i].min_value;
        attr->range.u.range.max = attributes[i].max_value;
        attr->atom = XInternAtom(xv->dpy, name, True);
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
        if (h && h->xv && h->xv->overlay) return True;
        else return False;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        if (h && h->xv && h->xv->texture) return True;
        else return False;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        if (h && h->xv && h->xv->blitter) return True;
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
        return h->xv->overlay->port;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        return h->xv->texture->port;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        return h->xv->blitter->port;
        
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
        return h->xv->overlay->saturation;
        
    case NV_CTRL_ATTR_XV_OVERLAY_CONTRAST:
        return h->xv->overlay->contrast;
        
    case NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS:
        return h->xv->overlay->brightness;
        
    case NV_CTRL_ATTR_XV_OVERLAY_HUE:
        return h->xv->overlay->hue;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK:
        return h->xv->texture->sync_to_vblank;
        
    case NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK:
        return h->xv->blitter->sync_to_vblank;
        
    case NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS:
        return h->xv->overlay->defaults;
        
    case NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS:
        return h->xv->texture->defaults;

    case NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS:
        return h->xv->blitter->defaults;

    default:
        return NULL;
    }
} /* getXvAttributePtr() */



/*
 * Frees and relinquishes any resource used by the Xv Attributes
 *
 */

void
NvCtrlXvAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if (h == NULL || h->xv == NULL) {
        return;
    }

    if (h->xv->dpy) {
        XCloseDisplay(h->xv->dpy);
    }

    if (h->xv->libXv) {
        dlclose(h->xv->libXv);
    }

    if (h->xv->overlay) {
        free(h->xv->overlay);
    }
    if (h->xv->texture) {
        free(h->xv->texture);
    }
    if (h->xv->blitter) {
        free(h->xv->blitter);
    }

    free(h->xv);
    h->xv = NULL;

} /* NvCtrlXvAttributesClose() */
