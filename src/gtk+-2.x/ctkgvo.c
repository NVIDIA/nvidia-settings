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

#include "ctkhelp.h"
#include "ctkgvo.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"
#include "ctkimage.h"
#include "ctkbanner.h"

#include "msg.h"

/* local prototypes */

static void init_gvo_banner(CtkGvo *ctk_gvo, CtkGvoBanner *banner);
static gboolean update_gvo_banner(gpointer data);

static void update_gvo_banner_video_output(CtkGvoBanner *banner,
                                           gint output_video_format,
                                           gint output_data_format);

static void update_gvo_banner_video_input(CtkGvoBanner *banner,
                                          gint sdi, gint comp);

static GtkWidget *start_menu(const gchar *name, GtkWidget *table,
                             const gint row);

static void finish_menu(GtkWidget *menu, GtkWidget *table, const gint row);


static void sync_mode_changed(CtkDropDownMenu *menu, gpointer user_data);

static void output_video_format_changed(CtkDropDownMenu *menu,
                                        gpointer user_data);

static void post_output_video_format_changed(CtkGvo *ctk_gvo, gint value);

static void output_data_format_changed(CtkDropDownMenu *menu,
                                       gpointer user_data);

static void post_output_data_format_changed(CtkGvo *ctk_gvo, gint value);

static void update_sync_mode_menu(CtkGvo *ctk_gvo,
                                  const gint composite_sync,
                                  const gint sdi_sync,
                                  const gint current);

static void init_sync_mode_menu(CtkGvo *ctk_gvo);

static void update_output_video_format_menu(CtkGvo *ctk_gvo);

static void init_output_video_format_menu(CtkGvo *ctk_gvo);

static void init_output_data_format_menu(CtkGvo *ctk_gvo);


static void create_toggle_sdi_output_button(CtkGvo *ctk_gvo, gboolean enabled);

static void toggle_sdi_output_button(GtkWidget *button, gpointer user_data);

static void post_toggle_sdi_output_button(CtkGvo *gvo, gboolean enabled);

static void init_sync_format_menu(CtkGvo *ctk_gvo);

static void update_sync_format_menu(CtkGvo *ctk_gvo);

static void sync_format_changed(CtkDropDownMenu *menu, gpointer user_data);

static void update_input_video_format_text_entry(CtkGvo *ctk_gvo);

static void init_input_video_format_text_entry(CtkGvo *ctk_gvo);

static void init_composite_termination(CtkGvo *ctk_gvo);

static int max_input_video_format_text_entry_length(void);

static void post_toggle_composite_termination(CtkGvo *gvo, gboolean enabled);

static void toggle_composite_termination(GtkWidget *button,
                                         CtkGvo *ctk_gvo);

static void detect_input(GtkToggleButton *togglebutton, CtkGvo *ctk_gvo);
static gint detect_input_done(gpointer data);

static const char *get_video_format_name(const gint format);
static const char *get_data_format_name(const gint format);
static void get_video_format_resolution(const gint format, gint *w, gint *h);

static void set_gvo_sensitivity(CtkGvo *ctk_gvo,
                                gboolean sensitive, guint flags);

static void get_video_format_details(CtkGvo *ctk_gvo);

static void update_gvo_current_info(CtkGvo *ctk_gvo, guint lock_owner);

static void update_delay_spin_buttons(CtkGvo *ctk_gvo);

static void hsync_delay_changed(GtkSpinButton *spinbutton, gpointer user_data);
static void vsync_delay_changed(GtkSpinButton *spinbutton, gpointer user_data);

static void update_offset_spin_buttons(CtkGvo *ctk_gvo);

static void x_offset_changed(GtkSpinButton *spinbutton, gpointer user_data);
static void y_offset_changed(GtkSpinButton *spinbutton, gpointer user_data);

static void register_for_gvo_events(CtkGvo *ctk_gvo);

static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data);

static void composite_callback(CtkBanner *banner, void *data);


GType ctk_gvo_get_type(void)
{
    static GType ctk_gvo_type = 0;

    if (!ctk_gvo_type) {
        static const GTypeInfo ctk_gvo_info = {
            sizeof (CtkGvoClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkGvo),
            0,    /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_gvo_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkGvo",
                                   &ctk_gvo_info, 0);
    }
    
    return ctk_gvo_type;
    
} /* ctk_gvo_get_type() */



/*
 * video format table -- should this be moved into NV-CONTROL?
 */

typedef struct {
    int format;
    const char *name;
} FormatName;

typedef struct {
    int format;
    int rate;
    int width;
    int height;
} FormatDetails;

static const FormatName videoFormatNames[] = {
    { NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC, "720  x 480i    59.94  Hz  (SMPTE259) NTSC"},
    { NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL,  "720  x 576i    50.00  Hz  (SMPTE259) PAL"},
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296,      "1280 x 720p    23.98  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296,      "1280 x 720p    24.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296,      "1280 x 720p    25.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296,      "1280 x 720p    29.97  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296,      "1280 x 720p    30.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296,      "1280 x 720p    50.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296,      "1280 x 720p    59.94  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296,      "1280 x 720p    60.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260,     "1920 x 1035i   59.94  Hz  (SMPTE260)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260,     "1920 x 1035i   60.00  Hz  (SMPTE260)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274,     "1920 x 1080i   47.96  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274,     "1920 x 1080i   48.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295,     "1920 x 1080i   50.00  Hz  (SMPTE295)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274,     "1920 x 1080i   50.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274,     "1920 x 1080i   59.94  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274,     "1920 x 1080i   60.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274,    "1920 x 1080p   23.976 Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274,     "1920 x 1080p   24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274,     "1920 x 1080p   25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274,     "1920 x 1080p   29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274,     "1920 x 1080p   30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274,   "1920 x 1080PsF 23.98  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274,   "1920 x 1080PsF 24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274,   "1920 x 1080PsF 25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274,   "1920 x 1080PsF 29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274,   "1920 x 1080PsF 30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_47_96_SMPTE372,     "2048 x 1080i   47.96  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_48_00_SMPTE372,     "2048 x 1080i   48.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_50_00_SMPTE372,     "2048 x 1080i   50.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_59_94_SMPTE372,     "2048 x 1080i   59.94  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_60_00_SMPTE372,     "2048 x 1080i   60.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_23_98_SMPTE372,     "2048 x 1080p   23.98  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_24_00_SMPTE372,     "2048 x 1080p   24.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_25_00_SMPTE372,     "2048 x 1080p   25.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_29_97_SMPTE372,     "2048 x 1080p   29.97  Hz  (SMPTE372)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_30_00_SMPTE372,     "2048 x 1080p   30.00  Hz  (SMPTE372)"    },
    { -1, NULL },
};


static FormatDetails videoFormatDetails[] = {
    { NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC, 0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL,  0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274,    0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_47_96_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_48_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_50_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_59_94_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048I_60_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_23_98_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_24_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_25_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_29_97_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVO_VIDEO_FORMAT_2048P_30_00_SMPTE372,     0, 0, 0 },
    { -1, -1, -1, -1 },
};


static const FormatName dataFormatNames[] = {
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444, "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422, "RGB -> YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8_444_PASSTHRU,"RGB (4:4:4)" },
    { -1, NULL },
};



#define TABLE_PADDING 5


#define SYNC_FORMAT_SDI 0
#define SYNC_FORMAT_COMP_AUTO 1
#define SYNC_FORMAT_COMP_BI_LEVEL 2
#define SYNC_FORMAT_COMP_TRI_LEVEL 3

static const FormatName syncFormatNames[] = {
    { SYNC_FORMAT_SDI,            "SDI Sync" },
    { SYNC_FORMAT_COMP_AUTO,      "COMP Sync" },
    { SYNC_FORMAT_COMP_BI_LEVEL,  "COMP Sync (Bi-level)" },
    { SYNC_FORMAT_COMP_TRI_LEVEL, "COMP Sync (Tri-level)" },
    { -1, NULL },
};


/* arguments to set_gvo_sensitivity() */

#define SET_SENSITIVITY_EXCLUDE_ENABLE_BUTTON 0x0001
#define SET_SENSITIVITY_EXCLUDE_DETECT_BUTTON 0x0002
#define SET_SENSITIVITY_EXCLUDE_ROI_BUTTONS   0x0004

#define DEFAULT_DETECT_INPUT_TIME_INTERVAL 2000
#define UPDATE_GVO_BANNER_TIME_INTERVAL    200
#define DEFAULT_GVO_PROBE_TIME_INTERVAL    1000

/*
 * ctk_gvo_new() - constructor for the CtkGvo widget
 */

