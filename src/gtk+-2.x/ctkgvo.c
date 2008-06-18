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

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "NvCtrlAttributes.h"

#include "ctkhelp.h"
#include "ctkgvo.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"
#include "ctkbanner.h"

#include "ctkgvo-banner.h"

#include "msg.h"



#define TABLE_PADDING 5


/* Some defaults */

#define DEFAULT_OUTPUT_VIDEO_FORMAT \
    NV_CTRL_GVO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC

#define DEFAULT_OUTPUT_DATA_FORMAT \
    NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444


/* General information help */

static const char * __general_firmware_version_help =
"The Firmware Version reports the version of the firmware running on the "
"SDI device.";

static const char * __general_current_sdi_resolution_help =
"The Current SDI Resolution reports the current active resolution that the "
"SDI device is driving or 'Inactive' if SDI is currently disabled.";

static const char * __general_current_sdi_state_help =
"The Current SDI state reports on the current usage of the SDI device.";


/* Clone mode help */

static const char * __clone_mode_video_format_help =
"The Video Format drop-down allows you to select the desired resolution and "
"refresh rate to be used for Clone Mode.";

static const char * __clone_mode_data_format_help =
"The Data Format drop-down allows you to select the desired data format that "
"the SDI will output.";

static const char * __clone_mode_x_offset_help =
"The X Offset determines the start location of the left side of SDI output "
"window when in Clone Mode.";

static const char * __clone_mode_y_offset_help =
"The Y Offset determines the start location of the top of the SDI output "
"window when in Clone Mode.";

static const char * __clone_mode_enable_clone_mode_help =
"The Enable Clone Mode button toggles SDI Clone mode.";



/* local prototypes */

static gboolean query_init_gvo_state(CtkGvo *ctk_gvo);

static void query_video_format_details(CtkGvo *ctk_gvo);

static void register_for_gvo_events(CtkGvo *ctk_gvo);


static GtkWidget *start_menu(const gchar *name, GtkWidget *table,
                             const gint row);
static void finish_menu(GtkWidget *menu, GtkWidget *table, const gint row);


static void fill_output_video_format_menu(CtkGvo *ctk_gvo);
static void trim_output_video_format_menu(CtkGvo *ctk_gvo);
static void output_video_format_ui_changed(CtkDropDownMenu *menu,
                                           gpointer user_data);
static void post_output_video_format_changed(CtkGvo *ctk_gvo);


static void validate_output_data_format(CtkGvo *ctk_gvo);
static void output_data_format_ui_changed(CtkDropDownMenu *menu,
                                          gpointer user_data);
static void post_output_data_format_changed(CtkGvo *ctk_gvo);


static void x_offset_ui_changed(GtkSpinButton *spinbutton, gpointer user_data);
static void y_offset_ui_changed(GtkSpinButton *spinbutton, gpointer user_data);


static void create_toggle_clone_mode_button(CtkGvo *ctk_gvo);
static void clone_mode_button_ui_toggled(GtkWidget *button,
                                         gpointer user_data);
static void post_clone_mode_button_toggled(CtkGvo *gvo);


static void update_gvo_current_info(CtkGvo *ctk_gvo);
static void update_offset_spin_button_ranges(CtkGvo *ctk_gvo);
static void update_gvo_sensitivity(CtkGvo *ctk_gvo);


static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data);
static void screen_changed_handler(GtkWidget *widget,
                                   XRRScreenChangeNotifyEvent *ev,
                                   gpointer data);


/*
 * video format table -- should this be moved into NV-CONTROL?
 */

const GvoFormatName videoFormatNames[] = {
    { NV_CTRL_GVO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC, "720  x 487i    59.94  Hz  (SMPTE259) NTSC"},
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


static GvoFormatDetails videoFormatDetails[] = {
    { NV_CTRL_GVO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC, 0, 0, 0 },
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


static const GvoFormatName dataFormatNames[] = {
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444, "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422, "RGB -> YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8_444_PASSTHRU,"RGB (4:4:4)" },
    { -1, NULL },
};



/**** Utility Functions ******************************************************/


/*
 * get_first_available_output_video_format() - returns the first available
 * output video format from the dropdown menu.  This is needed since when
 * Frame Lock/Genlock are enabled, the default case of 487i may not be
 * appropriate.
 */

static gint get_first_available_output_video_format(CtkGvo *ctk_gvo)
{
    CtkDropDownMenu *dmenu =
        CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu);
    int i;

    /* look through the output video format dropdown for the first
     * available selection.
     */
    for (i = 0; i < dmenu->num_entries; i++) {
        if (GTK_WIDGET_IS_SENSITIVE(dmenu->values[i].menu_item)) {
            return dmenu->values[i].value;
        }
    }

    /* There are no available video formats?  Fallback to 487i. */

    return NV_CTRL_GVO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC;

} /* get_first_available_output_video_format() */



/*
 * ctk_gvo_get_video_format_name() - return the name of the given video format
 */

const char *ctk_gvo_get_video_format_name(const gint format)
{
    gint i;
    
    for (i = 0; videoFormatNames[i].name; i++) {
        if (videoFormatNames[i].format == format) {
            return videoFormatNames[i].name;
        }
    }

    return "Unknown";
    
} /* ctk_gvo_get_video_format_name() */



/*
 * ctk_gvo_get_data_format_name() - return the name of the given data format
 */

const char *ctk_gvo_get_data_format_name(const gint format)
{
    gint i;
    
    for (i = 0; dataFormatNames[i].name; i++) {
        if (dataFormatNames[i].format == format) {
            return dataFormatNames[i].name;
        }
    }

    return "Unknown";

} /* ctk_gvo_get_data_format_name() */



/*
 * ctk_gvo_get_video_format_resolution() - return the width and height of the
 * given video format
 */

