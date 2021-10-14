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
#include <stdlib.h>

#include "ctkbanner.h"

#include "ctkdropdownmenu.h"
#include "ctkopengl.h"
#include "ctkscale.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

static void vblank_sync_button_toggled   (GtkWidget *, gpointer);

static void post_vblank_sync_button_toggled(CtkOpenGL *, gboolean);

static void post_allow_flipping_button_toggled(CtkOpenGL *, gboolean);

static void post_allow_vrr_button_toggled(CtkOpenGL *, gboolean);

static void post_show_vrr_visual_indicator_button_toggled(CtkOpenGL *, gboolean);

static void post_force_stereo_button_toggled(CtkOpenGL *, gboolean);

static void post_show_graphics_visual_indicator_button_toggled(CtkOpenGL *,
                                                               gboolean);
static void post_xinerama_stereo_button_toggled(CtkOpenGL *, gboolean);

static void post_stereo_eyes_exchange_button_toggled(CtkOpenGL *, gboolean);

static void post_aa_line_gamma_toggled(CtkOpenGL *, gboolean);

static void post_use_conformant_clamping_button_toggled(CtkOpenGL *, gint);

static void allow_flipping_button_toggled(GtkWidget *, gpointer);

static void allow_vrr_button_toggled(GtkWidget *, gpointer);

static void show_vrr_visual_indicator_button_toggled(GtkWidget *, gpointer);

static void force_stereo_button_toggled (GtkWidget *, gpointer);

static void xinerama_stereo_button_toggled (GtkWidget *, gpointer);

static void stereo_eyes_exchange_button_toggled (GtkWidget *, gpointer);

static void aa_line_gamma_toggled        (GtkWidget *, gpointer);

static void use_conformant_clamping_button_toggled(GtkWidget *, gpointer);

static void show_graphics_visual_indicator_button_toggled (GtkWidget *, gpointer);

static void value_changed (GObject *, CtrlEvent *, gpointer);

static const gchar *get_image_settings_string(gint val);

static void post_slider_value_changed(CtkOpenGL *ctk_opengl, gint val);

static void aa_line_gamma_update_received(GObject *object, CtrlEvent *event,
                                          gpointer user_data);

static void post_image_settings_value_changed(CtkOpenGL *ctk_opengl, gint val);

static void image_settings_value_changed(CtkDropDownMenu *widget,
                                         gpointer user_data);

static void image_settings_update_received(GObject *object, CtrlEvent *event,
                                           gpointer user_data);

static GtkWidget *create_slider(CtkOpenGL *ctk_opengl,
                                GtkWidget *vbox,
                                const gchar *name,
                                const char *help,
                                gint attribute,
                                unsigned int bit);

static void slider_changed(GtkAdjustment *adjustment, gpointer user_data);

static GtkWidget *create_stereo_swap_mode_menu(CtkOpenGL *ctk_opengl,
                                               CtkEvent *ctk_event,
                                               gint stereo_swap_mode);

static void stereo_swap_mode_menu_changed(GObject *object,
                                          gpointer user_data);

static void stereo_swap_mode_update_received(GObject *object,
                                             CtrlEvent *event,
                                             gpointer user_data);

static void post_stereo_swap_mode_changed(CtkOpenGL *ctk_opengl, gint idx);

static void build_stereo_swap_mode_table(CtkOpenGL *ctk_opengl);

static gint map_nvctrl_value_to_table(CtkOpenGL *ctk_opengl,
                                      gint val);
static void
ctk_opengl_new_class_init(CtkOpenGLClass *ctk_object_class, gpointer);

static void ctk_opengl_new_finalize(GObject *object);

#define FRAME_PADDING 5

static const char *__sync_to_vblank_help =
"When enabled, OpenGL applications will swap "
"buffers during the vertical retrace; this option is "
"applied to OpenGL applications that are started after "
"this option is set.";

static const char *__aa_line_gamma_checkbox_help =
"Enable the antialiased lines gamma correction checkbox to make the "
"gamma correction slider active.";

static const char *__ssm_menu_help =
"This menu controls the swap mode when Quad-Buffered stereo is used."
"  Application-controled: Stereo swap mode is derived from the "
"value of swap interval.  If it's odd, the per eye "
"swap mode is used.  If it's even, the per eye pair swap mode is used."
"  Per Eye: The driver swaps each eye as it is ready."
"  Per Eye-Pair: The driver waits for both eyes to complete rendering "
"before swapping.";

static const char *__aa_line_gamma_slider_help =
"This option allows Gamma-corrected "
"antialiased lines to consider variances in the color "
"display capabilities of output devices when rendering "
"smooth lines.  This option is applied to OpenGL applications "
"that are started after this option is set.";

static const char *__image_settings_dropdown_help =
"This option adjusts the image quality setting.";

static const char *__force_stereo_help =
"Enabling this option causes OpenGL to force "
"stereo flipping even if a stereo drawable is "
"not visible.  This option is applied "
"immediately.";

static const char *__xinerama_stereo_help =
 "Enabling this option causes OpenGL to allow "
"stereo flipping on multiple X screens configured "
"with Xinerama.  This option is applied immediately.";

