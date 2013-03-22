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

#include <gtk/gtk.h>
#include <string.h>

#include "NvCtrlAttributes.h"

#include "ctkmultisample.h"

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkbanner.h"


/* local prototypes */

static void build_fsaa_translation_table(CtkMultisample *ctk_multisample,
                                         NVCTRLAttributeValidValuesRec valid);

static int map_nv_ctrl_fsaa_value_to_slider(CtkMultisample *ctk_multisample,
                                            int value);


static gchar *format_fsaa_value(GtkScale *scale, gdouble arg1,
                                gpointer user_data);

static GtkWidget *create_fsaa_setting_menu(CtkMultisample *ctk_multisample,
                                           CtkEvent *ctk_event,
                                           gboolean override,
                                           gboolean enhance);

static void fsaa_setting_checkbox_toggled(GtkWidget *widget,
                                          gpointer user_data);

static void fsaa_setting_menu_changed(GtkObject *object, gpointer user_data);

static void fsaa_setting_update_received(GtkObject *object,
                                         gpointer arg1,
                                         gpointer user_data);

static void post_fsaa_value_changed(CtkMultisample *ctk_multisample, gint val);

static void fsaa_value_changed(GtkRange *range, gpointer user_data);

static void fsaa_update_received(GtkObject *object,
                                 gpointer arg1, gpointer user_data);

static void fxaa_checkbox_toggled(GtkWidget *widget,
                                  gpointer user_data);

static void fxaa_update_received(GtkObject *object,
                                 gpointer arg1, gpointer user_data);

static void post_fxaa_toggled(CtkMultisample *ctk_multisample, 
                              gboolean enable);

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

static void update_fxaa_from_fsaa_change(CtkMultisample *ctk_multisample,
                                         int fsaa_value);
static void update_fsaa_from_fxaa_change(CtkMultisample *ctk_multisample,
                                         gboolean fxaa_enabled);
static gchar *applicationSettings[] = {
    "Use Application Settings",
    "Override Application Settings",
    "Enhance Application Settings"
};

static const char *__aa_override_app_help =
"Enable the Antialiasing \"Override Application Setting\" "
"checkbox to make the antialiasing slider active and "
"override any application antialiasing setting with the "
"value of the slider.";

static const char *__aa_menu_help =
"The Application Antialiasing Settings Menu allows the antialiasing "
"setting of OpenGL applications to be overridden with the value of "
"the slider.";

static const char *__aa_slider_help =
"The Antialiasing slider controls the level of antialiasing. Using "
"antialiasing disables FXAA.";

static const char *__aniso_override_app_help =
"Enable the Anisotropic Filtering \"Override Application Setting\" "
"checkbox to make the anisotropic filtering slider "
"active and override any application anisotropic "
"filtering setting with the value of the slider.";

static const char *__aniso_slider_help =
"The Anisotropic Filtering slider controls the "
"level of automatic anisotropic texture filtering.";

static const char *__fxaa_enable_help = 
"Enable Fast Approximate Anti-Aliasing. This option is applied to "
"OpenGL applications that are started after this option is set. Enabling "
"FXAA disables triple buffering, antialiasing, and other antialiasing "
"setting methods.";

static const char *__texture_sharpening_help =
"To improve image quality, select this option "
"to sharpen textures when running OpenGL applications "
"with antialiasing enabled.";


/*
 * bits indicating which attributes require documenting in the online
 * help
 */