void ctk_gvo_get_video_format_resolution(const gint format, gint *w, gint *h)
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
} /* ctk_gvo_get_video_format_resolution() */



/*
 * ctk_gvo_get_type() - Returns the CtkGvo "class" type
 */

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
 * trim_output_video_format_menu() - given the current
 * OUTPUT_VIDEO_FORMAT and SYNC_MODE, set the sensitivity of each menu
 * entry and possibly update which of the output video mode dropdown
 * entries is currently selected.
 */

static void trim_output_video_format_menu(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    NVCTRLAttributeValidValuesRec valid;
    gint bitmask, bitmask2, i, refresh_rate;
    gboolean sensitive, current_available;

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

        /* Get the current input refresh rate */
        for (i = 0; videoFormatDetails[i].format != -1; i++) {
            if (videoFormatDetails[i].format == ctk_gvo->input_video_format) {
                refresh_rate = videoFormatDetails[i].rate;
                break;
            }
        }
        
        /* Mask out refresh rates that don't match */
        for (i = 0; videoFormatDetails[i].format != -1; i++) {
            gboolean match = FALSE;

            if (videoFormatDetails[i].rate == refresh_rate) {
                match = TRUE;

            } else if (ctk_gvo->caps &
                       NV_CTRL_GVO_CAPABILITIES_MULTIRATE_SYNC) {

                /* Some GVO devices support multi-rate synchronization.
                 * For these devices, we just need to check that the 
                 * fractional part of the rate are either both zero or
                 * both non-zero.
                 */
                if (((videoFormatDetails[i].rate % 1000) ? TRUE : FALSE) ==
                    ((refresh_rate % 1000) ? TRUE : FALSE)) {
                    match = TRUE;
                }
            } else {
                match = FALSE;
            }

            if (!match) {
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

    current_available = FALSE;
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
            sensitive) {
            current_available = TRUE;
        }
    }
    
    /*
     * if the current video is not available, then make the first
     * available format current
     */
    
    if (!current_available && bitmask) {

        for (i = 0; videoFormatNames[i].name; i++) {
            if (((videoFormatNames[i].format < 32) &&
                 ((1 << videoFormatNames[i].format) & bitmask)) ||
                ((videoFormatNames[i].format >= 32) &&
                 ((1 << (videoFormatNames[i].format - 32)) & bitmask2))) {
                
                /* Invalidate the format so it gets set when clone mode is
                 * enabled.
                 */
                ctk_gvo->output_video_format_valid = FALSE;

                g_signal_handlers_block_by_func
                    (G_OBJECT(ctk_gvo->output_video_format_menu),
                     G_CALLBACK(output_video_format_ui_changed),
                     (gpointer) ctk_gvo);

                ctk_drop_down_menu_set_current_value
                    (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu),
                     videoFormatNames[i].format);

                g_signal_handlers_unblock_by_func
                    (G_OBJECT(ctk_gvo->output_video_format_menu),
                     G_CALLBACK(output_video_format_ui_changed),
                     (gpointer) ctk_gvo);
                break;
            }
        }
    }

    /*
     * cache the bitmask
     */

    ctk_gvo->valid_output_video_format_mask[0] = bitmask;
    ctk_gvo->valid_output_video_format_mask[1] = bitmask2;
    
} /* trim_output_video_format_menu() */



/*
 * validate_output_video_format() - Keep tabs on the output
 * video format.  If some other client sets the output video format to
 * something clone mode can't do, we'll just need to make sure that
 * we re-set the output video format when the user selects to enable
 * clone mode.
 *
 * NOTE: The gtk signal handler for the video format menu should not
 *       be enabled while calling this function.
 */

static void validate_output_video_format(CtkGvo *ctk_gvo)
{
    gint bitmask, bitmask2;
    gint width, height;
    gint format = ctk_gvo->output_video_format;

    /* Keep track of whether we'll need to re-set the video format
     * when enabling clone mode.
     */

    /* Check to make sure the format size <= current screen size */
    ctk_gvo_get_video_format_resolution(format, &width, &height);

    /* Don't expose modes bigger than the current X Screen size */
    if ((width > ctk_gvo->screen_width) ||
        (height > ctk_gvo->screen_height)) {
        /* Format is invalid due to screen size limitations */
        ctk_gvo->output_video_format_valid = FALSE;
        return;
    }

    /* Check to make sure genlock/framelock requirements are ment */
    bitmask = ctk_gvo->valid_output_video_format_mask[0];
    bitmask2 = ctk_gvo->valid_output_video_format_mask[1];

    if (((format < 32) && !((1 << format) & bitmask)) ||
        ((format >= 32) && !((1 << (format - 32)) & bitmask2))) {
        /* Format is invalid due to genlock/framelock requirements */
        ctk_gvo->output_video_format_valid = FALSE;
        return;
    }

    /* If we got this far, then the format is valid. */
    ctk_gvo->output_video_format_valid = TRUE;

} /* validate_output_video_format() */



/*
 * validate_output_data_format() - Keep tabs on the output
 * data format.  If some other client sets the output data format to
 * something clone mode can't do, we'll just need to make sure that
 * we re-set the output data format when the user selects to enable
 * clone mode.
 *
 * NOTE: The gtk signal handler for the data format menu should not
 *       be enabled while calling this function.
 */

static void validate_output_data_format(CtkGvo *ctk_gvo)
{
    int i;

    /* Keep track of whether we'll need to re-set the data format
     * when enabling clone mode.
     */
    for (i = 0; dataFormatNames[i].name; i++) {
        if (ctk_gvo->output_data_format == dataFormatNames[i].format) {
            /* Value is OK */
            ctk_gvo->output_data_format_valid = TRUE;
            return;
        }
    }

    ctk_gvo->output_data_format_valid = FALSE;

} /* validate_output_data_format() */