static const char *__show_graphics_visual_indicator_help =
"Enabling this option causes the driver to draw "
"information about the graphics API in use, such as "
"instant framerate, vsync status, and whether the "
"application is blitting or flipping. "
"This option is applied to applications that are "
"started after this option is set.";

static const char *__stereo_eyes_exchange_help =
"Enabling this option causes OpenGL to draw the left "
"eye image in the right eye and vice versa for stereo "
"drawables.  This option is applied immediately.";

static const char *__use_conformant_clamping_help =
"Disabling this option causes OpenGL to replace GL_CLAMP with "
"GL_CLAMP_TO_EDGE for borderless 2D textures.  This eliminates "
"seams at the edges of textures in some older games such as "
"Quake 3.";

static const char *__show_vrr_visual_indicator_help  =
"Enabling this option causes OpenGL to draw an indicator showing whether "
"G-SYNC/G-SYNC Compatible is in use, when an application is swapping using "
"flipping.  This option is applied to OpenGL applications that are started "
"after this option is set.";

#define __SYNC_TO_VBLANK      (1 << 1)
#define __ALLOW_FLIPPING      (1 << 2)
#define __AA_LINE_GAMMA_VALUE (1 << 3)
#define __AA_LINE_GAMMA       (1 << 4)
#define __FORCE_GENERIC_CPU   (1 << 5)
#define __FORCE_STEREO        (1 << 6)
#define __IMAGE_SETTINGS      (1 << 7)
#define __XINERAMA_STEREO     (1 << 8)
#define __STEREO_EYES_EXCHANGE (1 << 10)
#define __CONFORMANT_CLAMPING (1 << 12)
#define __ALLOW_VRR         (1 << 13)
#define __SHOW_VRR_VISUAL_INDICATOR       (1 << 14)
#define __STEREO_SWAP_MODE    (1 << 15)
#define __SHOW_GRAPHICS_VISUAL_INDICATOR    (1 << 16)



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
            (GClassInitFunc) ctk_opengl_new_class_init, /* class_init, */
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


static void
ctk_opengl_new_class_init(CtkOpenGLClass *ctk_object_class,
                          gpointer class_data)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_opengl_new_finalize;
}

