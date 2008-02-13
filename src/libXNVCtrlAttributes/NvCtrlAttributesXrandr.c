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
 *  XRandR backend
 *
 *    Currently only rotation is supported.
 *
 */

#include <sys/utsname.h>

#include <stdlib.h> /* 64 bit malloc */

#include <dlfcn.h> /* To dynamically load libXrandr.so.2 */
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h> /* Xrandr */

#define NV_DLSYM(handle, symbol) ({ dlerror(); dlsym(handle, symbol); })

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"
#include "NVCtrlLib.h"

#include "msg.h"
#include "parse.h"



/******************************************************************************
 *
 *   Sets the requested rotation orientation.
 *
 ****/

static ReturnStatus
set_rotation(NvCtrlAttributePrivateHandle *h, Rotation rotation)
{
    XRRScreenConfiguration *sc;
    Rotation cur_rotation;
    SizeID cur_size;
    Status status;


    /* Check orientation is supported */
    if ( !( h->xrandr->rotations & rotation ) ) {
        return NvCtrlBadArgument;
    }

    /* Get current size & orientation */
    sc = h->xrandr->XRRGetScreenInfo(h->dpy, DefaultRootWindow(h->dpy));
    if ( !sc ) {
        return NvCtrlError;
    }
    cur_size = h->xrandr->XRRConfigCurrentConfiguration(sc, &cur_rotation);

    status = h->xrandr->XRRSetScreenConfig (h->dpy, sc,
                                            DefaultRootWindow(h->dpy),
                                            cur_size, rotation, CurrentTime);
    h->xrandr->XRRFreeScreenConfigInfo(sc);

    if ( status != Success ) {
        return NvCtrlError;
    }

    return NvCtrlSuccess;

} /* set_rotation() */



/******************************************************************************
 *
 * Initializes the NvCtrlXrandrAttributes Extension by linking the
 * libXrandr.so.2 library and resolving functions used.
 *
 ****/

NvCtrlXrandrAttributes *
NvCtrlInitXrandrAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlXrandrAttributes *xrandr = NULL;
    const char *error = NULL; /* libXrandr error string */
    Bool ret;
    Rotation rotation;
    XRRScreenSize *sizes;


    /* Check parameter */
    if ( h == NULL || h->dpy == NULL ) {
        goto fail;
    }


    /* Allocate attributes structure */
    xrandr = (NvCtrlXrandrAttributes *)
        calloc(1, sizeof (NvCtrlXrandrAttributes));
    if ( xrandr == NULL ) {
        error = "Out of memory.";
        goto fail;
    }


    /* Link the libXrandr lib */
    xrandr->libXrandr = dlopen("libXrandr.so.2", RTLD_LAZY);
    if ( xrandr->libXrandr == NULL ) {
        /* Silently fail */
        goto fail;
    }


    /* Resolve XRandR functions */
    xrandr->XRRQueryExtension = NV_DLSYM(xrandr->libXrandr,
                                      "XRRQueryExtension");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRSelectInput = NV_DLSYM(xrandr->libXrandr, "XRRSelectInput");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRConfigCurrentConfiguration = NV_DLSYM(xrandr->libXrandr,
                                                  "XRRConfigCurrentConfiguration");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRGetScreenInfo = NV_DLSYM(xrandr->libXrandr,
                                     "XRRGetScreenInfo");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRConfigRotations = NV_DLSYM(xrandr->libXrandr, "XRRConfigRotations");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRFreeScreenConfigInfo = NV_DLSYM(xrandr->libXrandr,
                                            "XRRFreeScreenConfigInfo");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRSetScreenConfig = NV_DLSYM(xrandr->libXrandr,
                                       "XRRSetScreenConfig");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRRotations = NV_DLSYM(xrandr->libXrandr, "XRRRotations");
    if ((error = dlerror()) != NULL) goto fail;

    xrandr->XRRSizes = NV_DLSYM(xrandr->libXrandr, "XRRSizes");
    if ((error = dlerror()) != NULL) goto fail;


    /* Verify rotation is supported */
    ret = xrandr->XRRQueryExtension(h->dpy, &xrandr->event_base,
                                    &xrandr->error_base);
    if ( !ret ) goto fail;

    xrandr->rotations = xrandr->XRRRotations(h->dpy, h->screen, &rotation);
    sizes             = xrandr->XRRSizes(h->dpy, h->screen, &(xrandr->nsizes));

    /* Must support more than one rotation orientation */
    if ( (xrandr->rotations == 1) || (xrandr->rotations == 2) ||
         (xrandr->rotations == 4) || (xrandr->rotations == 8) ) {
        goto fail;
    }

    
    /* Register to recieve XRandR events */
    xrandr->XRRSelectInput(h->dpy, DefaultRootWindow(h->dpy),
                           RRScreenChangeNotifyMask);   

    //    xrandr->rotations = 1;

    return xrandr;
    

    /* Handle failures */
 fail:
    if ( error != NULL ) {
        nv_error_msg("libXrandr setup error : %s\n", error);
    }
    if ( xrandr != NULL ) {
        if ( xrandr->libXrandr != NULL ) {
            dlclose(xrandr->libXrandr);
        }
        free(xrandr);
    }

    return NULL;

} /* NvCtrlInitXrandrAttributes() */



