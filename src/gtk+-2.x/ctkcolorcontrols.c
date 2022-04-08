/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <libintl.h>


#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkcolorcontrols.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"

#define _(STRING) gettext(STRING)
#define N_(STRING) STRING

/* function prototypes */
static void
ctk_color_controls_class_init(CtkColorControlsClass *ctk_object_class);

static void ctk_color_controls_finalize(GObject *object);

static gboolean build_color_space_table(CtkColorControls *ctk_color_controls,
                                        CtrlAttributeValidValues valid);

static gint map_nvctrl_value_to_table(CtkColorControls *ctk_color_controls,
                                      gint val);

static gboolean update_color_space_menu_info(CtkColorControls *ctk_color_controls);
static gboolean update_color_range_menu_info(CtkColorControls *ctk_color_controls);
static gboolean update_current_color_space_menu_info(CtkColorControls *ctk_color_controls);
static gboolean update_current_color_range_menu_info(CtkColorControls *ctk_color_controls);

static void setup_reset_button(CtkColorControls *ctk_color_controls);

static void color_space_menu_changed(GtkWidget *widget,
                                     gpointer user_data);
static void color_range_menu_changed(GtkWidget *widget,
                                     gpointer user_data);

static void color_control_update_received(GObject *object, CtrlEvent *event,
                                          gpointer user_data);
static void post_current_color_range_update(CtkColorControls *ctk_color_controls,
                                            gint color_range);
static void post_current_color_space_update(CtkColorControls *ctk_color_controls,
                                            gint color_space);
static void post_color_range_update(CtkColorControls *ctk_color_controls,
                                    gint color_range);
static void post_color_space_update(CtkColorControls *ctk_color_controls,
                                    gint color_space);

/* macros */
#define FRAME_PADDING 5

/* help text */
static const char * __color_controls_help =
N_("The Color Controls allow changing the preferred color space and color range "
"of the display device. These settings may be overridden depending on the "
"current mode and color space on the display device.");

static const char * __color_space_help =
N_("The possible values for Color Space vary depending on the capabilities of "
"the display device and the GPU, but may contain \"RGB\", \"YCbCr422\", "
"and \"YCbCr444\". If an HDMI 2.0 4K@60Hz mode is in use and the display "
"device or GPU is incapable of driving the mode in RGB, the preferred color "
"space is preserved, but the current color space is overridden to YCbCr420.");

static const char * __color_range_help =
N_("The possible values for Color Range are \"Limited\" and \"Full\". "
"If the current color space only allows a limited color range, the "
"preferred color range is preserved, but the current color range "
"is overridden to limited range.");

GType ctk_color_controls_get_type(void)
{
    static GType ctk_color_controls_type = 0;

    if (!ctk_color_controls_type) {
        static const GTypeInfo ctk_color_controls_info = {
            sizeof (CtkColorControlsClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_color_controls_class_init, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkColorControls),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_color_controls_type =
            g_type_register_static (GTK_TYPE_VBOX,
                                    "CtkColorControls",
                                    &ctk_color_controls_info, 0);
    }

    return ctk_color_controls_type;
} /* ctk_color_controls_get_type() */



static void
ctk_color_controls_class_init(CtkColorControlsClass *ctk_object_class)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_color_controls_finalize;
}



