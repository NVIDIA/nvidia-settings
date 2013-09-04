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
#include <string.h>

#include <X11/Xlib.h>

#include "NvCtrlAttributes.h"

#include "ctkhelp.h"
#include "ctkgvo.h"
#include "ctkdropdownmenu.h"
#include "ctkutils.h"
#include "ctkbanner.h"

#include "ctkgvo-banner.h"

#include "msg.h"



#define TABLE_PADDING 5


/* General information help */

static const char *__general_firmware_version_help =
"The Firmware Version reports the version of the firmware running on the "
"SDI device.";

static const char *__general_current_sdi_resolution_help =
"The Current SDI Resolution reports the current active resolution that the "
"SDI device is driving or 'Inactive' if SDI is currently disabled.";

static const char *__general_current_sdi_state_help =
"The Current SDI state reports the current usage of the SDI device.";

static const char *__requested_sdi_video_format_help =
"The Requested SDI Video Format indicates what video format is currently "
"requested through NV-CONTROL.";

static const char *__requested_sdi_data_format_help =
"The Requested SDI Data Format indicates what data format is currently "
"requested through NV-CONTROL.";


/* local prototypes */

static void query_video_format_details(CtkGvo *ctk_gvo);

static void register_for_gvo_events(CtkGvo *ctk_gvo, CtkEvent *ctk_event);

static void update_gvo_current_info(CtkGvo *ctk_gvo);

static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data);

/*
 * video format table -- should this be moved into NV-CONTROL?
 */