/**** Creation Functions *****************************************************/

/*
 * ctk_gvo_new() - constructor for the CtkGvo widget
 */

GtkWidget* ctk_gvo_new(NvCtrlAttributeHandle *handle,
                       CtkConfig *ctk_config,
                       CtkEvent *ctk_event)
{
    GObject *object;
    CtkGvo *ctk_gvo;
    GtkWidget *hbox, *vbox, *alignment, *label;
    ReturnStatus ret;
    gchar scratch[64], *firmware, *string;
    gint val, i, width, height;
    
    GtkWidget *frame, *table, *menu;
    
    /* make sure we have a handle */
    
    g_return_val_if_fail(handle != NULL, NULL);

    /* create and initialize the object */
    
    object = g_object_new(CTK_TYPE_GVO, NULL);
    
    ctk_gvo = CTK_GVO(object);
    ctk_gvo->handle = handle;
    ctk_gvo->ctk_config = ctk_config;
    ctk_gvo->ctk_event = ctk_event;
    
    /* Query the current GVO state */

    if ( !query_init_gvo_state(ctk_gvo) ) {
        // Free the object
        g_object_ref(object);
        gtk_object_sink(GTK_OBJECT(object));
        g_object_unref(object);
        return NULL;
    }

    /* Query the width, height and refresh rate for each video format */

    query_video_format_details(ctk_gvo);

    /* set container properties for the widget */
    
    gtk_box_set_spacing(GTK_BOX(ctk_gvo), 10);

    /* banner */
    
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(object), hbox, FALSE, FALSE, 0);

    ctk_gvo->banner_box = hbox;

    ctk_gvo->banner = ctk_gvo_banner_new(handle, ctk_config, ctk_event);
    g_object_ref(ctk_gvo->banner);

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
     * Clone mode options
     */
    
    frame = gtk_frame_new("Clone Mode");
    ctk_gvo->clone_mode_frame = frame;

    vbox = gtk_vbox_new(FALSE, 0);

    table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 0);
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    
    /* Output Video Format */

    menu = start_menu("Video Format: ", table, 0);
    ctk_gvo->output_video_format_menu = menu;
    ctk_config_set_tooltip(ctk_config, CTK_DROP_DOWN_MENU(menu)->option_menu,
                           __clone_mode_video_format_help);
    
    fill_output_video_format_menu(ctk_gvo);

    finish_menu(menu, table, 0);
    
    /* Make sure that the video format selected is valid for clone mode */

    validate_output_video_format(ctk_gvo);
    if (ctk_gvo->output_video_format_valid) {
        val = ctk_gvo->output_video_format;
    } else {
        val = get_first_available_output_video_format(ctk_gvo);
    }

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu), val);

    g_signal_connect(G_OBJECT(ctk_gvo->output_video_format_menu),
                     "changed", G_CALLBACK(output_video_format_ui_changed),
                     (gpointer) ctk_gvo);
    
    /* Output Data Format */
    
    menu = start_menu("Data Format: ", table, 1);
    ctk_gvo->output_data_format_menu = menu;
    ctk_config_set_tooltip(ctk_config, CTK_DROP_DOWN_MENU(menu)->option_menu,
                           __clone_mode_data_format_help);
    
    for (i = 0; dataFormatNames[i].name; i++) {
        ctk_drop_down_menu_append_item(CTK_DROP_DOWN_MENU(menu),
                                       dataFormatNames[i].name,
                                       dataFormatNames[i].format);
    }
    
    finish_menu(menu, table, 1);

    /* Make sure that the data format selected is valid for clone mode */

    validate_output_data_format(ctk_gvo);
    if (ctk_gvo->output_data_format_valid) {
        val = ctk_gvo->output_data_format;
    } else {
        val = DEFAULT_OUTPUT_DATA_FORMAT;
    }
    
    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_data_format_menu), val);

    g_signal_connect(G_OBJECT(ctk_gvo->output_data_format_menu),
                     "changed", G_CALLBACK(output_data_format_ui_changed),
                     (gpointer) ctk_gvo);

    /* Region of Interest */

    ctk_gvo_get_video_format_resolution(ctk_gvo->output_video_format,
                                        &width, &height);

    /* NV_CTRL_GVO_X_SCREEN_PAN_X */

    label = gtk_label_new("X Offset: ");
    
    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);

    gtk_table_attach(GTK_TABLE(table), alignment,
                     0, 1,  2, 3,
                     GTK_FILL, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_X_SCREEN_PAN_X, &val);
    if (ret != NvCtrlSuccess) val = 0;

    width = ctk_gvo->screen_width - width;
    if (width < 1) width = 1;

    ctk_gvo->x_offset_spin_button =
        gtk_spin_button_new_with_range(0.0, width, 1);
    
    ctk_config_set_tooltip(ctk_config, ctk_gvo->x_offset_spin_button,
                           __clone_mode_x_offset_help);
    
    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->x_offset_spin_button), val);

    g_signal_connect(G_OBJECT(ctk_gvo->x_offset_spin_button),
                     "value-changed",
                     G_CALLBACK(x_offset_ui_changed), ctk_gvo);

    hbox = gtk_hbox_new(FALSE, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_gvo->x_offset_spin_button,
                       FALSE, FALSE, 0);
    
    gtk_table_attach(GTK_TABLE(table), hbox,
                     1, 2,  2, 3,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);

    /* NV_CTRL_GVO_X_SCREEN_PAN_Y */

    label = gtk_label_new("Y Offset: ");
    alignment = gtk_alignment_new(0, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);

    gtk_table_attach(GTK_TABLE(table), alignment,
                     0, 1,  3, 4,
                     GTK_FILL, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
    
    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_X_SCREEN_PAN_Y, &val);
    if (ret != NvCtrlSuccess) val = 0;

    height = ctk_gvo->screen_height - height;
    if (height < 1) height = 1;

    ctk_gvo->y_offset_spin_button =
        gtk_spin_button_new_with_range(0.0, height, 1);
    
    gtk_spin_button_set_value
        (GTK_SPIN_BUTTON(ctk_gvo->y_offset_spin_button), val);

    ctk_config_set_tooltip(ctk_config, ctk_gvo->y_offset_spin_button,
                           __clone_mode_y_offset_help);

    g_signal_connect(G_OBJECT(ctk_gvo->y_offset_spin_button),
                     "value-changed",
                     G_CALLBACK(y_offset_ui_changed), ctk_gvo);

    hbox = gtk_hbox_new(FALSE, FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), ctk_gvo->y_offset_spin_button,
                       FALSE, FALSE, 0);
    
    gtk_table_attach(GTK_TABLE(table), hbox,
                     1, 2,  3, 4,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
    
    /*
     * "Enable Clone Mode" button
     */

    create_toggle_clone_mode_button(ctk_gvo);

    ctk_config_set_tooltip(ctk_config, ctk_gvo->toggle_clone_mode_button,
                           __clone_mode_enable_clone_mode_help);

    hbox = gtk_hbox_new(FALSE, 0);

    gtk_box_pack_end(GTK_BOX(hbox), ctk_gvo->toggle_clone_mode_button,
                     FALSE, FALSE, 5);

    gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);


    /*
     * Set the initial UI state
     */

    update_gvo_current_info(ctk_gvo);

    update_gvo_sensitivity(ctk_gvo);


    /*
     * Start listening for events
     */

    register_for_gvo_events(ctk_gvo);

    /* show the GVO widget */
    
    gtk_widget_show_all(GTK_WIDGET(ctk_gvo));

    return GTK_WIDGET(ctk_gvo);

} /* ctk_gvo_new() */



