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

#include "NvCtrlAttributes.h"
#include "NVCtrlLib.h"

#include "ctkbanner.h"
#include "ctk3dvisionpro.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/*
 * Icons.
 */
#include "svp_battery_0.h"
#include "svp_battery_25.h"
#include "svp_battery_50.h"
#include "svp_battery_75.h"
#include "svp_battery_100.h"
#include "svp_status_nosignal.h"
#include "svp_status_verylow.h"
#include "svp_status_low.h"
#include "svp_status_good.h"
#include "svp_status_verygood.h"
#include "svp_status_excellent.h"
#include "svp_add_glasses.h"

//-----------------------------------------------------------------------------

#define HTU(idx)                 (ctk_3d_vision_pro->htu_info[(idx)])
#define MAX_ATTRIB_LENGTH        128

#define PAIRING_TIMEOUT          3
#define PAIRING_DURATION         60
#define POLL_PAIRING_TIMEOUT     2000 //mS
#define POLL_PAIRING_CYCLE       ((POLL_PAIRING_TIMEOUT / 1000) * 2)

#define CHANNEL_RANGE_TO_OPTION_MENU_IDX(range) ((range) - 1)
#define OPTION_MENU_IDX_TO_CHANNEL_RANGE(menu)  ((menu) + 1)

enum {
    CHANGED,
    LAST_SIGNAL
};

typedef struct _RenameGlassesDlg {
    GtkWidget *parent;
    GtkWidget *mnu_glasses_name;
    GtkWidget *dlg_rename_glasses;
    int        glasses_selected_index;
    char      *glasses_new_name;
} RenameGlassesDlg;

typedef struct _IdentifyGlassesDlg {
    GtkWidget *parent;
    GtkWidget *mnu_glasses_name;
    GtkWidget *dlg_identify_glasses;
    int        glasses_selected_index;
} IdentifyGlassesDlg;

typedef struct _ChannelRangeDlg {
    GtkWidget *parent;
    GtkWidget *dlg_channel_range;
} ChannelRangeDlg;

typedef struct _SelectChannelDlg {
    GtkWidget *parent;
    GtkWidget *dlg_select_channel;
} SelectChannelDlg;

typedef struct _RemoveGlassesDlg {
    GtkWidget *parent;
    GtkWidget *dlg_remove_glasses;
    GtkWidget *mnu_glasses_name;
    int        glasses_selected_index;
} RemoveGlassesDlg;

typedef void (*BUTTON_CLICK)(GtkButton *button, gpointer user_data);

static void channel_range_changed(GtkWidget *widget, gpointer user_data);

//-----------------------------------------------------------------------------

static guint __signals[LAST_SIGNAL] = { 0 };

const char *__mnu_glasses_name_tooltip = "Select glasses name";
const char *__goggle_info_tooltip      = "Displays the list of glasses synced "
    "to the hub and their battery levels";
const char *__channel_range_tooltip    = "Change the 3D Vision Pro Hub range. "
    "Click the arrow and then select the hub range that you want.";
const char *__add_glasses_tooltip      = "Add more glasses to sync to the hub. "
    "Click this button to open the Add glasses dialog that lets you synchronize "
    "another pair of stereo glasses with the hub.";
const char *__refresh_tooltip          = "Updates the list of glasses that are "
    "synchronized with the hub.";
const char *__identify_tooltip         = "Identify a pair of glasses. "
    "Causes the LED on the selected pair of glasses to blink.";
const char *__rename_tooltip           = "Rename a pair of glasses. "
    "Opens the Rename glasses dialog that lets you assign a different name to "
    "the selected pair of glasses.";
const char *__remove_glasses_tooltip   = "Remove a pair of glasses currently "
    "synced to the hub. This removes the selected pair of glasses from the "
    "glasses information table and disconnects the glasses from the hub.";

/******************************************************************************
 *
 * Various helper and Widget callback functions
 *
 ******************************************************************************/

static const char** get_battery_status_icon(int battery)
{
    if (battery == 0) {
        return (const char **)&svp_battery_0_xpm;
    } else if (battery < 50) {
        return (const char **)&svp_battery_25_xpm;
    } else if (battery < 75) {
        return (const char **)&svp_battery_50_xpm;
    } else if (battery < 100) {
        return (const char **)&svp_battery_75_xpm;
    } else if (battery == 100) {
        return (const char **)&svp_battery_100_xpm;
    }

    return NULL;
}

static const char** get_signal_strength_icon(int signal_strength)
{
    if (signal_strength == 0) {
        return (const char **)&svp_status_nosignal_xpm;
    } else if (signal_strength < 25) {
        return (const char **)&svp_status_verylow_xpm;
    } else if (signal_strength < 50) {
        return (const char **)&svp_status_low_xpm;
    } else if (signal_strength < 75) {
        return (const char **)&svp_status_good_xpm;
    } else if (signal_strength < 100) {
        return (const char **)&svp_status_verygood_xpm;
    } else if (signal_strength == 100) {
        return (const char **)&svp_status_excellent_xpm;
    }

    return NULL;
}



static void ctk_3d_vision_pro_finalize(GObject *object)
{
    Ctk3DVisionPro *ctk_object = CTK_3D_VISION_PRO(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_object->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         (gpointer) ctk_object);
}



static void ctk_3d_vision_pro_class_init(Ctk3DVisionProClass
                                         *ctk_3d_vision_pro_class)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_3d_vision_pro_class;
    gobject_class->finalize = ctk_3d_vision_pro_finalize;
    
    __signals[CHANGED] = g_signal_new("changed",
                       G_OBJECT_CLASS_TYPE(ctk_3d_vision_pro_class),
                       G_SIGNAL_RUN_LAST,
                       G_STRUCT_OFFSET(Ctk3DVisionProClass, changed),
                       NULL, NULL, g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}



