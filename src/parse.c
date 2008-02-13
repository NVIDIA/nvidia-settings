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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/utsname.h>

#include "NVCtrl.h"

#include "parse.h"
#include "NvCtrlAttributes.h"

/* local helper functions */

static int nv_parse_display_and_target(char *start, char *end,
                                       ParsedAttribute *a);
static char **nv_strtok(char *s, char c, int *n);
static void nv_free_strtoks(char **s, int n);
static int ctoi(const char c);
static int count_number_of_chars(char *o, char d);
static char *nv_strndup(char *s, int n);

/*
 * Table of all attribute names recognized by the attribute string
 * parser.  Binds attribute names to attribute integers (for use in
 * the NvControl protocol).  The flags describe qualities of each
 * attribute.
 */

#define F NV_PARSER_TYPE_FRAMELOCK
#define C NV_PARSER_TYPE_COLOR_ATTRIBUTE
#define N NV_PARSER_TYPE_NO_CONFIG_WRITE
#define G NV_PARSER_TYPE_GUI_ATTRIUBUTE
#define V NV_PARSER_TYPE_XVIDEO_ATTRIBUTE
#define P NV_PARSER_TYPE_PACKED_ATTRIBUTE
#define D NV_PARSER_TYPE_VALUE_IS_DISPLAY
#define A NV_PARSER_TYPE_NO_QUERY_ALL
#define Z NV_PARSER_TYPE_NO_ZERO_VALUE
#define H NV_PARSER_TYPE_100Hz
#define K NV_PARSER_TYPE_1000Hz
#define S NV_PARSER_TYPE_STRING_ATTRIBUTE

