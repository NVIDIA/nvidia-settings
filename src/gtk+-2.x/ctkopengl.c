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
#include <NvCtrlAttributes.h>

#include "ctkbanner.h"

#include "ctkopengl.h"
#include "ctkscale.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

static void vblank_sync_button_toggled   (GtkWidget *, gpointer);

static void post_vblank_sync_button_toggled(CtkOpenGL *, gboolean);

static void post_allow_flipping_button_toggled(CtkOpenGL *, gboolean);

static void post_force_stereo_button_toggled(CtkOpenGL *, gboolean);

static void post_show_sli_visual_indicator_button_toggled(CtkOpenGL *,
                                                          gboolean);

static void post_show_multigpu_visual_indicator_button_toggled(CtkOpenGL *,
                                                               gboolean);

static void post_xinerama_stereo_button_toggled(CtkOpenGL *, gboolean);

static void post_stereo_eyes_exchange_button_toggled(CtkOpenGL *, gboolean);

static void post_aa_line_gamma_toggled(CtkOpenGL *, gboolean);

static void post_use_conformant_clamping_button_toggled(CtkOpenGL *, gint);

static void allow_flipping_button_toggled(GtkWidget *, gpointer);

static void force_stereo_button_toggled (GtkWidget *, gpointer);

static void xinerama_stereo_button_toggled (GtkWidget *, gpointer);

static void stereo_eyes_exchange_button_toggled (GtkWidget *, gpointer);

static void aa_line_gamma_toggled        (GtkWidget *, gpointer);

static void use_conformant_clamping_button_toggled(GtkWidget *, gpointer);

static void show_sli_visual_indicator_button_toggled (GtkWidget *, gpointer);

static void show_multigpu_visual_indicator_button_toggled (GtkWidget *, gpointer);

static void value_changed                (GtkObject *, gpointer, gpointer);

static const gchar *get_image_settings_string(gint val);

static gchar *format_image_settings_value(GtkScale *scale, gdouble arg1,
                                         gpointer user_data);

static void post_slider_value_changed(CtkOpenGL *ctk_opengl, gint val);

static void aa_line_gamma_update_received(GtkObject *object,
                                         gpointer arg1, gpointer user_data);

static void post_image_settings_value_changed(CtkOpenGL *ctk_opengl, gint val);

static void image_settings_value_changed(GtkRange *range, gpointer user_data);

static void image_settings_update_received(GtkObject *object,
                                           gpointer arg1, gpointer user_data);

static GtkWidget *create_slider(CtkOpenGL *ctk_opengl,
                                GtkWidget *vbox,
                                const gchar *name,
                                const char *help,
                                gint attribute,
                                unsigned int bit);

static void slider_changed(GtkAdjustment *adjustment, gpointer user_data);

#define FRAME_PADDING 5

static const char *__sync_to_vblank_help =
"When enabled, OpenGL applications will swap "
"buffers during the vertical retrace; this option is "
"applied to OpenGL applications that are started after "
"this option is set.";

static const char *__aa_line_gamma_checkbox_help =
"Enable the antialiased lines gamma correction checkbox to make the "
"gamma correction slider active.";

static const char *__aa_line_gamma_slider_help =
"This option allows Gamma-corrected "
"antialiased lines to consider variances in the color "
"display capabilities of output devices when rendering "
"smooth lines.  This option is applied to OpenGL applications "
"that are started after this option is set.";

static const char *__image_settings_slider_help =
"The Image Settings slider controls the image quality setting.";

static const char *__force_stereo_help =
"Enabling this option causes OpenGL to force "
"stereo flipping even if a stereo drawable is "
"not visible.  This option is applied "
"immediately.";

static const char *__xinerama_stereo_help =
 "Enabling this option causes OpenGL to allow "
"stereo flipping on multiple X screens configured "
"with Xinerama.  This option is applied immediately.";

static const char *__show_sli_visual_indicator_help =
"Enabling this option causes OpenGL to draw "
"information about the current SLI mode on the "
"screen.  This option is applied to OpenGL "
"applications that are started after this option is "
"set.";

