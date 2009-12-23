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
#include "ctkgvo-banner.h"

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


struct _CtkGvo
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    GtkWidget *parent_window;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;
    
    /* State */

    gint caps; // Capabilities

    gint lock_owner;

    gint sync_mode;
    gint input_video_format;

    gint output_video_format;
    guint valid_output_video_format_mask[3];
    
    gint output_data_format;

    gint screen_width;
    gint screen_height;

    /* Widgets */

    GtkWidget *banner_box;
    GtkWidget *banner;


    GtkWidget *current_resolution_label;
    GtkWidget *current_state_label;

    GtkWidget *clone_mode_frame;

    GtkWidget *output_video_format_menu;
    GtkWidget *output_data_format_menu;

    GtkWidget *x_offset_spin_button;
    GtkWidget *y_offset_spin_button;

    GtkWidget *toggle_clone_mode_button;
    GtkWidget *enable_clone_mode_label;
    GtkWidget *disable_clone_mode_label;
};


struct _CtkGvoClass
{
    GtkVBoxClass parent_class;
};


typedef struct {
    int format;
    const char *name;
} GvioFormatName;

typedef struct {
    int format;
    int rate;
    int width;
    int height;
} GvioFormatDetails;



GType          ctk_gvo_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_gvo_new         (NvCtrlAttributeHandle *,
                                    CtkConfig *, CtkEvent *);
void           ctk_gvo_select      (GtkWidget *);
void           ctk_gvo_unselect    (GtkWidget *);
GtkTextBuffer* ctk_gvo_create_help (GtkTextTagTable *);
void           ctk_gvo_pack_banner_slot (CtkGvoBanner *, gint slot, gint new);
gint           ctk_gvo_probe       (gpointer data);



const char *ctk_gvio_get_video_format_name(const gint format);
const char *ctk_gvo_get_data_format_name(const gint format);
void ctk_gvo_get_video_format_resolution(const gint format, gint *w, gint *h);


G_END_DECLS

#endif /* __CTK_GVO_H__*/