AttributeTableEntry attributeTable[] = {
   
    /* name                    constant                             flags */
 
    { "FlatpanelDithering",    NV_CTRL_FLATPANEL_DITHERING,         0     },
    { "DigitalVibrance",       NV_CTRL_DIGITAL_VIBRANCE,            0     },
    { "ImageSharpening",       NV_CTRL_IMAGE_SHARPENING,            0     },
    { "BusType",               NV_CTRL_BUS_TYPE,                    0     },
    { "BusRate",               NV_CTRL_BUS_RATE,                    0     },
    { "VideoRam",              NV_CTRL_VIDEO_RAM,                   0     },
    { "Irq",                   NV_CTRL_IRQ,                         0     },
    { "OperatingSystem",       NV_CTRL_OPERATING_SYSTEM,            0     },
    { "SyncToVBlank",          NV_CTRL_SYNC_TO_VBLANK,              0     },
    { "AllowFlipping",         NV_CTRL_FLIPPING_ALLOWED,            0     },
    { "ForceStereoFlipping",   NV_CTRL_FORCE_STEREO,                0     },
    { "XineramaStereoFlipping",NV_CTRL_XINERAMA_STEREO,             0     },
    { "LogAniso",              NV_CTRL_LOG_ANISO,                   0     },
    { "FSAA",                  NV_CTRL_FSAA_MODE,                   0     },
    { "TextureSharpen",        NV_CTRL_TEXTURE_SHARPEN,             0     },
    { "Ubb",                   NV_CTRL_UBB,                         0     },
    { "Overlay",               NV_CTRL_OVERLAY,                     0     },
    { "HWOverlay",             NV_CTRL_HWOVERLAY,                   0     },
    { "Stereo",                NV_CTRL_STEREO,                      0     },
    { "TwinView",              NV_CTRL_TWINVIEW,                    0     },
    { "ConnectedDisplays",     NV_CTRL_CONNECTED_DISPLAYS,          0     },
    { "EnabledDisplays",       NV_CTRL_ENABLED_DISPLAYS,            0     },
    { "AssociatedDisplays",    NV_CTRL_ASSOCIATED_DISPLAY_DEVICES,  N|D   },
    { "ProbeDisplays",         NV_CTRL_PROBE_DISPLAYS,              A     },
    { "ForceGenericCpu",       NV_CTRL_FORCE_GENERIC_CPU,           0     },
    { "GammaCorrectedAALines", NV_CTRL_OPENGL_AA_LINE_GAMMA,        0     },
    { "ShowSLIHUD",            NV_CTRL_SHOW_SLI_HUD,                0     },
    { "CursorShadow",          NV_CTRL_CURSOR_SHADOW,               0     },
    { "CursorShadowXOffset",   NV_CTRL_CURSOR_SHADOW_X_OFFSET,      0     },
    { "CursorShadowYOffset",   NV_CTRL_CURSOR_SHADOW_Y_OFFSET,      0     },
    { "CursorShadowAlpha",     NV_CTRL_CURSOR_SHADOW_ALPHA,         0     },
    { "CursorShadowRed",       NV_CTRL_CURSOR_SHADOW_RED,           0     },
    { "CursorShadowGreen",     NV_CTRL_CURSOR_SHADOW_GREEN,         0     },
    { "CursorShadowBlue",      NV_CTRL_CURSOR_SHADOW_BLUE,          0     },
    { "FSAAAppControlled",     NV_CTRL_FSAA_APPLICATION_CONTROLLED, 0     },
    { "LogAnisoAppControlled", NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED,0 },
    { "RefreshRate",           NV_CTRL_REFRESH_RATE,                N|H   },
    { "RefreshRate3",          NV_CTRL_REFRESH_RATE_3,              N|K   },
    { "InitialPixmapPlacement",NV_CTRL_INITIAL_PIXMAP_PLACEMENT,    N     },
    { "PCIBus",                NV_CTRL_PCI_BUS,                     N     },
    { "PCIDevice",             NV_CTRL_PCI_DEVICE,                  N     },
    { "PCIFunc",               NV_CTRL_PCI_FUNCTION,                N     },
    { "DynamicTwinview",       NV_CTRL_DYNAMIC_TWINVIEW,            N     },
    { "MultiGpuDisplayOwner",  NV_CTRL_MULTIGPU_DISPLAY_OWNER,      N     },
    { "GPUScaling",            NV_CTRL_GPU_SCALING,                 P     },
    { "FrontendResolution",    NV_CTRL_FRONTEND_RESOLUTION,         N|P   },
    { "BackendResolution",     NV_CTRL_BACKEND_RESOLUTION,          N|P   },
    { "FlatpanelNativeResolution",  NV_CTRL_FLATPANEL_NATIVE_RESOLUTION,   N|P },
    { "FlatpanelBestFitResolution", NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION, N|P },
    { "GPUScalingActive",      NV_CTRL_GPU_SCALING_ACTIVE,          N     },
    { "DFPScalingActive",      NV_CTRL_DFP_SCALING_ACTIVE,          N     },
    { "FSAAAppEnhanced",       NV_CTRL_FSAA_APPLICATION_ENHANCED,   0     },
    { "OnDemandVBlankInterrupts", NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS, 0   },

    { "FrameLockMaster",       NV_CTRL_FRAMELOCK_MASTER,            N|F|G|D },
    { "FrameLockSlaves",       NV_CTRL_FRAMELOCK_SLAVES,            N|F|G|D },
    { "FramelockUseHouseSync", NV_CTRL_USE_HOUSE_SYNC,              N|F|G   },
    { "FrameLockSyncInterval", NV_CTRL_FRAMELOCK_SYNC_INTERVAL,     N|F|G   },
    { "FrameLockPolarity",     NV_CTRL_FRAMELOCK_POLARITY,          N|F|G   },
    { "FrameLockVideoMode",    NV_CTRL_FRAMELOCK_VIDEO_MODE,        N|F|G   },
    { "FrameLockSyncDelay",    NV_CTRL_FRAMELOCK_SYNC_DELAY,        N|F|G },
    { "FrameLockEnable",       NV_CTRL_FRAMELOCK_SYNC,              N|F|G },
    { "FrameLockAvailable",    NV_CTRL_FRAMELOCK,                   N|F|G },
    { "FrameLockPort0Status",  NV_CTRL_FRAMELOCK_PORT0_STATUS,      N|F|G },
    { "FrameLockPort1Status",  NV_CTRL_FRAMELOCK_PORT1_STATUS,      N|F|G },
    { "FrameLockHouseStatus",  NV_CTRL_FRAMELOCK_HOUSE_STATUS,      N|F|G },
    { "FrameLockSyncReady",    NV_CTRL_FRAMELOCK_SYNC_READY,        N|F|G },
    { "FrameLockStereoSync",   NV_CTRL_FRAMELOCK_STEREO_SYNC,       N|F|G },
    { "FrameLockTestSignal",   NV_CTRL_FRAMELOCK_TEST_SIGNAL,       N|F|G },
    { "FrameLockEthDetected",  NV_CTRL_FRAMELOCK_ETHERNET_DETECTED, N|F|G },
    { "FrameLockSyncRate",     NV_CTRL_FRAMELOCK_SYNC_RATE,         N|F|G },
    { "FrameLockSyncRate4",    NV_CTRL_FRAMELOCK_SYNC_RATE_4,       N|F|G },
    { "FrameLockTiming",       NV_CTRL_FRAMELOCK_TIMING,            N|F|G },
    { "FrameLockMasterable",   NV_CTRL_FRAMELOCK_MASTERABLE,        N|F|G },
    { "FrameLockFPGARevision", NV_CTRL_FRAMELOCK_FPGA_REVISION,     N|F|G },

    { "GvoSupported",                    NV_CTRL_GVO_SUPPORTED,                        N },
    { "GvoSyncMode",                     NV_CTRL_GVO_SYNC_MODE,                        N },
    { "GvoSyncSource",                   NV_CTRL_GVO_SYNC_SOURCE,                      N },
    { "GvoOutputVideoFormat",            NV_CTRL_GVO_OUTPUT_VIDEO_FORMAT,              N },
    { "GvoInputVideoFormat",             NV_CTRL_GVO_INPUT_VIDEO_FORMAT,               N },
    { "GvoDataFormat",                   NV_CTRL_GVO_DATA_FORMAT,                      N },
    { "GvoDisplayXScreen",               NV_CTRL_GVO_DISPLAY_X_SCREEN,                 N },
    { "GvoCompositeSyncInputDetected",   NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED,    N },
    { "GvoCompositeSyncInputDetectMode", NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE, N },
    { "GvoSdiSyncInputDetected",         NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED,          N },
    { "GvoVideoOutputs",                 NV_CTRL_GVO_VIDEO_OUTPUTS,                    N },
    { "GvoSyncDelayPixels",              NV_CTRL_GVO_SYNC_DELAY_PIXELS,                N },
    { "GvoSyncDelayLines",               NV_CTRL_GVO_SYNC_DELAY_LINES,                 N },
    { "GvoGlxLocked",                    NV_CTRL_GVO_GLX_LOCKED,                       N },
    { "GvoXScreenPanX",                  NV_CTRL_GVO_X_SCREEN_PAN_X,                   N },
    { "GvoXScreenPanY",                  NV_CTRL_GVO_X_SCREEN_PAN_Y,                   N },
    { "GvoOverrideHwCsc",                NV_CTRL_GVO_OVERRIDE_HW_CSC,                  N },
    { "GvoCapabilities",                 NV_CTRL_GVO_CAPABILITIES,                     N },
    { "GvoCompositeTermination",         NV_CTRL_GVO_COMPOSITE_TERMINATION,            N },
    { "GvoFlipQueueSize",                NV_CTRL_GVO_FLIP_QUEUE_SIZE,                  N },
    { "GvoFirmwareVersion",              NV_CTRL_STRING_GVO_FIRMWARE_VERSION,          S|N },
    { "GvoLockOwner",                    NV_CTRL_GVO_LOCK_OWNER,                       N },

    { "Brightness",            BRIGHTNESS_VALUE|ALL_CHANNELS,       N|C|G },
    { "RedBrightness",         BRIGHTNESS_VALUE|RED_CHANNEL,        C|G   },
    { "GreenBrightness",       BRIGHTNESS_VALUE|GREEN_CHANNEL,      C|G   },
    { "BlueBrightness",        BRIGHTNESS_VALUE|BLUE_CHANNEL,       C|G   },
    { "Contrast",              CONTRAST_VALUE|ALL_CHANNELS,         N|C|G },
    { "RedContrast",           CONTRAST_VALUE|RED_CHANNEL,          C|G   },
    { "GreenContrast",         CONTRAST_VALUE|GREEN_CHANNEL,        C|G   },
    { "BlueContrast",          CONTRAST_VALUE|BLUE_CHANNEL,         C|G   },
    { "Gamma",                 GAMMA_VALUE|ALL_CHANNELS,            N|C|G },
    { "RedGamma",              GAMMA_VALUE|RED_CHANNEL,             C|G   },
    { "GreenGamma",            GAMMA_VALUE|GREEN_CHANNEL,           C|G   },
    { "BlueGamma",             GAMMA_VALUE|BLUE_CHANNEL,            C|G   },
    { "TVOverScan",            NV_CTRL_TV_OVERSCAN,                 0     },
    { "TVFlickerFilter",       NV_CTRL_TV_FLICKER_FILTER,           0     },
    { "TVBrightness",          NV_CTRL_TV_BRIGHTNESS,               0     },
    { "TVHue",                 NV_CTRL_TV_HUE,                      0     },
    { "TVContrast",            NV_CTRL_TV_CONTRAST,                 0     },
    { "TVSaturation",          NV_CTRL_TV_SATURATION,               0     },
    { "GPUCoreTemp",           NV_CTRL_GPU_CORE_TEMPERATURE,        N     },
    { "GPUAmbientTemp",        NV_CTRL_AMBIENT_TEMPERATURE,         N     },
    { "OpenGLImageSettings",   NV_CTRL_IMAGE_SETTINGS,              0     },

    { "XVideoOverlaySaturation",   NV_CTRL_ATTR_XV_OVERLAY_SATURATION,     V },
    { "XVideoOverlayContrast",     NV_CTRL_ATTR_XV_OVERLAY_CONTRAST,       V },
    { "XVideoOverlayBrightness",   NV_CTRL_ATTR_XV_OVERLAY_BRIGHTNESS,     V },
    { "XVideoOverlayHue",          NV_CTRL_ATTR_XV_OVERLAY_HUE,            V },
    { "XVideoTextureBrightness",   NV_CTRL_ATTR_XV_TEXTURE_BRIGHTNESS,     V },
    { "XVideoTextureContrast",     NV_CTRL_ATTR_XV_TEXTURE_CONTRAST,       V },
    { "XVideoTextureSyncToVBlank", NV_CTRL_ATTR_XV_TEXTURE_SYNC_TO_VBLANK, V },
    { "XVideoBlitterSyncToVBlank", NV_CTRL_ATTR_XV_BLITTER_SYNC_TO_VBLANK, V },
    { "XVideoSyncToDisplay",       NV_CTRL_XV_SYNC_TO_DISPLAY,           D|Z },
    
    { "GPUOverclockingState",   NV_CTRL_GPU_OVERCLOCKING_STATE,     N   },
    { "GPUDefault2DClockFreqs", NV_CTRL_GPU_DEFAULT_2D_CLOCK_FREQS, N|P },
    { "GPUDefault3DClockFreqs", NV_CTRL_GPU_DEFAULT_3D_CLOCK_FREQS, N|P },
    { "GPU2DClockFreqs",        NV_CTRL_GPU_2D_CLOCK_FREQS,         N|P },
    { "GPU3DClockFreqs",        NV_CTRL_GPU_3D_CLOCK_FREQS,         N|P },
    { "GPUCurrentClockFreqs",   NV_CTRL_GPU_CURRENT_CLOCK_FREQS,    N|P },

    { "NvidiaDriverVersion",    NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,    S|N },
    { "NvControlVersion",       NV_CTRL_STRING_NV_CONTROL_VERSION,       S|N },
    { "GLXServerVersion",       NV_CTRL_STRING_GLX_SERVER_VERSION,       S|N },
    { "GLXClientVersion",       NV_CTRL_STRING_GLX_CLIENT_VERSION,       S|N },
    { "OpenGLVersion",          NV_CTRL_STRING_GLX_OPENGL_VERSION,       S|N },
    { "XRandRVersion",          NV_CTRL_STRING_XRANDR_VERSION,           S|N },
    { "XF86VidModeVersion",     NV_CTRL_STRING_XF86VIDMODE_VERSION,      S|N },
    { "XvVersion",              NV_CTRL_STRING_XV_VERSION,               S|N },
    { "SLIMode",                NV_CTRL_STRING_SLI_MODE,                 S|N },
    
    { NULL,                    0,                                   0     }
};

