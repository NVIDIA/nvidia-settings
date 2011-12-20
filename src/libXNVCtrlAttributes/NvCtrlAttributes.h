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

#define NV_CTRL_ATTR_NV_BASE \
       (NV_CTRL_ATTR_EXT_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_NV_MAJOR_VERSION           (NV_CTRL_ATTR_NV_BASE + 0)
#define NV_CTRL_ATTR_NV_MINOR_VERSION           (NV_CTRL_ATTR_NV_BASE + 1)

#define NV_CTRL_ATTR_NV_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_NV_MINOR_VERSION)

#define NV_CTRL_ATTR_XV_BASE \
       (NV_CTRL_ATTR_NV_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_XV_OVERLAY_SATURATION      (NV_CTRL_ATTR_XV_BASE + 0)
#define NV_CTRL_ATTR_XV_OVERLAY_CONTRAST        (NV_CTRL_ATTR_XV_BASE + 1)
#define NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS      (NV_CTRL_ATTR_XV_BASE + 2)
#define NV_CTRL_ATTR_XV_OVERLAY_HUE             (NV_CTRL_ATTR_XV_BASE + 3)
#define NV_CTRL_ATTR_XV_OVERLAY_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 4)
#define NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK  (NV_CTRL_ATTR_XV_BASE + 5)
#define NV_CTRL_ATTR_XV_TEXTURE_CONTRAST        (NV_CTRL_ATTR_XV_BASE + 6)
#define NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS      (NV_CTRL_ATTR_XV_BASE + 7)
#define NV_CTRL_ATTR_XV_TEXTURE_SATURATION      (NV_CTRL_ATTR_XV_BASE + 8)
#define NV_CTRL_ATTR_XV_TEXTURE_HUE             (NV_CTRL_ATTR_XV_BASE + 9)
#define NV_CTRL_ATTR_XV_TEXTURE_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 10)
#define NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK  (NV_CTRL_ATTR_XV_BASE + 11)
#define NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS    (NV_CTRL_ATTR_XV_BASE + 12)


#define NV_CTRL_ATTR_XV_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_XV_BLITTER_SET_DEFAULTS)

#define NV_CTRL_ATTR_XV_NUM \
       (NV_CTRL_ATTR_XV_LAST_ATTRIBUTE - NV_CTRL_ATTR_XV_BASE + 1)

/* GLX */

#define NV_CTRL_ATTR_GLX_BASE \
       (NV_CTRL_ATTR_XV_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS  (NV_CTRL_ATTR_GLX_BASE +  0)

#define NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS)

/* XRandR */

#define NV_CTRL_ATTR_XRANDR_BASE \
       (NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_XRANDR_ROTATION_SUPPORTED (NV_CTRL_ATTR_XRANDR_BASE +  0)
#define NV_CTRL_ATTR_XRANDR_ROTATIONS          (NV_CTRL_ATTR_XRANDR_BASE +  1)
#define NV_CTRL_ATTR_XRANDR_ROTATION           (NV_CTRL_ATTR_XRANDR_BASE +  2)

#define NV_CTRL_ATTR_XRANDR_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_XRANDR_ROTATION)


#define NV_CTRL_ATTR_LAST_ATTRIBUTE (NV_CTRL_ATTR_XRANDR_LAST_ATTRIBUTE)


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


/* GLX FBConfig attribute structure */

typedef struct GLXFBConfigAttrRec {
    int fbconfig_id;
    int visual_id;

    int buffer_size;
    int level;
    int doublebuffer;
    int stereo;
    int aux_buffers;

    int red_size;
    int green_size;
    int blue_size;
    int alpha_size;
    int depth_size;
    int stencil_size;

    int accum_red_size;
    int accum_green_size;
    int accum_blue_size;
    int accum_alpha_size;

    int render_type;
    int drawable_type;
    int x_renderable;
    int x_visual_type;
    int config_caveat;

    int transparent_type;
    int transparent_index_value;
    int transparent_red_value;
    int transparent_green_value;
    int transparent_blue_value;
    int transparent_alpha_value;

    int pbuffer_width;
    int pbuffer_height;
    int pbuffer_max;

    int multi_sample_valid;
    int multi_samples;
    int multi_sample_buffers;
    int multi_sample_coverage_valid;
    int multi_samples_color;
    
} GLXFBConfigAttr;


/*
 * Additional NV-CONTROL string attributes for NvCtrlGetStringDisplayAttribute();
 * these are in addition to the ones in NVCtrl.h
 */

#define NV_CTRL_STRING_NV_CONTROL_BASE            (NV_CTRL_STRING_LAST_ATTRIBUTE + 1)

#define NV_CTRL_STRING_NV_CONTROL_VERSION         (NV_CTRL_STRING_NV_CONTROL_BASE)

#define NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE  (NV_CTRL_STRING_NV_CONTROL_VERSION)


/*
 * Valid string attributes for NvCtrlGetStringAttribute(); these are
 * in addition to the ones in NVCtrl.h
 */

#define NV_CTRL_STRING_GLX_BASE \
       (NV_CTRL_STRING_NV_CONTROL_LAST_ATTRIBUTE + 1)

#define NV_CTRL_STRING_GLX_DIRECT_RENDERING  (NV_CTRL_STRING_GLX_BASE +  0)
#define NV_CTRL_STRING_GLX_GLX_EXTENSIONS    (NV_CTRL_STRING_GLX_BASE +  1)

#define NV_CTRL_STRING_GLX_SERVER_VENDOR     (NV_CTRL_STRING_GLX_BASE +  2)
#define NV_CTRL_STRING_GLX_SERVER_VERSION    (NV_CTRL_STRING_GLX_BASE +  3)
#define NV_CTRL_STRING_GLX_SERVER_EXTENSIONS (NV_CTRL_STRING_GLX_BASE +  4)

#define NV_CTRL_STRING_GLX_CLIENT_VENDOR     (NV_CTRL_STRING_GLX_BASE +  5)
#define NV_CTRL_STRING_GLX_CLIENT_VERSION    (NV_CTRL_STRING_GLX_BASE +  6)
#define NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS (NV_CTRL_STRING_GLX_BASE +  7)

#define NV_CTRL_STRING_GLX_OPENGL_VENDOR     (NV_CTRL_STRING_GLX_BASE +  8)
#define NV_CTRL_STRING_GLX_OPENGL_RENDERER   (NV_CTRL_STRING_GLX_BASE +  9)
#define NV_CTRL_STRING_GLX_OPENGL_VERSION    (NV_CTRL_STRING_GLX_BASE + 10)
#define NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS (NV_CTRL_STRING_GLX_BASE + 11)

#define NV_CTRL_STRING_GLX_LAST_ATTRIBUTE \
       (NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS)

/*
 * Additional XRANDR string attributes for NvCtrlGetStringDisplayAttribute();
 */

#define NV_CTRL_STRING_XRANDR_BASE            (NV_CTRL_STRING_GLX_LAST_ATTRIBUTE + 1)

#define NV_CTRL_STRING_XRANDR_VERSION         (NV_CTRL_STRING_XRANDR_BASE)

#define NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE  (NV_CTRL_STRING_XRANDR_VERSION)


/*
 * Additional XF86VidMode string attributes for NvCtrlGetStringDisplayAttribute();
 */

#define NV_CTRL_STRING_XF86VIDMODE_BASE            (NV_CTRL_STRING_XRANDR_LAST_ATTRIBUTE + 1)

#define NV_CTRL_STRING_XF86VIDMODE_VERSION         (NV_CTRL_STRING_XF86VIDMODE_BASE)

#define NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE  (NV_CTRL_STRING_XF86VIDMODE_VERSION)


/*
 * Additional XVideo string attributes for NvCtrlGetStringDisplayAttribute();
 */

#define NV_CTRL_STRING_XV_BASE            (NV_CTRL_STRING_XF86VIDMODE_LAST_ATTRIBUTE + 1)

#define NV_CTRL_STRING_XV_VERSION         (NV_CTRL_STRING_XV_BASE)

#define NV_CTRL_STRING_XV_LAST_ATTRIBUTE  (NV_CTRL_STRING_XV_VERSION)


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
#define NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM          0x8
#define NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM       0x10
#define NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS    \
 (NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM  | \
  NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM | \
  NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM      | \
  NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM         | \
  NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM)




NvCtrlAttributeHandle *NvCtrlAttributeInit(Display *dpy, int target_type,
                                           int target_id,
                                           unsigned int subsystems);

char *NvCtrlGetDisplayName(NvCtrlAttributeHandle *handle);
Display *NvCtrlGetDisplayPtr(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreen(NvCtrlAttributeHandle *handle);
int NvCtrlGetTargetType(NvCtrlAttributeHandle *handle);
int NvCtrlGetTargetId(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenWidth(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenHeight(NvCtrlAttributeHandle *handle);
int NvCtrlGetEventBase(NvCtrlAttributeHandle *handle);
int NvCtrlGetXrandrEventBase(NvCtrlAttributeHandle *handle);
char *NvCtrlGetServerVendor(NvCtrlAttributeHandle *handle);
int NvCtrlGetVendorRelease(NvCtrlAttributeHandle *handle);
int NvCtrlGetProtocolVersion(NvCtrlAttributeHandle *handle);
int NvCtrlGetProtocolRevision(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenCount(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenWidthMM(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenHeightMM(NvCtrlAttributeHandle *handle);
int NvCtrlGetScreenPlanes(NvCtrlAttributeHandle *handle);

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
 * NvCtrlQueryTargetCount() - query the number of targets available
 * on the server of the given target type.  This is used, for example
 * to return the number of GPUs the server knows about.
 */
ReturnStatus NvCtrlQueryTargetCount(NvCtrlAttributeHandle *handle,
                                    int target_type,
                                    int *val);

/*
 * NvCtrlGetAttribute()/NvCtrlSetAttribute() - these get and set
 * functions can be used to query and modify all integer attributes.
 * Modifications made via SetAttribute() are made immediatedly.  The
 * attr argument is the attribute to query/modify; valid attributes
 * are those listed in NVCtrl.h and the NV_CTRL_ attributes #define'd
 * above.  NvCtrlGetAttribute64() behaves like NvCtrlGetAttribute(),
 * but supports 64-bit integer attributes.
 */

ReturnStatus NvCtrlGetAttribute (NvCtrlAttributeHandle *handle,
                                 int attr, int *val);

ReturnStatus NvCtrlSetAttribute (NvCtrlAttributeHandle *handle,
                                 int attr, int val);

ReturnStatus NvCtrlGetAttribute64 (NvCtrlAttributeHandle *handle,
                                   int attr, int64_t *val);


/*
 * NvCtrlGetVoidAttribute() - this function works like the
 * Get and GetString only it returns a void pointer.  The
 * data type pointed to is dependent on which attribute you
 * are requesting.
 */

ReturnStatus NvCtrlGetVoidAttribute (NvCtrlAttributeHandle *handle,
                                     int attr, void **ptr);


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
 * #defines in NVCtrl.h.
 */

ReturnStatus NvCtrlGetStringAttribute (NvCtrlAttributeHandle *handle,
                                       int attr, char **ptr);

/*
 * NvCtrlSetStringAttribute() - Set the string associated with the
 * specified attribute, where valid values are the NV_CTRL_STRING_
 * #defines in NVCtrl.h that have the 'W' (Write) flag set.  If 'ret'
 * is specified, (integer) result information is returned.
 */

ReturnStatus NvCtrlSetStringAttribute (NvCtrlAttributeHandle *handle,
                                       int attr, char *ptr, int *ret);

/*
 * The following four functions are identical to the above five,
 * except that they specify a particular display mask.
 */

ReturnStatus
NvCtrlGetDisplayAttribute (NvCtrlAttributeHandle *handle,
                           unsigned int display_mask, int attr, int *val);
ReturnStatus
NvCtrlSetDisplayAttribute (NvCtrlAttributeHandle *handle,
                           unsigned int display_mask, int attr, int val);

ReturnStatus
NvCtrlGetDisplayAttribute64 (NvCtrlAttributeHandle *handle,
                             unsigned int display_mask, int attr, int64_t *val);

ReturnStatus
NvCtrlSetDisplayAttributeWithReply (NvCtrlAttributeHandle *handle,
                                    unsigned int display_mask,
                                    int attr, int val);

ReturnStatus
NvCtrlGetVoidDisplayAttribute (NvCtrlAttributeHandle *handle,
                               unsigned int display_mask,
                               int attr, void **val);

ReturnStatus
NvCtrlGetValidDisplayAttributeValues (NvCtrlAttributeHandle *handle,
                                      unsigned int display_mask, int attr,
                                      NVCTRLAttributeValidValuesRec *val);
ReturnStatus
NvCtrlGetValidStringDisplayAttributeValues (NvCtrlAttributeHandle *handle,
                                            unsigned int display_mask, int attr,
                                            NVCTRLAttributeValidValuesRec *val);

ReturnStatus
NvCtrlGetStringDisplayAttribute (NvCtrlAttributeHandle *handle,
                                 unsigned int display_mask,
                                 int attr, char **ptr);

ReturnStatus
NvCtrlSetStringDisplayAttribute (NvCtrlAttributeHandle *handle,
                                 unsigned int display_mask,
                                 int attr, char *ptr, int *ret);

ReturnStatus
NvCtrlGetBinaryAttribute(NvCtrlAttributeHandle *handle,
                         unsigned int display_mask, int attr,
                         unsigned char **data, int *len);

/*
 * NvCtrlStringOperation() - Performs the string operation associated
 * with the specified attribute, where valid values are the
 * NV_CTRL_STRING_OPERATION_* #defines in NVCtrl.h. If 'ptrOut'
 * is specified, (string) result information is returned.
 */

ReturnStatus
NvCtrlStringOperation(NvCtrlAttributeHandle *handle,
                      unsigned int display_mask, int attr,
                      char *ptrIn, char **ptrOut);

/*
 * NvCtrl[SG]etGvoColorConversion() - get and set the color conversion
 * matrix and offset used in the Graphics to Video Out (GVO)
 * extension.  These should only be used if the NV_CTRL_GVO_SUPPORTED
 * attribute is TRUE.
 */

ReturnStatus
NvCtrlSetGvoColorConversion(NvCtrlAttributeHandle *handle,
                            float colorMatrix[3][3],
                            float colorOffset[3],
                            float colorScale[3]);

ReturnStatus
NvCtrlGetGvoColorConversion(NvCtrlAttributeHandle *handle,
                            float colorMatrix[3][3],
                            float colorOffset[3],
                            float colorScale[3]);

const char *NvCtrlGetMultisampleModeName(int multisample_mode);

char *NvCtrlAttributesStrError (ReturnStatus status);

void NvCtrlAttributeClose(NvCtrlAttributeHandle *handle);

ReturnStatus
NvCtrlXrandrSetScreenMode (NvCtrlAttributeHandle *handle,
                           int width, int height, int refresh);

ReturnStatus
NvCtrlXrandrGetScreenMode (NvCtrlAttributeHandle *handle,
                           int *width, int *height, int *refresh);

#endif /* __NVCTRL_ATTRIBUTES__ */