#define __FSAA_NONE       (1 << NV_CTRL_FSAA_MODE_NONE)
#define __FSAA_2x         (1 << NV_CTRL_FSAA_MODE_2x)
#define __FSAA_2x_5t      (1 << NV_CTRL_FSAA_MODE_2x_5t)
#define __FSAA_15x15      (1 << NV_CTRL_FSAA_MODE_15x15)
#define __FSAA_2x2        (1 << NV_CTRL_FSAA_MODE_2x2)
#define __FSAA_4x         (1 << NV_CTRL_FSAA_MODE_4x)
#define __FSAA_4x_9t      (1 << NV_CTRL_FSAA_MODE_4x_9t)
#define __FSAA_8x         (1 << NV_CTRL_FSAA_MODE_8x)
#define __FSAA_16x        (1 << NV_CTRL_FSAA_MODE_16x)
#define __FSAA_8xS        (1 << NV_CTRL_FSAA_MODE_8xS)
#define __FSAA_8xQ        (1 << NV_CTRL_FSAA_MODE_8xQ)
#define __FSAA_16xS       (1 << NV_CTRL_FSAA_MODE_16xS)
#define __FSAA_16xQ       (1 << NV_CTRL_FSAA_MODE_16xQ)
#define __FSAA_32xS       (1 << NV_CTRL_FSAA_MODE_32xS)
#define __FSAA_32x        (1 << NV_CTRL_FSAA_MODE_32x)
#define __FSAA_64xS       (1 << NV_CTRL_FSAA_MODE_64xS)
#define __FSAA            (1 << (NV_CTRL_FSAA_MODE_MAX + 1))
#define __FSAA_ENHANCE    (1 << (NV_CTRL_FSAA_MODE_MAX + 2))
#define __FXAA            (1 << (NV_CTRL_FSAA_MODE_MAX + 3))
#define __LOG_ANISO_RANGE (1 << (NV_CTRL_FSAA_MODE_MAX + 4))
#define __TEXTURE_SHARPEN (1 << (NV_CTRL_FSAA_MODE_MAX + 5))


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
            NULL  /* value_table */
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
    GtkWidget *menu;
    GtkWidget *scale;
    GtkObject *adjustment;
    gint min, max;
    
    gint val, app_control, override, enhance, mode, i;
    
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
        
        ret = NvCtrlGetAttribute(handle, NV_CTRL_FSAA_MODE, &mode);
        
        val = map_nv_ctrl_fsaa_value_to_slider(ctk_multisample, mode);

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
            
            /* "Application Setting" widget */
            
            ret = NvCtrlGetAttribute(ctk_multisample->handle,
                                     NV_CTRL_FSAA_APPLICATION_ENHANCED,
                                     &enhance);

            if (ret == NvCtrlSuccess) {
                /* Create a menu */

                ctk_multisample->active_attributes |= __FSAA_ENHANCE;

                menu = create_fsaa_setting_menu(ctk_multisample, ctk_event,
                                                override, enhance);
                
                ctk_multisample->fsaa_menu = menu;
                
                hbox = gtk_hbox_new(FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hbox), menu, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

            } else {
                /* Create a checkbox */
                
                check_button = gtk_check_button_new_with_label
                    ("Override Application Setting");
                
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                             override);
                
                g_signal_connect(G_OBJECT(check_button), "toggled",
                                 G_CALLBACK(fsaa_setting_checkbox_toggled),
                                 (gpointer) ctk_multisample);
                
                ctk_config_set_tooltip(ctk_config, check_button,
                                       __aa_override_app_help);

                gtk_box_pack_start(GTK_BOX(vbox), check_button,
                                   FALSE, FALSE, 0);

                ctk_multisample->fsaa_app_override_check_button = check_button;
            }

            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME
                             (NV_CTRL_FSAA_APPLICATION_CONTROLLED),
                             G_CALLBACK(fsaa_setting_update_received),
                             (gpointer) ctk_multisample);

            /* Antialiasing scale */

            min = 0;
            max = ctk_multisample->fsaa_translation_table_size - 1;

            /* create the slider */
            adjustment = gtk_adjustment_new(val, min, max, 1, 1, 0.0);
            scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
            gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), val);
            
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
                    (1 << ctk_multisample->fsaa_translation_table[i]);

            /* FXAA Option button */

            check_button = gtk_check_button_new_with_label("Enable FXAA");

            if (mode == NV_CTRL_FSAA_MODE_NONE) {
                ret = NvCtrlGetAttribute(handle, NV_CTRL_FXAA, &val);
                if (val == NV_CTRL_FXAA_ENABLE) {
                    gtk_widget_set_sensitive(GTK_WIDGET(scale), FALSE);
                }
            } else {
                val = NV_CTRL_FXAA_DISABLE;
            }
            gtk_widget_set_sensitive(GTK_WIDGET(check_button),
                                     (mode == NV_CTRL_FSAA_MODE_NONE));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

            g_signal_connect(G_OBJECT(check_button), "toggled",
                             G_CALLBACK(fxaa_checkbox_toggled),
                             (gpointer) ctk_multisample);

            g_signal_connect(G_OBJECT(ctk_event),
                             CTK_EVENT_NAME(NV_CTRL_FXAA),
                             G_CALLBACK(fxaa_update_received),
                             (gpointer) ctk_multisample);

            ctk_config_set_tooltip(ctk_config, check_button,
                                   __fxaa_enable_help);
            gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

            ctk_multisample->active_attributes |= __FXAA;
            ctk_multisample->fxaa_enable_check_button = check_button;
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

            min = valid.u.range.min;
            max = valid.u.range.max;

            /* create the slider */
            adjustment = gtk_adjustment_new(val, min, max, 1, 1, 0.0);
            scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
            gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), val);

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
    gint index_32x = -1;
    gint index_32xs = -1;
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
            if (i == NV_CTRL_FSAA_MODE_32x) index_32x = n;
            if (i == NV_CTRL_FSAA_MODE_32xS) index_32xs = n;
            
            n++;
        }
    }
    
    /*
     * XXX 8xS was added to the NV_CTRL_FSAA_MODE list after 16x, but should
     * appear before it in the slider.  Same with 32x and 32xS.  If both were
     * added to the fsaa_translation_table[], then re-order them appropriately.
     */

    if ((index_8xs != -1) && (index_16x != -1)) {
        ctk_multisample->fsaa_translation_table[index_8xs] =
            NV_CTRL_FSAA_MODE_16x;
        ctk_multisample->fsaa_translation_table[index_16x] =
            NV_CTRL_FSAA_MODE_8xS;
    }

    if ((index_32x != -1) && (index_32xs != -1)) {
        ctk_multisample->fsaa_translation_table[index_32x] =
            NV_CTRL_FSAA_MODE_32xS;
        ctk_multisample->fsaa_translation_table[index_32xs] =
            NV_CTRL_FSAA_MODE_32x;
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
    
    return g_strdup(NvCtrlGetMultisampleModeName(val));

} /* format_fsaa_value() */



