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

#include "opengl_banner.h"

#include "ctkopengl.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

static void vblank_sync_button_toggled   (GtkWidget *, gpointer);

static void allow_flipping_button_toggled(GtkWidget *, gpointer);

static void force_stereo_button_toggled (GtkWidget *, gpointer);

static void xinerama_stereo_button_toggled (GtkWidget *, gpointer);

static void force_generic_cpu_toggled    (GtkWidget *, gpointer);

static void aa_line_gamma_toggled        (GtkWidget *, gpointer);

static void show_sli_hud_button_toggled (GtkWidget *, gpointer);

static void value_changed                (GtkObject *, gpointer, gpointer);

static const gchar *get_image_settings_string(gint val);

static gchar *format_image_settings_value(GtkScale *scale, gdouble arg1,
                                         gpointer user_data);

static void post_image_settings_value_changed(CtkOpenGL *ctk_opengl, gint val);

static void image_settings_value_changed(GtkRange *range, gpointer user_data);

static void image_settings_update_received(GtkObject *object,
                                           gpointer arg1, gpointer user_data);

#define FRAME_PADDING 5

static const char *__sync_to_vblank_help =
"When enabled, OpenGL applications will swap "
"buffers during the vertical retrace; this option is "
"applied to OpenGL applications that are started after "
"this option is set.";

static const char *__force_generic_cpu_help =
"Enable this option to disable use of CPU "
"specific features such as MMX, SSE, or 3DNOW!.  "
"Use of this option may result in performance "
"loss, but may be useful in conjunction with "
"software such as the Valgrind memory "
"debugger.  This option is applied to "
"OpenGL applications that are started after "
"this option is set.";

static const char *__image_settings_slider_help =
"The Image Settings slider controls the image quality setting.";

static const char *__force_stereo_help =
"Enabling this option causes OpenGL to force "
"stereo flipping even if a stereo drawable is "
"not visible. This option is applied "
"immediately.";

static const char *__xinerama_stereo_help =
 "Enabling this option causes OpenGL to allow "
"stereo flipping on multiple X screens configured "
"with Xinerama.  This option is applied immediately.";   

static const char *__show_sli_hud_help =
"Enabling this option causes OpenGL to draw "
"information about the current SLI mode on the "
"screen.  This option is applied to OpenGL "
"applications that are started after this option is "
"set.";

#define __SYNC_TO_VBLANK    (1 << 1)
#define __ALLOW_FLIPPING    (1 << 2)
#define __AA_LINE_GAMMA     (1 << 3)
#define __FORCE_GENERIC_CPU (1 << 4)
#define __FORCE_STEREO      (1 << 5)
#define __IMAGE_SETTINGS    (1 << 6)
#define __XINERAMA_STEREO   (1 << 7)
#define __SHOW_SLI_HUD      (1 << 8)



