/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004,2012 NVIDIA Corporation.
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
#include <stdio.h>
#include <stdlib.h>

#include "ctkbanner.h"

#include "ctkdisplaydevice.h"

#include "ctkditheringcontrols.h"
#include "ctkcolorcontrols.h"
#include "ctkimagesliders.h"
#include "ctkedid.h"
#include "ctkcolorcorrection.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"

static void ctk_display_device_class_init(CtkDisplayDeviceClass *);
static void ctk_display_device_finalize(GObject *);

static void reset_button_clicked(GtkButton *button, gpointer user_data);

static void update_device_info(CtkDisplayDevice *ctk_object);

static void display_device_setup(CtkDisplayDevice *ctk_object);

static void enabled_displays_received(GtkObject *object, gpointer arg1,
                                      gpointer user_data);

static void callback_link_changed(GtkObject *object, gpointer arg1,
                                  gpointer user_data);

static void callback_refresh_rate_changed(GtkObject *object, gpointer arg1,
                                          gpointer user_data);

static gboolean update_guid_info(InfoEntry *entry);
static gboolean update_tv_encoder_info(InfoEntry *entry);
static gboolean update_chip_info(InfoEntry *entry);
static gboolean update_signal_info(InfoEntry *entry);
static gboolean update_link_info(InfoEntry *entry);
static gboolean update_refresh_rate(InfoEntry *entry);

static gboolean register_link_events(InfoEntry *entry);
static gboolean unregister_link_events(InfoEntry *entry);
static gboolean register_refresh_rate_events(InfoEntry *entry);
static gboolean unregister_refresh_rate_events(InfoEntry *entry);

static void add_color_correction_tab(CtkDisplayDevice *ctk_object,
                                     CtkConfig *ctk_config,
                                     CtkEvent *ctk_event,
                                     GtkWidget *notebook,
                                     ParsedAttribute *p);

#define FRAME_PADDING 5

static const char *__info_help =
"This section describes basic information about the connection to the display "
"device.";

static const char*__guid_help =
"The Global Unique Identifier for the display port display device.";

static const char* __tv_encoder_name_help =
"The TV Encoder name displays the name of the TV Encoder.";

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

static const char * __refresh_rate_help =
"The refresh rate displays the rate at which the screen is currently "
"refreshing the image.";

typedef gboolean (*InfoEntryFunc)(InfoEntry *entry);

typedef struct {
    const char *str;
    const gchar **tooltip;
    InfoEntryFunc update_func; /* return FALSE if not present */
    InfoEntryFunc register_events_func;
    InfoEntryFunc unregister_events_func;
} InfoEntryData;

static InfoEntryData __info_entry_data[] = {
    {
        "GUID",
        &__guid_help,
        update_guid_info,
        NULL,
        NULL,
    },
    {
        "TV Encoder",
        &__tv_encoder_name_help,
        update_tv_encoder_info,
        NULL,
        NULL,
    },
    {
        "Chip Location",
        &__info_chip_location_help,
        update_chip_info,
        NULL,
        NULL,
    },
    {
        "Signal",
        &__info_signal_help,
        update_signal_info,
        NULL,
        NULL,
    },
    {
        "Connection link",
        &__info_link_help,
        update_link_info,
        register_link_events,
        unregister_link_events,
    },
    {
        "Refresh Rate",
        &__refresh_rate_help,
        update_refresh_rate,
        register_refresh_rate_events,
        unregister_refresh_rate_events,
    },
};

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
    int i;

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event_gpu),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);

    for (i = 0; i < ctk_object->num_info_entries; i++) {
        InfoEntryData *entryData = &__info_entry_data[i];
        InfoEntry *entry = &ctk_object->info_entries[i];

        if (entryData->unregister_events_func) {
            entryData->unregister_events_func(entry);
        }
    }

    g_free(ctk_object->name);
}


/*
 * ctk_display_device_new() - constructor for the dissplay device page.
 */

