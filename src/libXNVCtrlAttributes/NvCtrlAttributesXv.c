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

#include "NvCtrlAttributes.h"
#include "NvCtrlAttributesPrivate.h"

#include <X11/extensions/Xvlib.h>

#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "common-utils.h"
#include "msg.h"

typedef struct __libXvInfoRec {

    /* libXv.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */

    /* libXv functions used */
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
        __libXv = nvalloc(sizeof(__libXvInfo));
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
    if ( !h || !h->dpy || h->target_type != X_SCREEN_TARGET ) {
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
    xv = nvalloc(sizeof(NvCtrlXvAttributes));


    /* Verify server support of Xv extension */
    ret = __libXv->XvQueryExtension(h->dpy, &(xv->major_version), &(xv->minor_version), &req,
                                    &event_base, &error_base);
    if (ret != Success) goto fail;
    
    /* Get the list of adaptors */
    ret = __libXv->XvQueryAdaptors(h->dpy, RootWindow(h->dpy, h->target_id),
                                   &nadaptors, &ainfo);

    if (ret != Success || !nadaptors || !ainfo) goto fail;
    
    for (i = 0; i < nadaptors; i++) {
        
        if ((strcmp(ainfo[i].name, "NV17 Video Overlay") == 0) ||
            (strcmp(ainfo[i].name, "NV10 Video Overlay") == 0)) {
            xv->overlay = True;
        }

        if (strcmp(ainfo[i].name, "NV17 Video Texture") == 0) {
            xv->texture = True;
        }

        if (strcmp(ainfo[i].name, "NV05 Video Blitter") == 0) {
            xv->blitter = True;
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

    free(xv);

    return NULL;

} /* NvCtrlInitXvAttributes() */

                      
/*
 * Get Xv String Attribute values 
 */

ReturnStatus NvCtrlXvGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                                        unsigned int display_mask,
                                        int attr, char **ptr)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != X_SCREEN_TARGET ) {
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

    free(h->xv);
    h->xv = NULL;

} /* NvCtrlXvAttributesClose() */