#undef F
#undef C
#undef N
#undef G
#undef V
#undef P
#undef D
#undef A
#undef Z
#undef H
#undef K
#undef S

/*
 * When new integer attributes are added to NVCtrl.h, an entry should
 * be added in the above attributeTable[].  The below #if should also
 * be updated to indicate the last attribute that the table knows
 * about.
 */

#if NV_CTRL_LAST_ATTRIBUTE != NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS
#warning "Have you forgotten to add a new integer attribute to attributeTable?"
#endif



/*
 * targetTypeTable[] - this table stores an association of the values
 * for each attribute target type.
 */

TargetTypeEntry targetTypeTable[] = {

    { "X Screen",                    /* name */
      "screen",                      /* parsed_name */
      X_SCREEN_TARGET,               /* target_index */
      NV_CTRL_TARGET_TYPE_X_SCREEN,  /* nvctrl */
      ATTRIBUTE_TYPE_X_SCREEN,       /* permission_bit */
      NV_TRUE },                     /* uses_display_devices */
    
    { "GPU",                         /* name */
      "gpu",                         /* parsed_name */
      GPU_TARGET,                    /* target_index */
      NV_CTRL_TARGET_TYPE_GPU,       /* nvctrl */
      ATTRIBUTE_TYPE_GPU,            /* permission_bit */
      NV_TRUE },                     /* uses_display_devices */
    
    { "Frame Lock Device",           /* name */
      "framelock",                   /* parsed_name */
      FRAMELOCK_TARGET,              /* target_index */
      NV_CTRL_TARGET_TYPE_FRAMELOCK, /* nvctrl */
      ATTRIBUTE_TYPE_FRAMELOCK,      /* permission_bit */
      NV_FALSE },                    /* uses_display_devices */

    { "VCSC",                        /* name */
      "vcsc",                        /* parsed_name */
      VCSC_TARGET,                   /* target_index */
      NV_CTRL_TARGET_TYPE_VCSC,      /* nvctrl */
      ATTRIBUTE_TYPE_VCSC,           /* permission_bit */
      NV_FALSE },                    /* uses_display_devices */

    { NULL, NULL, 0, 0, 0 },
};



/*
 * nv_parse_attribute_string() - see comments in parse.h
 */

