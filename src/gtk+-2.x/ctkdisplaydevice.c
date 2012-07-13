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
#include <string.h>

#include "ctkbanner.h"

#include "ctkdisplaydevice.h"

#include "ctkditheringcontrols.h"
#include "ctkcolorcontrols.h"
#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#include <stdio.h>

static void ctk_display_device_class_init(CtkDisplayDeviceClass *);
static void ctk_display_device_finalize(GObject *);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void update_device_info(CtkDisplayDevice *ctk_object);

static void display_device_setup(CtkDisplayDevice *ctk_object);

static void callback_link_changed(GtkObject *object, gpointer arg1,
                                  gpointer user_data);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data);


#define FRAME_PADDING 5

static const char *__info_help =
"This section describes basic information about the connection to the display "
"device.";

static const char *__info_chip_location_help =
"Report whether the display device is driven by the on-chip controller "
"(internal), or a separate controller chip elsewhere on the graphics "
"board (external).";

static const char *__info_link_help =
"For DVI connections, reports whether the specified display device is "
"driven by a single link or dual link connection. For DisplayPort "
"connections, reports the bandwidth of the connection.";

static const char *__info_signal_help =
"Report whether the flat panel is driven by an LVDS, TMDS, or DisplayPort "
"signal.";

static const char * __native_res_help =
"The Native Resolution is the width and height in pixels that the display "
"device uses to display the image.  All other resolutions must be scaled "
"to this resolution by the GPU and/or the device's built-in scaler.";

static const char * __refresh_rate_help =
"The refresh rate displays the rate at which the screen is currently "
"refreshing the image.";