const GvioFormatName videoFormatNames[] = {
    { NV_CTRL_GVIO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC, "720  x 487i    59.94  Hz  (SMPTE259) NTSC"},
    { NV_CTRL_GVIO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL,  "720  x 576i    50.00  Hz  (SMPTE259) PAL"},
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_23_98_SMPTE296,      "1280 x 720p    23.98  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_24_00_SMPTE296,      "1280 x 720p    24.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_25_00_SMPTE296,      "1280 x 720p    25.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_29_97_SMPTE296,      "1280 x 720p    29.97  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_30_00_SMPTE296,      "1280 x 720p    30.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_50_00_SMPTE296,      "1280 x 720p    50.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_59_94_SMPTE296,      "1280 x 720p    59.94  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_60_00_SMPTE296,      "1280 x 720p    60.00  Hz  (SMPTE296)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1035I_59_94_SMPTE260,     "1920 x 1035i   59.94  Hz  (SMPTE260)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1035I_60_00_SMPTE260,     "1920 x 1035i   60.00  Hz  (SMPTE260)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_47_96_SMPTE274,     "1920 x 1080i   47.96  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_48_00_SMPTE274,     "1920 x 1080i   48.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_SMPTE295,     "1920 x 1080i   50.00  Hz  (SMPTE295)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_SMPTE274,     "1920 x 1080i   50.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_59_94_SMPTE274,     "1920 x 1080i   59.94  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_60_00_SMPTE274,     "1920 x 1080i   60.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_23_976_SMPTE274,    "1920 x 1080p   23.976 Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_24_00_SMPTE274,     "1920 x 1080p   24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_25_00_SMPTE274,     "1920 x 1080p   25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_29_97_SMPTE274,     "1920 x 1080p   29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_30_00_SMPTE274,     "1920 x 1080p   30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274,   "1920 x 1080PsF 23.98  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274,   "1920 x 1080PsF 24.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274,   "1920 x 1080PsF 25.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274,   "1920 x 1080PsF 29.97  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274,   "1920 x 1080PsF 30.00  Hz  (SMPTE274)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_47_96_SMPTE372,     "2048 x 1080i   47.96  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_48_00_SMPTE372,     "2048 x 1080i   48.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_50_00_SMPTE372,     "2048 x 1080i   50.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_59_94_SMPTE372,     "2048 x 1080i   59.94  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_60_00_SMPTE372,     "2048 x 1080i   60.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_23_98_SMPTE372,     "2048 x 1080p   23.98  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_24_00_SMPTE372,     "2048 x 1080p   24.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_25_00_SMPTE372,     "2048 x 1080p   25.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_29_97_SMPTE372,     "2048 x 1080p   29.97  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_30_00_SMPTE372,     "2048 x 1080p   30.00  Hz  (SMPTE372)"    },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_50_00_3G_LEVEL_A_SMPTE274, "1920 x 1080p   50.00  Hz  (SMPTE274) 3G Level A" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_59_94_3G_LEVEL_A_SMPTE274, "1920 x 1080p   59.94  Hz  (SMPTE274) 3G Level A" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_60_00_3G_LEVEL_A_SMPTE274, "1920 x 1080p   60.00  Hz  (SMPTE274) 3G Level A" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_60_00_3G_LEVEL_B_SMPTE274, "1920 x 1080p   60.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_60_00_3G_LEVEL_B_SMPTE274, "1920 x 1080i   60.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_60_00_3G_LEVEL_B_SMPTE372, "2048 x 1080i   60.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_50_00_3G_LEVEL_B_SMPTE274, "1920 x 1080p   50.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_3G_LEVEL_B_SMPTE274, "1920 x 1080i   50.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_50_00_3G_LEVEL_B_SMPTE372, "2048 x 1080i   50.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_30_00_3G_LEVEL_B_SMPTE274, "1920 x 1080p   30.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_30_00_3G_LEVEL_B_SMPTE372, "2048 x 1080p   30.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_25_00_3G_LEVEL_B_SMPTE274, "1920 x 1080p   25.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_25_00_3G_LEVEL_B_SMPTE372, "2048 x 1080p   25.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_24_00_3G_LEVEL_B_SMPTE274, "1920 x 1080p   24.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_24_00_3G_LEVEL_B_SMPTE372, "2048 x 1080p   24.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_48_00_3G_LEVEL_B_SMPTE274, "1920 x 1080i   48.00  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_48_00_3G_LEVEL_B_SMPTE372, "2048 x 1080i   48.00  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_59_94_3G_LEVEL_B_SMPTE274, "1920 x 1080p   59.94  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_59_94_3G_LEVEL_B_SMPTE274, "1920 x 1080i   59.94  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_59_94_3G_LEVEL_B_SMPTE372, "2048 x 1080i   59.94  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_29_97_3G_LEVEL_B_SMPTE274, "1920 x 1080p   29.97  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_29_97_3G_LEVEL_B_SMPTE372, "2048 x 1080p   29.97  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_23_98_3G_LEVEL_B_SMPTE274, "1920 x 1080p   23.98  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_23_98_3G_LEVEL_B_SMPTE372, "2048 x 1080p   23.98  Hz  (SMPTE372) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_47_96_3G_LEVEL_B_SMPTE274, "1920 x 1080i   47.96  Hz  (SMPTE274) 3G Level B" },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_47_96_3G_LEVEL_B_SMPTE372, "2048 x 1080i   47.96  Hz  (SMPTE372) 3G Level B" },
    { -1, NULL },
};