int nv_parse_attribute_string(const char *str, int query, ParsedAttribute *a)
{
    char *s, *tmp, *name, *start, *display_device_name, *no_spaces = NULL;
    char tmpname[NV_PARSER_MAX_NAME_LEN];
    AttributeTableEntry *t;
    int len, ret;

#define stop(x) { if (no_spaces) free(no_spaces); return (x); }
    
    if (!a) stop(NV_PARSER_STATUS_BAD_ARGUMENT);

    /* clear the ParsedAttribute struct */

    memset((void *) a, 0, sizeof(ParsedAttribute));

    /* remove any white space from the string, to simplify parsing */

    no_spaces = remove_spaces(str);
    if (!no_spaces) stop(NV_PARSER_STATUS_EMPTY_STRING);
    
    /*
     * get the display name... ie: everything before the
     * DISPLAY_NAME_SEPARATOR
     */

    s = strchr(no_spaces, DISPLAY_NAME_SEPARATOR);

    /*
     * If we found a DISPLAY_NAME_SEPARATOR, and there is some text
     * before it, parse that text as an X Display name, X screen,
     * and/or a target specification.
     */

    if ((s) && (s != no_spaces)) {
        
        ret = nv_parse_display_and_target(no_spaces, s, a);

        if (ret != NV_PARSER_STATUS_SUCCESS) {
            stop(ret);
        }
    }
    
    /* move past the DISPLAY_NAME_SEPARATOR */
    
    if (s) s++;
    else s = no_spaces;
    
    /* read the attribute name */

    name = s;
    len = 0;
    while (*s && isalnum(*s)) { s++; len++; }
    
    if (len == 0) stop(NV_PARSER_STATUS_ATTR_NAME_MISSING);
    if (len >= NV_PARSER_MAX_NAME_LEN)
        stop(NV_PARSER_STATUS_ATTR_NAME_TOO_LONG);

    strncpy(tmpname, name, len);
    tmpname[len] = '\0';
    
    /* look up the requested name */

    for (t = attributeTable; t->name; t++) {
        if (nv_strcasecmp(tmpname, t->name)) {
            a->name = t->name;
            a->attr = t->attr;
            a->flags |= t->flags;
            break;
        }
    }
    
    if (!a->name) stop(NV_PARSER_STATUS_UNKNOWN_ATTR_NAME);
    
    /* read the display device name, if any */
    
    if (*s == '[') {
        s++;
        start = s;
        while (*s && *s != ']') s++;
        display_device_name = nv_strndup(start, s - start);
        a->display_device_mask =
            display_device_name_to_display_device_mask(display_device_name);
        if (a->display_device_mask == INVALID_DISPLAY_DEVICE_MASK)
            stop(NV_PARSER_STATUS_BAD_DISPLAY_DEVICE);
        a->flags |= NV_PARSER_HAS_DISPLAY_DEVICE;
        if (*s == ']') s++;
    }
    
    if (query == NV_PARSER_ASSIGNMENT) {
        
        /* there should be an equal sign */
    
        if (*s == '=') s++;
        else stop(NV_PARSER_STATUS_MISSING_EQUAL_SIGN);
        
        /* read the value */
    
        tmp = s;
        if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) {
            /* color attributes are floating point */
            a->fval = strtod(s, &tmp);
        } else if (a->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE) {
            /*
             * Either a single 32-bit integer or two 16-bit
             * integers, separated by ','.
             */
            a->val = strtol(s, &tmp, 10);
            if (tmp && *tmp == ',') {
                a->val = (a->val & 0xffff) << 16;
                a->val |= strtol((tmp + 1), &tmp, 10) & 0xffff;
            }
        } else if (a->flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY) {
            if (nv_strcasecmp(s, "alldisplays")) {
                a->flags |= NV_PARSER_TYPE_ASSIGN_ALL_DISPLAYS;
                tmp = s + strlen(s);
            } else {
                uint32 mask = 0;
                mask = display_device_name_to_display_device_mask(s);
                if (mask && (mask != INVALID_DISPLAY_DEVICE_MASK) &&
                    ((mask & (DISPLAY_DEVICES_WILDCARD_CRT |
                              DISPLAY_DEVICES_WILDCARD_TV |
                              DISPLAY_DEVICES_WILDCARD_DFP)) == 0)) {
                    a->val = mask;
                    tmp = s + strlen(s);
                } else {
                   a->val = strtol(s, &tmp, 0);
                }
            }
        } else {
            /* all other attributes are integer */
            a->val = strtol(s, &tmp, 0);
        }
         
        if (tmp && (s != tmp)) a->flags |= NV_PARSER_HAS_VAL;
        s = tmp;
        
        if (!(a->flags & NV_PARSER_HAS_VAL)) stop(NV_PARSER_STATUS_NO_VALUE);
    }
    
    /* this should be the end of the string */

    if (*s != '\0') stop(NV_PARSER_STATUS_TRAILING_GARBAGE);

    stop(NV_PARSER_STATUS_SUCCESS);
    
} /* nv_parse_attribute_string() */



/*
 * nv_parse_display_and_target() - helper function for
 * nv_parse_attribute_string() to parse all the text before the
 * DISPLAY_NAME_SEPARATOR.  This text is expected to be an X Display
 * name, just an X screen, and/or a target specification.
 */

