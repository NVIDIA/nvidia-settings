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
#include <string.h>

#include "NvCtrlAttributes.h"

#include "ctkmultisample.h"

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkimage.h"


/* local prototypes */

static void build_fsaa_translation_table(CtkMultisample *ctk_multisample,
                                         NVCTRLAttributeValidValuesRec valid);

static int map_nv_ctrl_fsaa_value_to_slider(CtkMultisample *ctk_multisample,
                                            int value);


static gchar *format_fsaa_value(GtkScale *scale, gdouble arg1,
                                gpointer user_data);

static const gchar *get_multisample_mode_name(gint multisample_mode);

static void post_fsaa_app_override_toggled(CtkMultisample *ctk_multisample,
                                           gboolean override);

static void fsaa_app_override_toggled(GtkWidget *widget, gpointer user_data);

static void fsaa_app_override_update_received(GtkObject *object,
                                              gpointer arg1,
                                              gpointer user_data);

static void post_fsaa_value_changed(CtkMultisample *ctk_multisample, gint val);

static void fsaa_value_changed(GtkRange *range, gpointer user_data);

static void fsaa_update_received(GtkObject *object,
                                 gpointer arg1, gpointer user_data);

static void
post_log_aniso_app_override_toggled(CtkMultisample *ctk_multisample,
                                    gboolean override);

static void log_aniso_app_override_toggled(GtkWidget *widget,
                                           gpointer user_data);

static void log_app_override_update_received(GtkObject *object,
                                             gpointer arg1,
                                             gpointer user_data);

static const gchar *get_log_aniso_name(gint val);

static gchar *format_log_aniso_value(GtkScale *scale, gdouble arg1,
                                     gpointer user_data);

static void post_log_aniso_value_changed(CtkMultisample *ctk_multisample,
                                         gint val);

static void log_aniso_value_changed(GtkRange *range, gpointer user_data);

static void log_aniso_range_update_received(GtkObject *object,
                                            gpointer arg1,
                                            gpointer user_data);

static void post_texture_sharpening_toggled(CtkMultisample *ctk_multisample,
                                            gboolean enabled);

static void texture_sharpening_toggled(GtkWidget *widget, gpointer user_data);

static void texture_sharpening_update_received(GtkObject *object,
                                               gpointer arg1,
                                               gpointer user_data);

static const char *__aa_override_app_help =
"Enable the Antialiasing \"Override Application Setting\" "
"checkbox to make the antialiasing slider active and "
"override any appliation antialiasing setting with the "
"value of the slider.";

static const char *__aa_slider_help =
"The Antialiasing slider controls the level of antialiasing.";

static const char *__aniso_override_app_help =
"Enable the Anisotropic Filtering \"Override Application Setting\" "
"checkbox to make the anisotropic filtering slider "
"active and override any appliation anisotropic "
"filtering setting with the value of the slider.";

static const char *__aniso_slider_help =
"The Anisotropic Filtering slider controls the "
"level of automatic anisotropic texture filtering.";

static const char *__texture_sharpening_help =
"To improve image quality, select this option "
"to sharpen textures when running OpenGL applications "
"with antialiasing enabled.";


/*
 * bits indicating which attributes require documenting in the online
 * help
 */

#define __LOG_ANISO_RANGE (1 << 2)
#define __TEXTURE_SHARPEN (1 << 3)
#define __FSAA            (1 << 4)
#define __FSAA_NONE       (1 << (__FSAA + NV_CTRL_FSAA_MODE_NONE))
#define __FSAA_2x         (1 << (__FSAA + NV_CTRL_FSAA_MODE_2x))
#define __FSAA_2x_5t      (1 << (__FSAA + NV_CTRL_FSAA_MODE_2x_5t))
#define __FSAA_15x15      (1 << (__FSAA + NV_CTRL_FSAA_MODE_15x15))
#define __FSAA_2x2        (1 << (__FSAA + NV_CTRL_FSAA_MODE_2x2))
#define __FSAA_4x         (1 << (__FSAA + NV_CTRL_FSAA_MODE_4x))
#define __FSAA_4x_9t      (1 << (__FSAA + NV_CTRL_FSAA_MODE_4x_9t))
#define __FSAA_8x         (1 << (__FSAA + NV_CTRL_FSAA_MODE_8x))
#define __FSAA_16x        (1 << (__FSAA + NV_CTRL_FSAA_MODE_16x))
#define __FSAA_8xS        (1 << (__FSAA + NV_CTRL_FSAA_MODE_8xS))

#define FRAME_PADDING 5

