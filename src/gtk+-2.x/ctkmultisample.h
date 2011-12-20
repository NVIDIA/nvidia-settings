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

#ifndef __CTK_MULTISAMPLE_H__
#define __CTK_MULTISAMPLE_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_MULTISAMPLE (ctk_multisample_get_type())

#define CTK_MULTISAMPLE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_MULTISAMPLE, CtkMultisample))

#define CTK_MULTISAMPLE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_MULTISAMPLE, \
                              CtkMultisampleClass))

#define CTK_IS_MULTISAMPLE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_MULTISAMPLE))

#define CTK_IS_MULTISAMPLE_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_MULTISAMPLE))

#define CTK_MULTISAMPLE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_MULTISAMPLE, \
                                CtkMultisampleClass))


typedef struct _CtkMultisample       CtkMultisample;
typedef struct _CtkMultisampleClass  CtkMultisampleClass;

struct _CtkMultisample
{
    GtkVBox parent;
    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *fsaa_app_override_check_button;
    GtkWidget *fsaa_menu;
    GtkWidget *fsaa_scale;
    GtkWidget *log_aniso_app_override_check_button;
    GtkWidget *log_aniso_scale;
    GtkWidget *texture_sharpening_button;
    
    guint active_attributes;

    gint fsaa_translation_table[NV_CTRL_FSAA_MODE_MAX + 1];
    gint fsaa_translation_table_size;
};

struct _CtkMultisampleClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_multisample_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_multisample_new       (NvCtrlAttributeHandle *,
                                       CtkConfig *, CtkEvent *);

GtkTextBuffer *ctk_multisample_create_help(GtkTextTagTable *,
                                           CtkMultisample *);
G_END_DECLS

#endif /* __CTK_MULTISAMPLE_H__ */

