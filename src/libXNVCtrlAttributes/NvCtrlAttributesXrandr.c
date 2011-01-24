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
 */

#include <sys/utsname.h>

#include <stdlib.h> /* 64 bit malloc */
#include <assert.h>
#include <string.h>

#include <dlfcn.h> /* To dynamically load libXrandr.so.2 */
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h> /* Xrandr */

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"
#include "NVCtrlLib.h"

#include "msg.h"
#include "parse.h"

/* Make sure we are compiling with XRandR version 1.1 or greater */
#define MIN_RANDR_MAJOR 1
#define MIN_RANDR_MINOR 1
#if (RANDR_MAJOR < MIN_RANDR_MAJOR) || ((RANDR_MAJOR == MIN_RANDR_MAJOR) && (RANDR_MINOR < MIN_RANDR_MINOR))
#error XRandR version 1.1 or greater is required.
#endif

typedef struct __libXrandrInfoRec {

    /* libXrandr.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */
    
    
    /* XRandR functions used */
    Bool (* XRRQueryExtension)
         (Display *dpy, int *event_base, int *error_base);
         
    Status (* XRRQueryVersion)
         (Display *dpy, int *major_versionp, int *minor_versionp);

    void (* XRRSelectInput)
         (Display *dpy, Window window, int mask);
         
    XRRScreenConfiguration * (* XRRGetScreenInfo)
         (Display *dpy, Drawable draw);
         
    SizeID (* XRRConfigCurrentConfiguration)
         (XRRScreenConfiguration *config, Rotation *rotation);

    short (* XRRConfigCurrentRate)
         (XRRScreenConfiguration *config);    
         
    Rotation (* XRRConfigRotations)
         (XRRScreenConfiguration *config, Rotation *rotation);
     
    XRRScreenSize * (* XRRConfigSizes)
         (XRRScreenConfiguration *config, int *nsizes);
    
    short * (* XRRConfigRates)
         (XRRScreenConfiguration *config, int size_index, int *nrates);

    Status (* XRRSetScreenConfig)
         (Display *dpy, XRRScreenConfiguration *config, Drawable draw,
          int size_index, Rotation rotation, Time timestamp);

    Status (* XRRSetScreenConfigAndRate)
         (Display *dpy, XRRScreenConfiguration *config, Drawable draw,
          int size_index, Rotation rotation, short rate, Time timestamp);

    void (* XRRFreeScreenConfigInfo)
         (XRRScreenConfiguration *);

} __libXrandrInfo;

static __libXrandrInfo *__libXrandr = NULL;



/******************************************************************************
 *
 * Opens libXrandr for usage
 *
 ****/

static Bool open_libxrandr(void)
{
    const char *error_str = NULL;


    /* Initialize bookkeeping structure */
    if ( !__libXrandr ) {
        __libXrandr = calloc(1, sizeof(__libXrandrInfo));
        if ( !__libXrandr ) {
            error_str = "Could not allocate memory.";
            goto fail;
        }
    }
    
    /* Library was already opened */
    if ( __libXrandr->handle ) {
        __libXrandr->ref_count++;
        return True;
    }


    /* We are the first to open the library */
    __libXrandr->handle = dlopen("libXrandr.so.2", RTLD_LAZY);
    if ( !__libXrandr->handle ) {
        error_str = dlerror();
        goto fail;
    }


    /* Resolve XRandR functions */
    __libXrandr->XRRQueryExtension =
        NV_DLSYM(__libXrandr->handle, "XRRQueryExtension");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRQueryVersion =
        NV_DLSYM(__libXrandr->handle, "XRRQueryVersion");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRSelectInput =
        NV_DLSYM(__libXrandr->handle, "XRRSelectInput");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRGetScreenInfo =
        NV_DLSYM(__libXrandr->handle, "XRRGetScreenInfo");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRConfigCurrentConfiguration =
        NV_DLSYM(__libXrandr->handle, "XRRConfigCurrentConfiguration");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRConfigCurrentRate =
        NV_DLSYM(__libXrandr->handle, "XRRConfigCurrentRate");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRConfigRotations =
        NV_DLSYM(__libXrandr->handle, "XRRConfigRotations");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRConfigSizes =
        NV_DLSYM(__libXrandr->handle, "XRRConfigSizes");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRConfigRates =
        NV_DLSYM(__libXrandr->handle, "XRRConfigRates");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRSetScreenConfig =
        NV_DLSYM(__libXrandr->handle, "XRRSetScreenConfig");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRSetScreenConfigAndRate =
        NV_DLSYM(__libXrandr->handle, "XRRSetScreenConfigAndRate");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libXrandr->XRRFreeScreenConfigInfo =
        NV_DLSYM(__libXrandr->handle, "XRRFreeScreenConfigInfo");
    if ((error_str = dlerror()) != NULL) goto fail;


    /* Up the ref count */
    __libXrandr->ref_count++;

    return True;


    /* Handle failures  */
 fail:
    if ( error_str ) {
        nv_error_msg("libXrandr setup error : %s\n", error_str);
    }
    if ( __libXrandr ) {
        if ( __libXrandr->handle ) {
            dlclose(__libXrandr->handle);
            __libXrandr->handle = NULL;
        }
        free(__libXrandr);
        __libXrandr = NULL;
    }
    return False;
    
} /* open_libxrandr() */



