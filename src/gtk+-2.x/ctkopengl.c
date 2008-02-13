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

#include "ctkimage.h"

#include "ctkopengl.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

static void vblank_sync_button_toggled   (GtkWidget *, gpointer);

static void allow_flipping_button_toggled(GtkWidget *, gpointer);

static void force_generic_cpu_toggled    (GtkWidget *, gpointer);

static void aa_line_gamma_toggled        (GtkWidget *, gpointer);

static void value_changed                (GtkObject *, gpointer, gpointer);


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


#define __SYNC_TO_VBLANK    (1 << 1)
#define __ALLOW_FLIPPING    (1 << 2)
#define __AA_LINE_GAMMA     (1 << 3)
#define __FORCE_GENERIC_CPU (1 << 4)



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
    GtkWidget *banner;
    GtkWidget *frame;
    GtkWidget *hseparator;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *check_button;
    gint force_generic_cpu_value, aa_line_gamma_value, val;
    ReturnStatus ret;

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
    case NV_CTRL_OPENGL_AA_LINE_GAMMA:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->aa_line_gamma_button);
        func = G_CALLBACK(aa_line_gamma_toggled);
        break;
    case NV_CTRL_FORCE_GENERIC_CPU:
        button = GTK_TOGGLE_BUTTON(ctk_opengl->force_generic_cpu_button);
        func = G_CALLBACK(force_generic_cpu_toggled);
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
    
    ctk_help_finish(b);

    return b;
}
