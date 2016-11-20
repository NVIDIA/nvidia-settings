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

    CtrlTarget            *ctrl_target;
    CtkConfig             *ctk_config;

    GtkWindow             *parent_window;

    GdkCursor             *wait_cursor;

    /* Device tree & buttons */
    gpointer               tree;
    GtkWidget             *add_devices_button;
    GtkWidget             *remove_devices_button;
    GtkWidget             *short_labels_button;
    GtkWidget             *extra_info_button;
    GtkWidget             *expand_all_button;
    gboolean               is_expanded;

    /* House sync */
    GtkWidget             *house_sync_frame;
    GtkWidget             *house_sync_vbox;
    GtkWidget             *house_sync_mode_frame;
    GtkWidget             *house_sync_mode_combo;
    GtkWidget             *sync_interval_frame;
    GtkWidget             *sync_interval_scale;
    GtkWidget             *sync_edge_frame;
    GtkWidget             *sync_edge_combo;
    GtkWidget             *video_mode_frame;
    GtkWidget             *video_mode_widget;
    GtkWidget             *video_mode_detect;

    gint                   current_detect_format;
    guint                  video_mode_detect_timer;
    gboolean               video_mode_read_only;
    gboolean               house_sync_output_supported;
    gboolean               house_sync_output_warning_dlg_shown;

    /* Dialogs */
    GtkWidget             *warn_dialog;
    GtkWidget             *add_devices_dialog;
    GtkWidget             *add_devices_entry;

    GtkWidget             *remove_devices_dialog;
    GtkWidget             *remove_devices_label;
    GtkTreeIter            remove_devices_iter;

    GtkWidget             *error_msg_dialog;
    GtkWidget             *error_msg_label;

    GtkWidget             *enable_confirm_dialog;
    GtkWidget             *enable_confirm_text;
    GtkWidget             *enable_confirm_cancel_button;
    guint                  enable_confirm_timer;
    int                    enable_confirm_countdown;

    /* Buttons */
    GtkWidget             *test_link_button;
    gboolean               test_link_enabled;

    GtkWidget             *sync_state_button;
    GtkWidget             *enable_syncing_label;
    GtkWidget             *disable_syncing_label;
    GtkWidget             *selected_syncing_label;
    gboolean               framelock_enabled;

    /* Images */
    GdkPixbuf             *led_grey_pixbuf;
    GdkPixbuf             *led_green_pixbuf;
    GdkPixbuf             *led_red_pixbuf;

    GdkPixbuf             *rj45_input_pixbuf;
    GdkPixbuf             *rj45_output_pixbuf;
    GdkPixbuf             *rj45_unused_pixbuf;
};

struct _CtkFramelockClass
{
    GtkVBoxClass parent_class;
};

GType       ctk_framelock_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_framelock_new       (CtrlTarget *, GtkWidget *, CtkConfig *,
                                     ParsedAttribute *);

GtkTextBuffer *ctk_framelock_create_help(GtkTextTagTable *);

void ctk_framelock_config_file_attributes(GtkWidget *, ParsedAttribute *);

void           ctk_framelock_select (GtkWidget *);
void           ctk_framelock_unselect (GtkWidget *);

G_END_DECLS

#endif /* __CTK_FRAMELOCK_H__ */