GType ctk_display_device_get_type(void)
{
    static GType ctk_object_type = 0;

    if (!ctk_object_type) {
        static const GTypeInfo ctk_object_info = {
            sizeof (CtkDisplayDeviceClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_display_device_class_init,
            NULL, /* class_finalize, */
            NULL, /* class_data */
            sizeof (CtkDisplayDevice),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_object_type = g_type_register_static(GTK_TYPE_VBOX,
                                                 "CtkDisplayDevice",
                                                 &ctk_object_info, 0);
    }

    return ctk_object_type;
}

static void ctk_display_device_class_init(
    CtkDisplayDeviceClass *ctk_object_class
)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_display_device_finalize;
}

static void ctk_display_device_finalize(
    GObject *object
)
{
    CtkDisplayDevice *ctk_object = CTK_DISPLAY_DEVICE(object);
    g_free(ctk_object->name);
    g_signal_handlers_disconnect_matched(ctk_object->ctk_event,
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}


/*
 * ctk_display_device_new() - constructor for the dissplay device page.
 */

GtkWidget* ctk_display_device_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config,
                                  CtkEvent *ctk_event,
                                  char *name,
                                  char *typeBaseName)
{
    GObject *object;
    CtkDisplayDevice *ctk_object;
    GtkWidget *banner;
    GtkWidget *hbox, *tmpbox;

    GtkWidget *alignment;
    GtkWidget *notebook;
    GtkWidget *nbox;
    GtkWidget *align;
    GtkWidget *label;
    GtkWidget *hseparator;
    GtkWidget *button;
    gchar *str;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE, NULL);
    if (!object) return NULL;

    ctk_object = CTK_DISPLAY_DEVICE(object);
    ctk_object->handle = handle;
    ctk_object->ctk_event = ctk_event;
    ctk_object->ctk_config = ctk_config;
    ctk_object->name = g_strdup(name);

    gtk_box_set_spacing(GTK_BOX(object), 10);

    /* Banner */

    if (strcmp(typeBaseName, "CRT") == 0) {
        banner = ctk_banner_image_new(BANNER_ARTWORK_CRT);
    } else {
        banner = ctk_banner_image_new(BANNER_ARTWORK_DFP);
    }
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

    /* Reset button */

    button = gtk_button_new_with_label("Reset Hardware Defaults");
    str = ctk_help_create_reset_hardware_defaults_text(typeBaseName,
                                                       name);
    ctk_config_set_tooltip(ctk_config, button, str);
    ctk_object->reset_button = button;

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_end(GTK_BOX(object), alignment, FALSE, FALSE, 0);


    /* Create tabbed notebook for widget */

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(object), notebook, TRUE, TRUE, 0);

    /* Create first tab for device info */

    nbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(nbox), FRAME_PADDING);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nbox,
                             gtk_label_new("Information"));


    /* Device info */

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(nbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Display Device Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);


    /* create the hbox to store device info */

    hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(nbox), hbox, FALSE, FALSE, FRAME_PADDING);

    /*
     * insert a vbox between the frame and the widgets, so that the
     * widgets don't expand to fill all of the space within the
     * frame
     */
    
    tmpbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(tmpbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(hbox), tmpbox);

    /* Make the txt widgets that will get updated */
    ctk_object->txt_chip_location = gtk_label_new("");
    ctk_object->txt_link = gtk_label_new("");
    ctk_object->txt_signal = gtk_label_new("");
    ctk_object->txt_native_resolution = gtk_label_new("");
    ctk_object->txt_refresh_rate = gtk_label_new("");

    /* Add information widget lines */
    {
        typedef struct {
            GtkWidget *label;
            GtkWidget *txt;
            const gchar *tooltip;
        } TextLineInfo;

        TextLineInfo lines[] = {
            {
                gtk_label_new("Chip location:"),
                ctk_object->txt_chip_location,
                __info_chip_location_help
            },
            {
                gtk_label_new("Connection link:"),
                ctk_object->txt_link,
                __info_link_help
            },
            {
                gtk_label_new("Signal:"),
                ctk_object->txt_signal,
                __info_signal_help
            },
            {
                gtk_label_new("Native Resolution:"),
                ctk_object->txt_native_resolution,
                __native_res_help,
            },
            {
                gtk_label_new("Refresh Rate:"),
                ctk_object->txt_refresh_rate,
                __refresh_rate_help,
            },
            { NULL, NULL, NULL }
        };
        int i;

        GtkRequisition req;
        int max_width;

        /* Compute max width of lables and setup text alignments */
        max_width = 0;
        for (i = 0; lines[i].label; i++) {
            gtk_misc_set_alignment(GTK_MISC(lines[i].label), 0.0f, 0.5f);
            gtk_misc_set_alignment(GTK_MISC(lines[i].txt), 0.0f, 0.5f);

            gtk_widget_size_request(lines[i].label, &req);
            if (max_width < req.width) {
                max_width = req.width;
            }
        }

        /* Pack labels */
        for (i = 0; lines[i].label; i++) {
            GtkWidget *tmphbox;

            /* Add separators */
            if (i == 3 || i == 5 || i == 7) {
                GtkWidget *separator = gtk_hseparator_new();
                gtk_box_pack_start(GTK_BOX(tmpbox), separator,
                                   FALSE, FALSE, 0);
            }

            /* Set the label's width */
            gtk_widget_set_size_request(lines[i].label, max_width, -1);

            /* add the widgets for this line */
            tmphbox = gtk_hbox_new(FALSE, FRAME_PADDING);
            gtk_box_pack_start(GTK_BOX(tmphbox), lines[i].label,
                               FALSE, TRUE, FRAME_PADDING);
            gtk_box_pack_start(GTK_BOX(tmphbox), lines[i].txt,
                               FALSE, TRUE, FRAME_PADDING);

            /* Include tooltips */
            if (lines[i].tooltip) {
                ctk_config_set_tooltip(ctk_config, 
                                       lines[i].label, 
                                       lines[i].tooltip);
                ctk_config_set_tooltip(ctk_config, 
                                       lines[i].txt, 
                                       lines[i].tooltip);
            }
            gtk_box_pack_start(GTK_BOX(tmpbox), tmphbox, FALSE, FALSE, 0);
        }
    }

    /* pack the EDID button */

    ctk_object->edid = ctk_edid_new(ctk_object->handle,
                                    ctk_object->ctk_config,
                                    ctk_object->ctk_event,
                                    ctk_object->name);

    hbox = gtk_hbox_new(FALSE, 0);
    align = gtk_alignment_new(0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(align), hbox);
    gtk_box_pack_end(GTK_BOX(nbox), align, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_object->edid, TRUE, TRUE, 0);

    /*
     * Create layout for second tab for controls but don't add the tab until we
     * make sure its required
     */

    nbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(nbox), FRAME_PADDING);

    /* pack the color controls */

    ctk_object->color_controls =
        ctk_color_controls_new(handle, ctk_config, ctk_event,
                               ctk_object->reset_button, name);

    if (ctk_object->color_controls) {
        gtk_box_pack_start(GTK_BOX(nbox), ctk_object->color_controls,
                           FALSE, FALSE, 0);
    }

    /* pack the dithering controls */

    ctk_object->dithering_controls =
        ctk_dithering_controls_new(handle, ctk_config, ctk_event,
                                   ctk_object->reset_button, name);

    if (ctk_object->dithering_controls) {
        gtk_box_pack_start(GTK_BOX(nbox), ctk_object->dithering_controls,
                           FALSE, FALSE, 0);
    }

    /* pack the image sliders */

    ctk_object->image_sliders =
        ctk_image_sliders_new(handle, ctk_config, ctk_event,
                              ctk_object->reset_button, name);
    if (ctk_object->image_sliders) {
        gtk_box_pack_start(GTK_BOX(nbox), ctk_object->image_sliders,
                           FALSE, FALSE, 0);
    }

    /* If no controls are created, don't add a controls tab */

    if (ctk_object->color_controls ||
        ctk_object->dithering_controls ||
        ctk_object->image_sliders) {
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), nbox,
                                 gtk_label_new("Controls"));
    }

    /* Update the GUI */

    gtk_widget_show_all(GTK_WIDGET(object));
    display_device_setup(ctk_object);

    /* Listen to events */

    g_signal_connect(G_OBJECT(ctk_object->reset_button),
                     "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_object);

    if (ctk_object->txt_link) {
        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_FLATPANEL_LINK),
                         G_CALLBACK(callback_link_changed),
                         (gpointer) ctk_object);

        g_signal_connect(G_OBJECT(ctk_event),
                         CTK_EVENT_NAME(NV_CTRL_DISPLAYPORT_LINK_RATE),
                         G_CALLBACK(callback_link_changed),
                         (gpointer) ctk_object);
    }

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_received),
                     (gpointer) ctk_object);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_REFRESH_RATE),
                     G_CALLBACK(info_update_received),
                     (gpointer) ctk_object);

    return GTK_WIDGET(object);

} /* ctk_display_device_new() */



