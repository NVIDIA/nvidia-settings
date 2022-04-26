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

#ifndef __NVCTRL_ATTRIBUTES_PRIVATE__
#define __NVCTRL_ATTRIBUTES_PRIVATE__

#include "NvCtrlAttributes.h"
#include "NVCtrl.h"
#include <GL/glx.h> /* GLX #defines */
#include <EGL/egl.h>
#include <X11/extensions/Xrandr.h> /* Xrandr */
#include <nvml.h>

/* Make sure we are compiling with XRandR version 1.2 or greater */
#define MIN_RANDR_MAJOR 1
#define MIN_RANDR_MINOR 2
#if (RANDR_MAJOR < MIN_RANDR_MAJOR) || ((RANDR_MAJOR == MIN_RANDR_MAJOR) && (RANDR_MINOR < MIN_RANDR_MINOR))
#error XRandR version 1.2 or greater is required.
#endif


#define EXTENSION_NV_CONTROL  0x1
#define EXTENSION_XF86VIDMODE 0x2
#define EXTENSION_XVIDEO      0x4
#define EXTENSION_GLX         0x8
#define EXTENSION_XRANDR      0x10

/* caps bits */

#define NV_XF86VM_NUM_BITS      1
#define NV_CTRL_NUM_BITS        (NV_CTRL_LAST_ATTRIBUTE + 1)
#define NV_CTRL_STRING_NUM_BITS (NV_CTRL_STRING_LAST_ATTRIBUTE + 1)


#define NV_XF86VM_CAPS_OFFSET      0
#define NV_CTRL_CAPS_OFFSET        NV_XF86VM_NUM_BITS
#define NV_CTRL_STRING_CAPS_OFFSET (NV_XF86VM_NUM_BITS + NV_CTRL_NUM_BITS)




#define CAPS_XF86VM_GAMMA       (1 << (0x1 + NV_XF86VM_CAPS_OFFSET))


/* minimum required version for the NV-CONTROL extension */

#define NV_MINMAJOR 1
#define NV_MINMINOR 11

/* minimum required version for the XF86VidMode extension */

#define VM_MINMAJOR 2
#define VM_MINMINOR 1

/* minimum required version for the XVideo extension */

#define XV_MINMAJOR 2
#define XV_MINMINOR 0


/* Use to resolve symbols in dynamically opened libraries */

#define NV_DLSYM(handle, symbol) ({ dlerror(); dlsym(handle, symbol); })


/* Useful macros to deal with target types */

/* XXX Modify to be TRUE only for target types which are NVML-related */
#define TARGET_TYPE_IS_NVML_COMPATIBLE(_TARGET_TYPE_) \
    (((_TARGET_TYPE_) == GPU_TARGET) ||               \
     ((_TARGET_TYPE_) == THERMAL_SENSOR_TARGET) ||    \
     ((_TARGET_TYPE_) == COOLER_TARGET) ||            \
     ((_TARGET_TYPE_) == MUX_TARGET))

#define TARGET_TYPE_NEEDS_NVCONTROL(_TARGET_TYPE_)     \
    (!(TARGET_TYPE_IS_NVML_COMPATIBLE(_TARGET_TYPE_)))


/* Useful macros to deal with attribute names */

#define ATTRIBUTE_NAME(_ATTR_, _ATTR_TYPE_)                  \
    ((nv_get_attribute_entry(_ATTR_, _ATTR_TYPE_) != NULL) ? \
     (nv_get_attribute_entry(_ATTR_, _ATTR_TYPE_)->name) :   \
     ("Unknown"))

#define INT_ATTRIBUTE_NAME(_ATTR_) ATTRIBUTE_NAME(_ATTR_, CTRL_ATTRIBUTE_TYPE_INTEGER)
#define STR_ATTRIBUTE_NAME(_ATTR_) ATTRIBUTE_NAME(_ATTR_, CTRL_ATTRIBUTE_TYPE_STRING)
#define SOP_ATTRIBUTE_NAME(_ATTR_) ATTRIBUTE_NAME(_ATTR_, CTRL_ATTRIBUTE_TYPE_STRING_OPERATION)
#define BIN_ATTRIBUTE_NAME(_ATTR_) ATTRIBUTE_NAME(_ATTR_, CTRL_ATTRIBUTE_TYPE_BINARY_DATA)
#define COL_ATTRIBUTE_NAME(_ATTR_) ATTRIBUTE_NAME(_ATTR_, CTRL_ATTRIBUTE_TYPE_COLOR)


