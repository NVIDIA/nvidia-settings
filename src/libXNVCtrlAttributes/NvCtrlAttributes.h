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
#include "common-utils.h"


typedef void NvCtrlAttributeHandle;

#define NV_FALSE                0
#define NV_TRUE                 1


/*
 * Indices into both targetTypeTable[] and CtrlSystem->targets[] array.
 */

typedef enum {
    X_SCREEN_TARGET = 0,
    GPU_TARGET,
    FRAMELOCK_TARGET,
    VCS_TARGET,
    GVI_TARGET,
    COOLER_TARGET,
    THERMAL_SENSOR_TARGET,
    NVIDIA_3D_VISION_PRO_TRANSCEIVER_TARGET,
    DISPLAY_TARGET,

    MAX_TARGET_TYPES,
    INVALID_TARGET = -1
} CtrlTargetType;


/*
 * Defines the values associated with each target type.
 */

typedef struct {
    char *name;        /* full name for logging */
    char *parsed_name; /* name used by parser */
    int nvctrl;        /* NV-CONTROL target type value (NV_CTRL_TARGET_TYPE) */

    /* flag set in CtrlAttributePerms.valid_targets */
    unsigned int permission_bit;

    /* whether this target type is aware of display devices */
    int uses_display_devices;

    /*
     * the minimum NV-CONTROL Protocol version required to use this target
     * type; note that all future target types should be able to use 1.18,
     * since that version and later allows NV-CONTROL clients to query the
     * count of TargetTypes not recognized by the X server
     */

    int major;
    int minor;

} CtrlTargetTypeInfo;


enum {
    NV_DPY_PROTO_NAME_TYPE_BASENAME = 0,
    NV_DPY_PROTO_NAME_TYPE_ID,
    NV_DPY_PROTO_NAME_DP_GUID,
    NV_DPY_PROTO_NAME_EDID_HASH,
    NV_DPY_PROTO_NAME_TARGET_INDEX,
    NV_DPY_PROTO_NAME_RANDR,
    NV_DPY_PROTO_NAME_MAX,
};

enum {
    NV_GPU_PROTO_NAME_TYPE_ID = 0,
    NV_GPU_PROTO_NAME_UUID,
    NV_GPU_PROTO_NAME_MAX,
};

#define NV_PROTO_NAME_MAX (NV_MAX((int)NV_DPY_PROTO_NAME_MAX, (int)NV_GPU_PROTO_NAME_MAX))

typedef struct _CtrlTarget CtrlTarget;
typedef struct _CtrlTargetNode CtrlTargetNode;
typedef struct _CtrlSystem CtrlSystem;
typedef struct _CtrlSystemList CtrlSystemList;

struct _CtrlTarget {
    NvCtrlAttributeHandle *h; /* handle for this target */

    CtrlSystem *system; /* the system this target belongs to */
    const CtrlTargetTypeInfo *targetTypeInfo;

    unsigned int d; /* display device mask for this target */
    unsigned int c; /* Connected display device mask for target */
    char *name;     /* Name for this target */
    char *protoNames[NV_PROTO_NAME_MAX]; /* List of valid names for this target */

    struct {
        Bool connected; /* Connection state of display device */
        Bool enabled;   /* Enabled state of display device */
    } display;

    struct _CtrlTargetNode *relations; /* List of associated targets */
};

/* Used to keep track of lists of targets */
struct _CtrlTargetNode {
    CtrlTargetNode *next;
    CtrlTarget *t;
};

/* Tracks all the targets for a single system. Note that
 * targets[X_SCREEN_TARGET] only holds API X screens targets.
 * In order to query to physical X screens targets 'physical_screens'
 * must be used instead. 'physical_screens' do not keep tracking of
 * target relationships
 */
struct _CtrlSystem {
    /* X system data */
    char *display;  /* string for XOpenDisplay */
    Display *dpy;   /* X display connection */

    CtrlTargetNode *targets[MAX_TARGET_TYPES]; /* Shadows targetTypeTable */
    CtrlTargetNode *physical_screens;
    CtrlSystemList *system_list; /* pointer to the system list being tracked */
};

/* Tracks all systems referenced by command line and/or the configuration
 * file (.nvidia-settings.).
 */
