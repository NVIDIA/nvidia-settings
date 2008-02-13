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

#include "msg.h"

#include <X11/extensions/xf86vmode.h>

#include <stdlib.h>
#include <math.h>

static unsigned short computeVal(NvCtrlAttributePrivateHandle *,
                                 int, float, float, float);

#define RED   RED_CHANNEL_INDEX
#define GREEN GREEN_CHANNEL_INDEX
#define BLUE  BLUE_CHANNEL_INDEX


#if defined(X_XF86VidModeGetGammaRampSize)

/*
 * XXX The XF86VidMode extension can block remote clients.
 * Unfortunately, there doesn't seem to be a good way to determine if
 * we're blocked or not.  So, we temporarily plug in an error handler,
 * and watch for the XF86VidModeClientNotLocal error code, set a flag
 * indicating that we should not use the XF86VidMode extension, and
 * then restore the previous error handler.  Yuck.
 *
 * XXX Different versions of XFree86 trigger errors on different
 * protocol; older versions trigger an error on
 * XF86VidModeGetGammaRampSize(), but newer versions appear to only
 * error on XF86VidModeSetGammaRamp().
 */

static Bool vidModeBlocked = False;
static int vidModeErrorBase = 0;
static int (*prev_error_handler)(Display *, XErrorEvent *) = NULL;

static int vidModeErrorHandler(Display *dpy, XErrorEvent *err)
{
    if (err->error_code == (XF86VidModeClientNotLocal + vidModeErrorBase)) {
        vidModeBlocked = True;
    } else {
        if (prev_error_handler) prev_error_handler(dpy, err);
    }
    
    return 1;
}

#endif /* X_XF86VidModeGetGammaRampSize */




NvCtrlVidModeAttributes *
NvCtrlInitVidModeAttributes(NvCtrlAttributePrivateHandle *h)
{
    NvCtrlVidModeAttributes *vm = NULL;
    int ret, event, ver, rev, i;
    

    /* Check parameters */
    if (!h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) {
        goto failed;
    }

#if defined(X_XF86VidModeGetGammaRampSize)

    ret = XF86VidModeQueryExtension(h->dpy, &event, &vidModeErrorBase);
    if (ret != True) goto failed;

    ret = XF86VidModeQueryVersion(h->dpy, &ver, &rev);
    if (ret != True) goto failed;
    
    if (ver < VM_MINMAJOR || (ver == VM_MINMAJOR && rev < VM_MINMINOR)) {
        nv_warning_msg("The version of the XF86VidMode extension present "
                       "on this display (%d.%d) does not support updating "
                       "gamma ramps.  If you'd like to be able to adjust "
                       "gamma ramps, please update your X server such that "
                       "the version of the XF86VidMode extension is %d.%d "
                       "or higher.", rev, ver, VM_MINMAJOR, VM_MINMINOR);
        goto failed;
    }

    vm = (NvCtrlVidModeAttributes *) malloc(sizeof(NvCtrlVidModeAttributes));

    /*
     * XXX setup an error handler to catch any errors caused by the
     * VidMode extension blocking remote clients; we'll restore the
     * original error handler below
     */
    
    prev_error_handler = XSetErrorHandler(vidModeErrorHandler);
    
    ret = XF86VidModeGetGammaRampSize(h->dpy, h->target_id, &vm->n);
    
    /* check if XF86VidModeGetGammaRampSize was blocked */
    
    if (vidModeBlocked) {
        goto blocked;
    }
    
    if (ret != True) goto failed;
    
    vm->lut[RED]   = (unsigned short *) malloc(sizeof(unsigned short) * vm->n);
    vm->lut[GREEN] = (unsigned short *) malloc(sizeof(unsigned short) * vm->n);
    vm->lut[BLUE]  = (unsigned short *) malloc(sizeof(unsigned short) * vm->n);
    
    ret = XF86VidModeGetGammaRamp(h->dpy, h->target_id, vm->n, vm->lut[RED],
                                  vm->lut[GREEN], vm->lut[BLUE]);
    
    /* check if XF86VidModeGetGammaRamp was blocked */

    if (vidModeBlocked) {
        goto blocked;
    }

    if (ret != True) goto failed;
    
    /*
     * XXX Currently, XF86VidModeSetGammaRamp() is the only other
     * XF86VidMode protocol we send, and depending on the XFree86
     * version, it may induce an X error for remote clients.  So, try
     * sending it here to see if we get an error (yes, this is the
     * data we just retrieved above from XF86VidModeGetGammaRamp).
     * It's terrible that we have to do this.
     */
    
    ret = XF86VidModeSetGammaRamp(h->dpy, h->target_id, vm->n,
                                  vm->lut[RED],
                                  vm->lut[GREEN],
                                  vm->lut[BLUE]);
    /*
     * sync the protocol stream to make sure we process any X error
     * before continuing
     */
    
    XSync(h->dpy, False);

    /* check if XF86VidModeSetGammaRamp was blocked */
    
    if (vidModeBlocked) {
        goto blocked;
    }
    
    /* finally, restore the original error handler */
    
    XSetErrorHandler(prev_error_handler);
    
    /*
     * XXX can we initialize this to anything based on the current
     * ramps?
     */
            
    for (i = RED; i <= BLUE; i++) {
        vm->brightness[i] = BRIGHTNESS_DEFAULT;
        vm->contrast[i]   = CONTRAST_DEFAULT;
        vm->gamma[i]      = GAMMA_DEFAULT;
    }
    
    /* take log2 of vm->n to find the sigbits */
    
    for (i = 0; ((vm->n >> i) > 0); i++);
    vm->sigbits = i - 1;
    
    return (vm);

#else
    
#warning Old xf86vmode.h; dynamic gamma ramp support will not be compiled.
    
#endif
    
 blocked:
    
    nv_warning_msg("The VidMode extension is blocked for remote "
                   "clients.  To allow remote VidMode clients, the "
                   "XF86Config option \"AllowNonLocalXvidtune\" must be "
                   "set in the ServerFlags section of the XF86Config "
                   "file.");
    
    /* fall through */

 failed:
    if (vm) free(vm);

    /* restore the original error handler, if we overrode it */

    if (prev_error_handler) {
        XSetErrorHandler(prev_error_handler);
        prev_error_handler = NULL;
    }

    return NULL;

} /* NvCtrlInitVidModeAttributes() */