typedef struct __NvCtrlAttributes NvCtrlAttributes;
typedef struct __NvCtrlVidModeAttributes NvCtrlVidModeAttributes;
typedef struct __NvCtrlAttributePrivateHandle NvCtrlAttributePrivateHandle;
typedef struct __NvCtrlNvControlAttributes NvCtrlNvControlAttributes;
typedef struct __NvCtrlXvAttributes NvCtrlXvAttributes;
typedef struct __NvCtrlXvOverlayAttributes NvCtrlXvOverlayAttributes;
typedef struct __NvCtrlXvTextureAttributes NvCtrlXvTextureAttributes;
typedef struct __NvCtrlXvBlitterAttributes NvCtrlXvBlitterAttributes;
typedef struct __NvCtrlXvAttribute NvCtrlXvAttribute;
typedef struct __NvCtrlXrandrAttributes NvCtrlXrandrAttributes;
typedef struct __NvCtrlNvmlAttributes NvCtrlNvmlAttributes;
typedef struct __NvCtrlEventPrivateHandle NvCtrlEventPrivateHandle;
typedef struct __NvCtrlEventPrivateHandleNode NvCtrlEventPrivateHandleNode;

typedef struct {
    float brightness[3];
    float contrast[3];
    float gamma[3];
} NvCtrlGammaInput;

struct __NvCtrlNvControlAttributes {
    int event_base;
    int error_base;
    int major_version;
    int minor_version;
};

struct __NvCtrlVidModeAttributes {
    int major_version;
    int minor_version;
    unsigned short *lut[3];
    int gammaRampSize;
    NvCtrlGammaInput gammaInput;
};

struct __NvCtrlXvAttributes {
    unsigned int major_version;
    unsigned int minor_version;
    Bool overlay; 
    Bool texture;
    Bool blitter;
};

struct __NvCtrlXrandrAttributes {
    int event_base;
    int error_base;
    int major_version;
    int minor_version;
    Bool gammaAvailable;
    RRCrtc gammaCrtc;
    NvCtrlGammaInput gammaInput;
    XRRCrtcGamma *pGammaRamp;
};

struct __NvCtrlNvmlAttributes {
    struct {
        void *handle;

        typeof(nvmlInit)                                (*init);
        typeof(nvmlShutdown)                            (*shutdown);
        typeof(nvmlDeviceGetHandleByIndex)              (*deviceGetHandleByIndex);
        typeof(nvmlDeviceGetUUID)                       (*deviceGetUUID);
        typeof(nvmlDeviceGetCount)                      (*deviceGetCount);
        typeof(nvmlDeviceGetTemperature)                (*deviceGetTemperature);
        typeof(nvmlDeviceGetName)                       (*deviceGetName);
        typeof(nvmlDeviceGetVbiosVersion)               (*deviceGetVbiosVersion);
        typeof(nvmlDeviceGetMemoryInfo)                 (*deviceGetMemoryInfo);
        typeof(nvmlDeviceGetMemoryInfo_v2)              (*deviceGetMemoryInfo_v2);
        typeof(nvmlDeviceGetPciInfo)                    (*deviceGetPciInfo);
        typeof(nvmlDeviceGetCurrPcieLinkWidth)          (*deviceGetCurrPcieLinkWidth);
        typeof(nvmlDeviceGetMaxPcieLinkGeneration)      (*deviceGetMaxPcieLinkGeneration);
        typeof(nvmlDeviceGetMaxPcieLinkWidth)           (*deviceGetMaxPcieLinkWidth);
        typeof(nvmlDeviceGetVirtualizationMode)         (*deviceGetVirtualizationMode);
        typeof(nvmlDeviceGetGridLicensableFeatures_v4)  (*deviceGetGridLicensableFeatures);
        typeof(nvmlDeviceGetGspFirmwareMode)            (*deviceGetGspFirmwareMode);
        typeof(nvmlDeviceGetUtilizationRates)           (*deviceGetUtilizationRates);
        typeof(nvmlDeviceGetTemperatureThreshold)       (*deviceGetTemperatureThreshold);
        typeof(nvmlDeviceGetFanSpeed_v2)                (*deviceGetFanSpeed_v2);
        typeof(nvmlSystemGetDriverVersion)              (*systemGetDriverVersion);
        typeof(nvmlDeviceGetEccMode)                    (*deviceGetEccMode);
        typeof(nvmlDeviceSetEccMode)                    (*deviceSetEccMode);
        typeof(nvmlDeviceGetTotalEccErrors)             (*deviceGetTotalEccErrors);
        typeof(nvmlDeviceClearEccErrorCounts)           (*deviceClearEccErrorCounts);
        typeof(nvmlDeviceGetMemoryErrorCounter)         (*deviceGetMemoryErrorCounter);
        typeof(nvmlSystemGetNVMLVersion)                (*systemGetNVMLVersion);
        typeof(nvmlDeviceGetNumGpuCores)                (*deviceGetNumGpuCores);
        typeof(nvmlDeviceGetMemoryBusWidth)             (*deviceGetMemoryBusWidth);
        typeof(nvmlDeviceGetIrqNum)                     (*deviceGetIrqNum);
        typeof(nvmlDeviceGetPowerSource)                (*deviceGetPowerSource);
        typeof(nvmlDeviceGetNumFans)                    (*deviceGetNumFans);

    } lib;

