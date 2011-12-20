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

#ifndef __CTK_DISPLAYDEVICE_DFP_H__
#define __CTK_DISPLAYDEVICE_DFP_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_DISPLAY_DEVICE_DFP (ctk_display_device_dfp_get_type())

#define CTK_DISPLAY_DEVICE_DFP(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DISPLAY_DEVICE_DFP, \
                                 CtkDisplayDeviceDfp))

#define CTK_DISPLAY_DEVICE_DFP_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DISPLAY_DEVICE_DFP, \
                              CtkDisplayDeviceDfpClass))

#define CTK_IS_DISPLAY_DEVICE_DFP(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DISPLAY_DEVICE_DFP))

#define CTK_IS_DISPLAY_DEVICE_DFP_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DISPLAY_DEVICE_DFP))

#define CTK_DISPLAY_DEVICE_DFP_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DISPLAY_DEVICE_DFP, \
                                CtkDisplayDeviceDfpClass))


typedef struct _CtkDisplayDeviceDfp       CtkDisplayDeviceDfp;
typedef struct _CtkDisplayDeviceDfpClass  CtkDisplayDeviceDfpClass;

struct _CtkDisplayDeviceDfp
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    GtkWidget *image_sliders;
    GtkWidget *reset_button;
    GtkWidget *edid_box;
    GtkWidget *edid;
    GtkWidget *dithering_controls;
    GtkWidget *color_controls;

    GtkWidget *txt_chip_location;
    GtkWidget *txt_link;
    GtkWidget *txt_signal;
    GtkWidget *txt_native_resolution;
    GtkWidget *txt_frontend_resolution;
    GtkWidget *txt_best_fit_resolution;
    GtkWidget *txt_backend_resolution;

    GtkWidget *txt_refresh_rate;
    GtkWidget *txt_scaling;

    GtkWidget *scaling_frame;
    GtkWidget *scaling_gpu_button;
    GtkWidget *scaling_method_buttons[NV_CTRL_GPU_SCALING_METHOD_ASPECT_SCALED];
    
    unsigned int display_device_mask;
    gboolean display_enabled;
    unsigned int active_attributes;

    char *name;
    gint default_scaling_target;
    gint default_scaling_method;
};

struct _CtkDisplayDeviceDfpClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_display_device_dfp_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_display_device_dfp_new       (NvCtrlAttributeHandle *,
                                              CtkConfig *, CtkEvent *,
                                              unsigned int, char *);

GtkTextBuffer *ctk_display_device_dfp_create_help(GtkTextTagTable *,
                                                  CtkDisplayDeviceDfp *);

G_END_DECLS

#endif /* __CTK_DISPLAYDEVICE_DFP_H__ */
