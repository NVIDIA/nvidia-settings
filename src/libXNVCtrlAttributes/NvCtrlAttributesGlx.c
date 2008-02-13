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

#include "parse.h"

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xvlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/utsname.h>

#include <dlfcn.h>  /* To dynamically load libGL.so */
#include <GL/glx.h> /* GLX #defines */

/****
 *
 * Provides a way to communicate GLX settings.
 *
 *
 * Currently available attributes:
 *
 * GLX Information -----------------
 *
 *  direct_rendering    - STR
 *  glx_extensions      - STR
 *
 *
 * OpenGL --------------------------
 *
 *  opengl_vendor_str   - STR
 *  opengl_renderer_str - STR
 *  opengl_version_str  - STR
 *  opengl_extensions   - STR
 *
 *
 * Server GLX Information ----------
 *
 *  server_vendor_str   - STR
 *  server_version_str  - STR
 *  server_extensions   - STR
 *
 *
 * Client GLX Information ----------
 *
 *  client_vendor_str   - STR
 *  client_version_str  - STR
 *  client_extensions   - STR
 *
 *
 * GLX Frame Buffer Information ----
 *
 *  fbconfigs_attrib    - ARRAY of GLXFBConfigAttr
 *
 ****/



/******************************************************************************
 *
 * NvCtrlInitGlxAttributes()
 *
 * Initializes the NvCtrlGlxAttributes Extension by linking the libGL.so.1 and
 * resolving functions used to retrieve GLX information.
 *
 * NOTE: A private dpy is kept due to a libGL.so.1 bug where closing the library
 *       before closing the dpy will cause XCloseDisplay to segfault.
 *
 ****/

NvCtrlGlxAttributes *
NvCtrlInitGlxAttributes (NvCtrlAttributePrivateHandle *h)
{
    NvCtrlGlxAttributes * glx   = NULL;
    const char          * error = NULL; /* libGL error string */

    /* For querying server about glx extension */
    int errorBase;
    int eventBase;
    Bool                 (* glXQueryExtension) (Display *, int *, int *);



    /* Check parameter */
    if ( h == NULL || h->dpy == NULL ) {
        goto fail;
    }


    /* Allocate for the GlxAttributes struct */
    glx = (NvCtrlGlxAttributes *)
        calloc(1, sizeof (NvCtrlGlxAttributes));
    if ( glx == NULL ) {
        error = "Out of memory.";
        goto fail;
    }


    /* Link the libGL lib */
    glx->libGL = dlopen("libGL.so.1", RTLD_LAZY);
    if ( glx->libGL == NULL ) {
        /* Silently fail */
        goto fail;
    }

    /* Make sure GLX is supported by the server */
    glXQueryExtension = dlsym(glx->libGL, "glXQueryExtension");
    glx->dpy          = XOpenDisplay( XDisplayString(h->dpy) );
    if ( glx->dpy == NULL ) {
        goto fail;
    }
    if ( !glXQueryExtension(glx->dpy, &errorBase, &eventBase) ) {
        goto fail;
    }


    /* Resolve GLX functions */
    glx->glGetString              = dlsym(glx->libGL,
                                          "glGetString");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXQueryExtensionsString = dlsym(glx->libGL,
                                          "glXQueryExtensionsString");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXQueryServerString     = dlsym(glx->libGL,
                                          "glXQueryServerString");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXGetClientString       = dlsym(glx->libGL,
                                          "glXGetClientString");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXIsDirect              = dlsym(glx->libGL,
                                          "glXIsDirect");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXMakeCurrent           = dlsym(glx->libGL,
                                          "glXMakeCurrent");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXCreateContext         = dlsym(glx->libGL,
                                          "glXCreateContext");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXDestroyContext        = dlsym(glx->libGL,
                                          "glXDestroyContext");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXChooseVisual          = dlsym(glx->libGL,
                                          "glXChooseVisual");
    if ((error = dlerror()) != NULL) goto fail;
#ifdef GLX_VERSION_1_3
    glx->glXGetFBConfigs          = dlsym(glx->libGL,
                                          "glXGetFBConfigs");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXGetFBConfigAttrib     = dlsym(glx->libGL,
                                          "glXGetFBConfigAttrib");
    if ((error = dlerror()) != NULL) goto fail;
    glx->glXGetVisualFromFBConfig = dlsym(glx->libGL,
                                          "glXGetVisualFromFBConfig");
    if ((error = dlerror()) != NULL) goto fail;
#endif /* GLX_VERSION_1_3 */


    return glx;
    

    /* Handle failures */
 fail:
    if ( error != NULL ) {
        nv_error_msg("libGL setup error : %s\n", error);
    }
    if ( glx != NULL ) {
        if ( glx->dpy != NULL ) {
            XCloseDisplay(glx->dpy);
        }
        if ( glx->libGL != NULL ) {
            dlclose(glx->libGL);
        }
        free(glx);
    }
    return NULL;
} /* NvCtrlInitGlxAttributes() */