/******************************************************************************
 *
 * Closes libXrandr when it is no longer being used.
 *
 ****/

static void close_libxrandr(void)
{
    if ( __libXrandr && __libXrandr->handle && __libXrandr->ref_count ) {
        __libXrandr->ref_count--;
    
#if !defined(NV_BSD) /* WAR for FreeBSD static TLS data bug */
        if ( __libXrandr->ref_count == 0 ) {
            dlclose(__libXrandr->handle);
            __libXrandr->handle = NULL;
            free(__libXrandr);
            __libXrandr = NULL;
        }
#endif
    }

} /* close_libxrandr() */



/******************************************************************************
 *
 * Some XR&R functions fails on XFree86 4.3.0 with BadImplementation if the
 * screen resolution is not the one the server started with.  We work around
 * that by temporarily installing an error handler, trying the call, and then
 * disabling the rotation page if it fails.
 *
 ****/

static int errors = 0;
static int (*old_error_handler)(Display *, XErrorEvent *);

static int
error_handler (Display *dpy, XErrorEvent *err)
{
    errors++;
    return 0;

} /* error_handler() */



/******************************************************************************
 *
 * Initializes the NvCtrlXrandrAttributes Extension by linking the
 * libXrandr.so.2 library and resolving functions used.
 *
 ****/

