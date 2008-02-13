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

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkimagesliders.h"

#include "ctkscale.h"
#include "ctkconfig.h"
#include "ctkhelp.h"


#define FRAME_PADDING 5

static void
dvc_adjustment_value_changed(GtkAdjustment *, gpointer);

static void dvc_update_slider(CtkImageSliders *ctk_image_sliders, gint value);

static void dvc_update_received(GtkObject *, gpointer arg1, gpointer);

static void
image_sharpening_adjustment_value_changed(GtkAdjustment *, gpointer);

static void
image_sharpening_update_slider(CtkImageSliders *ctk_image_sliders, gint value);

static void image_sharpening_update_received(GtkObject *, gpointer arg1,
                                             gpointer);



GType ctk_image_sliders_get_type(void)
{
    static GType ctk_image_sliders_type = 0;
    
    if (!ctk_image_sliders_type) {
        static const GTypeInfo ctk_image_sliders_info = {
            sizeof (CtkImageSlidersClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkImageSliders),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_image_sliders_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkImageSliders", &ctk_image_sliders_info, 0);
    }

    return ctk_image_sliders_type;
}


GtkWidget* ctk_image_sliders_new(NvCtrlAttributeHandle *handle,
                                 CtkConfig *ctk_config, CtkEvent *ctk_event,
                                 GtkWidget *reset_button,
                                 unsigned int display_device_mask,
                                 char *name)
{
    CtkImageSliders *ctk_image_sliders;
 
    GObject *object;
    
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *scale;
    GtkWidget *widget;

    ReturnStatus ret0, ret1, ret2, ret3;
    
    NVCTRLAttributeValidValuesRec dvc_valid, sharp_valid;
    
    int dvc, sharp;

    /*
     * retrieve the valid values and current value for DVC and Image
     * Sharpening; if we were unable to query any of those, then
     * return NULL.
     */
    
    ret0 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_DIGITAL_VIBRANCE,
                                                &dvc_valid);
    
    ret1 = NvCtrlGetDisplayAttribute(handle, display_device_mask,
                                     NV_CTRL_DIGITAL_VIBRANCE,
                                     &dvc);
    
    ret2 = NvCtrlGetValidDisplayAttributeValues(handle, display_device_mask,
                                                NV_CTRL_IMAGE_SHARPENING,
                                                &sharp_valid);
    
    ret3 = NvCtrlGetDisplayAttribute(handle, display_device_mask,
                                     NV_CTRL_IMAGE_SHARPENING,
                                     &sharp);
    
    if ((ret0 != NvCtrlSuccess) && (ret1 != NvCtrlSuccess) &&
        (ret2 != NvCtrlSuccess) && (ret3 != NvCtrlSuccess)) return NULL;

    /*
     * now that we know that we will have atleast one attribute,
     * create the object
     */

    object = g_object_new(CTK_TYPE_IMAGE_SLIDERS, NULL);

    ctk_image_sliders = CTK_IMAGE_SLIDERS(object);
    ctk_image_sliders->handle = handle;
    ctk_image_sliders->ctk_config = ctk_config;
    ctk_image_sliders->reset_button = reset_button;
    ctk_image_sliders->display_device_mask = display_device_mask;
    ctk_image_sliders->name = name;
    
    /* create the frame and vbox */
    
    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    

    /* DVC */
    
    if ((ret0 == NvCtrlSuccess) && (ret1 == NvCtrlSuccess)) {
        
        ctk_image_sliders->dvc_adjustment =
            gtk_adjustment_new(dvc,
                               dvc_valid.u.range.min,
                               dvc_valid.u.range.max,
                               1, 5, 0);

        g_signal_connect(G_OBJECT(ctk_image_sliders->dvc_adjustment),
                         "value_changed",
                         G_CALLBACK(dvc_adjustment_value_changed),
                         (gpointer) ctk_image_sliders);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_DIGITAL_VIBRANCE),
                         G_CALLBACK(dvc_update_received),
                         (gpointer) ctk_image_sliders);

        scale = ctk_scale_new
            (GTK_ADJUSTMENT(ctk_image_sliders->dvc_adjustment),
             "Digital Vibrance", ctk_config, G_TYPE_DOUBLE);
        
        gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
    
        widget = CTK_SCALE(scale)->gtk_scale;

        ctk_config_set_tooltip(ctk_config, widget,
                               "The Digital Vibrance slider alters the level "
                               "of Digital Vibrance for this display device.");
    } else {
        ctk_image_sliders->dvc_adjustment = NULL;
    }
    
    
    /* Image Sharpening */

    if ((ret2 == NvCtrlSuccess) && (ret3 == NvCtrlSuccess)) {
        
        ctk_image_sliders->image_sharpening_adjustment =
            gtk_adjustment_new(sharp,
                               sharp_valid.u.range.min,
                               sharp_valid.u.range.max,
                               1, 5, 0); 
        
        g_signal_connect
            (G_OBJECT(ctk_image_sliders->image_sharpening_adjustment),
             "value_changed",
             G_CALLBACK(image_sharpening_adjustment_value_changed),
             (gpointer) ctk_image_sliders);
        
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_IMAGE_SHARPENING),
                         G_CALLBACK(image_sharpening_update_received),
                         (gpointer) ctk_image_sliders);

        scale = ctk_scale_new
            (GTK_ADJUSTMENT(ctk_image_sliders->image_sharpening_adjustment),
             "Image Sharpening", ctk_config, G_TYPE_DOUBLE);
        
        gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);
        
        widget = CTK_SCALE(scale)->gtk_scale;
        
        ctk_config_set_tooltip(ctk_config, widget,
                               "The Image Sharpening slider alters the level "
                               "of Image Sharpening for this display device.");
    } else {
        ctk_image_sliders->image_sharpening_adjustment = NULL;
    }


    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
    
} /* ctk_image_sliders_new() */



