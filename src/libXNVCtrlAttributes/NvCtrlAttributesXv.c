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

static NvCtrlXvAttribute *getXvAttribute (NvCtrlAttributePrivateHandle *,
                                          XvPortID, const char *);

static Bool checkAdaptor(NvCtrlAttributePrivateHandle *h,
                         unsigned int attribute);

static unsigned int getXvPort(NvCtrlAttributePrivateHandle *h,
                              unsigned int attribute);

static NvCtrlXvAttribute *getXvAttributePtr(NvCtrlAttributePrivateHandle *h,
                                            unsigned int attribute);

typedef struct __libXvInfoRec {

    /* libXv.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */

    /* libXv functions used */
    int           (* XvGetPortAttribute)    (Display *, XvPortID, Atom, int *);
    int           (* XvSetPortAttribute)    (Display *, XvPortID, Atom, int);
    XvAttribute * (* XvQueryPortAttributes) (Display *, XvPortID, int *);
    int           (* XvQueryExtension)      (Display *, unsigned int *,
                                             unsigned int *, unsigned int *,
                                             unsigned int *, unsigned int *);
    int           (* XvQueryAdaptors)       (Display *, Window, unsigned int *,
                                             XvAdaptorInfo **);
} __libXvInfo;

static __libXvInfo *__libXv = NULL;



/*
 * Opens libXv for usage
 */

static Bool open_libxv(void)
{
    const char *error_str = NULL;


    /* Initialize bookkeeping structure */
    if ( !__libXv ) {
        __libXv = calloc(1, sizeof(__libXvInfo));
        if ( !__libXv ) {
            error_str = "Could not allocate memory.";
            goto fail;
        }
    }
    

    /* Library was already opened */
    if ( __libXv->handle ) {
        __libXv->ref_count++;
        return True;
    }


    /* We are the first to open the library */
    __libXv->handle = dlopen("libXv.so.1", RTLD_LAZY);
    if ( __libXv->handle == NULL ) {
        error_str = dlerror();
        goto fail;
    }


    /* Resolve Xv functions */
    __libXv->XvQueryExtension =
        NV_DLSYM(__libXv->handle, "XvQueryExtension");
    if ((error_str = dlerror()) != NULL) goto fail;
    
    __libXv->XvQueryAdaptors =
        NV_DLSYM(__libXv->handle, "XvQueryAdaptors");
    if ((error_str = dlerror()) != NULL) goto fail;
    
    __libXv->XvGetPortAttribute =
        NV_DLSYM(__libXv->handle, "XvGetPortAttribute");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXv->XvSetPortAttribute =
        NV_DLSYM(__libXv->handle, "XvSetPortAttribute");
    if ((error_str = dlerror()) != NULL) goto fail;
    
    __libXv->XvQueryPortAttributes =
        NV_DLSYM(__libXv->handle, "XvQueryPortAttributes");
    if ((error_str = dlerror()) != NULL) goto fail;


    /* Up the ref count */
    __libXv->ref_count++;

    return True;


 fail:
    if ( error_str ) {
        nv_error_msg("libXv setup error : %s\n", error_str);
    }
    if ( __libXv ) {
        if ( __libXv->handle ) {
            dlclose(__libXv->handle);
            __libXv->handle = NULL;
        }
        free(__libXv);
        __libXv = NULL;
    }
    return False;
    
} /* open_libxv() */



/*
 * Closes libXv when it is no longer used.
 */