GType ctk_multisample_get_type(
    void
)
{
    static GType ctk_multisample_type = 0;

    if (!ctk_multisample_type) {
        static const GTypeInfo ctk_multisample_info = {
            sizeof (CtkMultisampleClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkMultisample),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_multisample_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkMultisample",
                                   &ctk_multisample_info, 0);
    }

    return ctk_multisample_type;
}



/*
 * ctk_multisample_new() - constructor for the Multisample widget
 */

GtkWidget *ctk_multisample_new(NvCtrlAttributeHandle *handle,
                               CtkConfig *ctk_config, CtkEvent *ctk_event)
{
    GObject *object;
    CtkMultisample *ctk_multisample;
    GtkWidget *hbox, *vbox = NULL;
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *check_button;
    GtkWidget *scale;
    
    gint val, app_control, override, i;
    
    NVCTRLAttributeValidValuesRec valid;

    ReturnStatus ret, ret0;

    /* create the new object */
    
    object = g_object_new(CTK_TYPE_MULTISAMPLE, NULL);

    ctk_multisample = CTK_MULTISAMPLE(object);
    ctk_multisample->handle = handle;
    ctk_multisample->ctk_config = ctk_config;
    ctk_multisample->active_attributes = 0;

    gtk_box_set_spacing(GTK_BOX(object), 10);

    /* banner */
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    banner = ctk_banner_image_new(BANNER_ARTWORK_ANTIALIAS);
    gtk_box_pack_start(GTK_BOX(hbox), banner, TRUE, TRUE, 0);

    /* FSAA slider */

    ret = NvCtrlGetValidAttributeValues(handle, NV_CTRL_FSAA_MODE, &valid);
    
    if (ret == NvCtrlSuccess) {
        
        build_fsaa_translation_table(ctk_multisample, valid);
        
        ret = NvCtrlGetAttribute(handle, NV_CTRL_FSAA_MODE, &val);
        
        val = map_nv_ctrl_fsaa_value_to_slider(ctk_multisample, val);

        ret0 = NvCtrlGetAttribute(handle,
                                  NV_CTRL_FSAA_APPLICATION_CONTROLLED,
                                  &app_control);
        /*
         * The NV-CONTROL extension works in terms of whether the
         * application controls FSAA, but we invert the logic so that
         * we expose a checkbox to allow nvidia-settings to override
         * the application's setting.
         */

        override = !app_control;

        if ((ret == NvCtrlSuccess) && (ret0 == NvCtrlSuccess) &&
            (ctk_multisample->fsaa_translation_table_size > 1)) {
            
            /* create "Antialiasing Settings" frame */
            
            frame = gtk_frame_new("Antialiasing Settings");
            gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
            
            /* create the vbox to store the widgets inside the frame */
            
            vbox = gtk_vbox_new(FALSE, 5);
            gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
            gtk_container_add(GTK_CONTAINER(frame), vbox);
            
            /* "Override Application Setting" checkbox */
            
            check_button = gtk_check_button_new_with_label
                ("Override Application Setting");
            
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                         override);
            
            g_signal_connect(G_OBJECT(check_button), "toggled",
                             G_CALLBACK(fsaa_app_override_toggled),
                             (gpointer) ctk_multisample);
            
            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME
                             (NV_CTRL_FSAA_APPLICATION_CONTROLLED),
                             G_CALLBACK(fsaa_app_override_update_received),
                             (gpointer) ctk_multisample);
            
            ctk_config_set_tooltip(ctk_config, check_button,
                                   __aa_override_app_help);

            gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);
            
            ctk_multisample->fsaa_app_override_check_button = check_button;

            /* Antialiasing scale */

            scale = gtk_hscale_new_with_range
                (0, ctk_multisample->fsaa_translation_table_size - 1, 1);
            gtk_range_set_value(GTK_RANGE(scale), val);
            
            gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
            gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
            
            gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
            
            g_signal_connect(G_OBJECT(scale), "format-value",
                             G_CALLBACK(format_fsaa_value),
                             (gpointer) ctk_multisample);

            g_signal_connect(G_OBJECT(scale), "value-changed",
                             G_CALLBACK(fsaa_value_changed),
                             (gpointer) ctk_multisample);

            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME(NV_CTRL_FSAA_MODE),
                             G_CALLBACK(fsaa_update_received),
                             (gpointer) ctk_multisample);

            ctk_config_set_tooltip(ctk_config, scale, __aa_slider_help);

            ctk_multisample->active_attributes |= __FSAA;
            ctk_multisample->fsaa_scale = scale;

            gtk_widget_set_sensitive(GTK_WIDGET(ctk_multisample->fsaa_scale),
                                     override);

            for (i = 0; i < ctk_multisample->fsaa_translation_table_size; i++)
                ctk_multisample->active_attributes |=
                    (1 << (__FSAA+ctk_multisample->fsaa_translation_table[i]));
        }
    }
    
    /* Anisotropic filtering slider */

    ret = NvCtrlGetValidAttributeValues(handle, NV_CTRL_LOG_ANISO, &valid);
    
    ctk_multisample->log_aniso_scale = NULL;

    if (ret == NvCtrlSuccess) {
        
        ret = NvCtrlGetAttribute(handle, NV_CTRL_LOG_ANISO, &val);

        ret0 = NvCtrlGetAttribute(handle,
                                  NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED,
                                  &app_control);
        /*
         * The NV-CONTROL extension works in terms of whether the
         * application controls LOG_ANISO, but we invert the logic so
         * that we expose a checkbox to allow nvidia-settings to
         * override the application's setting.
         */
        
        override = !app_control;
        
        if ((ret == NvCtrlSuccess) && (ret0 == NvCtrlSuccess) &&
            (valid.type == ATTRIBUTE_TYPE_RANGE)) {
            
            /* create "Anisotropic Filtering" frame */
            
            frame = gtk_frame_new("Anisotropic Filtering");
            gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
            
            /* create the vbox to store the widgets inside the frame */

            vbox = gtk_vbox_new(FALSE, 5);
            gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
            gtk_container_add(GTK_CONTAINER(frame), vbox);
            
            /* "Override Application Setting" checkbox */
            
            check_button = gtk_check_button_new_with_label
                ("Override Application Setting");
            
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                         override);

            g_signal_connect(G_OBJECT(check_button), "toggled",
                             G_CALLBACK(log_aniso_app_override_toggled),
                             (gpointer) ctk_multisample);
                             
            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME
                             (NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED),
                             G_CALLBACK(log_app_override_update_received),
                             (gpointer) ctk_multisample);

            ctk_config_set_tooltip(ctk_config, check_button,
                                   __aniso_override_app_help);

            gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

            ctk_multisample->log_aniso_app_override_check_button =check_button;
            
            /* Aniso scale */

            scale = gtk_hscale_new_with_range(valid.u.range.min,
                                              valid.u.range.max, 1);
            gtk_range_set_value(GTK_RANGE(scale), val);

            gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
            gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);

            gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);
            
            g_signal_connect(G_OBJECT(scale), "format-value",
                             G_CALLBACK(format_log_aniso_value),
                             (gpointer) ctk_multisample);

            g_signal_connect(G_OBJECT(scale), "value-changed",
                             G_CALLBACK(log_aniso_value_changed),
                             (gpointer) ctk_multisample);

            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME(NV_CTRL_LOG_ANISO),
                             G_CALLBACK(log_aniso_range_update_received),
                             (gpointer) ctk_multisample);
            
            ctk_config_set_tooltip(ctk_config, scale, __aniso_slider_help);

            ctk_multisample->active_attributes |= __LOG_ANISO_RANGE;
            ctk_multisample->log_aniso_scale = scale;

            gtk_widget_set_sensitive(GTK_WIDGET(scale), override);
        }
    }
    
    /*
     * Texture sharpen
     *
     * If one of the supported multisample modes was enabled by the
     * user, this check button controls texture sharpening.
     */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_TEXTURE_SHARPEN, &val);

    if (ret == NvCtrlSuccess) {
        
        /* create "TextureSharpening" frame */

        frame = gtk_frame_new("Texture Quality");
        gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
        
        /* create the vbox to store the widgets inside the frame */

        vbox = gtk_vbox_new(FALSE, 5);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        
        /* "Texture Sharpening" checkbox */
        
        check_button = gtk_check_button_new_with_label("Texture Sharpening");
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);
        
        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);
        
        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(texture_sharpening_toggled),
                         (gpointer) ctk_multisample);
        
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_TEXTURE_SHARPEN),
                         G_CALLBACK(texture_sharpening_update_received),
                         (gpointer) ctk_multisample);
        
        ctk_config_set_tooltip(ctk_config, check_button,
                               __texture_sharpening_help);
        
        ctk_multisample->active_attributes |= __TEXTURE_SHARPEN;
        ctk_multisample->texture_sharpening_button = check_button;
    }
    
    /* if nothing is available, teardown this object and return NULL */

    if (!ctk_multisample->active_attributes) {
        /* XXX how to teardown? */
        return NULL;
    }
    
    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);

} /* ctk_multisample_new() */



