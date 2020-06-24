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

#include "common-utils.h"
#include "msg.h"
#include "parse.h"

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

    /* gamma-related entry points */

    XRRCrtcGamma *(* XRRGetCrtcGamma)(Display *dpy, RRCrtc crtc);

    void (* XRRSetCrtcGamma)(Display *dpy, RRCrtc crtc, XRRCrtcGamma *gamma);

    void (* XRRFreeGamma)(XRRCrtcGamma *gamma);

    /* output and crtc querying functions */

    XRROutputInfo *(* XRRGetOutputInfo)
        (Display *dpy, XRRScreenResources *resources, RROutput output);

    void (* XRRFreeOutputInfo)(XRROutputInfo *outputInfo);

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
        __libXrandr = nvalloc(sizeof(__libXrandrInfo));
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

    /* the gamma entry points are optional: we don't check dlerror(3) */

    __libXrandr->XRRGetCrtcGamma =
        NV_DLSYM(__libXrandr->handle, "XRRGetCrtcGamma");

    __libXrandr->XRRSetCrtcGamma =
        NV_DLSYM(__libXrandr->handle, "XRRSetCrtcGamma");

    __libXrandr->XRRFreeGamma =
        NV_DLSYM(__libXrandr->handle, "XRRFreeGamma");

    /* the output/crtc functions are optional: we don't check dlerror(3) */

    __libXrandr->XRRGetOutputInfo =
        NV_DLSYM(__libXrandr->handle, "XRRGetOutputInfo");

    __libXrandr->XRRFreeOutputInfo =
        NV_DLSYM(__libXrandr->handle, "XRRFreeOutputInfo");

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