static void close_libxv(void)
{
    if ( __libXv && __libXv->handle && __libXv->ref_count ) {
        __libXv->ref_count--;
        if ( __libXv->ref_count == 0 ) {
            dlclose(__libXv->handle);
            __libXv->handle = NULL;
            free(__libXv);
            __libXv = NULL;
        }
    }
} /* close_libxv() */



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
    unsigned int req, event_base, error_base, nadaptors;
    int ret, i;
    const char *error_str = NULL;
    const char *warn_str = NULL;
    

    /* Check parameters */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        goto fail;
    }


    /* Open libXv.so.1 */
    if ( !open_libxv() ) {
        warn_str = "Failed to open libXv.so.1: this library "
            "is not present in your system or is not in your "
            "LD_LIBRARY_PATH.";
        goto fail;
    }


    /* Allocate the attributes structure */
    xv = calloc(1, sizeof(NvCtrlXvAttributes));
    if ( xv == NULL ) {
        error_str = "Out of memory.";
        goto fail;
    }


    /* Verify server support of Xv extension */
    ret = __libXv->XvQueryExtension(h->dpy, &(xv->major_version), &(xv->minor_version), &req,
                                    &event_base, &error_base);
    if (ret != Success) goto fail;
    
    /* XXX do we have a minimum Xv version? */


    /* Get the list of adaptors */
    ret = __libXv->XvQueryAdaptors(h->dpy, RootWindow(h->dpy, h->target_id),
                                   &nadaptors, &ainfo);

    if (ret != Success || !nadaptors || !ainfo) goto fail;
    
    for (i = 0; i < nadaptors; i++) {
        
        if ((strcmp(ainfo[i].name, "NV17 Video Overlay") == 0) ||
            (strcmp(ainfo[i].name, "NV10 Video Overlay") == 0)) {
        
            NvCtrlXvOverlayAttributes *attrs;
            
            attrs = malloc(sizeof(NvCtrlXvOverlayAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }
        
            attrs->port = ainfo[i].base_id;
            attrs->saturation = getXvAttribute(h, attrs->port,
                                               "XV_SATURATION");
            attrs->contrast   = getXvAttribute(h, attrs->port,
                                               "XV_CONTRAST");
            attrs->brightness = getXvAttribute(h, attrs->port,
                                               "XV_BRIGHTNESS");
            attrs->hue        = getXvAttribute(h, attrs->port,
                                               "XV_HUE");
            attrs->defaults   = getXvAttribute(h, attrs->port,
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
            
            attrs = malloc(sizeof(NvCtrlXvTextureAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }

            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(h, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->contrast       = getXvAttribute(h, attrs->port,
                                                   "XV_CONTRAST");
            attrs->brightness     = getXvAttribute(h, attrs->port,
                                                   "XV_BRIGHTNESS");
            attrs->saturation     = getXvAttribute(h, attrs->port,
                                                   "XV_SATURATION");
            attrs->hue            = getXvAttribute(h, attrs->port,
                                                   "XV_HUE");
            attrs->defaults       = getXvAttribute(h, attrs->port,
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

            attrs = malloc(sizeof(NvCtrlXvBlitterAttributes));
            if ( !attrs ) {
                error_str = "Out of memory.";
                goto fail;
            }

            attrs->port = ainfo[i].base_id;
            attrs->sync_to_vblank = getXvAttribute(h, attrs->port,
                                                   "XV_SYNC_TO_VBLANK");
            attrs->defaults       = getXvAttribute(h, attrs->port,
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
        if (xv->overlay) {
            free(xv->overlay);
        }
        if (xv->texture) {
            free(xv->texture);
        }
        if (xv->blitter) {
            free(xv->blitter);
        }
        free(xv);
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
    
    if (__libXv->XvGetPortAttribute(h->dpy, port, a->atom, val) != Success) {
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
    
    if (__libXv->XvSetPortAttribute(h->dpy, port, a->atom, val) != Success) {
        return NvCtrlError;
    }
    
    XFlush(h->dpy);

    return NvCtrlSuccess; 
    
} /* NvCtrlXvSetAttribute() */


/*
 * Get Xv String Attribute values 
 */

ReturnStatus
NvCtrlXvGetStringAttribute(NvCtrlAttributePrivateHandle *h,
                           unsigned int display_mask,
                           int attr, char **ptr)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }
    if ( !h->xv || !__libXv ) {
        return NvCtrlMissingExtension;
    }

    /* Get Xv major & minor versions */
    if (attr == NV_CTRL_STRING_XV_VERSION) {
        char str[16];
        sprintf(str, "%d.%d", h->xv->major_version, h->xv->minor_version);
        *ptr = strdup(str);
        return NvCtrlSuccess;
    }

    return NvCtrlNoAttribute;

} /* NvCtrlXvGetStringAttribute() */


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

static NvCtrlXvAttribute *getXvAttribute(NvCtrlAttributePrivateHandle *h,
                                         XvPortID port,
                                         const char *name)
{
    NvCtrlXvAttribute *attr = NULL;
    XvAttribute *attributes = NULL;
    int i, n;
    
    attributes = __libXv->XvQueryPortAttributes(h->dpy, port, &n);

    if (!attributes || !n) goto failed;
    
    for (i = 0; i < n; i++) {
        if (strcmp(attributes[i].name, name) != 0) continue;

        attr = malloc(sizeof(NvCtrlXvAttribute));
        attr->range.type = ATTRIBUTE_TYPE_RANGE;
        attr->range.u.range.min = attributes[i].min_value;
        attr->range.u.range.max = attributes[i].max_value;
        attr->atom = XInternAtom(h->dpy, name, True);
        if (attr->atom == None) goto failed;
        
        if (! (attributes[i].flags & XvSettable)) goto failed;
        
        attr->range.permissions = ATTRIBUTE_TYPE_WRITE;
        
        if (attributes[i].flags & XvGettable) {
            attr->range.permissions |= ATTRIBUTE_TYPE_READ;
        }
        
        /* All Xv attributes are controlled with X screen target type */

        attr->range.permissions |= ATTRIBUTE_TYPE_X_SCREEN;

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
    case NV_CTRL_ATTR_XV_TEXTURE_CONTRAST:
    case NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS:
    case NV_CTRL_ATTR_XV_TEXTURE_HUE:
    case NV_CTRL_ATTR_XV_TEXTURE_SATURATION:
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
    case NV_CTRL_ATTR_XV_TEXTURE_CONTRAST:
    case NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS:
    case NV_CTRL_ATTR_XV_TEXTURE_HUE:
    case NV_CTRL_ATTR_XV_TEXTURE_SATURATION:
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
        
    case NV_CTRL_ATTR_XV_TEXTURE_CONTRAST:
        return h->xv->texture->contrast;
    
    case NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS:
        return h->xv->texture->brightness;

    case NV_CTRL_ATTR_XV_TEXTURE_HUE:
        return h->xv->texture->hue;
    
    case NV_CTRL_ATTR_XV_TEXTURE_SATURATION:
        return h->xv->texture->saturation;

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
 */

void
NvCtrlXvAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if (!h || !h->xv) {
        return;
    }

    close_libxv();

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