GType ctk_opengl_get_type(
    void
)
{
    static GType ctk_opengl_type = 0;

    if (!ctk_opengl_type) {
        static const GTypeInfo ctk_opengl_info = {
            sizeof (CtkOpenGLClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkOpenGL),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_opengl_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkOpenGL", &ctk_opengl_info, 0);
    }

    return ctk_opengl_type;
}


GtkWidget* ctk_opengl_new(NvCtrlAttributeHandle *handle,
                          CtkConfig *ctk_config, CtkEvent *ctk_event)
{
    GObject *object;
    CtkOpenGL *ctk_opengl;
    GtkWidget *label;
    GtkWidget *image;
    GtkWidget *frame;
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *check_button;
    GtkWidget *scale;
    gint force_generic_cpu_value, aa_line_gamma_value, val;
    ReturnStatus ret;

    NVCTRLAttributeValidValuesRec valid;

    guint8 *image_buffer = NULL;
    const nv_image_t *img;    

    object = g_object_new(CTK_TYPE_OPENGL, NULL);

    ctk_opengl = CTK_OPENGL(object);
    ctk_opengl->handle = handle;
    ctk_opengl->ctk_config = ctk_config;
    ctk_opengl->active_attributes = 0;

    gtk_box_set_spacing(GTK_BOX(object), 10);


    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    
    img = &opengl_banner_image;

    image_buffer = decompress_image_data(img);

    image = gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  FALSE, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));

    gtk_container_add(GTK_CONTAINER(frame), image);


    /*
     * Performance section: TOP->MIDDLE - LEFT->CENTER
     *
     * These settings directly influence OpenGLBox performance on the system
     *(related: multisample settings).
     */
    
    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Performance");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);


    vbox = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);


    /*
     * Sync to VBlank toggle: TOP->MIDDLE - LEFT->CENTER
     *
     * This toggle button specifies if OpenGLBox should sync to the vertical
     * retrace.
     */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_SYNC_TO_VBLANK, &val);
    if (ret == NvCtrlSuccess) {

        label = gtk_label_new("Sync to VBlank");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(vblank_sync_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_SYNC_TO_VBLANK),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);
 
        ctk_config_set_tooltip(ctk_config, check_button,
                               __sync_to_vblank_help);
    
        ctk_opengl->active_attributes |= __SYNC_TO_VBLANK;

        ctk_opengl->sync_to_vblank_button = check_button;
    }
    
    /*
     * allow flipping
     */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_FLIPPING_ALLOWED, &val);
    if (ret == NvCtrlSuccess) {

        label = gtk_label_new("Allow Flipping");
        
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(allow_flipping_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_FLIPPING_ALLOWED),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button,
                               "Enabling this option allows OpenGL to swap "
                               "by flipping when possible.  This option is "
                               "applied immediately.");
        
        ctk_opengl->active_attributes |= __ALLOW_FLIPPING;
        
        ctk_opengl->allow_flipping_button = check_button;
    }
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_FORCE_STEREO, &val);
    if (ret == NvCtrlSuccess) {

        label = gtk_label_new("Force Stereo Flipping");
    
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
    
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(force_stereo_button_toggled),
                     (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FORCE_STEREO),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button, __force_stereo_help);
    
        ctk_opengl->active_attributes |= __FORCE_STEREO;
    
        ctk_opengl->force_stereo_button = check_button;
    }
        
    ret = NvCtrlGetAttribute(handle, NV_CTRL_XINERAMA_STEREO, &val);
    if (ret == NvCtrlSuccess) {

        label = gtk_label_new("Allow Xinerama Stereo Flipping");
 
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
 
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(xinerama_stereo_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_FORCE_STEREO),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button, __xinerama_stereo_help);
 
        ctk_opengl->active_attributes |= __XINERAMA_STEREO;
 
        ctk_opengl->xinerama_stereo_button = check_button;
    }

    /*
     * Image Quality settings.
     */

    ret = NvCtrlGetValidAttributeValues(handle, NV_CTRL_IMAGE_SETTINGS, &valid);

    if ((ret == NvCtrlSuccess) && (valid.type == ATTRIBUTE_TYPE_RANGE) &&
        (NvCtrlGetAttribute(handle, NV_CTRL_IMAGE_SETTINGS, &val) == NvCtrlSuccess)) {

        frame = gtk_frame_new("Image Settings");
        gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 3);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), hbox);

        scale = gtk_hscale_new_with_range(valid.u.range.min, valid.u.range.max, 1);
        gtk_range_set_value(GTK_RANGE(scale), val);

        gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);

        gtk_container_add(GTK_CONTAINER(hbox), scale);

        g_signal_connect(G_OBJECT(scale), "format-value",
                         G_CALLBACK(format_image_settings_value),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(scale), "value-changed",
                         G_CALLBACK(image_settings_value_changed),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_IMAGE_SETTINGS),
                         G_CALLBACK(image_settings_update_received),
                         (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, scale, __image_settings_slider_help);

        ctk_opengl->active_attributes |= __IMAGE_SETTINGS;
        ctk_opengl->image_settings_scale = scale;
    }

    /*
     * Miscellaneous section:
     */

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Miscellaneous");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 0);

    vbox = gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(object), vbox, FALSE, FALSE, 0);


    /*
     * NV_CTRL_OPENGL_AA_LINE_GAMMA
     */

    ret = NvCtrlGetAttribute(ctk_opengl->handle,
                             NV_CTRL_OPENGL_AA_LINE_GAMMA,
                             &aa_line_gamma_value);

    if (ret == NvCtrlSuccess) {
        label = gtk_label_new("Enable gamma correction for antialiased lines");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     aa_line_gamma_value ==
                                     NV_CTRL_OPENGL_AA_LINE_GAMMA_ENABLE);
        
        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(aa_line_gamma_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_OPENGL_AA_LINE_GAMMA),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);
        
        ctk_config_set_tooltip(ctk_opengl->ctk_config,
                               check_button, "Allow Gamma-corrected "
                               "antialiased lines to consider variances in "
                               "the color display capabilities of output "
                               "devices when rendering smooth lines");

        ctk_opengl->active_attributes |= __AA_LINE_GAMMA;

        ctk_opengl->aa_line_gamma_button = check_button;
    }
    
    /*
     * Force Generic CPU
     */

    ret = NvCtrlGetAttribute(ctk_opengl->handle,
                             NV_CTRL_FORCE_GENERIC_CPU,
                             &force_generic_cpu_value);
    
    if (ret == NvCtrlSuccess) {
        label = gtk_label_new("Disable use of enhanced CPU instruction sets");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     force_generic_cpu_value ==
                                     NV_CTRL_FORCE_GENERIC_CPU_ENABLE);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(force_generic_cpu_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_FORCE_GENERIC_CPU),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);
        
        ctk_config_set_tooltip(ctk_config, check_button,
                               __force_generic_cpu_help);

        ctk_opengl->active_attributes |= __FORCE_GENERIC_CPU;

        ctk_opengl->force_generic_cpu_button = check_button;
    }

    ret = NvCtrlGetAttribute(handle, NV_CTRL_SHOW_SLI_HUD, &val);
    if (ret == NvCtrlSuccess) {

        label = gtk_label_new("Enable SLI Heads-Up-Display");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), val);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(show_sli_hud_button_toggled),
                     (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_SHOW_SLI_HUD),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button, __show_sli_hud_help);

        ctk_opengl->active_attributes |= __SHOW_SLI_HUD;

        ctk_opengl->show_sli_hud_button = check_button;
    }


    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
}

