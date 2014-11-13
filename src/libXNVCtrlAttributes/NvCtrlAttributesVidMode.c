/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004,2012 NVIDIA Corporation.
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

#include "common-utils.h"
#include "msg.h"

#include <X11/extensions/xf86vmode.h>

#include <stdlib.h>
#include <string.h>


NvCtrlVidModeAttributes *
NvCtrlInitVidModeAttributes(NvCtrlAttributePrivateHandle *h)
{
    NvCtrlVidModeAttributes *vm = NULL;
    int event, error, size, permissions = 0;
    Bool ret;

    /* Check parameters */
    if (!h || !h->dpy || h->target_type != X_SCREEN_TARGET) {
        goto failed;
    }

    ret = XF86VidModeQueryExtension(h->dpy, &event, &error);
    if (!ret) goto failed;

    vm = nvalloc(sizeof(NvCtrlVidModeAttributes));

    ret = XF86VidModeQueryVersion(h->dpy, &(vm->major_version), &(vm->minor_version));
    if (!ret) goto failed;

    if (NV_VERSION2(vm->major_version, vm->minor_version) <
        NV_VERSION2(VM_MINMAJOR, VM_MINMINOR)) {
        nv_warning_msg("The version of the XF86VidMode extension present "
                       "on this display (%d.%d) does not support updating "
                       "gamma ramps.  If you'd like to be able to adjust "
                       "gamma ramps, please update your X server such that "
                       "the version of the XF86VidMode extension is %d.%d "
                       "or higher.", vm->major_version, vm->minor_version,
                       VM_MINMAJOR, VM_MINMINOR);
        goto failed;
    }

    ret = XF86VidModeGetPermissions(h->dpy, h->target_id, &permissions);

    if (!ret) {
        goto failed;
    }

    if (((permissions & XF86VM_READ_PERMISSION) == 0) ||
        ((permissions & XF86VM_WRITE_PERMISSION) == 0)) {
        goto failed;
    }


    ret = XF86VidModeGetGammaRampSize(h->dpy, h->target_id, &size);

    if (!ret) goto failed;

    vm->lut[RED_CHANNEL_INDEX]   = nvalloc(sizeof(unsigned short) * size);
    vm->lut[GREEN_CHANNEL_INDEX] = nvalloc(sizeof(unsigned short) * size);
    vm->lut[BLUE_CHANNEL_INDEX]  = nvalloc(sizeof(unsigned short) * size);

    vm->gammaRampSize = size;

    ret = XF86VidModeGetGammaRamp(h->dpy, h->target_id, size,
                                  vm->lut[RED_CHANNEL_INDEX],
                                  vm->lut[GREEN_CHANNEL_INDEX],
                                  vm->lut[BLUE_CHANNEL_INDEX]);
    if (!ret) goto failed;

    NvCtrlInitGammaInputStruct(&vm->gammaInput);

    return vm;

 failed:
    NvCtrlFreeVidModeAttributes(h);

    return NULL;

} /* NvCtrlInitVidModeAttributes() */


ReturnStatus NvCtrlFreeVidModeAttributes(NvCtrlAttributePrivateHandle *h)
{
    if (!h || !h->vm) {
        return NvCtrlBadHandle;
    }

    free(h->vm->lut[RED_CHANNEL_INDEX]);
    free(h->vm->lut[GREEN_CHANNEL_INDEX]);
    free(h->vm->lut[BLUE_CHANNEL_INDEX]);
    free(h->vm);

    h->vm = NULL;
    
    return NvCtrlSuccess;
}


ReturnStatus NvCtrlVidModeGetColorAttributes(const NvCtrlAttributePrivateHandle *h,
                                             float contrast[3],
                                             float brightness[3],
                                             float gamma[3])
{
    int i;

    if (!h->vm) return NvCtrlMissingExtension;

    for (i = FIRST_COLOR_CHANNEL; i <= LAST_COLOR_CHANNEL; i++) {
        contrast[i]   = h->vm->gammaInput.contrast[i];
        brightness[i] = h->vm->gammaInput.brightness[i];
        gamma[i]      = h->vm->gammaInput.gamma[i];
    }
    
    return NvCtrlSuccess;
    
}





/*
 * Update the color attributes specified by bitmask, recompute the LUT,
 * and send the LUT to the X server.
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

ReturnStatus NvCtrlVidModeSetColorAttributes(NvCtrlAttributePrivateHandle *h,
                                             float c[3],
                                             float b[3],
                                             float g[3],
                                             unsigned int bitmask)
{
    Bool ret;

    if (!h || !h->dpy || h->target_type != X_SCREEN_TARGET) {
        return NvCtrlBadHandle;
    }

    if (!h->vm) {
        return NvCtrlMissingExtension;
    }

    NvCtrlAssignGammaInput(&h->vm->gammaInput, c, b, g, bitmask);

    NvCtrlUpdateGammaRamp(&h->vm->gammaInput,
                          h->vm->gammaRampSize,
                          h->vm->lut,
                          bitmask);

    ret = XF86VidModeSetGammaRamp(h->dpy, h->target_id, h->vm->gammaRampSize,
                                  h->vm->lut[RED_CHANNEL_INDEX],
                                  h->vm->lut[GREEN_CHANNEL_INDEX],
                                  h->vm->lut[BLUE_CHANNEL_INDEX]);

    return ret ? NvCtrlSuccess : NvCtrlError;
}


ReturnStatus NvCtrlVidModeGetColorRamp(const NvCtrlAttributePrivateHandle *h,
                                       unsigned int channel,
                                       unsigned short **lut,
                                       int *n)
{
    if (!h || !h->dpy || h->target_type != X_SCREEN_TARGET) {
        return NvCtrlBadHandle;
    }
    if (!h->vm) {
        return NvCtrlMissingExtension;
    }

    *n = h->vm->gammaRampSize;

    switch (channel) {
      case RED_CHANNEL:   *lut = h->vm->lut[RED_CHANNEL_INDEX];   break;
      case GREEN_CHANNEL: *lut = h->vm->lut[GREEN_CHANNEL_INDEX]; break;
      case BLUE_CHANNEL:  *lut = h->vm->lut[BLUE_CHANNEL_INDEX];  break;
      default: return NvCtrlBadArgument;
    }
    
    return NvCtrlSuccess;
}

ReturnStatus NvCtrlVidModeReloadColorRamp(NvCtrlAttributePrivateHandle *h)
{
    NvCtrlFreeVidModeAttributes(h);

    h->vm = NvCtrlInitVidModeAttributes(h);

    return h->vm ? NvCtrlSuccess : NvCtrlError;
}

/*
 * Get XF86 Video Mode String Attribute Values
 */

ReturnStatus
NvCtrlVidModeGetStringAttribute(const NvCtrlAttributePrivateHandle *h,
                                unsigned int display_mask,
                                int attr, char **ptr)
{
    /* Validate */
    if ( !h || !h->dpy || h->target_type != X_SCREEN_TARGET ) {
        return NvCtrlBadHandle;
    }

    if ( !h->vm) {
        return NvCtrlMissingExtension;
    }

    /* Get Video Mode major & minor versions */
    if (attr == NV_CTRL_STRING_XF86VIDMODE_VERSION) {
        char str[16];
        sprintf(str, "%d.%d", h->vm->major_version, h->vm->minor_version);
        *ptr = strdup(str);
        return NvCtrlSuccess;
    }

    return NvCtrlNoAttribute;

} /* NvCtrlVidModeGetStringAttribute() */