/*
 * build_fsaa_translation_table() - given the ValidValues rec for the
 * FSAA_MODE attribute, scan through the bits.ints field (which has
 * bits set to indicate which integer attributes are valid for the
 * attribute), assigning fsaa_translation_table[] as appropriate.
 * fsaa_translation_table[] will map from slider value to
 * NV_CTRL_FSAA_MODE value.
 */

static void build_fsaa_translation_table(CtkMultisample *ctk_multisample,
                                         NVCTRLAttributeValidValuesRec valid)
{
    gint i, n = 0;
    gint index_8xs = -1;
    gint index_16x = -1;
    gint mask = valid.u.bits.ints;

    ctk_multisample->fsaa_translation_table_size = 0;

    memset(ctk_multisample->fsaa_translation_table, 0,
           sizeof(gint) * (NV_CTRL_FSAA_MODE_MAX + 1));

    if (valid.type != ATTRIBUTE_TYPE_INT_BITS) return;
    
    for (i = 0; i <= NV_CTRL_FSAA_MODE_MAX; i++) {
        if (mask & (1 << i)) {
            ctk_multisample->fsaa_translation_table[n] = i;

            /* index_8xs and index_16x are needed below */

            if (i == NV_CTRL_FSAA_MODE_8xS) index_8xs = n;
            if (i == NV_CTRL_FSAA_MODE_16x) index_16x = n;
            
            n++;
        }
    }
    
    /*
     * XXX 8xS was added to the NV_CTRL_FSAA_MODE list after 16x, but
     * should appear before it in the slider.  If both were added to
     * the fsaa_translation_table[], then swap their positions.
     */

    if ((index_8xs != -1) && (index_16x != -1)) {
        ctk_multisample->fsaa_translation_table[index_8xs] =
            NV_CTRL_FSAA_MODE_16x;
        ctk_multisample->fsaa_translation_table[index_16x] =
            NV_CTRL_FSAA_MODE_8xS;
    }
    
    ctk_multisample->fsaa_translation_table_size = n;

} /* build_fsaa_translation_table() */



