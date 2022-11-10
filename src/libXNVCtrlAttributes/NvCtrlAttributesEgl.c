/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2018 NVIDIA Corporation.
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

#include "common-utils.h"
#include "msg.h"

#include "parse.h"
#include "wayland-connector.h"

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xvlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/utsname.h>

#include <dlfcn.h>  /* To dynamically load libEGL.so */
#include <EGL/egl.h>


typedef struct __libEGLInfoRec {

    /* libEGL.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */

    /* EGL functions used */
    EGLBoolean   (* eglInitialize)      (EGLDisplay,
                                         EGLint *, EGLint *);
    EGLBoolean   (* eglTerminate)       (EGLDisplay);
    EGLDisplay   (* eglGetDisplay)      (NativeDisplayType);

    const char * (* eglQueryString)     (EGLDisplay, EGLint);
    EGLBoolean   (* eglGetConfigs)      (EGLDisplay, EGLConfig,
                                         EGLint, EGLint *);
    EGLBoolean   (* eglGetConfigAttrib) (EGLDisplay, EGLConfig,
                                         EGLint, EGLint *);

} __libEGLInfo;

static __libEGLInfo *__libEGL = NULL;



/****
 *
 * Provides a way to communicate EGL settings.
 *
 *
 * Currently available attributes:
 *
 * EGL --------------------------
 *
 *  egl_vendor_str   - STR
 *  egl_version_str  - STR
 *  egl_extensions   - STR
 *
 *
 * EGL Frame Buffer Information ----
 *
 *  fbconfigs_attrib    - ARRAY of EGLConfigAttr
 *
 ****/



/******************************************************************************
 *
 * Opens libEGL for usage
 *
 ****/

static Bool open_libegl(void)
{
    const char *error_str = NULL;


    /* Initialize bookkeeping structure */
    if ( !__libEGL ) {
        __libEGL = nvalloc(sizeof(__libEGLInfo));
    }


    /* Library was already opened */
    if ( __libEGL->handle ) {
        __libEGL->ref_count++;
        return True;
    }


    /* We are the first to open the library */
    __libEGL->handle = dlopen("libEGL.so.1", RTLD_LAZY);
    if ( !__libEGL->handle ) {
        error_str = dlerror();
        goto fail;
    }


    /* Resolve EGL functions */
    __libEGL->eglInitialize =
        NV_DLSYM(__libEGL->handle, "eglInitialize");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libEGL->eglTerminate =
        NV_DLSYM(__libEGL->handle, "eglTerminate");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libEGL->eglGetDisplay =
        NV_DLSYM(__libEGL->handle, "eglGetDisplay");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libEGL->eglQueryString =
        NV_DLSYM(__libEGL->handle, "eglQueryString");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libEGL->eglGetConfigs =
        NV_DLSYM(__libEGL->handle, "eglGetConfigs");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libEGL->eglGetConfigAttrib =
        NV_DLSYM(__libEGL->handle, "eglGetConfigAttrib");
    if ((error_str = dlerror()) != NULL) goto fail;


    /* Up the ref count */
    __libEGL->ref_count++;

    return True;


    /* Handle failures */
 fail:
    if ( error_str ) {
        nv_error_msg("libEGL setup error : %s\n", error_str);
    }
    if ( __libEGL ) {
        if ( __libEGL->handle ) {
            dlclose(__libEGL->handle);
            __libEGL->handle = NULL;
        }
        free(__libEGL);
        __libEGL = NULL;
    }
    return False;

} /* open_libegl() */



/******************************************************************************
 *
 * Closes libEGL -
 *
 ****/

static void close_libegl(void)
{
    if ( __libEGL && __libEGL->handle && __libEGL->ref_count ) {
        __libEGL->ref_count--;
        if ( __libEGL->ref_count == 0 ) {
            dlclose(__libEGL->handle);
            __libEGL->handle = NULL;
            free(__libEGL);
            __libEGL = NULL;
        }
    }
} /* close_libegl() */



/******************************************************************************
 *
 * NvCtrlEglDelayedInit()
 *
 * Resolves get_wayland_display and gets the EGL Display needed.
 *
 ****/