static const char *__show_multigpu_visual_indicator_help =
"Enabling this option causes OpenGL to draw "
"information about the current Multi-GPU mode on the "
"screen.  This option is applied to OpenGL "
"applications that are started after this option is "
"set.";

static const char *__stereo_eyes_exchange_help =
"Enabling this option causes OpenGL to draw the left "
"eye image in the right eye and vice versa for stereo "
"drawables.  This option is applied immediately.";

static const char *__use_conformant_clamping_help =
"Disabling this option causes OpenGL to replace GL_CLAMP with "
"GL_CLAMP_TO_EDGE for borderless 2D textures.  This eliminates "
"seams at the edges of textures in some older games such as "
"Quake 3.";

#define __SYNC_TO_VBLANK      (1 << 1)
#define __ALLOW_FLIPPING      (1 << 2)
#define __AA_LINE_GAMMA_VALUE (1 << 3)
#define __AA_LINE_GAMMA       (1 << 4)
#define __FORCE_GENERIC_CPU   (1 << 5)
#define __FORCE_STEREO        (1 << 6)
#define __IMAGE_SETTINGS      (1 << 7)
#define __XINERAMA_STEREO     (1 << 8)
#define __SHOW_SLI_VISUAL_INDICATOR        (1 << 9)
#define __STEREO_EYES_EXCHANGE (1 << 10)
#define __SHOW_MULTIGPU_VISUAL_INDICATOR    (1 << 11)
#define __CONFORMANT_CLAMPING (1 << 12)



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
            NULL  /* value_table */
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
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *check_button;
    GtkWidget *scale;
    GtkObject *adjustment;

    gint sync_to_vblank;
    gint flipping_allowed;
    gint force_stereo;
    gint xinerama_stereo;
    gint stereo_eyes_exchange;
    NVCTRLAttributeValidValuesRec image_settings_valid;
    gint image_settings_value;
    gint aa_line_gamma;
    gint use_conformant_clamping;
    gint show_sli_visual_indicator;
    gint show_multigpu_visual_indicator;

    ReturnStatus ret_sync_to_vblank;
    ReturnStatus ret_flipping_allowed;
    ReturnStatus ret_force_stereo;
    ReturnStatus ret_xinerama_stereo;
    ReturnStatus ret_stereo_eyes_exchange;
    ReturnStatus ret_image_settings;
    ReturnStatus ret_aa_line_gamma;
    ReturnStatus ret_use_conformant_clamping;
    ReturnStatus ret_show_sli_visual_indicator;
    ReturnStatus ret_show_multigpu_visual_indicator;

    /* Query OpenGL settings */

    ret_sync_to_vblank =
        NvCtrlGetAttribute(handle, NV_CTRL_SYNC_TO_VBLANK, &sync_to_vblank);

    ret_flipping_allowed =
        NvCtrlGetAttribute(handle, NV_CTRL_FLIPPING_ALLOWED,
                           &flipping_allowed);

    ret_force_stereo =
        NvCtrlGetAttribute(handle, NV_CTRL_FORCE_STEREO, &force_stereo);

    ret_xinerama_stereo =
        NvCtrlGetAttribute(handle, NV_CTRL_XINERAMA_STEREO, &xinerama_stereo);

    ret_stereo_eyes_exchange =
        NvCtrlGetAttribute(handle, NV_CTRL_STEREO_EYES_EXCHANGE,
                           &stereo_eyes_exchange);

    ret_image_settings =
        NvCtrlGetValidAttributeValues(handle, NV_CTRL_IMAGE_SETTINGS,
                                      &image_settings_valid);
    if ((ret_image_settings == NvCtrlSuccess) &&
        (image_settings_valid.type == ATTRIBUTE_TYPE_RANGE)) {
        ret_image_settings =
            NvCtrlGetAttribute(handle, NV_CTRL_IMAGE_SETTINGS,
                               &image_settings_value);
    } else {
        ret_image_settings = NvCtrlError;
    }

    ret_aa_line_gamma = NvCtrlGetAttribute(handle, NV_CTRL_OPENGL_AA_LINE_GAMMA,
                                           &aa_line_gamma);

    ret_use_conformant_clamping =
        NvCtrlGetAttribute(handle, NV_CTRL_TEXTURE_CLAMPING,
                           &use_conformant_clamping);

    ret_show_sli_visual_indicator = NvCtrlGetAttribute(handle,
                                          NV_CTRL_SHOW_SLI_VISUAL_INDICATOR,
                                          &show_sli_visual_indicator);
    ret_show_multigpu_visual_indicator = NvCtrlGetAttribute(handle,
                                          NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR,
                                          &show_multigpu_visual_indicator);

    /* There are no OpenGL settings to change (OpenGL disabled?) */
    if ((ret_sync_to_vblank != NvCtrlSuccess) &&
        (ret_flipping_allowed != NvCtrlSuccess) &&
        (ret_force_stereo != NvCtrlSuccess) &&
        (ret_xinerama_stereo != NvCtrlSuccess) &&
        (ret_stereo_eyes_exchange != NvCtrlSuccess) &&
        (ret_image_settings != NvCtrlSuccess) &&
        (ret_aa_line_gamma != NvCtrlSuccess) &&
        (ret_use_conformant_clamping != NvCtrlSuccess) &&
        (ret_show_sli_visual_indicator != NvCtrlSuccess) &&
        (ret_show_multigpu_visual_indicator != NvCtrlSuccess)) {
        return NULL;
    }

    object = g_object_new(CTK_TYPE_OPENGL, NULL);

    ctk_opengl = CTK_OPENGL(object);
    ctk_opengl->handle = handle;
    ctk_opengl->ctk_config = ctk_config;
    ctk_opengl->active_attributes = 0;

    gtk_box_set_spacing(GTK_BOX(object), 10);

    banner = ctk_banner_image_new(BANNER_ARTWORK_OPENGL);
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);


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

    if (ret_sync_to_vblank == NvCtrlSuccess) {

        label = gtk_label_new("Sync to VBlank");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     sync_to_vblank);

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

    if (ret_flipping_allowed == NvCtrlSuccess) {

        label = gtk_label_new("Allow Flipping");
        
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     flipping_allowed);

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
    
    if (ret_force_stereo == NvCtrlSuccess) {

        label = gtk_label_new("Force Stereo Flipping");
    
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
    
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     force_stereo);

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
        
    if (ret_xinerama_stereo == NvCtrlSuccess) {

        label = gtk_label_new("Allow Xinerama Stereo Flipping");
 
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
 
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     xinerama_stereo);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(xinerama_stereo_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_XINERAMA_STEREO),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button, __xinerama_stereo_help);
 
        ctk_opengl->active_attributes |= __XINERAMA_STEREO;
 
        ctk_opengl->xinerama_stereo_button = check_button;
    }

    if (ret_stereo_eyes_exchange == NvCtrlSuccess) {

        label = gtk_label_new("Exchange Stereo Eyes");
    
        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);
    
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     stereo_eyes_exchange);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(stereo_eyes_exchange_button_toggled),
                     (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_STEREO_EYES_EXCHANGE),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button,
                               __stereo_eyes_exchange_help);
    
        ctk_opengl->active_attributes |= __STEREO_EYES_EXCHANGE;
    
        ctk_opengl->stereo_eyes_exchange_button = check_button;
    }

    /*
     * Image Quality settings.
     */

    if (ret_image_settings == NvCtrlSuccess) {

        frame = gtk_frame_new("Image Settings");
        gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 3);

        hbox = gtk_hbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
        gtk_container_add(GTK_CONTAINER(frame), hbox);

        /* create the slider */
        adjustment = gtk_adjustment_new(image_settings_value,
                                        image_settings_valid.u.range.min,
                                        image_settings_valid.u.range.max,
                                        1, 1, 0.0);
        scale = gtk_hscale_new(GTK_ADJUSTMENT(adjustment));
        gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment), image_settings_value);

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

    if (ret_aa_line_gamma == NvCtrlSuccess) {
        label = gtk_label_new("Enable gamma correction for antialiased lines");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     aa_line_gamma ==
                                     NV_CTRL_OPENGL_AA_LINE_GAMMA_ENABLE);
        
        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(aa_line_gamma_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_OPENGL_AA_LINE_GAMMA),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);
        
        ctk_config_set_tooltip(ctk_opengl->ctk_config,
                               check_button, __aa_line_gamma_checkbox_help);
        
        ctk_opengl->aa_line_gamma_button = check_button;
        ctk_opengl->active_attributes |= __AA_LINE_GAMMA;
        
        ctk_opengl->aa_line_gamma_scale =
            create_slider(ctk_opengl, vbox, "Gamma correction",
                          __aa_line_gamma_slider_help,
                          NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE,
                          __AA_LINE_GAMMA_VALUE);
        
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE),
                         G_CALLBACK(aa_line_gamma_update_received),
                         (gpointer) ctk_opengl);

        if (ctk_opengl->aa_line_gamma_scale)
            gtk_widget_set_sensitive(ctk_opengl->aa_line_gamma_scale,
                                     gtk_toggle_button_get_active
                                     (GTK_TOGGLE_BUTTON(check_button)));
    }

    /*
     * NV_CTRL_TEXTURE_CLAMPING
     */

    if (ret_use_conformant_clamping == NvCtrlSuccess) {
        label = gtk_label_new("Use Conformant Texture Clamping");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     use_conformant_clamping);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(use_conformant_clamping_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_TEXTURE_CLAMPING),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button,
                               __use_conformant_clamping_help);

        ctk_opengl->active_attributes |= __CONFORMANT_CLAMPING;

        ctk_opengl->use_conformant_clamping_button = check_button;
    }
    
    if (ret_show_sli_visual_indicator == NvCtrlSuccess) {

        label = gtk_label_new("Enable SLI Visual Indicator");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     show_sli_visual_indicator);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(show_sli_visual_indicator_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_SHOW_SLI_VISUAL_INDICATOR),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config,
                               check_button, __show_sli_visual_indicator_help);

        ctk_opengl->active_attributes |= __SHOW_SLI_VISUAL_INDICATOR;

        ctk_opengl->show_sli_visual_indicator_button = check_button;
    }

    if (ret_show_multigpu_visual_indicator == NvCtrlSuccess) {

        label = gtk_label_new("Enable Multi-GPU Visual Indicator");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     show_multigpu_visual_indicator);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(show_multigpu_visual_indicator_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button,
                               __show_multigpu_visual_indicator_help);

        ctk_opengl->active_attributes |= __SHOW_MULTIGPU_VISUAL_INDICATOR;

        ctk_opengl->show_multigpu_visual_indicator_button = check_button;
    }

    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
}

