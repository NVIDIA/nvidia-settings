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

    GtkWidget *txt_chip_location;
    GtkWidget *txt_link;
    GtkWidget *txt_signal;

    GtkWidget *scaling_frame;
    GtkWidget *scaling_buttons[NV_CTRL_FLATPANEL_SCALING_ASPECT_SCALED+1];

    GtkWidget *dithering_frame;
    GtkWidget *dithering_buttons[NV_CTRL_FLATPANEL_DITHERING_DISABLED+1];
    
    unsigned int display_device_mask;
    gboolean display_enabled;
    unsigned int active_attributes;

    char *name;
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