GtkWidget* ctk_gvo_new(NvCtrlAttributeHandle *handle,
                       GtkWidget *parent_window,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkGvo *ctk_gvo;
    GtkWidget *hbox, *alignment, *button, *label;
    ReturnStatus ret;
    gchar scratch[64], *firmware, *string;
    gint val, i, width, height, n, caps;
    
    GtkWidget *frame, *table, *menu;
    
    /* make sure we have a handle */
    
    g_return_val_if_fail(handle != NULL, NULL);
    
    /* check if this screen supports GVO */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_SUPPORTED, &val);
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_GVO_SUPPORTED_TRUE)) {
        /* GVO not available */
        return NULL;
    }

    /* Get this GVO device's capabilities */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_CAPABILITIES, &caps);
    if (ret != NvCtrlSuccess) return NULL;
    
    /* create the CtkGvo object */
    
    object = g_object_new(CTK_TYPE_GVO, NULL);
    
    /* initialize fields in the CtkGvo object */
    
    ctk_gvo = CTK_GVO(object);
    ctk_gvo->handle = handle;
    ctk_gvo->parent_window = parent_window;
    ctk_gvo->ctk_config = ctk_config;
    ctk_gvo->ctk_event = ctk_event;
    
    /* set container properties for the CtkGvo widget */
    
    gtk_box_set_spacing(GTK_BOX(ctk_gvo), 10);

    /* banner */
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    ctk_gvo->banner_box = hbox;

    init_gvo_banner(ctk_gvo, &ctk_gvo->banner);


    /*
     * General information
     */
    
    frame = gtk_frame_new("General Information");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(3, 2, FALSE);
    
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_container_add(GTK_CONTAINER(frame), table);
    
    /* GVO_FIRMWARE_VERSION */
    
    string = NULL;
    
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_GVO_FIRMWARE_VERSION, 
                                   &string);
    
    if ((ret == NvCtrlSuccess) && (string)) {
        firmware = strdup(string);
    } else {
        
        /*
         * NV_CTRL_STRING_GVO_FIRMWARE_VERSION was added later, so
         * older X servers may not know about it; fallback to
         * NV_CTRL_GVO_FIRMWARE_VERSION
         */
        
        ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_FIRMWARE_VERSION, &val);
        
        if (ret == NvCtrlSuccess) {
            snprintf(scratch, 64, "1.%02d", val);
            firmware = strdup(scratch);
        } else {
            firmware = strdup("???");
        }
    }
    
    add_table_row(table, 0,
                  0, 0.5, "Firmware Version:",
                  0, 0.5, firmware);
    ctk_gvo->current_resolution_label =
        add_table_row(table, 1,
                      0, 0.5, "Current SDI Resolution:",
                      0, 0.5, "Inactive");
    ctk_gvo->current_state_label =
        add_table_row(table, 2,
                      0, 0.5, "Current SDI State:",
                      0, 0.5, "Inactive");
    
    
    /*
     * Sync options
     */
    
    frame = gtk_frame_new("Sync Options");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(4, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);

    gtk_container_add(GTK_CONTAINER(frame), table);
    
    /* Sync Mode */
    
    menu = start_menu("Sync Mode: ", table, 0);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu), "Free Running",
                                   NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu), "GenLock",
                                   NV_CTRL_GVO_SYNC_MODE_GENLOCK);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu), "FrameLock",
                                   NV_CTRL_GVO_SYNC_MODE_FRAMELOCK);
    
    finish_menu(menu, table, 0);
    
    ctk_gvo->sync_mode_menu = menu;

    init_sync_mode_menu(ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->sync_mode_menu), "changed",
                     G_CALLBACK(sync_mode_changed), (gpointer) ctk_gvo);
    
    /* Sync Format */

    menu = start_menu("Sync Format: ", table, 1);
    
    for (i = 0; syncFormatNames[i].name; i++) {
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       syncFormatNames[i].name,
                                       syncFormatNames[i].format);
    }
    
    finish_menu(menu, table, 1);

    ctk_gvo->sync_format_menu = menu;

    init_sync_format_menu(ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->sync_format_menu),
                     "changed", G_CALLBACK(sync_format_changed),
                     (gpointer) ctk_gvo);
    
    /* input type */

    label = gtk_label_new("Input Video Format: ");
    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    
    gtk_table_attach(GTK_TABLE(table),
                     alignment, 0, 1, 2, 3, GTK_FILL, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);

    ctk_gvo->input_video_format_text_entry = gtk_entry_new();
    
    gtk_widget_set_sensitive(ctk_gvo->input_video_format_text_entry, FALSE);
    gtk_entry_set_width_chars
        (GTK_ENTRY(ctk_gvo->input_video_format_text_entry),
         max_input_video_format_text_entry_length());

    init_input_video_format_text_entry(ctk_gvo);
    
    gtk_table_attach(GTK_TABLE(table), ctk_gvo->input_video_format_text_entry,
                     1, 2, 2, 3, GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
    
    /* detect button */
    
    button = gtk_toggle_button_new_with_label("Detect");
    
    alignment = gtk_alignment_new(1, 1, 0, 0);
    
    gtk_container_add(GTK_CONTAINER(alignment), button);
    gtk_table_attach(GTK_TABLE(table), alignment,
                     1, 2, 3, 4, GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);

    ctk_gvo->input_video_format_detect_button = button;
    
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(detect_input), ctk_gvo);

    /* Composite Termination */

    if (caps & NV_CTRL_GVO_CAPABILITIES_COMPOSITE_TERMINATION) {
        
        button =
            gtk_check_button_new_with_label("Enable Composite Termination");
        
        alignment = gtk_alignment_new(1, 1, 0, 0);
        
        gtk_container_add(GTK_CONTAINER(alignment), button);
        gtk_table_attach(GTK_TABLE(table), alignment,
                         1, 2, 4, 5, GTK_FILL | GTK_EXPAND, GTK_FILL,
                         TABLE_PADDING, TABLE_PADDING);
        
        ctk_gvo->composite_termination_button = button;
        
        init_composite_termination(ctk_gvo);
        
        g_signal_connect(G_OBJECT(button), "toggled",
                         G_CALLBACK(toggle_composite_termination), ctk_gvo);
    } else {
        ctk_gvo->composite_termination_button = NULL;
    }

    /*
     * Output options
     */
    
    frame = gtk_frame_new("Output Options");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);

    gtk_container_add(GTK_CONTAINER(frame), table);
    
    /* Output Video Format */

    menu = start_menu("Output Video Format: ", table, 0);
    
    ctk_gvo->screen_width = NvCtrlGetScreenWidth(handle);
    ctk_gvo->screen_height = NvCtrlGetScreenHeight(handle);
    
    /* get the width, height, and refresh rate for each format */

    get_video_format_details(ctk_gvo);
    
    n = 0;
    
    for (i = 0; videoFormatNames[i].name; i++) {
        
        /* 
         * runtime check that videoFormatDetails[] and
         * videoFormatNames[] are in sync
         */
        
        if (videoFormatDetails[i].format != videoFormatNames[i].format) {
            nv_error_msg("GVO format tables out of alignment!");
            return NULL;
        }
        
        /* check that the current X screen can support width and height */
        
        width = videoFormatDetails[i].width;
        height = videoFormatDetails[i].height;
        
        if ((width > ctk_gvo->screen_width) ||
            (height > ctk_gvo->screen_height)) {

            nv_warning_msg("Not exposing GVO video format '%s' (this video "
                           "format requires a resolution of atleast %d x %d, "
                           "but the current X screen is %d x %d)",
                           videoFormatNames[i].name,
                           width, height, ctk_gvo->screen_width,
                           ctk_gvo->screen_height);
            continue;
        }
        
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       videoFormatNames[i].name,
                                       videoFormatNames[i].format);
        n++;
    }

    finish_menu(menu, table, 0);
    
    if (!n) {
        nv_warning_msg("No GVO video formats will fit the current X screen of "
                       "%d x %d; please create an X screen of atleast "
                       "720 x 487; not exposing GVO page.\n",
                       ctk_gvo->screen_width, ctk_gvo->screen_height);
        return NULL;
    }


    ctk_gvo->output_video_format_menu = menu;
    
    init_output_video_format_menu(ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->output_video_format_menu),
                     "changed", G_CALLBACK(output_video_format_changed),
                     (gpointer) ctk_gvo);
    
    
    /* Output Data Format */
    
    menu = start_menu("Output Data Format: ", table, 1);
    
    for (i = 0; dataFormatNames[i].name; i++) {
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       dataFormatNames[i].name,
                                       dataFormatNames[i].format);
    }
    
    finish_menu(menu, table, 1);
    
    ctk_gvo->output_data_format_menu = menu;
    
    g_signal_connect(G_OBJECT(ctk_gvo->output_data_format_menu),
                     "changed", G_CALLBACK(output_data_format_changed),
                     (gpointer) ctk_gvo);
    
    init_output_data_format_menu(ctk_gvo);
    

    /*
     * Synchronization Delay
     */
    
    frame = gtk_frame_new("Synchronization Delay");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(2, 2, FALSE);
    
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_container_add(GTK_CONTAINER(frame), table);
    
    get_video_format_resolution(ctk_gvo->input_video_format, &width, &height);

    /* NV_CTRL_GVO_SYNC_DELAY_PIXELS */

    label = gtk_label_new("HSync Delay (in pixels): ");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_SYNC_DELAY_PIXELS, &val);
    if (ret != NvCtrlSuccess) val = 0;

    if (width < 1) width = 1;

    ctk_gvo->hsync_delay_spin_button =
        gtk_spin_button_new_with_range(0.0, width, 1);

    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->hsync_delay_spin_button), val);
    
    g_signal_connect(G_OBJECT(ctk_gvo->hsync_delay_spin_button),
                     "value-changed",
                     G_CALLBACK(hsync_delay_changed), ctk_gvo);

    gtk_table_attach(GTK_TABLE(table), ctk_gvo->hsync_delay_spin_button,
                     1, 2, 0, 1,
                     GTK_FILL /*| GTK_EXPAND*/, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);

    /* NV_CTRL_GVO_SYNC_DELAY_LINES */
    
    label = gtk_label_new("VSync Delay (in lines): ");
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_SYNC_DELAY_LINES, &val);
    if (ret != NvCtrlSuccess) val = 0;
    
    if (height < 1) height = 1;

    ctk_gvo->vsync_delay_spin_button =
        gtk_spin_button_new_with_range(0.0, height, 1);
    
    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->vsync_delay_spin_button), val);
    
    g_signal_connect(G_OBJECT(ctk_gvo->vsync_delay_spin_button),
                     "value-changed",
                     G_CALLBACK(vsync_delay_changed), ctk_gvo);

    gtk_table_attach(GTK_TABLE(table), ctk_gvo->vsync_delay_spin_button,
                     1, 2, 1, 2,
                     GTK_FILL /*| GTK_EXPAND*/, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);

    /*
     * Region of Interest
     */
    
    frame = gtk_frame_new("Region of Interest");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    
    get_video_format_resolution(ctk_gvo->output_video_format, &width, &height);

    /* NV_CTRL_GVO_X_SCREEN_PAN_X */

    label = gtk_label_new("X Offset: ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, TABLE_PADDING);
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_X_SCREEN_PAN_X, &val);
    if (ret != NvCtrlSuccess) val = 0;

    width = ctk_gvo->screen_width - width;
    if (width < 1) width = 1;

    ctk_gvo->x_offset_spin_button =
        gtk_spin_button_new_with_range(0.0, width, 1);
    
    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->x_offset_spin_button), val);

    g_signal_connect(G_OBJECT(ctk_gvo->x_offset_spin_button),
                     "value-changed",
                     G_CALLBACK(x_offset_changed), ctk_gvo);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_gvo->x_offset_spin_button,
                       FALSE, FALSE, TABLE_PADDING);

    /* NV_CTRL_GVO_X_SCREEN_PAN_Y */
    
    label = gtk_label_new("Y Offset: ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, TABLE_PADDING);
 
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_X_SCREEN_PAN_Y, &val);
    if (ret != NvCtrlSuccess) val = 0;

    height = ctk_gvo->screen_height - height;
    if (height < 1) height = 1;

    ctk_gvo->y_offset_spin_button =
        gtk_spin_button_new_with_range(0.0, height, 1);
    
    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->y_offset_spin_button), val);

    g_signal_connect(G_OBJECT(ctk_gvo->y_offset_spin_button),
                     "value-changed",
                     G_CALLBACK(y_offset_changed), ctk_gvo);

    gtk_box_pack_start(GTK_BOX(hbox), ctk_gvo->y_offset_spin_button,
                       FALSE, FALSE, TABLE_PADDING);

    
    /*
     * "Enable SDI Output" button
     */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_DISPLAY_X_SCREEN, &val);
    if (ret != NvCtrlSuccess) val = 0;

    create_toggle_sdi_output_button(ctk_gvo, val);
    
    post_toggle_sdi_output_button(ctk_gvo, val);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_gvo->toggle_sdi_output_button);
    
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* create the watch cursor (for use when "Detect" is toggled" */
    
    ctk_gvo->wait_cursor = gdk_cursor_new(GDK_WATCH);
    
    /*
     * register a timeout function (directly with glib, not through
     * ctk_config) to update the LEDs
     */

    g_timeout_add(UPDATE_GVO_BANNER_TIME_INTERVAL, update_gvo_banner,
                  &ctk_gvo->banner);

    string = g_strdup_printf("Graphics To Video Probe (Screen %u)",
                             NvCtrlGetTargetId(handle));

    ctk_config_add_timer(ctk_gvo->ctk_config,
                         DEFAULT_GVO_PROBE_TIME_INTERVAL,
                         string,
                         (GSourceFunc) ctk_gvo_probe,
                         (gpointer) ctk_gvo);

    g_free(string);

    
    register_for_gvo_events(ctk_gvo);

    /* show the GVO widget */
    
    gtk_widget_show_all(GTK_WIDGET(ctk_gvo));

    return GTK_WIDGET(ctk_gvo);

} /* ctk_gvo_new() */



