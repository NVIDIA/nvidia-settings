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

#ifndef __CTK_GVO_H__
#define __CTK_GVO_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS


#define CTK_TYPE_GVO (ctk_gvo_get_type())

#define CTK_GVO(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GVO, CtkGvo))

#define CTK_GVO_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GVO, CtkGvoClass))

#define CTK_IS_GVO(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GVO))

#define CTK_IS_GVO_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GVO))

#define CTK_GVO_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GVO, CtkGvoClass))



typedef struct _CtkGvo       CtkGvo;
typedef struct _CtkGvoClass  CtkGvoClass;

typedef struct _CtkGvoBanner CtkGvoBanner;


struct _CtkGvoBanner
{
    GtkWidget *table;
    
    GtkWidget *slots[4];

    struct {
        guint8 vid1;
        guint8 vid2;
        guint8 sdi;
        guint8 comp;
    } img;

    struct {
        guint8 vid1;
        guint8 vid2;
        guint8 sdi;
        guint8 comp;
    } state;
};


struct _CtkGvo
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    GtkWidget *parent_window;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;

    GtkWidget *banner_frame;

    GtkWidget *sync_mode_menu;
    GtkWidget *output_video_format_menu;
    GtkWidget *output_data_format_menu;

    GtkWidget *enable_sdi_output_label;
    GtkWidget *disable_sdi_output_label;
    GtkWidget *toggle_sdi_output_button;

    GtkWidget *sync_format_menu;
    GtkWidget *input_video_format_text_entry;
    GtkWidget *input_video_format_detect_button;

    GtkWidget *current_resolution_label;
    GtkWidget *current_state_label;
    
    gint sync_mode;
    gint sync_source;
    gint input_video_format;
    gint output_video_format;
    gint output_data_format;

    gint valid_putput_video_format_mask;
    gint input_video_format_detect_timer;

    GdkCursor *wait_cursor;
    CtkGvoBanner banner;

    GtkWidget *hsync_delay_spin_button;
    GtkWidget *vsync_delay_spin_button;

    GtkWidget *x_offset_spin_button;
    GtkWidget *y_offset_spin_button;

    gboolean sync_format_sensitive;
    gboolean sdi_output_enabled;

    gint screen_width;
    gint screen_height;
};


struct _CtkGvoClass
{
    GtkVBoxClass parent_class;
};


GType          ctk_gvo_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_gvo_new         (NvCtrlAttributeHandle *,
                                    GtkWidget *, CtkConfig *, CtkEvent *);
void           ctk_gvo_select      (GtkWidget *);
void           ctk_gvo_unselect    (GtkWidget *);
GtkTextBuffer* ctk_gvo_create_help (GtkTextTagTable *);


G_END_DECLS

#endif /* __CTK_GVO_H__*/