static void vblank_sync_button_toggled(
    GtkWidget *widget,
    gpointer user_data
)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_SYNC_TO_VBLANK, enabled);

    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Sync to VBlank %s.",
                                 enabled ? "enabled" : "disabled");
}


static void allow_flipping_button_toggled(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_FLIPPING_ALLOWED, enabled);
    
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Flipping %s.",
                                 enabled ? "allowed" : "prohibited");
    
}

static void force_stereo_button_toggled(GtkWidget *widget,
                                         gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_FORCE_STEREO, enabled);
    
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Stereo Flipping %s.",
                                 enabled ? "forced" : "not forced");
}

static void show_sli_hud_button_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;

    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_SHOW_SLI_HUD, enabled);

    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL SLI HUD %s.",
                                 enabled ? "enabled" : "disabled");
}

static void xinerama_stereo_button_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_XINERAMA_STEREO, enabled);
    
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Xinerama Stereo Flipping %s.",
                                 enabled ? "allowed" : "not allowed");
}

static void force_generic_cpu_toggled(
    GtkWidget *widget,
    gpointer user_data
)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    ReturnStatus ret;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    ret = NvCtrlSetAttribute(ctk_opengl->handle,
                             NV_CTRL_FORCE_GENERIC_CPU, enabled);

    if (ret != NvCtrlSuccess) return;
    
    /*
     * XXX the logic is awkward, but correct: when
     * NV_CTRL_FORCE_GENERIC_CPU is enabled, use of enhanced CPU
     * instructions is disabled, and vice versa.
     */

    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL use of enhanced CPU instructions %s.",
                                 enabled ? "disabled" : "enabled");
}

static void aa_line_gamma_toggled(
    GtkWidget *widget,
    gpointer user_data
)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    ReturnStatus ret;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    ret = NvCtrlSetAttribute(ctk_opengl->handle,
                             NV_CTRL_OPENGL_AA_LINE_GAMMA, enabled);

    if (ret != NvCtrlSuccess) return;

    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL gamma correction for antialiased "
                                 "lines %s.",
                                 enabled ? "enabled" : "disabled");
}



/*
 * value_changed() - callback function for changed OpenGL settings.
 */

