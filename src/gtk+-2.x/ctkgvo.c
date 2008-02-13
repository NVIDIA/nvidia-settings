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

#include "NvCtrlAttributes.h"

#include "ctkhelp.h"
#include "ctkgvo.h"
#include "ctkdropdownmenu.h"

#include "gvo_banner_left.h"

#include "gvo_banner_vid1_green.h"
#include "gvo_banner_vid1_grey.h"
#include "gvo_banner_vid1_red.h"
#include "gvo_banner_vid1_yellow.h"

#include "gvo_banner_vid2_green.h"
#include "gvo_banner_vid2_grey.h"
#include "gvo_banner_vid2_red.h"
#include "gvo_banner_vid2_yellow.h"

#include "gvo_banner_sdi_sync_green.h"
#include "gvo_banner_sdi_sync_grey.h"
#include "gvo_banner_sdi_sync_red.h"
#include "gvo_banner_sdi_sync_yellow.h"

#include "gvo_banner_comp_sync_green.h"
#include "gvo_banner_comp_sync_grey.h"
#include "gvo_banner_comp_sync_red.h"
#include "gvo_banner_comp_sync_yellow.h"

#include "gvo_banner_right.h"

#include "msg.h"

/* local prototypes */

static void init_gvo_banner(CtkGvo *ctk_gvo);

static void pack_gvo_banner(CtkGvo *ctk_gvo);


static GtkWidget *start_menu(const gchar *name, GtkWidget *table,
                             const gint row);

static void finish_menu(GtkWidget *menu, GtkWidget *table, const gint row);


static void sync_mode_changed(CtkDropDownMenu *menu, gpointer user_data);

static void output_video_format_changed(CtkDropDownMenu *menu,
                                        gpointer user_data);

static void output_data_format_changed(CtkDropDownMenu *menu,
                                       gpointer user_data);


static void update_sync_mode_menu(CtkGvo *ctk_gvo,
                                  const gint composite_sync,
                                  const gint sdi_sync,
                                  const gint current);

static void init_sync_mode_menu(CtkGvo *ctk_gvo);

static void update_output_video_format_menu(CtkGvo *ctk_gvo,
                                            const gint current);

static void init_output_video_format_menu(CtkGvo *ctk_gvo);

static void init_output_data_format_menu(CtkGvo *ctk_gvo);


static void create_toggle_sdi_output_button(CtkGvo *ctk_gvo);

static void toggle_sdi_output_button(GtkWidget *button, gpointer user_data);

static void sync_format_changed(CtkDropDownMenu *menu, gpointer user_data);

static void
update_input_video_format_text_entry(CtkGvo *ctk_gvo,
                                     const gint input_video_format);

static void init_input_video_format_text_entry(CtkGvo *ctk_gvo);

static void
input_video_format_detect_button_toggled(GtkToggleButton *togglebutton,
                                         CtkGvo *ctk_gvo);


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

static const FormatName videoFormatNames[] = {
    { NV_CTRL_GVO_VIDEO_FORMAT_480I_59_94_SMPTE259_NTSC, "480i    59.94  Hz  (SMPTE259) NTSC"},
    { NV_CTRL_GVO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL,  "576i    50.00  Hz  (SMPTE259) PAL"},
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_23_98_SMPTE296,      "720p    23.98  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_24_00_SMPTE296,      "720p    24.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_25_00_SMPTE296,      "720p    25.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_29_97_SMPTE296,      "720p    29.97  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_30_00_SMPTE296,      "720p    30.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_50_00_SMPTE296,      "720p    50.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_59_94_SMPTE296,      "720p    59.94  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_720P_60_00_SMPTE296,      "720p    60.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_59_94_SMPTE260,     "1035i   59.94  Hz  (SMPTE260)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1035I_60_00_SMPTE260,     "1035i   60.00  Hz  (SMPTE260)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE295,     "1080i   50.00  Hz  (SMPTE295)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_50_00_SMPTE274,     "1080i   50.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_59_94_SMPTE274,     "1080i   59.94  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_60_00_SMPTE274,     "1080i   60.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_23_976_SMPTE274,    "1080p   23.976 Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_24_00_SMPTE274,     "1080p   24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_25_00_SMPTE274,     "1080p   25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_29_97_SMPTE274,     "1080p   29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080P_30_00_SMPTE274,     "1080p   30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_47_96_SMPTE274,     "1080i   47.96  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080I_48_00_SMPTE274,     "1080i   48.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274,   "1080PsF 25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274,   "1080PsF 29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274,   "1080PsF 30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274,   "1080PsF 24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274,   "1080PsF 23.98  Hz  (SMPTE274)"    },
    { -1, NULL },
};