static void ctk_color_controls_finalize(GObject *object)
{
    CtkColorControls *ctk_object = CTK_COLOR_CONTROLS(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}



GtkWidget* ctk_color_controls_new(CtrlTarget *ctrl_target,
                                  CtkConfig *ctk_config,
                                  CtkEvent *ctk_event,
                                  GtkWidget *reset_button,
                                  char *name)
{
    GObject *object;
    CtkColorControls *ctk_color_controls;
    GtkWidget *frame, *vbox, *hbox, *label;
    GtkWidget *table, *separator;
    CtkDropDownMenu *menu;
    ReturnStatus ret1, ret2;
    CtrlAttributeValidValues valid_color_spaces, valid_throwaway;
    gint i;

    /* check if color configuration is supported */
    ret1 = NvCtrlGetValidAttributeValues(ctrl_target,
                                         NV_CTRL_COLOR_SPACE,
                                         &valid_color_spaces);
    ret2 = NvCtrlGetValidAttributeValues(ctrl_target,
                                         NV_CTRL_COLOR_RANGE,
                                         &valid_throwaway);

    if ((ret1 != NvCtrlSuccess) || (ret2 != NvCtrlSuccess)) {
        return NULL;
    }

    /* create the object */
    object = g_object_new(CTK_TYPE_COLOR_CONTROLS, NULL);
    if (!object) {
        return NULL;
    }

    ctk_color_controls = CTK_COLOR_CONTROLS(object);
    ctk_color_controls->ctrl_target = ctrl_target;
    ctk_color_controls->ctk_config = ctk_config;
    ctk_color_controls->ctk_event = ctk_event;
    ctk_color_controls->reset_button = reset_button;
    ctk_color_controls->name = strdup(name);

    /* check if querying current color space and range are supported */
    ret1 = NvCtrlGetValidAttributeValues(ctrl_target,
                                         NV_CTRL_CURRENT_COLOR_SPACE,
                                         &valid_throwaway);
    ret2 = NvCtrlGetValidAttributeValues(ctrl_target,
                                         NV_CTRL_CURRENT_COLOR_RANGE,
                                         &valid_throwaway);

    if ((ret1 != NvCtrlSuccess) || (ret2 != NvCtrlSuccess)) {
        /*
         * Fall back to querying the preferred color space and range, to
         * support older drivers which don't support querying the current
         * color range.
         */
        ctk_color_controls->current_color_attributes_supported = FALSE;
    } else {
        ctk_color_controls->current_color_attributes_supported = TRUE;
    }

    /* build a table holding available color space */
    if (!build_color_space_table(ctk_color_controls, valid_color_spaces)) {
        return NULL;
    }

    /* create main color box & frame */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, FRAME_PADDING);
    ctk_color_controls->color_controls_box = hbox;

    frame = gtk_frame_new(_("Color Controls"));
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);

    table = gtk_table_new(3, 4, FALSE);
    gtk_container_add(GTK_CONTAINER(frame), table);
    gtk_table_set_row_spacings(GTK_TABLE(table), 5);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    /* dropdown list for color space */
    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    for (i = 0; i < ctk_color_controls->color_space_table_size; i++) {
        switch (ctk_color_controls->color_space_table[i]) {
        case NV_CTRL_COLOR_SPACE_YCbCr422:
            ctk_drop_down_menu_append_item(menu, "YCbCr422", i);
            break;
        case NV_CTRL_COLOR_SPACE_YCbCr444:
            ctk_drop_down_menu_append_item(menu, "YCbCr444", i);
            break;
        default:
        case NV_CTRL_COLOR_SPACE_RGB:
            ctk_drop_down_menu_append_item(menu, "RGB", i);
            break;
        }
    }
    ctk_color_controls->color_space_menu = GTK_WIDGET(menu);
    ctk_config_set_tooltip(ctk_config,
                           ctk_color_controls->color_space_menu,
                           _(__color_space_help));

    /* If dropdown only has one item, disable it */
    if (ctk_color_controls->color_space_table_size > 1) {
        gtk_widget_set_sensitive(ctk_color_controls->color_space_menu, True);
    } else {
        gtk_widget_set_sensitive(ctk_color_controls->color_space_menu, False);
    }


    g_signal_connect(G_OBJECT(ctk_color_controls->color_space_menu),
                     "changed", G_CALLBACK(color_space_menu_changed),
                     (gpointer) ctk_color_controls);

    /* pack the label & drop down */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(_("Color Space: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_color_controls->color_space_menu,
                       FALSE, FALSE, 0);

    /* Build current color space widget  */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(_("Current Color Space: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 0, 1,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_color_controls->current_color_space_txt = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    /* H-bar */
    vbox = gtk_vbox_new(FALSE, 0);
    separator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), vbox, 0, 4, 1, 2,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    /* Build color widgets & pack them in table */
    /* dropdown list for color range */

    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    ctk_drop_down_menu_append_item(menu, _("Full"), 0);
    ctk_drop_down_menu_append_item(menu, _("Limited"), 1);
    ctk_color_controls->color_range_menu = GTK_WIDGET(menu);

    ctk_config_set_tooltip(ctk_config,
                           ctk_color_controls->color_range_menu,
                           _(__color_range_help));

    g_signal_connect(G_OBJECT(ctk_color_controls->color_range_menu),
                     "changed", G_CALLBACK(color_range_menu_changed),
                     (gpointer) ctk_color_controls);

    /* Packing label & dropdown */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);

    label = gtk_label_new(_("Color Range: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    gtk_box_pack_start(GTK_BOX(hbox),
                       ctk_color_controls->color_range_menu,
                       FALSE, FALSE, 0);

    /* Build CurrentMode widget  */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 2, 3, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(_("Current Color Range: "));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 3, 4, 2, 3,
                     GTK_FILL, GTK_FILL | GTK_EXPAND, 5, 0);
    label = gtk_label_new(NULL);
    ctk_color_controls->current_color_range_txt = label;
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.5f);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    ctk_color_controls_setup(ctk_color_controls);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_COLOR_RANGE),
                     G_CALLBACK(color_control_update_received),
                     (gpointer) ctk_color_controls);
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_COLOR_SPACE),
                     G_CALLBACK(color_control_update_received),
                     (gpointer) ctk_color_controls);

    if (ctk_color_controls->current_color_attributes_supported) {
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_CURRENT_COLOR_RANGE),
                         G_CALLBACK(color_control_update_received),
                         (gpointer) ctk_color_controls);
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_CURRENT_COLOR_SPACE),
                         G_CALLBACK(color_control_update_received),
                         (gpointer) ctk_color_controls);
    }

    return GTK_WIDGET(object);

} /* ctk_color_controls_new() */