    unsigned int deviceIdx; /* XXX Needed while using NV-CONTROL as fallback */
    unsigned int deviceCount;
    unsigned int sensorCount;
    unsigned int *sensorCountPerGPU;
    unsigned int coolerCount;
    unsigned int *coolerCountPerGPU;
};

struct __NvCtrlAttributePrivateHandle {
    Display *dpy;                   /* display connection */
    CtrlTargetType target_type;     /* Type of target this handle controls */
    int target_id;                  /* screen num, gpu num (etc) of target */

    /* Common attributes */
    NvCtrlNvControlAttributes *nv;  /* NV-CONTROL extension info */

    /* Screen-specific attributes */
    NvCtrlVidModeAttributes *vm;    /* XF86VidMode extension info */
    NvCtrlXvAttributes *xv;         /* XVideo info */
    Bool glx;                       /* GLX extension available */
    Bool egl;                       /* EGL extension available */
    EGLDisplay egl_dpy;
    NvCtrlXrandrAttributes *xrandr; /* XRandR extension info */

    /* NVML-specific attributes */
    NvCtrlNvmlAttributes *nvml;
};

struct __NvCtrlEventPrivateHandle {
    Display *dpy;          /* display connection */
    int fd;                /* file descriptor to poll for new events */
    int nvctrl_event_base; /* NV-CONTROL base for indexing & identifying evts */
    int xrandr_event_base; /* RandR base for indexing & identifying evts */
};

struct __NvCtrlEventPrivateHandleNode {
    NvCtrlEventPrivateHandle *handle;
    NvCtrlEventPrivateHandleNode *next;
};


/* Useful inline functions to deal with control targets */

static inline Bool isTargetValid(const CtrlTarget *ctrl_target)
{
    return ((ctrl_target != NULL) && (ctrl_target->h != NULL));
}

static inline NvCtrlAttributePrivateHandle
*getPrivateHandle(CtrlTarget *ctrl_target)
{
    if (!isTargetValid(ctrl_target)) {
        return NULL;
    }

    return (NvCtrlAttributePrivateHandle *)(ctrl_target->h);
}

static inline const NvCtrlAttributePrivateHandle
*getPrivateHandleConst(const CtrlTarget *ctrl_target)
{
    if (!isTargetValid(ctrl_target)) {
        return NULL;
    }

    return (const NvCtrlAttributePrivateHandle *)(ctrl_target->h);
}


NvCtrlNvControlAttributes *
NvCtrlInitNvControlAttributes (NvCtrlAttributePrivateHandle *);

NvCtrlVidModeAttributes *
NvCtrlInitVidModeAttributes (NvCtrlAttributePrivateHandle *);