static int nv_parse_display_and_target(char *start,
                                       char *end, /* exclusive */
                                       ParsedAttribute *a)
{
    int digits_only, i, target_type, target_id, len;
    char *tmp, *s, *pOpen, *pClose, *colon;

    /*
     * are all characters numeric? compute the target_id integer as we
     * scan the string to check
     */
    
    digits_only = NV_TRUE;
    target_id = 0;
    
    for (s = start; s < end; s++) {
        if (!isdigit(*s)) {
            digits_only = NV_FALSE;
            break;
        }
        target_id = (target_id * 10) + ctoi(*s);
    }

    /*
     * if all characters are numeric, assume the target type is
     * X_SCREEN, and build the target_id; we have no X Display name in
     * this case.
     */
    
    if (digits_only) {
        a->display = NULL;
        a->flags &= ~NV_PARSER_HAS_X_DISPLAY;
        a->flags |= NV_PARSER_HAS_TARGET;
        a->target_id = target_id;
        a->target_type = NV_CTRL_TARGET_TYPE_X_SCREEN;
        
        /* we are done */
        
        return NV_PARSER_STATUS_SUCCESS;
    }
    
    /*
     * if we get here, then there are non-digit characters; look for a
     * pair of brackets, and treat the contents as a target
     * specification.
     */
    
    pOpen = pClose = NULL;

    for (s = start; s < end; s++) {
        if (*s == '[') pOpen = s;
        if (*s == ']') pClose = s;
    }

    if (pOpen && pClose && (pClose > pOpen) && ((pClose - pOpen) > 1)) {

        /*
         * we have a pair of brackets and something inside the
         * brackets, pull that into a temporary string.
         */

        len = pClose - pOpen - 1;

        tmp = nv_strndup(pOpen + 1, len);

        /* find the colon within the temp string */

        colon = strchr(tmp, ':');
        
        /* no colon? give up */

        if (!colon) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_NO_COLON;
        }
        
        /*
         * check that what is between the opening bracket and the
         * colon is a target type name
         */

        *colon = '\0';
        target_type = -1;

        for (i = 0; targetTypeTable[i].name; i++) {
            if (nv_strcasecmp(tmp, targetTypeTable[i].parsed_name)) {
                target_type = targetTypeTable[i].nvctrl;
                break;
            }
        }
        
        *colon = ':';
        
        /* if we did not find a matching target name, give up */
        
        if (target_type == -1) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET;
        }
        
        /*
         * check that we have something between the colon and the end
         * of the temp string
         */

        if ((colon + 1 - tmp) >= len) {
            free(tmp);
            return NV_PARSER_STATUS_TARGET_SPEC_NO_TARGET_ID;
        }

        /*
         * everything after the colon should be numeric; assign it to
         * the target_id
         */
        
        target_id = 0;

        for (s = colon + 1; *s; s++) {
            if (!isdigit(*s)) {
                free(tmp);
                return NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET_ID;
            }
            target_id = (target_id * 10) + ctoi(*s);
        }
        
        a->target_type = target_type;
        a->target_id = target_id;
        
        a->flags |= NV_PARSER_HAS_TARGET;

        /* we're finally done with the temp string */

        free(tmp);
        
        /*
         * check that there is no stray text between the closing
         * bracket and the end of our parsable string
         */

        if ((end - pClose) > 1) {
            return NV_PARSER_STATUS_TARGET_SPEC_TRAILING_GARBAGE;
        }
        
        /*
         * make end now point at the start of the bracketed target
         * info for the X Display name processing below
         */
        
        end = pOpen;
    }
    

    /* treat everything between start and end as an X Display name */
    
    if (start < end) {

        a->display = nv_strndup(start, end - start);
        a->flags |= NV_PARSER_HAS_X_DISPLAY;
            
        /*
         * this will attempt to parse out any screen number from the
         * display name
         */
    
        nv_assign_default_display(a, NULL);
    }
    
    /* done */

    return NV_PARSER_STATUS_SUCCESS;
    
} /* nv_parse_display_and_target() */



/*
 * nv_parse_strerror() - given the error status returned by
 * nv_parse_attribute_string(), return a string describing the
 * error.
 */

char *nv_parse_strerror(int status)
{
    switch (status) {
    case NV_PARSER_STATUS_SUCCESS :
        return "No error"; break;
    case NV_PARSER_STATUS_BAD_ARGUMENT :
        return "Bad argument"; break;
    case NV_PARSER_STATUS_EMPTY_STRING :
        return "Emtpy string"; break;
    case NV_PARSER_STATUS_ATTR_NAME_TOO_LONG :
        return "The attribute name is too long"; break;
    case NV_PARSER_STATUS_ATTR_NAME_MISSING :
        return "Missing attribute name"; break;
    case NV_PARSER_STATUS_BAD_DISPLAY_DEVICE :
        return "Malformed display device identification"; break;
    case NV_PARSER_STATUS_MISSING_EQUAL_SIGN :
        return "Missing equal sign after attribute name"; break;
    case NV_PARSER_STATUS_NO_VALUE :
        return "No attribute value specified"; break;
    case NV_PARSER_STATUS_TRAILING_GARBAGE :
        return "Trailing garbage"; break;
    case NV_PARSER_STATUS_UNKNOWN_ATTR_NAME :
        return "Unrecognized attribute name"; break;
    case NV_PARSER_STATUS_MISSING_COMMA:
        return "Missing comma in packed integer value"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_NO_COLON:
        return "No colon in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET:
        return "Bad target in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_NO_TARGET_ID:
        return "No target ID in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_BAD_TARGET_ID:
        return "Bad target ID in target specification"; break;
    case NV_PARSER_STATUS_TARGET_SPEC_TRAILING_GARBAGE:
        return "Trailing garbage after target specification"; break;
        
    default:
        return "Unknown error"; break;
    }
} /* nv_parse_strerror() */



/*
 * *sigh* strcasecmp() is a BSDism, and when building with "-ansi" we
 * don't get the prototype, so reimplement it to avoid a compiler
 * warning.  Returns NV_TRUE if a match, returns NV_FALSE if there is
 * no match.
 */

int nv_strcasecmp(const char *a, const char *b)
{
    if (!a && !b) return NV_TRUE;
    if (!a &&  b) return NV_FALSE;
    if ( a && !b) return NV_FALSE;

    while (toupper(*a) == toupper(*b)) {
        a++;
        b++;
        if ((*a == '\0') && (*b == '\0')) return NV_TRUE;
    }

    return NV_FALSE;

} /* nv_strcasecmp() */



/*
 * display_name_to_display_device_mask() - parse the string that describes a
 * display device mask; the string is a comma-separated list of
 * display device names, where valid names are:
 *
 * CRT-[0,7] TV-[0,7] and DFP[0,7]
 *
 * Non-specific names ("CRT", "TV", and "DFP") are also allowed; if
 * these are specified, then the appropriate WILDCARD flag in the
 * upper-most byte of the display device mask is set:
 *
 *    DISPLAY_DEVICES_WILDCARD_CRT
 *    DISPLAY_DEVICES_WILDCARD_TV
 *    DISPLAY_DEVICES_WILDCARD_DFP
 *
 * If a parse error occurs, INVALID_DISPLAY_DEVICE_MASK is returned,
 * otherwise the display mask is returned.
 
 */