GType ctk_3d_vision_pro_get_type(void)
{
    static GType ctk_3d_vision_pro_type = 0;

    if (!ctk_3d_vision_pro_type) {
        static const GTypeInfo ctk_3d_vision_pro_info = {
            sizeof (Ctk3DVisionProClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_3d_vision_pro_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (Ctk3DVisionPro),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_3d_vision_pro_type = g_type_register_static(GTK_TYPE_VBOX,
                                 "Ctk3DVisionPro", &ctk_3d_vision_pro_info, 0);
    }

    return ctk_3d_vision_pro_type;
}

static GtkWidget *add_button(char *label, BUTTON_CLICK handler,
                             Ctk3DVisionPro *ctk_3d_vision_pro,
                             GtkWidget *pack_in, const char *tooltip)
{
    GtkWidget *button;
    GtkWidget *hbox;
    GtkWidget *alignment;

    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new_with_label(label);
    gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(handler),
                     (gpointer) ctk_3d_vision_pro);
    ctk_config_set_tooltip(ctk_3d_vision_pro->ctk_config, button, tooltip);

    alignment = gtk_alignment_new(0, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_box_pack_start(GTK_BOX(pack_in), alignment, TRUE, TRUE, 0);

    return button;
}

static GtkWidget * add_label(char *text, GtkWidget *pack_in)
{
    GtkWidget *hbox;
    GtkWidget *alignment;
    GtkWidget *label;

    hbox = gtk_hbox_new(FALSE, 5);
    alignment = gtk_alignment_new(0, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);
    label = gtk_label_new(text);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_box_pack_start(GTK_BOX(pack_in), hbox, FALSE, FALSE, 0);

    return label;
}

static void glasses_name_changed(GtkWidget *widget,
                                 gpointer user_data)
{
    RemoveGlassesDlg *dlg = (RemoveGlassesDlg *)user_data;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    dlg->glasses_selected_index =
        ctk_drop_down_menu_get_current_value(menu);
}

static GtkWidget *create_glasses_list_menu(Ctk3DVisionPro *ctk_3d_vision_pro,
                                           GlassesInfo **glasses_info,
                                           guint num_glasses, gpointer dlg)
{
    CtkDropDownMenu *mnu_glasses_name;
    int i;

    mnu_glasses_name = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    g_signal_connect(G_OBJECT(mnu_glasses_name), "changed",
                     G_CALLBACK(glasses_name_changed),
                     (gpointer) dlg);

    ctk_config_set_tooltip(ctk_3d_vision_pro->ctk_config,
                           GTK_WIDGET(mnu_glasses_name),
                           __mnu_glasses_name_tooltip);

    g_signal_handlers_block_by_func(G_OBJECT(mnu_glasses_name),
                                    G_CALLBACK(glasses_name_changed),
                                    (gpointer) dlg);
    
    for (i = 0; i < num_glasses; i++) {
        ctk_drop_down_menu_append_item(mnu_glasses_name,
                                       glasses_info[i]->name, i);
    }

    /* Setup the menu and select the glasses name */

    ctk_drop_down_menu_set_current_value(mnu_glasses_name, 0);

    /* If dropdown has only one item, disable menu selection */
    if (num_glasses > 1) {
        gtk_widget_set_sensitive(GTK_WIDGET(mnu_glasses_name), True);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(mnu_glasses_name), False);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(mnu_glasses_name),
         G_CALLBACK(glasses_name_changed), (gpointer) dlg);

    return GTK_WIDGET(mnu_glasses_name);
}

static const char *new_glasses_name_activate(GtkWidget *widget,
                                             gpointer user_data)
{
    RenameGlassesDlg *dlg = (RenameGlassesDlg *)user_data;
    const gchar *str = gtk_entry_get_text(GTK_ENTRY(widget));

    // Store new glasses name in dialog box
    dlg->glasses_new_name = realloc(dlg->glasses_new_name, strlen(str) + 1);
    strncpy(dlg->glasses_new_name, str, strlen(str) + 1);

    return str;
}

static gboolean new_glasses_name_focus_out(GtkWidget *widget, GdkEvent *event,
                                           gpointer user_data)
{
    new_glasses_name_activate(widget, user_data);

    return FALSE;

} /* display_panning_focus_out() */

static void update_glasses_info_data_table(GlassesInfoTable *table, GlassesInfo** glasses_info)
{
    int i;
    GtkRequisition req;

    if (table->rows > 0) {
        gtk_table_resize(GTK_TABLE(table->data_table), table->rows, table->columns);
    }

    for (i = 0; i < table->rows; i++) {
        char str[NUM_GLASSES_INFO_ATTRIBS + 1][MAX_ATTRIB_LENGTH];
        int cell = 0;

        snprintf((char *)(&(str[cell++])), MAX_ATTRIB_LENGTH, "%s", glasses_info[i]->name);
        snprintf((char *)(&(str[cell++])), MAX_ATTRIB_LENGTH, "%d", glasses_info[i]->battery);

        // destroy older widgets
        if (glasses_info[i]->image) {
            gtk_widget_destroy (glasses_info[i]->image);
        }
        for (cell = 0; cell < NUM_GLASSES_INFO_ATTRIBS; cell++) {
            if (glasses_info[i]->label[cell]) {
                gtk_widget_destroy (glasses_info[i]->label[cell]);
            }
            if (glasses_info[i]->hbox[cell]) {
                gtk_widget_destroy (glasses_info[i]->hbox[cell]);
            }
        }

        for (cell = 0; cell < NUM_GLASSES_INFO_ATTRIBS; cell++) {
            GtkWidget *hbox;
            GtkWidget * label;
            GtkWidget *image;

            hbox = gtk_hbox_new(FALSE, 0);
            glasses_info[i]->hbox[cell] = hbox;

            label = gtk_label_new(str[cell]);
            glasses_info[i]->label[cell] = label;
            gtk_label_set_justify( GTK_LABEL(label), GTK_JUSTIFY_CENTER);
            gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
            gtk_table_attach(GTK_TABLE(table->data_table), hbox,
                             cell, cell+1,
                             i+1, i+2,
                             GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

            if (cell == 1) {
                const char **bat_icon =  get_battery_status_icon(glasses_info[i]->battery);
                GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data(bat_icon);
                image = gtk_image_new_from_pixbuf(pixbuf);
                glasses_info[i]->image = image;
                gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
            }
            /* Make sure the table headers are the same width
             * as their table data column
             */
            gtk_widget_size_request(label, &req);

            if (table->glasses_header_sizes[cell].width > req.width ) {
                gtk_widget_set_size_request(label,
                                            table->glasses_header_sizes[cell].width,
                                            -1);
            } else if (table->glasses_header_sizes[cell].width < req.width ) {
                table->glasses_header_sizes[cell].width = req.width + 6;
                gtk_widget_set_size_request(table->glasses_header_sizes[cell].widget,
                                            table->glasses_header_sizes[cell].width,
                                            -1);
            }
        }
    }
}

static void create_glasses_info_table(GlassesInfoTable *table, GlassesInfo** glasses_info,
                                      GtkWidget *pack_in, CtkConfig *ctk_config)
{
    GtkWidget *hbox, *hbox1;
    GtkWidget *vbox;
    GtkWidget *label;
    GtkWidget *hseparator;
    GtkWidget *data_viewport, *full_viewport;
    GtkWidget *vscrollbar, *hscrollbar, *vpan;
    GtkWidget *data_table, *header_table;
    GtkRequisition req;
    GtkWidget *event;    /* For setting the background color to white */
    gchar *goggle_info_titles[NUM_GLASSES_INFO_ATTRIBS] =
           {"Glasses Name", "Battery Level (%)"};
    int i;

    /* Create clist in a scroll box */
    hbox1 = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Glasses Information");
    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), hseparator, TRUE, TRUE, 5);

    hbox  = gtk_hbox_new(FALSE, 0);
    vbox  = gtk_vbox_new(FALSE, 5);
    vpan  = gtk_vpaned_new();

    data_viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_size_request(data_viewport, 250, 100);
    vscrollbar = gtk_vscrollbar_new(gtk_viewport_get_vadjustment
                                    (GTK_VIEWPORT(data_viewport)));

    full_viewport = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_size_request(full_viewport, 300, 150);
    hscrollbar = gtk_hscrollbar_new(gtk_viewport_get_hadjustment
                                    (GTK_VIEWPORT(full_viewport)));

    table->data_viewport = data_viewport;
    table->full_viewport = full_viewport;
    table->vscrollbar = vscrollbar;
    table->hscrollbar = hscrollbar;

    /* Create the header table */
    header_table = gtk_table_new(1, NUM_GLASSES_INFO_ATTRIBS, FALSE);
    for ( i = 0; i < NUM_GLASSES_INFO_ATTRIBS; i++ ) {
        GtkWidget * btn  = gtk_button_new_with_label(goggle_info_titles[i]);
        ctk_config_set_tooltip(ctk_config, btn, __goggle_info_tooltip);
        gtk_table_attach(GTK_TABLE(header_table), btn, i, i+1, 0, 1,
                         GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

        table->glasses_header_sizes[i].widget = btn;
        gtk_widget_size_request(btn, &req);
        table->glasses_header_sizes[i].width = req.width;
    }

    /* Create the data table */
    data_table = gtk_table_new(table->rows, table->columns, FALSE);
    event = gtk_event_box_new();
    ctk_force_text_colors_on_widget(event);
    gtk_container_add (GTK_CONTAINER(event), data_table);
    gtk_container_add(GTK_CONTAINER(data_viewport), event);

    /* Pack the glasses info header and data tables */
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), header_table, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data_viewport, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER(full_viewport), vbox);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), full_viewport, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hscrollbar, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

//    gtk_paned_pack1 (GTK_PANED (vpan), scrollWin, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (vpan), vbox, TRUE, FALSE);
    gtk_box_pack_start(GTK_BOX(pack_in), vpan, TRUE, TRUE, 0);

    /* Fill the data table */
    table->data_table = data_table;
    update_glasses_info_data_table(table, glasses_info);
}



static void init_glasses_info_widgets(GlassesInfo *glasses)
{
    int i;

    for (i = 0; i < NUM_GLASSES_INFO_ATTRIBS; i++) {
        glasses->label[i] = NULL;
        glasses->hbox[i]  = NULL;
    }
    glasses->image = NULL;
}

