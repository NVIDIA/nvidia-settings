/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#ifndef __CTK_3D_VISION_PRO_H__
#define __CTK_3D_VISION_PRO_H__

#include "parse.h"
#include "ctkevent.h"
#include "ctkconfig.h"

#define NUM_GLASSES_INFO_ATTRIBS 2
#define GLASSES_NAME_MAX_LENGTH  128

G_BEGIN_DECLS

#define CTK_TYPE_3D_VISION_PRO (ctk_3d_vision_pro_get_type())

#define CTK_3D_VISION_PRO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), CTK_TYPE_3D_VISION_PRO, \
                                Ctk3DVisionPro))

#define CTK_3D_VISION_PRO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), CTK_TYPE_3D_VISION_PRO, \
                             Ctk3DVisionProClass))

#define CTK_IS_3D_VISION_PRO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_3D_VISION_PRO))

#define CTK_IS_3D_VISION_PRO_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_3D_VISION_PRO))

#define CTK_3D_VISION_PRO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), CTK_TYPE_3D_VISION_PRO, \
                               Ctk3DVisionProClass))

typedef struct _Ctk3DVisionPro       Ctk3DVisionPro;

typedef struct _Ctk3DVisionProClass  Ctk3DVisionProClass;

typedef enum {
    SVP_SHORT_RANGE = 1,
    SVP_MEDIUM_RANGE,
    SVP_LONG_RANGE
} SVP_RANGE;

typedef struct GlassesInfoRec {
    unsigned int            glasses_id;
    char                    name[GLASSES_NAME_MAX_LENGTH];
    int                     battery;
    GtkWidget              *label[NUM_GLASSES_INFO_ATTRIBS];
    GtkWidget              *hbox[NUM_GLASSES_INFO_ATTRIBS];
    GtkWidget              *image;
} GlassesInfo;

typedef struct HtuInfoRec {
    SVP_RANGE               channel_range;
    int                     channel_num;
    int                     signal_strength;
    guint                   num_glasses;
    GlassesInfo**           glasses_info;
    struct HtuInfoRec      *next;
} HtuInfo;

typedef struct WidgetSizeRec {
    GtkWidget *widget;
    int        width;
} WidgetSize;

typedef struct GlassesInfoTableRec {
    WidgetSize          glasses_header_sizes[NUM_GLASSES_INFO_ATTRIBS];
    GtkWidget          *data_table;
    GtkWidget          *data_viewport, *full_viewport;
    GtkWidget          *vscrollbar, *hscrollbar;
    int                 rows;
    int                 columns;
} GlassesInfoTable;


typedef struct _AddGlassesDlg {
    GtkWidget              *parent;
    GtkWidget              *dlg_add_glasses;
    GlassesInfo**           glasses_info;
    GlassesInfoTable        table;
    int                     new_glasses;
    Bool                    in_pairing;
    int                     pairing_attempts;
} AddGlassesDlg;

struct _Ctk3DVisionPro
{
    GtkVBox                 parent;
    CtrlTarget             *ctrl_target;
    GtkWindow              *parent_wnd;
    CtkConfig              *ctk_config;
    CtkEvent               *ctk_event;
    GtkWidget              *menu;
    guint                   num_htu;
    HtuInfo**               htu_info;

    GlassesInfoTable        table;

    GtkLabel               *glasses_num_label;
    GtkWidget              *identify_button;
    GtkWidget              *refresh_button;
    GtkWidget              *rename_button;
    GtkWidget              *remove_button;
    GtkLabel               *channel_num_label;
    GtkLabel               *signal_strength_label;
    GtkWidget              *signal_strength_image;

    AddGlassesDlg          *add_glasses_dlg;
};

struct _Ctk3DVisionProClass
{
    GtkVBoxClass parent_class;
    void (*changed) (Ctk3DVisionPro *);
};

GType          ctk_3d_vision_pro_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_3d_vision_pro_new         (CtrlTarget *, CtkConfig *,
                                              ParsedAttribute *, CtkEvent *);
GtkTextBuffer *ctk_3d_vision_pro_create_help (GtkTextTagTable *);
void           ctk_3d_vision_pro_select      (GtkWidget *w);
void           ctk_3d_vision_pro_unselect    (GtkWidget *w);

void ctk_3d_vision_pro_config_file_attributes(GtkWidget *w,
                                              ParsedAttribute *head);

G_END_DECLS

#endif /* __CTK_3D_VISION_PRO_H__ */