static const FormatName dataFormatNames[] = {
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444, "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422, "RGB -> YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_RGB444,   "RGB (4:4:4)" },
    { -1, NULL },
};



#define TABLE_PADDING 5


#define SYNC_FORMAT_SDI 0
#define SYNC_FORMAT_COMP_AUTO 1
#define SYNC_FORMAT_COMP_BI_LEVEL 2
#define SYNC_FORMAT_COMP_TRI_LEVEL 3


/*
 * ctk_gvo_new() - constructor for the CtkGvo widget
 */

GtkWidget* ctk_gvo_new(NvCtrlAttributeHandle *handle,
                       CtkConfig *ctk_config)
{
    GObject *object;
    CtkGvo *ctk_gvo;
    GtkWidget *hbox, *alignment, *button;
    ReturnStatus ret;
    int val, i, screen_width, screen_height, width, height, n;
    
    GtkWidget *frame, *table, *menu;
    
    /* make sure we have a handle */
    
    g_return_val_if_fail(handle != NULL, NULL);
    
    /* check if this screen supports GVO */
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_SUPPORTED, &val);
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_GVO_SUPPORTED_TRUE)) {
        /* GVO not available */
        return NULL;
    }
    
    /* create the CtkGvo object */
    
    object = g_object_new(CTK_TYPE_GVO, NULL);
    
    /* initialize fields in the CtkGvo object */
    
    ctk_gvo = CTK_GVO(object);
    ctk_gvo->handle = handle;
    ctk_gvo->ctk_config = ctk_config;
    
    /* set container properties for the CtkGvo widget */
    
    gtk_box_set_spacing(GTK_BOX(ctk_gvo), 10);

    /* banner */
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);
    
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
    
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    ctk_gvo->banner_table = gtk_table_new(1, 6, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(ctk_gvo->banner_table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(ctk_gvo->banner_table), 0);

    gtk_container_add(GTK_CONTAINER(frame), ctk_gvo->banner_table);

    init_gvo_banner(ctk_gvo);
    pack_gvo_banner(ctk_gvo);

    
    /*
     * Output options
     */
    
    frame = gtk_frame_new("Output Options");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    table = gtk_table_new(3, 2, FALSE);
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
    
    
    /* Output Video Format */

    menu = start_menu("Output Video Format: ", table, 1);
    
    screen_width = NvCtrlGetScreenWidth(handle);
    screen_height = NvCtrlGetScreenHeight(handle);
    
    n = 0;
    
    for (i = 0; videoFormatNames[i].name; i++) {
        
        /* query the width/height needed for the video format */

        NvCtrlGetDisplayAttribute(handle,
                                  videoFormatNames[i].format,
                                  NV_CTRL_GVO_VIDEO_FORMAT_WIDTH,
                                  &width);

        NvCtrlGetDisplayAttribute(handle,
                                  videoFormatNames[i].format,
                                  NV_CTRL_GVO_VIDEO_FORMAT_HEIGHT,
                                  &height);
        
        if ((width > screen_width) ||
            (height > screen_height)) {

            nv_warning_msg("Not exposing GVO video format '%s' (this video "
                           "format requires a resolution of atleast %d x %d, "
                           "but the current X screen is %d x %d)",
                           videoFormatNames[i].name,
                           width, height, screen_width, screen_height);
            continue;
        }

        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       videoFormatNames[i].name,
                                       videoFormatNames[i].format);
        n++;
    }

    finish_menu(menu, table, 1);
    
    if (!n) {
        nv_warning_msg("No GVO video formats will fit the current X screen of "
                       "%d x %d; please create an X screen of atleast "
                       "720 x 487; not exposing GVO page.\n",
                       screen_width, screen_height);
        return NULL;
    }


    ctk_gvo->output_video_format_menu = menu;
    
    init_output_video_format_menu(ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->output_video_format_menu),
                     "changed", G_CALLBACK(output_video_format_changed),
                     (gpointer) ctk_gvo);
    
    
    /* Output Data Format */
    
    menu = start_menu("Output Data Format: ", table, 2);
    
    for (i = 0; dataFormatNames[i].name; i++) {
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       dataFormatNames[i].name,
                                       dataFormatNames[i].format);
    }
    
    finish_menu(menu, table, 2);
    
    ctk_gvo->output_data_format_menu = menu;
    
    g_signal_connect(G_OBJECT(ctk_gvo->output_data_format_menu),
                     "changed", G_CALLBACK(output_data_format_changed),
                     (gpointer) ctk_gvo);
    
    init_output_data_format_menu(ctk_gvo);
    

    /*
     * Sync options
     */
    
    frame = gtk_frame_new("Sync Options");
    
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), hbox);
    
    /* menu */

    menu = ctk_drop_down_menu_new(0);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu), "SDI Sync",
                                   SYNC_FORMAT_SDI);

    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu), "COMP Sync",
                                   SYNC_FORMAT_COMP_AUTO);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                   "COMP Sync (Bi-level)",
                                   SYNC_FORMAT_COMP_BI_LEVEL);
    
    ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                   "COMP Sync (Tri-level)",
                                   SYNC_FORMAT_COMP_TRI_LEVEL);
    
    ctk_drop_down_menu_finalize(CTK_DROP_DOWN_MENU(menu));
    
    gtk_box_pack_start(GTK_BOX(hbox), menu, FALSE, FALSE, TABLE_PADDING);

    ctk_gvo->sync_format_menu = menu;

    /* input type */

    ctk_gvo->input_video_format_text_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), ctk_gvo->input_video_format_text_entry,
                       TRUE, TRUE, TABLE_PADDING);
    gtk_widget_set_sensitive(ctk_gvo->input_video_format_text_entry, FALSE);

    /* XXX maybe set the text entry to the maximum format name length */

    init_input_video_format_text_entry(ctk_gvo);
    
    g_signal_connect(G_OBJECT(ctk_gvo->sync_format_menu),
                     "changed", G_CALLBACK(sync_format_changed),
                     (gpointer) ctk_gvo);

    /* detect button */
    
    button = gtk_toggle_button_new_with_label("Detect");
    
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, TABLE_PADDING);
    
    ctk_gvo->input_video_format_detect_button = button;
    
    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(input_video_format_detect_button_toggled),
                     ctk_gvo);

    
    /* "Enable SDI Output" button */

    create_toggle_sdi_output_button(ctk_gvo);

    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment),
                      ctk_gvo->toggle_sdi_output_button);
    
    gtk_box_pack_start(GTK_BOX(object), alignment, TRUE, TRUE, 0);

    /* show the GVO widget */
    
    gtk_widget_show_all(GTK_WIDGET(ctk_gvo));

    return GTK_WIDGET(ctk_gvo);

} /* ctk_gvo_new() */