/******************************************************************************
 *
 * NvCtrlGlxAttributesClose()
 * 
 * Frees and relinquishes any resource used by the NvCtrlGlxAttributes
 * extension.
 *
 ****/

void
NvCtrlGlxAttributesClose (NvCtrlAttributePrivateHandle *h)
{
    if ( h == NULL || h->glx == NULL ) {
        return;
    }

    if ( h->glx->dpy != NULL ) {
        XCloseDisplay( h->glx->dpy );
    }

    if ( h->glx->libGL != NULL ) {
        dlclose( h->glx->libGL );
    }

    free(h->glx);
    h->glx = NULL;

} /* NvCtrlGlxAttributesClose() */



/******************************************************************************
 *
 * get_fbconfig_attribs()
 *
 *
 * Returns an array of GLX Frame Buffer Configuration Attributes for the
 * given Display/Screen.
 *
 * NOTE: A seperate display connection is used to avoid the dependence on
 *       libGL when an XCloseDisplay is issued.   If we did not, calling
 *       XCloseDisplay AFTER the libGL library has been dlclose'ed (after
 *       having made at least one GLX call) would cause a segfault.
 *
 ****/

#ifdef GLX_VERSION_1_3

static GLXFBConfigAttr *
get_fbconfig_attribs(NvCtrlAttributePrivateHandle *h)
{
    XVisualInfo     * visinfo;

    GLXFBConfigAttr * fbcas      = NULL;
    GLXFBConfig     * fbconfigs  = NULL;

    int               nfbconfigs;
    int               i;   /* Used for indexing */
    int               ret; /* Return value of glXGetFBConfigAttr */



    /* Some sanity */
    if ( h == NULL || h->dpy == NULL || h->glx == NULL ) {
        goto fail;
    }

    /* Get all fbconfigs for the display/screen */
    fbconfigs = (* (h->glx->glXGetFBConfigs))( h->glx->dpy, h->screen,
                                               &nfbconfigs);
    if ( fbconfigs == NULL || nfbconfigs == 0 ) {
        goto fail;
    }

    /* Allocate to hold the fbconfig attributes */
    fbcas = calloc(nfbconfigs + 1, sizeof (GLXFBConfigAttr));
    if ( fbcas == NULL ) {
        goto fail;        
    }

    /* Query each fbconfig's attributes and populate the attrib array */
    for ( i = 0; i < nfbconfigs; i++ ) {

        /* Get related visual id if any */
        visinfo = (* (h->glx->glXGetVisualFromFBConfig)) (h->glx->dpy,
                                                          fbconfigs[i]);
        if ( visinfo ) {
            fbcas[i].visual_id = visinfo->visualid;
            XFree(visinfo);
        } else {
            fbcas[i].visual_id = 0;           
        }

        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy,
                                                  fbconfigs[i],
                                                  GLX_FBCONFIG_ID,
                                                  &(fbcas[i].fbconfig_id));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_BUFFER_SIZE,
                                                  &(fbcas[i].buffer_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_LEVEL,
                                                  &(fbcas[i].level));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_DOUBLEBUFFER,
                                                  &(fbcas[i].doublebuffer));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_STEREO,
                                                  &(fbcas[i].stereo));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_AUX_BUFFERS,
                                                  &(fbcas[i].aux_buffers));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_RED_SIZE,
                                                  &(fbcas[i].red_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_GREEN_SIZE,
                                                  &(fbcas[i].green_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_BLUE_SIZE,
                                                  &(fbcas[i].blue_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_ALPHA_SIZE,
                                                  &(fbcas[i].alpha_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_DEPTH_SIZE,
                                                  &(fbcas[i].depth_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_STENCIL_SIZE,
                                                  &(fbcas[i].stencil_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_ACCUM_RED_SIZE,
                                                  &(fbcas[i].accum_red_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_ACCUM_GREEN_SIZE,
                                                  &(fbcas[i].accum_green_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_ACCUM_BLUE_SIZE,
                                                  &(fbcas[i].accum_blue_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_ACCUM_ALPHA_SIZE,
                                                  &(fbcas[i].accum_alpha_size));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_RENDER_TYPE,
                                                  &(fbcas[i].render_type));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_DRAWABLE_TYPE,
                                                  &(fbcas[i].drawable_type));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_X_RENDERABLE,
                                                  &(fbcas[i].x_renderable));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_X_VISUAL_TYPE,
                                                  &(fbcas[i].x_visual_type));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_CONFIG_CAVEAT,
                                                  &(fbcas[i].config_caveat));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_TYPE,
                                                  &(fbcas[i].transparent_type));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_INDEX_VALUE,
                                                  &(fbcas[i].transparent_index_value));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_RED_VALUE,
                                                  &(fbcas[i].transparent_red_value));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_GREEN_VALUE,
                                                  &(fbcas[i].transparent_green_value));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_BLUE_VALUE,
                                                  &(fbcas[i].transparent_blue_value));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_ALPHA_VALUE,
                                                  &(fbcas[i].transparent_alpha_value));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_WIDTH,
                                                  &(fbcas[i].pbuffer_width));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_HEIGHT,
                                                  &(fbcas[i].pbuffer_height));
        if ( ret != Success ) goto fail;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_PIXELS,
                                                  &(fbcas[i].pbuffer_max));
        if ( ret != Success ) goto fail;