static void callback_glasses_paired(GObject *object,
                                    CtrlEvent *event,
                                    gpointer user_data)
{
    int battery_level;
    char *glasses_name = NULL;
    unsigned int glasses_id;
    GlassesInfo *glasses;
    ReturnStatus ret;
    char temp[64]; //scratchpad memory used to construct labels.
    int index;

    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    AddGlassesDlg  *dlg = ctk_3d_vision_pro->add_glasses_dlg;

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    glasses_id = event->int_attr.value;

    /* It is possible for the user to accidentally try pairing a glass even if 
     * it is already paired leading to multiple entries. To avoid this, return if 
     * the glass entry is already present in the local table.
     */ 

    if (dlg) {
        for (index = 0; index < dlg->new_glasses; index++) {
            if (dlg->glasses_info[index]->glasses_id == glasses_id) {
                return;
            }        
        }
    }

    ret = NvCtrlGetStringDisplayAttribute(ctrl_target, glasses_id,
                                          NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,
                                          &glasses_name);
    if (ret != NvCtrlSuccess) {
        glasses_name = NULL;
    }

    ret = NvCtrlGetDisplayAttribute(ctrl_target, glasses_id,
                                    NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL,
                                    (int *)&battery_level);
    if (ret != NvCtrlSuccess) {
        battery_level = 0;
    }
    // Create glasses_info
    glasses = (GlassesInfo *)malloc(sizeof(GlassesInfo));
    strncpy(glasses->name, glasses_name, sizeof(glasses->name));;
    glasses->name[sizeof(glasses->name) - 1] = '\0';
    glasses->battery = battery_level;
    glasses->glasses_id = glasses_id;
    init_glasses_info_widgets(glasses);

    if (dlg) {
        // add entry into local glasses_info structure.
        dlg->new_glasses++;

        dlg->glasses_info = (GlassesInfo **)realloc(dlg->glasses_info,
                             sizeof(GlassesInfo *) * dlg->new_glasses);

        dlg->glasses_info[dlg->new_glasses - 1] = glasses;

        // update dialog boxes glasses info table
        dlg->table.rows++;
        update_glasses_info_data_table(&(dlg->table), dlg->glasses_info);
        gtk_widget_show_all(GTK_WIDGET(dlg->table.data_table));
    }

    /* This is to avoid multiple entries of a glass's information 
     * being displayed by nvidia-settings. Return in case the glass 
     * entry is already present in the HTU table.
     */

    for (index = 0; index < HTU(0)->num_glasses; index++) {
        if (HTU(0)->glasses_info[index]->glasses_id == glasses_id) {
            return;
        }
    }

    // Add glasses_info into HTU(0)->glasses_info list.
    HTU(0)->glasses_info = realloc(HTU(0)->glasses_info,
                           (HTU(0)->num_glasses + 1) * sizeof(GlassesInfo *));

    HTU(0)->glasses_info[HTU(0)->num_glasses] = glasses;
    init_glasses_info_widgets(HTU(0)->glasses_info[HTU(0)->num_glasses]);
    HTU(0)->num_glasses += 1;

    // update glasses info table
    ctk_3d_vision_pro->table.rows += 1;
    update_glasses_info_data_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info);
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->table.data_table));

    snprintf(temp, sizeof(temp), "Glasses Connected: %d", HTU(0)->num_glasses);
    gtk_label_set_text(ctk_3d_vision_pro->glasses_num_label, temp);
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->glasses_num_label));

    free(glasses_name);
}

static void callback_glasses_unpaired(GObject *object,
                                      CtrlEvent *event,
                                      gpointer user_data)
{
    int j;
    unsigned int glasses_id = 0;
    GlassesInfo *glasses = NULL;
    char temp[64]; //scratchpad memory used to construct labels.
    int overwrite = FALSE;

    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }

    glasses_id = event->int_attr.value;

    for (j = 0; j < (HTU(0)->num_glasses); j++) {
        if (!overwrite && HTU(0)->glasses_info[j]->glasses_id == glasses_id) {
            if (j != HTU(0)->num_glasses - 1) {
                overwrite = TRUE;
            }
            glasses = HTU(0)->glasses_info[j];
        }
        if (overwrite && HTU(0)->num_glasses > 1) {
            HTU(0)->glasses_info[j] = HTU(0)->glasses_info[j + 1];
        }
    }
    
    HTU(0)->num_glasses--;
    HTU(0)->glasses_info = (GlassesInfo **)realloc(HTU(0)->glasses_info,
                            sizeof(GlassesInfo *) * HTU(0)->num_glasses);

    gtk_widget_destroy (glasses->label[0]);
    gtk_widget_destroy (glasses->label[1]);
    gtk_widget_destroy (glasses->image);
    gtk_widget_destroy (glasses->hbox[0]);
    gtk_widget_destroy (glasses->hbox[1]);

    // update glasses info table
    ctk_3d_vision_pro->table.rows--;
    update_glasses_info_data_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info);
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->table.data_table));

    snprintf(temp, sizeof(temp), "Glasses Connected: %d", HTU(0)->num_glasses);
    gtk_label_set_text(ctk_3d_vision_pro->glasses_num_label, temp);
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->glasses_num_label));

    free(glasses);
}

static gboolean poll_pairing(gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;

    if (ctk_3d_vision_pro->add_glasses_dlg->pairing_attempts >
        PAIRING_DURATION / POLL_PAIRING_CYCLE) {
        return FALSE;
    }

    if (ctk_3d_vision_pro->add_glasses_dlg->in_pairing) {
        /* Enable pairing for PAIRING_TIMEOUT seconds */
        NvCtrlSetAttribute(ctrl_target,
                           NV_CTRL_3D_VISION_PRO_PAIR_GLASSES,
                           PAIRING_TIMEOUT);
        XFlush(NvCtrlGetDisplayPtr(ctrl_target));
    }

    ctk_3d_vision_pro->add_glasses_dlg->in_pairing =
        !(ctk_3d_vision_pro->add_glasses_dlg->in_pairing);
    ctk_3d_vision_pro->add_glasses_dlg->pairing_attempts++;
    return TRUE;
}

static void enable_widgets(Ctk3DVisionPro *ctk_3d_vision_pro, Bool enable)
{
    gtk_widget_set_sensitive(ctk_3d_vision_pro->refresh_button, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->identify_button, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->rename_button, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->remove_button, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->table.data_viewport, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->table.full_viewport, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->table.vscrollbar, enable);
    gtk_widget_set_sensitive(ctk_3d_vision_pro->table.hscrollbar, enable);
}