static GvioFormatDetails videoFormatDetails[] = {
    { NV_CTRL_GVIO_VIDEO_FORMAT_487I_59_94_SMPTE259_NTSC, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_576I_50_00_SMPTE259_PAL,  0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_23_98_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_24_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_25_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_29_97_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_30_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_50_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_59_94_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_720P_60_00_SMPTE296,      0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1035I_59_94_SMPTE260,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1035I_60_00_SMPTE260,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_47_96_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_48_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_SMPTE295,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_59_94_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_60_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_23_976_SMPTE274,    0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_24_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_25_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_29_97_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_30_00_SMPTE274,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_23_98_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_24_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_25_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_29_97_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080PSF_30_00_SMPTE274,   0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_47_96_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_48_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_50_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_59_94_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_60_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_23_98_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_24_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_25_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_29_97_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_30_00_SMPTE372,     0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_50_00_3G_LEVEL_A_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_59_94_3G_LEVEL_A_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_60_00_3G_LEVEL_A_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_60_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_60_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_60_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_50_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_50_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_50_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_30_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_30_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_25_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_25_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_24_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_24_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_48_00_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_48_00_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_59_94_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_59_94_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_59_94_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_29_97_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_29_97_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080P_23_98_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048P_23_98_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_1080I_47_96_3G_LEVEL_B_SMPTE274, 0, 0, 0 },
    { NV_CTRL_GVIO_VIDEO_FORMAT_2048I_47_96_3G_LEVEL_B_SMPTE372, 0, 0, 0 },
    { -1, -1, -1, -1 },
};