/**************************************************************************/


#define GVO_BANNER_LEFT 0
#define GVO_BANNER_VID1_GREEN 1
#define GVO_BANNER_VID1_GREY 2
#define GVO_BANNER_VID1_RED 3
#define GVO_BANNER_VID1_YELLOW 4
#define GVO_BANNER_VID2_GREEN 5
#define GVO_BANNER_VID2_GREY 6
#define GVO_BANNER_VID2_RED 7
#define GVO_BANNER_VID2_YELLOW 8
#define GVO_BANNER_SDI_SYNC_GREEN 9
#define GVO_BANNER_SDI_SYNC_GREY 10
#define GVO_BANNER_SDI_SYNC_RED 11
#define GVO_BANNER_SDI_SYNC_YELLOW 12
#define GVO_BANNER_COMP_SYNC_GREEN 13
#define GVO_BANNER_COMP_SYNC_GREY 14
#define GVO_BANNER_COMP_SYNC_RED 15
#define GVO_BANNER_COMP_SYNC_YELLOW 16
#define GVO_BANNER_RIGHT 17

#define GVO_BANNER_COUNT 18


static GtkWidget *__gvo_banner_widget[GVO_BANNER_COUNT];

static const nv_image_t *__gvo_banner_imgs[GVO_BANNER_COUNT] = {
    &gvo_banner_left_image,
    &gvo_banner_vid1_green_image,
    &gvo_banner_vid1_grey_image,
    &gvo_banner_vid1_red_image,
    &gvo_banner_vid1_yellow_image,
    &gvo_banner_vid2_green_image,
    &gvo_banner_vid2_grey_image,
    &gvo_banner_vid2_red_image,
    &gvo_banner_vid2_yellow_image,
    &gvo_banner_sdi_sync_green_image,
    &gvo_banner_sdi_sync_grey_image,
    &gvo_banner_sdi_sync_red_image,
    &gvo_banner_sdi_sync_yellow_image,
    &gvo_banner_comp_sync_green_image,
    &gvo_banner_comp_sync_grey_image,
    &gvo_banner_comp_sync_red_image,
    &gvo_banner_comp_sync_yellow_image,
    &gvo_banner_right_image,
};