/*
 * create_fsaa_setting_menu() - Helper function that creates the
 * FSAA application control dropdown menu.
 */

static GtkWidget *create_fsaa_setting_menu(CtkMultisample *ctk_multisample,
                                           CtkEvent *ctk_event,
                                           gboolean override, gboolean enhance)
{
    CtkDropDownMenu *d;

    gint idx, i;

    /* Create the menu */

    d = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_COMBO);
    
    for (i = 0; i < ARRAY_LEN(applicationSettings); i++) {
        ctk_drop_down_menu_append_item(d, applicationSettings[i], i);
    }

    if (!override) {
        idx = 0;
    } else {
        if (!enhance) {
            idx = 1;
        } else {
            idx = 2;
        }
    }

    /* set the menu item */
    ctk_drop_down_menu_set_current_value(d, idx);

    ctk_config_set_tooltip(ctk_multisample->ctk_config, d->menu,
                           __aa_menu_help);

    g_signal_connect(G_OBJECT(d),
                     "changed",
                     G_CALLBACK(fsaa_setting_menu_changed),
                     (gpointer) ctk_multisample);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME
                     (NV_CTRL_FSAA_APPLICATION_ENHANCED),
                     G_CALLBACK(fsaa_setting_update_received),
                     (gpointer) ctk_multisample);

    return GTK_WIDGET(d);

} /* create_fsaa_setting_menu() */



/*
 * post_fsaa_setting_changed() - helper function for update_fsaa_setting()
 * and fsaa_menu_update_received();  This does whatever work is necessary
 * after the dropdown/checkbox has changed -- update the slider's
 * sensitivity and post a statusbar message.
 */

static void post_fsaa_setting_changed(CtkMultisample *ctk_multisample,
                                      gboolean override, gboolean enhance)
{
    GtkWidget *fxaa_checkbox = ctk_multisample->fxaa_enable_check_button;
    gboolean fxaa_value = 
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fxaa_checkbox));

    if (ctk_multisample->fsaa_scale) {
        gtk_widget_set_sensitive
            (GTK_WIDGET(ctk_multisample->fsaa_scale), 
            (override &&
            (fxaa_value == NV_CTRL_FXAA_DISABLE)));
    }

    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "%s Application's Antialiasing Settings.",
                                 (!override ? "Using" :
                                  (enhance ? "Enhancing" : "Overriding")));

} /* post_fsaa_setting_changed() */



