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

#ifndef __CTK_GVO_BANNER_H__
#define __CTK_GVO_BANNER_H__

#include "NvCtrlAttributes.h"
#include "ctkconfig.h"
#include "ctkevent.h"

G_BEGIN_DECLS


#define CTK_TYPE_GVO_BANNER (ctk_gvo_banner_get_type())

#define CTK_GVO_BANNER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_GVO_BANNER, CtkGvoBanner))

#define CTK_GVO_BANNER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_GVO_BANNER, CtkGvoBannerClass))

#define CTK_IS_GVO_BANNER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_GVO_BANNER))

#define CTK_IS_GVO_BANNER_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_GVO_BANNER))

#define CTK_GVO_BANNER_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_GVO_BANNER, CtkGvoBannerClass))



typedef gint (* ctk_gvo_banner_probe_callback) (gpointer data);

typedef struct _CtkGvoBanner       CtkGvoBanner;
typedef struct _CtkGvoBannerClass  CtkGvoBannerClass;

#define GVO_BANNER_VID1  0
#define GVO_BANNER_VID2  1
#define GVO_BANNER_SDI   2
#define GVO_BANNER_COMP  3

struct _CtkGvoBanner
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    GtkWidget *parent_box;
    CtkConfig *ctk_config;
    CtkEvent *ctk_event;

    gint sync_mode;
    gint sync_source;
    gboolean shared_sync_bnc; // GVO device has single sync BNC

    GtkWidget *image;      // Image
    GtkWidget *ctk_banner; // CtkBanner widget using the image

    gboolean flash; // Used to flash the LEDs at the same time
    guint8 img[4];  // Current color of LEDs
    guint state[4]; // Current state of LEDs

    ctk_gvo_banner_probe_callback  probe_callback; // Function to call
    gpointer                       probe_callback_data;
    
    // Other GVO state probed
    gint gvo_lock_owner;
    gint output_video_format;
    gint output_data_format;
    gint input_video_format;
    gint composite_sync_input_detected;
    gint sdi_sync_input_detected;
    gint sync_lock_status;
};

struct _CtkGvoBannerClass
{
    GtkVBoxClass parent_class;
};


GType          ctk_gvo_banner_get_type (void) G_GNUC_CONST;
GtkWidget*     ctk_gvo_banner_new      (NvCtrlAttributeHandle *, CtkConfig *,
                                        CtkEvent *);

gint ctk_gvo_banner_probe(gpointer data);

void ctk_gvo_banner_set_parent(CtkGvoBanner *ctk_gvo_banner,
                               GtkWidget *new_parent_box,
                               ctk_gvo_banner_probe_callback probe_callback,
                               gpointer probe_callback_data);

G_END_DECLS

#endif /* __CTK_GVO_BANNER_H__*/