GtkWidget* ctk_display_device_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config,
                                  CtkEvent *ctk_event,
                                  CtkEvent *ctk_event_gpu,
                                  char *name,
                                  char *typeBaseName,
                                  ParsedAttribute *p)
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
    int i;

    object = g_object_new(CTK_TYPE_DISPLAY_DEVICE, NULL);
    if (!object) return NULL;

    ctk_object = CTK_DISPLAY_DEVICE(object);
    ctk_object->handle = handle;
    ctk_object->ctk_event = ctk_event;
    ctk_object->ctk_event_gpu = ctk_event_gpu;
    ctk_object->ctk_config = ctk_config;
    ctk_object->name = g_strdup(name);
    ctk_object->color_correction_available = FALSE;

    gtk_box_set_spacing(GTK_BOX(object), 10);

    /* Banner */

    if (strcmp(typeBaseName, "CRT") == 0) {
        banner = ctk_banner_image_new(BANNER_ARTWORK_CRT);
    } else {
        banner = ctk_banner_image_new(BANNER_ARTWORK_DFP);
    }
    gtk_box_pack_start(GTK_BOX(object), banner, FALSE, FALSE, 0);

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

    /* Create and add the information widgets */

    ctk_object->num_info_entries = ARRAY_LEN(__info_entry_data);
    ctk_object->info_entries = calloc(ctk_object->num_info_entries,
                                      sizeof(InfoEntry));
    if (!ctk_object->info_entries) {
        ctk_object->num_info_entries = 0;
    }

    for (i = 0; i < ctk_object->num_info_entries; i++) {
        InfoEntryData *entryData = __info_entry_data+i;
        InfoEntry *entry = ctk_object->info_entries+i;
        gchar *str;

        entry->ctk_object = ctk_object;
        str = g_strconcat(entryData->str, ":", NULL);
        entry->label = gtk_label_new(str);
        g_free(str);

        entry->txt = gtk_label_new("");

        gtk_misc_set_alignment(GTK_MISC(entry->label), 0.0f, 0.5f);
        gtk_misc_set_alignment(GTK_MISC(entry->txt), 0.0f, 0.5f);

        ctk_config_set_tooltip(ctk_config,
                               entry->label,
                               *(entryData->tooltip));
        ctk_config_set_tooltip(ctk_config,
                               entry->txt,
                               *(entryData->tooltip));

        entry->hbox = gtk_hbox_new(FALSE, FRAME_PADDING);
        gtk_box_pack_start(GTK_BOX(entry->hbox), entry->label,
                           FALSE, TRUE, FRAME_PADDING);
        gtk_box_pack_start(GTK_BOX(entry->hbox), entry->txt,
                           FALSE, TRUE, FRAME_PADDING);

        gtk_box_pack_start(GTK_BOX(tmpbox), entry->hbox, FALSE, FALSE, 0);
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
     * make sure it's required
     */

    nbox = gtk_vbox_new(FALSE, FRAME_PADDING);
    gtk_container_set_border_width(GTK_CONTAINER(nbox), FRAME_PADDING);

    /* pack the reset button */

    button = gtk_button_new_with_label("Reset Hardware Defaults");
    str = ctk_help_create_reset_hardware_defaults_text(typeBaseName,
                                                       name);
    ctk_config_set_tooltip(ctk_config, button, str);
    ctk_object->reset_button = button;

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_box_pack_end(GTK_BOX(nbox), alignment, FALSE, FALSE, 0);

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

    /*
     * Show all widgets on this page so far. After this, the color correction
     * tab and other widgets can control their own visibility.
     */

    gtk_widget_show_all(GTK_WIDGET(object));

    /* add the color correction tab if RandR is available */

    add_color_correction_tab(ctk_object, ctk_config, ctk_event, notebook, p);

    /* Update the GUI */

    display_device_setup(ctk_object);

    /* Listen to events */

    g_signal_connect(G_OBJECT(ctk_object->reset_button),
                     "clicked", G_CALLBACK(reset_button_clicked),
                     (gpointer) ctk_object);

    g_signal_connect(G_OBJECT(ctk_event_gpu),
                     CTK_EVENT_NAME(NV_CTRL_ENABLED_DISPLAYS),
                     G_CALLBACK(enabled_displays_received),
                     (gpointer) ctk_object);

    for (i = 0; i < ctk_object->num_info_entries; i++) {
        InfoEntryData *entryData = __info_entry_data+i;
        InfoEntry *entry = ctk_object->info_entries+i;

        if (entryData->register_events_func) {
            entryData->register_events_func(entry);
        }
    }

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
    int j;

    b = gtk_text_buffer_new(table);

    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "%s Help", ctk_object->name);

    ctk_help_heading(b, &i, "Device Information");
    ctk_help_para(b, &i, "%s", __info_help);

    for (j = 0; j < ARRAY_LEN(__info_entry_data); j++) {
        InfoEntryData *entryData = __info_entry_data+j;
        InfoEntry *entry = ctk_object->info_entries+j;

        if (entry->present) {
            ctk_help_term(b, &i, "%s", entryData->str);
            ctk_help_para(b, &i, "%s", *entryData->tooltip);
        }
    }

    add_acquire_edid_help(b, &i);

    add_color_controls_help
        (CTK_COLOR_CONTROLS(ctk_object->color_controls), b, &i);

    add_dithering_controls_help
        (CTK_DITHERING_CONTROLS(ctk_object->dithering_controls), b, &i);

    add_image_sliders_help
        (CTK_IMAGE_SLIDERS(ctk_object->image_sliders), b, &i);

    if (ctk_object->color_correction_available) {
        ctk_color_correction_tab_help(b, &i, "X Server Color Correction", TRUE);
    }

    td = gtk_tooltips_data_get(GTK_WIDGET(ctk_object->reset_button));
    ctk_help_reset_hardware_defaults(b, &i, td->tip_text);

    ctk_help_finish(b);

    return b;

} /* ctk_display_device_create_help() */



