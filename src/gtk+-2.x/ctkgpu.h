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

#ifndef __CTK_GPU_H__
#define __CTK_GPU_H__

#include <gtk/gtk.h>
#include <query-assign.h>

#include "ctkevent.h"
#include "ctkconfig.h"

#include "NvCtrlAttributes.h"

G_BEGIN_DECLS

#define CTK_TYPE_GPU (ctk_gpu_get_type())

#define CTK_GPU(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GPU, CtkGpu))

#define CTK_GPU_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GPU, CtkGpuClass))

#define CTK_IS_GPU(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GPU))

#define CTK_IS_GPU_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GPU))

#define CTK_GPU_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GPU, CtkGpuClass))


typedef struct _CtkGpu       CtkGpu;
typedef struct _CtkGpuClass  CtkGpuClass;

struct _CtkGpu
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *bus_label;
    GtkWidget *link_speed_label;
    GtkWidget *displays;
    gint gpu_cores;
    gint memory_interface;
    gboolean pcie_gen_queriable;
};

struct _CtkGpuClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_gpu_get_type (void) G_GNUC_CONST;
GtkWidget*  ctk_gpu_new      (NvCtrlAttributeHandle *handle,
                              CtrlHandleTarget *t,
                              CtkEvent *ctk_event,
                              CtkConfig *ctk_config);

void get_bus_type_str(NvCtrlAttributeHandle *handle,
                      gchar **bus);
void get_bus_id_str(NvCtrlAttributeHandle *handle,
                    gchar **pci_bus_id);

GtkTextBuffer *ctk_gpu_create_help(GtkTextTagTable *,
                                   CtkGpu *);

void ctk_gpu_start_timer(GtkWidget *widget);
void ctk_gpu_stop_timer(GtkWidget *widget);

G_END_DECLS

#endif /* __CTK_GPU_H__ */