/* 
 * Prints status bar message
 */
static void post_vblank_sync_button_toggled(CtkOpenGL *ctk_opengl, 
                                            gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Sync to VBlank %s.",
                                 enabled ? "enabled" : "disabled");
}

static void post_allow_flipping_button_toggled(CtkOpenGL *ctk_opengl, 
                                                gboolean enabled) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Flipping %s.",
                                 enabled ? "allowed" : "not allowed");
}

static void post_force_stereo_button_toggled(CtkOpenGL *ctk_opengl, 
                                             gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Stereo Flipping %s.",
                                 enabled ? "forced" : "not forced");
}

static void post_show_sli_visual_indicator_button_toggled(CtkOpenGL *ctk_opengl, 
                                                          gboolean enabled) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL SLI Visual Indicator %s.",
                                 enabled ? "enabled" : "disabled");
}

static void
post_show_multigpu_visual_indicator_button_toggled(CtkOpenGL *ctk_opengl, 
                                                   gboolean enabled) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Multi-GPU Visual Indicator %s.",
                                 enabled ? "enabled" : "disabled");
}

static void post_xinerama_stereo_button_toggled(CtkOpenGL *ctk_opengl, 
                                                gboolean enabled) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Xinerama Stereo Flipping %s.",
                                 enabled ? "allowed" : "not allowed");
}

