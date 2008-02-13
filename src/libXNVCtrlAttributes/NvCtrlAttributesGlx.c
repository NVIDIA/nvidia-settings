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
#include <assert.h>

#include <sys/utsname.h>

#include <dlfcn.h>  /* To dynamically load libGL.so */
#include <GL/glx.h> /* GLX #defines */


typedef struct __libGLInfoRec {

    /* libGL.so library handle */
    void *handle;
    int   ref_count; /* # users of the library */

    /* OpenGL functions used */
    const GLubyte * (* glGetString)              (GLenum);

    /* GLX functions used */
    Bool            (* glXQueryExtension)        (Display *, int *, int *);
    const char *    (* glXQueryServerString)     (Display *, int, int);
    const char *    (* glXGetClientString)       (Display *, int);
    const char *    (* glXQueryExtensionsString) (Display *, int);

    Bool            (* glXIsDirect)              (Display *, GLXContext);
    Bool            (* glXMakeCurrent)           (Display *, GLXDrawable,
                                                  GLXContext);
    GLXContext      (* glXCreateContext)         (Display *, XVisualInfo *,
                                                  GLXContext, Bool);
    void            (* glXDestroyContext)        (Display *, GLXContext);
    XVisualInfo *   (* glXChooseVisual)          (Display *, int, int *);
#ifdef GLX_VERSION_1_3
    GLXFBConfig *   (* glXGetFBConfigs)          (Display *, int, int *);
    int             (* glXGetFBConfigAttrib)     (Display *, GLXFBConfig,
                                                  int, int *);
    XVisualInfo *   (* glXGetVisualFromFBConfig) (Display *, GLXFBConfig);
#endif /* GLX_VERSION_1_3 */

} __libGLInfo;

static __libGLInfo *__libGL = NULL;



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
 * Opens libGL for usage
 *
 ****/

static Bool open_libgl(void)
{
    const char *error_str = NULL;


    /* Initialize bookkeeping structure */
    if ( !__libGL ) {
        __libGL = (__libGLInfo *) calloc(1, sizeof(__libGLInfo));
        if ( !__libGL ) {
            goto fail;
        }
    }
    

    /* Library was already opened */
    if ( __libGL->handle ) {
        __libGL->ref_count++;
        return True;
    }


    /* We are the first to open the library */
    __libGL->handle = dlopen("libGL.so.1", RTLD_LAZY);
    if ( !__libGL->handle ) {
        goto fail;
    }


    /* Resolve GLX functions */
    __libGL->glGetString =
        NV_DLSYM(__libGL->handle, "glGetString");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXQueryExtension =
        NV_DLSYM(__libGL->handle, "glXQueryExtension");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXQueryServerString =
        NV_DLSYM(__libGL->handle, "glXQueryServerString");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXGetClientString =
        NV_DLSYM(__libGL->handle, "glXGetClientString");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXQueryExtensionsString =
        NV_DLSYM(__libGL->handle, "glXQueryExtensionsString");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXIsDirect =
        NV_DLSYM(__libGL->handle, "glXIsDirect");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXMakeCurrent =
        NV_DLSYM(__libGL->handle, "glXMakeCurrent");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXCreateContext =
        NV_DLSYM(__libGL->handle, "glXCreateContext");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXDestroyContext =
        NV_DLSYM(__libGL->handle, "glXDestroyContext");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXChooseVisual =
        NV_DLSYM(__libGL->handle, "glXChooseVisual");
    if ((error_str = dlerror()) != NULL) goto fail;

#ifdef GLX_VERSION_1_3
    __libGL->glXGetFBConfigs =
        NV_DLSYM(__libGL->handle, "glXGetFBConfigs");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXGetFBConfigAttrib =
        NV_DLSYM(__libGL->handle, "glXGetFBConfigAttrib");
    if ((error_str = dlerror()) != NULL) goto fail;

    __libGL->glXGetVisualFromFBConfig =
        NV_DLSYM(__libGL->handle, "glXGetVisualFromFBConfig");
    if ((error_str = dlerror()) != NULL) goto fail;
#endif /* GLX_VERSION_1_3 */


    /* Up the ref count */
    __libGL->ref_count++;

    return True;


    /* Handle failures */
 fail:
    if ( error_str ) {
        nv_error_msg("libGL setup error : %s\n", error_str);
    }
    if ( __libGL ) {
        if ( __libGL->handle ) {
            dlclose(__libGL->handle);
            __libGL->handle = NULL;
        }
        free(__libGL);
        __libGL = NULL;
    }
    return False;
    
} /* open_libgL() */



/******************************************************************************
 *
 * Closes libGL - 
 *
 ****/