/*
 * update_fsaa_setting() - Helper function for updating the server when the
 * user changes the Application's Antialiasing settings.
 *
 */

static void update_fsaa_setting(CtkMultisample *ctk_multisample,
                                gboolean override, gboolean enhance)
{
    GtkRange *range = GTK_RANGE(ctk_multisample->fsaa_scale);

    NvCtrlSetAttribute(ctk_multisample->handle,
                       NV_CTRL_FSAA_APPLICATION_CONTROLLED, !override);
    
    if (ctk_multisample->active_attributes & __FSAA_ENHANCE) {
        NvCtrlSetAttribute(ctk_multisample->handle,
                           NV_CTRL_FSAA_APPLICATION_ENHANCED, enhance);
    }

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

        update_fxaa_from_fsaa_change(ctk_multisample,
                                     NV_CTRL_FSAA_MODE_NONE);
    }

    post_fsaa_setting_changed(ctk_multisample, override, enhance);

} /* update_fsaa_setting() */



/*
 * fsaa_setting_checkbox_toggled() - called when the FSAA Application
 * checkbox is changed; update the server and set the sensitivity of
 * the fsaa slider.
 */

static void fsaa_setting_checkbox_toggled(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gboolean override;

    override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    update_fsaa_setting(ctk_multisample, override, FALSE /* enhance */ );

} /* fsaa_setting_checkbox_toggled() */



/*
 * fsaa_setting_menu_changed() - called when the FSAA Application
 * menu is changed; update the server and set the sensitivity of
 * the fsaa slider.
 */

static void fsaa_setting_menu_changed(GtkObject *object, gpointer user_data)
{
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gint idx;
    gboolean override;
    gboolean enhance;
    
    CTK_DROP_DOWN_MENU(ctk_multisample->fsaa_menu)->current_selected_item_widget
        = GTK_WIDGET(object);
    idx = ctk_drop_down_menu_get_current_value
        (CTK_DROP_DOWN_MENU(ctk_multisample->fsaa_menu)); 
    
    /* The FSAA dropdown menu is setup this way:
     *
     * 0 == app
     * 1 == override
     * 2 == enhance
     */

    override = (idx > 0);
    enhance = (idx == 2);

    update_fsaa_setting(ctk_multisample, override, enhance);

} /* fsaa_setting_menu_changed() */



/*
 * fsaa_setting_update_received() - callback function for when the
 * NV_CTRL_FSAA_APPLICATION_CONTROLLED/ENHANCE attribute is changed by another
 * NV-CONTROL client.
 */

static void fsaa_setting_update_received(GtkObject *object,
                                         gpointer arg1,
                                         gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gint idx;
    gboolean override;
    gboolean enhance = FALSE;

    gint val;
    ReturnStatus ret;


    switch (event_struct->attribute) {
    case NV_CTRL_FSAA_APPLICATION_CONTROLLED:
        override = !event_struct->value;

        if (!override) {
            idx = 0;
        } else if (ctk_multisample->active_attributes & __FSAA_ENHANCE) {
            ret = NvCtrlGetAttribute(ctk_multisample->handle,
                                     NV_CTRL_FSAA_APPLICATION_ENHANCED,
                                     &val);
            if (ret == NvCtrlSuccess) {
                enhance = val;
                idx = enhance ? 2 : 1;
            } else {
                enhance = FALSE;
                idx = 0;
            }
        } else {
            idx = 1;
        }
        break;

    case NV_CTRL_FSAA_APPLICATION_ENHANCED:
        enhance = event_struct->value;

        ret = NvCtrlGetAttribute(ctk_multisample->handle,
                                 NV_CTRL_FSAA_APPLICATION_CONTROLLED,
                                 &val);
        if (ret == NvCtrlSuccess) {
            override = !val; /* = !app_controlled */
        } else {
            override = FALSE;
        }

        if (override) {
            idx = enhance ? 2 : 1;
        } else {
            idx = 0;
        }
        break;

    default:
        return;
    }


    if (ctk_multisample->fsaa_menu) {
        /* Update the dropdown menu */
        GtkWidget *menu = ctk_multisample->fsaa_menu;

        g_signal_handlers_block_by_func
            (G_OBJECT(menu),
             G_CALLBACK(fsaa_setting_menu_changed),
             (gpointer) ctk_multisample);

        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(ctk_multisample->fsaa_menu), idx);
        
        g_signal_handlers_unblock_by_func
            (G_OBJECT(menu),
             G_CALLBACK(fsaa_setting_menu_changed),
             (gpointer) ctk_multisample);
    } else {
        /* Update the checkbox */
        GtkWidget *button = ctk_multisample->fsaa_app_override_check_button;

        g_signal_handlers_block_by_func
            (G_OBJECT(button), G_CALLBACK(fsaa_setting_checkbox_toggled),
             (gpointer) ctk_multisample);
    
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), override);
        
        g_signal_handlers_unblock_by_func
            (G_OBJECT(button), G_CALLBACK(fsaa_setting_checkbox_toggled),
             (gpointer) ctk_multisample);
    }

    post_fsaa_setting_changed(ctk_multisample, override, enhance);
        

} /* fsaa_setting_update_received() */



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
                                 NvCtrlGetMultisampleModeName(val));
    
} /* post_fsaa_value_changed() */


