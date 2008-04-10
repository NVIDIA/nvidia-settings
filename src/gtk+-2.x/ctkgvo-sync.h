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

#ifndef __CTK_GVO_SYNC_H__
#define __CTK_GVO_SYNC_H__

#include "ctkevent.h"
#include "ctkconfig.h"
#include "ctkgvo.h"


G_BEGIN_DECLS

#define CTK_TYPE_GVO_SYNC (ctk_gvo_sync_get_type())

#define CTK_GVO_SYNC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GVO_SYNC, \
                                 CtkGvoSync))

#define CTK_GVO_SYNC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GVO_SYNC, \
                              CtkGvoSyncClass))

#define CTK_IS_GVO_SYNC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GVO_SYNC))

#define CTK_IS_GVO_SYNC_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GVO_SYNC))

#define CTK_GVO_SYNC_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GVO_SYNC, \
                                CtkGvoSyncClass))


typedef struct _CtkGvoSync       CtkGvoSync;
typedef struct _CtkGvoSyncClass  CtkGvoSyncClass;


struct _CtkGvoSync
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    GtkWidget *parent_window;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;

    /* State */

    gint caps;               // SDI device capabilities
    gint sync_mode;          // NV_CTRL_GVO_SYNC_MODE

    gint input_video_format; // NV_CTRL_GVO_INPUT_VIDEO_FORMAT
    gint sync_source;        // NV_CTRL_ SDI and COMP sync formats...

    gint sdi_sync_input_detected;
    gint comp_sync_input_detected;
    gint comp_mode;
    gint sync_lock_status; // Genlocked/Framelock status

    /* Widgets */

    GtkWidget *frame;

    CtkGvo *gvo_parent;
    GtkWidget *banner_box;

    GtkWidget *input_video_format_text_entry;
    GtkWidget *input_video_format_detect_button;
    GtkWidget *composite_termination_button;

    GtkWidget *sync_mode_menu;
    GtkWidget *sync_format_menu; // SDI/Composite sync format
    GtkWidget *sync_lock_status_text;

    GtkWidget *hsync_delay_spin_button;
    GtkWidget *vsync_delay_spin_button;

    GdkCursor *wait_cursor;
};

struct _CtkGvoSyncClass
{
    GtkVBoxClass parent_class;
};


GType       ctk_gvo_sync_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_gvo_sync_new       (NvCtrlAttributeHandle *, GtkWidget *,
                                    CtkConfig *, CtkEvent *, CtkGvo *);

GtkTextBuffer* ctk_gvo_sync_create_help(GtkTextTagTable *, CtkGvoSync *);

void ctk_gvo_sync_select(GtkWidget *);
void ctk_gvo_sync_unselect(GtkWidget *);

//GtkTextBuffer* ctk_gvo_sync_create_help (GtkTextTagTable *table);


G_END_DECLS

#endif /* __CTK_GVO_SYNC_H__ */
