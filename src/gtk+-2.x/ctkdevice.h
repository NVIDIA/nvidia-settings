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

#ifndef __CTK_DEVICE_H__
#define __CTK_DEVICE_H__

#include <gtk/gtk.h>

#include "NvCtrlAttributes.h"

G_BEGIN_DECLS

#define CTK_TYPE_DEVICE (ctk_device_get_type())

#define CTK_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DEVICE, CtkDevice))

#define CTK_DEVICE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DEVICE, CtkDeviceClass))

#define CTK_IS_DEVICE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DEVICE))

#define CTK_IS_DEVICE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DEVICE))

#define CTK_DEVICE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DEVICE, CtkDeviceClass))


typedef struct _CtkDevice       CtkDevice;
typedef struct _CtkDeviceClass  CtkDeviceClass;

struct _CtkDevice
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
};

struct _CtkDeviceClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_device_get_type (void) G_GNUC_CONST;
GtkWidget*  ctk_device_new      (NvCtrlAttributeHandle *handle);

GtkTextBuffer *ctk_device_create_help(GtkTextTagTable *, const gchar *);

G_END_DECLS

#endif /* __CTK_DEVICE_H__ */