/*
 * setup_reset_button() - enables the reset button if any of the current
 * settings are not the default.
 */
static void setup_reset_button(CtkColorControls *ctk_color_controls)
{
    gint history;
    gint val;
    CtkDropDownMenu *color_space_menu, *color_range_menu;

    if (!ctk_widget_get_sensitive(ctk_color_controls->color_controls_box)) {
        /* Nothing is available, don't bother enabling the reset button yet. */
        return;
    }

    color_space_menu = CTK_DROP_DOWN_MENU(ctk_color_controls->color_space_menu);
    history = ctk_drop_down_menu_get_current_value(color_space_menu);

    val = ctk_color_controls->color_space_table[history];
    if (val != NV_CTRL_COLOR_SPACE_RGB) {
        goto enable;
    }

    color_range_menu = CTK_DROP_DOWN_MENU(ctk_color_controls->color_range_menu);
    history = ctk_drop_down_menu_get_current_value(color_range_menu);
    if (val != NV_CTRL_COLOR_RANGE_FULL) {
        goto enable;
    }

    /* Don't disable reset button here, since other settings that are not
     * managed by the ctk_image_slider here may need it enabled
     */
    return;

 enable:
    gtk_widget_set_sensitive(ctk_color_controls->reset_button, TRUE);
}



/*
 * ctk_color_controls_setup() - Setup routine for color attributes. Used
 * in DFP setup stage as well as for updating the GUI when there is change in
 * color range or color space.
 */
void ctk_color_controls_setup(CtkColorControls *ctk_color_controls)
{
    if (!ctk_color_controls) {
        return;
    }

    /* color space */
    if (!update_color_space_menu_info(ctk_color_controls)) {
        gtk_widget_set_sensitive(ctk_color_controls->color_controls_box, FALSE);
        gtk_widget_hide(ctk_color_controls->color_controls_box);
    }

    if (!update_color_range_menu_info(ctk_color_controls)) {
        gtk_widget_set_sensitive(ctk_color_controls->color_controls_box, FALSE);
        gtk_widget_hide(ctk_color_controls->color_controls_box);
    }

    if (ctk_color_controls->current_color_attributes_supported &&
        !update_current_color_space_menu_info(ctk_color_controls)) {
        gtk_widget_set_sensitive(ctk_color_controls->color_controls_box, FALSE);
        gtk_widget_hide(ctk_color_controls->color_controls_box);
    }

    if (ctk_color_controls->current_color_attributes_supported &&
        !update_current_color_range_menu_info(ctk_color_controls)) {
        gtk_widget_set_sensitive(ctk_color_controls->color_controls_box, FALSE);
        gtk_widget_hide(ctk_color_controls->color_controls_box);
    }

    setup_reset_button(ctk_color_controls);

} /* ctk_color_controls_setup() */