static void close_libgl(void)
{
    if ( __libGL && __libGL->handle && __libGL->ref_count ) {
        __libGL->ref_count--;
        if ( __libGL->ref_count == 0 ) {
            dlclose(__libGL->handle);
            __libGL->handle = NULL;
            free(__libGL);
            __libGL = NULL;
        }
    }
} /* close_libgl() */



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

Bool
NvCtrlInitGlxAttributes (NvCtrlAttributePrivateHandle *h)
{
    int event_base;
    int error_base;


    /* Check parameters */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return False;
    }


    /* Open libGL.so.1 */
    if ( !open_libgl() ) {
        return False;
    }


    /* Verify server support of GLX extension */
    if ( !__libGL->glXQueryExtension(h->dpy,
                                     &(error_base),
                                     &(event_base)) ) {
        return False;
    }

    return True;

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
    if ( !h || !h->glx ) {
        return;
    }
 
    close_libgl();

    h->glx = False;

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



    assert(h->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN);


    /* Get all fbconfigs for the display/screen */
    fbconfigs = (* (__libGL->glXGetFBConfigs)) (h->dpy, h->target_id,
                                                &nfbconfigs);
    if ( fbconfigs == NULL || nfbconfigs == 0 ) {
        goto fail;
    }

    /* Allocate to hold the fbconfig attributes */
    fbcas = calloc(nfbconfigs + 1, sizeof(GLXFBConfigAttr));
    if ( fbcas == NULL ) {
        goto fail;        
    }

    /* Query each fbconfig's attributes and populate the attrib array */
    for ( i = 0; i < nfbconfigs; i++ ) {

        /* Get related visual id if any */
        visinfo = (* (__libGL->glXGetVisualFromFBConfig)) (h->dpy,
                                                           fbconfigs[i]);
        if ( visinfo ) {
            fbcas[i].visual_id = visinfo->visualid;
            XFree(visinfo);
        } else {
            fbcas[i].visual_id = 0;           
        }

        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy,
                                                  fbconfigs[i],
                                                  GLX_FBCONFIG_ID,
                                                  &(fbcas[i].fbconfig_id));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_BUFFER_SIZE,
                                                  &(fbcas[i].buffer_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_LEVEL,
                                                  &(fbcas[i].level));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_DOUBLEBUFFER,
                                                  &(fbcas[i].doublebuffer));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_STEREO,
                                                  &(fbcas[i].stereo));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_AUX_BUFFERS,
                                                  &(fbcas[i].aux_buffers));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_RED_SIZE,
                                                  &(fbcas[i].red_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_GREEN_SIZE,
                                                  &(fbcas[i].green_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_BLUE_SIZE,
                                                  &(fbcas[i].blue_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_ALPHA_SIZE,
                                                  &(fbcas[i].alpha_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_DEPTH_SIZE,
                                                  &(fbcas[i].depth_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_STENCIL_SIZE,
                                                  &(fbcas[i].stencil_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_ACCUM_RED_SIZE,
                                                  &(fbcas[i].accum_red_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_ACCUM_GREEN_SIZE,
                                                  &(fbcas[i].accum_green_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_ACCUM_BLUE_SIZE,
                                                  &(fbcas[i].accum_blue_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_ACCUM_ALPHA_SIZE,
                                                  &(fbcas[i].accum_alpha_size));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_RENDER_TYPE,
                                                  &(fbcas[i].render_type));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_DRAWABLE_TYPE,
                                                  &(fbcas[i].drawable_type));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_X_RENDERABLE,
                                                  &(fbcas[i].x_renderable));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_X_VISUAL_TYPE,
                                                  &(fbcas[i].x_visual_type));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_CONFIG_CAVEAT,
                                                  &(fbcas[i].config_caveat));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_TYPE,
                                                  &(fbcas[i].transparent_type));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_INDEX_VALUE,
                                                  &(fbcas[i].transparent_index_value));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_RED_VALUE,
                                                  &(fbcas[i].transparent_red_value));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_GREEN_VALUE,
                                                  &(fbcas[i].transparent_green_value));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_BLUE_VALUE,
                                                  &(fbcas[i].transparent_blue_value));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_TRANSPARENT_ALPHA_VALUE,
                                                  &(fbcas[i].transparent_alpha_value));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_WIDTH,
                                                  &(fbcas[i].pbuffer_width));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_HEIGHT,
                                                  &(fbcas[i].pbuffer_height));
        if ( ret != Success ) goto fail;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_MAX_PBUFFER_PIXELS,
                                                  &(fbcas[i].pbuffer_max));
        if ( ret != Success ) goto fail;

