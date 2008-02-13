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

#ifndef __CTK_EDID_H__
#define __CTK_EDID_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_EDID (ctk_edid_get_type())

#define CTK_EDID(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EDID, \
                                 CtkEdid))

#define CTK_EDID_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_EDID, \
                              CtkEdidClass))

#define CTK_IS_EDID(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EDID))

#define CTK_IS_EDID_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_EDID))

#define CTK_EDID_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_EDID, \
                                CtkEdidClass))

typedef struct _CtkEdid       CtkEdid;
typedef struct _CtkEdidClass  CtkEdidClass;

struct _CtkEdid
{
    GtkVBox parent;
    
    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;
    GtkWidget *reset_button;
    GtkWidget *button;
    GtkWidget *file_selector;
    
    const gchar *filename;
    char *name;

    unsigned int display_device_mask;
};

struct _CtkEdidClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_edid_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_edid_new       (NvCtrlAttributeHandle *,
                                CtkConfig *, CtkEvent *,
                                GtkWidget *reset_button,
                                unsigned int display_device_mask,
                                char *name);

void ctk_edid_reset(CtkEdid *);

void add_acquire_edid_help(GtkTextBuffer *b, GtkTextIter *i);


G_END_DECLS

#endif /* __CTK_EDID_H__ */
