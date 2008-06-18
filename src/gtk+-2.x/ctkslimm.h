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

#ifndef __CTK_SLIMM_H__
#define __CTK_SLIMM_H__

#include <gtk/gtk.h>

#include "NvCtrlAttributes.h"
#include "ctkevent.h"
#include "ctkdisplaylayout.h"
#include "ctkconfig.h"

#include "XF86Config-parser/xf86Parser.h"

G_BEGIN_DECLS

#define CTK_TYPE_SLIMM (ctk_slimm_get_type())

#define CTK_SLIMM(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_SLIMM, CtkSLIMM))

#define CTK_SLIMM_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_SLIMM, CtkSLIMMClass))

#define CTK_IS_SLIMM(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_SLIMM))

#define CTK_IS_SLIMM_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_SLIMM))

#define CTK_SLIMM_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_SLIMM, CtkSLIMMClass))

typedef struct _CtkSLIMM       CtkSLIMM;
typedef struct _CtkSLIMMClass  CtkSLIMMClass;

struct _CtkSLIMM
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    GtkWidget *mnu_display_config;
    GtkWidget *mnu_display_resolution;
    GtkWidget *mnu_display_refresh;
    GtkWidget *spbtn_hedge_overlap;
    GtkWidget *spbtn_vedge_overlap;
    GtkWidget *lbl_total_size;
    GtkWidget *box_total_size;
    GtkWidget *btn_save_config;
    GtkWidget *cbtn_slimm_enable;
    nvModeLinePtr *resolution_table;
    nvModeLinePtr *refresh_table;
    int resolution_table_len;
    int refresh_table_len;
    gboolean mnu_refresh_disabled;
    nvModeLinePtr modelines;
    nvModeLinePtr cur_modeline;
    gint num_modelines;
};

struct _CtkSLIMMClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_slimm_get_type (void) G_GNUC_CONST;
GtkWidget*  ctk_slimm_new      (NvCtrlAttributeHandle *handle,
                                CtkEvent *ctk_event, CtkConfig *ctk_config);

GtkTextBuffer *ctk_slimm_create_help(GtkTextTagTable *, const gchar *);

G_END_DECLS

#endif /* __CTK_SLIMM_H__ */