static gboolean update_guid_info(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    char *str;

    ret = NvCtrlGetStringDisplayAttribute(ctk_object->handle, 0,
                                          NV_CTRL_STRING_DISPLAY_NAME_DP_GUID, &str);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    gtk_label_set_text(GTK_LABEL(entry->txt), str);

    XFree(str);

    return TRUE;
}



static gboolean update_tv_encoder_info(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    char *str;

    ret = NvCtrlGetStringDisplayAttribute(ctk_object->handle, 0,
                                          NV_CTRL_STRING_TV_ENCODER_NAME, &str);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    gtk_label_set_text(GTK_LABEL(entry->txt), str);

    XFree(str);

    return TRUE;
}



static gboolean update_chip_info(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    gint val;
    const gchar *str;

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_FLATPANEL_CHIP_LOCATION, &val);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    switch (val) {
    case NV_CTRL_FLATPANEL_CHIP_LOCATION_INTERNAL:
        str = "Internal";
        break;
    case NV_CTRL_FLATPANEL_CHIP_LOCATION_EXTERNAL:
        str = "External";
        break;
    default:
        str = "Unknown";
        break;
    }

    gtk_label_set_text(GTK_LABEL(entry->txt), str);

    return TRUE;
}



static gboolean update_signal_info(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    gint val;
    const char *str;

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_FLATPANEL_SIGNAL,
                             &val);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

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
    default:
        str = "Unknown";
        break;
    }

    gtk_label_set_text(GTK_LABEL(entry->txt), str);

    ctk_object->signal_type = val;

    return TRUE;
}


/* NOTE: Link information is dependent on signal type, and this function
 * assumes the signal type is queried first.
 */
static gboolean update_link_info(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    gint val;
    const char *link;
    char tmp[32];

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_FLATPANEL_LINK, &val);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    if (ctk_object->signal_type == NV_CTRL_FLATPANEL_SIGNAL_DISPLAYPORT) {
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
        default:
            link = "Unknown";
            break;
        }
    }

    gtk_label_set_text(GTK_LABEL(entry->txt), link);

    return TRUE;
}



