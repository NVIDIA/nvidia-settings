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

#ifndef __NVCTRL_ATTRIBUTES__
#define __NVCTRL_ATTRIBUTES__

#include <X11/Xlib.h>

#include "NVCtrl.h"


typedef void NvCtrlAttributeHandle;

#define NV_FALSE                0
#define NV_TRUE                 1


/*
 * Constants for controlling values (brightness, contrast, gamma) for
 * each color channel.  The *INDEX constants are only meant for
 * internal use.  The CHANNEL and VALUE constants are meant to be used
 * in a bitmask, so that multiple values for multiple channels may be
 * specified at once.
 */

#define RED_CHANNEL_INDEX     0
#define GREEN_CHANNEL_INDEX   1
#define BLUE_CHANNEL_INDEX    2

#define CONTRAST_INDEX        3
#define BRIGHTNESS_INDEX      4
#define GAMMA_INDEX           5

#define RED_CHANNEL            (1 << RED_CHANNEL_INDEX)
#define GREEN_CHANNEL          (1 << GREEN_CHANNEL_INDEX)
#define BLUE_CHANNEL           (1 << BLUE_CHANNEL_INDEX)
#define ALL_CHANNELS           (RED_CHANNEL|GREEN_CHANNEL|BLUE_CHANNEL)

#define CONTRAST_VALUE         (1 << CONTRAST_INDEX)
#define BRIGHTNESS_VALUE       (1 << BRIGHTNESS_INDEX)
#define GAMMA_VALUE            (1 << GAMMA_INDEX)
#define ALL_VALUES             (CONTRAST_VALUE|BRIGHTNESS_VALUE|GAMMA_VALUE)

#define GAMMA_MAX               10.0
#define GAMMA_MIN               (1.0 / GAMMA_MAX)
#define GAMMA_DEFAULT           1.0

#define BRIGHTNESS_MAX          1.0
#define BRIGHTNESS_MIN          -1.0
#define BRIGHTNESS_DEFAULT      0.0

#define CONTRAST_MAX            1.0
#define CONTRAST_MIN            -1.0
#define CONTRAST_DEFAULT        0.0


/*
 * Valid integer attributes for NvCtrl[Get|Set]Attribute(); these are
 * in addition to the ones in NVCtrl.h
 */

#define NV_CTRL_ATTR_BASE                       (NV_CTRL_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_EXT_BASE                   (NV_CTRL_ATTR_BASE)
#define NV_CTRL_ATTR_EXT_NV_PRESENT             (NV_CTRL_ATTR_EXT_BASE + 0)
#define NV_CTRL_ATTR_EXT_VM_PRESENT             (NV_CTRL_ATTR_EXT_BASE + 1)
#define NV_CTRL_ATTR_EXT_XV_OVERLAY_PRESENT     (NV_CTRL_ATTR_EXT_BASE + 2)
#define NV_CTRL_ATTR_EXT_XV_TEXTURE_PRESENT     (NV_CTRL_ATTR_EXT_BASE + 3)
#define NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT     (NV_CTRL_ATTR_EXT_BASE + 4)

#define NV_CTRL_ATTR_EXT_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_EXT_XV_BLITTER_PRESENT)