NvCtrlXrandrAttributes *
NvCtrlInitXrandrAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlXrandrAttributes * xrandr = NULL;
    XRRScreenConfiguration *sc;
    Rotation rotation, rotations;

    /* Check parameters */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        goto fail;
    }


    /* Open libXrandr.so.2 */
    if ( !open_libxrandr() ) {
        /* Silently fail */
        goto fail;
    }


    /* Create storage for XRandR attributes */
    xrandr =
        calloc(1, sizeof(NvCtrlXrandrAttributes));
    if ( !xrandr ) {
        goto fail;
    }
    

    XSync(h->dpy, False);
    errors = 0;
    old_error_handler = XSetErrorHandler(error_handler);
    
    /* Verify server support of XRandR extension */
    if ( !__libXrandr->XRRQueryExtension(h->dpy,
                                         &(xrandr->event_base),
                                         &(xrandr->error_base)) ) {
        XSync(h->dpy, False);
        XSetErrorHandler(old_error_handler);
        goto fail;
    }

    /* Verify server version of the XRandR extension */ 
    if ( !__libXrandr->XRRQueryVersion(h->dpy, &(xrandr->major_version), &(xrandr->minor_version)) ||
         ((xrandr->major_version < MIN_RANDR_MAJOR) ||
          ((xrandr->major_version == MIN_RANDR_MAJOR) &&
           (xrandr->minor_version < MIN_RANDR_MINOR)))) {
        XSync(h->dpy, False);
        XSetErrorHandler(old_error_handler);
        goto fail;
    }
   
    /* Register to receive XRandR events */
    __libXrandr->XRRSelectInput(h->dpy, RootWindow(h->dpy, h->target_id),
                                RRScreenChangeNotifyMask);


    /* Try calling a couple functions that are known to fail */
    sc = __libXrandr->XRRGetScreenInfo(h->dpy, RootWindow(h->dpy, h->target_id));
    if ( sc ) {
        rotations = __libXrandr->XRRConfigRotations(sc, &rotation);
        __libXrandr->XRRFreeScreenConfigInfo(sc);
    } else {
        errors++;
    }

    XSync(h->dpy, False);
    XSetErrorHandler(old_error_handler);
    if ( errors > 0 ) {
        goto fail;
    }

    return xrandr;

 fail:
    if ( xrandr ) {
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
    /* Check parameters */
    if ( !h || !h->xrandr || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return;
    }

    close_libxrandr();

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
    Rotation rotation, rotations;


    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }
    if ( !h->xrandr || !__libXrandr ) {
        return NvCtrlMissingExtension;
    }
    if ( !val ) {
        return NvCtrlBadArgument;
    }


    /* Get current screen configuration information */
    sc = __libXrandr->XRRGetScreenInfo(h->dpy, RootWindow(h->dpy,
                                                          h->target_id));
    if ( !sc ) {
        return NvCtrlError;
    }
    rotations = __libXrandr->XRRConfigRotations(sc, &rotation);
    __libXrandr->XRRFreeScreenConfigInfo(sc);


    /* Fetch right attribute */
    switch ( attr ) {

    case NV_CTRL_ATTR_XRANDR_ROTATION_SUPPORTED:
        /* For rotation to be supported, there must
         * be at least 2 orientations
         */
        *val = ( rotations != 1 && rotations != 2 &&
                 rotations != 4 && rotations != 8 ) ? 1 : 0;
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATIONS:
        /* Return the available rotations */
        *val = rotations;
        break;

    case NV_CTRL_ATTR_XRANDR_ROTATION:
        /* Return the current rotation */
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

static ReturnStatus
set_rotation(NvCtrlAttributePrivateHandle *h, Rotation rotation)
{
    XRRScreenConfiguration *sc;
    Rotation cur_rotation, rotations;
    SizeID cur_size;
    Status ret;


    assert(h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN);

    /* Get current screen configuration information */
    sc = __libXrandr->XRRGetScreenInfo(h->dpy, RootWindow(h->dpy,
                                                          h->target_id));
    if ( !sc ) {
        return NvCtrlError;
    }
    cur_size = __libXrandr->XRRConfigCurrentConfiguration(sc, &cur_rotation);
    rotations = __libXrandr->XRRConfigRotations(sc, &cur_rotation);


    /* Check orientation we want is supported */
    if ( !(rotations & rotation) ) {
        __libXrandr->XRRFreeScreenConfigInfo(sc);
        return NvCtrlBadArgument;
    }


    /* Set the orientation */
    ret = __libXrandr->XRRSetScreenConfig(h->dpy, sc,
                                          RootWindow(h->dpy, h->target_id),
                                          cur_size, rotation, CurrentTime);
    __libXrandr->XRRFreeScreenConfigInfo(sc);
    
    return ( ret == Success )?NvCtrlSuccess:NvCtrlError;

} /* set_rotation() */


ReturnStatus
NvCtrlXrandrSetAttribute (NvCtrlAttributePrivateHandle *h,
                          int attr, int val)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }
    if ( !h->xrandr || !__libXrandr ) {
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
        return set_rotation(h, (Rotation)val);
        break;

    default:
        return NvCtrlNoAttribute;
        break;
    }

    return NvCtrlSuccess;

} /* NvCtrlXrandrSetAttribute */


/*
 * Get Xrandr String Attribute Values
 */