ReturnStatus
NvCtrlFreeVidModeAttributes (NvCtrlAttributePrivateHandle *);


/* Xv attribute functions */

NvCtrlXvAttributes *
NvCtrlInitXvAttributes (NvCtrlAttributePrivateHandle *);

void
NvCtrlXvAttributesClose (NvCtrlAttributePrivateHandle *);

ReturnStatus NvCtrlXvGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                                        unsigned int, int, char **);

/* GLX extension attribute functions */

Bool
NvCtrlInitGlxAttributes (NvCtrlAttributePrivateHandle *);

void
NvCtrlGlxAttributesClose (NvCtrlAttributePrivateHandle *);

ReturnStatus NvCtrlGlxGetVoidAttribute(const NvCtrlAttributePrivateHandle *,
                                       unsigned int, int, void **);

ReturnStatus NvCtrlGlxGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                                         unsigned int, int, char **);

/* EGL extension attribute functions */

Bool NvCtrlInitEglAttributes(NvCtrlAttributePrivateHandle *);

void NvCtrlEglAttributesClose(NvCtrlAttributePrivateHandle *);

ReturnStatus NvCtrlEglGetVoidAttribute(const NvCtrlAttributePrivateHandle *,
                                       unsigned int, int, void **);

ReturnStatus NvCtrlEglGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                                         unsigned int, int, char **);

/* XRandR extension attribute functions */

NvCtrlXrandrAttributes *
NvCtrlInitXrandrAttributes (NvCtrlAttributePrivateHandle *);

void
NvCtrlXrandrAttributesClose (NvCtrlAttributePrivateHandle *);

ReturnStatus
NvCtrlXrandrGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                               unsigned int, int, char **);


ReturnStatus NvCtrlXrandrGetColorAttributes(const NvCtrlAttributePrivateHandle *h,
                                            float contrast[3],
                                            float brightness[3],
                                            float gamma[3]);

ReturnStatus NvCtrlXrandrSetColorAttributes(NvCtrlAttributePrivateHandle *h,
                                            float c[3],
                                            float b[3],
                                            float g[3],
                                            unsigned int bitmask);

ReturnStatus NvCtrlXrandrGetColorRamp(const NvCtrlAttributePrivateHandle *h,
                                      unsigned int channel,
                                      uint16_t **lut,
                                      int *n);

ReturnStatus NvCtrlXrandrReloadColorRamp(NvCtrlAttributePrivateHandle *h);

/* XF86 Video Mode extension attribute functions */

ReturnStatus NvCtrlVidModeGetColorAttributes(const NvCtrlAttributePrivateHandle *h,
                                             float contrast[3],
                                             float brightness[3],
                                             float gamma[3]);

ReturnStatus NvCtrlVidModeSetColorAttributes(NvCtrlAttributePrivateHandle *h,
                                             float c[3],
                                             float b[3],
                                             float g[3],
                                             unsigned int bitmask);

ReturnStatus NvCtrlVidModeGetColorRamp(const NvCtrlAttributePrivateHandle *h,
                                       unsigned int channel,
                                       uint16_t **lut,
                                       int *n);

ReturnStatus NvCtrlVidModeReloadColorRamp(NvCtrlAttributePrivateHandle *h);

ReturnStatus
NvCtrlVidModeGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                                unsigned int, int, char **);

ReturnStatus
NvCtrlXrandrGetAttribute(const NvCtrlAttributePrivateHandle *h,
                         unsigned int display_mask, int attr, int64_t *val);

/* Generic attribute functions */

NvCtrlAttributeHandle *NvCtrlAttributeInit(CtrlSystem *system,
                                           CtrlTargetType target_type,
                                           int target_id,
                                           unsigned int subsystems);

ReturnStatus
NvCtrlNvControlQueryTargetCount(const NvCtrlAttributePrivateHandle *, int,
                                int *);

ReturnStatus NvCtrlNvControlGetAttribute(const NvCtrlAttributePrivateHandle *,
                                         unsigned int, int, int64_t *);

ReturnStatus
NvCtrlNvControlSetAttribute (NvCtrlAttributePrivateHandle *, unsigned int,
                             int, int);