static void post_stereo_eyes_exchange_button_toggled(CtkOpenGL *ctk_opengl, 
                                                     gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Stereo Eyes Exchanged %s.",
                                 enabled ? "enabled" : "disabled");
}

static void post_aa_line_gamma_toggled(CtkOpenGL *ctk_opengl, 
                                       gboolean enabled) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL gamma correction for antialiased "
                                 "lines %s.",
                                 enabled ? "enabled" : "disabled");
}

static void
post_use_conformant_clamping_button_toggled(CtkOpenGL *ctk_opengl,
                                               int clamping) 
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "Use %sConformant OpenGL Texture Clamping",
                                 (clamping == NV_CTRL_TEXTURE_CLAMPING_SPEC) ?
                                 "" : "Non-");
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

    post_vblank_sync_button_toggled(ctk_opengl, enabled);
}


static void allow_flipping_button_toggled(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_FLIPPING_ALLOWED, enabled);
    post_allow_flipping_button_toggled(ctk_opengl, enabled);
    
}

static void force_stereo_button_toggled(GtkWidget *widget,
                                         gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_FORCE_STEREO, enabled);
    post_force_stereo_button_toggled(ctk_opengl, enabled);
}

static void show_sli_visual_indicator_button_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;

    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle,
                       NV_CTRL_SHOW_SLI_VISUAL_INDICATOR, enabled);
    post_show_sli_visual_indicator_button_toggled(ctk_opengl, enabled); 
}