#if defined(GLX_SAMPLES_ARB) && defined (GLX_SAMPLE_BUFFERS_ARB)
        fbcas[i].multi_sample_valid = 1;
        ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy, fbconfigs[i],
                                                  GLX_SAMPLES_ARB,
                                                  &(fbcas[i].multi_samples));
        if ( ret != Success ) {
            fbcas[i].multi_sample_valid = 0;
        } else {
            ret = (* (__libGL->glXGetFBConfigAttrib))(h->dpy,
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
    if ( fbcas ) {
        free(fbcas);
    }
    if ( fbconfigs ) {
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


    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }
    if ( !h->glx || !__libGL ) {
        return NvCtrlMissingExtension;
    }
    if ( !ptr ) {
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
  root    = RootWindow(h->dpy, h->target_id); \
  visinfo = __libGL->glXChooseVisual(h->dpy, h->target_id, \
                                    &(attribListSgl[0])); \
  win_attr.background_pixel = 0; \
  win_attr.border_pixel     = 0; \
  win_attr.colormap         = XCreateColormap(h->dpy, root, \
                                              visinfo->visual, AllocNone); \
  win_attr.event_mask       = 0; \
  mask                      = CWBackPixel | CWBorderPixel | CWColormap | \
                              CWEventMask; \
  win  = XCreateWindow(h->dpy, root, 0, 0, width, height, \
                       0, visinfo->depth, InputOutput, \
                       visinfo->visual, mask, &win_attr); \
  ctx  = __libGL->glXCreateContext(h->dpy, visinfo, NULL, True ); \
  if ( ctx ) { __libGL->glXMakeCurrent(h->dpy, win, ctx); }

#define CLEAN_CONTEXT() \
  if ( visinfo )   { XFree(visinfo); } \
  if ( ctx )       { __libGL->glXDestroyContext(h->dpy, ctx); } \
  if ( win )       { XDestroyWindow(h->dpy, win); }


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
    
    /* Validate */
    if ( !h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN ) {
        return NvCtrlBadHandle;
    }
    if ( !h->glx || !__libGL ) {
        return NvCtrlMissingExtension;
    }
    if ( !ptr ) {
        return NvCtrlBadArgument;
    }


    /* Get the right string */
    switch (attr) {
        
    case NV_CTRL_STRING_GLX_DIRECT_RENDERING:
        GET_CONTEXT();
        str = (  (* (__libGL->glXIsDirect))(h->dpy, ctx)  ) ? "Yes" : "No";
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_GLX_EXTENSIONS:
        str = (* (__libGL->glXQueryExtensionsString))(h->dpy, h->target_id);
        break;
    case NV_CTRL_STRING_GLX_SERVER_VENDOR:
        str = (* (__libGL->glXQueryServerString))(h->dpy, h->target_id,
                                                  GLX_VENDOR);
        break;
    case NV_CTRL_STRING_GLX_SERVER_VERSION:
        str = (* (__libGL->glXQueryServerString))(h->dpy, h->target_id,
                                                  GLX_VERSION);
        break;
    case NV_CTRL_STRING_GLX_SERVER_EXTENSIONS:
        str = (* (__libGL->glXQueryServerString))(h->dpy, h->target_id,
                                                  GLX_EXTENSIONS);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_VENDOR:
        str = (* (__libGL->glXGetClientString))(h->dpy, GLX_VENDOR);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_VERSION:
        str = (* (__libGL->glXGetClientString))(h->dpy, GLX_VERSION);
        break;
    case NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS:
        str = (* (__libGL->glXGetClientString))(h->dpy, GLX_EXTENSIONS);
        break;
    case NV_CTRL_STRING_GLX_OPENGL_VENDOR:
        GET_CONTEXT();
        str = (const char *) (* (__libGL->glGetString))(GL_VENDOR);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_RENDERER:
        GET_CONTEXT();
        str = (const char *) (* (__libGL->glGetString))(GL_RENDERER);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_VERSION:
        GET_CONTEXT();
        str = (const char *) (* (__libGL->glGetString))(GL_VERSION);
        CLEAN_CONTEXT();
        break;
    case NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS:
        GET_CONTEXT();
        str = (const char *) (* (__libGL->glGetString))(GL_EXTENSIONS);
        CLEAN_CONTEXT();
        break;
        
    default:
        return NvCtrlNoAttribute;
        break;
    } /* Done - Fetching corresponding string attribute */


    /* Copy the string and return it */
    if ( !str || (*ptr = strdup(str)) == NULL ) {
        return NvCtrlError;           
    }

    return NvCtrlSuccess;
    
} /* NvCtrlGlxGetStringAttribute() */