static void update_current_color_range_text(CtkColorControls *ctk_color_controls,
                                            gint color_range)
{
    switch(color_range) {
    case NV_CTRL_CURRENT_COLOR_RANGE_FULL:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_range_txt),
                           _("Full"));
        break;
    case NV_CTRL_CURRENT_COLOR_RANGE_LIMITED:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_range_txt),
                           _("Limited"));
        break;
    default:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_range_txt),
                           _("Unknown"));
        break;
    }
} /* update_current_color_range_text() */

static gboolean update_current_color_range_menu_info(CtkColorControls *ctk_color_controls)
{
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    gint current_color_range = NV_CTRL_CURRENT_COLOR_RANGE_FULL;

    if (NvCtrlSuccess !=
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_CURRENT_COLOR_RANGE,
                           &current_color_range)) {
        return FALSE;
    }

    update_current_color_range_text(ctk_color_controls, current_color_range);

    return TRUE;

} /* update_current_color_range_menu_info() */

static gboolean update_color_range_menu_info(CtkColorControls *ctk_color_controls)
{
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    gint color_range = NV_CTRL_COLOR_RANGE_FULL;

    /* color space */
    if (NvCtrlSuccess !=
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_COLOR_RANGE,
                           &color_range)) {
        return FALSE;
    }

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_color_controls->color_range_menu), color_range);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_range_menu),
         G_CALLBACK(color_range_menu_changed),
         (gpointer) ctk_color_controls);

    if (!ctk_color_controls->current_color_attributes_supported) {
        /*
         * If querying the current color space isn't supported, fall back to
         * displaying the preferred color space as the current color space.
         */
        update_current_color_range_text(ctk_color_controls, color_range);
    }

    return TRUE;

} /* update_color_range_menu_info() */

static void update_current_color_space_text(CtkColorControls *ctk_color_controls,
                                            gint color_space)
{
    switch(color_space) {
    case NV_CTRL_CURRENT_COLOR_SPACE_YCbCr420:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_space_txt),
                           "YCbCr420");
        break;
    case NV_CTRL_CURRENT_COLOR_SPACE_YCbCr422:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_space_txt),
                           "YCbCr422");
        break;
    case NV_CTRL_CURRENT_COLOR_SPACE_YCbCr444:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_space_txt),
                           "YCbCr444");
        break;
    case NV_CTRL_CURRENT_COLOR_SPACE_RGB:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_space_txt),
                           "RGB");
        break;
    default:
        gtk_label_set_text(GTK_LABEL(ctk_color_controls->current_color_space_txt),
                           _("Unknown"));
        break;
    }
} /* update_current_color_space_text() */

static gboolean update_current_color_space_menu_info(CtkColorControls *ctk_color_controls)
{
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    gint current_color_space = NV_CTRL_CURRENT_COLOR_SPACE_RGB;

    if (NvCtrlSuccess !=
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_CURRENT_COLOR_SPACE,
                           &current_color_space)) {
        return FALSE;
    }

    update_current_color_space_text(ctk_color_controls, current_color_space);

    return TRUE;

} /* update_current_color_space_menu_info() */

static gboolean update_color_space_menu_info(CtkColorControls *ctk_color_controls)
{
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    gint color_space_nvctrl, color_space;

    /* color space */
    if (NvCtrlSuccess !=
        NvCtrlGetAttribute(ctrl_target,
                           NV_CTRL_COLOR_SPACE,
                           &color_space_nvctrl)) {
        return FALSE;
    }

    color_space = map_nvctrl_value_to_table(ctk_color_controls,
                                            color_space_nvctrl);

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_color_controls->color_space_menu), color_space);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_color_controls->color_space_menu),
         G_CALLBACK(color_space_menu_changed),
         (gpointer) ctk_color_controls);

    if (!ctk_color_controls->current_color_attributes_supported) {
        /*
         * If querying the current color space isn't supported, fall back to
         * displaying the preferred color space as the current color space.
         */
        update_current_color_space_text(ctk_color_controls, color_space_nvctrl);
    }

    return TRUE;

} /* update_color_space_menu_info() */