static void ctk_opengl_new_finalize(GObject *object)
{
    CtkOpenGL *ctk_object = CTK_OPENGL(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}


GtkWidget* ctk_opengl_new(CtrlTarget *ctrl_target,
                          CtkConfig *ctk_config,
                          CtkEvent *ctk_event)
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
    GtkWidget *dropdown;
    GtkWidget *menu;

    gint sync_to_vblank = 0;
    gint flipping_allowed = 0;
    gint vrr_allowed = 0;
    gint show_vrr_visual_indicator = 0;
    gint force_stereo = 0;
    gint xinerama_stereo = 0;
    gint stereo_eyes_exchange = 0;
    gint stereo_swap_mode = 0;
    CtrlAttributeValidValues image_settings_valid;
    gint image_settings_value = 0;
    gint aa_line_gamma = 0;
    gint use_conformant_clamping = 0;
    gint show_graphics_visual_indicator = 0;

    ReturnStatus ret_sync_to_vblank;
    ReturnStatus ret_flipping_allowed;
    ReturnStatus ret_vrr_allowed;
    ReturnStatus ret_show_vrr_visual_indicator;
    ReturnStatus ret_force_stereo;
    ReturnStatus ret_xinerama_stereo;
    ReturnStatus ret_stereo_eyes_exchange;
    ReturnStatus ret_stereo_swap_mode;
    ReturnStatus ret_image_settings;
    ReturnStatus ret_aa_line_gamma;
    ReturnStatus ret_use_conformant_clamping;
    ReturnStatus ret_show_graphics_visual_indicator;

    /* Query OpenGL settings */

    ret_sync_to_vblank =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_SYNC_TO_VBLANK,
                           &sync_to_vblank);

    ret_flipping_allowed =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_FLIPPING_ALLOWED,
                           &flipping_allowed);

    ret_vrr_allowed =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_VRR_ALLOWED,
                           &vrr_allowed);

    ret_show_vrr_visual_indicator =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_SHOW_VRR_VISUAL_INDICATOR,
                           &show_vrr_visual_indicator);

    ret_force_stereo =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_FORCE_STEREO,
                           &force_stereo);

    ret_xinerama_stereo =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_XINERAMA_STEREO,
                           &xinerama_stereo);

    ret_stereo_eyes_exchange =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_STEREO_EYES_EXCHANGE,
                           &stereo_eyes_exchange);

    ret_stereo_swap_mode =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_STEREO_SWAP_MODE,
                           &stereo_swap_mode);

    ret_image_settings = NvCtrlGetValidAttributeValues(ctrl_target,
                                                       NV_CTRL_IMAGE_SETTINGS,
                                                       &image_settings_valid);
    if ((ret_image_settings == NvCtrlSuccess) &&
        (image_settings_valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_RANGE)) {
        ret_image_settings =
            NvCtrlGetAttribute(ctrl_target,
                               NV_CTRL_IMAGE_SETTINGS,
                               &image_settings_value);
    } else {
        ret_image_settings = NvCtrlError;
    }

    ret_aa_line_gamma =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_OPENGL_AA_LINE_GAMMA,
                           &aa_line_gamma);

    ret_use_conformant_clamping =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_TEXTURE_CLAMPING,
                           &use_conformant_clamping);

    ret_show_graphics_visual_indicator =
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_SHOW_GRAPHICS_VISUAL_INDICATOR,
                           &show_graphics_visual_indicator);

    /* There are no OpenGL settings to change (OpenGL disabled?) */
    if ((ret_sync_to_vblank != NvCtrlSuccess) &&
        (ret_flipping_allowed != NvCtrlSuccess) &&
        (ret_vrr_allowed != NvCtrlSuccess) &&
        (ret_show_vrr_visual_indicator != NvCtrlSuccess) &&
        (ret_force_stereo != NvCtrlSuccess) &&
        (ret_xinerama_stereo != NvCtrlSuccess) &&
        (ret_stereo_eyes_exchange != NvCtrlSuccess) &&
        (ret_stereo_swap_mode != NvCtrlSuccess) &&
        (ret_image_settings != NvCtrlSuccess) &&
        (ret_aa_line_gamma != NvCtrlSuccess) &&
        (ret_use_conformant_clamping != NvCtrlSuccess) &&
        (ret_show_graphics_visual_indicator != NvCtrlSuccess)) {
        return NULL;
    }

    object = g_object_new(CTK_TYPE_OPENGL, NULL);

    ctk_opengl = CTK_OPENGL(object);
    ctk_opengl->ctrl_target = ctrl_target;
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

    /*
     * allow G-SYNC/G-SYNC Compatible
     *
     * Always create the checkbox, but only show it if the attribute starts out
     * available.
     */

    label = gtk_label_new("Allow G-SYNC/G-SYNC Compatible");

    check_button = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(check_button), label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                 vrr_allowed);

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(allow_vrr_button_toggled),
                     (gpointer) ctk_opengl);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_VRR_ALLOWED),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

    ctk_config_set_tooltip(ctk_config, check_button,
                           "Enabling this option allows OpenGL to flip "
                           "using G-SYNC/G-SYNC Compatible when possible.  "
                           "This option is applied immediately.");

    ctk_opengl->active_attributes |= __ALLOW_VRR;

    ctk_opengl->allow_vrr_button = check_button;

    /*
     * show G-SYNC/G-SYNC Compatible visual indicator
     *
     * Always create the checkbox, but only show it if the attribute starts out
     * available.
     */

    label = gtk_label_new("Enable G-SYNC/G-SYNC Compatible Visual Indicator");

    check_button = gtk_check_button_new();
    gtk_container_add(GTK_CONTAINER(check_button), label);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                 show_vrr_visual_indicator);

    gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(check_button), "toggled",
                     G_CALLBACK(show_vrr_visual_indicator_button_toggled),
                     (gpointer) ctk_opengl);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_SHOW_VRR_VISUAL_INDICATOR),
                     G_CALLBACK(value_changed), (gpointer) ctk_opengl);

    ctk_config_set_tooltip(ctk_config, check_button,
                           __show_vrr_visual_indicator_help);

    ctk_opengl->active_attributes |= __SHOW_VRR_VISUAL_INDICATOR;

    ctk_opengl->show_vrr_visual_indicator_button = check_button;


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
    if (ret_stereo_eyes_exchange == NvCtrlSuccess) {
        /* Create a menu */
        label = gtk_label_new("Stereo - swap mode:");
        ctk_opengl->active_attributes |= __STEREO_SWAP_MODE;

        menu = create_stereo_swap_mode_menu(ctk_opengl, ctk_event,
                                            stereo_swap_mode);

        ctk_opengl->stereo_swap_mode_menu = menu;

        hbox = gtk_hbox_new(FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(hbox), menu, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    }
    /*
     * Image Quality settings.
     */

    if (ret_image_settings == NvCtrlSuccess &&
            image_settings_valid.range.min == 0) {

        hbox = gtk_hbox_new(FALSE, 0);
        frame = gtk_label_new("Image Settings: ");
        gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 3);

        gtk_container_add(GTK_CONTAINER(vbox), hbox);

        /* create the dropdown */

        dropdown = ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

        if (image_settings_valid.range.max >=
                NV_CTRL_IMAGE_SETTINGS_HIGH_QUALITY) {
            ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(dropdown),
                get_image_settings_string(
                    NV_CTRL_IMAGE_SETTINGS_HIGH_QUALITY),
                NV_CTRL_IMAGE_SETTINGS_HIGH_QUALITY);
        }

        if (image_settings_valid.range.max >=
                NV_CTRL_IMAGE_SETTINGS_QUALITY) {
            ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(dropdown),
                get_image_settings_string(
                    NV_CTRL_IMAGE_SETTINGS_QUALITY),
                NV_CTRL_IMAGE_SETTINGS_QUALITY);
        }

        if (image_settings_valid.range.max >=
                NV_CTRL_IMAGE_SETTINGS_PERFORMANCE) {
            ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(dropdown),
                get_image_settings_string(
                    NV_CTRL_IMAGE_SETTINGS_PERFORMANCE),
                NV_CTRL_IMAGE_SETTINGS_PERFORMANCE);
        }

        if (image_settings_valid.range.max >=
                NV_CTRL_IMAGE_SETTINGS_HIGH_PERFORMANCE) {
            ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(dropdown),
                get_image_settings_string(
                    NV_CTRL_IMAGE_SETTINGS_HIGH_PERFORMANCE),
                NV_CTRL_IMAGE_SETTINGS_HIGH_PERFORMANCE);
        }

        ctk_drop_down_menu_set_current_value(CTK_DROP_DOWN_MENU(dropdown),
                                             image_settings_value);

        gtk_container_add(GTK_CONTAINER(hbox), dropdown);

        g_signal_connect(G_OBJECT(dropdown), "changed",
                         G_CALLBACK(image_settings_value_changed),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_IMAGE_SETTINGS),
                         G_CALLBACK(image_settings_update_received),
                         (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, dropdown,
                               __image_settings_dropdown_help);

        ctk_opengl->active_attributes |= __IMAGE_SETTINGS;
        ctk_opengl->image_settings_dropdown = dropdown;
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

    if (ret_show_graphics_visual_indicator == NvCtrlSuccess) {

        label = gtk_label_new("Enable Graphics API Visual Indicator");

        check_button = gtk_check_button_new();
        gtk_container_add(GTK_CONTAINER(check_button), label);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                     show_graphics_visual_indicator);

        gtk_box_pack_start(GTK_BOX(vbox), check_button, FALSE, FALSE, 0);

        g_signal_connect(G_OBJECT(check_button), "toggled",
                         G_CALLBACK(show_graphics_visual_indicator_button_toggled),
                         (gpointer) ctk_opengl);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_SHOW_GRAPHICS_VISUAL_INDICATOR),
                         G_CALLBACK(value_changed), (gpointer) ctk_opengl);

        ctk_config_set_tooltip(ctk_config, check_button,
                               __show_graphics_visual_indicator_help);

        ctk_opengl->active_attributes |= __SHOW_GRAPHICS_VISUAL_INDICATOR;

        ctk_opengl->show_graphics_visual_indicator_button = check_button;
    }

    gtk_widget_show_all(GTK_WIDGET(object));

    /*
     * If G-SYNC/G-SYNC Compatible is not currently available, start out with
     * the button hidden.
     */
    if (ret_vrr_allowed != NvCtrlSuccess) {
        gtk_widget_hide(GTK_WIDGET(ctk_opengl->allow_vrr_button));
    }
    if (ret_show_vrr_visual_indicator != NvCtrlSuccess) {
        gtk_widget_hide(GTK_WIDGET(ctk_opengl->show_vrr_visual_indicator_button));
    }

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