/*
 * map_nv_ctrl_fsaa_value_to_slider() - given an NV_CTRL_FSAA_MODE_*
 * value, map that to a slider value.  There is no good way to do
 * this, so just scan the lookup table for the NV_CTRL value and
 * return the table index.
 */

static int map_nv_ctrl_fsaa_value_to_slider(CtkMultisample *ctk_multisample,
                                            int value)
{
    int i;

    for (i = 0; i < ctk_multisample->fsaa_translation_table_size; i++) {
        if (ctk_multisample->fsaa_translation_table[i] == value) return i;
    }

    return 0;
    
} /* map_nv_ctrl_fsaa_value_to_slider() */



/*
 * format_fsaa_value() - callback for the "format-value" signal from
 * the fsaa scale; return a string describing the current value of the
 * scale.
 */

static gchar *format_fsaa_value(GtkScale *scale, gdouble arg1,
                                gpointer user_data)
{
    CtkMultisample *ctk_multisample;
    gint val;

    ctk_multisample = CTK_MULTISAMPLE(user_data);

    val = arg1;
    if (val > NV_CTRL_FSAA_MODE_MAX) val = NV_CTRL_FSAA_MODE_MAX;
    if (val < 0) val = 0;
    val = ctk_multisample->fsaa_translation_table[val];
    
    return g_strdup(get_multisample_mode_name(val));

} /* format_fsaa_value() */



/*
 * get_multisample_mode_name() - lookup a string desribing the
 * NV-CONTROL constant.
 */

static const gchar *get_multisample_mode_name(gint multisample_mode)
{
    static const gchar *mode_names[] = {
        
        "Off",                 /* FSAA_MODE_NONE  */
        "2x Bilinear",         /* FSAA_MODE_2x    */
        "2x Quincunx",         /* FSAA_MODE_2x_5t */
        "1.5 x 1.5",           /* FSAA_MODE_15x15 */
        "2 x 2 Supersampling", /* FSAA_MODE_2x2   */
        "4x Bilinear",         /* FSAA_MODE_4x    */
        "4x, 9-tap Gaussian",  /* FSAA_MODE_4x_9t */
        "8x",                  /* FSAA_MODE_8x    */
        "16x",                 /* FSAA_MODE_16x   */
        "8xS"                  /* FSAA_MODE_8xS   */
    };
    
    if ((multisample_mode < NV_CTRL_FSAA_MODE_NONE) ||
        (multisample_mode > NV_CTRL_FSAA_MODE_MAX)) {
        return "Unknown Multisampling";
    }
    
    return mode_names[multisample_mode];
    
} /* get_multisample_mode_name() */