ReturnStatus
NvCtrlNvControlSetAttributeWithReply (NvCtrlAttributePrivateHandle *,
                                      unsigned int, int, int);

ReturnStatus
NvCtrlNvControlGetAttributePerms(const NvCtrlAttributePrivateHandle *,
                                 CtrlAttributeType, int,
                                 CtrlAttributePerms *);

ReturnStatus
NvCtrlNvControlGetValidAttributeValues(const NvCtrlAttributePrivateHandle *,
                                       unsigned int, int,
                                       CtrlAttributeValidValues *);

ReturnStatus
NvCtrlNvControlGetValidStringDisplayAttributeValues
                                      (const NvCtrlAttributePrivateHandle *,
                                       unsigned int, int,
                                       CtrlAttributeValidValues *);

ReturnStatus
NvCtrlNvControlGetStringAttribute(const NvCtrlAttributePrivateHandle *,
                                  unsigned int, int, char **);

ReturnStatus
NvCtrlNvControlSetStringAttribute (NvCtrlAttributePrivateHandle *,
                                   unsigned int, int, const char *);

ReturnStatus
NvCtrlNvControlGetBinaryAttribute(const NvCtrlAttributePrivateHandle *h,
                                  unsigned int display_mask, int attr,
                                  unsigned char **data, int *len);

ReturnStatus
NvCtrlNvControlStringOperation (NvCtrlAttributePrivateHandle *h,
                                unsigned int display_mask, int attr,
                                const char *ptrIn, char **ptrOut);

/* helper functions for XV86VidMode and RandR backends */

void NvCtrlInitGammaInputStruct(NvCtrlGammaInput *pGammaInput);

void NvCtrlUpdateGammaRamp(const NvCtrlGammaInput *pGammaInput,
                           int gammaRampSize,
                           unsigned short *gammaRamp[3],
                           unsigned int bitmask);

void NvCtrlAssignGammaInput(NvCtrlGammaInput *pGammaInput,
                            const float inContrast[3],
                            const float inBrightness[3],
                            const float inGamma[3],
                            const unsigned int bitmask);

/* NVML backend functions */

NvCtrlNvmlAttributes *NvCtrlInitNvmlAttributes(NvCtrlAttributePrivateHandle *);
void                  NvCtrlNvmlAttributesClose(NvCtrlAttributePrivateHandle *);

ReturnStatus NvCtrlNvmlQueryTargetCount(const CtrlTarget *ctrl_target,
                                        int target_type, int *val);
ReturnStatus NvCtrlNvmlGetStringAttribute(const CtrlTarget *ctrl_target,
                                          int attr, char **ptr);
ReturnStatus NvCtrlNvmlSetStringAttribute(CtrlTarget *ctrl_target,
                                          int attr, const char *ptr);
ReturnStatus NvCtrlNvmlGetAttribute(const CtrlTarget *ctrl_target,
                                    int attr, int64_t *val);
ReturnStatus NvCtrlNvmlGetGridLicenseAttributes(const CtrlTarget *ctrl_target,
                                                int attr, nvmlGridLicensableFeatures_t **val);
ReturnStatus NvCtrlNvmlDeviceGetGspAttributes(const CtrlTarget *ctrl_target, int attr,
                                              unsigned int *isEnabled, unsigned int *defaultMode);
ReturnStatus NvCtrlNvmlSetAttribute(CtrlTarget *ctrl_target, int attr,
                                    int index, int val);
ReturnStatus
NvCtrlNvmlGetBinaryAttribute(const CtrlTarget *ctrl_target,
                             int attr, unsigned char **data, int *len);
ReturnStatus
NvCtrlNvmlGetValidStringAttributeValues(const CtrlTarget *ctrl_target,
                                        int attr,
                                        CtrlAttributeValidValues *val);
ReturnStatus
NvCtrlNvmlGetValidAttributeValues(const CtrlTarget *ctrl_target,
                                  int attr,
                                  CtrlAttributeValidValues *val);

ReturnStatus
NvCtrlNvmlGetAttributePerms(const NvCtrlAttributePrivateHandle *,
                            CtrlAttributeType, int,
                            CtrlAttributePerms *);

#endif /* __NVCTRL_ATTRIBUTES_PRIVATE__ */