static void post_allow_vrr_button_toggled(CtkOpenGL *ctk_opengl,
                                          gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "G-SYNC/G-SYNC Compatible %s.",
                                 enabled ? "allowed" : "not allowed");
}

static void post_show_vrr_visual_indicator_button_toggled(CtkOpenGL *ctk_opengl,
                                                          gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "G-SYNC/G-SYNC Compatible visual indicator "
                                 "%s.", enabled ? "enabled" : "disabled");
}

static void post_force_stereo_button_toggled(CtkOpenGL *ctk_opengl, 
                                             gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "OpenGL Stereo Flipping %s.",
                                 enabled ? "forced" : "not forced");
}

static void
post_show_graphics_visual_indicator_button_toggled(CtkOpenGL *ctk_opengl,
                                              gboolean enabled)
{
    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "Graphics API Visual Indicator %s.",
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

static void vblank_sync_button_toggled(GtkWidget *widget,
                                       gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_SYNC_TO_VBLANK, enabled);

    post_vblank_sync_button_toggled(ctk_opengl, enabled);
}


static void allow_flipping_button_toggled(GtkWidget *widget,
                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_FLIPPING_ALLOWED, enabled);
    post_allow_flipping_button_toggled(ctk_opengl, enabled);
}

static void allow_vrr_button_toggled(GtkWidget *widget,
                                     gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_VRR_ALLOWED, enabled);
    post_allow_vrr_button_toggled(ctk_opengl, enabled);
}

static void show_vrr_visual_indicator_button_toggled(GtkWidget *widget,
                                                     gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_SHOW_VRR_VISUAL_INDICATOR, enabled);
    post_show_vrr_visual_indicator_button_toggled(ctk_opengl, enabled);
}

static void force_stereo_button_toggled(GtkWidget *widget,
                                         gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_FORCE_STEREO, enabled);
    post_force_stereo_button_toggled(ctk_opengl, enabled);
}

static void show_graphics_visual_indicator_button_toggled(GtkWidget *widget,
                                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_SHOW_GRAPHICS_VISUAL_INDICATOR, enabled);
    post_show_graphics_visual_indicator_button_toggled(ctk_opengl, enabled);
}