/******************************************************************************
 *
 * Frees and relinquishes any resource used by the NvCtrlXrandrAttributes
 * extension.
 *
 ****/

void
NvCtrlXrandrAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if ( h == NULL || h->xrandr == NULL ) {
        return;
    }

    if ( h->xrandr->libXrandr != NULL ) {
        dlclose( h->xrandr->libXrandr );
    }

    free(h->xrandr);
    h->xrandr = NULL;

} /* NvCtrlXrandrAttributesClose() */



/******************************************************************************
 *
 * Retrieves XRandR integer attributes
 *
 ****/

ReturnStatus
NvCtrlXrandrGetAttribute (NvCtrlAttributePrivateHandle *h,
                          int attr, int *val)
{
    XRRScreenConfiguration *sc;
    Rotation rotation;


    /* Validate Arguments */
    if ( h == NULL ) {
        return NvCtrlBadHandle;
    }
    if ( h->xrandr == NULL ) {
        return NvCtrlMissingExtension;
    }
    if ( val == NULL ) {
        return NvCtrlBadArgument;
    }


    /* Fetch right attribute */
    switch ( attr ) {

    case NV_CTRL_ATTR_XRANDR_ROTATION_SUPPORTED:
        *val = ( (h->xrandr!=NULL)?1:0 );
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATIONS:
        *val = h->xrandr->rotations;
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATION:
        sc = h->xrandr->XRRGetScreenInfo(h->dpy, DefaultRootWindow(h->dpy));
        h->xrandr->XRRConfigRotations(sc, &rotation);
        h->xrandr->XRRFreeScreenConfigInfo(sc);

        *val = (int)rotation;
        break;

    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done fetching attribute */

    return NvCtrlSuccess;

} /* NvCtrlXrandrGetAttribute */



/******************************************************************************
 *
 * Sets XRandR integer attributes
 *
 ****/

ReturnStatus
NvCtrlXrandrSetAttribute (NvCtrlAttributePrivateHandle *h,
                          int attr, int val)
{
    /* Validate Arguments */
    if ( h == NULL ) {
        return NvCtrlBadHandle;
    }
    if ( h->xrandr == NULL ) {
        return NvCtrlMissingExtension;
    }


    /* Set right attribute */
    switch ( attr ) {

    case NV_CTRL_ATTR_XRANDR_ROTATION_SUPPORTED:
        return NvCtrlReadOnlyAttribute;
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATIONS:
        return NvCtrlReadOnlyAttribute;
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATION:
        return set_rotation(h, (Rotation) val);
        break;

    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done setting attribute */

    return NvCtrlSuccess;

} /* NvCtrlXrandrSetAttribute */