/*
 * post_fsaa_app_override_toggled() - helper function for
 * fsaa_app_override_toggled() and fsaa_app_override_update_received();
 * this does whatever work is necessary after the app control check
 * button has been toggled -- update the slider's sensitivity and post
 * a statusbar message.
 */

static void post_fsaa_app_override_toggled(CtkMultisample *ctk_multisample,
                                           gboolean override)
{
    if (ctk_multisample->fsaa_scale) {
        gtk_widget_set_sensitive
            (GTK_WIDGET(ctk_multisample->fsaa_scale), override);
    }

    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "Application Antialiasing Override %s.",
                                 override ? "enabled" : "disabled");

} /* post_fsaa_app_override_toggled() */



/*
 * fsaa_app_override_toggled() - called when the FSAA Application
 * override check button is toggled; update the server and set the
 * sensitivity of the fsaa slider.
 */

static void fsaa_app_override_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    GtkRange *range = GTK_RANGE(ctk_multisample->fsaa_scale);
    gboolean override;

    override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_multisample->handle,
                       NV_CTRL_FSAA_APPLICATION_CONTROLLED, !override);
    if (!override) {
        NvCtrlSetAttribute(ctk_multisample->handle,
                           NV_CTRL_FSAA_MODE, NV_CTRL_FSAA_MODE_NONE);

        g_signal_handlers_block_by_func(G_OBJECT(range),
                                        G_CALLBACK(fsaa_value_changed),
                                        (gpointer) ctk_multisample);

        gtk_range_set_value(range, NV_CTRL_FSAA_MODE_NONE);

        g_signal_handlers_unblock_by_func(G_OBJECT(range),
                                          G_CALLBACK(fsaa_value_changed),
                                          (gpointer) ctk_multisample);
    }

    post_fsaa_app_override_toggled(ctk_multisample, override);

} /* fsaa_app_override_toggled() */



/*
 * fsaa_app_override_update_received() - callback function for when the
 * NV_CTRL_FSAA_APPLICATION_CONTROLLED attribute is changed by another
 * NV-CONTROL client.
 */

static void fsaa_app_override_update_received(GtkObject *object,
                                              gpointer arg1,
                                              gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    GtkWidget *check_button;
    gboolean override = !event_struct->value;
    
    check_button = ctk_multisample->fsaa_app_override_check_button;
    
    g_signal_handlers_block_by_func(G_OBJECT(check_button),
                                    G_CALLBACK(fsaa_app_override_toggled),
                                    (gpointer) ctk_multisample);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), override);
    
    post_fsaa_app_override_toggled(ctk_multisample, override);
    
    g_signal_handlers_unblock_by_func(G_OBJECT(check_button),
                                      G_CALLBACK(fsaa_app_override_toggled),
                                      (gpointer) ctk_multisample);

} /* fsaa_app_override_update_received() */



/*
 * post_fsaa_value_changed() - helper function for
 * fsaa_value_changed() and fsaa_update_received(); this does whatever
 * work is necessary after the fsaa value is changed -- currently just
 * post a statusbar message.
 */

static void post_fsaa_value_changed(CtkMultisample *ctk_multisample, gint val)
{
    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "Antialiasing set to %s.",
                                 get_multisample_mode_name(val));
    
} /* post_fsaa_value_changed() */



/*
 * fsaa_value_changed() - callback for the "value-changed" signal from
 * fsaa scale.
 */

static void fsaa_value_changed(GtkRange *range, gpointer user_data)
{
    CtkMultisample *ctk_multisample;
    gint val;

    ctk_multisample = CTK_MULTISAMPLE(user_data); 

    val = gtk_range_get_value(range);
    if (val > NV_CTRL_FSAA_MODE_MAX) val = NV_CTRL_FSAA_MODE_MAX;
    if (val < 0) val = 0;
    val = ctk_multisample->fsaa_translation_table[val];

    NvCtrlSetAttribute(ctk_multisample->handle, NV_CTRL_FSAA_MODE, val);
    
    post_fsaa_value_changed(ctk_multisample, val);

} /* fsaa_value_changed() */



/*
 * fsaa_update_received() - callback function for when the
 * NV_CTRL_FSAA_MODE attribute is changed by another NV-CONTROL
 * client.
 */