static void init_gvo_banner(CtkGvo *ctk_gvo)
{
    int i;
    const nv_image_t *img;
    guint8 *image_buffer = NULL;
    
    for (i = 0; i < GVO_BANNER_COUNT; i++) {
        img = __gvo_banner_imgs[i];

        image_buffer = decompress_image_data(img);
        
        __gvo_banner_widget[i] = gtk_image_new_from_pixbuf
            (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                      FALSE, 8, img->width, img->height,
                                      img->width * img->bytes_per_pixel,
                                      free_decompressed_image, NULL));
    }
}

static void pack_gvo_banner_slot(CtkGvo *ctk_gvo, int n,
                                 int banner_image_index)
{
    GtkWidget *image = __gvo_banner_widget[banner_image_index];
    
    gtk_table_attach(GTK_TABLE(ctk_gvo->banner_table),
                     image, n, n+1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
}


static void pack_gvo_banner(CtkGvo *ctk_gvo)
{
    pack_gvo_banner_slot(ctk_gvo, 0, GVO_BANNER_LEFT);
    pack_gvo_banner_slot(ctk_gvo, 1, GVO_BANNER_VID1_RED);
    pack_gvo_banner_slot(ctk_gvo, 2, GVO_BANNER_VID2_GREY);
    pack_gvo_banner_slot(ctk_gvo, 3, GVO_BANNER_SDI_SYNC_GREEN);
    pack_gvo_banner_slot(ctk_gvo, 4, GVO_BANNER_COMP_SYNC_YELLOW);
    pack_gvo_banner_slot(ctk_gvo, 5, GVO_BANNER_RIGHT);
}



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



static void sync_mode_changed(CtkDropDownMenu *menu, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    /* XXX confirm that the value is valid */

    //printf("%s(): %d\n", __FUNCTION__, value);

    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_MODE, value);
    
    /* XXX statusbar */

    /* XXX update the output video format menu */
}

static void output_video_format_changed(CtkDropDownMenu *menu,
                                        gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    /* XXX confirm that the value is valid */
    
    //printf("%s(): %d\n", __FUNCTION__, value);

    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT, value);
    
    /* XXX statusbar */
}

static void output_data_format_changed(CtkDropDownMenu *menu,
                                       gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value;
    
    value = ctk_drop_down_menu_get_current_value(menu);
    
    /* XXX confirm that the value is valid */
    
    //printf("%s(): %d\n", __FUNCTION__, value);


    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DATA_FORMAT, value);
    
    /* XXX statusbar */
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
        sdi = NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED;
    }

    /* update the menu */

    update_sync_mode_menu(ctk_gvo, comp, sdi, val);
    
} /* init_sync_mode_menu() */



/*
 * update_output_video_format_menu() - given the current
 * OUTPUT_VIDEO_FORMAT, set the sensitivity of each menu entry and
 * possibly update which entry is current.
 */

