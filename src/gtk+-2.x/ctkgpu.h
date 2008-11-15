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

#ifndef __CTK_GPU_H__
#define __CTK_GPU_H__

#include <gtk/gtk.h>
#include <query-assign.h>

#include "ctkevent.h"

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

    GtkWidget *displays;
};

struct _CtkGpuClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_gpu_get_type (void) G_GNUC_CONST;
GtkWidget*  ctk_gpu_new      (NvCtrlAttributeHandle *handle,
                              CtrlHandleTarget *t,
                              CtkEvent *ctk_event);

GtkTextBuffer *ctk_gpu_create_help(GtkTextTagTable *);



G_END_DECLS

#endif /* __CTK_GPU_H__ */