/**************************************************************************/
/*
 * code for handling the GVO banner
 */

/* value for controlling LED state */

#define GVO_LED_VID_OUT_NOT_IN_USE 0
#define GVO_LED_VID_OUT_HD_MODE    1
#define GVO_LED_VID_OUT_SD_MODE    2

#define GVO_LED_SDI_SYNC_NONE      0
#define GVO_LED_SDI_SYNC_HD        1
#define GVO_LED_SDI_SYNC_SD        2
#define GVO_LED_SDI_SYNC_ERROR     3

#define GVO_LED_COMP_SYNC_NONE     0
#define GVO_LED_COMP_SYNC_SYNC     1

#define LED_GREY    0
#define LED_GREEN   1
#define LED_YELLOW  2
#define LED_RED     3

/* Position of LEDs relative to the SDI image. */

static int __led_pos_x[] = { 74, 101, 128, 156 }; // From sdi.png
static int __led_pos_y   = 36;                    // From sdi.png



/*
 * draw_led() - Updates the LED to the given color in the banner's
 * backing pixbuf.
 */

static void draw_led(CtkBanner *ctk_banner, int led, int color) {
    
    /* Which LED to draw */
    int dst_x = ctk_banner->artwork_x +__led_pos_x[led];
    int dst_y = ctk_banner->artwork_y +__led_pos_y;

    /* Offset LED color into LED position */
    int offset_x = ctk_banner->artwork_x +__led_pos_x[led] -__led_pos_x[color];
    int offset_y = ctk_banner->artwork_y;

    gdk_pixbuf_composite(ctk_banner->artwork.pixbuf,    // src
                         ctk_banner->back.pixbuf,       // dest
                         dst_x,                         // dest_x
                         dst_y,                         // dest_y
                         10,                            // dest_width
                         10,                            // dest_height
                         offset_x,                      // offset_x
                         offset_y,                      // offset_y
                         1.0,                           // scale_x
                         1.0,                           // scale_y
                         GDK_INTERP_BILINEAR,           // interp_type
                         255);                          // overall_alpha
}



/*
 * composite_callback() - Draws all the LEDs to the banner.
 */
static void composite_callback(CtkBanner *ctk_banner, void *data)
{
    CtkGvo *ctk_gvo = (CtkGvo *) data;
    int i;

    /* Grab the latest banner widget */
    ctk_gvo->banner.ctk_banner = GTK_WIDGET(ctk_banner);

    /* Draw the current state */
    for (i = 0; i < 4; i++) {
        draw_led(ctk_banner, i, ctk_gvo->banner.img[i]);
    }
}



/*
 * update_led() - Updates the state of an LED and causes and expose event.
 */

static void update_led(CtkGvoBanner *banner, int led, int color)
{
    GtkWidget *ctk_banner = banner->ctk_banner;
    GdkRectangle rec = {0, __led_pos_y,  10,10};

    /* Update the state of the LED */
    banner->img[led] = color;

    /* Draw the LED and tell gdk to draw it to the window */
    if (ctk_banner && ctk_banner->window) {

        draw_led(CTK_BANNER(ctk_banner), led, color);

        rec.x = CTK_BANNER(ctk_banner)->artwork_x + __led_pos_x[led];
        rec.y = CTK_BANNER(ctk_banner)->artwork_y + __led_pos_y;
        gdk_window_invalidate_rect(ctk_banner->window, &rec, TRUE);
    }
}



/*
 * init_gvo_banner() - initialize the GVO banner
 */