#if defined(GLX_SAMPLES_ARB) && defined (GLX_SAMPLE_BUFFERS_ARB)
        fbcas[i].multi_sample_valid = 1;
        ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy, fbconfigs[i],
                                                  GLX_SAMPLES_ARB,
                                                  &(fbcas[i].multi_samples));
        if ( ret != Success ) {
            fbcas[i].multi_sample_valid = 0;
        } else {
            ret = (* (h->glx->glXGetFBConfigAttrib)) (h->glx->dpy,
                                                      fbconfigs[i],
                                                      GLX_SAMPLE_BUFFERS_ARB,
                                                      &(fbcas[i].multi_sample_buffers));
            if ( ret != Success ) {
                fbcas[i].multi_sample_valid = 0;
            }
        }
#else
#warning Multisample extension not found, will not print multisample information!
        fbcas[i].multi_sample_valid = 0;
#endif /* Multisample extension */

    } /* Done reading fbconfig information */


    XFree(fbconfigs);
    return fbcas;


    /* Handle failures */
 fail:
    if ( fbcas != NULL ) {
        free(fbcas);
    }
    if ( fbconfigs != NULL ) {
        XFree(fbconfigs);
    }

    return NULL;
} /* get_fbconfig_attribs() */

#endif /* GLX_VERSION_1_3 */



/******************************************************************************
 *
 * NvCtrlGlxGetVoidAttribute()
 *
 * Retrieves various GLX attributes (other than strings and ints)
 *
 ****/