static void svp_config_changed(GObject *object,
                               CtrlEvent *event,
                               gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro;
    CtrlTarget *ctrl_target;
    char temp[64];

    ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    ctrl_target = ctk_3d_vision_pro->ctrl_target;

    if (event->type == CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        gint value = event->int_attr.value;

        switch (event->int_attr.attribute) {
        case NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL:
            if (HTU(0)->channel_num != value) {
                HTU(0)->channel_num = value;
                snprintf(temp, sizeof(temp), "%d", HTU(0)->channel_num);
                gtk_label_set_text(ctk_3d_vision_pro->channel_num_label, temp);
                gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->channel_num_label));
            }
            break;
        case NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE:
            {
                SVP_RANGE range;
                CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(ctk_3d_vision_pro->menu);
                range = ctk_drop_down_menu_get_current_value(menu);

                if (range != CHANNEL_RANGE_TO_OPTION_MENU_IDX(value)) {
                    g_signal_handlers_block_by_func(ctk_3d_vision_pro->menu,
                        channel_range_changed, ctk_3d_vision_pro);
                    HTU(0)->channel_range = value;
                    ctk_drop_down_menu_set_current_value(menu,
                        CHANNEL_RANGE_TO_OPTION_MENU_IDX(value));
                    enable_widgets(ctk_3d_vision_pro,
                        (HTU(0)->channel_range == SVP_LONG_RANGE ? FALSE : TRUE));
                    g_signal_handlers_unblock_by_func(ctk_3d_vision_pro->menu,
                        channel_range_changed, ctk_3d_vision_pro);
                }
                break;
            }
        case NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY:
            if (HTU(0)->signal_strength != value) {
                const char **signal_strength_icon;
                GdkPixbuf *pixbuf;

                HTU(0)->signal_strength = value;
                snprintf(temp, sizeof(temp), "[%d%%]", HTU(0)->signal_strength);
                gtk_label_set_text(ctk_3d_vision_pro->signal_strength_label, temp);

                signal_strength_icon = get_signal_strength_icon(HTU(0)->signal_strength);
                pixbuf = gdk_pixbuf_new_from_xpm_data(signal_strength_icon);
                gtk_image_set_from_pixbuf(GTK_IMAGE(ctk_3d_vision_pro->signal_strength_image), pixbuf);

                gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->signal_strength_label));
                gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->signal_strength_image));
            }
            break;
        default:
            ;
        }
    }
    else if (event->type == CTRL_EVENT_TYPE_STRING_ATTRIBUTE) {
        switch (event->str_attr.attribute) {
        case NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME:
            {
                int i;
                for (i = 0; i < HTU(0)->num_glasses; i++) {
                    ReturnStatus ret;
                    char *glasses_name = NULL;
                    GlassesInfo *glasses = glasses = HTU(0)->glasses_info[i];

                    ret = NvCtrlGetStringDisplayAttribute(ctrl_target,
                                                          glasses->glasses_id,
                                                          NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,
                                                          &glasses_name);

                    if (ret != NvCtrlSuccess || glasses_name == NULL) {
                        continue;
                    }
                    strncpy(glasses->name, glasses_name, sizeof(glasses->name));
                    glasses->name[sizeof(glasses->name)-1] = '\0';
                    free(glasses_name);
                }
                update_glasses_info_data_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info);
                gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->table.data_table));
                break;
            }
        default:
            ;
        }
    }
}

/******************************************************************************
 *
 * Create dialog boxes and Button click callbacks
 *
 ******************************************************************************/

static void refresh_button_clicked(GtkButton *button, gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    int i;
    char temp[64];
    const char **signal_strength_icon;
    GdkPixbuf *pixbuf;
    Bool ret;

    for (i = 0; i < HTU(0)->num_glasses; i++) {
        int battery_level;
        GlassesInfo *glasses = glasses = HTU(0)->glasses_info[i];

        ret = NvCtrlGetDisplayAttribute(ctrl_target,
                                        glasses->glasses_id,
                                        NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL,
                                        (int *)&battery_level);
        if (ret != NvCtrlSuccess) {
            battery_level = 0;
        }

        glasses->battery = battery_level;
    }
    update_glasses_info_data_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info);
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->table.data_table));

    ret = NvCtrlGetDisplayAttribute(ctrl_target,
                                    HTU(0)->channel_num,
                                    NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY,
                                    (int *)&(HTU(0)->signal_strength));
    if (ret != TRUE) {
        HTU(0)->signal_strength = 0;
    }
    snprintf(temp, sizeof(temp), "[%d%%]", HTU(0)->signal_strength);
    gtk_label_set_text(ctk_3d_vision_pro->signal_strength_label, temp);

    signal_strength_icon = get_signal_strength_icon(HTU(0)->signal_strength);
    pixbuf = gdk_pixbuf_new_from_xpm_data(signal_strength_icon);
    gtk_image_set_from_pixbuf(GTK_IMAGE(ctk_3d_vision_pro->signal_strength_image), pixbuf);

    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->signal_strength_label));
    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->signal_strength_image));
}

//=============================================================================

static AddGlassesDlg *create_add_glasses_dlg(Ctk3DVisionPro *ctk_3d_vision_pro)
{
    AddGlassesDlg *dlg;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *image;
    GtkWidget *parent = GTK_WIDGET(ctk_3d_vision_pro);

    dlg = (AddGlassesDlg *)malloc(sizeof(AddGlassesDlg));
    if (dlg == NULL) {
        return NULL;
    }

    dlg->parent = parent;
    dlg->new_glasses = 0;
    dlg->glasses_info = NULL;
    dlg->in_pairing = TRUE;
    dlg->pairing_attempts = 0;

    /* Create the dialog */
    dlg->dlg_add_glasses = gtk_dialog_new_with_buttons
        ("Add glasses",
         ctk_3d_vision_pro->parent_wnd,
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_SAVE,
         GTK_RESPONSE_ACCEPT,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_add_glasses),
                                    GTK_RESPONSE_REJECT);

    label = gtk_label_new("1. Press button on the glasses\n"
                          "   to initiate the connection.");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses))),
         hbox, TRUE, TRUE, 5);

    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses))),
         hbox, TRUE, TRUE, 5);

    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(
                svp_add_glasses_xpm));
    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses))),
                       image, FALSE, FALSE, 0);

    label = gtk_label_new("2. List of glasses connected:");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses))),
         hbox, TRUE, TRUE, 5);

    //generate menu option for glasses' name
    hbox = gtk_hbox_new(TRUE, 0);

    // create glasses info table
    dlg->table.rows = 0;
    dlg->table.columns = NUM_GLASSES_INFO_ATTRIBS;
    create_glasses_info_table(&(dlg->table), dlg->glasses_info, hbox,
                              ctk_3d_vision_pro->ctk_config);

    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses))),
         hbox, TRUE, TRUE, 5);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_add_glasses)));

    return dlg;
}

static void add_glasses_button_clicked(GtkButton *button, gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    AddGlassesDlg *dlg;
    gint result;
    int i;
    char *s;

    dlg = create_add_glasses_dlg(ctk_3d_vision_pro);
    if (dlg == NULL) {
        return;
    }

    ctk_3d_vision_pro->add_glasses_dlg = dlg;

    if (HTU(0)->channel_range == SVP_LONG_RANGE) {
        gtk_widget_set_sensitive(dlg->table.data_viewport, FALSE);
        gtk_widget_set_sensitive(dlg->table.full_viewport, FALSE);
        gtk_widget_set_sensitive(dlg->table.vscrollbar, FALSE);
        gtk_widget_set_sensitive(dlg->table.hscrollbar, FALSE);
    }
 
    s = g_strdup_printf("NVIDIA 3D VisionPro Pairing");
    ctk_config_add_timer(ctk_3d_vision_pro->ctk_config,
                         POLL_PAIRING_TIMEOUT,
                         s,
                         (GSourceFunc) poll_pairing,
                         (gpointer)ctk_3d_vision_pro);
    g_free(s);

    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_add_glasses),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_window_resize(GTK_WINDOW(dlg->dlg_add_glasses), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_add_glasses),
                             FALSE);

    ctk_config_start_timer(ctk_3d_vision_pro->ctk_config,
                           (GSourceFunc) poll_pairing,
                           (gpointer) ctk_3d_vision_pro);

    gtk_widget_show(dlg->dlg_add_glasses);
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_add_glasses));
    gtk_widget_hide(dlg->dlg_add_glasses);

    ctk_config_stop_timer(ctk_3d_vision_pro->ctk_config,
                          (GSourceFunc) poll_pairing,
                          (gpointer)ctk_3d_vision_pro);

    /* Handle user's response */
    switch (result) {
    case GTK_RESPONSE_ACCEPT:
        break;
    default:
        for (i = 0; i < dlg->new_glasses; i++) {
            NvCtrlSetAttribute(ctrl_target,
                               NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES,
                               dlg->glasses_info[i]->glasses_id);
        }
        break;
    }

    free(dlg->glasses_info);
    free(dlg);
    ctk_3d_vision_pro->add_glasses_dlg = NULL;
}

//=============================================================================