static void init_gvo_banner(CtkGvo *ctk_gvo, CtkGvoBanner *banner)
{
    /* Create the banner image */
    banner->widget = ctk_banner_image_new_with_callback(BANNER_ARTWORK_SDI,
                                                        composite_callback,
                                                        ctk_gvo);
    g_object_ref(banner->widget);

    banner->ctk_banner = NULL;

    /*
     * initialize LED state
     */

    banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_NOT_IN_USE;
    banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    banner->state[GVO_BANNER_SDI]  = GVO_LED_SDI_SYNC_NONE;
    banner->state[GVO_BANNER_COMP] = GVO_LED_COMP_SYNC_NONE;

    banner->img[GVO_BANNER_VID1] = LED_GREY;
    banner->img[GVO_BANNER_VID2] = LED_GREY;
    banner->img[GVO_BANNER_SDI]  = LED_GREY;
    banner->img[GVO_BANNER_COMP] = LED_GREY;

    gtk_widget_show_all(GTK_WIDGET(banner->widget));

} /* init_gvo_banner() */



/*
 * update_gvo_banner() - called by a timer to update the LED images
 * based on current state
 */

static gboolean update_gvo_banner(gpointer data)
{
    guint8 old, new;
    CtkGvoBanner *banner = (CtkGvoBanner *) data;

    /*
     * we store the flashing state here:
     *
     * 0 == no LED is currently flashing
     * 1 == some LED is flashing; currently "on" (lit)
     * 2 == some LED is flashing; currently "off" (grey)
     *
     * this is used to track the current state, so that we can make
     * all LEDs flash at the same time.
     */

    gint flashing = 0;
    

    /* Vid 1 out */

    old = banner->img[GVO_BANNER_VID1];
    
    if (banner->state[GVO_BANNER_VID1] == GVO_LED_VID_OUT_HD_MODE) {
        new = (old == LED_GREY) ? LED_GREEN: LED_GREY;
        flashing = (new == LED_GREY) ? 2 : 1;

    } else if (banner->state[GVO_BANNER_VID1] == GVO_LED_VID_OUT_SD_MODE) {
        new = (old == LED_GREY) ? LED_YELLOW: LED_GREY;
        flashing = (new == LED_GREY) ? 2 : 1;

    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led(banner, GVO_BANNER_VID1, new);
    }

    /* Vid 2 out */

    old = banner->img[GVO_BANNER_VID2];
    
    if (banner->state[GVO_BANNER_VID2] == GVO_LED_VID_OUT_HD_MODE) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN: LED_GREY;
        } else {
            new = (old == LED_GREY) ? LED_GREEN: LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_VID2] == GVO_LED_VID_OUT_SD_MODE) {
        if (flashing) {
            new = (flashing == 1) ? LED_YELLOW: LED_GREY;
        } else {
            new = (old == LED_GREY) ? LED_YELLOW: LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led(banner, GVO_BANNER_VID2, new);
    }
    
    /* SDI sync */
    
    old = banner->img[GVO_BANNER_SDI];
    
    if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_HD) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_SDI] == LED_GREY) ?
                LED_GREEN : LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_SD) {
        if (flashing) {
            new = (flashing == 1) ? LED_YELLOW : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_SDI] == LED_GREY) ?
                LED_YELLOW : LED_GREY;
            flashing = (new == LED_GREY) ? 2 : 1;
        }
    } else if (banner->state[GVO_BANNER_SDI] == GVO_LED_SDI_SYNC_ERROR) {
        new = LED_YELLOW;
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led(banner, GVO_BANNER_SDI, new);
    }

    /* COMP sync */
    
    old = banner->img[GVO_BANNER_COMP];
    
    if (banner->state[GVO_BANNER_COMP] == GVO_LED_COMP_SYNC_SYNC) {
        if (flashing) {
            new = (flashing == 1) ? LED_GREEN : LED_GREY;
        } else {
            new = (banner->img[GVO_BANNER_COMP] == LED_GREY) ?
                LED_GREEN : LED_GREY;
        }
    } else {
        new = LED_GREY;
    }
    
    if (old != new) {
        update_led(banner, GVO_BANNER_COMP, new);
    }

    return TRUE;

} /* update_gvo_banner() */



/*
 * update_gvo_banner_video_output() - update banner state accordingly,
 * based on the current output_video_format and output_data_format
 */

static void update_gvo_banner_video_output(CtkGvoBanner *banner,
                                           gint output_video_format,
                                           gint output_data_format)
{
    if (output_video_format == NV_CTRL_GVO_VIDEO_FORMAT_NONE) {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_NOT_IN_USE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    } else if ((output_video_format ==
                NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC) ||
               (output_video_format ==
                NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL)) {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_SD_MODE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_SD_MODE;
    } else {
        banner->state[GVO_BANNER_VID1] = GVO_LED_VID_OUT_HD_MODE;
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_HD_MODE;
    }
    
    if (output_data_format == NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422) {
        banner->state[GVO_BANNER_VID2] = GVO_LED_VID_OUT_NOT_IN_USE;
    }
} /* update_gvo_banner_video_output() */



/*
 * update_gvo_banner_video_input() - update banner state accordingly,
 * based on the current sdi and composite input
 */

static void update_gvo_banner_video_input(CtkGvoBanner *banner,
                                          gint sdi, gint comp)
{
    if (sdi == NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_HD) {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_HD;
    } else if (sdi == NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_SD) {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_SD;
    } else {
        banner->state[GVO_BANNER_SDI] = GVO_LED_SDI_SYNC_NONE;
    }
    
    banner->state[GVO_BANNER_COMP] = comp ?
        GVO_LED_COMP_SYNC_SYNC : GVO_LED_COMP_SYNC_NONE;
    
} /* update_gvo_banner_video_input() */



/**************************************************************************/



static GtkWidget *start_menu(const gchar *name, GtkWidget *table,
                             const gint row)
{
    GtkWidget *menu, *label, *alignment;
    
    label = gtk_label_new(name);
    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);

    gtk_table_attach(GTK_TABLE(table),
                     alignment, 0, 1, row, row+1, GTK_FILL, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
    
    menu = ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_MONOSPACE);
    
    return menu;
}


static void finish_menu(GtkWidget *menu, GtkWidget *table, const gint row)
{
    ctk_drop_down_menu_finalize(CTK_DROP_DOWN_MENU(menu));

    gtk_table_attach(GTK_TABLE(table), menu, 1, 2, row, row+1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
}


/**************************************************************************/


/*
 * sync_mode_changed() - callback when the SyncMode menu changes
 */

static void sync_mode_changed(CtkDropDownMenu *menu, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    char *name;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    switch (value) {
    case NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING: name = "Free Running"; break;
    case NV_CTRL_GVO_SYNC_MODE_GENLOCK:      name = "GenLock";      break;
    case NV_CTRL_GVO_SYNC_MODE_FRAMELOCK:    name = "FrameLock";    break;
    default: return;
    }
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_MODE, value);
    
    if (value != NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) {
        NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_SOURCE,
                           ctk_gvo->sync_source);
    }

    ctk_gvo->sync_mode = value;
    
    update_output_video_format_menu(ctk_gvo);

    update_sync_format_menu(ctk_gvo);

    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Sync Mode set to %s.", name);
    
} /* sync_mode_changed() */



/*
 * output_video_format_changed() - callback when the output video
 * format menu changes
 */

static void output_video_format_changed(CtkDropDownMenu *menu,
                                        gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    if (((value < 32) &&
         !(ctk_gvo->valid_output_video_format_mask[0] & (1 << value))) ||
        ((value >= 32) &&
         !(ctk_gvo->valid_output_video_format_mask[1] & (1 << (value - 32))))){
        
        ctk_config_statusbar_message(ctk_gvo->ctk_config, "Invalid "
                                     "Output Video Format: %s; ignoring.",
                                     get_video_format_name(value));
        return;
    }
    
    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT, value);
    
    post_output_video_format_changed(ctk_gvo, value);
    
} /* output_video_format_changed() */


static void post_output_video_format_changed(CtkGvo *ctk_gvo, gint value)
{
    ctk_gvo->output_video_format = value;
    
    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Output Video Format set to: %s.",
                                 get_video_format_name(value));
}



/*
 * output_data_format_changed() - callback when the output data format
 * menu changes
 */

static void output_data_format_changed(CtkDropDownMenu *menu,
                                       gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DATA_FORMAT, value);
    
    post_output_data_format_changed(ctk_gvo, value);
    
} /* output_data_format_changed() */


static void post_output_data_format_changed(CtkGvo *ctk_gvo, gint value)
{
    ctk_gvo->output_data_format = value;

    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Output Data Format set to: %s.",
                                 get_data_format_name(value));
}


/**************************************************************************/



/*
 * update_sync_mode_menu() - given the COMPOSITE_SYNC and SDI_SYNC
 * values, set the sensitivity of each entry in sync_mode_menu.
 */