/*
 * reset_button_clicked() - callback when the reset button is clicked
 */

static void reset_button_clicked(GtkButton *button, gpointer user_data)
{
    CtkDisplayDevice *ctk_object = CTK_DISPLAY_DEVICE(user_data);

    /* Disable the reset button here and allow the controls below to (re)enable
     * it if need be,.
     */
    gtk_widget_set_sensitive(ctk_object->reset_button, FALSE);

    ctk_color_controls_reset(CTK_COLOR_CONTROLS(ctk_object->color_controls));

    ctk_dithering_controls_reset
	(CTK_DITHERING_CONTROLS(ctk_object->dithering_controls));

    ctk_image_sliders_reset(CTK_IMAGE_SLIDERS(ctk_object->image_sliders));

    ctk_config_statusbar_message(ctk_object->ctk_config,
                                 "Reset hardware defaults for %s.",
                                 ctk_object->name);

} /* reset_button_clicked() */



/*
 * ctk_display_device_create_help() - construct the display device help page
 */

GtkTextBuffer *ctk_display_device_create_help(GtkTextTagTable *table,
                                              CtkDisplayDevice *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    GtkTooltipsData *td;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "%s Help", ctk_object->name);

    ctk_help_heading(b, &i, "Device Information");
    ctk_help_para(b, &i, __info_help);

    ctk_help_term(b, &i, "Chip Location");
    ctk_help_para(b, &i, __info_chip_location_help);

    ctk_help_term(b, &i, "Link");
    ctk_help_para(b, &i, __info_link_help);

    ctk_help_term(b, &i, "Signal");
    ctk_help_para(b, &i, __info_signal_help);

    ctk_help_term(b, &i, "Native Resolution");
    ctk_help_para(b, &i, __native_res_help);

    ctk_help_term(b, &i, "Refresh Rate");
    ctk_help_para(b, &i, __refresh_rate_help);

    add_acquire_edid_help(b, &i);

    add_color_controls_help
        (CTK_COLOR_CONTROLS(ctk_object->color_controls), b, &i);

    add_dithering_controls_help
        (CTK_DITHERING_CONTROLS(ctk_object->dithering_controls), b, &i);

    add_image_sliders_help
        (CTK_IMAGE_SLIDERS(ctk_object->image_sliders), b, &i);

    td = gtk_tooltips_data_get(GTK_WIDGET(ctk_object->reset_button));
    ctk_help_reset_hardware_defaults(b, &i, td->tip_text);

    ctk_help_finish(b);

    return b;

} /* ctk_display_device_create_help() */


static void update_link(CtkDisplayDevice *ctk_object)
{
    ReturnStatus ret;
    gint val, signal_type = ctk_object->signal_type;
    const char *link = "Unknown";
    char tmp[32];

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_FLATPANEL_LINK, &val);
    if (ret == NvCtrlSuccess) {
        if (signal_type == NV_CTRL_FLATPANEL_SIGNAL_DISPLAYPORT) {
            int lanes;

            lanes = val + 1;

            ret = NvCtrlGetAttribute(ctk_object->handle,
                                     NV_CTRL_DISPLAYPORT_LINK_RATE, &val);
            if ((ret == NvCtrlSuccess) &&
                (val == NV_CTRL_DISPLAYPORT_LINK_RATE_DISABLED)) {
                link = "Disabled";
            } else {
                if (ret != NvCtrlSuccess) {
                    val = 0;
                }

                if (val > 0) {
                    snprintf(tmp, 32, "%d lane%s @ %.2f Gbps", lanes, lanes == 1 ? "" : "s",
                             val * 0.27);
                } else {
                    snprintf(tmp, 32, "%d lane%s @ unknown bandwidth", lanes,
                             lanes == 1 ? "" : "s");
                }
                link = tmp;
            }
        } else {
            // LVDS or TMDS
            switch(val) {
            case NV_CTRL_FLATPANEL_LINK_SINGLE:
                link = "Single";
                break;
            case NV_CTRL_FLATPANEL_LINK_DUAL:
                link = "Dual";
                break;
            }
        }
    }

    gtk_label_set_text(GTK_LABEL(ctk_object->txt_link), link);
}