static void fsaa_update_received(GtkObject *object,
                                 gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkMultisample *ctk_multisample;
    GtkRange *range;
    gint val;

    event_struct = (CtkEventStruct *) arg1;
    ctk_multisample = CTK_MULTISAMPLE(user_data);
    range = GTK_RANGE(ctk_multisample->fsaa_scale);

    val = map_nv_ctrl_fsaa_value_to_slider(ctk_multisample,
                                           event_struct->value);

    g_signal_handlers_block_by_func(G_OBJECT(range),
                                    G_CALLBACK(fsaa_value_changed),
                                    (gpointer) ctk_multisample);
    
    gtk_range_set_value(range, val);
    
    g_signal_handlers_unblock_by_func(G_OBJECT(range),
                                      G_CALLBACK(fsaa_value_changed),
                                      (gpointer) ctk_multisample);

    post_fsaa_value_changed(ctk_multisample, event_struct->value);

} /* fsaa_update_received() */



/*
 * post_log_aniso_app_override_toggled() - helper function for
 * log_aniso_app_override_toggled() and
 * log_aniso_app_override_update_received(); this does whatever work is
 * necessary after the app control check button has been toggled --
 * update the slider's sensitivity and post a statusbar message.
 */

static void
post_log_aniso_app_override_toggled(CtkMultisample *ctk_multisample,
                                    gboolean override)
{
    if (ctk_multisample->log_aniso_scale) {
        gtk_widget_set_sensitive
            (GTK_WIDGET(ctk_multisample->log_aniso_scale), override);
    }
    
    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "Application Anisotropic Filtering Override "
                                 "%s.", override ? "enabled" : "disabled");

} /* post_log_aniso_app_override_toggled() */



/*
 * log_aniso_app_override_toggled() - called when the LOG_ANISO
 * "Override Application Setting" check button is toggled; update the
 * server and set the sensitivity of the log_aniso slider.
 */

static void log_aniso_app_override_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    GtkRange *range = GTK_RANGE(ctk_multisample->log_aniso_scale);
    gboolean override;

    override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_multisample->handle,
                       NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED, !override);
    if (!override) {
        NvCtrlSetAttribute(ctk_multisample->handle,
                           NV_CTRL_LOG_ANISO, 0 /* default(?) */);

        g_signal_handlers_block_by_func(G_OBJECT(range),
                                        G_CALLBACK(log_aniso_value_changed),
                                        (gpointer) ctk_multisample);

        gtk_range_set_value(range, 0);

        g_signal_handlers_unblock_by_func(G_OBJECT(range),
                                          G_CALLBACK(log_aniso_value_changed),
                                          (gpointer) ctk_multisample);
    }

    post_log_aniso_app_override_toggled(ctk_multisample, override);

} /* log_aniso_app_override_toggled() */



/*
 * log_app_override_update_received() - callback function for when the
 * NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED attribute is changed by
 * another NV-CONTROL client.
 */

static void log_app_override_update_received(GtkObject *object,
                                             gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gboolean override = !event_struct->value;
    GtkWidget *check_button;

    check_button = ctk_multisample->log_aniso_app_override_check_button;

    g_signal_handlers_block_by_func(G_OBJECT(check_button),
                                    G_CALLBACK(log_aniso_app_override_toggled),
                                    (gpointer) ctk_multisample);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), override);
    
    post_log_aniso_app_override_toggled(ctk_multisample, override);

    g_signal_handlers_unblock_by_func(G_OBJECT(check_button),
                                      G_CALLBACK
                                      (log_aniso_app_override_toggled),
                                      (gpointer) ctk_multisample);
    
} /* log_app_override_update_received() */



/*
 * get_log_aniso_name() - translate a log_aniso integer value to an
 * aniso name.
 */

static const gchar *get_log_aniso_name(gint val)
{
    static const gchar *log_aniso_names[] = { "1x", "2x", "4x", "8x", "16x" };
    
    if ((val < 0) || (val > 4)) return "Unknown";

    return log_aniso_names[val];

} /* get_log_aniso_name() */



/*
 * format_log_aniso_value() - callback for the "format-value" signal
 * from the log aniso scale.
 */

static gchar *format_log_aniso_value(GtkScale *scale, gdouble arg1,
                                     gpointer user_data)
{
    return g_strdup(get_log_aniso_name(arg1));
    
} /* format_log_aniso_value() */



/*
 * post_log_aniso_value_changed() - helper function for
 * log_aniso_value_changed(); this does whatever work is necessary
 * after the log aniso value has changed -- currently just post a
 * statusbar message.
 */

static void post_log_aniso_value_changed(CtkMultisample *ctk_multisample,
                                         gint val)
{
    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "Anisotropic Filtering set to %s.",
                                 get_log_aniso_name(val));
    
} /* post_log_aniso_value_changed() */



/*
 * log_aniso_value_changed() - callback for the "value-changed" signal
 * from the log aniso scale.
 */