/*
 * ctk_image_sliders_reset() - 
 */

void ctk_image_sliders_reset(CtkImageSliders *ctk_image_sliders)
{
    if (!ctk_image_sliders) return;

    if (ctk_image_sliders->dvc_adjustment) {
        
        NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                                  ctk_image_sliders->display_device_mask,
                                  NV_CTRL_DIGITAL_VIBRANCE,
                                  0);

        dvc_update_slider(ctk_image_sliders, 0);
    }

    if (ctk_image_sliders->image_sharpening_adjustment) {
        
        NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                                  ctk_image_sliders->display_device_mask,
                                  NV_CTRL_IMAGE_SHARPENING,
                                  0);
        
        image_sharpening_update_slider(ctk_image_sliders, 0);
    }
    
} /* ctk_image_sliders_reset() */



/*
 * post_dvc_update() - helper function for
 * dvc_adjustment_value_changed() and dvc_update_received(); this does
 * whatever work is necessary after the the DVC adjustment widget is
 * updated -- currently, this is just posting a statusbar message.
 */

static void post_dvc_update(CtkImageSliders *ctk_image_sliders, int value)
{
    ctk_config_statusbar_message(ctk_image_sliders->ctk_config,
                                 "Digital Vibrance for %s set to %d.",
                                 ctk_image_sliders->name, value);
    
} /* post_dvc_update() */



/*
 * dvc_adjustment_value_changed() - update the DVC value with the
 * current value of the adjustment widget.
 */

static void dvc_adjustment_value_changed(GtkAdjustment *adjustment,
                                         gpointer user_data)
{
    CtkImageSliders *ctk_image_sliders;
    int value;
    
    ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);
    
    value = (int) gtk_adjustment_get_value(adjustment);
    
    NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                              ctk_image_sliders->display_device_mask,
                              NV_CTRL_DIGITAL_VIBRANCE,
                              value);
    
    post_dvc_update(ctk_image_sliders, value);

} /* dvc_adjustment_value_changed() */



/*
 * dvc_update_slider() - update the slider with the specified value
 */

static void dvc_update_slider(CtkImageSliders *ctk_image_sliders, gint value)
{
    GtkAdjustment *adjustment =
        GTK_ADJUSTMENT(ctk_image_sliders->dvc_adjustment);
    
    g_signal_handlers_block_by_func(G_OBJECT(adjustment),
                                    G_CALLBACK(dvc_adjustment_value_changed),
                                    (gpointer) ctk_image_sliders);
    
    gtk_adjustment_set_value(adjustment, value);
    
    g_signal_handlers_unblock_by_func(G_OBJECT(adjustment),
                                      G_CALLBACK(dvc_adjustment_value_changed),
                                      (gpointer) ctk_image_sliders);
    
} /* dvc_update_slider() */



