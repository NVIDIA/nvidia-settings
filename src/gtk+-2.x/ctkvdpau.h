/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2012 NVIDIA Corporation.
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

#ifndef __CTK_VDPAU_H__
#define __CTK_VDPAU_H__

#include "ctkevent.h"
#include "ctkconfig.h"

#include "vdpau/vdpau.h"
#include "vdpau/vdpau_x11.h"

G_BEGIN_DECLS

#define CTK_TYPE_VDPAU (ctk_vdpau_get_type())

#define CTK_VDPAU(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_VDPAU, CtkVDPAU))

#define CTK_VDPAU_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_VDPAU, CtkVDPAUClass))

#define CTK_IS_VDPAU(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_VDPAU))

#define CTK_IS_VDPAU_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_VDPAU))

#define CTK_VDPAU_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_VDPAU, CtkVDPAUClass))


struct VDPAUDeviceImpl {

    VdpGetErrorString *GetErrorString;
    VdpGetProcAddress *GetProcAddress;
    VdpGetApiVersion *GetApiVersion;
    VdpGetInformationString *GetInformationString;
    VdpVideoSurfaceQueryCapabilities *VideoSurfaceQueryCapabilities;
    VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities
        *VideoSurfaceQueryGetPutBitsYCbCrCapabilities;
    VdpOutputSurfaceQueryCapabilities *OutputSurfaceQueryCapabilities;
    VdpOutputSurfaceQueryGetPutBitsNativeCapabilities
        *OutputSurfaceQueryGetPutBitsNativeCapabilities;
    VdpOutputSurfaceQueryPutBitsYCbCrCapabilities
        *OutputSurfaceQueryPutBitsYCbCrCapabilities;
    VdpBitmapSurfaceQueryCapabilities *BitmapSurfaceQueryCapabilities;
    VdpDecoderQueryCapabilities *DecoderQueryCapabilities;
    VdpVideoMixerQueryFeatureSupport *VideoMixerQueryFeatureSupport;
    VdpVideoMixerQueryParameterSupport *VideoMixerQueryParameterSupport;
    VdpVideoMixerQueryAttributeSupport *VideoMixerQueryAttributeSupport;
    VdpVideoMixerQueryParameterValueRange *VideoMixerQueryParameterValueRange;
    VdpVideoMixerQueryAttributeValueRange *VideoMixerQueryAttributeValueRange;
} VDPAUDeviceFunctions;

/* Generic description structure */
typedef struct
{
    const char *name;
    uint32_t id;
    uint32_t aux; /* optional extra parameter... */
} Desc;

typedef struct _CtkVDPAU       CtkVDPAU;
typedef struct _CtkVDPAUClass  CtkVDPAUClass;

struct _CtkVDPAU
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget* notebook;
    GtkWidget* surfaceVbox;
    GtkWidget* baseInfoVbox;
};

struct _CtkVDPAUClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_vdpau_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_vdpau_new         (NvCtrlAttributeHandle *,
                                      CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_vdpau_create_help (GtkTextTagTable *, CtkVDPAU *);

G_END_DECLS

#endif /* __CTK_VDPAU_H__ */