static void update_sync_mode_menu(CtkGvo *ctk_gvo,
                                  const gint composite_sync,
                                  const gint sdi_sync,
                                  const gint current)
{
    gboolean sensitive;

    if ((composite_sync == NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_FALSE) &&
        (sdi_sync == NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE)) {
        
        /* no external sync detected; do not expose GenLock or FrameLock */
        sensitive = FALSE;
        
    } else {
        
        /* external sync detected; expose GenLock amd FrameLock */
        sensitive = TRUE;
    }
    
    /* set the sensitivity for GenLock and FrameLock */

    ctk_drop_down_menu_set_value_sensitive
        (CTK_DROP_DOWN_MENU(ctk_gvo->sync_mode_menu),
         NV_CTRL_GVO_SYNC_MODE_GENLOCK, sensitive);
        
    ctk_drop_down_menu_set_value_sensitive
        (CTK_DROP_DOWN_MENU(ctk_gvo->sync_mode_menu),
         NV_CTRL_GVO_SYNC_MODE_FRAMELOCK, sensitive);
    
    /*
     * if the current SYNC_MODE is not FREE_RUNNING, but other modes
     * are not available, then we need to change the current setting
     * to FREE_RUNNING
     */
    
    if ((current != NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) && !sensitive) {
        
        NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_MODE,
                           NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING);
        
        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(ctk_gvo->sync_mode_menu),
             NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING);
    }
    
} /* update_sync_mode_menu() */



/*
 * init_sync_mode_menu() - initialize the sync mode menu: set the
 * currently active menu selection and set the sensitivity of the
 * menu entries.
 */

static void init_sync_mode_menu(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint val, comp, sdi;

    /* get the current mode */

    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_MODE, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING;
    }
    
    ctk_gvo->sync_mode = val;

    /* set the current mode in the menu */

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->sync_mode_menu), val);
    
    /* query COMPOSITE_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED, &comp);
    if (ret != NvCtrlSuccess) {
        comp = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_FALSE;
    }
    
    /* query SDI_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED, &sdi);
    if (ret != NvCtrlSuccess) {
        sdi = NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE;
    }

    /* update the menu */

    update_sync_mode_menu(ctk_gvo, comp, sdi, val);
    
} /* init_sync_mode_menu() */



/*
 * update_output_video_format_menu() - given the current
 * OUTPUT_VIDEO_FORMAT, set the sensitivity of each menu entry and
 * possibly update which entry is current.
 */

static void update_output_video_format_menu(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    NVCTRLAttributeValidValuesRec valid;
    gint bitmask, bitmask2, i, refresh_rate;
    gboolean sensitive, current_not_available = FALSE;

    /* retrieve the currently available values */

    ret = NvCtrlGetValidAttributeValues(ctk_gvo->handle,
                                        NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                                        &valid);

    /* if we failed to get the available values; assume none are valid */

    if ((ret != NvCtrlSuccess) || (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        bitmask = 0;
    } else {
        bitmask = valid.u.bits.ints;
    }
    
    /* retrieve additional available values */

    ret = NvCtrlGetValidAttributeValues(ctk_gvo->handle,
                                        NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT2,
                                        &valid);

    /* if we failed to get the available values; assume none are valid */
    
    if ((ret != NvCtrlSuccess) || (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        bitmask2 = 0;
    } else {
        bitmask2 = valid.u.bits.ints;
    }

    /* 
     * if the SyncMode is genlock or framelock, trim the bitmask
     * accordingly: if GENLOCK, then the only bit allowed is the bit
     * that corresponds to the exact input mode; if FRAMELOCK, then
     * only modes with the same refresh rate as the input mode are
     * allowed.
     */
    
    if ((ctk_gvo->sync_mode == NV_CTRL_GVO_SYNC_MODE_GENLOCK) &&
        (ctk_gvo->input_video_format != NV_CTRL_GVO_VIDEO_FORMAT_NONE)) {

        if (ctk_gvo->input_video_format < 32) {
            bitmask &= (1 << ctk_gvo->input_video_format);
            bitmask2 = 0;
        } else {
            bitmask = 0;
            bitmask2 &= (1 << (ctk_gvo->input_video_format - 32));
        }
    }
    
    if ((ctk_gvo->sync_mode == NV_CTRL_GVO_SYNC_MODE_FRAMELOCK) &&
        (ctk_gvo->input_video_format != NV_CTRL_GVO_VIDEO_FORMAT_NONE)) {
        
        refresh_rate = 0;
        
        for (i = 0; videoFormatDetails[i].format != -1; i++) {
            if (videoFormatDetails[i].format == ctk_gvo->input_video_format) {
                refresh_rate = videoFormatDetails[i].rate;
                break;
            }
        }
        
        for (i = 0; videoFormatDetails[i].format != -1; i++) {
            if (videoFormatDetails[i].rate != refresh_rate) {
                
                if (videoFormatDetails[i].format < 32) {
                    bitmask &= ~(1 << videoFormatDetails[i].format);
                } else {
                    bitmask2 &= ~(1 << (videoFormatDetails[i].format - 32));
                }
            }
        }
    }

    /*
     * loop over each video format; if that format is set in the
     * bitmask, then it is available
     */

    for (i = 0; videoFormatNames[i].name; i++) {
        if (((videoFormatNames[i].format < 32) &&
             ((1 << videoFormatNames[i].format) & bitmask)) ||
            ((videoFormatNames[i].format >= 32) &&
             (((1 << (videoFormatNames[i].format - 32)) & bitmask2)))) {
            sensitive = TRUE;
        } else {
            sensitive = FALSE;
        }

        ctk_drop_down_menu_set_value_sensitive
            (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu),
             videoFormatNames[i].format, sensitive);

        /* if the current video is not sensitive, take note */
        
        if ((ctk_gvo->output_video_format == videoFormatNames[i].format) &&
            !sensitive) {
            current_not_available = TRUE;
        }
    }
    
    /*
     * if the current video is not available, then make the first
     * available format current
     */
    
    if (current_not_available && bitmask) {
        for (i = 0; videoFormatNames[i].name; i++) {
            if (((videoFormatNames[i].format < 32) &&
                 ((1 << videoFormatNames[i].format) & bitmask)) ||
                ((videoFormatNames[i].format >= 32) &&
                 ((1 << (videoFormatNames[i].format - 32)) & bitmask2))) {
                
                NvCtrlSetAttribute(ctk_gvo->handle,
                                   NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                                   videoFormatNames[i].format);
                
                ctk_drop_down_menu_set_current_value
                    (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu),
                     videoFormatNames[i].format);
                
                ctk_gvo->output_video_format = videoFormatNames[i].format;

                break;
            }
        }
    }

    /*
     * cache the bitmask
     */

    ctk_gvo->valid_output_video_format_mask[0] = bitmask;
    ctk_gvo->valid_output_video_format_mask[1] = bitmask2;
    
} /* update_output_video_format_menu() */



/*
 * init_sync_format_menu() - initialize the sync format menu
 */

static void init_sync_format_menu(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint sync_source, sync_mode, comp_mode, val;
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_SYNC_SOURCE,
                             &sync_source);
    
    if (ret != NvCtrlSuccess) {
        sync_source = NV_CTRL_GVO_SYNC_SOURCE_SDI;
    }

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_SYNC_MODE,
                             &sync_mode);
    
    if (ret != NvCtrlSuccess) {
        sync_mode = NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING;
    }
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle, 
                             NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE,
                             &comp_mode);
    
    if (ret != NvCtrlSuccess) {
        comp_mode = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_AUTO;
    }
    
    if (sync_source == NV_CTRL_GVO_SYNC_SOURCE_SDI) {
        val = SYNC_FORMAT_SDI;
    } else if (comp_mode ==
               NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_AUTO) {
        val = SYNC_FORMAT_COMP_AUTO;
    } else if (comp_mode ==
               NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_BI_LEVEL) {
        val = SYNC_FORMAT_COMP_BI_LEVEL;
    } else if (comp_mode ==
               NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_TRI_LEVEL){
        val = SYNC_FORMAT_COMP_TRI_LEVEL;
    } else {
        val = SYNC_FORMAT_SDI; // should not get here
    }
         
    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->sync_format_menu), val);
    
    if (sync_mode == NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) {
        ctk_gvo->sync_format_sensitive = FALSE;
    } else {
        ctk_gvo->sync_format_sensitive = TRUE;
    }

    ctk_gvo->sync_source = sync_source;

    gtk_widget_set_sensitive(ctk_gvo->sync_format_menu,
                             ctk_gvo->sync_format_sensitive);

} /* init_sync_format_menu() */


static void update_sync_format_menu(CtkGvo *ctk_gvo)
{
    if (ctk_gvo->sync_mode == NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) {
        ctk_gvo->sync_format_sensitive = FALSE;
    } else {
        ctk_gvo->sync_format_sensitive = TRUE;
    }
    
    gtk_widget_set_sensitive(ctk_gvo->sync_format_menu,
                             ctk_gvo->sync_format_sensitive);
}

/*
 * init_output_video_format_menu() - initialize the output video
 * format menu: set the currently active menu entry and update the
 * sensitivity of all entries.
 */

static void init_output_video_format_menu(CtkGvo *ctk_gvo)
{
    gint val;
    ReturnStatus ret;

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC;
    }
    
    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu), val);
    
    ctk_gvo->output_video_format = val;

    update_output_video_format_menu(ctk_gvo);
    
} /* init_output_video_format_menu() */