static void show_multigpu_visual_indicator_button_toggled(GtkWidget *widget,
                                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;

    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle,
                       NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR, enabled);
    post_show_multigpu_visual_indicator_button_toggled(ctk_opengl, enabled);
}

static void xinerama_stereo_button_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_XINERAMA_STEREO, enabled);
    post_xinerama_stereo_button_toggled(ctk_opengl, enabled);
}

static void stereo_eyes_exchange_button_toggled(GtkWidget *widget,
                                                gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    
    ctk_opengl = CTK_OPENGL(user_data);

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_STEREO_EYES_EXCHANGE,
                       enabled);

    post_stereo_eyes_exchange_button_toggled(ctk_opengl, enabled);
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
    if (ctk_opengl->aa_line_gamma_scale)
        gtk_widget_set_sensitive(ctk_opengl->aa_line_gamma_scale, enabled);
    post_aa_line_gamma_toggled(ctk_opengl, enabled);
}

static void use_conformant_clamping_button_toggled(GtkWidget *widget,
                                                   gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    int clamping;
    
    ctk_opengl = CTK_OPENGL(user_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        clamping = NV_CTRL_TEXTURE_CLAMPING_SPEC;
    } else {
        clamping = NV_CTRL_TEXTURE_CLAMPING_EDGE;
    }
   
    NvCtrlSetAttribute(ctk_opengl->handle, NV_CTRL_TEXTURE_CLAMPING, clamping);

    post_use_conformant_clamping_button_toggled(ctk_opengl, clamping);
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
        post_vblank_sync_button_toggled(ctk_opengl, event_struct->value);
        break;
    case NV_CTRL_FLIPPING_ALLOWED:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->allow_flipping_button);
        func = G_CALLBACK(allow_flipping_button_toggled);
        post_allow_flipping_button_toggled(ctk_opengl, event_struct->value);
        break;
    case NV_CTRL_FORCE_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->force_stereo_button);
        func = G_CALLBACK(force_stereo_button_toggled);
        post_force_stereo_button_toggled(ctk_opengl, event_struct->value);
        break;
    case NV_CTRL_XINERAMA_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->xinerama_stereo_button);
        func = G_CALLBACK(xinerama_stereo_button_toggled);
        post_xinerama_stereo_button_toggled(ctk_opengl, event_struct->value);
        break;
    case NV_CTRL_STEREO_EYES_EXCHANGE:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->stereo_eyes_exchange_button);
        func = G_CALLBACK(stereo_eyes_exchange_button_toggled);
        post_stereo_eyes_exchange_button_toggled(ctk_opengl,
                                                 event_struct->value);
        break;
    case NV_CTRL_OPENGL_AA_LINE_GAMMA:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->aa_line_gamma_button);
        func = G_CALLBACK(aa_line_gamma_toggled);
        post_aa_line_gamma_toggled(ctk_opengl, event_struct->value);
        gtk_widget_set_sensitive(ctk_opengl->aa_line_gamma_scale,
                                 event_struct->value);
        break;
    case NV_CTRL_TEXTURE_CLAMPING:
        button =
            GTK_TOGGLE_BUTTON(ctk_opengl->use_conformant_clamping_button);
        func = G_CALLBACK(use_conformant_clamping_button_toggled);
        post_use_conformant_clamping_button_toggled(ctk_opengl,
                                                    event_struct->value);
        break;
    case NV_CTRL_SHOW_SLI_VISUAL_INDICATOR:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->show_sli_visual_indicator_button);
        func = G_CALLBACK(show_sli_visual_indicator_button_toggled);
        post_show_sli_visual_indicator_button_toggled(ctk_opengl,
                                                      event_struct->value);
        break;
    case NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR:
        button =
            GTK_TOGGLE_BUTTON(ctk_opengl->show_multigpu_visual_indicator_button);
        func = G_CALLBACK(show_multigpu_visual_indicator_button_toggled);
        post_show_multigpu_visual_indicator_button_toggled(ctk_opengl,
                                                           event_struct->value);
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