static void xinerama_stereo_button_toggled(GtkWidget *widget,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_XINERAMA_STEREO, enabled);
    post_xinerama_stereo_button_toggled(ctk_opengl, enabled);
}

static void stereo_eyes_exchange_button_toggled(GtkWidget *widget,
                                                gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_STEREO_EYES_EXCHANGE, enabled);

    post_stereo_eyes_exchange_button_toggled(ctk_opengl, enabled);
}

static void aa_line_gamma_toggled(GtkWidget *widget,
                                  gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gboolean enabled;
    ReturnStatus ret;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    ret = NvCtrlSetAttribute(ctrl_target,
                             NV_CTRL_OPENGL_AA_LINE_GAMMA, enabled);

    if (ret != NvCtrlSuccess) return;
    if (ctk_opengl->aa_line_gamma_scale)
        gtk_widget_set_sensitive(ctk_opengl->aa_line_gamma_scale, enabled);
    post_aa_line_gamma_toggled(ctk_opengl, enabled);
}

static void use_conformant_clamping_button_toggled(GtkWidget *widget,
                                                   gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    int clamping;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        clamping = NV_CTRL_TEXTURE_CLAMPING_SPEC;
    } else {
        clamping = NV_CTRL_TEXTURE_CLAMPING_EDGE;
    }

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_TEXTURE_CLAMPING, clamping);

    post_use_conformant_clamping_button_toggled(ctk_opengl, clamping);
}


/*
 * value_changed() - callback function for changed OpenGL settings.
 */

static void value_changed(GObject *object, CtrlEvent *event, gpointer user_data)
{
    CtkOpenGL *ctk_opengl;
    gboolean enabled;
    gboolean check_available = FALSE;
    GtkToggleButton *button;
    GCallback func;
    gint value;

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    ctk_opengl = CTK_OPENGL(user_data);
    value = event->int_attr.value;

    switch (event->int_attr.attribute) {
    case NV_CTRL_SYNC_TO_VBLANK:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->sync_to_vblank_button);
        func = G_CALLBACK(vblank_sync_button_toggled);
        post_vblank_sync_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_FLIPPING_ALLOWED:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->allow_flipping_button);
        func = G_CALLBACK(allow_flipping_button_toggled);
        post_allow_flipping_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_VRR_ALLOWED:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->allow_vrr_button);
        func = G_CALLBACK(allow_vrr_button_toggled);
        post_allow_vrr_button_toggled(ctk_opengl, value);
        check_available = TRUE;
        break;
    case NV_CTRL_SHOW_VRR_VISUAL_INDICATOR:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->show_vrr_visual_indicator_button);
        func = G_CALLBACK(show_vrr_visual_indicator_button_toggled);
        post_show_vrr_visual_indicator_button_toggled(ctk_opengl, value);
        check_available = TRUE;
        break;
    case NV_CTRL_FORCE_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->force_stereo_button);
        func = G_CALLBACK(force_stereo_button_toggled);
        post_force_stereo_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_XINERAMA_STEREO:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->xinerama_stereo_button);
        func = G_CALLBACK(xinerama_stereo_button_toggled);
        post_xinerama_stereo_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_STEREO_EYES_EXCHANGE:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->stereo_eyes_exchange_button);
        func = G_CALLBACK(stereo_eyes_exchange_button_toggled);
        post_stereo_eyes_exchange_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_OPENGL_AA_LINE_GAMMA:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->aa_line_gamma_button);
        func = G_CALLBACK(aa_line_gamma_toggled);
        post_aa_line_gamma_toggled(ctk_opengl, value);
        gtk_widget_set_sensitive(ctk_opengl->aa_line_gamma_scale, value);
        break;
    case NV_CTRL_TEXTURE_CLAMPING:
        button =
            GTK_TOGGLE_BUTTON(ctk_opengl->use_conformant_clamping_button);
        func = G_CALLBACK(use_conformant_clamping_button_toggled);
        post_use_conformant_clamping_button_toggled(ctk_opengl, value);
        break;
    case NV_CTRL_SHOW_GRAPHICS_VISUAL_INDICATOR:
        button =
            GTK_TOGGLE_BUTTON(ctk_opengl->show_graphics_visual_indicator_button);
        func = G_CALLBACK(show_graphics_visual_indicator_button_toggled);
        post_show_graphics_visual_indicator_button_toggled(ctk_opengl, value);
        break;
    default:
        return;
    }
    
    enabled = gtk_toggle_button_get_active(button);

    if (enabled != value) {
        
        g_signal_handlers_block_by_func(button, func, ctk_opengl);
        gtk_toggle_button_set_active(button, value);
        g_signal_handlers_unblock_by_func(button, func, ctk_opengl);
    }

    if (check_available && event->int_attr.is_availability_changed) {
        if (event->int_attr.availability) {
            gtk_widget_show(GTK_WIDGET(button));
        } else {
            gtk_widget_hide(GTK_WIDGET(button));
        }
    }

} /* value_changed() */



/*
 * build_stereo_swap_mode_table() - build a table of stereo swap mode, showing
 * modes supported by the hardware.
 */