Bool NvCtrlEglDelayedInit(NvCtrlAttributePrivateHandle *h)
{
    int major, minor;

    if (!h || !h->egl || !__libEGL) {
        return False;
    } else if (h->egl_dpy) {
        return True;
    }

    h->wayland_dpy = wconn_get_wayland_display();

    if (h->nv) {
        h->egl_dpy = __libEGL->eglGetDisplay(h->dpy);
    } else {
        h->egl_dpy = __libEGL->eglGetDisplay(h->wayland_dpy);
    }

    if (!__libEGL->eglInitialize(h->egl_dpy, &major, &minor)) {
        h->egl_dpy = NULL;
        return False;
    }

    return (h->egl_dpy != EGL_NO_DISPLAY);
}



/******************************************************************************
 *
 * NvCtrlInitEglAttributes()
 *
 * Initializes the NvCtrlEglAttributes Extension by linking the libEGL.so.1 and
 * resolving functions used to retrieve EGL information.
 *
 ****/

Bool NvCtrlInitEglAttributes (NvCtrlAttributePrivateHandle *h)
{
    /* Check parameters */
    if (!h || (!h->dpy && h->target_type == X_SCREEN_TARGET) ||
              (!wconn_wayland_handle_loaded() && h->target_type == GPU_TARGET)) {
        return NvCtrlBadHandle;
    }

    /* Open libEGL.so.1 */
    if ( !open_libegl() ) {
        return False;
    }

    return True;

} /* NvCtrlInitEglAttributes() */



/******************************************************************************
 *
 * NvCtrlEglAttributesClose()
 *
 * Frees and relinquishes any resource used by the NvCtrlEglAttributes
 * extension.
 *
 ****/

void
NvCtrlEglAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if ( !h || !h->egl ) {
        return;
    }

    if (__libEGL) {
        __libEGL->eglTerminate(h->egl_dpy);

        close_libegl();
    }

    h->egl_dpy = NULL;
    h->egl = False;

} /* NvCtrlEglAttributesClose() */



/******************************************************************************
 *
 * get_configs()
 *
 *
 * Returns an array of EGL Frame Buffer Configuration Attributes for the
 * given Display/Screen.
 *
 ****/

static EGLConfigAttr *get_configs(const NvCtrlAttributePrivateHandle *h)
{
    EGLConfig       *configs = NULL;
    EGLConfigAttr   *cas     = NULL;

    int               nconfigs;
    int               i;
    EGLBoolean        ret; /* Return value of eglGetConfigAttr */



    /* Get all fbconfigs for the display/screen */
    ret = (* (__libEGL->eglGetConfigs)) (h->egl_dpy, NULL, 0, &nconfigs);
    if ( !ret ) {
        goto fail;
    }

    /* Allocate to hold the fbconfig attributes */
    configs = nvalloc((nconfigs + 1) * sizeof(EGLConfig));
    cas     = nvalloc((nconfigs + 1) * sizeof(EGLConfigAttr));

    ret = (* (__libEGL->eglGetConfigs))(h->egl_dpy, configs, nconfigs,
                                        &nconfigs);


    for (i = 0; i < nconfigs; i++) {
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_RED_SIZE,
                                           &cas[i].red_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_BLUE_SIZE,
                                           &cas[i].blue_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_GREEN_SIZE,
                                           &cas[i].green_size);
        if (!ret) goto fail;

        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_DEPTH_SIZE,
                                           &cas[i].depth_size);
        if (!ret) goto fail;

        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_MAX_PBUFFER_WIDTH,
                                           &cas[i].max_pbuffer_width);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_MAX_PBUFFER_HEIGHT,
                                           &cas[i].max_pbuffer_height);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_MAX_PBUFFER_PIXELS,
                                           &cas[i].max_pbuffer_pixels);
        if (!ret) goto fail;


        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_ALPHA_SIZE,
                                           &cas[i].alpha_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_ALPHA_MASK_SIZE,
                                           &cas[i].alpha_mask_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_BIND_TO_TEXTURE_RGB,
                                           &cas[i].bind_to_texture_rgb);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_BIND_TO_TEXTURE_RGBA,
                                           &cas[i].bind_to_texture_rgba);
        if (!ret) goto fail;


        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_BUFFER_SIZE,
                                           &cas[i].buffer_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_COLOR_BUFFER_TYPE,
                                           &cas[i].color_buffer_type);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_CONFIG_CAVEAT,
                                           &cas[i].config_caveat);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_CONFIG_ID,
                                           &cas[i].config_id);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_CONFORMANT,
                                           &cas[i].conformant);
        if (!ret) goto fail;



        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_LEVEL,
                                           &cas[i].level);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_LUMINANCE_SIZE,
                                           &cas[i].luminance_size);
        if (!ret) goto fail;



        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_MAX_SWAP_INTERVAL,
                                           &cas[i].max_swap_interval);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_MIN_SWAP_INTERVAL,
                                           &cas[i].min_swap_interval);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_NATIVE_RENDERABLE,
                                           &cas[i].native_renderable);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_NATIVE_VISUAL_ID,
                                           &cas[i].native_visual_id);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_NATIVE_VISUAL_TYPE,
                                           &cas[i].native_visual_type);
        if (!ret) goto fail;


        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_RENDERABLE_TYPE,
                                           &cas[i].renderable_type);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_SAMPLE_BUFFERS,
                                           &cas[i].sample_buffers);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_SAMPLES,
                                           &cas[i].samples);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_STENCIL_SIZE,
                                           &cas[i].stencil_size);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_SURFACE_TYPE,
                                           &cas[i].surface_type);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_TRANSPARENT_TYPE,
                                           &cas[i].transparent_type);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_TRANSPARENT_RED_VALUE,
                                           &cas[i].transparent_red_value);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_TRANSPARENT_GREEN_VALUE,
                                           &cas[i].transparent_green_value);
        if (!ret) goto fail;
        ret = __libEGL->eglGetConfigAttrib(h->egl_dpy, configs[i],
                                           EGL_TRANSPARENT_BLUE_VALUE,
                                           &cas[i].transparent_blue_value);
        if (!ret) goto fail;
    }

    free(configs);
    return cas;


    /* Handle failures */
 fail:
    free(configs);
    free(cas);

    return NULL;
} /* get_configs() */