uint32 display_device_name_to_display_device_mask(const char *str)
{
    uint32 mask = 0;
    char *s, **toks;
    int i, n;

    /* sanity check */

    if (!str || !*str) return INVALID_DISPLAY_DEVICE_MASK;
    
    /* remove spaces from the string */

    s = remove_spaces(str);
    if (!s || !*s) return INVALID_DISPLAY_DEVICE_MASK;
    
    /* break up the string by commas */

    toks = nv_strtok(s, ',', &n);
    if (!toks) {
        free(s);
        return INVALID_DISPLAY_DEVICE_MASK;
    }

    /* match each token, updating mask as appropriate */

    for (i = 0; i < n; i++) {
        
        if      (nv_strcasecmp(toks[i], "CRT-0")) mask |= ((1 << 0) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-1")) mask |= ((1 << 1) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-2")) mask |= ((1 << 2) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-3")) mask |= ((1 << 3) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-4")) mask |= ((1 << 4) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-5")) mask |= ((1 << 5) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-6")) mask |= ((1 << 6) << 0);
        else if (nv_strcasecmp(toks[i], "CRT-7")) mask |= ((1 << 7) << 0);

        else if (nv_strcasecmp(toks[i], "TV-0" )) mask |= ((1 << 0) << 8);
        else if (nv_strcasecmp(toks[i], "TV-1" )) mask |= ((1 << 1) << 8);
        else if (nv_strcasecmp(toks[i], "TV-2" )) mask |= ((1 << 2) << 8);
        else if (nv_strcasecmp(toks[i], "TV-3" )) mask |= ((1 << 3) << 8);
        else if (nv_strcasecmp(toks[i], "TV-4" )) mask |= ((1 << 4) << 8);
        else if (nv_strcasecmp(toks[i], "TV-5" )) mask |= ((1 << 5) << 8);
        else if (nv_strcasecmp(toks[i], "TV-6" )) mask |= ((1 << 6) << 8);
        else if (nv_strcasecmp(toks[i], "TV-7" )) mask |= ((1 << 7) << 8);

        else if (nv_strcasecmp(toks[i], "DFP-0")) mask |= ((1 << 0) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-1")) mask |= ((1 << 1) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-2")) mask |= ((1 << 2) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-3")) mask |= ((1 << 3) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-4")) mask |= ((1 << 4) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-5")) mask |= ((1 << 5) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-6")) mask |= ((1 << 6) << 16);
        else if (nv_strcasecmp(toks[i], "DFP-7")) mask |= ((1 << 7) << 16);
        
        else if (nv_strcasecmp(toks[i], "CRT"))
            mask |= DISPLAY_DEVICES_WILDCARD_CRT;
        
        else if (nv_strcasecmp(toks[i], "TV"))
            mask |= DISPLAY_DEVICES_WILDCARD_TV;
                
        else if (nv_strcasecmp(toks[i], "DFP"))
            mask |= DISPLAY_DEVICES_WILDCARD_DFP;

        else {
            mask = INVALID_DISPLAY_DEVICE_MASK;
            break;
        }
    }
    
    nv_free_strtoks(toks, n);
    
    free(s);

    return mask;
    
} /* display_name_to_display_device_mask() */



/*
 * display_device_mask_to_display_name() - construct a string
 * describing the given display device mask.  The returned pointer
 * points to a newly allocated string, so callers to this function
 * are responsible for freeing the memory.
 */

#define DISPLAY_DEVICE_STRING_LEN 256

char *display_device_mask_to_display_device_name(const uint32 mask)
{
    char *s;
    int first = NV_TRUE;
    uint32 devcnt, devmask;
    char *display_device_name_string;

    display_device_name_string = malloc(DISPLAY_DEVICE_STRING_LEN);

    s = display_device_name_string;

    devmask = 1 << BITSHIFT_CRT;
    devcnt = 0;
    while (devmask & BITMASK_ALL_CRT) {
        if (devmask & mask) {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "CRT-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }

    devmask = 1 << BITSHIFT_DFP;
    devcnt = 0;
    while (devmask & BITMASK_ALL_DFP) {
        if (devmask & mask)  {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "DFP-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    devmask = 1 << BITSHIFT_TV;
    devcnt = 0;
    while (devmask & BITMASK_ALL_TV) {
        if (devmask & mask)  {
            if (first) first = NV_FALSE;
            else s += sprintf(s, ", ");
            s += sprintf(s, "TV-%X", devcnt);
        }
        devmask <<= 1;
        devcnt++;
    }
    
    if (mask & DISPLAY_DEVICES_WILDCARD_CRT) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "CRT");
    }

    if (mask & DISPLAY_DEVICES_WILDCARD_TV) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "TV");
    }

    if (mask & DISPLAY_DEVICES_WILDCARD_DFP) {
        if (first) first = NV_FALSE;
        else s += sprintf(s, ", ");
        s += sprintf(s, "DFP");
    }
    
    *s = '\0';
    
    return (display_device_name_string);

} /* display_device_mask_to_display_name() */



/*
 * expand_display_device_mask_wildcards() - build a display mask by
 * taking any of the real display mask bits; if there are any wildcard
 * flags set, or in all display devices of that type into the display
 * mask.
 */

uint32 expand_display_device_mask_wildcards(const uint32 d, const uint32 e)
{
    uint32 mask = d & VALID_DISPLAY_DEVICES_MASK;

    if (d & DISPLAY_DEVICES_WILDCARD_CRT) mask |= (e & BITMASK_ALL_CRT);
    if (d & DISPLAY_DEVICES_WILDCARD_TV)  mask |= (e & BITMASK_ALL_TV);
    if (d & DISPLAY_DEVICES_WILDCARD_DFP) mask |= (e & BITMASK_ALL_DFP);
    
    return mask;

} /* expand_display_device_mask_wildcards() */



/*
 * nv_assign_default_display() - assign an X display, if none has been
 * assigned already.  Also, parse the the display name to find any
 * specified X screen.
 */

void nv_assign_default_display(ParsedAttribute *a, const char *display)
{
    char *colon, *dot, *s;
    int digits_only;

    if (!(a->flags & NV_PARSER_HAS_X_DISPLAY)) {
        if (display) a->display = strdup(display);
        else a->display = NULL;
        a->flags |= NV_PARSER_HAS_X_DISPLAY;
    }

    if (!(a->flags & NV_PARSER_HAS_TARGET) && a->display) {
        colon = strchr(a->display, ':');
        if (colon) {
            dot = strchr(colon, '.');
            if (dot) {
                
                /*
                 * if all characters afer the '.' are digits,
                 * interpret it as a screen number.
                 */

                digits_only = NV_FALSE;
                a->target_id = 0;

                for (s = dot + 1; *s; s++) {
                    if (isdigit(*s)) {
                        digits_only = NV_TRUE;
                        a->target_id = (a->target_id * 10) + ctoi(*s);
                    } else {
                        digits_only = NV_FALSE;
                        break;
                    }
                }

                if (digits_only) {
                    a->target_type = NV_CTRL_TARGET_TYPE_X_SCREEN;
                    a->flags |= NV_PARSER_HAS_TARGET;
                }
            }
        }
    }
} /* nv_assign_default_display() */