static RemoveGlassesDlg *create_remove_glasses_dlg(Ctk3DVisionPro *ctk_3d_vision_pro)
{
    RemoveGlassesDlg *dlg;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *parent = GTK_WIDGET(ctk_3d_vision_pro);

    dlg = (RemoveGlassesDlg *)malloc(sizeof(RemoveGlassesDlg));
    if (dlg == NULL) {
        return NULL;
    }

    dlg->parent = parent;

    /* Create the dialog */
    dlg->dlg_remove_glasses = gtk_dialog_new_with_buttons
        ("Remove glasses",
         ctk_3d_vision_pro->parent_wnd,
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_OK,
         GTK_RESPONSE_OK,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_remove_glasses),
                                    GTK_RESPONSE_REJECT);

    label = gtk_label_new("Remove glasses synced to this hub:");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);

    //generate menu option for glasses' name
    dlg->mnu_glasses_name = create_glasses_list_menu(ctk_3d_vision_pro,
                                HTU(0)->glasses_info, HTU(0)->num_glasses,
                                (gpointer)dlg);
    gtk_box_pack_start(GTK_BOX(hbox), dlg->mnu_glasses_name,
                       TRUE, TRUE, 0);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_remove_glasses))),
         hbox, TRUE, TRUE, 20);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_remove_glasses)));

    return dlg;
}

static void remove_button_clicked(GtkButton *button, gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    CtkDropDownMenu *menu;
    RemoveGlassesDlg *dlg;
    gint result;

    dlg = create_remove_glasses_dlg(ctk_3d_vision_pro);

    if (dlg == NULL) {
        return;
    }

    dlg->glasses_selected_index = -1;
    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_remove_glasses),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_window_resize(GTK_WINDOW(dlg->dlg_remove_glasses), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_remove_glasses),
                              FALSE);

    gtk_widget_show(dlg->dlg_remove_glasses);
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_remove_glasses));
    gtk_widget_hide(dlg->dlg_remove_glasses);

    /* Handle user's response */
    switch (result) {
    case GTK_RESPONSE_OK:
        menu = CTK_DROP_DOWN_MENU(dlg->mnu_glasses_name);
        dlg->glasses_selected_index = ctk_drop_down_menu_get_current_value(menu);

        if (dlg->glasses_selected_index >= 0 &&
            dlg->glasses_selected_index < HTU(0)->num_glasses) {
            unsigned int glasses_id = HTU(0)->glasses_info[dlg->glasses_selected_index]->glasses_id;

            NvCtrlSetAttribute(ctrl_target,
                               NV_CTRL_3D_VISION_PRO_UNPAIR_GLASSES,
                               glasses_id);
        }
        break;
    default:
        /* do nothing. */
        break;
    }

    free(dlg);
}

//=============================================================================

static IdentifyGlassesDlg *create_identify_glasses_dlg(Ctk3DVisionPro *ctk_3d_vision_pro)
{
    IdentifyGlassesDlg *dlg;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *parent = GTK_WIDGET(ctk_3d_vision_pro);

    dlg = (IdentifyGlassesDlg *)malloc(sizeof(IdentifyGlassesDlg));
    if (dlg == NULL) {
        return NULL;
    }

    dlg->parent = parent;

    /* Create the dialog */
    dlg->dlg_identify_glasses = gtk_dialog_new_with_buttons
        ("Identify glasses",
         ctk_3d_vision_pro->parent_wnd,
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_OK,
         GTK_RESPONSE_OK,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_identify_glasses),
                                    GTK_RESPONSE_REJECT);

    label = gtk_label_new("Identify selected glasses:");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);

    /* Select glasses to identify */
    dlg->mnu_glasses_name = create_glasses_list_menu(ctk_3d_vision_pro,
                                HTU(0)->glasses_info, HTU(0)->num_glasses,
                                (gpointer)dlg);
    gtk_box_pack_start(GTK_BOX(hbox), dlg->mnu_glasses_name,
                       TRUE, TRUE, 0);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_identify_glasses))),
         hbox, TRUE, TRUE, 20);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_identify_glasses)));

    return dlg;
}

static void identify_button_clicked(GtkButton *button, gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    IdentifyGlassesDlg *dlg;
    gint result;
    unsigned int glasses_id;

    dlg = create_identify_glasses_dlg(ctk_3d_vision_pro);

    if (dlg == NULL) {
        return;
    }

    dlg->glasses_selected_index = -1;
    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_identify_glasses),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_window_resize(GTK_WINDOW(dlg->dlg_identify_glasses), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_identify_glasses),
                              FALSE);

    gtk_widget_show(dlg->dlg_identify_glasses);
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_identify_glasses));
    gtk_widget_hide(dlg->dlg_identify_glasses);

    /* Handle user's response */
    switch (result) {
    case GTK_RESPONSE_OK:
        dlg->glasses_selected_index =
            ctk_drop_down_menu_get_current_value(CTK_DROP_DOWN_MENU(dlg->mnu_glasses_name));

        if (dlg->glasses_selected_index >= 0 &&
            dlg->glasses_selected_index < HTU(0)->num_glasses) {
            glasses_id = HTU(0)->glasses_info[dlg->glasses_selected_index]->glasses_id;

            NvCtrlSetAttribute(ctrl_target,
                               NV_CTRL_3D_VISION_PRO_IDENTIFY_GLASSES,
                               glasses_id);
        }
        break;
    default:
        /* do nothing. */
        break;
    }

    free(dlg);
}

//=============================================================================

static RenameGlassesDlg *create_rename_glasses_dlg(Ctk3DVisionPro *ctk_3d_vision_pro)
{
    RenameGlassesDlg *dlg;
    GtkWidget *label;
    GtkWidget *hbox;
    GtkWidget *new_glasses_name;
    const char *__new_glasses_name_tooltip = "Add new glasses name";

    GtkWidget *parent = GTK_WIDGET(ctk_3d_vision_pro);
    dlg = (RenameGlassesDlg *)malloc(sizeof(RenameGlassesDlg));
    if (dlg == NULL) {
        return NULL;
    }

    dlg->parent = parent;

    /* Create the dialog */
    dlg->dlg_rename_glasses = gtk_dialog_new_with_buttons
        ("Rename glasses",
         ctk_3d_vision_pro->parent_wnd,
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_SAVE,
         GTK_RESPONSE_ACCEPT,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_REJECT,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_rename_glasses),
                                    GTK_RESPONSE_REJECT);


    label = gtk_label_new("Name:");
    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);

    /* Select glasses to remove */
    dlg->mnu_glasses_name = create_glasses_list_menu(ctk_3d_vision_pro,
                                HTU(0)->glasses_info, HTU(0)->num_glasses,
                                (gpointer)dlg);
    gtk_box_pack_start(GTK_BOX(hbox), dlg->mnu_glasses_name,
                       TRUE, TRUE, 0);

    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_rename_glasses))),
         hbox, TRUE, TRUE, 20);

    new_glasses_name = gtk_entry_new();
    ctk_config_set_tooltip(ctk_3d_vision_pro->ctk_config, new_glasses_name,
                           __new_glasses_name_tooltip);
    g_signal_connect(G_OBJECT(new_glasses_name), "activate",
                     G_CALLBACK(new_glasses_name_activate),
                     (gpointer) dlg);
    g_signal_connect(G_OBJECT(new_glasses_name), "focus-out-event",
                     G_CALLBACK(new_glasses_name_focus_out),
                     (gpointer) dlg);

    gtk_box_pack_start(GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_rename_glasses))),
                       new_glasses_name, TRUE, TRUE, 0);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_rename_glasses)));

    return dlg;
}