static void build_stereo_swap_mode_table(CtkOpenGL *ctk_opengl)
{
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    ReturnStatus ret;
    CtrlAttributeValidValues valid;
    gint i = 0, n = 0, num_of_modes = 0, mask;

    if (ctk_opengl->stereo_swap_mode_table_size > 0 &&
        ctk_opengl->stereo_swap_mode_table != NULL) {
        ctk_opengl->stereo_swap_mode_table_size = 0;
        free(ctk_opengl->stereo_swap_mode_table);
    }

    ret =
        NvCtrlGetValidAttributeValues(ctrl_target,
                                      NV_CTRL_STEREO_SWAP_MODE,
                                      &valid);
    if ((ret != NvCtrlSuccess) ||
        (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS)) {
        /*
         * We do not have valid information to build a mode table
         * so we need to create default data for the placeholder menu.
         */
        ctk_opengl->stereo_swap_mode_table_size = 1;
        ctk_opengl->stereo_swap_mode_table =
            calloc(1, sizeof(ctk_opengl->stereo_swap_mode_table[0]));
        if (ctk_opengl->stereo_swap_mode_table) {
            ctk_opengl->stereo_swap_mode_table[0] =
                NV_CTRL_STEREO_SWAP_MODE_APPLICATION_CONTROL;
        } else {
            ctk_opengl->stereo_swap_mode_table_size = 0;
        }
        return;
    }

    /* count no. of supported modes */
    mask = valid.allowed_ints;
    while(mask) {
        mask = mask & (mask - 1);
        num_of_modes++;
    }

    ctk_opengl->stereo_swap_mode_table_size = num_of_modes;
    ctk_opengl->stereo_swap_mode_table =
        calloc(num_of_modes, sizeof(ctk_opengl->stereo_swap_mode_table[0]));
    if (!ctk_opengl->stereo_swap_mode_table) {
        ctk_opengl->stereo_swap_mode_table_size = 0;
        return;
    }

    mask = valid.allowed_ints;
    while (mask) {

        while (!(mask & (1 << i))) {
            i++;
        }

        ctk_opengl->stereo_swap_mode_table[n++] = i;
        mask = mask & (mask - 1);
    }

    return;
}



/*
 * create_stereo_swap_mode_menu() - Helper function that creates the
 * Stereo swap mode dropdown menu.
 */

static GtkWidget *create_stereo_swap_mode_menu(CtkOpenGL *ctk_opengl,
                                               CtkEvent *ctk_event,
                                               gint stereo_swap_mode)
{
    CtkDropDownMenu *stereo_swap_mode_menu;
    gint i, idx = 0;

    /* Create the menu */

    stereo_swap_mode_menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    /* setup stereo swap mode */
    build_stereo_swap_mode_table(ctk_opengl);

    /* populate dropdown list for stereo swap mode */

    ctk_drop_down_menu_reset(stereo_swap_mode_menu);

    for (i = 0; i < ctk_opengl->stereo_swap_mode_table_size; i++) {
        switch (ctk_opengl->stereo_swap_mode_table[i]) {
        case NV_CTRL_STEREO_SWAP_MODE_PER_EYE:
            ctk_drop_down_menu_append_item(stereo_swap_mode_menu, "Per Eye", i);
            break;
        case NV_CTRL_STEREO_SWAP_MODE_PER_EYE_PAIR:
            ctk_drop_down_menu_append_item(stereo_swap_mode_menu,
                                           "Per Eye-Pair", i);
            break;
        default:
        case NV_CTRL_STEREO_SWAP_MODE_APPLICATION_CONTROL:
            ctk_drop_down_menu_append_item(stereo_swap_mode_menu,
                                           "Application-controlled", i);
            break;
        }
    }

    /* set the menu item */
    idx = map_nvctrl_value_to_table(ctk_opengl, stereo_swap_mode);
    ctk_drop_down_menu_set_current_value(stereo_swap_mode_menu, idx);

    ctk_drop_down_menu_set_tooltip(ctk_opengl->ctk_config, stereo_swap_mode_menu,
                                   __ssm_menu_help);

    g_signal_connect(G_OBJECT(stereo_swap_mode_menu),
                     "changed",
                     G_CALLBACK(stereo_swap_mode_menu_changed),
                     (gpointer) ctk_opengl);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME
                     (NV_CTRL_STEREO_SWAP_MODE),
                     G_CALLBACK(stereo_swap_mode_update_received),
                     (gpointer) ctk_opengl);

    return GTK_WIDGET(stereo_swap_mode_menu);
}



static gint map_nvctrl_value_to_table(CtkOpenGL *ctk_opengl,
                                      gint val)
{
    int i;
    for (i = 0; i < ctk_opengl->stereo_swap_mode_table_size; i++) {
        if (val == ctk_opengl->stereo_swap_mode_table[i]) {
            return i;
        }
    }

    return 0;
}



/*
 * stereo_swap_mode_menu_changed() - callback function for menu change
 */

