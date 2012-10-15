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


typedef struct _CtkDisplayDevice       CtkDisplayDevice;
typedef struct _CtkDisplayDeviceClass  CtkDisplayDeviceClass;

typedef struct InfoEntryRec {
    gboolean present;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *txt;

    struct _CtkDisplayDevice *ctk_object;

} InfoEntry;

struct _CtkDisplayDevice
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    CtkEvent *ctk_event_gpu;
    GtkWidget *image_sliders;
    GtkWidget *reset_button;
    GtkWidget *edid;
    GtkWidget *dithering_controls;
    GtkWidget *color_controls;

    InfoEntry *info_entries;
    int num_info_entries;

    gboolean display_enabled;
    unsigned int active_attributes;

    char *name;
    gint signal_type;
};

struct _CtkDisplayDeviceClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_display_device_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_display_device_new       (NvCtrlAttributeHandle *,
                                          CtkConfig *, CtkEvent *,
                                          CtkEvent *, char *, char *,
                                          ParsedAttribute *);

GtkTextBuffer *ctk_display_device_create_help(GtkTextTagTable *,
                                              CtkDisplayDevice *);

G_END_DECLS

#endif /* __CTK_DISPLAYDEVICE_H__ */