static void log_aniso_value_changed(GtkRange *range, gpointer user_data)
{
    CtkMultisample *ctk_multisample;
    gint val;

    ctk_multisample = CTK_MULTISAMPLE(user_data);

    val = gtk_range_get_value(range);

    NvCtrlSetAttribute(ctk_multisample->handle, NV_CTRL_LOG_ANISO, val);
    
    post_log_aniso_value_changed(ctk_multisample, val);

} /* log_aniso_value_changed() */



/*
 * log_aniso_range_update_received() - callback function for when the
 * NV_CTRL_LOG_ANISO attribute is changed by another NV-CONTROL
 * client.
 */

static void log_aniso_range_update_received(GtkObject *object,
                                            gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkMultisample *ctk_multisample;
    GtkRange *range;

    event_struct = (CtkEventStruct *) arg1;
    ctk_multisample = CTK_MULTISAMPLE(user_data);
    range = GTK_RANGE(ctk_multisample->log_aniso_scale);

    g_signal_handlers_block_by_func(G_OBJECT(range),
                                    G_CALLBACK(log_aniso_value_changed),
                                    (gpointer) ctk_multisample);
    
    gtk_range_set_value(range, event_struct->value);
    
    post_log_aniso_value_changed(ctk_multisample, event_struct->value);

    g_signal_handlers_unblock_by_func(G_OBJECT(range),
                                      G_CALLBACK(log_aniso_value_changed),
                                      (gpointer) ctk_multisample);
    
} /* log_aniso_range_update_received() */



/*
 * post_texture_sharpening_toggled() - helper function for
 * texture_sharpening_toggled() and
 * texture_sharpening_update_received(); this does whatever work is
 * necessary after the texture sharpening button has been toggled --
 * currently, just post a statusbar message.
 */

static void post_texture_sharpening_toggled(CtkMultisample *ctk_multisample,
                                            gboolean enabled)
{
    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "Texture sharpening %s.",
                                 enabled ? "enabled" : "disabled");
    
} /* post_texture_sharpening_toggled() */



/*
 * texture_sharpening_toggled() - callback for the "toggled" signal
 * from the texture sharpening check button.
 */

static void texture_sharpening_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkMultisample *ctk_multisample;
    gboolean enabled;

    ctk_multisample = CTK_MULTISAMPLE(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_multisample->handle, NV_CTRL_TEXTURE_SHARPEN,
                       enabled);

    post_texture_sharpening_toggled(ctk_multisample, enabled);
    
} /* texture_sharpening_toggled() */



/*
 * texture_sharpening_update_received() - callback function for when
 * the NV_CTRL_TEXTURE_SHARPEN attribute is changed by another
 * NV-CONTROL client.
 */

static void texture_sharpening_update_received(GtkObject *object,
                                               gpointer arg1,
                                               gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkMultisample *ctk_multisample;
    GtkToggleButton *button;
    
    event_struct = (CtkEventStruct *) arg1;
    ctk_multisample = CTK_MULTISAMPLE(user_data);
    button = GTK_TOGGLE_BUTTON(ctk_multisample->texture_sharpening_button);

    g_signal_handlers_block_by_func(G_OBJECT(button),
                                    G_CALLBACK(texture_sharpening_toggled),
                                    (gpointer) ctk_multisample);

    gtk_toggle_button_set_active(button, event_struct->value);
    
    post_texture_sharpening_toggled(ctk_multisample, event_struct->value);

    g_signal_handlers_unblock_by_func(G_OBJECT(button), 
                                      G_CALLBACK(texture_sharpening_toggled),
                                      (gpointer) ctk_multisample);
    
} /* texture_sharpening_update_received() */



/*
 * ctk_multisample_create_help() - create a GtkTextBuffer describing
 * the available image quality options.
 */

