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

#ifndef __NVCTRL_ATTRIBUTES_PRIVATE__
#define __NVCTRL_ATTRIBUTES_PRIVATE__

#include "NvCtrlAttributes.h"
#include "NVCtrl.h"

#define EXTENSION_NV_CONTROL  0x1
#define EXTENSION_XF86VIDMODE 0x2
#define EXTENSION_XVIDEO      0x4

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
#define NV_MINMINOR 6

/* minimum required version for the XF86VidMode extension */

#define VM_MINMAJOR 2
#define VM_MINMINOR 1

/* minimum required version for the XVideo extension */

#define XV_MINMAJOR 2
#define XV_MINMINOR 0


typedef struct __NvCtrlAttributes NvCtrlAttributes;
typedef struct __NvCtrlVidModeAttributes NvCtrlVidModeAttributes;
typedef struct __NvCtrlAttributePrivateHandle NvCtrlAttributePrivateHandle;
typedef struct __NvCtrlNvControlAttributes NvCtrlNvControlAttributes;
typedef struct __NvCtrlXvOverlayAttributes NvCtrlXvOverlayAttributes;
typedef struct __NvCtrlXvTextureAttributes NvCtrlXvTextureAttributes;
typedef struct __NvCtrlXvBlitterAttributes NvCtrlXvBlitterAttributes;
typedef struct __NvCtrlXvAttribute NvCtrlXvAttribute;

struct __NvCtrlNvControlAttributes {
    int event_base;
    int error_base;
};

struct __NvCtrlVidModeAttributes {
    int n;
    int sigbits;
    unsigned short *lut[3];
    float brightness[3];
    float contrast[3];
    float gamma[3];
};

struct __NvCtrlXvAttribute {
    Atom atom;
    NVCTRLAttributeValidValuesRec range;
};

struct __NvCtrlXvOverlayAttributes {
    unsigned int port;
    NvCtrlXvAttribute *saturation;
    NvCtrlXvAttribute *contrast;
    NvCtrlXvAttribute *brightness;
    NvCtrlXvAttribute *hue;
    NvCtrlXvAttribute *defaults;
};

struct __NvCtrlXvTextureAttributes {
    unsigned int port;
    NvCtrlXvAttribute *sync_to_vblank;
    NvCtrlXvAttribute *defaults;
};

struct __NvCtrlXvBlitterAttributes {
    unsigned int port;
    NvCtrlXvAttribute *sync_to_vblank;
    NvCtrlXvAttribute *defaults;
};

struct __NvCtrlAttributePrivateHandle {
    Display *dpy;                  /* display connection */
    int screen;                    /* the screen that this handle controls */
    NvCtrlNvControlAttributes *nv; /* NV-CONTROL extension info */
    NvCtrlVidModeAttributes *vm;   /* XF86VidMode extension info */    
    NvCtrlXvOverlayAttributes *xv_overlay; /* XVideo info (overlay) */
    NvCtrlXvTextureAttributes *xv_texture; /* XVideo info (texture) */
    NvCtrlXvBlitterAttributes *xv_blitter; /* XVideo info (blitter) */
};

NvCtrlNvControlAttributes *
NvCtrlInitNvControlAttributes (NvCtrlAttributePrivateHandle *);

NvCtrlVidModeAttributes *
NvCtrlInitVidModeAttributes (NvCtrlAttributePrivateHandle *);

void NvCtrlInitXvOverlayAttributes (NvCtrlAttributePrivateHandle *);

ReturnStatus
NvCtrlNvControlGetAttribute (NvCtrlAttributePrivateHandle *, unsigned int,
                             int, int *);

ReturnStatus
NvCtrlNvControlSetAttribute (NvCtrlAttributePrivateHandle *, unsigned int,
                             int, int);

ReturnStatus
NvCtrlNvControlGetValidAttributeValues (NvCtrlAttributePrivateHandle *,
                                        unsigned int, int,
                                        NVCTRLAttributeValidValuesRec *);
ReturnStatus
NvCtrlNvControlGetStringAttribute (NvCtrlAttributePrivateHandle *,
                                   unsigned int, int, char **);

ReturnStatus
NvCtrlXvGetAttribute (NvCtrlAttributePrivateHandle *, int, int *);

ReturnStatus
NvCtrlXvSetAttribute (NvCtrlAttributePrivateHandle *, int, int);

ReturnStatus
NvCtrlXvGetValidAttributeValues (NvCtrlAttributePrivateHandle *, int,
                                 NVCTRLAttributeValidValuesRec *);

#endif /* __NVCTRL_ATTRIBUTES_PRIVATE__ */