/*
 * create_toggle_clone_mode_button() - 
 */

static void create_toggle_clone_mode_button(CtkGvo *ctk_gvo)
{
    GtkWidget *label;
    GtkWidget *hbox, *hbox2;
    GdkPixbuf *pixbuf;
    GtkWidget *image = NULL;
    GtkWidget *button;
    gboolean enabled;
    
    button = gtk_toggle_button_new();
    
    /* create the Enable Clone Mode icon */
    
    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_EXECUTE,
                                    GTK_ICON_SIZE_BUTTON,
                                    "Enable Clone Mode");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Enable Clone Mode");
    
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
    
    ctk_gvo->enable_clone_mode_label = hbox2;

    
    /* create the Disable Clone Mode icon */
    
    pixbuf = gtk_widget_render_icon(button,
                                    GTK_STOCK_STOP,
                                    GTK_ICON_SIZE_BUTTON,
                                    "Disable Clone Mode");
    if (pixbuf) image = gtk_image_new_from_pixbuf(pixbuf);
    label = gtk_label_new("Disable Clone Mode");
    
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
    
    ctk_gvo->disable_clone_mode_label = hbox2;
    
    /* Set the initial Clone Mode enabled state */

    enabled = (ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_CLONE);
    
    gtk_container_add(GTK_CONTAINER(button),
                      enabled ? ctk_gvo->disable_clone_mode_label :
                      ctk_gvo->enable_clone_mode_label);
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), enabled);

    ctk_gvo->toggle_clone_mode_button = button;

    g_signal_connect(G_OBJECT(button), "toggled",
                     G_CALLBACK(clone_mode_button_ui_toggled),
                     GTK_OBJECT(ctk_gvo));
    
} /* create_toggle_clone_mode_button() */



/*
 * start_menu() - Start the creation of a labled dropdown menu.  (Packs
 * the dropdown label into the table row.
 */

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

} /* start_menu() */



/*
 * finish_menu() - Finish/Finalize a dropdown menu. (Packs the menu in
 * the table row.)
 */

static void finish_menu(GtkWidget *menu, GtkWidget *table, const gint row)
{
    ctk_drop_down_menu_finalize(CTK_DROP_DOWN_MENU(menu));

    gtk_table_attach(GTK_TABLE(table), menu, 1, 2, row, row+1,
                     GTK_FILL | GTK_EXPAND, GTK_FILL,
                     TABLE_PADDING, TABLE_PADDING);
}



/*
 * fill_output_video_format_menu() - Populates the output video
 * format menu.
 */

static void fill_output_video_format_menu(CtkGvo *ctk_gvo)
{
    int i;
    int width, height;
    CtkDropDownMenu *dmenu =
        CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu);
   
    for (i = 0; videoFormatNames[i].name; i++) {
        
        /* 
         * runtime check that videoFormatDetails[] and
         * videoFormatNames[] are in sync
         */
        
        if (videoFormatDetails[i].format != videoFormatNames[i].format) {
            nv_error_msg("GVO format tables out of alignment!");
            return;
        }
        
        /* check that the current X screen can support the width and height */
        
        width = videoFormatDetails[i].width;
        height = videoFormatDetails[i].height;

        /* Don't expose modes bigger than the current X Screen size */
        if ((width > ctk_gvo->screen_width) ||
            (height > ctk_gvo->screen_height)) {
            continue;
        }

        ctk_drop_down_menu_append_item(dmenu,
                                       videoFormatNames[i].name,
                                       videoFormatNames[i].format);
    }
    
    ctk_gvo->has_output_video_formats =
        ((dmenu->num_entries > 0) ? TRUE : FALSE);

    if (!ctk_gvo->has_output_video_formats) {
        nv_warning_msg("No GVO video formats will fit the current X screen of "
                       "%d x %d; please create an X screen of atleast "
                       "720 x 487; not exposing GVO page.\n",
                       ctk_gvo->screen_width, ctk_gvo->screen_height);

        ctk_drop_down_menu_append_item
            (dmenu,
             "*** X Screen is smaller than 720x487 ***",
             0);
    }

    /* Trim output video format based on sync mode */
    trim_output_video_format_menu(ctk_gvo);

} /* fill_output_video_format_menu() */



