/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
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
 * Initializes the NvCtrlXrandrAttributes Extension by linking the
 * libXrandr.so.2 library and resolving functions used.
 *
 ****/

NvCtrlXrandrAttributes *
NvCtrlInitXrandrAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlXrandrAttributes * xrandr = NULL;

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
    

    /* Verify server support of XRandR extension */
    if ( !__libXrandr->XRRQueryExtension(h->dpy,
                                         &(xrandr->event_base),
                                         &(xrandr->error_base)) ) {
        goto fail;
    }

    /* Verify server version of the XRandR extension */ 
    if ( !__libXrandr->XRRQueryVersion(h->dpy, &(xrandr->major_version), &(xrandr->minor_version)) ||
         ((xrandr->major_version < MIN_RANDR_MAJOR) ||
          ((xrandr->major_version == MIN_RANDR_MAJOR) &&
           (xrandr->minor_version < MIN_RANDR_MINOR)))) {
        goto fail;
    }
   
    /* Register to receive XRandR events */
    __libXrandr->XRRSelectInput(h->dpy, RootWindow(h->dpy, h->target_id),
                                RRScreenChangeNotifyMask);

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