static const GvioFormatName dataFormatNames[] = {

    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB444,               "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8_TO_YCRCB422,               "RGB -> YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8_444_PASSTHRU,              "RGB (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8A8_TO_YCRCBA4444,           "RGBA -> YCrCbA (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8Z10_TO_YCRCBZ4444,          "RGBZ -> YCrCbZ (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8A8_TO_YCRCBA4224,           "RGBA -> YCrCbA (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R8G8B8Z10_TO_YCRCBZ4224,          "RGBZ -> YCrCbZ (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8A8_4444_PASSTHRU,           "RGBA (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8Z8_4444_PASSTHRU,           "RGBZ (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X10X10_444_PASSTHRU,           "RGBA (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8_444_PASSTHRU,             "RGB (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8A10_4444_PASSTHRU,         "RGBA (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8Z10_4444_PASSTHRU,         "RGBZ (4:4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_DUAL_R8G8B8_TO_DUAL_YCRCB422,     "Dual RGB -> Dual YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_DUAL_X8X8X8_TO_DUAL_422_PASSTHRU, "Dual RGB (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_R10G10B10_TO_YCRCB422,            "RGB -> YCrCb (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_R10G10B10_TO_YCRCB444,            "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X12X12X12_444_PASSTHRU,           "RGB (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_R12G12B12_TO_YCRCB444,            "RGB -> YCrCb (4:4:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8_422_PASSTHRU,              "RGB (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8A8_4224_PASSTHRU,           "RGB (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X8X8X8Z8_4224_PASSTHRU,           "RGB (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X10X10_422_PASSTHRU,           "RGB (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8_422_PASSTHRU,             "RGB (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8A10_4224_PASSTHRU,         "RGBA (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X10X8X8Z10_4224_PASSTHRU,         "RGBZ (4:2:2:4)" },
    { NV_CTRL_GVO_DATA_FORMAT_X12X12X12_422_PASSTHRU,           "RGB (4:2:2)" },
    { NV_CTRL_GVO_DATA_FORMAT_R12G12B12_TO_YCRCB422,            "RGB -> YCrCb (4:2:2)" },

    { -1, NULL },
};



/**** Utility Functions ******************************************************/


/*
 * ctk_gvio_get_video_format_name() - return the name of the given video format
 */

const char *ctk_gvio_get_video_format_name(const gint format)
{
    gint i;
    
    for (i = 0; videoFormatNames[i].name; i++) {
        if (videoFormatNames[i].format == format) {
            return videoFormatNames[i].name;
        }
    }

    return "Unknown";
    
} /* ctk_gvio_get_video_format_name() */



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
            NULL  /* value_table */
        };

        ctk_gvo_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkGvo",
                                   &ctk_gvo_info, 0);
    }
    
    return ctk_gvo_type;
    
} /* ctk_gvo_get_type() */



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
    GtkWidget *hbox;
    ReturnStatus ret;
    gchar scratch[64], *firmware, *string;
    gint val;
    
    GtkWidget *frame, *table;
    
    /* make sure we have a handle */
    
    g_return_val_if_fail(handle != NULL, NULL);

    /* Check if this screen supports GVO */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_GVO_SUPPORTED, &val);
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_GVO_SUPPORTED_TRUE)) {
        /* GVO not available */
        return NULL;
    }

    /* create and initialize the object */
    
    object = g_object_new(CTK_TYPE_GVO, NULL);
    
    ctk_gvo = CTK_GVO(object);
    ctk_gvo->handle = handle;

    /*
     * Query the validness, width, height and refresh rate for each
     * video format
     */

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
                                   NV_CTRL_STRING_GVIO_FIRMWARE_VERSION, 
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
     * Requested SDI Configuration
     */

    frame = gtk_frame_new("Requested SDI Configuration");

    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

    table = gtk_table_new(2, 2, FALSE);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    gtk_container_add(GTK_CONTAINER(frame), table);

    ctk_gvo->current_output_video_format_label =
        add_table_row(table, 3,
                      0, 0.5, "Requested SDI Video Format:",
                      0, 0.5, "Inactive");
    ctk_gvo->current_output_data_format_label =
        add_table_row(table, 4,
                      0, 0.5, "Requested SDI Data Format:",
                      0, 0.5, "Inactive");

    /*
     * Set the initial UI state
     */

    update_gvo_current_info(ctk_gvo);

    /*
     * Start listening for events
     */

    register_for_gvo_events(ctk_gvo, ctk_event);

    /* show the GVO widget */
    
    gtk_widget_show_all(GTK_WIDGET(ctk_gvo));

    return GTK_WIDGET(ctk_gvo);

} /* ctk_gvo_new() */



/**** Initialization Functions ***********************************************/

/*
 * query_video_format_details() - initialize the videoFormatDetails[]
 * table by querying each of refresh rate, width, and height from
 * NV-CONTROL.
 */

static void query_video_format_details(CtkGvo *ctk_gvo)
{
    ReturnStatus ret;
    NVCTRLAttributeValidValuesRec valid;
    gint i, val;

    /* Valid output video formats */

    ret = NvCtrlGetValidAttributeValues(ctk_gvo->handle,
                                        NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,
                                        &valid);
    if ((ret != NvCtrlSuccess) || (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        ctk_gvo->valid_output_video_format_mask[0] = 0;
    } else {
        ctk_gvo->valid_output_video_format_mask[0] = valid.u.bits.ints;
    }

    ret = NvCtrlGetValidAttributeValues(ctk_gvo->handle,
                                        NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT2,
                                        &valid);

    if ((ret != NvCtrlSuccess) || (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        ctk_gvo->valid_output_video_format_mask[1] = 0;
    } else {
        ctk_gvo->valid_output_video_format_mask[1] = valid.u.bits.ints;
    }

    ret = NvCtrlGetValidAttributeValues(ctk_gvo->handle,
                                        NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT3,
                                        &valid);

    if ((ret != NvCtrlSuccess) || (valid.type != ATTRIBUTE_TYPE_INT_BITS)) {
        ctk_gvo->valid_output_video_format_mask[2] = 0;
    } else {
        ctk_gvo->valid_output_video_format_mask[2] = valid.u.bits.ints;
    }

    for (i = 0; videoFormatDetails[i].format != -1; i++) {
        
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVIO_VIDEO_FORMAT_REFRESH_RATE,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].rate = val;
        
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVIO_VIDEO_FORMAT_WIDTH,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].width = val;
                                       
        ret = NvCtrlGetDisplayAttribute(ctk_gvo->handle,
                                        videoFormatDetails[i].format,
                                        NV_CTRL_GVIO_VIDEO_FORMAT_HEIGHT,
                                        &val);
        
        if (ret != NvCtrlSuccess) val = 0;
        
        videoFormatDetails[i].height = val; 
    }

} /* query_video_format_details() */



/*
 * register_for_gvo_events() - Configure ctk_gvo object to listen for
 * GVO related evens.
 */

static void register_for_gvo_events(CtkGvo *ctk_gvo, CtkEvent *ctk_event)
{
    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_DATA_FORMAT),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_GVO_LOCK_OWNER),
                     G_CALLBACK(gvo_event_received),
                     (gpointer) ctk_gvo);

} /* register_for_gvo_events() */