static gboolean update_refresh_rate(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;
    ReturnStatus ret;
    gint val;
    gboolean hdmi3D;
    char *str;
    float fvalue;

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_REFRESH_RATE, &val);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_DPY_HDMI_3D, &hdmi3D);
    if (ret != NvCtrlSuccess) {
        return FALSE;
    }

    fvalue = ((float)(val)) / 100.0f;

    if (hdmi3D) {
        fvalue /= 2;
    }

    str = g_strdup_printf("%.2f Hz%s", fvalue, hdmi3D ? " (HDMI 3D)" : "");

    gtk_label_set_text(GTK_LABEL(entry->txt), str);
    g_free(str);

    return TRUE;
}



/*
 * update_device_info() - (Re)Queries the static display device information.
 */
static void update_device_info(CtkDisplayDevice *ctk_object)
{
    int i;
    int max_width;
    GtkRequisition req;


    max_width = 0;
    for (i = 0; i < ctk_object->num_info_entries; i++) {
        InfoEntryData *entryData = __info_entry_data+i;
        InfoEntry *entry = ctk_object->info_entries+i;

        entry->present = entryData->update_func(entry);

        if (entry->present) {
            gtk_widget_show(entry->hbox);
            gtk_widget_size_request(entry->label, &req);
            if (max_width < req.width) {
                max_width = req.width;
            }
        } else {
            gtk_widget_hide(entry->hbox);
        }
    }

    for (i = 0; i < ctk_object->num_info_entries; i++) {
        InfoEntry *entry = ctk_object->info_entries+i;
        if (entry->present) {
            gtk_widget_set_size_request(entry->label, max_width, -1);
        }
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



static gboolean register_link_events(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;

    g_signal_connect(G_OBJECT(ctk_object->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_FLATPANEL_LINK),
                     G_CALLBACK(callback_link_changed),
                     (gpointer) entry);

    g_signal_connect(G_OBJECT(ctk_object->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DISPLAYPORT_LINK_RATE),
                     G_CALLBACK(callback_link_changed),
                     (gpointer) entry);

    return TRUE;
}

static gboolean unregister_link_events(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0, /* signal_id */
                                         0, /* detail */
                                         NULL, /* closure */
                                         NULL, /* func */
                                         (gpointer) entry);
    return TRUE;
}

static gboolean register_refresh_rate_events(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;

    g_signal_connect(G_OBJECT(ctk_object->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_REFRESH_RATE),
                     G_CALLBACK(callback_refresh_rate_changed),
                     (gpointer) entry);
    return TRUE;
}

static gboolean unregister_refresh_rate_events(InfoEntry *entry)
{
    CtkDisplayDevice *ctk_object = entry->ctk_object;

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0, /* signal_id */
                                         0, /* detail */
                                         NULL, /* closure */
                                         NULL, /* func */
                                         (gpointer) entry);
    return TRUE;
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


static void callback_link_changed(GtkObject *object, gpointer arg1,
                                  gpointer user_data)
{
    InfoEntry *entry = (InfoEntry *)user_data;

    update_link_info(entry);
}


static void callback_refresh_rate_changed(GtkObject *object, gpointer arg1,
                                          gpointer user_data)
{
    InfoEntry *entry = (InfoEntry *)user_data;

    update_refresh_rate(entry);
}


static void add_color_correction_tab(CtkDisplayDevice *ctk_object,
                                     CtkConfig *ctk_config,
                                     CtkEvent *ctk_event,
                                     GtkWidget *notebook,
                                     ParsedAttribute *p)
{
    ReturnStatus ret;
    gint val;
    GtkWidget *ctk_color_correction;
    GtkWidget *box;

    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE, &val);

    if (ret != NvCtrlSuccess) {
        return;
    }

    if (val != 1) {
        return;
    }

    ctk_color_correction = ctk_color_correction_new(ctk_object->handle,
                                                    ctk_config,
                                                    p,
                                                    ctk_event);
    if (ctk_color_correction == NULL) {
        return;
    }
    ctk_object->color_correction_available = TRUE;

    box = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(box), FRAME_PADDING);
    gtk_box_pack_start(GTK_BOX(box), ctk_color_correction, TRUE, TRUE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box,
                             gtk_label_new("Color Correction"));
    gtk_widget_show(box);
}