ReturnStatus NvCtrlGetColorAttributes(NvCtrlAttributeHandle *handle,
                                      float contrast[3],
                                      float brightness[3],
                                      float gamma[3])
{
    int i;
    NvCtrlAttributePrivateHandle *h;
    
    h = (NvCtrlAttributePrivateHandle *) handle;

    if (!h->vm) return NvCtrlMissingExtension;

    for (i = RED; i <= BLUE; i++) {
        contrast[i]   = h->vm->contrast[i];
        brightness[i] = h->vm->brightness[i];
        gamma[i]      = h->vm->gamma[i];
    }
    
    return NvCtrlSuccess;
    
} /* NvCtrlGetColorAttributes() */





/*
 * NvCtrlSetColorAttributes() - update the color attributes specified
 * by bitmask, recompute the LUT, and send the LUT to the X server.
 *
 * The bitmask parameter is a bitmask of which channels (RED_CHANNEL,
 * GREEN_CHANNEL, and BLUE_CHANNEL) and which values (CONTRAST_VALUE,
 * BRIGHTNESS_VALUE, GAMMA_VALUE) should be updated.
 *
 * XXX future optimization: if each channel has the same c/b/g values,
 * don't need to compute the ramp separately per channel.
 *
 * XXX future optimization: if the input is the same as what we
 * already have, we don't actually need to recompute the ramp and send
 * it to the X server.
 */