ReturnStatus
NvCtrlXrandrGetStringAttribute (NvCtrlAttributePrivateHandle *h,
                                unsigned int display_mask,
                                int attr, char **ptr)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }

    if ( !h->xrandr || !__libXrandr ) {
        return NvCtrlMissingExtension;
    }

    /* Get Xrandr major & minor versions */
    if (attr == NV_CTRL_STRING_XRANDR_VERSION) {
        char str[16];
        sprintf(str, "%d.%d", h->xrandr->major_version, h->xrandr->minor_version);
        *ptr = strdup(str);
        return NvCtrlSuccess;
    }

    return NvCtrlNoAttribute;

} /* NvCtrlXrandrGetStringAttribute() */


/******************************************************************************
 *
 * Sets XRandR size and refresh rate.
 *
 ****/

ReturnStatus
NvCtrlXrandrSetScreenMagicMode (NvCtrlAttributePrivateHandle *h,
                                int width, int height, int magic_ref_rate)
{
    XRRScreenConfiguration *sc;
    Rotation cur_rotation;
    int nsizes;
    XRRScreenSize *sizes;
    int nrates;
    short *rates;
    Status ret;


    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }

    if ( !h->xrandr || !__libXrandr ) {
        return NvCtrlMissingExtension;
    }


    /* Get current screen configuration information */
    sc = __libXrandr->XRRGetScreenInfo(h->dpy, RootWindow(h->dpy,
                                                          h->target_id));
    if ( !sc ) {
        return NvCtrlError;
    }
    __libXrandr->XRRConfigRotations(sc, &cur_rotation);


    /* Look for size that has matching refresh rate */
    sizes = __libXrandr->XRRConfigSizes(sc, &nsizes);
    while ( --nsizes >= 0 ) {

        /* Must match in size */
        if ( sizes[nsizes].width  != width ||
             sizes[nsizes].height != height ) {
            continue;
        }

        rates = __libXrandr->XRRConfigRates(sc, nsizes, &nrates);
        while ( --nrates >= 0 ) {

            /* Set the mode if we found the rate */
            if ( *rates == magic_ref_rate ) {
                ret = __libXrandr->XRRSetScreenConfigAndRate
                    (h->dpy, sc, RootWindow(h->dpy, h->target_id), nsizes,
                     cur_rotation, magic_ref_rate, CurrentTime);
                
                __libXrandr->XRRFreeScreenConfigInfo(sc);
                return (ret == Success)?NvCtrlSuccess:NvCtrlError;
            }
            rates++;
        }
    }
    
    /* If we are here, then we could not find the correct mode to set */
    __libXrandr->XRRFreeScreenConfigInfo(sc);
    return NvCtrlError;

} /* NvCtrlXrandrSetScreenMagicMode */



/******************************************************************************
 *
 * Gets XRandR size and refresh rate.
 *
 ****/

ReturnStatus
NvCtrlXrandrGetScreenMagicMode (NvCtrlAttributePrivateHandle *h,
                                int *width, int *height, int *magic_ref_rate)
{
    XRRScreenConfiguration *sc;
    Rotation cur_rotation;
    int nsizes;
    XRRScreenSize *sizes;
    int cur_size;
    short cur_rate;


    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }

    if ( !h->xrandr || !__libXrandr ) {
        return NvCtrlMissingExtension;
    }


    /* Get current screen configuration information */
    sc = __libXrandr->XRRGetScreenInfo(h->dpy, RootWindow(h->dpy,
                                                          h->target_id));
    if ( !sc ) {
        return NvCtrlError;
    }
    cur_size = __libXrandr->XRRConfigCurrentConfiguration(sc, &cur_rotation);
    cur_rate = __libXrandr->XRRConfigCurrentRate(sc);
    sizes    = __libXrandr->XRRConfigSizes(sc, &nsizes);
    if (cur_size >= nsizes) {
        __libXrandr->XRRFreeScreenConfigInfo(sc);
        return NvCtrlError;
    }


    /* Get the width & height from the size information */
    *width = sizes[cur_size].width;
    *height = sizes[cur_size].height;
    *magic_ref_rate = (int)cur_rate;
    
    __libXrandr->XRRFreeScreenConfigInfo(sc);
    return NvCtrlSuccess;

} /* NvCtrlXrandrGetScreenMagicMode */