/*
 * update_device_info() - (Re)Queries the static display device information.
 */
static void update_device_info(CtkDisplayDevice *ctk_object)
{
    ReturnStatus ret;
    gint val;
    gchar *str;

    /* Chip location */

    str = "Unknown";
    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_FLATPANEL_CHIP_LOCATION, &val);
    if (ret == NvCtrlSuccess) {
        switch (val) {
        case NV_CTRL_FLATPANEL_CHIP_LOCATION_INTERNAL:
            str = "Internal";
            break;
        case NV_CTRL_FLATPANEL_CHIP_LOCATION_EXTERNAL:
            str = "External";
            break;
        }
    }
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_chip_location), str);

    /* Signal */

    str = "Unknown";
    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_FLATPANEL_SIGNAL,
                             &val);
    if (ret == NvCtrlSuccess) {
        switch (val) {
        case NV_CTRL_FLATPANEL_SIGNAL_LVDS:
            str = "LVDS";
            break;
        case NV_CTRL_FLATPANEL_SIGNAL_TMDS:
            str = "TMDS";
            break;
        case NV_CTRL_FLATPANEL_SIGNAL_DISPLAYPORT:
            str = "DisplayPort";
            break;
        }
    }
    gtk_label_set_text(GTK_LABEL(ctk_object->txt_signal), str);
    ctk_object->signal_type = val;

    /* Link */

    update_link(ctk_object);

    /* Native Resolution */

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_FLATPANEL_NATIVE_RESOLUTION, &val);
    if (ret == NvCtrlSuccess) {
        str = g_strdup_printf("%dx%d", (val >> 16), (val & 0xFFFF));
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_native_resolution),
                           str);
        g_free(str);
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_native_resolution),
                           "Unknown");
    }

    /* Refresh Rate */

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_REFRESH_RATE, &val);
    if (ret == NvCtrlSuccess) {
        float fvalue = ((float)(val)) / 100.0f;
        str = g_strdup_printf("%.2f Hz", fvalue);
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_refresh_rate), str);
        g_free(str);
    } else {
        gtk_label_set_text(GTK_LABEL(ctk_object->txt_refresh_rate), "Unknown");
    }

} /* update_device_info() */



/*
 * Updates the display device page to reflect the current
 * configuration of the display device.
 */
static void display_device_setup(CtkDisplayDevice *ctk_object)
{
    /* Disable the reset button here and allow the controls below to (re)enable
     * it if need be,.
     */
    gtk_widget_set_sensitive(ctk_object->reset_button, FALSE);

    update_display_enabled_flag(ctk_object->handle,
                                &ctk_object->display_enabled);

    /* Update info */

    update_device_info(ctk_object);

    ctk_edid_setup(CTK_EDID(ctk_object->edid));

    /* Update controls */

    ctk_color_controls_setup(CTK_COLOR_CONTROLS(ctk_object->color_controls));

    ctk_dithering_controls_setup
        (CTK_DITHERING_CONTROLS(ctk_object->dithering_controls));

    ctk_image_sliders_setup(CTK_IMAGE_SLIDERS(ctk_object->image_sliders));

} /* display_device_setup() */



static void callback_link_changed(GtkObject *object, gpointer arg1,
                                  gpointer user_data)
{
    CtkDisplayDevice *ctk_object = CTK_DISPLAY_DEVICE(user_data);

    update_link(ctk_object);
}



/*
 * When the list of enabled displays on the GPU changes,
 * this page should disable/enable access based on whether
 * or not the display device is enabled.
 */
static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data)
{
    CtkDisplayDevice *ctk_object = CTK_DISPLAY_DEVICE(user_data);

    /* Requery display information only if display disabled */

    display_device_setup(ctk_object);

} /* enabled_displays_received() */


/*
 * Update UI when display information changed.
 */
static void info_update_received(GtkObject *object, gpointer arg1,
                                 gpointer user_data)
{
    CtkDisplayDevice *ctk_object = CTK_DISPLAY_DEVICE(user_data);

    update_device_info(ctk_object);
}