static
void post_current_color_range_update(CtkColorControls *ctk_color_controls,
                                     gint color_range)
{
    static const char *color_range_table[] = {
        N_("Full"),     /* NV_CTRL_CURRENT_COLOR_RANGE_FULL */
        N_("Limited"),  /* NV_CTRL_CURRENT_COLOR_RANGE_LIMITED */
    };

    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 _("Current Color Range set to %s for %s."),
                                 _(color_range_table[color_range]),
                                 ctk_color_controls->name);
}

static
void post_color_range_update(CtkColorControls *ctk_color_controls,
                             gint color_range)
{
    static const char *color_range_table[] = {
        N_("Full"),     /* NV_CTRL_COLOR_RANGE_FULL */
        N_("Limited"),  /* NV_CTRL_COLOR_RANGE_LIMITED */
    };

    gtk_widget_set_sensitive(ctk_color_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 _("Color Range set to %s for %s."),
                                 _(color_range_table[color_range]),
                                 ctk_color_controls->name);
}

static
void post_current_color_space_update(CtkColorControls *ctk_color_controls,
                                     gint color_space)
{
    static const char *color_space_table[] = {
        "RGB",       /* NV_CTRL_CURRENT_COLOR_SPACE_RGB */
        "YCbCr422",  /* NV_CTRL_CURRENT_COLOR_SPACE_YCbCr422 */
        "YCbCr444",  /* NV_CTRL_CURRENT_COLOR_SPACE_YCbCr444 */
        "YCbCr420"   /* NV_CTRL_CURRENT_COLOR_SPACE_YCbCr420 */
    };

    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 _("Current Color Space set to %s for %s."),
                                 color_space_table[color_space],
                                 ctk_color_controls->name);
}

static
void post_color_space_update(CtkColorControls *ctk_color_controls,
                             gint color_space)
{
    static const char *color_space_table[] = {
        "RGB",       /* NV_CTRL_COLOR_SPACE_RGB */
        "YCbCr422",  /* NV_CTRL_COLOR_SPACE_YCbCr422 */
        "YCbCr444"   /* NV_CTRL_COLOR_SPACE_YCbCr444 */
    };

    gtk_widget_set_sensitive(ctk_color_controls->reset_button, TRUE);
    ctk_config_statusbar_message(ctk_color_controls->ctk_config,
                                 _("Color Space set to %s for %s."),
                                 color_space_table[ctk_color_controls->color_space_table[color_space]],
                                 ctk_color_controls->name);
}

static void color_range_menu_changed(GtkWidget *widget,
                                     gpointer user_data)
{
    CtkColorControls *ctk_color_controls =
        CTK_COLOR_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    gint color_range = ctk_drop_down_menu_get_current_value(menu);

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_COLOR_RANGE,
                       color_range);

    /* reflecting the change to statusbar message and the reset button */
    post_color_range_update(ctk_color_controls, color_range);

    if (!ctk_color_controls->current_color_attributes_supported) {
        /*
         * If querying the current color space isn't supported, fall back to
         * displaying the preferred color space as the current color space.
         */
        update_current_color_range_text(ctk_color_controls, color_range);
    }
} /* color_range_menu_changed() */


static void color_space_menu_changed(GtkWidget *widget,
                                     gpointer user_data)
{
    CtkColorControls *ctk_color_controls =
        CTK_COLOR_CONTROLS(user_data);
    CtrlTarget *ctrl_target = ctk_color_controls->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    gint history, color_space_nvctrl, color_space;

    history = ctk_drop_down_menu_get_current_value(menu);

    color_space_nvctrl = ctk_color_controls->color_space_table[history];

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_COLOR_SPACE,
                       color_space_nvctrl);

    color_space = map_nvctrl_value_to_table(ctk_color_controls,
                                            color_space_nvctrl);

    post_color_space_update(ctk_color_controls, color_space);

    if (!ctk_color_controls->current_color_attributes_supported) {
        /*
         * If querying the current color space isn't supported, fall back to
         * displaying the preferred color space as the current color space.
         */
        update_current_color_space_text(ctk_color_controls, color_space_nvctrl);
    }
} /* color_space_menu_changed() */