/*
 * init_output_data_format_menu() - initialize the output data format
 * menu: make sure the current value is available in the list that we
 * expose, and set the currently active menu entry.
 */

static void init_output_data_format_menu(CtkGvo *ctk_gvo)
{
    gint current, i;
    ReturnStatus ret;
    gboolean found;
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DATA_FORMAT,
                             &current);
    
    if (ret != NvCtrlSuccess) {
        current = NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444;
    }
    
    /*
     * is this value in the list that we expose? if not, then pick the
     * first value that is in the list
     */
    
    for (i = 0, found = FALSE; dataFormatNames[i].name; i++) {
        if (current == dataFormatNames[i].format) {
            found = TRUE;
            break;
        }
    }
    
    if (!found) {
        current = dataFormatNames[0].format;
    
        /* make this value current */
    
        NvCtrlSetAttribute(ctk_gvo->handle,
                           NV_CTRL_GVO_DATA_FORMAT, current);
    }
    
    /* update the menu */
            
    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_data_format_menu), current);
    
} /* init_output_data_format_menu() */




/**************************************************************************/



/*
 * create_toggle_sdi_output_button() - 
 */

static void create_toggle_sdi_output_button(CtkGvo *ctk_gvo, gboolean enabled)
{
    GtkWidget *label;
    GtkWidget *hbox, *hbox2;
    GdkPixbuf *pixbuf;
    GtkWidget *image = NULL;
    GtkWidget *button;
    
    button = gtk_toggle_button_new();
    
    /* create the Enable SDI Output icon */
    
    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_EXECUTE,
                                    GTK_ICON_SIZE_BUTTON,
                                    "Enable SDI Output");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Enable SDI Output");
    
    hbox = gtk_hbox_new(FALSE, 2);

    if (image) gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, FALSE, 15);

    gtk_widget_show_all(hbox2);
    
    /*
     * XXX increment the reference count, so that when we do
     * gtk_container_remove() later, it doesn't get destroyed
     */
    
    gtk_object_ref(GTK_OBJECT(hbox2));
    
    ctk_gvo->enable_sdi_output_label = hbox2;

    
    /* create the Disable SDI Output icon */
    
    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_STOP,
                                    GTK_ICON_SIZE_BUTTON,
                                    "Disable SDI Output");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Disable SDI Output");
    
    hbox = gtk_hbox_new(FALSE, 2);
    
    if (image) gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    hbox2 = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), hbox, FALSE, FALSE, 15);

    gtk_widget_show_all(hbox2);
    
    /*
     * XXX increment the reference count, so that when we do
     * gtk_container_remove() later, it doesn't get destroyed
     */

    gtk_object_ref(GTK_OBJECT(hbox2));
    
    ctk_gvo->disable_sdi_output_label = hbox2;
    
    /* Set the initial SDI Output state */
    
    gtk_container_add(GTK_CONTAINER(button),
                      enabled ? ctk_gvo->disable_sdi_output_label :
                      ctk_gvo->enable_sdi_output_label);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), enabled);

    ctk_gvo->toggle_sdi_output_button = button;

    ctk_gvo->sdi_output_enabled = enabled;

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(toggle_sdi_output_button),
                     GTK_OBJECT(ctk_gvo));
    
} /* create_toggle_sdi_output_button() */



/*
 * toggle_sdi_output_button() - 
 */

static void toggle_sdi_output_button(GtkWidget *button, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gboolean enabled;
    ReturnStatus ret;
    gint val, val2;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    
    if (enabled) val = NV_CTRL_GVO_DISPLAY_X_SCREEN_ENABLE;
    else         val = NV_CTRL_GVO_DISPLAY_X_SCREEN_DISABLE;

    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DISPLAY_X_SCREEN, val);
    
    /*
     * XXX NV_CTRL_GVO_DISPLAY_X_SCREEN can silently fail if GLX
     * locked GVO output for use by pbuffer(s).  Check that the
     * setting actually stuck.
     */
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_DISPLAY_X_SCREEN, &val2);
    
    if ((ret != NvCtrlSuccess) || (val != val2)) {

        /*
         * setting did not apply; restore the button to its previous
         * state
         */
        
        g_signal_handlers_block_matched
            (G_OBJECT(button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(toggle_sdi_output_button), NULL);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !enabled);

        g_signal_handlers_unblock_matched
            (G_OBJECT(button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(toggle_sdi_output_button), NULL);

        // XXX update the status bar; maybe pop up a dialog box?

        return;
    }
    
    post_toggle_sdi_output_button(ctk_gvo, enabled);

} /* toggle_sdi_output_button() */


static void post_toggle_sdi_output_button(CtkGvo *ctk_gvo, gboolean enabled)
{
    GList *children;
    GList *child;

    children = gtk_container_get_children
        (GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button));

    for (child = children; child; child = child->next) {
        gtk_container_remove
            (GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
             (GtkWidget *) child->data);
    }

    g_list_free(children);
    

    if (enabled) {
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                          ctk_gvo->disable_sdi_output_label);

        update_gvo_banner_video_output(&ctk_gvo->banner,
                                       ctk_gvo->output_video_format,
                                       ctk_gvo->output_data_format);
        
        update_gvo_current_info(ctk_gvo, NV_CTRL_GVO_LOCK_OWNER_CLONE);
        
    } else {
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                          ctk_gvo->enable_sdi_output_label);
        
        update_gvo_banner_video_output(&ctk_gvo->banner,
                                       NV_CTRL_GVO_VIDEO_FORMAT_NONE,
                                       ctk_gvo->output_data_format);
        
        update_gvo_current_info(ctk_gvo, NV_CTRL_GVO_LOCK_OWNER_NONE);
    }
    
    
    ctk_gvo->sdi_output_enabled = enabled;
    
    ctk_config_statusbar_message(ctk_gvo->ctk_config, "SDI Output %s.",
                                 enabled ? "enabled" : "disabled");
}



/*
 * sync_format_changed() - 
 */

static void sync_format_changed(CtkDropDownMenu *menu, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint i, value, sync_source, mode;
    const char *name = "Unknown";

    value = ctk_drop_down_menu_get_current_value(menu);
    
    switch (value) {
    case SYNC_FORMAT_SDI:
        sync_source = NV_CTRL_GVO_SYNC_SOURCE_SDI;
        mode = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_AUTO;
        break;
    case SYNC_FORMAT_COMP_AUTO:
        sync_source = NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE;
        mode = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_AUTO;
        break;
    case SYNC_FORMAT_COMP_BI_LEVEL:
        sync_source = NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE;
        mode = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_BI_LEVEL;
        break;
    case SYNC_FORMAT_COMP_TRI_LEVEL:
        sync_source = NV_CTRL_GVO_SYNC_SOURCE_COMPOSITE;
        mode = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE_TRI_LEVEL;
        break;
    default:
        return;
    }
    
    for (i = 0; syncFormatNames[i].name; i++) {
        if (syncFormatNames[i].format == value) {
            name = syncFormatNames[i].name;
            break;
        }
    }

    ctk_gvo->sync_source = sync_source;

    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_SOURCE, sync_source);
    NvCtrlSetAttribute(ctk_gvo->handle, 
                       NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE, mode);

    ctk_config_statusbar_message(ctk_gvo->ctk_config, "Sync Format set to "
                                 "\"%s\".", name);
    
} /* sync_format_changed() */



/*
 * update_input_video_format_text_entry() - 
 */

static void update_input_video_format_text_entry(CtkGvo *ctk_gvo)
{
    gint i;
    const gchar *str;
    
    if (ctk_gvo->sync_mode == NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING) {
        str = "Free Running";
    } else {
        str = "No incoming signal detected";
        for (i = 0; videoFormatNames[i].name; i++) {
            if (videoFormatNames[i].format == ctk_gvo->input_video_format) {
                str = videoFormatNames[i].name;
            }
        }
    }
    gtk_entry_set_text(GTK_ENTRY(ctk_gvo->input_video_format_text_entry), str);
    
} /* update_input_video_format_text_entry() */



/*
 * init_input_video_format_text_entry() - 
 */

static void init_input_video_format_text_entry(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint val;

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_INPUT_VIDEO_FORMAT, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_VIDEO_FORMAT_NONE;
    }
    
    ctk_gvo->input_video_format = val;

    update_input_video_format_text_entry(ctk_gvo);
    
} /* init_input_video_format_text_entry() */



/*
 * init_composite_termination() - 
 */

static void init_composite_termination(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint val;

    if (!ctk_gvo->composite_termination_button) return;

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_COMPOSITE_TERMINATION, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_COMPOSITE_TERMINATION_DISABLE;
    }
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_gvo->composite_termination_button),
         (val == NV_CTRL_GVO_COMPOSITE_TERMINATION_ENABLE));

} /* init_composite_termination() */