/**** Initialization Functions ***********************************************/

/*
 * query_init_gvo_state() - Query the initial GVO state so we can setup
 * the UI correctly.
 */

static gboolean query_init_gvo_state(CtkGvo *ctk_gvo)
{
    gint val;
    ReturnStatus ret;


    /* Check if this screen supports GVO */
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SUPPORTED, &val);
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_GVO_SUPPORTED_TRUE)) {
        /* GVO not available */
        return FALSE;
    }

    /* Get this GVO device's capabilities */
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_CAPABILITIES, &val);
    if (ret != NvCtrlSuccess) return FALSE;
    ctk_gvo->caps = val;

    /* Query the current GVO lock owner (GVO enabled/disabled) */

    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_LOCK_OWNER, &val);
    if (ret != NvCtrlSuccess) return FALSE;
    ctk_gvo->lock_owner = val;

    /* Query the sync mode */

    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_SYNC_MODE, &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_SYNC_MODE_FREE_RUNNING;
    }
    ctk_gvo->sync_mode = val;

    /* Query the current input/output video formats */

    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_INPUT_VIDEO_FORMAT,
                             &val);
    if (ret != NvCtrlSuccess) {
        val = NV_CTRL_GVO_VIDEO_FORMAT_NONE;
    }
    ctk_gvo->input_video_format = val;
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                             &val);
    if (ret != NvCtrlSuccess) {
        val = DEFAULT_OUTPUT_VIDEO_FORMAT;
    }
    ctk_gvo->output_video_format = val;

    /* Output data format */

    ret = NvCtrlGetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DATA_FORMAT, &val);
    if (ret != NvCtrlSuccess) {
        val = DEFAULT_OUTPUT_DATA_FORMAT;
    }
    ctk_gvo->output_data_format = val;

    /* Get the current screen dimensions */

    ctk_gvo->screen_width = NvCtrlGetScreenWidth(ctk_gvo->handle);
    ctk_gvo->screen_height = NvCtrlGetScreenHeight(ctk_gvo->handle);

    return TRUE;

} /* query_init_gvo_state() */



/*
 * query_video_format_details() - initialize the videoFormatDetails[]
 * table by querying each of refresh rate, width, and height from
 * NV-CONTROL.
 */

static void query_video_format_details(CtkGvo *ctk_gvo)
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

} /* query_video_format_details() */



/*
 * register_for_gvo_events() - Configure ctk_gvo object to listen for
 * GVO related evens.
 */

static void register_for_gvo_events(CtkGvo *ctk_gvo)
{
    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_DATA_FORMAT),
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
                     CTK_EVENT_NAME(NV_CTRL_GVO_LOCK_OWNER),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    /* Ask for screen change notify events so we can
     * know when to reconstruct the output video format drop down
     */

    g_signal_connect(G_OBJECT(ctk_gvo->ctk_event),
                     "CTK_EVENT_RRScreenChangeNotify",
                     G_CALLBACK(screen_changed_handler),
                     (gpointer) ctk_gvo);

} /* register_for_gvo_events() */



/**** User Interface Update Functions ****************************************/

/*
 * output_video_format_ui_changed() - callback when the user makes a
 * selection from the output video format menu (from the UI.)
 */

static void output_video_format_ui_changed(CtkDropDownMenu *menu,
                                           gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);

    ctk_gvo->output_video_format = ctk_drop_down_menu_get_current_value(menu);
    ctk_gvo->output_video_format_valid = TRUE;

    NvCtrlSetAttribute(ctk_gvo->handle,
                       NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                       ctk_gvo->output_video_format);

    post_output_video_format_changed(ctk_gvo);
    
} /* output_video_format_ui_changed() */



/*
 * output_data_format_ui_changed() - callback when the output data format
 * menu changes
 */

static void output_data_format_ui_changed(CtkDropDownMenu *menu,
                                          gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    
    ctk_gvo->output_data_format = ctk_drop_down_menu_get_current_value(menu);
    ctk_gvo->output_data_format_valid = TRUE;
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DATA_FORMAT,
                       ctk_gvo->output_data_format);
    
    post_output_data_format_changed(ctk_gvo);
    
} /* output_data_format_ui_changed() */



/*
 * x_offset_ui_changed() - Updates the X Server with the current setting of
 * the spin button.
 */

static void x_offset_ui_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_X_SCREEN_PAN_X, val);
    
} /* x_offset_ui_changed() */



/*
 * y_offset_ui_changed() - Updates the X Server with the current setting of
 * the spin button.
 */

static void y_offset_ui_changed(GtkSpinButton *spinbutton, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gint val;
    
    val = gtk_spin_button_get_value(spinbutton);
    
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_X_SCREEN_PAN_Y, val);
    
} /* y_offset_ui_changed() */



/*
 * clone_mode_button_ui_toggled() - Updates the X server to enable/disable
 * clone mode when the UI button is toggled.
 */