ReturnStatus
NvCtrlGlxGetVoidAttribute (NvCtrlAttributePrivateHandle *h,
                           unsigned int display_mask,
                           int attr, void **ptr) 
{
    GLXFBConfigAttr * fbconfig_attribs = NULL;


    /* Validate Arguments */
    if (h == NULL || ptr == NULL) {
        return NvCtrlBadArgument;
    }


    /* Fetch the right attribute */
    switch ( attr ) {

#ifdef GLX_VERSION_1_3
    case NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS:
        fbconfig_attribs = get_fbconfig_attribs(h);
        *ptr = fbconfig_attribs;
        break;
#endif

    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done fetching attribute */


    if ( *ptr == NULL ) {
        return NvCtrlError;
    }
    return NvCtrlSuccess;

} /* NvCtrlGlxGetAttribute */




/******************************************************************************
 *
 * NvCtrlGlxGetStringAttribute()
 *
 *
 * Retrieves a particular GLX information string by calling the appropreate
 * OpenGL/GLX function.
 *
 *
 * But first, the following macros are used to setup a rendering context
 * such that valid information may be retrieved. (Having a context is
 * required for getting OpenGL and 'Direct rendering' information.)
 *
 * NOTE: A seperate display connection is used to avoid the dependence on
 *       libGL when an XCloseDisplay is issued.   If we did not, calling
 *       XCloseDisplay AFTER the libGL library has been dlclose'ed (after
 *       having made at least one GLX call) would cause a segfault.
 *
 ****/

/* Macros to set up/tear down a rendering context */

#define GET_CONTEXT() \
  \
  root    = RootWindow(h->glx->dpy, h->screen); \
  visinfo = h->glx->glXChooseVisual(h->glx->dpy, h->screen, \
                                    &(attribListSgl[0])); \
  win_attr.background_pixel = 0; \
  win_attr.border_pixel     = 0; \
  win_attr.colormap         = XCreateColormap(h->glx->dpy, root, \
                                              visinfo->visual, AllocNone); \
  win_attr.event_mask       = StructureNotifyMask | ExposureMask; \
  mask                      = CWBackPixel | CWBorderPixel | CWColormap | \
                              CWEventMask; \
  win  = XCreateWindow(h->glx->dpy, root, 0, 0, width, height, \
                       0, visinfo->depth, InputOutput, \
                       visinfo->visual, mask, &win_attr); \
  ctx  = h->glx->glXCreateContext(h->glx->dpy, visinfo, NULL, True ); \
  if ( ctx ) { h->glx->glXMakeCurrent(h->glx->dpy, win, ctx); }

#define CLEAN_CONTEXT() \
  \
  if ( visinfo )   { XFree(visinfo); } \
  if ( ctx )       { h->glx->glXDestroyContext(h->glx->dpy, ctx); } \
  if ( win )       { XDestroyWindow(h->glx->dpy, win); }


ReturnStatus
NvCtrlGlxGetStringAttribute (NvCtrlAttributePrivateHandle *h,
                             unsigned int display_mask,
                             int attr, char **ptr)
{
    const char *str = NULL;

    /* These variables are required for getting some OpenGL/GLX Information */
    Window win;
    Window root;
    GLXContext ctx;
    XVisualInfo *visinfo;
    XSetWindowAttributes win_attr;       /* Used for creating a gc */
    unsigned long mask;
    int width = 100;
    int height = 100;

    static int attribListSgl[] = { GLX_RGBA,
                                   GLX_RED_SIZE, 1,
                                   GLX_GREEN_SIZE, 1,
                                   GLX_BLUE_SIZE, 1,
                                   None };
    
    
    /* Validate Arguments */
    if (h == NULL || ptr == NULL) {
        return NvCtrlBadArgument;
    }

    /* Make sure extension was initialized */
    if ( h->glx == NULL ) {
        return NvCtrlError;
    }

    /* Get the right string */
    switch (attr) {
        
    case NV_CTRL_STRING_GLX_DIRECT_RENDERING:
        GET_CONTEXT();
        str = (  (* (h->glx->glXIsDirect))(h->glx->dpy, ctx)  ) ? "Yes" : "No";
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_GLX_EXTENSIONS:
        str = (* (h->glx->glXQueryExtensionsString))(h->glx->dpy, h->screen);
        break;
    case NV_CTRL_STRING_GLX_SERVER_VENDOR:
        str = (* (h->glx->glXQueryServerString))(h->glx->dpy, h->screen,
                                                 GLX_VENDOR);
        break;
    case NV_CTRL_STRING_GLX_SERVER_VERSION:
        str = (* (h->glx->glXQueryServerString))(h->glx->dpy, h->screen,
                                                 GLX_VERSION);
        break;
    case NV_CTRL_STRING_GLX_SERVER_EXTENSIONS:
        str = (* (h->glx->glXQueryServerString))(h->glx->dpy, h->screen,
                                                 GLX_EXTENSIONS);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_VENDOR:
        str = (* (h->glx->glXGetClientString))(h->glx->dpy, GLX_VENDOR);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_VERSION:
        str = (* (h->glx->glXGetClientString))(h->glx->dpy, GLX_VERSION);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS:
        str = (* (h->glx->glXGetClientString))(h->glx->dpy, GLX_EXTENSIONS);
        break;
    case NV_CTRL_STRING_GLX_OPENGL_VENDOR:
        GET_CONTEXT();
        str = (const char *) (* (h->glx->glGetString))(GL_VENDOR);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_RENDERER:
        GET_CONTEXT();
        str = (const char *) (* (h->glx->glGetString))(GL_RENDERER);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_VERSION:
        GET_CONTEXT();
        str = (const char *) (* (h->glx->glGetString))(GL_VERSION);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS:
        GET_CONTEXT();
        str = (const char *) (* (h->glx->glGetString))(GL_EXTENSIONS);
        CLEAN_CONTEXT();
        break;
        
    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done - Fetching corresponding string attribute */


    /* Copy the string and return it */
    if ( str == NULL ||
         (*ptr = strdup(str)) == NULL ) {
        return NvCtrlError;           
    }

    return NvCtrlSuccess;
    
} /* NvCtrlGlxGetStringAttribute() */