/******************************************************************************
 *
 * NvCtrlEglGetVoidAttribute()
 *
 * Retrieves various EGL attributes (other than strings and ints)
 *
 ****/

ReturnStatus NvCtrlEglGetVoidAttribute(const NvCtrlAttributePrivateHandle *h,
                                       unsigned int display_mask,
                                       int attr, void **ptr)
{
    EGLConfigAttr *config_attribs = NULL;


    /* Validate */
    if ( !h || !h->egl_dpy) {
        return NvCtrlBadHandle;
    }

    if ( !h->egl || !__libEGL ) {
        return NvCtrlMissingExtension;
    }
    if ( !ptr ) {
        return NvCtrlBadArgument;
    }


    /* Fetch the right attribute */
    switch ( attr ) {

    case NV_CTRL_ATTR_EGL_CONFIG_ATTRIBS:
        config_attribs = get_configs(h);
        *ptr = config_attribs;
        break;

    default:
        return NvCtrlNoAttribute;
    } /* Done fetching attribute */


    if ( *ptr == NULL ) {
        return NvCtrlError;
    }
    return NvCtrlSuccess;

} /* NvCtrlEglGetAttribute */




/******************************************************************************
 *
 * NvCtrlEglGetStringAttribute()
 *
 * Retrieves a particular EGL information string by calling the appropriate
 * function.
 */

ReturnStatus NvCtrlEglGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                                         unsigned int display_mask,
                                         int attr, char **ptr)
{
    const char *str = NULL;

    /* Validate */
    if ( !h || !h->egl_dpy) {
        return NvCtrlBadHandle;
    }
    if ( !h->egl || !__libEGL ) {
        return NvCtrlMissingExtension;
    }
    if ( !ptr ) {
        return NvCtrlBadArgument;
    }


    /* Get the right string */
    switch (attr) {
    case NV_CTRL_STRING_EGL_VENDOR:
        str = __libEGL->eglQueryString(h->egl_dpy, EGL_VENDOR);
        break;

    case NV_CTRL_STRING_EGL_VERSION:
        str = __libEGL->eglQueryString(h->egl_dpy, EGL_VERSION);
        break;

    case NV_CTRL_STRING_EGL_EXTENSIONS:
        str = __libEGL->eglQueryString(h->egl_dpy, EGL_EXTENSIONS);
        break;

    default:
        return NvCtrlNoAttribute;
    } /* Done - Fetching corresponding string attribute */


    /* Copy the string and return it */
    if ( !str || (*ptr = strdup(str)) == NULL ) {
        return NvCtrlError;
    }

    return NvCtrlSuccess;

} /* NvCtrlEglGetStringAttribute() */