static void stereo_swap_mode_menu_changed(GObject *object, gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gint idx;

    idx = ctk_drop_down_menu_get_current_value
        (CTK_DROP_DOWN_MENU(ctk_opengl->stereo_swap_mode_menu));

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_STEREO_SWAP_MODE,
                       ctk_opengl->stereo_swap_mode_table[idx]);

    post_stereo_swap_mode_changed(ctk_opengl,
                                  ctk_opengl->stereo_swap_mode_table[idx]);
}



/*
 * post_stereo_swap_mode_changed() - helper function to print status bar message
 */

static void post_stereo_swap_mode_changed(CtkOpenGL *ctk_opengl, gint idx)
{
    static const char *stereo_swap_mode_table[] = {
        "Application-controlled", /* NV_CTRL_STEREO_SWAP_MODE_APPLICATION_CONTROL */
        "Per Eye",                /* NV_CTRL_STEREO_SWAP_MODE_PER_EYE */
        "Per Eye-Pair",           /* NV_CTRL_STEREO_SWAP_MODE_PER_EYE_PAIR */
    };

    if (idx < NV_CTRL_STEREO_SWAP_MODE_APPLICATION_CONTROL ||
        idx > NV_CTRL_STEREO_SWAP_MODE_PER_EYE_PAIR) {
        return;
    }

    ctk_config_statusbar_message(ctk_opengl->ctk_config,
                                 "Set %s Stereo swap mode.",
                                 stereo_swap_mode_table[idx]);
}



static void stereo_swap_mode_update_received(GObject *object,
                                             CtrlEvent *event,
                                             gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    GtkWidget *menu;
    gint idx = map_nvctrl_value_to_table(ctk_opengl,
                                         event->int_attr.value);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }
    /* Update the dropdown menu */
    menu = GTK_WIDGET(ctk_opengl->stereo_swap_mode_menu);

    g_signal_handlers_block_by_func
        (G_OBJECT(menu),
         G_CALLBACK(stereo_swap_mode_menu_changed),
         (gpointer) ctk_opengl);

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_opengl->stereo_swap_mode_menu), idx);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(menu),
         G_CALLBACK(stereo_swap_mode_menu_changed),
         (gpointer) ctk_opengl);
    post_stereo_swap_mode_changed(ctk_opengl, idx);
}

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
 * image_settings_value_changed() - callback for the "changed" signal
 * from the image settings combobox.
 */

static void image_settings_value_changed(CtkDropDownMenu *widget,
                                         gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gint val = ctk_drop_down_menu_get_current_value(widget);

    NvCtrlSetAttribute(ctrl_target, NV_CTRL_IMAGE_SETTINGS, val);
    post_image_settings_value_changed(ctk_opengl, val);

} /* image_settings_value_changed() */

/*
 * image_settings_update_received() - this function is called when the
 * NV_CTRL_IMAGE_SETTINGS attribute is changed by another NV-CONTROL client.
 */