static void update_output_video_format_menu(CtkGvo *ctk_gvo,
                                            const gint current)
{
    ReturnStatus ret;
    NVCTRLAttributeValidValuesRec valid;
    gint bitmask, i;
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
    
    /*
     * loop over each video format; if that format is set in the
     * bitmask, then it is available
     */

    for (i = 0; videoFormatNames[i].name; i++) {
        if ((1 << videoFormatNames[i].format) & bitmask) {
            sensitive = TRUE;
        } else {
            sensitive = FALSE;
        }

        ctk_drop_down_menu_set_value_sensitive
            (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu),
             videoFormatNames[i].format, sensitive);

        /* if the current video is not sensitive, take note */
        
        if ((current == videoFormatNames[i].format) && !sensitive) {
            current_not_available = TRUE;
        }
    }
    
    /*
     * if the current video is not available, then make the first
     * available format current
     */

    if (current_not_available && bitmask) {
        for (i = 0; videoFormatNames[i].name; i++) {
            if ((1 << videoFormatNames[i].format) & bitmask) {
                
                NvCtrlSetAttribute(ctk_gvo->handle,
                                   NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                                   videoFormatNames[i].format);
                
                ctk_drop_down_menu_set_current_value
                    (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu),
                     videoFormatNames[i].format);
                
                break;
            }
        }
    }
} /* update_output_video_format_menu() */



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
    
    update_output_video_format_menu(ctk_gvo, val);
    
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

static void create_toggle_sdi_output_button(CtkGvo *ctk_gvo)
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
    
    /* start with SDI Output disabled */
    
    gtk_container_add(GTK_CONTAINER(button), ctk_gvo->enable_sdi_output_label);
    

    ctk_gvo->toggle_sdi_output_button = button;

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
    
    if (enabled) {
        gtk_container_remove(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                             ctk_gvo->enable_sdi_output_label);
        
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                          ctk_gvo->disable_sdi_output_label);
    } else {
        gtk_container_remove(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                             ctk_gvo->disable_sdi_output_label);
        
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_sdi_output_button),
                          ctk_gvo->enable_sdi_output_label);
    }
    
    gtk_widget_set_sensitive(ctk_gvo->sync_mode_menu, !enabled);
    gtk_widget_set_sensitive(ctk_gvo->output_video_format_menu, !enabled);
    gtk_widget_set_sensitive(ctk_gvo->output_data_format_menu, !enabled);

        
    // XXX set sensitivity

    // status bar
    
} /* toggle_sdi_output_button() */



/*
 * sync_format_changed() - 
 */

static void sync_format_changed(CtkDropDownMenu *menu, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint value, sync_source, mode;

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
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_SOURCE, sync_source);
    NvCtrlSetAttribute(ctk_gvo->handle, 
                       NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE, mode);

} /* sync_format_changed() */



/*
 * update_input_video_format_text_entry() - 
 */

static void update_input_video_format_text_entry(CtkGvo *ctk_gvo,
                                                 const gint input_video_format)
{
    gint i;
    const gchar *str = "No Signal Detected";

    for (i = 0; videoFormatNames[i].name; i++) {
        if (videoFormatNames[i].format == input_video_format) {
            str = videoFormatNames[i].name;
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
    
    update_input_video_format_text_entry(ctk_gvo, val);
    
} /* init_input_video_format_text_entry() */



/*
 * input_video_format_detect_button_toggled() - 
 */

static void
input_video_format_detect_button_toggled(GtkToggleButton *togglebutton,
                                         CtkGvo *ctk_gvo)
{
    if (gtk_toggle_button_get_active(togglebutton)) {
        
        /* XXX need to make the other widgets insensitive */

        NvCtrlSetAttribute(ctk_gvo->handle,
                           NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE,
                           NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_TRUE);

        /*
         * need to register a timer to automatically turn this off for
         * 2 seconds
         */

    } else {
        
        /* XXX need to make the other widgets sensitive */

        NvCtrlSetAttribute(ctk_gvo->handle,
                           NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE,
                           NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE_FALSE);
    }
} /* input_video_format_detect_button_toggled() */


/**************************************************************************/



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