/*
 * max_input_video_format_text_entry_length()
 */

static int max_input_video_format_text_entry_length(void)
{
    gint i, tmp, max = 0;

    for (i = 0; videoFormatNames[i].name; i++) {
        tmp = strlen(videoFormatNames[i].name);
        if (max < tmp) max = tmp;
    }

    return max;

} /* max_input_video_format_text_entry_length() */



/*
 * post_toggle_composite_termination() - Call this function after
 * the composite termination attribute has changed.
 */

static void post_toggle_composite_termination(CtkGvo *ctk_gvo,
                                              gboolean enabled)
{
    /* update the statusbar */
    
    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Composite Termination %s.",
                                 enabled ? "Enabled" : "Disabled");

} /* post_toggle_composite_termination() */



/*
 * toggle_composite_termination() - Called when the user clicks
 * on the "Enable Composite Termination" check button.
 */

static void toggle_composite_termination(GtkWidget *button,
                                         CtkGvo *ctk_gvo)
{
    gboolean enabled;
    
    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_COMPOSITE_TERMINATION,
                       enabled ? NV_CTRL_GVO_COMPOSITE_TERMINATION_ENABLE :
                       NV_CTRL_GVO_COMPOSITE_TERMINATION_DISABLE);

    post_toggle_composite_termination(ctk_gvo, enabled);

} /* toggle_composite_termination() */



/*
 * detect_input() - if the Detect button is enabled, then enable
 * INPUT_VIDEO_FORMAT_REACQUIRE, make everything else insensitive,
 * switch to the wait cursor, and queue detect_input_done() to be
 * called.
 */

static void detect_input(GtkToggleButton *togglebutton, CtkGvo *ctk_gvo)
{
    gboolean enabled;

    enabled = gtk_toggle_button_get_active(togglebutton);

    if (!enabled) return;
        
    /* grab the server */
    
    gtk_grab_add(ctk_gvo->input_video_format_detect_button);

    /* change the cursor */

    gdk_window_set_cursor
        ((GTK_WIDGET(ctk_gvo->parent_window))->window,
         ctk_gvo->wait_cursor);
    
    /* make all other widgets insensitive */

    set_gvo_sensitivity(ctk_gvo, FALSE,
                        SET_SENSITIVITY_EXCLUDE_DETECT_BUTTON);
    
    /* enable REACQUIRE */

    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE,
                       NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_TRUE);
    
    /* update the statusbar */

    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Detecting incoming signal...");
    
    /* register the "done" function */
    
    g_timeout_add(DEFAULT_DETECT_INPUT_TIME_INTERVAL,
                  detect_input_done, (gpointer) ctk_gvo);
    
} /* detect_input() */



/*
 * detect_input_done() - done detecting; disable REACQUIRE, make all
 * widgets sensitive again, and probe the new state
 */

static gint detect_input_done(gpointer data)
{
    CtkGvo *ctk_gvo = CTK_GVO(data);
    
    /* disable REACQUIRE */

    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE,
                       NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_FALSE);
    
    /* reprobe */
    
    ctk_gvo_probe((gpointer) ctk_gvo);
    
    /* un-press the detect button */
    
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_gvo->input_video_format_detect_button),
         G_CALLBACK(detect_input),
         (gpointer) ctk_gvo);
    
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_gvo->input_video_format_detect_button), FALSE);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_gvo->input_video_format_detect_button),
         G_CALLBACK(detect_input),
         (gpointer) ctk_gvo);
    
    /* update the status bar */
    
    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Done detecting incoming signal.");
    
    /* restore sensitivity */
    
    set_gvo_sensitivity(ctk_gvo, TRUE,
                        SET_SENSITIVITY_EXCLUDE_DETECT_BUTTON);

    /* restore the cursor */

    gdk_window_set_cursor((GTK_WIDGET(ctk_gvo->parent_window))->window,
                          NULL);

    /* ungrab the server */
    
    gtk_grab_remove(ctk_gvo->input_video_format_detect_button);
    
    return FALSE;

} /* detect_input_done() */




/**************************************************************************/


/*
 * get_video_format_name() - return the name of the given video format
 */

static const char *get_video_format_name(const gint format)
{
    gint i;
    
    for (i = 0; videoFormatNames[i].name; i++) {
        if (videoFormatNames[i].format == format) {
            return videoFormatNames[i].name;
        }
    }

    return "Unknown";
    
} /* get_video_format_name() */



/*
 * get_data_format_name() - return the name of the given data format
 */

static const char *get_data_format_name(const gint format)
{
    gint i;
    
    for (i = 0; dataFormatNames[i].name; i++) {
        if (dataFormatNames[i].format == format) {
            return dataFormatNames[i].name;
        }
    }

    return "Unknown";

} /* get_data_format_name() */



/*
 * get_video_format_resolution() - return the width and height of the
 * given video format
 */

static void get_video_format_resolution(const gint format, gint *w, gint *h)
{
    gint i;
    
    *w = *h = 0;

    for (i = 0; videoFormatDetails[i].format != -1; i++) {
        if (videoFormatDetails[i].format == format) {
            *w = videoFormatDetails[i].width;
            *h = videoFormatDetails[i].height;
            return;
        }
    }
} /* get_video_format_resolution() */


/*
 * set_gvo_sensitivity() - set the sensitivity of all the GVO widgets
 */

static void set_gvo_sensitivity(CtkGvo *ctk_gvo,
                                gboolean sensitive, guint flags)
{
    gtk_widget_set_sensitive(ctk_gvo->sync_mode_menu,           sensitive);
    gtk_widget_set_sensitive(ctk_gvo->output_video_format_menu, sensitive);
    gtk_widget_set_sensitive(ctk_gvo->output_data_format_menu,  sensitive);
    gtk_widget_set_sensitive(ctk_gvo->hsync_delay_spin_button,  sensitive);
    gtk_widget_set_sensitive(ctk_gvo->vsync_delay_spin_button,  sensitive);
    
    if (!(flags & SET_SENSITIVITY_EXCLUDE_ROI_BUTTONS)) {
        gtk_widget_set_sensitive(ctk_gvo->x_offset_spin_button, sensitive);
        gtk_widget_set_sensitive(ctk_gvo->y_offset_spin_button, sensitive);
    }
    
    if (!(flags & SET_SENSITIVITY_EXCLUDE_ENABLE_BUTTON)) {
        gtk_widget_set_sensitive(ctk_gvo->toggle_sdi_output_button, sensitive);
    }
    
    if (!(flags & SET_SENSITIVITY_EXCLUDE_DETECT_BUTTON)) {
        gtk_widget_set_sensitive(ctk_gvo->input_video_format_detect_button,
                                 sensitive);
    }
    
    if (!sensitive || (sensitive && ctk_gvo->sync_format_sensitive)) {
        gtk_widget_set_sensitive(ctk_gvo->sync_format_menu, sensitive);
    }

} /* set_gvo_sensitivity() */



/*
 * get_video_format_details() - initialize the videoFormatDetails[]
 * table by querying each of refresh rate, width, and height from
 * NV-CONTROL.
 */

static void get_video_format_details(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint i, val;

    for (i = 0; videoFormatDetails[i].format != -1; i++) {
        
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVO_VIDEO_FORMAT_REFRESH_RATE,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].rate = val;
        
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVO_VIDEO_FORMAT_WIDTH,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].width = val;
                                       
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVO_VIDEO_FORMAT_HEIGHT,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].height = val; 
    }
} /* get_video_format_details() */


/**************************************************************************/


/*
 * update_gvo_current_info() - Updates the state information to reflect
 * the GVO device's current state.
 */

static void update_gvo_current_info(CtkGvo *ctk_gvo, guint lock_owner)
{
    int width;
    int height;
    gchar res_string[64], state_string[64];
    gboolean sensitive = FALSE;
    guint flags = 0;

    if (!ctk_gvo->current_resolution_label) return;
    if (!ctk_gvo->current_state_label) return;

    /* Get the current video format sizes */
    get_video_format_resolution(ctk_gvo->output_video_format, &width, &height);
    
    switch (lock_owner) {
        
    case NV_CTRL_GVO_LOCK_OWNER_NONE:
        snprintf(res_string, 64, "Inactive");
        snprintf(state_string, 64, "Inactive");
        sensitive = TRUE;
        break;

    case NV_CTRL_GVO_LOCK_OWNER_CLONE:
        snprintf(res_string, 64, "%d x %d", width, height);
        snprintf(state_string, 64, "In use by X (Clone mode)");
        flags = SET_SENSITIVITY_EXCLUDE_ENABLE_BUTTON |
            SET_SENSITIVITY_EXCLUDE_ROI_BUTTONS;
        break;
        
    case NV_CTRL_GVO_LOCK_OWNER_X_SCREEN:
        snprintf(res_string, 64, "%d x %d", width, height);
        snprintf(state_string, 64, "In use by X");
        break;

    case NV_CTRL_GVO_LOCK_OWNER_GLX:
        snprintf(res_string, 64, "%d x %d", width, height);
        snprintf(state_string, 64, "In use by GLX");
        break;
        
    default:
        return;
    }
        
    gtk_label_set_text(GTK_LABEL(ctk_gvo->current_resolution_label),
                       res_string);

    gtk_label_set_text(GTK_LABEL(ctk_gvo->current_state_label),
                       state_string);

    set_gvo_sensitivity(ctk_gvo, sensitive, flags);

} /* update_gvo_current_info() */