/*
 * dvc_update_received() - callback function for when the
 * NV_CTRL_DIGITAL_VIBRANCE attribute is changed by another NV-CONTROL
 * client.
 */

static void dvc_update_received(GtkObject *object,
                                gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkImageSliders *ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);
    
    /* if the event is not for this display device, return */
    
    if (!(event_struct->display_mask &
          ctk_image_sliders->display_device_mask)) {
        return;
    }
    
    dvc_update_slider(ctk_image_sliders, event_struct->value);

    post_dvc_update(ctk_image_sliders, event_struct->value);
    
} /* dvc_update_received() */



/*
 * post_image_sharpening_update() - helper function for
 * image_sharpening_adjustment_value_changed() and
 * image_sharpening_update_received(); this does whatever work is
 * necessary after the the image sharpening adjustment widget is
 * updated -- currently, this is just posting a statusbar message.
 */

static void
post_image_sharpening_update(CtkImageSliders *ctk_image_sliders, gint value)
{
    ctk_config_statusbar_message(ctk_image_sliders->ctk_config,
                                 "Image Sharpening for %s set to %d.",
                                 ctk_image_sliders->name, value);
    
} /* post_image_sharpening_update() */



/*
 * image_sharpening_adjustment_value_changed() - 
 */

static void
image_sharpening_adjustment_value_changed(GtkAdjustment *adjustment,
                                          gpointer user_data)
{
    CtkImageSliders *ctk_image_sliders;
    int value;
    
    ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);
    
    value = (int) gtk_adjustment_get_value(adjustment);
    
    NvCtrlSetDisplayAttribute(ctk_image_sliders->handle,
                              ctk_image_sliders->display_device_mask,
                              NV_CTRL_IMAGE_SHARPENING,
                              value);
    
    post_image_sharpening_update(ctk_image_sliders, value);
    
} /* image_sharpening_adjustment_value_changed() */



/*
 * image_sharpening_update_slider() - update the slider with the
 * specified value
 */

static void image_sharpening_update_slider(CtkImageSliders *ctk_image_sliders,
                                           gint value)
{
    GtkAdjustment *adjustment =
        GTK_ADJUSTMENT(ctk_image_sliders->image_sharpening_adjustment);

    g_signal_handlers_block_by_func
        (G_OBJECT(adjustment),
         G_CALLBACK(image_sharpening_adjustment_value_changed),
         (gpointer) ctk_image_sliders);
    
    gtk_adjustment_set_value(adjustment, value);
    
    g_signal_handlers_unblock_by_func
        (G_OBJECT(adjustment),
         G_CALLBACK(image_sharpening_adjustment_value_changed),
         (gpointer) ctk_image_sliders);
    
} /* image_sharpening_update_slider() */



/*
 * image_sharpening_update_received() - callback function for when the
 * NV_CTRL_IMAGE_SHARPENING attribute is change by another NV-CONTROL
 * client.
 */

static void image_sharpening_update_received(GtkObject *object,
                                             gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkImageSliders *ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);
    
    /* if the event is not for this display device, return */

    if (!(event_struct->display_mask &
          ctk_image_sliders->display_device_mask)) {
        return;
    }
    
    image_sharpening_update_slider(ctk_image_sliders, event_struct->value);

    post_image_sharpening_update(ctk_image_sliders, event_struct->value);
    
} /* image_sharpening_update_received() */



/*
 * add_image_sliders_help() - 
 */

gboolean add_image_sliders_help(CtkImageSliders *ctk_image_sliders,
                                GtkTextBuffer *b,
                                GtkTextIter *i)
{
    gboolean ret = FALSE;    

    if (ctk_image_sliders->dvc_adjustment) {
        ctk_help_heading(b, i, "Digital Vibrance");
        ctk_help_para(b, i, "Digital Vibrance, a mechanism for "
                      "controlling color separation and intensity, boosts "
                      "the color saturation of an image so that all images "
                      "including 2D, 3D, and video appear brighter and "
                      "crisper (even on flat panels) in your applications.");
        ret = TRUE;
    }

    if (ctk_image_sliders->image_sharpening_adjustment) {
        ctk_help_heading(b, i, "Image Sharpening");
        ctk_help_para(b, i, "Use the Image Sharpening slider to adjust the "
                      "sharpness of the image quality by amplifying high "
                      "frequency content.");
        ret = TRUE;
    }

    return ret;
    
} /* add_image_sliders_help() */