#define NV_CTRL_ATTR_XV_BASE \
       (NV_CTRL_ATTR_EXT_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_XV_OVERLAY_SATURATION      (NV_CTRL_ATTR_XV_BASE + 0)
#define NV_CTRL_ATTR_XV_OVERLAY_CONTRAST        (NV_CTRL_ATTR_XV_BASE + 1)
#define NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS      (NV_CTRL_ATTR_XV_BASE + 2)
#define NV_CTRL_ATTR_XV_OVERLAY_HUE             (NV_CTRL_ATTR_XV_BASE + 3)
#define NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 4)
#define NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK  (NV_CTRL_ATTR_XV_BASE + 5)
#define NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 6)
#define NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK  (NV_CTRL_ATTR_XV_BASE + 7)
#define NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 8)

#define NV_CTRL_ATTR_XV_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS)

#define NV_CTRL_ATTR_XV_NUM \
       (NV_CTRL_ATTR_XV_LAST_ATTRIBUTE - NV_CTRL_ATTR_XV_BASE + 1)

#define NV_CTRL_ATTR_LAST_ATTRIBUTE (NV_CTRL_ATTR_XV_LAST_ATTRIBUTE)


typedef enum {
    NvCtrlSuccess = 0,
    NvCtrlBadArgument,
    NvCtrlBadHandle,
    NvCtrlNoAttribute,
    NvCtrlMissingExtension,
    NvCtrlReadOnlyAttribute,
    NvCtrlWriteOnlyAttribute,
    NvCtrlAttributeNotAvailable,
    NvCtrlError
} ReturnStatus;



/*
 * NvCtrlAttributeInit() - intializes the control panel backend; this
 * includes probing for the various extensions, downloading the
 * initial state of attributes, etc.  Takes a Display pointer and
 * screen number, and returns an opaque handle on success; returns
 * NULL if the backend cannot use this screen.
 */


#define NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM   0x1
#define NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM  0x2
#define NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM       0x4
#define NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS    \
 (NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM  | \
  NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM | \
  NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM)




NvCtrlAttributeHandle *NvCtrlAttributeInit(Display *dpy, int screen,
                                           unsigned int subsystems);

char *NvCtrlGetDisplayName(NvCtrlAttributeHandle *handle);
Display *NvCtrlGetDisplayPtr(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreen(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenWidth(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenHeight(NvCtrlAttributeHandle *handle);
int NvCtrlGetEventBase(NvCtrlAttributeHandle *handle);

ReturnStatus NvCtrlGetColorAttributes (NvCtrlAttributeHandle *handle,
                                       float contrast[3],
                                       float brightness[3],
                                       float gamma[3]);

ReturnStatus NvCtrlSetColorAttributes (NvCtrlAttributeHandle *handle,
                                       float contrast[3],
                                       float brightness[3],
                                       float gamma[3],
                                       unsigned int flags);

/*
 * NvCtrlGetColorRamp() - get a pointer to the current color ramp for
 * the specified channel; values in the ramp are scaled [0,65536).  If
 * NvCtrlSuccess is returned, lut will point to the color ramp, and n
 * will be the number of entries in the color ramp.
 */

ReturnStatus NvCtrlGetColorRamp (NvCtrlAttributeHandle *handle,
                                 unsigned int channel,
                                 unsigned short **lut,
                                 int *n);


/*
 * NvCtrlGetAttribute()/NvCtrlSetAttribute() - these get and set
 * functions can be used to query and modify all integer attributes.
 * Modifications made via SetAttribute() are made immediatedly.  The
 * attr argument is the attribute to query/modify; valid attributes
 * are those listed in NVCtrl.h and the NV_CTRL_ attributes #define'd
 * above.
 */

ReturnStatus NvCtrlGetAttribute (NvCtrlAttributeHandle *handle,
                                 int attr, int *val);

ReturnStatus NvCtrlSetAttribute (NvCtrlAttributeHandle *handle,
                                 int attr, int val);

/*
 * NvCtrlGetValidAttributeValues() - get the valid settable values for
 * the specified attribute.  See the description of
 * NVCTRLAttributeValidValuesRec in NVCtrl.h.
 */

ReturnStatus NvCtrlGetValidAttributeValues (NvCtrlAttributeHandle *handle,
                                 int attr, NVCTRLAttributeValidValuesRec *val);


/*
 * NvCtrlGetStringAttribute() - get the string associated with the
 * specified attribute, where valid values are the NV_CTRL_STRING_
 * #defines in NVCtrl.h.  These strings are read-only (thus there is
 * no parallel Set function).
 */

ReturnStatus NvCtrlGetStringAttribute (NvCtrlAttributeHandle *handle,
                                       int attr, char **ptr);

/*
 * The following four functions are identical to the above four,
 * except that they specify a particular display mask.
 */

ReturnStatus
NvCtrlGetDisplayAttribute (NvCtrlAttributeHandle *handle,
                           unsigned int display_mask, int attr, int *val);
ReturnStatus
NvCtrlSetDisplayAttribute (NvCtrlAttributeHandle *handle,
                           unsigned int display_mask, int attr, int val);
ReturnStatus
NvCtrlGetValidDisplayAttributeValues (NvCtrlAttributeHandle *handle,
                                      unsigned int display_mask, int attr,
                                      NVCTRLAttributeValidValuesRec *val);
ReturnStatus
NvCtrlGetStringDisplayAttribute (NvCtrlAttributeHandle *handle,
                                 unsigned int display_mask,
                                 int attr, char **ptr);

/*
 * While some attributes should update on-the-fly, other attributes
 * should not be applied until an "Apply" button is pressed.
 *
 * To support this, NvCtrlSetAttributePending() can be called to set
 * the value of an attribute in a special cache in the backend.  These
 * cached values will not be sent to the server until
 * NvCtrlFlushPendingAttributes() is called.
 * NvCtrlCancelPendingAttributes() can be called to clear the cache
 * without sending any values to the server.
 */

ReturnStatus NvCtrlSetAttributePending (NvCtrlAttributeHandle *handle,
                                        int attr, int val);

ReturnStatus NvCtrlFlushPendingAttributes (NvCtrlAttributeHandle *handle);

ReturnStatus NvCtrlCancelPendingAttributes (NvCtrlAttributeHandle *handle);


char *NvCtrlAttributesStrError (ReturnStatus status);

void NvCtrlAttributeClose(NvCtrlAttributeHandle *handle);

#endif /* __NVCTRL_ATTRIBUTES__ */