struct _CtrlSystemList {
    int n;              /* number of systems */
    CtrlSystem **array; /* dynamically allocated array */
};


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

#define FIRST_COLOR_CHANNEL   RED_CHANNEL_INDEX
#define LAST_COLOR_CHANNEL    BLUE_CHANNEL_INDEX

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
 * Attribute types used to know how to access each attribute - for example, to
 * know which of the NvCtrlXXX() backend functions to call.
 */

typedef enum {
    CTRL_ATTRIBUTE_TYPE_INTEGER,
    CTRL_ATTRIBUTE_TYPE_STRING,
    CTRL_ATTRIBUTE_TYPE_STRING_OPERATION,
    CTRL_ATTRIBUTE_TYPE_BINARY_DATA,
    CTRL_ATTRIBUTE_TYPE_COLOR,
    CTRL_ATTRIBUTE_TYPE_SDI_CSC,
} CtrlAttributeType;


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

/* GLX */

#define NV_CTRL_ATTR_GLX_BASE \
       (NV_CTRL_ATTR_NV_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS  (NV_CTRL_ATTR_GLX_BASE +  0)

#define NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS)

/* RandR */

#define NV_CTRL_ATTR_RANDR_BASE \
       (NV_CTRL_ATTR_GLX_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE (NV_CTRL_ATTR_RANDR_BASE +  0)

#define NV_CTRL_ATTR_RANDR_LAST_ATTRIBUTE \
       (NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE)

/* NVML */

#define NV_CTRL_ATTR_NVML_BASE \
        (NV_CTRL_ATTR_RANDR_LAST_ATTRIBUTE + 1)

#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE               (NV_CTRL_ATTR_NVML_BASE + 0)
#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_NONE                                     0
#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_PASSTHROUGH                              1
#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_VGPU                                     2
#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_HOST_VGPU                                3
#define NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE_HOST_VSGA                                4

#define NV_CTRL_ATTR_NVML_LAST_ATTRIBUTE (NV_CTRL_ATTR_NVML_GPU_VIRTUALIZATION_MODE)

#define NV_CTRL_ATTR_LAST_ATTRIBUTE \
        (NV_CTRL_ATTR_NVML_LAST_ATTRIBUTE)


typedef enum {
    NvCtrlSuccess = 0,
    NvCtrlBadArgument,
    NvCtrlBadHandle,
    NvCtrlNoAttribute,
    NvCtrlMissingExtension,
    NvCtrlReadOnlyAttribute,
    NvCtrlWriteOnlyAttribute,
    NvCtrlAttributeNotAvailable,
    NvCtrlNotSupported,
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
 * Used to pack CtrlAttributePerms.valid_targets
 */
#define CTRL_TARGET_PERM_BIT(TARGET_TYPE) (1 << ((int)(TARGET_TYPE)))

typedef struct {

    int read  : 1;
    int write : 1;

    unsigned int valid_targets;

} CtrlAttributePerms;


/*
 * Used to return valid values of an attribute
 */
typedef enum {
    CTRL_ATTRIBUTE_VALID_TYPE_UNKNOWN = 0,
    CTRL_ATTRIBUTE_VALID_TYPE_INTEGER,
    CTRL_ATTRIBUTE_VALID_TYPE_BITMASK,
    CTRL_ATTRIBUTE_VALID_TYPE_BOOL,
    CTRL_ATTRIBUTE_VALID_TYPE_RANGE,
    CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS,
    CTRL_ATTRIBUTE_VALID_TYPE_64BIT_INTEGER,
    CTRL_ATTRIBUTE_VALID_TYPE_STRING,
    CTRL_ATTRIBUTE_VALID_TYPE_BINARY_DATA,
    CTRL_ATTRIBUTE_VALID_TYPE_STRING_OPERATION,
} CtrlAttributeValidType;

typedef struct {
    CtrlAttributeValidType valid_type;
    union {
        /* Only used by RANGE attributes */
        struct {
            int64_t min;
            int64_t max;
        } range;

        /* Only used by INT_BITS attributes */
        unsigned int allowed_ints;
    };
    CtrlAttributePerms permissions;
} CtrlAttributeValidValues;


/*
 * Event handle and event structure used to provide an event mechanism to
 * communicate different backends with the frontend
 */
typedef void NvCtrlEventHandle;

typedef enum {
    CTRL_EVENT_TYPE_UNKNOWN = 0,
    CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE,
    CTRL_EVENT_TYPE_STRING_ATTRIBUTE,
    CTRL_EVENT_TYPE_BINARY_ATTRIBUTE,
    CTRL_EVENT_TYPE_SCREEN_CHANGE
} CtrlEventType;

typedef struct {
    int  attribute;
    int  value;
    Bool is_availability_changed;
    Bool availability;
} CtrlEventIntAttribute;

typedef struct {
    int attribute;
} CtrlEventStrAttribute;

typedef struct {
    int attribute;
} CtrlEventBinAttribute;

typedef struct {
    int width;
    int height;
    int mwidth;
    int mheight;
} CtrlEventScreenChange;

typedef struct {
    CtrlEventType  type;
    CtrlTargetType target_type;
    int            target_id;

    union {
        CtrlEventIntAttribute int_attr;
        CtrlEventStrAttribute str_attr;
        CtrlEventBinAttribute bin_attr;
        CtrlEventScreenChange screen_change;
    };

} CtrlEvent;


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



#define NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM   0x1
#define NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM  0x2
#define NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM       0x4
#define NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM          0x8
#define NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM       0x10
#define NV_CTRL_ATTRIBUTES_NVML_SUBSYSTEM         0x20
#define NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS    \
 (NV_CTRL_ATTRIBUTES_NV_CONTROL_SUBSYSTEM  | \
  NV_CTRL_ATTRIBUTES_XF86VIDMODE_SUBSYSTEM | \
  NV_CTRL_ATTRIBUTES_XVIDEO_SUBSYSTEM      | \
  NV_CTRL_ATTRIBUTES_GLX_SUBSYSTEM         | \
  NV_CTRL_ATTRIBUTES_XRANDR_SUBSYSTEM      | \
  NV_CTRL_ATTRIBUTES_NVML_SUBSYSTEM)



CtrlSystem *NvCtrlConnectToSystem(const char *display, CtrlSystemList *systems);
CtrlSystem *NvCtrlGetSystem      (const char *display, CtrlSystemList *systems);
void        NvCtrlFreeAllSystems (CtrlSystemList *systems);


int         NvCtrlGetTargetTypeCount    (const CtrlSystem *system,
                                         CtrlTargetType target_type);
CtrlTarget *NvCtrlGetTarget             (const CtrlSystem *system,
                                         CtrlTargetType target_type,
                                         int target_id);
CtrlTarget *NvCtrlGetDefaultTarget      (const CtrlSystem *system);
CtrlTarget *NvCtrlGetDefaultTargetByType(const CtrlSystem *system,
                                         CtrlTargetType target_type);

Bool                      NvCtrlIsTargetTypeValid      (CtrlTargetType target_type);
const CtrlTargetTypeInfo *NvCtrlGetTargetTypeInfo      (CtrlTargetType target_type);
const CtrlTargetTypeInfo *NvCtrlGetTargetTypeInfoByName(const char *name);

void NvCtrlTargetListAdd (CtrlTargetNode **head,
                          CtrlTarget *target,
                          Bool enabled_display_check);
void NvCtrlTargetListFree(CtrlTargetNode *head);

/*
 *  XXX Changes to the system topology should not be allowed directly from the
 *      front-end
 */
CtrlTarget *nv_add_target(CtrlSystem *system, CtrlTargetType target_type, int target_id);

const char *NvCtrlGetDisplayConfigName(const CtrlSystem *system, int target_id);

void NvCtrlRebuildSubsystems(CtrlTarget *ctrl_target, unsigned int subsystem);

Display *NvCtrlGetDisplayPtr (CtrlTarget *ctrl_target);
char    *NvCtrlGetDisplayName(const CtrlTarget *ctrl_target);

int NvCtrlGetTargetType(const CtrlTarget *ctrl_target);
int NvCtrlGetTargetId  (const CtrlTarget *ctrl_target);

char *NvCtrlGetServerVendor    (const CtrlTarget *ctrl_target);
int   NvCtrlGetVendorRelease   (const CtrlTarget *ctrl_target);
int   NvCtrlGetProtocolVersion (const CtrlTarget *ctrl_target);
int   NvCtrlGetProtocolRevision(const CtrlTarget *ctrl_target);

int NvCtrlGetScreen        (const CtrlTarget *ctrl_target);
int NvCtrlGetScreenWidth   (const CtrlTarget *ctrl_target);
int NvCtrlGetScreenHeight  (const CtrlTarget *ctrl_target);
int NvCtrlGetScreenCount   (const CtrlTarget *ctrl_target);
int NvCtrlGetScreenWidthMM (const CtrlTarget *ctrl_target);
int NvCtrlGetScreenHeightMM(const CtrlTarget *ctrl_target);
int NvCtrlGetScreenPlanes  (const CtrlTarget *ctrl_target);

ReturnStatus NvCtrlGetColorAttributes(const CtrlTarget *ctrl_target,
                                      float contrast[3],
                                      float brightness[3],
                                      float gamma[3]);

ReturnStatus NvCtrlSetColorAttributes(CtrlTarget *ctrl_target,
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

ReturnStatus NvCtrlGetColorRamp(const CtrlTarget *ctrl_target,
                                unsigned int channel,
                                unsigned short **lut,
                                int *n);

/*
 * NvCtrlReloadColorRamp() - Reloads the current color ramp for all
 * channels for the given target.
 */

ReturnStatus NvCtrlReloadColorRamp(CtrlTarget *ctrl_target);

/*
 * NvCtrlQueryTargetCount() - query the number of targets available
 * on the server of the given target type.  This is used, for example
 * to return the number of GPUs the server knows about.
 */
ReturnStatus NvCtrlQueryTargetCount(const CtrlTarget *ctrl_target,
                                    CtrlTargetType target_type,
                                    int *val);

/*
 * NvCtrlGetAttribute()/NvCtrlSetAttribute() - these get and set
 * functions can be used to query and modify all integer attributes.
 * Modifications made via SetAttribute() are made immediately.  The
 * attr argument is the attribute to query/modify; valid attributes
 * are those listed in NVCtrl.h and the NV_CTRL_ attributes #define'd
 * above.  NvCtrlGetAttribute64() behaves like NvCtrlGetAttribute(),
 * but supports 64-bit integer attributes.
 */

ReturnStatus NvCtrlGetAttribute(const CtrlTarget *ctrl_target,
                                int attr, int *val);

ReturnStatus NvCtrlSetAttribute(CtrlTarget *ctrl_target, int attr, int val);

ReturnStatus NvCtrlGetAttribute64(const CtrlTarget *ctrl_target,
                                  int attr, int64_t *val);


/*
 * NvCtrlGetVoidAttribute() - this function works like the
 * Get and GetString only it returns a void pointer.  The
 * data type pointed to is dependent on which attribute you
 * are requesting.
 */

ReturnStatus NvCtrlGetVoidAttribute(const CtrlTarget *ctrl_target,
                                    int attr, void **ptr);


/*
 * NvCtrlGetValidAttributeValues() - get the valid settable values for
 * the specified attribute.
 */

ReturnStatus NvCtrlGetValidAttributeValues(const CtrlTarget *ctrl_target,
                                           int attr,
                                           CtrlAttributeValidValues *val);


/*
 * NvCtrlGetAttributePerms() - get the attribute permissions.
 */

ReturnStatus NvCtrlGetAttributePerms(const CtrlTarget *ctrl_target,
                                     CtrlAttributeType attr_type,
                                     int attr,
                                     CtrlAttributePerms *perms);


/*
 * NvCtrlGetStringAttribute() - get the string associated with the
 * specified attribute, where valid values are the NV_CTRL_STRING_
 * #defines in NVCtrl.h.
 */

ReturnStatus NvCtrlGetStringAttribute(const CtrlTarget *ctrl_target,
                                      int attr, char **ptr);

/*
 * NvCtrlSetStringAttribute() - Set the string associated with the
 * specified attribute, where valid values are the NV_CTRL_STRING_
 * #defines in NVCtrl.h that have the 'W' (Write) flag set.
 */

ReturnStatus NvCtrlSetStringAttribute(CtrlTarget *ctrl_target,
                                      int attr, const char *ptr);

/*
 * The following four functions are identical to the above five,
 * except that they specify a particular display mask.
 */

ReturnStatus NvCtrlGetDisplayAttribute(const CtrlTarget *ctrl_target,
                                       unsigned int display_mask,
                                       int attr, int *val);
ReturnStatus NvCtrlSetDisplayAttribute(CtrlTarget *ctrl_target,
                                       unsigned int display_mask,
                                       int attr, int val);

ReturnStatus NvCtrlGetDisplayAttribute64(const CtrlTarget *ctrl_target,
                                         unsigned int display_mask,
                                         int attr, int64_t *val);

ReturnStatus NvCtrlGetVoidDisplayAttribute(const CtrlTarget *ctrl_target,
                                           unsigned int display_mask,
                                           int attr, void **val);

ReturnStatus
NvCtrlGetValidDisplayAttributeValues(const CtrlTarget *ctrl_target,
                                     unsigned int display_mask, int attr,
                                     CtrlAttributeValidValues *val);
ReturnStatus
NvCtrlGetValidStringDisplayAttributeValues(const CtrlTarget *ctrl_target,
                                           unsigned int display_mask, int attr,
                                           CtrlAttributeValidValues *val);

ReturnStatus NvCtrlGetStringDisplayAttribute(const CtrlTarget *ctrl_target,
                                             unsigned int display_mask,
                                             int attr, char **ptr);

ReturnStatus NvCtrlSetStringDisplayAttribute(CtrlTarget *ctrl_target,
                                             unsigned int display_mask,
                                             int attr, const char *ptr);

ReturnStatus NvCtrlGetBinaryAttribute(const CtrlTarget *ctrl_target,
                                      unsigned int display_mask, int attr,
                                      unsigned char **data, int *len);

/*
 * NvCtrlStringOperation() - Performs the string operation associated
 * with the specified attribute, where valid values are the
 * NV_CTRL_STRING_OPERATION_* #defines in NVCtrl.h. If 'ptrOut'
 * is specified, (string) result information is returned.
 */

ReturnStatus NvCtrlStringOperation(CtrlTarget *ctrl_target,
                                   unsigned int display_mask, int attr,
                                   const char *ptrIn, char **ptrOut);

/*
 * NvCtrl[SG]etGvoColorConversion() - get and set the color conversion
 * matrix and offset used in the Graphics to Video Out (GVO)
 * extension.  These should only be used if the NV_CTRL_GVO_SUPPORTED
 * attribute is TRUE.
 */

ReturnStatus NvCtrlSetGvoColorConversion(CtrlTarget *ctrl_target,
                                         float colorMatrix[3][3],
                                         float colorOffset[3],
                                         float colorScale[3]);

ReturnStatus NvCtrlGetGvoColorConversion(const CtrlTarget *ctrl_target,
                                         float colorMatrix[3][3],
                                         float colorOffset[3],
                                         float colorScale[3]);

const char *NvCtrlGetStereoModeNameIfExists(int stereo_mode);
const char *NvCtrlGetStereoModeName(int stereo_mode);

const char *NvCtrlGetMultisampleModeName(int multisample_mode);

char *NvCtrlAttributesStrError (ReturnStatus status);

void NvCtrlAttributeClose(NvCtrlAttributeHandle *handle);


/*
 * NvCtrlGetEventHandle() - Returns the unique event handle associated with the
 * specified control target. If it does not exist, creates a new one.
 */
NvCtrlEventHandle *NvCtrlGetEventHandle(const CtrlTarget *ctrl_target);

/*
 * NvCtrlCloseEventHandle() - Closes and frees the previously allocated
 * resources of the specified event handle.
 */
ReturnStatus
NvCtrlCloseEventHandle(NvCtrlEventHandle *handle);

/*
 * NvCtrlEventHandleGetFD() - Get the file descriptor associated with the
 * specified event handle.
 */
ReturnStatus
NvCtrlEventHandleGetFD(NvCtrlEventHandle *handle, int *fd);

/*
 * NvCtrlEventHandlePending() - Check whether there are pending events or not in
 * the specified event handle.
 */
ReturnStatus
NvCtrlEventHandlePending(NvCtrlEventHandle *handle, Bool *pending);

/*
 * NvCtrlEventHandleNextEvent() - Get the next event data in the specified event
 * handle.
 */
ReturnStatus
NvCtrlEventHandleNextEvent(NvCtrlEventHandle *handle, CtrlEvent *event);



#endif /* __NVCTRL_ATTRIBUTES__ */