static void clone_mode_button_ui_toggled(GtkWidget *button, gpointer user_data)
{
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    gboolean enabled;
    ReturnStatus ret;
    gint val;

    enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    

    /*
     * When enabling clone mode, we must make sure that the output
     * video format and output data format are something clone mode
     * can support.
     */

    if (enabled) {
        if (!ctk_gvo->output_video_format_valid) {
            val = ctk_drop_down_menu_get_current_value
                (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu));
            ctk_gvo->output_video_format = val;
            ctk_gvo->output_video_format_valid = TRUE;
            NvCtrlSetAttribute(ctk_gvo->handle,
                               NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,
                               ctk_gvo->output_video_format);
        }
        if (!ctk_gvo->output_data_format_valid) {
            val = ctk_drop_down_menu_get_current_value
                (CTK_DROP_DOWN_MENU(ctk_gvo->output_data_format_menu));
            ctk_gvo->output_data_format = val;
            ctk_gvo->output_data_format_valid = TRUE;
            NvCtrlSetAttribute(ctk_gvo->handle,
                               NV_CTRL_GVO_DATA_FORMAT,
                               val);
        }
    }

    if (enabled) val = NV_CTRL_GVO_DISPLAY_X_SCREEN_ENABLE;
    else         val = NV_CTRL_GVO_DISPLAY_X_SCREEN_DISABLE;
    NvCtrlSetAttribute(ctk_gvo->handle, NV_CTRL_GVO_DISPLAY_X_SCREEN, val);
    
    /*
     * XXX NV_CTRL_GVO_DISPLAY_X_SCREEN can silently fail if GLX
     * locked GVO output for use by pbuffer(s).  Check that the
     * setting actually stuck.
     */
    
    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_LOCK_OWNER,
                             &ctk_gvo->lock_owner);

    if ((ret != NvCtrlSuccess) ||
        (enabled &&
         (ctk_gvo->lock_owner != NV_CTRL_GVO_LOCK_OWNER_CLONE))) {

        /*
         * setting did not apply; restore the button to its previous
         * state
         */
        
        g_signal_handlers_block_matched
            (G_OBJECT(button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(clone_mode_button_ui_toggled), NULL);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !enabled);

        g_signal_handlers_unblock_matched
            (G_OBJECT(button), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
             G_CALLBACK(clone_mode_button_ui_toggled), NULL);

        // XXX update the status bar; maybe pop up a dialog box?
        return;
    }
    
    post_clone_mode_button_toggled(ctk_gvo);

} /* clone_mode_button_ui_toggled() */



/**** Common Update Functions ************************************************/

/*
 * post_output_video_format_changed() - Makes sure the CPL does the right
 * things after the output video format has been changed.  (by UI interaction
 * or through an NV-CONTROL event.
 *
 */

static void post_output_video_format_changed(CtkGvo *ctk_gvo)
{
    /* Update UI state */

    update_gvo_current_info(ctk_gvo);

    update_offset_spin_button_ranges(ctk_gvo);
    
    ctk_config_statusbar_message(ctk_gvo->ctk_config,
                                 "Output Video Format set to: %s.",
                                 ctk_gvo_get_video_format_name
                                 (ctk_gvo->output_video_format));

} /* post_output_video_format_changed() */



/*
 * post_output_data_format_changed() - Makes sure the CPL does the right
 * things after the output data format has been changed (by UI interaction
 * or through an NV-CONTROL event.
 */

static void post_output_data_format_changed(CtkGvo *ctk_gvo)
{
    ctk_config_statusbar_message
        (ctk_gvo->ctk_config,
         "Output Data Format set to: %s.",
         ctk_gvo_get_data_format_name(ctk_gvo->output_data_format));

} /* post_output_data_format_changed() */



/*
 * post_clone_mode_button_toggled() - Makes sure the CPL does the right
 * things after clone mode is enabled/disabled (by UI interaction or
 * through an NV-CONTROL event.
 */

static void post_clone_mode_button_toggled(CtkGvo *ctk_gvo)
{
    GList *children;
    GList *child;
    gboolean enabled;

    children = gtk_container_get_children
        (GTK_CONTAINER(ctk_gvo->toggle_clone_mode_button));

    for (child = children; child; child = child->next) {
        gtk_container_remove
            (GTK_CONTAINER(ctk_gvo->toggle_clone_mode_button),
             (GtkWidget *) child->data);
    }

    g_list_free(children);

    enabled =
        (ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_CLONE) ? TRUE : FALSE;

    if (enabled) {
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_clone_mode_button),
                          ctk_gvo->disable_clone_mode_label);
    } else {
        gtk_container_add(GTK_CONTAINER(ctk_gvo->toggle_clone_mode_button),
                          ctk_gvo->enable_clone_mode_label);
    }
    
    /* Update UI state */

    update_gvo_current_info(ctk_gvo);

    update_gvo_sensitivity(ctk_gvo);

    ctk_config_statusbar_message(ctk_gvo->ctk_config, "Clone Mode %s.",
                                 enabled ? "enabled" : "disabled");

} /* post_clone_mode_button_toggled() */



/*
 * update_gvo_current_info() - Updates the page's information to reflect
 * the GVO device's current state.
 *
 * This function must be called when the following have changed:
 *
 * ctk_gvo->lock_owner
 * ctk_gvo->output_video_format

 */

static void update_gvo_current_info(CtkGvo *ctk_gvo)
{
    int width;
    int height;
    gchar res_string[64], state_string[64];

    /* Get the current video format sizes */
    ctk_gvo_get_video_format_resolution(ctk_gvo->output_video_format,
                                        &width, &height);
    
    switch (ctk_gvo->lock_owner) {
        
    case NV_CTRL_GVO_LOCK_OWNER_NONE:
        snprintf(res_string, 64, "Inactive");
        snprintf(state_string, 64, "Inactive");
        break;

    case NV_CTRL_GVO_LOCK_OWNER_CLONE:
        snprintf(res_string, 64, "%d x %d", width, height);
        snprintf(state_string, 64, "In use by X (Clone mode)");
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

    if (ctk_gvo->current_resolution_label) {
        gtk_label_set_text(GTK_LABEL(ctk_gvo->current_resolution_label),
                           res_string);
    }

    if (ctk_gvo->current_state_label) {
        gtk_label_set_text(GTK_LABEL(ctk_gvo->current_state_label),
                           state_string);
    }

} /* update_gvo_current_info() */



/*
 * update_offset_spin_button_ranges() - Updates the range of the
 * offset spin buttons based on the current screen's width/height.
 */

static void update_offset_spin_button_ranges(CtkGvo *ctk_gvo)
{
    gint w, h, x, y;
    
    ctk_gvo_get_video_format_resolution(ctk_gvo->output_video_format,
                                        &w, &h);

    x = ctk_gvo->screen_width - w;
    y = ctk_gvo->screen_height - h;

    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->x_offset_spin_button), 0, x);
    gtk_spin_button_set_range
        (GTK_SPIN_BUTTON(ctk_gvo->y_offset_spin_button), 0, y);
    
} /* update_offset_spin_button_ranges() */