static void image_settings_update_received(GObject *object,
                                           CtrlEvent *event,
                                           gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtkDropDownMenu *dropdown = CTK_DROP_DOWN_MENU(
                                    ctk_opengl->image_settings_dropdown);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    g_signal_handlers_block_by_func(G_OBJECT(dropdown),
                                    G_CALLBACK(image_settings_value_changed),
                                    (gpointer) ctk_opengl);

    ctk_drop_down_menu_set_current_value(dropdown, event->int_attr.value);
    post_image_settings_value_changed(ctk_opengl, event->int_attr.value);

    g_signal_handlers_unblock_by_func(G_OBJECT(dropdown),
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
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    gint attribute, value;

    user_data = g_object_get_data(G_OBJECT(adjustment), "opengl_attribute");
    attribute = GPOINTER_TO_INT(user_data);
    value = (gint) gtk_adjustment_get_value(adjustment);

    NvCtrlSetAttribute(ctrl_target, attribute, value);

    post_slider_value_changed(ctk_opengl, value);

} /* slider_changed() */


/*
 * aa_line_gamma_update_received() - this function is called when the
 * NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE attribute is changed by another NV-CONTROL
 * client.
 */

static void aa_line_gamma_update_received(GObject *object,
                                          CtrlEvent *event,
                                          gpointer user_data)
{
    CtkOpenGL *ctk_opengl = CTK_OPENGL(user_data);
    CtkScale *scale = CTK_SCALE(ctk_opengl->aa_line_gamma_scale);
    GtkAdjustment *adjustment;

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    adjustment = GTK_ADJUSTMENT(scale->gtk_adjustment);
    g_signal_handlers_block_by_func(G_OBJECT(adjustment),
                                    G_CALLBACK(slider_changed),
                                    (gpointer) ctk_opengl);

    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustment),
                             (gint)event->int_attr.value);
    post_slider_value_changed(ctk_opengl, event->int_attr.value);

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
    CtrlTarget *ctrl_target = ctk_opengl->ctrl_target;
    GtkAdjustment *adjustment;
    GtkWidget *scale, *widget;
    gint min, max, val, step_incr, page_incr;
    CtrlAttributeValidValues range;
    ReturnStatus ret;
    /* get the attribute value */

    ret = NvCtrlGetAttribute(ctrl_target, attribute, &val);
    if (ret != NvCtrlSuccess) return NULL;
    /* get the range for the attribute */

    NvCtrlGetValidAttributeValues(ctrl_target, attribute, &range);

    if (range.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_RANGE) {
        return NULL;
    }
    min = range.range.min;
    max = range.range.max;

    step_incr = ((max) - (min))/10;
    if (step_incr <= 0) step_incr = 1;
    page_incr = ((max) - (min))/25;
    if (page_incr <= 0) page_incr = 1;

    /* create the slider */
    adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(val, min, max,
                                                   step_incr, page_incr, 0.0));

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
        ctk_help_para(b, &i, "%s", __sync_to_vblank_help);
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

    if (ctk_opengl->active_attributes & __ALLOW_VRR) {
        ctk_help_heading(b, &i, "Allow G-SYNC/G-SYNC Compatible");
        ctk_help_para(b, &i, "Enabling this option allows OpenGL to use "
                      "G-SYNC/G-SYNC Compatible when available.  G-SYNC and "
                      "G-SYNC Compatible are technologies that allow monitors "
                      "to delay updating the screen until the GPU is ready to "
                      "display a new frame.  Without G-SYNC or G-SYNC "
                      "Compatible, the GPU waits for the display to be ready "
                      "to accept a new frame instead.");

        ctk_help_para(b, &i, "Note that this option is applied immediately, "
                      "unlike most other OpenGL options which are only "
                      "applied to OpenGL applications that are started "
                      "after the option is set.");

        ctk_help_para(b, &i, "When G-SYNC is active on a G-SYNC or G-SYNC "
                      "Compatible display and \"Sync to VBlank\" is disabled, "
                      "applications rendering faster than the maximum refresh "
                      "rate will tear. This eliminates tearing for frame rates "
                      "below the monitor's maximum refresh rate while "
                      "minimizing latency for frame rates above it. When "
                      "\"Sync to VBlank\" is enabled, the frame rate is "
                      "limited to the monitor's maximum refresh rate to "
                      "eliminate tearing completely. When a G-SYNC Compatible "
                      "display is in use, applications rendering slower than "
                      "the minimum refresh rate may tear when "
                      "\"Sync to VBlank\" is disabled, and their swaps may "
                      "not complete until the next vblank when \"Sync to "
                      "VBlank\" is enabled.");

        ctk_help_para(b, &i, "This option can be overridden on a "
                      "per-application basis using the GLVRRAllowed "
                      "application profile key.");
    }

    if (ctk_opengl->active_attributes & __SHOW_VRR_VISUAL_INDICATOR) {
        ctk_help_heading(b, &i, "G-SYNC/G-SYNC Compatible Visual Indicator");
        ctk_help_para(b, &i, "%s", __show_vrr_visual_indicator_help);
    }

    if (ctk_opengl->active_attributes & __FORCE_STEREO) {
        ctk_help_heading(b, &i, "Force Stereo Flipping");
        ctk_help_para(b, &i, "%s", __force_stereo_help);
    }
    
    if (ctk_opengl->active_attributes & __XINERAMA_STEREO) {
        ctk_help_heading(b, &i, "Allow Xinerama Stereo Flipping");
        ctk_help_para(b, &i, "%s", __xinerama_stereo_help);
    }
    
    if (ctk_opengl->active_attributes & __STEREO_EYES_EXCHANGE) {
        ctk_help_heading(b, &i, "Exchange Stereo Eyes");
        ctk_help_para(b, &i, "%s", __stereo_eyes_exchange_help);
    }
    
    if (ctk_opengl->active_attributes & __STEREO_SWAP_MODE) {
        ctk_help_term(b, &i, "Stereo - swap mode");
        ctk_help_para(b, &i, "%s", __ssm_menu_help);
    }

    if (ctk_opengl->active_attributes & __IMAGE_SETTINGS) {
        ctk_help_heading(b, &i, "Image Settings");
        ctk_help_para(b, &i, "This setting gives you full control over the "
                      "image quality in your applications.");
        ctk_help_para(b, &i, "Several quality settings are available for "
                      "you to choose from with the Image Settings options.  "
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
        ctk_help_para(b, &i, "This setting results in a blend of optimal "
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
        ctk_help_para(b, &i, "%s", __aa_line_gamma_checkbox_help );
    }

    if (ctk_opengl->active_attributes & __AA_LINE_GAMMA_VALUE) {
        ctk_help_heading(b, &i, "Set gamma correction for "
                         "antialiased lines");
        ctk_help_para(b, &i, "%s", __aa_line_gamma_slider_help);
    }

    if (ctk_opengl->active_attributes & __CONFORMANT_CLAMPING) {
        ctk_help_heading(b, &i, "Use Conformant Texture Clamping");
        ctk_help_para(b, &i, "%s", __use_conformant_clamping_help);
    }

    if (ctk_opengl->active_attributes & __SHOW_GRAPHICS_VISUAL_INDICATOR) {
        ctk_help_heading(b, &i, "Graphics API Visual Indicator");
        ctk_help_para(b, &i, "%s", __show_graphics_visual_indicator_help);
    }

    ctk_help_finish(b);

    return b;
}