/*
 * ctk_color_controls_reset() - Resets the color range and
 * & color space when Reset HW Defaults is clicked
 */
void ctk_color_controls_reset(CtkColorControls *ctk_color_controls)
{
    CtrlTarget *ctrl_target;

    if (!ctk_color_controls) {
        return;
    }

    ctrl_target = ctk_color_controls->ctrl_target;

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_COLOR_SPACE,
                       NV_CTRL_COLOR_SPACE_RGB);

    NvCtrlSetAttribute(ctrl_target,
                       NV_CTRL_COLOR_RANGE,
                       NV_CTRL_COLOR_RANGE_FULL);

    ctk_color_controls_setup(ctk_color_controls);
} /* ctk_color_controls_reset() */


/*
 * add_color_controls_help() -
 */
void add_color_controls_help(CtkColorControls *ctk_color_controls,
                             GtkTextBuffer *b,
                             GtkTextIter *i)
{
    if (!ctk_color_controls) {
        return;
    }

    ctk_help_heading(b, i, _("Color Controls"));
    ctk_help_para(b, i, "%s", _(__color_controls_help));

    ctk_help_term(b, i, _("Color Space"));
    ctk_help_para(b, i, "%s", _(__color_space_help));

    ctk_help_term(b, i, _("Color Range"));
    ctk_help_para(b, i, "%s", _(__color_range_help));
} /* add_color_controls_help() */


/*
 * When other client updated color controls
 * we should update the GUI to reflect the current color range
 * and color space.
 */
static void color_control_update_received(GObject *object,
                                          CtrlEvent *event,
                                          gpointer user_data)
{
    CtkColorControls *ctk_object = CTK_COLOR_CONTROLS(user_data);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    ctk_color_controls_setup(ctk_object);

    /* update status bar message */
    switch (event->int_attr.attribute) {
    case NV_CTRL_CURRENT_COLOR_RANGE:
        post_current_color_range_update(ctk_object, event->int_attr.value);
        break;
    case NV_CTRL_CURRENT_COLOR_SPACE:
        post_current_color_space_update(ctk_object, event->int_attr.value);
        break;
    case NV_CTRL_COLOR_RANGE:
        post_color_range_update(ctk_object, event->int_attr.value);
        break;
    case NV_CTRL_COLOR_SPACE:
        post_color_space_update(ctk_object, event->int_attr.value);
        break;
    }
} /* color_control_update_received()  */


/*
 * build_color_space_table() - build a table of color space, showing
 * modes supported by the DFP.
 */
static gboolean build_color_space_table(CtkColorControls *ctk_color_controls,
                                        CtrlAttributeValidValues valid)
{
    gint i, n = 0, color_space_count = 0;
    gint mask = valid.allowed_ints;

    if (valid.valid_type != CTRL_ATTRIBUTE_VALID_TYPE_INT_BITS) {
        return False;
    }

    /* count no. of supported color space */
    while(mask) {
        mask = mask & (mask - 1);
        color_space_count++;
    }

    ctk_color_controls->color_space_table_size = color_space_count;
    ctk_color_controls->color_space_table =
        calloc(color_space_count, sizeof(ctk_color_controls->color_space_table[0]));
    if (!ctk_color_controls->color_space_table) {
        return False;
    }

    for (i = 0, n = 0; n < ctk_color_controls->color_space_table_size; i++) {
        if (valid.allowed_ints & (1 << i)) {
            ctk_color_controls->color_space_table[n] = i;
            n++;
        }
    }

    return True;

} /* build_color_space_table() */

static gint map_nvctrl_value_to_table(CtkColorControls *ctk_color_controls,
                                      gint val)
{
    int i;
    for (i = 0; i < ctk_color_controls->color_space_table_size; i++) {
        if (val == ctk_color_controls->color_space_table[i]) {
            return i;
        }
    }

    return 0;
} /*map_nvctrl_value_to_table() */