static void rename_button_clicked(GtkButton *button, gpointer user_data)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    RenameGlassesDlg *dlg;
    gint result;

    dlg = create_rename_glasses_dlg(ctk_3d_vision_pro);

    if (dlg == NULL) {
        return;
    }

    dlg->glasses_new_name = NULL;

    dlg->glasses_selected_index = -1;
    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_rename_glasses),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_window_resize(GTK_WINDOW(dlg->dlg_rename_glasses), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_rename_glasses),
                             FALSE);

    gtk_widget_show(dlg->dlg_rename_glasses);

    do {
        result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_rename_glasses));

        if (result == GTK_RESPONSE_ACCEPT) {
            int i;
            dlg->glasses_selected_index = 
                ctk_drop_down_menu_get_current_value(CTK_DROP_DOWN_MENU(dlg->mnu_glasses_name));

            if (dlg->glasses_new_name == NULL || strlen(dlg->glasses_new_name) == 0) {
                continue;
            }

            for (i = 0; i < HTU(0)->num_glasses; i++) {
                if (!strncmp(dlg->glasses_new_name, HTU(0)->glasses_info[i]->name,
                    sizeof(HTU(0)->glasses_info[i]->name))) {
                    break;
                }
            }
            if (i == HTU(0)->num_glasses) {
                if (dlg->glasses_selected_index >= 0 &&
                    dlg->glasses_selected_index < HTU(0)->num_glasses) {
                    ReturnStatus ret;
                    unsigned int glasses_id = HTU(0)->glasses_info[dlg->glasses_selected_index]->glasses_id;

                    ret = NvCtrlSetStringDisplayAttribute(ctrl_target,
                                                          glasses_id,
                                                          NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,
                                                          dlg->glasses_new_name);
                    if (ret != NvCtrlSuccess) {
                        continue;
                    }
                    strncpy(HTU(0)->glasses_info[dlg->glasses_selected_index]->name, dlg->glasses_new_name,
                            sizeof(HTU(0)->glasses_info[dlg->glasses_selected_index]->name));
                    HTU(0)->glasses_info[dlg->glasses_selected_index]->name[GLASSES_NAME_MAX_LENGTH - 1] = '\0';

                    update_glasses_info_data_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info);
                    gtk_widget_show_all(GTK_WIDGET(ctk_3d_vision_pro->table.data_table));
                }
                break;
            }
        }

    } while (result != GTK_RESPONSE_REJECT);

    gtk_widget_hide(dlg->dlg_rename_glasses);

    free(dlg->glasses_new_name);
    free(dlg);
}

//=============================================================================

static ChannelRangeDlg *create_channel_range_change_dlg(Ctk3DVisionPro *ctk_3d_vision_pro,
                                                        SVP_RANGE range)
{
    ChannelRangeDlg *dlg;
    GtkWidget *label = NULL;
    GtkWidget *hbox;
    GtkWidget *parent = GTK_WIDGET(ctk_3d_vision_pro);

    dlg = (ChannelRangeDlg *)malloc(sizeof(ChannelRangeDlg));
    if (dlg == NULL) {
        return NULL;
    }

    dlg->parent = parent;

    /* Create the dialog */
    dlg->dlg_channel_range = gtk_dialog_new_with_buttons
        ("Modify Hub Range",
         ctk_3d_vision_pro->parent_wnd,
         GTK_DIALOG_MODAL |  GTK_DIALOG_DESTROY_WITH_PARENT,
         GTK_STOCK_YES,
         GTK_RESPONSE_YES,
         GTK_STOCK_NO,
         GTK_RESPONSE_NO,
         NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg->dlg_channel_range),
                                    GTK_RESPONSE_REJECT);

    switch (range) {
    case SVP_SHORT_RANGE:
        label = gtk_label_new("You have changed transceiver range to short range (less than 5m.).\n"
                              "Only glasses in this range will be available.\n\n"
                              "Do you want to apply changes?");
        break;
    case SVP_MEDIUM_RANGE:
        label = gtk_label_new("You have changed transceiver range to medium range (less than 15m.).\n"
                              "Only glasses in this range will be available.\n\n"
                              "Do you want to apply changes?");
        break;
    case SVP_LONG_RANGE:
        label = gtk_label_new("You have changed transceiver range to long range.\n\n"
                              "Do you want to apply changes?");
        break;
    }

    hbox = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 20);
    gtk_box_pack_start
        (GTK_BOX(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_channel_range))),
         hbox, TRUE, TRUE, 20);

    gtk_widget_show_all(ctk_dialog_get_content_area(GTK_DIALOG(dlg->dlg_channel_range)));

    return dlg;
}

static void channel_range_changed(
    GtkWidget *widget,
    gpointer user_data
)
{
    Ctk3DVisionPro *ctk_3d_vision_pro = CTK_3D_VISION_PRO(user_data);
    CtrlTarget *ctrl_target = ctk_3d_vision_pro->ctrl_target;
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    ChannelRangeDlg *dlg;
    gint result;
    SVP_RANGE range;
    SVP_RANGE prev_range;

    range = 
        OPTION_MENU_IDX_TO_CHANNEL_RANGE(ctk_drop_down_menu_get_current_value(menu));
    prev_range = HTU(0)->channel_range;

    if (HTU(0)->channel_range == range) {
        return;
    }

    dlg = create_channel_range_change_dlg(ctk_3d_vision_pro, range);

    gtk_window_set_transient_for
        (GTK_WINDOW(dlg->dlg_channel_range),
         GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dlg->parent))));

    gtk_window_resize(GTK_WINDOW(dlg->dlg_channel_range), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(dlg->dlg_channel_range),
                              FALSE);

    gtk_widget_show(dlg->dlg_channel_range);
    result = gtk_dialog_run(GTK_DIALOG(dlg->dlg_channel_range));
    gtk_widget_hide(dlg->dlg_channel_range);

    /* Handle user's response */
    switch (result) {
    case GTK_RESPONSE_YES:
        HTU(0)->channel_range = range;
        /* Send NV-Control command */
        NvCtrlSetAttribute(ctrl_target,
                           NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE,
                           (HTU(0)->channel_range));

        enable_widgets(ctk_3d_vision_pro, (HTU(0)->channel_range == SVP_LONG_RANGE ? FALSE : TRUE));

        break;
    case GTK_RESPONSE_NO:
        ctk_drop_down_menu_set_current_value(menu, 
                                             CHANNEL_RANGE_TO_OPTION_MENU_IDX(prev_range));
        break;
    default:
        /* do nothing. */
        break;
    }
    free(dlg);
}

//*****************************************************************************

GtkWidget* ctk_3d_vision_pro_new(CtrlTarget *ctrl_target,
                                 CtkConfig *ctk_config,
                                 ParsedAttribute *p,
                                 CtkEvent *ctk_event)
{
    GObject *object;
    Ctk3DVisionPro *ctk_3d_vision_pro;
    GtkWidget *mainhbox;
    GtkWidget *leftvbox;
    GtkWidget *rightvbox;
    GtkWidget *banner;
    GtkWidget *alignment;
    GtkWidget *vbox;
    GtkWidget *vbox1, *vbox2;
    GtkWidget *frame_vbox;
    GtkWidget *label;
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *hbox1;
    GtkWidget *hseparator;
    int i;
    GtkWidget *image;
    CtkDropDownMenu *menu;
    char temp[64]; //scratchpad memory used to construct labels.
    unsigned char *paired_glasses_list = NULL;
    int len;
    ReturnStatus ret;

    object = g_object_new(CTK_TYPE_3D_VISION_PRO, NULL);
    ctk_3d_vision_pro = CTK_3D_VISION_PRO(object);

    ctk_3d_vision_pro->ctrl_target = ctrl_target;
    ctk_3d_vision_pro->ctk_config = ctk_config;
    ctk_3d_vision_pro->ctk_event = ctk_event;
    ctk_3d_vision_pro->add_glasses_dlg = NULL;

    // populate ctk_3d_vision_pro...
    // query attributes and fill the data ..
    ctk_3d_vision_pro->num_htu = 1;
    ctk_3d_vision_pro->htu_info = (HtuInfo **) malloc(sizeof(HtuInfo *) *
                                  ctk_3d_vision_pro->num_htu);;
    for (i = 0; i < ctk_3d_vision_pro->num_htu; i++) {
        HtuInfo *htu = (HtuInfo*) malloc(sizeof(HtuInfo));
        HTU(i) = htu;
    }

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL,
                             (int *)&(HTU(0)->channel_num));
    if (ret != NvCtrlSuccess) {
        HTU(0)->channel_num = 0;
    }

    ret = NvCtrlGetDisplayAttribute(ctrl_target, HTU(0)->channel_num,
                                    NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY,
                                    (int *)&(HTU(0)->signal_strength));
    if (ret != NvCtrlSuccess) {
        HTU(0)->signal_strength = 0;
    }

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE,
                             (int *)&(HTU(0)->channel_range));
    if (ret != NvCtrlSuccess ||
        (!(HTU(0)->channel_range == SVP_SHORT_RANGE ||
          HTU(0)->channel_range == SVP_MEDIUM_RANGE ||
          HTU(0)->channel_range == SVP_LONG_RANGE))) {
        HTU(0)->channel_range = SVP_SHORT_RANGE;
    }

    ret = NvCtrlGetBinaryAttribute(ctrl_target, 0,
                                   NV_CTRL_BINARY_DATA_GLASSES_PAIRED_TO_3D_VISION_PRO_TRANSCEIVER,
                                   &paired_glasses_list, &len);

    if (ret != NvCtrlSuccess) {
        HTU(0)->num_glasses = 0;
        HTU(0)->glasses_info = NULL;
    } else {
        HTU(0)->num_glasses = ((unsigned int *)paired_glasses_list)[0];
        HTU(0)->glasses_info = (GlassesInfo **)malloc(sizeof(GlassesInfo *) *
                               HTU(0)->num_glasses);
    }

    for (i = 0; i < HTU(0)->num_glasses; i++) {
        int battery_level;
        char *glasses_name;
        GlassesInfo *glasses = (GlassesInfo *)malloc(sizeof(GlassesInfo));
        unsigned int glasses_id = ((unsigned int *)paired_glasses_list)[i+1];

        HTU(0)->glasses_info[i] = glasses;

        ret = NvCtrlGetStringDisplayAttribute(ctrl_target, glasses_id,
                                              NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME,
                                              &glasses_name);
        if (ret != NvCtrlSuccess) {
            glasses_name = NULL;
        }

        ret = NvCtrlGetDisplayAttribute(ctrl_target,  glasses_id,
                                        NV_CTRL_3D_VISION_PRO_GLASSES_BATTERY_LEVEL,
                                        (int *)&battery_level);
        if (ret != NvCtrlSuccess) {
            battery_level = 0;
        }

        strncpy(glasses->name, glasses_name, sizeof(glasses->name));
        glasses->name[sizeof(glasses->name) - 1] = '\0';
        glasses->battery = battery_level;
        glasses->glasses_id = glasses_id;
        init_glasses_info_widgets(glasses);
        free(glasses_name);
    }