/* 
 * nv_parsed_attribute_init() - initialize a ParsedAttribute linked
 * list
 */

ParsedAttribute *nv_parsed_attribute_init(void)
{
    ParsedAttribute *p = calloc(1, sizeof(ParsedAttribute));

    p->next = NULL;

    return p;
    
} /* nv_parsed_attribute_init() */



/*
 * nv_parsed_attribute_add() - add a new parsed attribute node to the
 * linked list
 */

void nv_parsed_attribute_add(ParsedAttribute *head, ParsedAttribute *a)
{
    ParsedAttribute *p, *t;

    p = calloc(1, sizeof(ParsedAttribute));

    p->next = NULL;
    
    for (t = head; t->next; t = t->next);
    
    t->next = p;
    
    if (a->display) t->display = strdup(a->display);
    else t->display = NULL;
    
    t->target_type         = a->target_type;
    t->target_id           = a->target_id;
    t->attr                = a->attr;
    t->val                 = a->val;
    t->fval                = a->fval;
    t->display_device_mask = a->display_device_mask;
    t->flags               = a->flags;
    
} /* nv_parsed_attribute_add() */



/*
 * nv_parsed_attribute_free() - free the linked list
 */

void nv_parsed_attribute_free(ParsedAttribute *p)
{
    ParsedAttribute *n;
    
    while(p) {
        n = p->next;
        if (p->display) free(p->display);
        free(p);
        p = n;
    }

} /* nv_parsed_attribute_free() */



/*
 * nv_parsed_attribute_clean() - clean out the ParsedAttribute list,
 * so that only the empty head node remains.
 */

void nv_parsed_attribute_clean(ParsedAttribute *p)
{
    nv_parsed_attribute_free(p->next);

    if (p->display) free(p->display);
    if (p->name) free(p->name);
    
    memset(p, 0, sizeof(ParsedAttribute));

} /* nv_parsed_attribute_clean() */



/*
 * nv_get_attribute_name() - scan the attributeTable for the name that
 * corresponds to the attribute constant.
 */

char *nv_get_attribute_name(const int attr)
{
    int i;

    for (i = 0; attributeTable[i].name; i++) {
        if (attributeTable[i].attr == attr) return attributeTable[i].name;
    }

    return NULL;
    
} /* nv_get_attribute_name() */



/*
 * nv_standardize_screen_name() - standardize the X Display name, by
 * inserting the hostname (if necessary), and using the specified
 * screen number.  If 'screen' is -1, use the screen number already in
 * the string.  If 'screen' is -2, do not put a screen number in the
 * Display name.
 */

char *nv_standardize_screen_name(const char *orig, int screen)
{
    char *display_name, *screen_name, *colon, *dot, *tmp;
    struct utsname uname_buf;
    int len;
    
    /* get the string describing this display connection */
    
    if (!orig) return NULL;
    
    /* create a working copy */
    
    display_name = strdup(orig);
    if (!display_name) return NULL;
    
    /* skip past the host */
    
    colon = strchr(display_name, ':');
    if (!colon) return NULL;
    
    /* if no host is specified, prepend the local hostname */
    
    /* XXX should we try to catch "localhost"? */

    if (display_name == colon) {
        if (uname(&uname_buf) == 0) {
            len = strlen(display_name) + strlen(uname_buf.nodename) + 1;
            tmp = malloc(len);
            snprintf(tmp, len, "%s%s", uname_buf.nodename, display_name);
            free(display_name);
            display_name = tmp;
            colon = strchr(display_name, ':');
            if (!colon) return NULL;
        }
    }
    
    /*
     * if the screen parameter is -1, then extract the screen number,
     * either from the string or default to 0
     */
    
    if (screen == -1) {
        dot = strchr(colon, '.');
        if (dot) {
            screen = atoi(dot + 1);
        } else {
            screen = 0;
        }
    }
    
    /*
     * find the separation between the display and the screen; if we
     * find it, then truncate the string before the screen, so that we
     * can append the correct screen number.
     */
    
    dot = strchr(colon, '.');
    if (dot) *dot = '\0';

    /*
     * if the screen parameter is -2, then do not write out a screen
     * number.
     */
    
    if (screen == -2) {
        screen_name = display_name;
    } else {
        len = strlen(display_name) + 8;
        screen_name = malloc(len);
        snprintf(screen_name, len, "%s.%d", display_name, screen);
        free(display_name);
    }
    
    return (screen_name);

} /* nv_standardize_screen_name() */



/*
 * allocate an output string, and copy the input string to the output
 * string, omitting whitespace
 */

char *remove_spaces(const char *o)
{
    int len;
    char *m, *no_spaces;
   
    if (!o) return (NULL);
    
    len = strlen (o);
    
    no_spaces = (char *) malloc (len+1);

    m = no_spaces;
    while (*o) {
        if (!isspace (*o)) { *m++ = *o; }
        o++;
    }
    *m = '\0';
    
    len = m - no_spaces + 1;
    no_spaces = (char *) (realloc (no_spaces, len));
    
    return (no_spaces);

} /* remove_spaces() */



/**************************************************************************/



/*
 * nv_strtok () - returns a dynamically allocated array of strings,
 * which are the separate segments of the passed in string, divided by
 * the character indicated.  The passed-by-reference argument num will
 * hold the number of segments found.  When you are done with the
 * array of strings, it is best to call nvFreeStrToks () to free the
 * memory allocated here.
 */

static char **nv_strtok(char *s, char c, int *n)
{
    int count, i, len;
    char **delims, **tokens, *m;
    
    count = count_number_of_chars(s, c);
    
    /*
     * allocate and set pointers to each division (each instance of the
     * dividing character, and the terminating NULL of the string)
     */
    
    delims = (char **) malloc((count+1) * sizeof(char *));
    m = s;
    for (i = 0; i < count; i++) {
        while (*m != c) m++;
        delims[i] = m;
        m++;
    }
    delims[count] = (char *) strchr(s, '\0');
    
    /*
     * so now, we have pointers to each deliminator; copy what's in between
     * the divisions (the tokens) into the dynamic array of strings
     */
    
    tokens = (char **) malloc((count+1) * sizeof(char *));
    len = delims[0] - s;
    tokens[0] = nv_strndup(s, len);
    
    for (i = 1; i < count+1; i++) {
        len = delims[i] - delims[i-1];
        tokens[i] = nv_strndup(delims[i-1]+1, len-1);
    }
    
    free(delims);
    
    *n = count+1;
    return (tokens);
    
} /* nv_strtok() */



