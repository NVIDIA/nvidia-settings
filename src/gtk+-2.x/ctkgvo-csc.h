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

#ifndef __CTK_GVO_CSC_H__
#define __CTK_GVO_CSC_H__

#include "ctkevent.h"
#include "ctkconfig.h"
#include "ctkgvo.h"


G_BEGIN_DECLS

#define CTK_TYPE_GVO_CSC (ctk_gvo_csc_get_type())

#define CTK_GVO_CSC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GVO_CSC, \
                                 CtkGvoCsc))

#define CTK_GVO_CSC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GVO_CSC, \
                              CtkGvoCscClass))

#define CTK_IS_GVO_CSC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GVO_CSC))

#define CTK_IS_GVO_CSC_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GVO_CSC))

#define CTK_GVO_CSC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GVO_CSC, \
                                CtkGvoCscClass))


typedef struct _CtkGvoCsc       CtkGvoCsc;
typedef struct _CtkGvoCscClass  CtkGvoCscClass;

struct _CtkGvoCsc
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    CtkGvo *gvo_parent;
    GtkWidget *banner_box;

    int caps;

    float matrix[3][3]; // [row][column]
    float offset[3];
    float scale[3];

    gboolean applyImmediately;

    GtkWidget *matrixWidget[3][3];
    GtkWidget *offsetWidget[3];
    GtkWidget *scaleWidget[3];
    
    GtkWidget *matrixTable;
    GtkWidget *offsetTable;
    GtkWidget *scaleTable;
    
    GtkWidget *overrideButton;
    GtkWidget *initializeDropDown;
    GtkWidget *applyImmediateButton;
    GtkWidget *applyButton;

    GtkWidget *cscOptions;
};

struct _CtkGvoCscClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_gvo_csc_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_gvo_csc_new       (NvCtrlAttributeHandle *,
                                   CtkConfig *, CtkEvent *, CtkGvo *);

GtkTextBuffer *ctk_gvo_csc_create_help(GtkTextTagTable *, CtkGvoCsc *);

void ctk_gvo_csc_select(GtkWidget *);
void ctk_gvo_csc_unselect(GtkWidget *);

G_END_DECLS

#endif /* __CTK_GVO_CSC_H__ */