//-----------------------------------------------------------------------------
//  Construct and display NVIDIA 3D VisionPro page

    gtk_box_set_spacing(GTK_BOX(ctk_3d_vision_pro), 5);

    /*
     * Banner: TOP - LEFT -> RIGHT
     *
     * This image serves as a visual reference for basic color_box correction
     * purposes.
     */

    banner = ctk_banner_image_new(BANNER_ARTWORK_SVP);
    gtk_box_pack_start(GTK_BOX(ctk_3d_vision_pro), banner, FALSE, FALSE, 0);

    mainhbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_3d_vision_pro), mainhbox,
                       FALSE, FALSE, 0);

//-----------------------------------------------------------------------------
//  left vertical box

    leftvbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(mainhbox), leftvbox, FALSE, FALSE, 0);

    frame = gtk_frame_new("Glasses");
    gtk_box_pack_start(GTK_BOX(leftvbox), frame, FALSE, FALSE, 0);

    
    frame_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(frame_vbox), 5);
    gtk_container_add(GTK_CONTAINER(frame), frame_vbox);
    alignment = gtk_alignment_new(0, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(frame_vbox), alignment, TRUE, TRUE, 0);

    snprintf(temp, sizeof(temp), "Glasses Connected: %d", HTU(0)->num_glasses);
    label = gtk_label_new(temp);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    ctk_3d_vision_pro->glasses_num_label = GTK_LABEL(label);


    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    alignment = gtk_alignment_new(0, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(frame_vbox), alignment, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);

    add_button("Add Glasses", add_glasses_button_clicked, ctk_3d_vision_pro,
               hbox, __add_glasses_tooltip);
    ctk_3d_vision_pro->refresh_button = 
        add_button("Refresh", refresh_button_clicked, ctk_3d_vision_pro,
                   hbox, __refresh_tooltip);
    ctk_3d_vision_pro->identify_button =
        add_button("Identify", identify_button_clicked, ctk_3d_vision_pro,
                   hbox, __identify_tooltip);
    ctk_3d_vision_pro->rename_button = 
        add_button("Rename", rename_button_clicked, ctk_3d_vision_pro,
                   hbox, __rename_tooltip);
    ctk_3d_vision_pro->remove_button =
        add_button("Remove", remove_button_clicked, ctk_3d_vision_pro,
                   hbox, __remove_glasses_tooltip);

    ctk_3d_vision_pro->table.rows = HTU(0)->num_glasses;
    ctk_3d_vision_pro->table.columns =  NUM_GLASSES_INFO_ATTRIBS;
    create_glasses_info_table(&(ctk_3d_vision_pro->table), HTU(0)->glasses_info,
                              frame_vbox, ctk_3d_vision_pro->ctk_config);

//-----------------------------------------------------------------------------
//  right vertical box

    rightvbox = gtk_vbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(mainhbox), rightvbox, FALSE, FALSE, 0);

    frame = gtk_frame_new("RF Hub");
    gtk_box_pack_start(GTK_BOX(rightvbox), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    vbox1 = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, FALSE, 0);
    vbox2 = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);

    snprintf(temp, sizeof(temp), "RF Hubs Connected:");
    label = add_label(temp, vbox1);

    hbox1 = gtk_hbox_new(FALSE, 5);
    snprintf(temp, sizeof(temp), "Signal Strength:");
    label = add_label(temp, hbox1);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);

    snprintf(temp, sizeof(temp), "%d", ctk_3d_vision_pro->num_htu);
    hbox1 = gtk_hbox_new(FALSE, 5);
    label = add_label(temp, hbox1);
    gtk_box_pack_start(GTK_BOX(vbox2), hbox1, FALSE, FALSE, 0);

    hbox1 = gtk_hbox_new(FALSE, 5);
    snprintf(temp, sizeof(temp), "[%d%%]", HTU(0)->signal_strength);
    image = gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(
              get_signal_strength_icon(HTU(0)->signal_strength)));
    gtk_box_pack_start(GTK_BOX(hbox1), image, FALSE, FALSE, 0);
    label = add_label(temp, hbox1);
    ctk_3d_vision_pro->signal_strength_label = GTK_LABEL(label);
    ctk_3d_vision_pro->signal_strength_image = image;
    gtk_box_pack_start(GTK_BOX(vbox2), hbox1, FALSE, FALSE, 0);

    snprintf(temp, sizeof(temp), "Current Channel ID:");
    label = add_label(temp, vbox1);

    snprintf(temp, sizeof(temp), "%d", HTU(0)->channel_num);
    label = add_label(temp, vbox2);
    ctk_3d_vision_pro->channel_num_label = GTK_LABEL(label);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = add_label("Hub Range:", hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    ctk_drop_down_menu_append_item(menu, "Short Range (up to 5 meters)", 0);
    ctk_drop_down_menu_append_item(menu, "Medium Range (up to 15 meters)", 1);
    ctk_drop_down_menu_append_item(menu, "Long Range", 2);
    
    ctk_3d_vision_pro->menu = GTK_WIDGET(menu);

    alignment = gtk_alignment_new(0, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), alignment, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), ctk_3d_vision_pro->menu); 

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(ctk_3d_vision_pro->menu),
                      "channel_range", GINT_TO_POINTER(0));

    ctk_drop_down_menu_set_current_value((menu),
                                         CHANNEL_RANGE_TO_OPTION_MENU_IDX(HTU(0)->channel_range));
    g_signal_connect(G_OBJECT(ctk_3d_vision_pro->menu), "changed",
                     G_CALLBACK(channel_range_changed),
                     (gpointer) ctk_3d_vision_pro);
    enable_widgets(ctk_3d_vision_pro, (HTU(0)->channel_range == SVP_LONG_RANGE ? FALSE : TRUE));

    ctk_config_set_tooltip(ctk_config, ctk_3d_vision_pro->menu,
                           __channel_range_tooltip);

    ctk_3d_vision_pro->parent_wnd = GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(ctk_3d_vision_pro)));

    /* finally, show the widget */
    gtk_widget_show_all(GTK_WIDGET(object));

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_3D_VISION_PRO_GLASSES_PAIR_EVENT),
                     G_CALLBACK(callback_glasses_paired),
                     (gpointer) ctk_3d_vision_pro);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_3D_VISION_PRO_GLASSES_UNPAIR_EVENT),
                     G_CALLBACK(callback_glasses_unpaired),
                     (gpointer) ctk_3d_vision_pro);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL),
                     G_CALLBACK(svp_config_changed),
                     (gpointer) ctk_3d_vision_pro);


    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE),
                     G_CALLBACK(svp_config_changed),
                     (gpointer) ctk_3d_vision_pro);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY),
                     G_CALLBACK(svp_config_changed),
                     (gpointer) ctk_3d_vision_pro);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME),
                     G_CALLBACK(svp_config_changed),
                     (gpointer) ctk_3d_vision_pro);

    return GTK_WIDGET(object);
}