/**** Common Update Functions ************************************************/

/*
 * update_gvo_current_info() - Updates the page's information to reflect
 * the GVO device's current state.
 */

static void update_gvo_current_info(CtkGvo *ctk_gvo)
{
    int width;
    int height;
    ReturnStatus ret;
    gchar res_string[64], state_string[64];
    int output_video_format;
    int output_data_format;
    int lock_owner;

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT,
                             &output_video_format);
    if (ret != NvCtrlSuccess) {
        output_video_format = NV_CTRL_GVIO_VIDEO_FORMAT_NONE;
    }

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_DATA_FORMAT,
                             &output_data_format);
    if (ret != NvCtrlSuccess) {
        output_data_format = -1;
    }

    ret = NvCtrlGetAttribute(ctk_gvo->handle,
                             NV_CTRL_GVO_LOCK_OWNER,
                             &lock_owner);
    if (ret != NvCtrlSuccess) {
        lock_owner = NV_CTRL_GVO_LOCK_OWNER_NONE;
    }


    /* Get the current video format sizes */
    ctk_gvo_get_video_format_resolution(output_video_format, &width, &height);
    
    switch (lock_owner) {
        
    case NV_CTRL_GVO_LOCK_OWNER_NONE:
        snprintf(res_string, 64, "Inactive");
        snprintf(state_string, 64, "Inactive");
        break;

    case NV_CTRL_GVO_LOCK_OWNER_CLONE:
        /* fall through for compatibility */
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

    if (ctk_gvo->current_output_video_format_label) {
        gtk_label_set_text
            (GTK_LABEL(ctk_gvo->current_output_video_format_label),
             ctk_gvio_get_video_format_name(output_video_format));
    }

    if (ctk_gvo->current_output_data_format_label) {
        gtk_label_set_text
            (GTK_LABEL(ctk_gvo->current_output_data_format_label),
             ctk_gvo_get_data_format_name(output_data_format));
    }

} /* update_gvo_current_info() */



/**** Event Handlers *********************************************************/

/*
 * gvo_event_received() - Handles GVO NV-CONTROL events.
 */

static void gvo_event_received(GtkObject *object,
                               gpointer arg1,
                               gpointer user_data)
{
    update_gvo_current_info(CTK_GVO(user_data));

} /* gvo_event_received(() */



/**** Callback Handlers ******************************************************/


/*
 * ctk_gvo_select() - Called when the ctk_gvo page is selected
 */

void ctk_gvo_select(GtkWidget *widget)
{
    CtkGvo *ctk_gvo = CTK_GVO(widget);

    /* Grab the GVO banner */

    ctk_gvo_banner_set_parent(CTK_GVO_BANNER(ctk_gvo->banner),
                              ctk_gvo->banner_box, NULL, NULL);

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
    ctk_help_para(b, &i, "%s", __general_firmware_version_help);
    ctk_help_heading(b, &i, "Current SDI Resolution");
    ctk_help_para(b, &i, "%s", __general_current_sdi_resolution_help);
    ctk_help_heading(b, &i, "Current SDI State");
    ctk_help_para(b, &i, "%s", __general_current_sdi_state_help);

    ctk_help_heading(b, &i, "Requested SDI Video Format");
    ctk_help_para(b, &i, "%s", __requested_sdi_video_format_help);
    ctk_help_heading(b, &i, "Requested SDI Data Format");
    ctk_help_para(b, &i, "%s", __requested_sdi_data_format_help);

    ctk_help_finish(b);

    return b;

} /* ctk_gvo_create_help() */