/*
 * update_fxaa_from_fsaa_change - helper function for changes to fsaa in order
 * to update fxaa and enable/disable fxaa or fsaa widgets based on the new 
 * value of fsaa.
 */

static void update_fxaa_from_fsaa_change(CtkMultisample *ctk_multisample,
                                         int fsaa_value)
{
    GtkRange *fsaa_range = GTK_RANGE(ctk_multisample->fsaa_scale);
    GtkWidget *fsaa_menu = ctk_multisample->fsaa_menu;
    GtkWidget *fxaa_checkbox = ctk_multisample->fxaa_enable_check_button;
    gboolean fxaa_value;

    /* The FSAA dropdown menu is: 0 == app, 1 == override, 2 == enhance */
    gint fsaa_idx = CTK_DROP_DOWN_MENU(fsaa_menu)->current_selected_item;
    
    if (fsaa_value != NV_CTRL_FSAA_MODE_NONE) {
        g_signal_handlers_block_by_func(G_OBJECT(fxaa_checkbox),
                                        G_CALLBACK(fxaa_checkbox_toggled),
                                        (gpointer) ctk_multisample);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fxaa_checkbox), FALSE);

        g_signal_handlers_unblock_by_func(G_OBJECT(fxaa_checkbox),
                                          G_CALLBACK(fxaa_checkbox_toggled),
                                          (gpointer) ctk_multisample);
    }

    fxaa_value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fxaa_checkbox));

    gtk_widget_set_sensitive(GTK_WIDGET(fsaa_range), 
                             (fsaa_idx != 0) && // not app controlled
                             (fxaa_value == NV_CTRL_FXAA_DISABLE));
    gtk_widget_set_sensitive(GTK_WIDGET(fxaa_checkbox),
                             fsaa_value == NV_CTRL_FSAA_MODE_NONE);
} /* update_fxaa_from_fsaa_change() */

/*
 * update_fsaa_from_fxaa_change - helper function for changes to fxaa in order
 * to update fsaa and enable/disable fxaa or fsaa widgets based on the new 
 * value of fxaa.
 */