GtkTextBuffer *ctk_3d_vision_pro_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table); 
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "NVIDIA® 3D VisionPro™ help");
    ctk_help_para(b, &i, "Use this page to configure the NVIDIA® 3D VisionPro™ hub "
                         "and glasses. You can set up new glasses, change the "
                         "hub's range, view which glasses are synchronized with "
                         "the hub, and select a different channel to improve the "
                         "hub-to-glasses signal strength.");

    ctk_help_para(b, &i, "NVIDIA® 3D Vision™ Pro is the professional version "
                         "of the 3D Vision™ stereo glasses and emitter. While "
                         "the 3D Vision kit uses infrared (IR) communication "
                         "from the emitter to the stereo glasses, the 3D Vision "
                         "Pro kit uses radio frequency (RF) bi-directional "
                         "communication between the emitter and the stereo "
                         "glasses. This allows multiple 3D Vision Pro hubs to "
                         "be used in the same area without conflicts.");

    ctk_help_para(b, &i, "3D Vision Pro does not require line of sight between "
                         "the hub and the 3D Vision Pro glasses. This provides "
                         "more flexibility in the location, distance, and "
                         "position of the glasses with respect to the emitter.");


    ctk_help_heading(b, &i, "Glasses Section");
    ctk_help_para(b, &i, "This section contains various actions/configurations "
                         "that can be performed with the NVIDIA 3D VisionPro RF "
                         "glasses. This section also displays a list of glasses "
                         "synced to the hub and their battery levels.");

    ctk_help_heading(b, &i, "Glasses Connected");
    ctk_help_para(b, &i, "Shows how many glasses are connected and synchronized "
                         "with the hub.");

    ctk_help_heading(b, &i, "Add glasses");
    ctk_help_para(b, &i, "%s", __add_glasses_tooltip);
    ctk_help_para(b, &i, "This action is used to set up new 3D Vision Pro Glasses. "
                         "On clicking this button the hub enters into pairing mode. "
                         "Follow the instructions on Add Glasses dialog box. "
                         "On pairing the new glasses, they appear in the glasses "
                         "information table. Choose 'Save' to save the newly paired "
                         " glasses or 'Cancel' if do not wish to store them.");

    ctk_help_heading(b, &i, "Refresh Glasses' Information");
    ctk_help_para(b, &i, "%s", __refresh_tooltip);
    ctk_help_para(b, &i, "Refresh glasses information is typically required when- \n"
                         "o Glasses move in and out of the range.\n"
                         "o Get the updated battery level of all the glasses.");

    ctk_help_heading(b, &i, "Identify glasses");
    ctk_help_para(b, &i, "Select the glasses from the list of paired glasses that "
                         "you want to identify. Hub will communicate with the "
                         "selected glasses and make LED on the glasses blink "
                         "for a few seconds.");

    ctk_help_heading(b, &i, "Rename glasses");
    ctk_help_para(b, &i, "%s", __rename_tooltip);
    ctk_help_para(b, &i, "Select the glasses from the list of paired glasses "
                         "that you want to rename and provide an unique new name.");

    ctk_help_heading(b, &i, "Remove glasses");
    ctk_help_para(b, &i, "%s", __remove_glasses_tooltip);
    ctk_help_para(b, &i, "Select the glasses from the list of paired glasses "
                         "that you want to remove. On removal glasses get "
                         "unpaired and will not sync to the hub.");

    ctk_help_heading(b, &i, "Glasses Information");
    ctk_help_para(b, &i, "%s", __goggle_info_tooltip);

    ctk_help_heading(b, &i, "Glasses Name");
    ctk_help_para(b, &i, "Each pair of glasses has an unique name and the name should "
                         "start and end with an alpha-numeric character. "
                         "Glasses can be renamed using Rename button.");

    ctk_help_heading(b, &i, "Battery Level");
    ctk_help_para(b, &i, "Displays battery level icon along with the value in "
                         "percentage.");

    ctk_help_heading(b, &i, "RF Hub section");
    ctk_help_para(b, &i, "This section contains various actions that can be "
                         "performed on the NVIDIA® 3D VisionPro™ hub. This "
                         "section also displays signal strength of the channel "
                         "currently used and current channel ID.");

    ctk_help_heading(b, &i, "Signal strength");
    ctk_help_para(b, &i, "Shows the signal strength of the current hub channel as an icon "
                         "and also value in percentage. \n"
                         "Signal strength is from one of the six ranges below-\n"
                         "\tExcellent\t\t [100%%]\n"
                         "\tVery Good\t [>75%% - <100%%]\n"
                         "\tGood     \t\t [>50%% - <75%%]\n"
                         "\tLow      \t\t [>25%% - <50%%]\n"
                         "\tVery Low \t\t [>0%%  - <25%%]\n"
                         "\tNo Signal\t\t [0%%]");

    ctk_help_heading(b, &i, "Hub Range");
    ctk_help_para(b, &i, "%s", __channel_range_tooltip);
    ctk_help_para(b, &i, "The hub range is the farthest distance that the "
                         "glasses can synchronize with the 3D Vision Pro Hub. "
                         "You can reduce the hub range to limit the experience "
                         "to a small group, or increase the range to include "
                         "everyone in a large room.\n"
                         "Possible values for transceiver range are 'Short "
                         "Range' 'Medium Range' and 'Long Range'.");
    ctk_help_para(b, &i, "Short Range: \n"
                         "Allows glasses within a 5-meter (16.5-foot) range to "
                         "be synced with the hub. This range is typically used "
                         "for sharing 3D simulations and training information "
                         "on a local workstation.");
    ctk_help_para(b, &i, "Medium Range: \n"
                         "Allows glasses within a 15-meter (49-foot) range to "
                         "be synced with the hub. This range is typically used "
                         "for sharing a presentation with a limited audience or "
                         "interacting with 3D CAD models during a collaborative "
                         "design session.");
    ctk_help_para(b, &i, "Long Range: \n"
                         "All glasses detected within the range and frequency of "
                         "the hub will be synced. This range is typically used "
                         "in a theater or visualization center.");

    ctk_help_finish(b);

    return b;
}

/* ctk_3d_vision_pro_select() *******************************************
 *
 * Callback function for when the 3D Vision Pro page is being displayed
 * in the control panel.
 *
 */
void ctk_3d_vision_pro_select(GtkWidget *w)
{

}

/* ctk_3d_vision_pro_unselect() *****************************************
 *
 * Callback function for when the 3D VisionPro page is no longer being
 * displayed by the control panel.  (User clicked on another page.)
 *
 */
void ctk_3d_vision_pro_unselect(GtkWidget *w)
{

}

/* ctk_3d_vision_pro_config_file_attributes() ****************************
 *
 * Add to the ParsedAttribute list any attributes that we want saved
 * in the config file.
 *
 */
void ctk_3d_vision_pro_config_file_attributes(GtkWidget *w,
                                              ParsedAttribute *head)
{

}