/*
 * post_slider_value_changed() - helper function for
 * aa_line_gamma_value_changed(); this does whatever work is necessary
 * after the aa line gamma value has changed.
 */

static void post_slider_value_changed(CtkOpenGL *ctk_opengl, gint val)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL anti-aliased lines edge smoothness "
                                 "changed to %d%%.",
                                 val);

} /* post_slider_value_changed() */


/*
 * slider_changed() -
 */

static void slider_changed(GtkAdjustment *adjustment, gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    gint attribute, value;
    user_data = g_object_get_data(G_OBJECT(adjustment), "opengl_attribute");
    attribute = GPOINTER_TO_INT(user_data);
    value = (gint) adjustment->value;
    NvCtrlSetAttribute(ctk_opengl->handle, attribute, value);
    post_slider_value_changed(ctk_opengl, value);
} /* slider_changed() */


/*
 * aa_line_gamma_update_received() - this function is called when the
 * NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE attribute is changed by another NV-CONTROL
 * client.
 */

static void aa_line_gamma_update_received(GtkObject *object,
                                         gpointer arg1, gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtkScale *scale = CTK_SCALE(ctk_opengl->aa_line_gamma_scale);
    GtkAdjustment *adjustment;
    adjustment = GTK_ADJUSTMENT(scale->gtk_adjustment);
    g_signal_handlers_block_by_func(G_OBJECT(adjustment),
                                    G_CALLBACK(slider_changed),
                                    (gpointer) ctk_opengl);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment),
                             (gint)event_struct->value);
    post_slider_value_changed(ctk_opengl, event_struct->value);

    g_signal_handlers_unblock_by_func(G_OBJECT(adjustment),
                                      G_CALLBACK(slider_changed),
                                      (gpointer) ctk_opengl);

} /* aa_line_gamma_update_received() */



