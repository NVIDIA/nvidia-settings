/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
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

#ifndef __CTK_GVI_H__
#define __CTK_GVI_H__

#include "ctkevent.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_GVI (ctk_gvi_get_type())

#define CTK_GVI(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GVI, CtkGvi))

#define CTK_GVI_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GVI, CtkGviClass))

#define CTK_IS_GVI(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GVI))

#define CTK_IS_GVI_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GVI))

#define CTK_GVI_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GVI, CtkGviClass))


typedef struct _CtkGvi       CtkGvi;
typedef struct _CtkGviClass  CtkGviClass;

struct _CtkGvi
{
    GtkVBox parent;

    CtrlTarget *ctrl_target;
    CtkConfig *ctk_config;

    int num_jacks;
    int max_channels_per_jack;

    GtkWidget *gpu_name;

    GtkWidget *jack_channel_omenu;

    GtkWidget *input_info_vbox;

    GtkWidget *show_detailed_info_btn;
    unsigned int cur_jack_channel;
    unsigned int *jack_channel_table;
};

struct _CtkGviClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_gvi_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_gvi_new         (CtrlTarget *, CtkConfig *, CtkEvent *);
GtkTextBuffer* ctk_gvi_create_help (GtkTextTagTable *, CtkGvi *);

void           ctk_gvi_start_timer (GtkWidget *);
void           ctk_gvi_stop_timer  (GtkWidget *);

G_END_DECLS

#endif /* __CTK_GVI_H__ */
