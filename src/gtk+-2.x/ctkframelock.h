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

#ifndef __CTK_FRAMELOCK_H__
#define __CTK_FRAMELOCK_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"

#include "parse.h"

G_BEGIN_DECLS

#define CTK_TYPE_FRAMELOCK (ctk_framelock_get_type())

#define CTK_FRAMELOCK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
     CTK_TYPE_FRAMELOCK, CtkFramelock))

#define CTK_FRAMELOCK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
     CTK_TYPE_FRAMELOCK, CtkFramelockClass))

#define CTK_IS_FRAMELOCK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), CTK_TYPE_FRAMELOCK))

#define CTK_IS_FRAMELOCK_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), CTK_TYPE_FRAMELOCK))

#define CTK_FRAMELOCK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
     CTK_TYPE_FRAMELOCK, CtkFramelockClass))


typedef struct _CtkFramelock      CtkFramelock;
typedef struct _CtkFramelockClass CtkFramelockClass;

struct _CtkFramelock
{
    GtkVBox                parent;

    NvCtrlAttributeHandle *attribute_handle;
    
    GtkWindow             *parent_window;

    GtkWidget             *house_sync_frame;
    GtkWidget             *use_house_sync_button;
    GtkWidget             *sync_interval_entry;
    GtkWidget             *sync_interval_frame;

    GtkWidget             *house_format_combo;
    GtkWidget             *house_format_frame;
    GtkWidget             *house_format_detect;
    gint                   current_detect_format;
    guint                  house_format_detect_timer;

    GtkWidget             *add_x_screen_dialog;
    GtkWidget             *add_x_screen_entry;

    GtkWidget             *remove_x_screen_dialog;
    GtkWidget             *remove_x_screen_label;
    GtkTreeIter            remove_x_screen_iter;

    GtkWidget             *error_msg_dialog;
    GtkWidget             *error_msg_label;

    GtkListStore          *list_store;
    GtkTreeView           *treeview;

    GtkWidget             *add_x_screen_button;
    GtkWidget             *remove_x_screen_button;
    GtkWidget             *test_link_button;
    GtkWidget             *enable_syncing;

    GtkWidget             *sync_state_button;
    GtkWidget             *enable_syncing_label;
    GtkWidget             *disable_syncing_label;

    CtkConfig             *ctk_config;

    GdkCursor             *wait_cursor;

    gboolean               framelock_enabled;
};

struct _CtkFramelockClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_framelock_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_framelock_new       (NvCtrlAttributeHandle *, GtkWidget *,
                                     CtkConfig *, ParsedAttribute *);

GtkTextBuffer *ctk_framelock_create_help(GtkTextTagTable *);

void ctk_framelock_config_file_attributes(GtkWidget *, ParsedAttribute *);

void           ctk_framelock_select (GtkWidget *);
void           ctk_framelock_unselect (GtkWidget *);

G_END_DECLS

#endif /* __CTK_FRAMELOCK_H__ */