static void update_fsaa_from_fxaa_change (CtkMultisample *ctk_multisample,
                                          gboolean fxaa_enabled)
{
    GtkWidget *fxaa_checkbox = ctk_multisample->fxaa_enable_check_button;
    GtkRange *fsaa_range = GTK_RANGE(ctk_multisample->fsaa_scale);
    gint fsaa_value_none = map_nv_ctrl_fsaa_value_to_slider(ctk_multisample, 0);
    GtkWidget *fsaa_menu = ctk_multisample->fsaa_menu;

    /* The FSAA dropdown menu is: 0 == app, 1 == override, 2 == enhance */
    gint fsaa_idx = CTK_DROP_DOWN_MENU(fsaa_menu)->current_selected_item;
    
    gint fsaa_val;

    if (fxaa_enabled == NV_CTRL_FXAA_ENABLE) {
        g_signal_handlers_block_by_func(G_OBJECT(fsaa_range),
                                        G_CALLBACK(fsaa_value_changed),
                                        (gpointer) ctk_multisample);

        gtk_range_set_value(fsaa_range, fsaa_value_none);

        g_signal_handlers_unblock_by_func(G_OBJECT(fsaa_range),
                                          G_CALLBACK(fsaa_value_changed),
                                          (gpointer) ctk_multisample);
    }

    fsaa_val = gtk_range_get_value(fsaa_range);
    if (fsaa_val > NV_CTRL_FSAA_MODE_MAX) fsaa_val = NV_CTRL_FSAA_MODE_MAX;
    if (fsaa_val < 0) fsaa_val = 0;
    fsaa_val = ctk_multisample->fsaa_translation_table[fsaa_val];

    gtk_widget_set_sensitive(GTK_WIDGET(fxaa_checkbox), 
                             (fxaa_enabled == NV_CTRL_FXAA_ENABLE) || 
                             (fsaa_val == NV_CTRL_FSAA_MODE_NONE));
    gtk_widget_set_sensitive(GTK_WIDGET(fsaa_range), 
                             (fsaa_idx != 0) && // not app controlled
                             (fxaa_enabled == NV_CTRL_FXAA_DISABLE));
} /* update_fsaa_from_fxaa_change */

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

    update_fxaa_from_fsaa_change(ctk_multisample, val);
    
    post_fsaa_value_changed(ctk_multisample, val);

} /* fsaa_value_changed() */


/*
 * fxaa_checkbox_toggled - callback for a change to
 * the FXAA settings in the control panel
 */
static void fxaa_checkbox_toggled(GtkWidget *widget,
                                  gpointer user_data)
{
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_multisample->handle, NV_CTRL_FXAA, enabled);

    update_fsaa_from_fxaa_change(ctk_multisample, enabled);

    post_fxaa_toggled(ctk_multisample, enabled);

} /* fxaa_checkbox_toggled */


/*
 * fxaa_update_received() - callback function for when the
 * NV_CTRL_FXAA attribute is changed by another NV-CONTROL
 * client.
 */

static void fxaa_update_received(GtkObject *object,
                                 gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkMultisample *ctk_multisample = CTK_MULTISAMPLE(user_data);
    gboolean fxaa_value = event_struct->value;
    GtkWidget *check_button = ctk_multisample->fxaa_enable_check_button;

    g_signal_handlers_block_by_func(G_OBJECT(check_button),
                                    G_CALLBACK(fxaa_checkbox_toggled),
                                    (gpointer) ctk_multisample);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), fxaa_value);
    
    update_fsaa_from_fxaa_change(ctk_multisample, fxaa_value);

    g_signal_handlers_unblock_by_func(G_OBJECT(check_button),
                                      G_CALLBACK(fxaa_checkbox_toggled),
                                      (gpointer) ctk_multisample);

    post_fxaa_toggled(ctk_multisample, fxaa_value);

} /* fxaa_update_received() */

/*
 * post_fxaa_toggled() - helper function for fxaa_button_toggled() 
 * and fxaa_update_received(); this does whatever work is necessary 
 * after the app control check button has been toggled.
 */