static void value_changed(GtkObject *object, gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct;
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    GtkToggleButton *button;
    GCallback func;

    event_struct = (CtkEventStruct *) arg1;
    ctk_opengl = CTK_OPENGL(user_data);

    switch (event_struct->attribute) {
    case NV_CTRL_SYNC_TO_VBLANK:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->sync_to_vblank_button);
        func = G_CALLBACK(vblank_sync_button_toggled);
        break;
    case NV_CTRL_FLIPPING_ALLOWED:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->allow_flipping_button);
        func = G_CALLBACK(allow_flipping_button_toggled);
        break;
    case NV_CTRL_FORCE_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->force_stereo_button);
        func = G_CALLBACK(force_stereo_button_toggled);
        break;
    case NV_CTRL_XINERAMA_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->xinerama_stereo_button);
        func = G_CALLBACK(xinerama_stereo_button_toggled);
        break;
    case NV_CTRL_OPENGL_AA_LINE_GAMMA:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->aa_line_gamma_button);
        func = G_CALLBACK(aa_line_gamma_toggled);
        break;
    case NV_CTRL_FORCE_GENERIC_CPU:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->force_generic_cpu_button);
        func = G_CALLBACK(force_generic_cpu_toggled);
        break;
    case NV_CTRL_SHOW_SLI_HUD:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->show_sli_hud_button);
        func = G_CALLBACK(show_sli_hud_button_toggled);
        break;
    default:
        return;
    }
    
    enabled = gtk_toggle_button_get_active(button);

    if (enabled != event_struct->value) {
        
        g_signal_handlers_block_by_func(button, func, ctk_opengl);
        gtk_toggle_button_set_active(button, event_struct->value);
        g_signal_handlers_unblock_by_func(button, func, ctk_opengl);
    }
    
} /* value_changed() */


/*
 * get_image_settings_string() - translate the NV-CONTROL image settings value
 * to a more comprehensible string.
 */

static const gchar *get_image_settings_string(gint val)
{
    static const gchar *image_settings_strings[] = {
        "High Quality", "Quality", "Performance", "High Performance"
    };

    if ((val < NV_CTRL_IMAGE_SETTINGS_HIGH_QUALITY) ||
        (val > NV_CTRL_IMAGE_SETTINGS_HIGH_PERFORMANCE)) return "Unknown";

    return image_settings_strings[val];

} /* get_image_settings_string() */

/*
 * format_image_settings_value() - callback for the "format-value" signal
 * from the image settings scale.
 */

static gchar *format_image_settings_value(GtkScale *scale, gdouble arg1,
                                         gpointer user_data)
{
    return g_strdup(get_image_settings_string(arg1));

} /* format_image_settings_value() */

/*
 * post_image_settings_value_changed() - helper function for
 * image_settings_value_changed(); this does whatever work is necessary
 * after the image settings value has changed.
 */

static void post_image_settings_value_changed(CtkOpenGL *ctk_opengl, gint val)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "Image Settings set to %s.",
                                 get_image_settings_string(val));

} /* post_image_settings_value_changed() */

/*
 * image_settings_value_changed() - callback for the "value-changed" signal
 * from the image settings scale.
 */

static void image_settings_value_changed(GtkRange *range, gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    gint val = gtk_range_get_value(range);

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_IMAGE_SETTINGS, val);
    post_image_settings_value_changed(ctk_opengl, val);

} /* image_settings_value_changed() */

/*
 * image_settings_update_received() - this function is called when the
 * NV_CTRL_IMAGE_SETTINGS atribute is changed by another NV-CONTROL client.
 */

static void image_settings_update_received(GtkObject *object,
                                          gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    GtkRange *range = GTK_RANGE(ctk_opengl->image_settings_scale);

    g_signal_handlers_block_by_func(G_OBJECT(range),
                                    G_CALLBACK(image_settings_value_changed),
                                    (gpointer) ctk_opengl);

    gtk_range_set_value(range, event_struct->value);
    post_image_settings_value_changed(ctk_opengl, event_struct->value);

    g_signal_handlers_unblock_by_func(G_OBJECT(range),
                                      G_CALLBACK(image_settings_value_changed),
                                      (gpointer) ctk_opengl);

} /* image_settings_update_received() */