static RROutput GetRandRCrtcForGamma(NvCtrlAttributePrivateHandle *h,
                                     NvCtrlXrandrAttributes *xrandr)
{
    int64_t output_64;
    int output;
    RRCrtc crtc;
    ReturnStatus status;
    XRROutputInfo *pOutputInfo;
    XRRScreenResources screenResources;

    /* finding the RandR output only makes sense for display targets */

    if (h->target_type != DISPLAY_TARGET) {
        return None;
    }

    /* if the server does not support gamma manipulation, return */

    if (!xrandr->gammaAvailable) {
        return None;
    }

    /*
     * if the libXrandr library does not provide the needed entry
     * points, return
     */

    if ((__libXrandr->XRRGetOutputInfo == NULL) ||
        (__libXrandr->XRRFreeOutputInfo == NULL)) {
        return None;
    }

    status = NvCtrlNvControlGetAttribute(h, 0, NV_CTRL_DISPLAY_RANDR_OUTPUT_ID,
                                         &output_64);

    if (status != NvCtrlSuccess) {
        return None;
    }

    output = (int)output_64;

    if (output == 0) {
        return None;
    }

    if ((__libXrandr->XRRGetOutputInfo == NULL) ||
        (__libXrandr->XRRFreeOutputInfo == NULL)) {
        return None;
    }

    /*
     * XXX Normally, an X client should query XRRGetScreenResources(3)
     * to get an appropriately initialized XRRScreenResources data
     * structure.  However, XRRGetOutputInfo(3) only uses
     * XRRScreenResources to get the configTimestamp for the protocol
     * request, and XRRGetScreenResources(3) can be an expensive
     * request (triggers reprobing all display hardware, etc).  So,
     * just zero-initialize XRRScreenResources and pass it into
     * XRRGetOutputInfo().
     */
    memset(&screenResources, 0, sizeof(screenResources));
    screenResources.configTimestamp = CurrentTime;

    pOutputInfo =
        __libXrandr->XRRGetOutputInfo(h->dpy, &screenResources, output);

    if (pOutputInfo == NULL) {
        return None;
    }

    crtc = pOutputInfo->crtc;

    __libXrandr->XRRFreeOutputInfo(pOutputInfo);

    return crtc;
}

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
    if (!h || !h->dpy) {
        goto fail;
    }

    /* allow RandR on X_SCREEN and DISPLAY target types */
    if ((h->target_type != X_SCREEN_TARGET) &&
        (h->target_type != DISPLAY_TARGET)) {
        goto fail;
    }

    /* Open libXrandr.so.2 */
    if ( !open_libxrandr() ) {
        /* Silently fail */
        goto fail;
    }


    /* Create storage for XRandR attributes */
    xrandr = nvalloc(sizeof(NvCtrlXrandrAttributes));


    /* Verify server support of XRandR extension */
    if ( !__libXrandr->XRRQueryExtension(h->dpy,
                                         &(xrandr->event_base),
                                         &(xrandr->error_base)) ) {
        goto fail;
    }

    /* Verify server version of the XRandR extension */ 
    if ( !__libXrandr->XRRQueryVersion(h->dpy, &(xrandr->major_version),
                                       &(xrandr->minor_version)) ||
         ((xrandr->major_version < MIN_RANDR_MAJOR) ||
          ((xrandr->major_version == MIN_RANDR_MAJOR) &&
           (xrandr->minor_version < MIN_RANDR_MINOR)))) {
        goto fail;
    }
   
    /* Register to receive XRandR events if this is an X screen */
    if (h->target_type == X_SCREEN_TARGET) {
        __libXrandr->XRRSelectInput(h->dpy, RootWindow(h->dpy, h->target_id),
                                    RRScreenChangeNotifyMask);
    }

    /* check if this configuration supports gamma manipulation */

    xrandr->gammaAvailable =
        ((xrandr->major_version > 1) ||
         ((xrandr->major_version == 1) && (xrandr->minor_version >= 2))) &&
        (__libXrandr->XRRSetCrtcGamma != NULL);


    /*
     * get the RandR CRTC and gamma; the mapping of NV-CONTROL display
     * device target to RandR CRTC could change at each modeset, so
     * the frontend needs to reallocate this handle after each modeset
     */

    xrandr->gammaCrtc = GetRandRCrtcForGamma(h, xrandr);

    if ((xrandr->gammaCrtc != None) && (__libXrandr->XRRGetCrtcGamma != NULL)) {
        xrandr->pGammaRamp =
            __libXrandr->XRRGetCrtcGamma(h->dpy, xrandr->gammaCrtc);
        NvCtrlInitGammaInputStruct(&xrandr->gammaInput);
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
    if ( !h || !h->xrandr || h->target_type != X_SCREEN_TARGET ) {
        return;
    }

    if ((h->xrandr->pGammaRamp != NULL) &&
        (__libXrandr->XRRFreeGamma != NULL)) {
        __libXrandr->XRRFreeGamma(h->xrandr->pGammaRamp);
    }

    close_libxrandr();

    free(h->xrandr);
    h->xrandr = NULL;

} /* NvCtrlXrandrAttributesClose() */



/*
 * Get Xrandr String Attribute Values
 */

ReturnStatus
NvCtrlXrandrGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                               unsigned int display_mask,
                               int attr, char **ptr)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != X_SCREEN_TARGET ) {
        return NvCtrlBadHandle;
    }

    if ( !h->xrandr || !__libXrandr ) {
        return NvCtrlMissingExtension;
    }

    /* Get Xrandr major & minor versions */
    if (attr == NV_CTRL_STRING_XRANDR_VERSION) {
        char str[24];
        sprintf(str, "%d.%d", h->xrandr->major_version, h->xrandr->minor_version);
        *ptr = strdup(str);
        return NvCtrlSuccess;
    }

    return NvCtrlNoAttribute;

} /* NvCtrlXrandrGetStringAttribute() */


ReturnStatus
NvCtrlXrandrGetAttribute(const NvCtrlAttributePrivateHandle *h,
                         unsigned int display_mask, int attr, int64_t *val)
{
    if (!h || !h->xrandr) {
        return NvCtrlBadHandle;
    }

    if (attr != NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE) {
        return NvCtrlNoAttribute;
    }

    if (h->target_type == X_SCREEN_TARGET) {
        *val = h->xrandr->gammaAvailable;
    } else {
        *val = (h->xrandr->pGammaRamp != NULL);
    }

    return NvCtrlSuccess;
}