/*
 * nv_free_strtoks() - free an array of arrays, such as what is
 * allocated and returned by nv_strtok()
 */

static void nv_free_strtoks(char **s, int n)
{
    int i;
    for (i = 0; i < n; i++) free(s[i]);
    free(s);
    
} /* nv_free_strtoks() */



/*
 * character to integer conversion
 */

static int ctoi(const char c)
{
    return (c - '0');

} /* ctoi */



/*
 * count_number_of_chars() - return the number of times the
 * character d appears in the string
 */

static int count_number_of_chars(char *o, char d)
{
    int c = 0;
    while (*o) {
        if (*o == d) c++;
        o++;
    }
    return (c);
    
} /* count_number_of_chars() */



/*
 * nv_strndup() - this function takes a pointer to a string and a
 * length n, mallocs a new string of n+1, copies the first n chars
 * from the original string into the new, and null terminates the new
 * string.  The caller should free the string.
 */

static char *nv_strndup(char *s, int n)
{
    char *m = (char *) malloc(n+1);
    strncpy (m, s, n);
    m[n] = '\0';
    return (m);
    
} /* nv_strndup() */



/** parse_skip_whitespace() ******************************************
 *
 * Returns a pointer to the start of non-whitespace chars in string 'str'
 *
 **/
const char *parse_skip_whitespace(const char *str)
{
    while (*str &&
           (*str == ' '  || *str == '\t' ||
            *str == '\n' || *str == '\r')) {
        str++;
    }
    return str;

} /* parse_skip_whitespace() */



/** parse_chop_whitespace() ******************************************
 *
 * Removes all trailing whitespace chars from the given string 'str'
 *
 ***/
void parse_chop_whitespace(char *str)
{
    char *tmp = str + strlen(str) -1;
    
    while (tmp >= str &&
           (*tmp == ' '  || *tmp == '\t' ||
            *tmp == '\n' || *tmp == '\r')) {
        *tmp = '\0';
        tmp--;
    }

} /* parse_chop_whitespace() */



/** parse_skip_integer() *********************************************
 *
 * Returns a pointer to the location just after any integer in string 'str'
 *
 **/
const char *parse_skip_integer(const char *str)
{
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str && *str >= '0' && *str <= '9') {
        str++;
    }
    return str;

} /* parse_skip_integer() */



/** parse_read_integer() *********************************************
 *
 * Reads an integer from string str and returns a pointer
 *
 **/
const char *parse_read_integer(const char *str, int *num)
{
    str = parse_skip_whitespace(str);
    *num = atoi(str);
    str = parse_skip_integer(str);
    return parse_skip_whitespace(str);

} /* parse_read_integer() */



/** parse_read_integer_pair() ****************************************
 *
 * Reads two integers separated by 'separator' and returns a pointer
 * to the location in 'str' where parsing finished. (After the two
 * integers).  NULL is returned on failure.
 *
 **/
const char *parse_read_integer_pair(const char *str,
                                    char separator, int *a, int *b)
 {
    str = parse_read_integer(str, a);
    if (!str) return NULL;

    if (separator) {
        if (*str != separator) return NULL;
        str++;
    }
    return parse_read_integer(str, b);

} /* parse_read_integer_pair() */



/** parse_read_name() ************************************************
 *
 * Skips whitespace and copies characters up to and excluding the
 * terminating character 'term'.  The location where parsing stopped
 * is returned, or NULL on failure.
 *
 **/
const char *parse_read_name(const char *str, char **name, char term)
{
    const char *tmp;

    str = parse_skip_whitespace(str);
    tmp = str;
    while (*str && *str != term) {
        str++;
    }
    *name = (char *)calloc(1, str -tmp +1);
    if (!(*name)) {
        return NULL;
    }
    strncpy(*name, tmp, str -tmp);
    if (*str == term) {
        str++;
    }
    return parse_skip_whitespace(str);

} /* parse_read_name() */



/** parse_read_display_name() ****************************************
 *
 * Convert a 'CRT-1' style display device name into a device_mask
 * '0x00000002' bitmask.  The location where parsing stopped is returned
 * or NULL if an error occured.
 *
 **/
const char *parse_read_display_name(const char *str, unsigned int *mask)
{
    if (!str || !mask) {
        return NULL;
    }

    str = parse_skip_whitespace(str);
    if (!strncmp(str, "CRT-", 4)) {
        *mask = 1 << (atoi(str+4));

    } else if (!strncmp(str, "TV-", 3)) {
        *mask = (1 << (atoi(str+3))) << 8;

    } else if (!strncmp(str, "DFP-", 4)) {
        *mask = (1 << (atoi(str+4))) << 16;

    } else {
        return NULL;
    }

    while (*str && *str != ':') {
        str++;
    }
    if (*str == ':') {
        str++;
    }

    return parse_skip_whitespace(str);

} /* parse_read_display_name() */



/** parse_read_float_range() *****************************************
 *
 * Reads the maximun/minimum information from a string in the
 * following format:
 *     "MIN-MAX"
 * or
 *     "MIN"
 *
 **/
int parse_read_float_range(char *str, float *min, float *max)
{
    if (!str) return 0;

    str = (char *)parse_skip_whitespace(str);
    *min = atof(str);
    str = strstr(str, "-");
    if (!str) {
        *max = *min;
        return 1;
    }
    str++;
    *max = atof(str);
    
    return 1;

} /* parse_read_float_range() */



/** parse_token_value_pairs() ****************************************
 *
 * Parses the given string for "token=value, token=value, ..." pairs
 * and dispatches the handeling of tokens to the given function with
 * the given data as an extra argument.
 *
 **/
int parse_token_value_pairs(const char *str, apply_token_func func,
                            void *data)
{
    char *token;
    char *value;


    if (str) {

        /* Parse each token */
        while (*str) {
            
            /* Read the token */
            str = parse_read_name(str, &token, '=');
            if (!str) return 0;
            
            /* Read the value */
            str = parse_read_name(str, &value, ',');
            if (!str) return 0;
            
            /* Remove trailing whitespace */
            parse_chop_whitespace(token);
            parse_chop_whitespace(value);
            
            func(token, value, data);
            
            free(token);
            free(value);
        }
    }

    return 1;

} /* parse_token_value_pairs() */