ReturnStatus NvCtrlSetColorAttributes(NvCtrlAttributeHandle *handle,
                                      float c[3],
                                      float b[3],
                                      float g[3],
                                      unsigned int bitmask)
{
    int i, ch;
    Bool ret;
    
#if defined(X_XF86VidModeGetGammaRampSize)
    
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    /* Check parameters */
    if (!h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlBadHandle;
    }
    if (!h->vm) return NvCtrlMissingExtension;

    /* clamp input, but only the input specified in the bitmask */
    
    for (ch = RED; ch <= BLUE; ch++) {
        if (bitmask & (1 << ch)) {
            if (bitmask & CONTRAST_VALUE) {
                if (c[ch] > CONTRAST_MAX)   c[ch] = CONTRAST_MAX;
                if (c[ch] < CONTRAST_MIN)   c[ch] = CONTRAST_MIN;
            }
            if (bitmask & BRIGHTNESS_VALUE) {
                if (b[ch] > BRIGHTNESS_MAX) b[ch] = BRIGHTNESS_MAX;
                if (b[ch] < BRIGHTNESS_MIN) b[ch] = BRIGHTNESS_MIN;
            }
            if (bitmask & GAMMA_VALUE) {
                if (g[ch] > GAMMA_MAX)      g[ch] = GAMMA_MAX;
                if (g[ch] < GAMMA_MIN)      g[ch] = GAMMA_MIN;
            }
        }
    }
    
    /* assign specified values */

    if (bitmask & CONTRAST_VALUE) {
        if (bitmask & RED_CHANNEL)   h->vm->contrast[RED]     = c[RED];
        if (bitmask & GREEN_CHANNEL) h->vm->contrast[GREEN]   = c[GREEN];
        if (bitmask & BLUE_CHANNEL)  h->vm->contrast[BLUE]    = c[BLUE];
    }

    if (bitmask & BRIGHTNESS_VALUE) {
        if (bitmask & RED_CHANNEL)   h->vm->brightness[RED]   = b[RED];
        if (bitmask & GREEN_CHANNEL) h->vm->brightness[GREEN] = b[GREEN];
        if (bitmask & BLUE_CHANNEL)  h->vm->brightness[BLUE]  = b[BLUE];
    }
    
    if (bitmask & GAMMA_VALUE) {
        if (bitmask & RED_CHANNEL)   h->vm->gamma[RED]        = g[RED];
        if (bitmask & GREEN_CHANNEL) h->vm->gamma[GREEN]      = g[GREEN];
        if (bitmask & BLUE_CHANNEL)  h->vm->gamma[BLUE]       = g[BLUE];
    }

    for (ch = RED; ch <= BLUE; ch++) {
        if ( !(bitmask & (1 << ch))) continue; /* don't update this channel */
        for (i = 0; i < h->vm->n; i++) {
            h->vm->lut[ch][i] = computeVal(h, i, h->vm->contrast[ch],
                                           h->vm->brightness[ch],
                                           h->vm->gamma[ch]);
        }
    }
    
    ret = XF86VidModeSetGammaRamp(h->dpy, h->target_id, h->vm->n,
                                  h->vm->lut[RED],
                                  h->vm->lut[GREEN],
                                  h->vm->lut[BLUE]);
    
    if (ret != True) return NvCtrlError;

    return NvCtrlSuccess;
#else
    return NvCtrlMissingExtension;

#endif /* X_XF86VidModeGetGammaRampSize */
    
} /* NvCtrlSetColorAttribute() */


ReturnStatus NvCtrlGetColorRamp(NvCtrlAttributeHandle *handle,
                                unsigned int channel,
                                unsigned short **lut,
                                int *n)
{
#if defined(X_XF86VidModeGetGammaRampSize)
    
    NvCtrlAttributePrivateHandle *h;

    h = (NvCtrlAttributePrivateHandle *) handle;

    /* Check parameters */
    if (!h || !h->dpy || h->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN) {
        return NvCtrlBadHandle;
    }
    if (!h->vm) return NvCtrlMissingExtension;

    *n = h->vm->n;
    
    switch (channel) {
      case RED_CHANNEL:   *lut = h->vm->lut[RED];   break;
      case GREEN_CHANNEL: *lut = h->vm->lut[GREEN]; break;
      case BLUE_CHANNEL:  *lut = h->vm->lut[BLUE];  break;
      default: return NvCtrlBadArgument;
    }
    
    return NvCtrlSuccess;
#else
    return NvCtrlMissingExtension;
#endif
}


/*
 * computeVal() - compute the LUT entry given its index, and the
 * contrast, brightness, and gamma.
 */
static unsigned short computeVal(NvCtrlAttributePrivateHandle *h,
                                 int i, float c, float b, float g)
{
    double j, half, scale;
    int shift, val, num;
    
    num = h->vm->n - 1;
    shift = 16 - h->vm->sigbits;
    
    scale = (double) num / 3.0; /* how much brightness and contrast
                                   affect the value */
    j = (double) i;
    
    /* contrast */

    c *= scale;

    if (c > 0.0) {
        half = ((double) num / 2.0) - 1.0;
        j -= half;
        j *= half / (half - c);
        j += half;
    } else {
        half = (double) num / 2.0;
        j -= half;
        j *= (half + c) / half;
        j += half;
    }
    
    if (j < 0.0) j = 0.0;
            
    /* gamma */

    g = 1.0 / (double) g;
    
    if (g == 1.0) {
        val = (int) j;
    } else {
        val = (int) (pow (j / (double)num, g) * (double)num + 0.5);
    }

    /* brightness */
    
    b *= scale;

    val += (int)b;
    if (val > num) val = num;
    if (val < 0) val = 0;
    
    val <<= shift;
    return (unsigned short) val;
    
} /* computeVal() */
