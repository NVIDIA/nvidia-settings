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

#ifndef __CTK_DISPLAYDEVICE_H__
#define __CTK_DISPLAYDEVICE_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_DISPLAY_DEVICE (ctk_display_device_get_type())

#define CTK_DISPLAY_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DISPLAY_DEVICE, \
                                 CtkDisplayDevice))

#define CTK_DISPLAY_DEVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DISPLAY_DEVICE, \
                              CtkDisplayDeviceClass))

#define CTK_IS_DISPLAY_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DISPLAY_DEVICE))

#define CTK_IS_DISPLAY_DEVICE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DISPLAY_DEVICE))

#define CTK_DISPLAY_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DISPLAY_DEVICE, \
                                CtkDisplayDeviceClass))


#define CTK_DISPLAY_DEVICE_CRT_MASK 0x000000FF
#define CTK_DISPLAY_DEVICE_TV_MASK  0x0000FF00
#define CTK_DISPLAY_DEVICE_DFP_MASK 0x00FF0000


typedef struct _CtkDisplayDevice       CtkDisplayDevice;
typedef struct _CtkDisplayDeviceClass  CtkDisplayDeviceClass;

struct _CtkDisplayDevice
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    
    unsigned int enabled_display_devices;
    
    int num_display_devices;
    char *display_device_names[24];
    
};

struct _CtkDisplayDeviceClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_display_device_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_display_device_new       (NvCtrlAttributeHandle *,
                                          CtkConfig *, CtkEvent *);

GtkTextBuffer *ctk_display_device_create_help(GtkTextTagTable *,
                                              CtkDisplayDevice *);

G_END_DECLS

#endif /* __CTK_DISPLAYDEVICE_H__ */