/*
 * query_gvo_current_info() - Queries the state of the GVO device
 * from the X server and updates the information on the page.
 */

static void query_gvo_current_info(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    gint lock_owner;

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_LOCK_OWNER, &lock_owner);

    if (ret == NvCtrlSuccess) {
        update_gvo_current_info(ctk_gvo, lock_owner);
    }

} /* query_gvo_current_info() */



/*
 * update_delay_spin_buttons() - 
 */

static void update_delay_spin_buttons(CtkGvo *ctk_gvo)
{
    gint w, h;
    
    get_video_format_resolution(ctk_gvo->input_video_format, &w, &h);

    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->hsync_delay_spin_button), 0, w);
    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->vsync_delay_spin_button), 0, h);
    
} /* update_delay_spin_buttons() */



/*
 * hsync_delay_changed()
 */

static void hsync_delay_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_DELAY_PIXELS, val);
    
} /* hsync_delay_changed() */


/*
 * vsync_delay_changed()
 */

static void vsync_delay_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_DELAY_LINES, val);
    
} /* vsync_delay_changed() */



static void update_offset_spin_buttons(CtkGvo *ctk_gvo)
{
    gint w, h, x, y;
    
    get_video_format_resolution(ctk_gvo->output_video_format, &w, &h);

    x = ctk_gvo->screen_width - w;
    y = ctk_gvo->screen_height - h;

    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->x_offset_spin_button), 0, x);
    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->y_offset_spin_button), 0, y);
    
} /* update_delay_spin_buttons() */



/*
 * x_offset_changed()
 */

static void x_offset_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_X_SCREEN_PAN_X, val);
    
} /* x_offset_changed() */



/*
 * y_offset_changed()
 */

static void y_offset_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_X_SCREEN_PAN_Y, val);
    
} /* y_offset_changed() */


/**************************************************************************/

/*
 * ctk_gvo_probe() - query the incoming signal
 */

gint ctk_gvo_probe(gpointer data)
{
    ReturnStatus ret;
    gint input, sdi, comp;
    CtkGvo *ctk_gvo = CTK_GVO(data);

    /* query NV_CTRL_GVO_INPUT_VIDEO_FORMAT */
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_INPUT_VIDEO_FORMAT, &input);
    
    if (ret != NvCtrlSuccess) {
        input = NV_CTRL_GVO_VIDEO_FORMAT_NONE;
    }
    
    ctk_gvo->input_video_format = input;

    /* query COMPOSITE_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED, &comp);
    
    if (ret != NvCtrlSuccess) {
        comp = NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED_FALSE;
    }
    
    /* query SDI_SYNC_INPUT_DETECTED */

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED, &sdi);
    
    if (ret != NvCtrlSuccess) {
        sdi = NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED_NONE;
    }
    
    /* update the input video format text entry */
    
    update_input_video_format_text_entry(ctk_gvo);
    
    if (!ctk_gvo->sdi_output_enabled) {
        
        /* update the available SyncModes */
        
        update_sync_mode_menu(ctk_gvo, comp, sdi, ctk_gvo->sync_mode);    
        
        /* update the available output video formats */
        
        update_output_video_format_menu(ctk_gvo);    
    }
    
    /* update whatever data will be needed to change the LED colors */

    update_gvo_banner_video_input(&ctk_gvo->banner, sdi, comp);
    
    /* update the range for the sync delay spin buttons */

    update_delay_spin_buttons(ctk_gvo);
    update_offset_spin_buttons(ctk_gvo);

    return TRUE;
    
} /* ctk_gvo_probe() */


/**************************************************************************/

static void register_for_gvo_events(CtkGvo *ctk_gvo)
{
    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_MODE),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_SOURCE),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_DATA_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_DELAY_PIXELS),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_SYNC_DELAY_LINES),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);
    
    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_X_SCREEN_PAN_X),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);
    
    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_X_SCREEN_PAN_Y),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);
    
    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_DISPLAY_X_SCREEN),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_COMPOSITE_TERMINATION),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_LOCK_OWNER),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);
}


static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    ReturnStatus ret;
    GtkWidget *widget;    
    gint attribute = event_struct->attribute;
    gint value = event_struct->value;

    switch (attribute) {
    case NV_CTRL_GVO_SYNC_MODE:
    case NV_CTRL_GVO_SYNC_SOURCE:
    case NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE:
        // XXX implement me
        break;
        
    case NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT:
        widget = ctk_gvo->output_video_format_menu;
        
        g_signal_handlers_block_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_video_format_changed),
             (gpointer) ctk_gvo);
        
        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(widget), value);
        
        post_output_video_format_changed(ctk_gvo, value);
        query_gvo_current_info(ctk_gvo);
        
        g_signal_handlers_unblock_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_video_format_changed),
             (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_DATA_FORMAT:
        widget = ctk_gvo->output_data_format_menu;
        
        g_signal_handlers_block_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_data_format_changed),
             (gpointer) ctk_gvo);
        
        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(widget), value);
        
        post_output_data_format_changed(ctk_gvo, value);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_data_format_changed),
             (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_SYNC_DELAY_PIXELS:
        widget = ctk_gvo->hsync_delay_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(hsync_delay_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(hsync_delay_changed),
                                          (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_SYNC_DELAY_LINES:
        widget = ctk_gvo->vsync_delay_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(vsync_delay_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(vsync_delay_changed),
                                          (gpointer) ctk_gvo);
        break;

    case NV_CTRL_GVO_X_SCREEN_PAN_X:
        widget = ctk_gvo->x_offset_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(x_offset_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(x_offset_changed),
                                          (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_X_SCREEN_PAN_Y:
        widget = ctk_gvo->y_offset_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(y_offset_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(y_offset_changed),
                                          (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_LOCK_OWNER:
        update_gvo_current_info(ctk_gvo, value);
        break;

    case NV_CTRL_GVO_DISPLAY_X_SCREEN:
        
        /*
         * XXX explicitly re-query this value, since a change may not
         * actually have been successfully applied (if GLX or some other
         * mode already had GVO locked)
         */

        ret = NvCtrlGetAttribute(ctk_gvo->handle,
                                 NV_CTRL_GVO_DISPLAY_X_SCREEN, &value);
        
        if (ret == NvCtrlSuccess) {

            widget = ctk_gvo->toggle_sdi_output_button;
            
            g_signal_handlers_block_by_func
                (G_OBJECT(widget),
                 G_CALLBACK(toggle_sdi_output_button),
                 ctk_gvo);
            
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);

            post_toggle_sdi_output_button(ctk_gvo, value);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(widget),
                 G_CALLBACK(toggle_sdi_output_button),
                 ctk_gvo);
        }
        break;

    case NV_CTRL_GVO_COMPOSITE_TERMINATION:
        widget = ctk_gvo->composite_termination_button;

        if (widget) {
            g_signal_handlers_block_by_func
                (G_OBJECT(widget),
                 G_CALLBACK(toggle_composite_termination),
                 ctk_gvo);
            
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), value);
            
            post_toggle_composite_termination(ctk_gvo, value);
            
            g_signal_handlers_unblock_by_func
                (G_OBJECT(widget),
                 G_CALLBACK(toggle_composite_termination),
                 ctk_gvo);
        }
        break;

    }
}


/**************************************************************************/


void ctk_gvo_select(GtkWidget *widget)
{
    CtkGvo *ctk_gvo = CTK_GVO(widget);

    /* Greb the GVO parent banner */

    gtk_container_add(GTK_CONTAINER(ctk_gvo->banner_box),
                      ctk_gvo->banner.widget);

    ctk_config_start_timer(ctk_gvo->ctk_config, (GSourceFunc) ctk_gvo_probe,
                           (gpointer) ctk_gvo);
}



void ctk_gvo_unselect(GtkWidget *widget)
{
    CtkGvo *ctk_gvo = CTK_GVO(widget);

    /* Return the GVO parent banner */

    gtk_container_remove(GTK_CONTAINER(ctk_gvo->banner_box),
                         ctk_gvo->banner.widget);

    ctk_config_stop_timer(ctk_gvo->ctk_config, (GSourceFunc) ctk_gvo_probe,
                          (gpointer) ctk_gvo);
}



GtkTextBuffer* ctk_gvo_create_help (GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "GVO (Graphics to Video Out)");

    ctk_help_finish(b);

    return b;
}