GtkTextBuffer *ctk_opengl_create_help(GtkTextTagTable *table,
                                      CtkOpenGL *ctk_opengl)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "OpenGL Help");

    if (ctk_opengl->active_attributes & __SYNC_TO_VBLANK) {
        ctk_help_heading(b, &i, "Sync to VBlank");
        ctk_help_para(b, &i, __sync_to_vblank_help);
    }

    if (ctk_opengl->active_attributes & __ALLOW_FLIPPING) {
        ctk_help_heading(b, &i, "Allow Flipping");
        ctk_help_para(b, &i, "Enabling this option allows OpenGL to swap "
                      "by flipping when possible.  Flipping is a mechanism "
                      "of performing swaps where the OpenGL driver changes "
                      "which buffer is scanned out by the DAC.  The "
                      "alternative swapping mechanism is blitting, where "
                      "buffer contents are copied from the back buffer to "
                      "the front buffer.  It is usually faster to flip than "
                      "it is to blit.");

        ctk_help_para(b, &i, "Note that this option is applied immediately, "
                      "unlike most other OpenGL options which are only "
                      "applied to OpenGL applications that are started "
                      "after the option is set.");
    }

    if (ctk_opengl->active_attributes & __FORCE_STEREO) {
        ctk_help_heading(b, &i, "Force Stereo Flipping");
        ctk_help_para(b, &i, __force_stereo_help);
    }
    
    if (ctk_opengl->active_attributes & __XINERAMA_STEREO) {
        ctk_help_heading(b, &i, "Allow Xinerama Stereo Flipping");
        ctk_help_para(b, &i, __xinerama_stereo_help);
    }
    
    if (ctk_opengl->active_attributes & __IMAGE_SETTINGS) {
        ctk_help_heading(b, &i, "Image Settings");
        ctk_help_para(b, &i, "This setting gives you full control over the "
                      "image quality in your applications.");
        ctk_help_para(b, &i, "Several quality settings are available for "
                      "you to choose from with the Image Settings slider.  "
                      "Note that choosing higher image quality settings may "
                      "result in decreased performance.");

        ctk_help_term(b, &i, "High Quality");
        ctk_help_para(b, &i, "This setting results in the best image quality "
                      "for your applications.  It is not necessary for "
                      "average users who run game applications, and designed "
                      "for more advanced users to generate images that do not "
                      "take advantage of the programming capability of the "
                      "texture filtering hardware.");

        ctk_help_term(b, &i, "Quality");
        ctk_help_para(b, &i, "This is the default setting that results in "
                      "optimal image quality for your applications.");

        ctk_help_term(b, &i, "Performance");
        ctk_help_para(b, &i, "This setting offers an optimal blend of image "
                      "quality and performance.  The result is optimal "
                      "performance and good image quality for your "
                      "applications.");

        ctk_help_term(b, &i, "High Performance");
        ctk_help_para(b, &i, "This setting offers the highest frame rate "
                      "possible, resulting in the best performance for your "
                      "applications.");
    }

    if (ctk_opengl->active_attributes & __AA_LINE_GAMMA) {
        ctk_help_heading(b, &i, "Enable gamma correction for "
                         "antialiased lines");
        ctk_help_para(b, &i, "Enabling this option allows Gamma-corrected "
                      "antialiased lines to consider variances in the color "
                      "display capabilities of output devices when rendering "
                      "smooth lines.  This option is available only on "
                      "Quadro FX or newer NVIDIA GPUs.  This option is "
                      "applied to OpenGL applications that are started after "
                      "this option is set.");
    }

    if (ctk_opengl->active_attributes & __FORCE_GENERIC_CPU) {
        ctk_help_heading(b, &i, "Disable use of enhanced CPU "
                         "instruction sets");
        ctk_help_para(b, &i, __force_generic_cpu_help);
    }
    
    if (ctk_opengl->active_attributes & __SHOW_SLI_HUD) {
        ctk_help_heading(b, &i, "SLI Heads-Up-Display");
        ctk_help_para(b, &i, "This option draws information about the current "
                      "SLI mode on top of OpenGL windows.  Its behavior "
                      "depends on which SLI mode is in use:");
        ctk_help_term(b, &i, "Alternate Frame Rendering");
        ctk_help_para(b, &i, "In AFR mode, a vertical green bar displays the "
                      "amount of scaling currently being achieved.  A longer "
                      "bar indicates more scaling.");
        ctk_help_term(b, &i, "Split-Frame Rendering");
        ctk_help_para(b, &i, "In this mode, OpenGL draws a horizontal green "
                      "line showing where the screen is split.  Everything "
                      "above the line is drawn on one GPU and everything "
                      "below is drawn on the other.");
        ctk_help_term(b, &i, "SLI Antialiasing");
        ctk_help_para(b, &i, "In this mode, OpenGL draws a horizontal green "
                      "line one third of the way across the screen.  Above "
                      "this line, the images from both GPUs are blended to "
                      "produce the currently selected SLIAA mode.  Below the "
                      "line, the image from just one GPU is displayed without "
                      "blending.  This allows easy comparison between the "
                      "SLIAA and single-GPU AA modes.");
    }

    ctk_help_finish(b);

    return b;
}