static GtkWidget *create_slider(CtkOpenGL *ctk_opengl,
                                GtkWidget *vbox,
                                const gchar *name,
                                const char *help,
                                gint attribute,
                                unsigned int bit)
{
    GtkObject *adjustment;
    GtkWidget *scale, *widget;
    gint min, max, val, step_incr, page_incr;
    NVCTRLAttributeValidValuesRec range;
    ReturnStatus ret;
    /* get the attribute value */

    ret = NvCtrlGetAttribute(ctk_opengl->handle, attribute, &val);
    if (ret != NvCtrlSuccess) return NULL;
    /* get the range for the attribute */

    NvCtrlGetValidAttributeValues(ctk_opengl->handle, attribute, &range);

    if (range.type != ATTRIBUTE_TYPE_RANGE) return NULL;
    min = range.u.range.min;
    max = range.u.range.max;

    step_incr = ((max) - (min))/10;
    if (step_incr <= 0) step_incr = 1;
    page_incr = ((max) - (min))/25;
    if (page_incr <= 0) page_incr = 1;

    /* create the slider */
    adjustment = gtk_adjustment_new(val, min, max,
                                    step_incr, page_incr, 0.0);

    g_object_set_data(G_OBJECT(adjustment), "opengl_attribute",
                      GINT_TO_POINTER(attribute));

    g_signal_connect(G_OBJECT(adjustment), "value_changed",
                     G_CALLBACK(slider_changed),
                     (gpointer) ctk_opengl);

    scale = ctk_scale_new(GTK_ADJUSTMENT(adjustment), name,
                          ctk_opengl->ctk_config, G_TYPE_INT);

    gtk_box_pack_start(GTK_BOX(vbox), scale, TRUE, TRUE, 0);

    ctk_opengl->active_attributes |= bit;

    widget = CTK_SCALE(scale)->gtk_scale;
    ctk_config_set_tooltip(ctk_opengl->ctk_config, widget, help);

    return scale;
} /* create_slider() */


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
    
    if (ctk_opengl->active_attributes & __STEREO_EYES_EXCHANGE) {
        ctk_help_heading(b, &i, "Exchange Stereo Eyes");
        ctk_help_para(b, &i, __stereo_eyes_exchange_help);
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
        ctk_help_para(b, &i, __aa_line_gamma_checkbox_help );
    }

    if (ctk_opengl->active_attributes & __AA_LINE_GAMMA_VALUE) {
        ctk_help_heading(b, &i, "Set gamma correction for "
                         "antialiased lines");
        ctk_help_para(b, &i, __aa_line_gamma_slider_help);
    }

    if (ctk_opengl->active_attributes & __CONFORMANT_CLAMPING) {
        ctk_help_heading(b, &i, "Use Conformant Texture Clamping");
        ctk_help_para(b, &i, __use_conformant_clamping_help);
    }

    if (ctk_opengl->active_attributes & __SHOW_SLI_VISUAL_INDICATOR) {
        ctk_help_heading(b, &i, "SLI Visual Indicator");
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

    if (ctk_opengl->active_attributes & __SHOW_MULTIGPU_VISUAL_INDICATOR) {
        ctk_help_heading(b, &i, "Multi-GPU Visual Indicator");
        ctk_help_para(b, &i, "This option draws information about the current "
                      "Multi-GPU mode on top of OpenGL windows.  Its behavior "
                      "depends on which Multi-GPU mode is in use:");
        ctk_help_term(b, &i, "Alternate Frame Rendering");
        ctk_help_para(b, &i, "In AFR mode, a vertical green bar displays the "
                      "amount of scaling currently being achieved.  A longer "
                      "bar indicates more scaling.");
        ctk_help_term(b, &i, "Split-Frame Rendering");
        ctk_help_para(b, &i, "In this mode, OpenGL draws a horizontal green "
                      "line showing where the screen is split.  Everything "
                      "above the line is drawn on one GPU and everything "
                      "below is drawn on the other.");
        ctk_help_term(b, &i, "Multi-GPU Antialiasing");
        ctk_help_para(b, &i, "In this mode, OpenGL draws a horizontal green "
                      "line one third of the way across the screen.  Above "
                      "this line, the images from both GPUs are blended to "
                      "produce the currently selected multi-GPU AA mode.  Below the "
                      "line, the image from just one GPU is displayed without "
                      "blending.  This allows easy comparison between the "
                      "multi-GPU AA and single-GPU AA modes.");
    }

    ctk_help_finish(b);

    return b;
}