/*
 * update_gvo_sensitivity() - Set the sensitivity of the GVO panel's widgets
 */

static void update_gvo_sensitivity(CtkGvo *ctk_gvo)
{
    gboolean sensitive;

    sensitive = ((ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_NONE) ||
                 (ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_CLONE));

    gtk_widget_set_sensitive(ctk_gvo->clone_mode_frame, sensitive);

    if (sensitive) {

        /* Video & data formats */
        
        sensitive = (ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_NONE);
        gtk_widget_set_sensitive(ctk_gvo->output_data_format_menu, sensitive);
        
        sensitive = (sensitive && ctk_gvo->has_output_video_formats);
        gtk_widget_set_sensitive(ctk_gvo->output_video_format_menu, sensitive);

        /* Enable/Disable clone mode button */

        sensitive = 
            ((ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_NONE) &&
             ctk_gvo->has_output_video_formats) ||
            (ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_CLONE);
        gtk_widget_set_sensitive(ctk_gvo->toggle_clone_mode_button, sensitive);
    }

} /* update_gvo_sensitivity() */



/**** Event Handlers *********************************************************/

/*
 * gvo_event_received(() - Handles GVO NV-CONTROL events.
 */

static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data)
{
    CtkEventStruct *event_struct = (CtkEventStruct *) arg1;
    CtkGvo *ctk_gvo = CTK_GVO(user_data);
    GtkWidget *widget;
    gint attribute = event_struct->attribute;
    gint value = event_struct->value;
    gboolean active;


    switch (attribute) {

    case NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT:
        widget = ctk_gvo->output_video_format_menu;
        
        g_signal_handlers_block_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_video_format_ui_changed),
             (gpointer) ctk_gvo);
        
        ctk_gvo->output_video_format = value;
        validate_output_video_format(ctk_gvo);
       
        /* Update the dropdown with a reasonable value */
        if (!ctk_gvo->output_video_format_valid) {
            value = get_first_available_output_video_format(ctk_gvo);
        }
        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(widget), value);

        post_output_video_format_changed(ctk_gvo);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_video_format_ui_changed),
             (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_DATA_FORMAT:
        widget = ctk_gvo->output_data_format_menu;
        
        g_signal_handlers_block_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_data_format_ui_changed),
             (gpointer) ctk_gvo);

        ctk_gvo->output_data_format = value;
        validate_output_data_format(ctk_gvo);
       
        /* Update the dropdown with a reasonable value */
        if (!ctk_gvo->output_data_format_valid) {
            value = DEFAULT_OUTPUT_DATA_FORMAT;
        }
        ctk_drop_down_menu_set_current_value
            (CTK_DROP_DOWN_MENU(widget), value);

        post_output_data_format_changed(ctk_gvo);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(widget),
             G_CALLBACK(output_data_format_ui_changed),
             (gpointer) ctk_gvo);
        break;
 
    case NV_CTRL_GVO_X_SCREEN_PAN_X:
        widget = ctk_gvo->x_offset_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(x_offset_ui_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(x_offset_ui_changed),
                                          (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_X_SCREEN_PAN_Y:
        widget = ctk_gvo->y_offset_spin_button;
        
        g_signal_handlers_block_by_func(G_OBJECT(widget),
                                        G_CALLBACK(y_offset_ui_changed),
                                        (gpointer) ctk_gvo);
        
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), value);
        
        g_signal_handlers_unblock_by_func(G_OBJECT(widget),
                                          G_CALLBACK(y_offset_ui_changed),
                                          (gpointer) ctk_gvo);
        break;
        
    case NV_CTRL_GVO_LOCK_OWNER:
        widget = ctk_gvo->toggle_clone_mode_button;

        g_signal_handlers_block_by_func
            (G_OBJECT(widget),
             G_CALLBACK(clone_mode_button_ui_toggled),
             ctk_gvo);

        ctk_gvo->lock_owner = value;
        active = (value != NV_CTRL_GVO_LOCK_OWNER_NONE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), active);
        
        post_clone_mode_button_toggled(ctk_gvo);

        g_signal_handlers_unblock_by_func
            (G_OBJECT(widget),
             G_CALLBACK(clone_mode_button_ui_toggled),
             ctk_gvo);
        break;
    }

} /* gvo_event_received(() */



/*
 * screen_changed_handler() - Handle XRandR screen size update events.
 */