static void
post_fxaa_toggled(CtkMultisample *ctk_multisample, gboolean enable)
{
    ctk_config_statusbar_message(ctk_multisample->ctk_config,
                                 "FXAA "
                                 "%s.", enable ? "enabled" : "disabled");

} /* post_fxaa_toggled() */



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

    update_fxaa_from_fsaa_change(ctk_multisample, event_struct->value);
    
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
                      "using the __GL_FSAA_MODE environment variable (see "
                      "the README for details).  The __GL_FSAA_MODE "
                      "environment variable overrides the value in "
                      "nvidia-settings.");
        
        ctk_help_term(b, &i, "Application Antialiasing Settings");
        
        if (ctk_multisample->active_attributes & __FSAA_ENHANCE) {
            ctk_help_para(b, &i, __aa_menu_help);
            ctk_help_para(b, &i, "Use Application Settings will let applications "
                          "choose the AA mode.");
            ctk_help_para(b, &i, "Override Application Settings will override "
                          "all OpenGL applications to use the mode selected by "
                          "the slider.");
            ctk_help_para(b, &i, "Enhance Application Settings will make "
                          "applications that are requesting some type of "
                          "antialiasing mode use the mode selected by the "
                          "slider.");
        } else {
            ctk_help_para(b, &i, __aa_override_app_help);
        }

        if (ctk_multisample->active_attributes & __FSAA_NONE) {
            ctk_help_term(b, &i, "Off");
            ctk_help_para(b, &i, "Disables antialiasing in OpenGL "
                          "applications.  "
                          "Select this option if you require maximum "
                          "performance in your applications.");
        }
        
        if (ctk_multisample->active_attributes & __FSAA_2x) {
            ctk_help_term(b, &i, "2x (2xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 2x (2xMS)"
                          "Bilinear mode.  This mode offers improved image "
                          "quality and high performance in OpenGL "
                          "applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_2x_5t) {
            ctk_help_term(b, &i, "2x Quincunx");
            ctk_help_para(b, &i, "This enables the patented Quincunx "
                          "Antialiasing technique available in the GeForce "
                          "GPU family.  "
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
                          "Supersampling mode.  This mode offers higher image "
                          "quality at the expense of some performance in "
                          "OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_4x) {
            ctk_help_term(b, &i, "4x (4xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 4x (4xMS)"
                          "Bilinear mode.  This mode offers higher image "
                          "quality at the expense of some performance in "
                          "OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_4x_9t) {
            ctk_help_term(b, &i, "4x, 9-tap Gaussian");
            ctk_help_para(b, &i, "This enables antialiasing using the 4x, "
                          "9-tap (Gaussian) mode.  This mode offers higher "
                          "image quality but at the expense of some "
                          "performance in OpenGL applications.");
        }

        if (ctk_multisample->active_attributes & __FSAA_8x) {
            ctk_help_term(b, &i, "8x (4xMS, 4xCS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 8x "
                          "(4xMS, 4xCS) mode.  This mode offers better image "
                          "quality than the 4x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_8xS) {
            ctk_help_term(b, &i, "8x (4xSS, 2xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 8x "
                          "(4xSS, 2xMS) mode.  This mode offers better image "
                          "quality than the 4x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_16x) {
            ctk_help_term(b, &i, "16x (4xMS, 12xCS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 16x "
                          "(4xMS, 12xCS) mode.  This mode offers better image "
                          "quality than the 8x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_8xQ) {
            ctk_help_term(b, &i, "8x (8xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 8x (8xMS) "
                          "mode.  This mode offers better image "
                          "quality than the 8x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_16xS) {
            ctk_help_term(b, &i, "16x (4xSS, 4xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 16x "
                          "(4xSS, 4xMS) mode.  This mode offers better image "
                          "quality than the 16x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_16xQ) {
            ctk_help_term(b, &i, "16x (8xMS, 8xCS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 16x "
                          "(8xMS, 8xCS) mode.  This mode offers better image "
                          "quality than the 16x mode.");
        }

        if (ctk_multisample->active_attributes & __FSAA_32xS) {
            ctk_help_term(b, &i, "32x (4xSS, 8xMS)");
            ctk_help_para(b, &i, "This enables antialiasing using the 32x "
                          "(4xSS, 8xMS) mode.  This mode offers better image "
                          "quality than the 16x mode.");
        }
    }

    if (ctk_multisample->active_attributes & __FXAA) {
        ctk_help_term(b, &i, "Enable FXAA");
        ctk_help_para(b, &i, __fxaa_enable_help);
    }

    if (ctk_multisample->active_attributes & __LOG_ANISO_RANGE) {
        ctk_help_heading(b, &i, "Anisotropic Filtering");
        
        ctk_help_para(b, &i, "Anisotropic filtering is a technique used to "
                      "improve the quality of textures applied to the "
                      "surfaces of 3D objects when drawn at a sharp angle.  "
                      "Use the Anisotropic filtering slider to set the degree "
                      "of anisotropic filtering for improved image quality.  "
                      "Enabling this option improves image quality at the "
                      "expense of some performance.");

        ctk_help_para(b, &i, "You can also configure Anisotropic filtering "
                      "using the __GL_LOG_MAX_ANISO environment variable "
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