ReturnStatus NvCtrlXrandrGetColorAttributes(const NvCtrlAttributePrivateHandle *h,
                                            float contrast[3],
                                            float brightness[3],
                                            float gamma[3])
{
    int i;

    if (!h->xrandr) return NvCtrlMissingExtension;

    for (i = FIRST_COLOR_CHANNEL; i <= LAST_COLOR_CHANNEL; i++) {
        contrast[i]   = h->xrandr->gammaInput.contrast[i];
        brightness[i] = h->xrandr->gammaInput.brightness[i];
        gamma[i]      = h->xrandr->gammaInput.gamma[i];
    }

    return NvCtrlSuccess;

}

ReturnStatus NvCtrlXrandrSetColorAttributes(NvCtrlAttributePrivateHandle *h,
                                            float c[3],
                                            float b[3],
                                            float g[3],
                                            unsigned int bitmask)
{
    unsigned short *tmpGammaArray[3];

    if (!h || !h->dpy) {
        return NvCtrlBadHandle;
    }

    if (!h->xrandr) {
        return NvCtrlMissingExtension;
    }

    if (h->xrandr->pGammaRamp == NULL) {
        return NvCtrlMissingExtension;
    }

    if (h->xrandr->gammaCrtc == None) {
        return NvCtrlMissingExtension;
    }

    NvCtrlAssignGammaInput(&h->xrandr->gammaInput, c, b, g, bitmask);

    tmpGammaArray[RED_CHANNEL_INDEX]   = h->xrandr->pGammaRamp->red;
    tmpGammaArray[GREEN_CHANNEL_INDEX] = h->xrandr->pGammaRamp->green;
    tmpGammaArray[BLUE_CHANNEL_INDEX]  = h->xrandr->pGammaRamp->blue;

    NvCtrlUpdateGammaRamp(&h->xrandr->gammaInput,
                          h->xrandr->pGammaRamp->size,
                          tmpGammaArray,
                          bitmask);

    __libXrandr->XRRSetCrtcGamma(h->dpy, h->xrandr->gammaCrtc,
                                 h->xrandr->pGammaRamp);

    XFlush(h->dpy);

    return NvCtrlSuccess;
}

ReturnStatus NvCtrlXrandrGetColorRamp(const NvCtrlAttributePrivateHandle *h,
                                      unsigned int channel,
                                      uint16_t **lut,
                                      int *n)
{
    if (!h || !h->dpy) {
        return NvCtrlBadHandle;
    }

    if (!h->xrandr) {
        return NvCtrlMissingExtension;
    }

    if (h->xrandr->pGammaRamp == NULL) {
        return NvCtrlMissingExtension;
    }

    *n = h->xrandr->pGammaRamp->size;

    switch (channel) {
      case RED_CHANNEL:
          *lut = h->xrandr->pGammaRamp->red;
          break;
      case GREEN_CHANNEL:
          *lut = h->xrandr->pGammaRamp->green;
          break;
      case BLUE_CHANNEL:
          *lut = h->xrandr->pGammaRamp->blue;
          break;
      default:
          return NvCtrlBadArgument;
    }

    return NvCtrlSuccess;
}

ReturnStatus NvCtrlXrandrReloadColorRamp(NvCtrlAttributePrivateHandle *h)
{
    if ((h->xrandr->pGammaRamp != NULL) &&
        (__libXrandr->XRRFreeGamma != NULL)) {
        __libXrandr->XRRFreeGamma(h->xrandr->pGammaRamp);
    } else {
        assert(!"There should already be a Gamma Ramp");
    }

    if ((h->xrandr->gammaCrtc != None) && (__libXrandr->XRRGetCrtcGamma != NULL)) {
        h->xrandr->pGammaRamp =
            __libXrandr->XRRGetCrtcGamma(h->dpy, h->xrandr->gammaCrtc);
        NvCtrlInitGammaInputStruct(&h->xrandr->gammaInput);
    } else {
        return NvCtrlError;
    }

    return h->xrandr->pGammaRamp ? NvCtrlSuccess : NvCtrlError;

}