GtkTextBuffer *ctk_multisample_create_help(GtkTextTagTable *table,
                                           CtkMultisample *ctk_multisample)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "Antialiasing Help");

    if (ctk_multisample->active_attributes & __FSAA) {
        ctk_help_heading(b, &i, "Antialiasing Settings");
        ctk_help_para(b, &i, "Antialiasing is a technique used in OpenGL "
                      "to smooth the edges of objects in a scene to reduce "
                      "the jagged 'stairstep' effect that sometimes appears "
                      "along the edges of 3D objects.  This is accomplished "
                      "by rendering an image larger than normal (with "
                      "multiple 'samples' per pixel), and then using a "
                      "filter to average multiple samples into a "
                      "single pixel.");
        
        ctk_help_para(b, &i, "Several antialiasing "
                      "methods are available which you may select between "
                      "with the Antialiasing slider.  Note that increasing "
                      "the number of samples used during Antialiased "
                      "rendering may decrease performance.");

        ctk_help_para(b, &i, "You can also configure Antialiasing "
                      "using the __GL_FSAA_MODE environment varible (see "
                      "the README for details).  The __GL_FSAA_MODE "
                      "environment variable overrides the value in "
                      "nvidia-settings.");
        
        ctk_help_term(b, &i, "Override Application Setting");
        
        ctk_help_para(b, &i, __aa_override_app_help);

        if (ctk_multisample->active_attributes & __FSAA_NONE) {
            ctk_help_term(b, &i, "Off");
            ctk_help_para(b, &i, "Disables antialiasing in OpenGL "
                          "applications. "
                          "Select this option if you require maximum "
                          "performance in your applications.");
        }
        
        if (ctk_multisample->active_attributes & __FSAA_2x) {
            ctk_help_term(b, &i, "2x Bilinear");
            ctk_help_para(b, &i, "This enables antialiasing using the 2x "
                          "Bilinear mode.  This mode offers improved image "
                          "quality and high performance in OpenGL "
                          "applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_2x_5t) {
            ctk_help_term(b, &i, "2x Quincunx");
            ctk_help_para(b, &i, "This enables the patented Quincunx "
                          "Antialiasing technique available in the GeForce "
                          "GPU family. "
                          "Quincunx Antialiasing offers the quality of the "
                          "slower, 4x antialiasing mode, but at nearly the "
                          "performance of the faster, 2x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_15x15) {
            ctk_help_term(b, &i, "1.5 x 1.5");
            ctk_help_para(b, &i, "This enables antialiasing using the 1.5x1.5 "
                          "mode.  This mode offers improved image quality and "
                          "high performance in OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_2x2) {
            ctk_help_term(b, &i, "2 x 2 Supersampling");
            ctk_help_para(b, &i, "This enables antialiasing using the 2x2 "
                          "Supersampling mode. This mode offers higher image "
                          "quality at the expense of some performance in "
                          "OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_4x) {
            ctk_help_term(b, &i, "4x Bilinear");
            ctk_help_para(b, &i, "This enables antialiasing using the 4x "
                          "Bilinearmode. This mode offers higher image "
                          "quality at the expense of some performance in "
                          "OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_4x_9t) {
            ctk_help_term(b, &i, "4x, 9-tap Gaussian");
            ctk_help_para(b, &i, "This enables antialiasing using the 4x, "
                          "9-tap (Gaussian) mode. This mode offers higher "
                          "image quality but at the expense of some "
                          "performance in OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_8x) {
            ctk_help_term(b, &i, "8x");
            ctk_help_para(b, &i, "This enables antialiasing using the 8x "
                          "mode.  This mode offers better image quality than "
                          "the 4x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_8xS) {
            ctk_help_term(b, &i, "8xS");
            ctk_help_para(b, &i, "This enables antialiasing using the 8xS "
                          "mode.  This mode offers better image quality than "
                          "the 4x mode; 8xS is only available on Geforce "
                          "(non-Quadro) FX or better GPUs.");
        }

        if (ctk_multisample->active_attributes & __FSAA_16x) {
            ctk_help_term(b, &i, "16x");
            ctk_help_para(b, &i, "This enables antialiasing using the 16x "
                          "mode.  This mode offers better image quality than "
                          "the 8x mode.");
        }
    }

    if (ctk_multisample->active_attributes & __LOG_ANISO_RANGE) {
        ctk_help_heading(b, &i, "Anisotropic Filtering");
        
        ctk_help_para(b, &i, "Anisotropic filtering is a technique used to "
                      "improve the quality of textures applied to the "
                      "surfaces of 3D objects when drawn at a sharp angle. "
                      "Use the Anisotropic filtering slider to set the degree "
                      "of anisotropic filtering for improved image quality. "
                      "Enabling this option improves image quality at the "
                      "expense of some performance.");

        ctk_help_para(b, &i, "You can also configure Anisotropic filtering "
                      "using the __GL_LOG_MAX_ANISO environment varible "
                      "(see the README for details).  The "
                      "__GL_LOG_MAX_ANISO environment variable overrides "
                      "the value in nvidia-settings.");
        
        ctk_help_term(b, &i, "Override Application Setting");
        
        ctk_help_para(b, &i, __aniso_override_app_help);
        
        ctk_help_para(b, &i, __aniso_slider_help);
    }

    if (ctk_multisample->active_attributes & __TEXTURE_SHARPEN) {
        ctk_help_heading(b, &i, "Texture Sharpening");
        ctk_help_para(b, &i, __texture_sharpening_help);
    }

    ctk_help_finish(b);

    return b;

} /* ctk_multisample_create_help() */