static void screen_changed_handler(GtkWidget *widget,
                                   XRRScreenChangeNotifyEvent *ev,
                                   gpointer data)
{
    CtkGvo *ctk_gvo = CTK_GVO(data);
    CtkDropDownMenu *dmenu;
    gint val;

    /* Cache the new screen dimensions */

    ctk_gvo->screen_width = ev->width;
    ctk_gvo->screen_height = ev->height;

    /* Update the output video format drop down menu and reset the list */

    dmenu = CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu);

    /* Get the currently selected value */

    val = ctk_drop_down_menu_get_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu));

    /* Reset the drop down menu */

    ctk_drop_down_menu_reset(dmenu);
    
    /* Fill the menu with the new valid video modes */

    fill_output_video_format_menu(ctk_gvo);

    /* Finalize and load the menu */

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_gvo->output_video_format_menu),
         G_CALLBACK(output_video_format_ui_changed),
         (gpointer) ctk_gvo);

    ctk_drop_down_menu_finalize(dmenu);
    
    /* Set best valid output video format value possible.
     * Revalidate here since the screen change could cause
     * the current output video mode to now be valid.
     */
    
    validate_output_video_format(ctk_gvo);
    if (ctk_gvo->output_video_format_valid) {
        val = ctk_gvo->output_video_format;
    }

    ctk_drop_down_menu_set_current_value
        (CTK_DROP_DOWN_MENU(ctk_gvo->output_video_format_menu), val);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_gvo->output_video_format_menu),
         G_CALLBACK(output_video_format_ui_changed),
         (gpointer) ctk_gvo);    

    /* Update UI */

    update_gvo_current_info(ctk_gvo);

    update_offset_spin_button_ranges(ctk_gvo);

    update_gvo_sensitivity(ctk_gvo);

} /* screen_changed_handler() */



/**** Callback Handlers ******************************************************/


/*
 * ctk_gvo_probe_callback() - This function gets called when the
 * GVO probe occurs so that we can update the state of attributes that
 * do not emit events.
 *
 * These attributes are:
 *
 * - NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED
 * - NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED
 * - NV_CTRL_GVO_INPUT_VIDEO_FORMAT
 *
 */

gint ctk_gvo_probe_callback(gpointer data)
{
    CtkGvo *ctk_gvo = CTK_GVO(data);
    gint old_input_format = ctk_gvo->input_video_format;
    gint old_sync_mode = ctk_gvo->sync_mode;

    /* Update our copies of some SDI state variables */

    ctk_gvo->input_video_format =
        CTK_GVO_BANNER(ctk_gvo->banner)->input_video_format;

    ctk_gvo->sync_mode =
        CTK_GVO_BANNER(ctk_gvo->banner)->sync_mode;

    if ((ctk_gvo->lock_owner == NV_CTRL_GVO_LOCK_OWNER_NONE) &&
        ((old_input_format != ctk_gvo->input_video_format) ||
         (old_sync_mode != ctk_gvo->sync_mode))) {
        
        /* update the available output video formats */
        
        trim_output_video_format_menu(ctk_gvo);    
    }

    return TRUE;
    
} /* ctk_gvo_probe_callback() */


/*
 * ctk_gvo_select() - Called when the ctk_gvo page is selected
 */

void ctk_gvo_select(GtkWidget *widget)
{
    CtkGvo *ctk_gvo = CTK_GVO(widget);

    /* Grab the GVO banner */

    ctk_gvo_banner_set_parent(CTK_GVO_BANNER(ctk_gvo->banner),
                              ctk_gvo->banner_box,
                              ctk_gvo_probe_callback, ctk_gvo);

} /* ctk_gvo_select() */



/*
 * ctk_gvo_unselect() - Called when a page other than the ctk_gvo
 * page is selected and the ctk_gvo page was the last page to be
 * selected.
 */

void ctk_gvo_unselect(GtkWidget *widget)
{
    CtkGvo *ctk_gvo = CTK_GVO(widget);

    /* Release the GVO banner */

    ctk_gvo_banner_set_parent(CTK_GVO_BANNER(ctk_gvo->banner),
                              NULL, NULL, NULL);

} /* ctk_gvo_unselect() */



/*
 * ctk_gvo_create_help() - Creates the GVO help page.
 */

GtkTextBuffer* ctk_gvo_create_help(GtkTextTagTable *table)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "GVO (Graphics to Video Out) Help");
    ctk_help_para(b, &i, "This page gives access to general information about "
                  "the SDI device as well as configuration of Clone Mode.");

    ctk_help_heading(b, &i, "General Information");
    ctk_help_para(b, &i, "This section shows information about the SDI device "
                  "that is associated with the X screen.");
    ctk_help_heading(b, &i, "Firmware Version");
    ctk_help_para(b, &i, __general_firmware_version_help);
    ctk_help_heading(b, &i, "Current SDI Resolution");
    ctk_help_para(b, &i, __general_current_sdi_resolution_help);
    ctk_help_heading(b, &i, "Current SDI State");
    ctk_help_para(b, &i, __general_current_sdi_state_help);

    ctk_help_heading(b, &i, "Clone Mode");
    ctk_help_para(b, &i, "This section allows configuration and operation "
                  "of the SDI device in Clone Mode.");
    ctk_help_heading(b, &i, "Video Format");
    ctk_help_para(b, &i, "%s  The current size of the associated X screen "
                  "will limit the available clone mode video formats such "
                  "that only video modes that are smaller than or equal to "
                  "the current X screen size will be available.  Also, the "
                  "current Sync Mode may limit available modes when not in "
                  "Free-Running (see Synchronization Options page for more "
                  "information).", __clone_mode_video_format_help);
    ctk_help_heading(b, &i, "Data Format");
    ctk_help_para(b, &i, __clone_mode_data_format_help);
    ctk_help_heading(b, &i, "X Offset");
    ctk_help_para(b, &i, __clone_mode_x_offset_help);
    ctk_help_heading(b, &i, "Y Offset");
    ctk_help_para(b, &i, __clone_mode_y_offset_help);
    ctk_help_heading(b, &i, "Enable Clone Mode");
    ctk_help_para(b, &i, "%s Clone mode may only be enabled when the SDI "
                  "device is currently free (It is not being used by other "
                  "modes such as OpenGL, TwinView, or Separate X Screens).",
                  __clone_mode_enable_clone_mode_help);

    ctk_help_finish(b);

    return b;

} /* ctk_gvo_create_help() */
