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

/*
 * ctkevent.c - the CtkEvent object registers a new input source (the
 * filedescriptor associated with the NV-CONTROL Display connection)
 * with the glib main loop, and emits signals when any relevant
 * NV-CONTROL events occur.  GUI elements can then register
 * callback(s) on the CtkEvent object & Signal(s).
 *
 * In short:
 *   NV-CONTROL -> event -> glib -> CtkEvent -> signal -> GUI
 */

#include <string.h>

#include <gtk/gtk.h>

#include <X11/Xlib.h> /* Xrandr */
#include <X11/extensions/Xrandr.h> /* Xrandr */

#include "ctkevent.h"
#include "NVCtrlLib.h"
#include "msg.h"

static void ctk_event_class_init(CtkEventClass *ctk_event_class);

static gboolean ctk_event_prepare(GSource *, gint *);
static gboolean ctk_event_check(GSource *);
static gboolean ctk_event_dispatch(GSource *, GSourceFunc, gpointer);


/* List of who to contact on dpy events */
typedef struct __CtkEventNodeRec {
    CtkEvent *ctk_event;
    int target_type;
    int target_id;
    struct __CtkEventNodeRec *next;
} CtkEventNode;

/* dpys should have a single event source object */
typedef struct __CtkEventSourceRec {
    GSource source;
    NvCtrlEventHandle *event_handle;
    GPollFD event_poll_fd;

    CtkEventNode *ctk_events;
    struct __CtkEventSourceRec *next;
} CtkEventSource;

static guint binary_signals[NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE + 1];
static guint string_signals[NV_CTRL_STRING_LAST_ATTRIBUTE + 1];
static guint signals[NV_CTRL_LAST_ATTRIBUTE + 1];
static guint signal_RRScreenChangeNotify;

/* List of event sources to track (one per dpy) */
CtkEventSource *event_sources = NULL;



GType ctk_event_get_type(void)
{
    static GType ctk_event_type = 0;

    if (!ctk_event_type) {
        static const GTypeInfo ctk_event_info = {
            sizeof(CtkEventClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) ctk_event_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(CtkEvent),
            0,                  /* n_preallocs */
            NULL,               /* instance_init */
            NULL                /* value_table */
        };

        ctk_event_type = g_type_register_static
            (G_TYPE_OBJECT, "CtkEvent", &ctk_event_info, 0);
    }

    return ctk_event_type;
    
} /* ctk_event_get_type() */


static void ctk_event_class_init(CtkEventClass *ctk_event_class)
{
    gint i;

    /* clear the signal array */

    for (i = 0; i <= NV_CTRL_LAST_ATTRIBUTE; i++) signals[i] = 0;
    
#define MAKE_SIGNAL(x)                                              \
    signals[x] = g_signal_new(("CTK_EVENT_"  #x),                   \
                              G_OBJECT_CLASS_TYPE(ctk_event_class), \
                              G_SIGNAL_RUN_LAST, 0, NULL, NULL,     \
                              g_cclosure_marshal_VOID__POINTER,     \
                              G_TYPE_NONE, 1, G_TYPE_POINTER);
    
    /* create signals for all the NV-CONTROL attributes */
    
    MAKE_SIGNAL(NV_CTRL_DIGITAL_VIBRANCE);
    MAKE_SIGNAL(NV_CTRL_BUS_TYPE);
    MAKE_SIGNAL(NV_CTRL_VIDEO_RAM);
    MAKE_SIGNAL(NV_CTRL_IRQ);
    MAKE_SIGNAL(NV_CTRL_OPERATING_SYSTEM);
    MAKE_SIGNAL(NV_CTRL_SYNC_TO_VBLANK);
    MAKE_SIGNAL(NV_CTRL_LOG_ANISO);
    MAKE_SIGNAL(NV_CTRL_FSAA_MODE);
    MAKE_SIGNAL(NV_CTRL_TEXTURE_SHARPEN);
    MAKE_SIGNAL(NV_CTRL_UBB);
    MAKE_SIGNAL(NV_CTRL_OVERLAY);
    MAKE_SIGNAL(NV_CTRL_STEREO);
    MAKE_SIGNAL(NV_CTRL_EMULATE);
    MAKE_SIGNAL(NV_CTRL_TWINVIEW);
    MAKE_SIGNAL(NV_CTRL_CONNECTED_DISPLAYS);
    MAKE_SIGNAL(NV_CTRL_ENABLED_DISPLAYS);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_MASTER);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_POLARITY);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_DELAY);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_INTERVAL);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_PORT0_STATUS);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_PORT1_STATUS);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_HOUSE_STATUS);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_READY);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_TIMING);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_STEREO_SYNC);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_TEST_SIGNAL);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_ETHERNET_DETECTED);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_VIDEO_MODE);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_RATE);
    MAKE_SIGNAL(NV_CTRL_OPENGL_AA_LINE_GAMMA);
    MAKE_SIGNAL(NV_CTRL_FLIPPING_ALLOWED);
    MAKE_SIGNAL(NV_CTRL_FORCE_STEREO);
    MAKE_SIGNAL(NV_CTRL_ARCHITECTURE);
    MAKE_SIGNAL(NV_CTRL_TEXTURE_CLAMPING);
    MAKE_SIGNAL(NV_CTRL_FSAA_APPLICATION_CONTROLLED);
    MAKE_SIGNAL(NV_CTRL_LOG_ANISO_APPLICATION_CONTROLLED);
    MAKE_SIGNAL(NV_CTRL_IMAGE_SHARPENING);
    MAKE_SIGNAL(NV_CTRL_TV_OVERSCAN);  
    MAKE_SIGNAL(NV_CTRL_TV_FLICKER_FILTER);
    MAKE_SIGNAL(NV_CTRL_TV_BRIGHTNESS);
    MAKE_SIGNAL(NV_CTRL_TV_HUE);
    MAKE_SIGNAL(NV_CTRL_TV_CONTRAST);
    MAKE_SIGNAL(NV_CTRL_TV_SATURATION);
    MAKE_SIGNAL(NV_CTRL_TV_RESET_SETTINGS);
    MAKE_SIGNAL(NV_CTRL_GPU_CORE_TEMPERATURE);
    MAKE_SIGNAL(NV_CTRL_GPU_CORE_THRESHOLD);
    MAKE_SIGNAL(NV_CTRL_GPU_DEFAULT_CORE_THRESHOLD);
    MAKE_SIGNAL(NV_CTRL_GPU_MAX_CORE_THRESHOLD);
    MAKE_SIGNAL(NV_CTRL_AMBIENT_TEMPERATURE);
    MAKE_SIGNAL(NV_CTRL_GVO_SUPPORTED);
    MAKE_SIGNAL(NV_CTRL_GVO_SYNC_MODE);
    MAKE_SIGNAL(NV_CTRL_GVO_SYNC_SOURCE);
    MAKE_SIGNAL(NV_CTRL_GVIO_REQUESTED_VIDEO_FORMAT);
    MAKE_SIGNAL(NV_CTRL_GVIO_DETECTED_VIDEO_FORMAT);
    MAKE_SIGNAL(NV_CTRL_GVO_DATA_FORMAT);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECTED);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE_SYNC_INPUT_DETECT_MODE);
    MAKE_SIGNAL(NV_CTRL_GVO_SDI_SYNC_INPUT_DETECTED);
    MAKE_SIGNAL(NV_CTRL_GVO_VIDEO_OUTPUTS);
    MAKE_SIGNAL(NV_CTRL_GVO_FIRMWARE_VERSION);
    MAKE_SIGNAL(NV_CTRL_GVO_SYNC_DELAY_PIXELS);
    MAKE_SIGNAL(NV_CTRL_GVO_SYNC_DELAY_LINES);
    MAKE_SIGNAL(NV_CTRL_GVO_INPUT_VIDEO_FORMAT_REACQUIRE);
    MAKE_SIGNAL(NV_CTRL_GVO_GLX_LOCKED);
    MAKE_SIGNAL(NV_CTRL_GVIO_VIDEO_FORMAT_WIDTH);
    MAKE_SIGNAL(NV_CTRL_GVIO_VIDEO_FORMAT_HEIGHT);
    MAKE_SIGNAL(NV_CTRL_GVIO_VIDEO_FORMAT_REFRESH_RATE);
    MAKE_SIGNAL(NV_CTRL_FLATPANEL_LINK);
    MAKE_SIGNAL(NV_CTRL_USE_HOUSE_SYNC);
    MAKE_SIGNAL(NV_CTRL_IMAGE_SETTINGS);
    MAKE_SIGNAL(NV_CTRL_XINERAMA_STEREO);
    MAKE_SIGNAL(NV_CTRL_BUS_RATE);
    MAKE_SIGNAL(NV_CTRL_SHOW_SLI_VISUAL_INDICATOR);
    MAKE_SIGNAL(NV_CTRL_XV_SYNC_TO_DISPLAY);
    MAKE_SIGNAL(NV_CTRL_GVO_OVERRIDE_HW_CSC);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE_TERMINATION);
    MAKE_SIGNAL(NV_CTRL_ASSOCIATED_DISPLAY_DEVICES);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SLAVES);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_MASTERABLE);
    MAKE_SIGNAL(NV_CTRL_PROBE_DISPLAYS);
    MAKE_SIGNAL(NV_CTRL_REFRESH_RATE);
    MAKE_SIGNAL(NV_CTRL_INITIAL_PIXMAP_PLACEMENT);
    MAKE_SIGNAL(NV_CTRL_GLYPH_CACHE);
    MAKE_SIGNAL(NV_CTRL_PCI_BUS);
    MAKE_SIGNAL(NV_CTRL_PCI_DEVICE);
    MAKE_SIGNAL(NV_CTRL_PCI_FUNCTION);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_FPGA_REVISION);
    MAKE_SIGNAL(NV_CTRL_MAX_SCREEN_WIDTH);
    MAKE_SIGNAL(NV_CTRL_MAX_SCREEN_HEIGHT);
    MAKE_SIGNAL(NV_CTRL_MAX_DISPLAYS);
    MAKE_SIGNAL(NV_CTRL_MULTIGPU_DISPLAY_OWNER);
    MAKE_SIGNAL(NV_CTRL_GPU_SCALING);
    MAKE_SIGNAL(NV_CTRL_GPU_SCALING_DEFAULT_TARGET);
    MAKE_SIGNAL(NV_CTRL_GPU_SCALING_DEFAULT_METHOD);
    MAKE_SIGNAL(NV_CTRL_FRONTEND_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_BACKEND_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_FLATPANEL_NATIVE_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_FLATPANEL_BEST_FIT_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_GPU_SCALING_ACTIVE);
    MAKE_SIGNAL(NV_CTRL_DFP_SCALING_ACTIVE);
    MAKE_SIGNAL(NV_CTRL_FSAA_APPLICATION_ENHANCED);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_RATE_4);
    MAKE_SIGNAL(NV_CTRL_GVO_LOCK_OWNER);
    MAKE_SIGNAL(NV_CTRL_NUM_GPU_ERRORS_RECOVERED);
    MAKE_SIGNAL(NV_CTRL_REFRESH_RATE_3);
    MAKE_SIGNAL(NV_CTRL_GVO_OUTPUT_VIDEO_LOCKED);
    MAKE_SIGNAL(NV_CTRL_GVO_SYNC_LOCK_STATUS);
    MAKE_SIGNAL(NV_CTRL_GVO_ANC_TIME_CODE_GENERATION);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE_ALPHA_KEY);
    MAKE_SIGNAL(NV_CTRL_GVO_COMPOSITE_NUM_KEY_RANGES);
    MAKE_SIGNAL(NV_CTRL_NOTEBOOK_DISPLAY_CHANGE_LID_EVENT);
    MAKE_SIGNAL(NV_CTRL_MODE_SET_EVENT);
    MAKE_SIGNAL(NV_CTRL_OPENGL_AA_LINE_GAMMA_VALUE);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SLAVEABLE);
    MAKE_SIGNAL(NV_CTRL_DISPLAYPORT_LINK_RATE);
    MAKE_SIGNAL(NV_CTRL_STEREO_EYES_EXCHANGE);
    MAKE_SIGNAL(NV_CTRL_NO_SCANOUT);
    MAKE_SIGNAL(NV_CTRL_GVO_CSC_CHANGED_EVENT);
    MAKE_SIGNAL(NV_CTRL_X_SERVER_UNIQUE_ID);
    MAKE_SIGNAL(NV_CTRL_PIXMAP_CACHE);
    MAKE_SIGNAL(NV_CTRL_PIXMAP_CACHE_ROUNDING_SIZE_KB);
    MAKE_SIGNAL(NV_CTRL_IS_GVO_DISPLAY);
    MAKE_SIGNAL(NV_CTRL_PCI_ID);
    MAKE_SIGNAL(NV_CTRL_GVO_FULL_RANGE_COLOR);
    MAKE_SIGNAL(NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE);
    MAKE_SIGNAL(NV_CTRL_GVO_ENABLE_RGB_DATA);
    MAKE_SIGNAL(NV_CTRL_IMAGE_SHARPENING_DEFAULT);
    MAKE_SIGNAL(NV_CTRL_GVI_NUM_JACKS);
    MAKE_SIGNAL(NV_CTRL_GVI_MAX_LINKS_PER_STREAM);
    MAKE_SIGNAL(NV_CTRL_GVI_DETECTED_CHANNEL_BITS_PER_COMPONENT);
    MAKE_SIGNAL(NV_CTRL_GVI_REQUESTED_STREAM_BITS_PER_COMPONENT);
    MAKE_SIGNAL(NV_CTRL_GVI_DETECTED_CHANNEL_COMPONENT_SAMPLING);
    MAKE_SIGNAL(NV_CTRL_GVI_REQUESTED_STREAM_COMPONENT_SAMPLING);
    MAKE_SIGNAL(NV_CTRL_GVI_REQUESTED_STREAM_CHROMA_EXPAND);
    MAKE_SIGNAL(NV_CTRL_GVI_DETECTED_CHANNEL_COLOR_SPACE);
    MAKE_SIGNAL(NV_CTRL_GVI_DETECTED_CHANNEL_LINK_ID);
    MAKE_SIGNAL(NV_CTRL_GVI_DETECTED_CHANNEL_SMPTE352_IDENTIFIER);
    MAKE_SIGNAL(NV_CTRL_GVI_GLOBAL_IDENTIFIER);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_GPU_COOLER_MANUAL_CONTROL);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_LEVEL);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_LEVEL_SET_DEFAULT);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_CONTROL_TYPE);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_TARGET);
    MAKE_SIGNAL(NV_CTRL_GPU_ECC_CONFIGURATION);
    MAKE_SIGNAL(NV_CTRL_GPU_POWER_MIZER_MODE);
    MAKE_SIGNAL(NV_CTRL_GVI_SYNC_OUTPUT_FORMAT);
    MAKE_SIGNAL(NV_CTRL_GVI_MAX_CHANNELS_PER_JACK);
    MAKE_SIGNAL(NV_CTRL_GVI_MAX_STREAMS);
    MAKE_SIGNAL(NV_CTRL_GVI_NUM_CAPTURE_SURFACES);
    MAKE_SIGNAL(NV_CTRL_OVERSCAN_COMPENSATION);
    MAKE_SIGNAL(NV_CTRL_GPU_PCIE_GENERATION);
    MAKE_SIGNAL(NV_CTRL_GVI_BOUND_GPU);
    MAKE_SIGNAL(NV_CTRL_ACCELERATE_TRAPEZOIDS);
    MAKE_SIGNAL(NV_CTRL_GPU_CORES);
    MAKE_SIGNAL(NV_CTRL_GPU_MEMORY_BUS_WIDTH);
    MAKE_SIGNAL(NV_CTRL_GVI_TEST_MODE);
    MAKE_SIGNAL(NV_CTRL_COLOR_SPACE);
    MAKE_SIGNAL(NV_CTRL_COLOR_RANGE);
    MAKE_SIGNAL(NV_CTRL_CURRENT_COLOR_SPACE);
    MAKE_SIGNAL(NV_CTRL_CURRENT_COLOR_RANGE);
    MAKE_SIGNAL(NV_CTRL_DITHERING);
    MAKE_SIGNAL(NV_CTRL_DITHERING_MODE);
    MAKE_SIGNAL(NV_CTRL_DITHERING_DEPTH);
    MAKE_SIGNAL(NV_CTRL_CURRENT_DITHERING);
    MAKE_SIGNAL(NV_CTRL_CURRENT_DITHERING_MODE);
    MAKE_SIGNAL(NV_CTRL_CURRENT_DITHERING_DEPTH);
    MAKE_SIGNAL(NV_CTRL_THERMAL_SENSOR_READING);
    MAKE_SIGNAL(NV_CTRL_THERMAL_SENSOR_PROVIDER);
    MAKE_SIGNAL(NV_CTRL_THERMAL_SENSOR_TARGET);
    MAKE_SIGNAL(NV_CTRL_SHOW_MULTIGPU_VISUAL_INDICATOR);
    MAKE_SIGNAL(NV_CTRL_GPU_CURRENT_PROCESSOR_CLOCK_FREQS);
    MAKE_SIGNAL(NV_CTRL_GVIO_VIDEO_FORMAT_FLAGS);
    MAKE_SIGNAL(NV_CTRL_GPU_PCIE_MAX_LINK_SPEED);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_MODE);
    MAKE_SIGNAL(NV_CTRL_SYNCHRONOUS_PALETTE_UPDATES);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_TRANSCEIVER_CHANNEL_QUALITY);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_GLASSES_MISSED_SYNC_CYCLES);
    MAKE_SIGNAL(NV_CTRL_GVO_ANC_PARITY_COMPUTATION);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_GLASSES_PAIR_EVENT);
    MAKE_SIGNAL(NV_CTRL_3D_VISION_PRO_GLASSES_UNPAIR_EVENT);
    MAKE_SIGNAL(NV_CTRL_GPU_PCIE_MAX_LINK_WIDTH);
    MAKE_SIGNAL(NV_CTRL_GPU_PCIE_CURRENT_LINK_WIDTH);
    MAKE_SIGNAL(NV_CTRL_GPU_PCIE_CURRENT_LINK_SPEED);
    MAKE_SIGNAL(NV_CTRL_GVO_AUDIO_BLANKING);
    MAKE_SIGNAL(NV_CTRL_CURRENT_METAMODE_ID);
    MAKE_SIGNAL(NV_CTRL_DISPLAY_ENABLED);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_INCOMING_HOUSE_SYNC_RATE);
    MAKE_SIGNAL(NV_CTRL_FXAA);
    MAKE_SIGNAL(NV_CTRL_DISPLAY_RANDR_OUTPUT_ID);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_DISPLAY_CONFIG);
    MAKE_SIGNAL(NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY);
    MAKE_SIGNAL(NV_CTRL_USED_DEDICATED_GPU_MEMORY);
    MAKE_SIGNAL(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_IMMEDIATE);
    MAKE_SIGNAL(NV_CTRL_GPU_DOUBLE_PRECISION_BOOST_REBOOT);
    MAKE_SIGNAL(NV_CTRL_DPY_HDMI_3D);
    MAKE_SIGNAL(NV_CTRL_BASE_MOSAIC);
    MAKE_SIGNAL(NV_CTRL_MULTIGPU_MASTER_POSSIBLE);
    MAKE_SIGNAL(NV_CTRL_GPU_POWER_MIZER_DEFAULT_MODE);
    MAKE_SIGNAL(NV_CTRL_XV_SYNC_TO_DISPLAY_ID);
    MAKE_SIGNAL(NV_CTRL_CURRENT_XV_SYNC_TO_DISPLAY_ID);
    MAKE_SIGNAL(NV_CTRL_BACKLIGHT_BRIGHTNESS);
    MAKE_SIGNAL(NV_CTRL_GPU_LOGO_BRIGHTNESS);
    MAKE_SIGNAL(NV_CTRL_GPU_SLI_LOGO_BRIGHTNESS);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_SPEED);
    MAKE_SIGNAL(NV_CTRL_PALETTE_UPDATE_EVENT);
    MAKE_SIGNAL(NV_CTRL_VIDEO_ENCODER_UTILIZATION);
    MAKE_SIGNAL(NV_CTRL_GSYNC_ALLOWED);
    MAKE_SIGNAL(NV_CTRL_GPU_NVCLOCK_OFFSET);
    MAKE_SIGNAL(NV_CTRL_GPU_MEM_TRANSFER_RATE_OFFSET);
    MAKE_SIGNAL(NV_CTRL_VIDEO_DECODER_UTILIZATION);
    MAKE_SIGNAL(NV_CTRL_GPU_OVER_VOLTAGE_OFFSET);
    MAKE_SIGNAL(NV_CTRL_GPU_CURRENT_CORE_VOLTAGE);
    MAKE_SIGNAL(NV_CTRL_SHOW_GSYNC_VISUAL_INDICATOR);
    MAKE_SIGNAL(NV_CTRL_THERMAL_COOLER_CURRENT_LEVEL);
    MAKE_SIGNAL(NV_CTRL_STEREO_SWAP_MODE);
    MAKE_SIGNAL(NV_CTRL_GPU_FRAMELOCK_FIRMWARE_UNSUPPORTED);
#undef MAKE_SIGNAL

    /*
     * When new integer attributes are added to NVCtrl.h, a
     * MAKE_SIGNAL() line should be added above.  The below #if should
     * also be updated to indicate the last attribute that ctkevent.c
     * knows about.
     */

#if NV_CTRL_LAST_ATTRIBUTE != NV_CTRL_GPU_FRAMELOCK_FIRMWARE_UNSUPPORTED
#warning "There are attributes that do not emit signals!"
#endif

    /* make signals for string attribute */
    for (i = 0; i <= NV_CTRL_STRING_LAST_ATTRIBUTE; i++) string_signals[i] = 0;

#define MAKE_STRING_SIGNAL(x)                                              \
        string_signals[x] = g_signal_new(("CTK_EVENT_"  #x),                   \
                                  G_OBJECT_CLASS_TYPE(ctk_event_class), \
                                  G_SIGNAL_RUN_LAST, 0, NULL, NULL,     \
                                  g_cclosure_marshal_VOID__POINTER,     \
                                  G_TYPE_NONE, 1, G_TYPE_POINTER);

    MAKE_STRING_SIGNAL(NV_CTRL_STRING_PRODUCT_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VBIOS_VERSION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_NVIDIA_DRIVER_VERSION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_DEVICE_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_TV_ENCODER_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GVIO_FIRMWARE_VERSION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_CURRENT_MODELINE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_ADD_MODELINE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DELETE_MODELINE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_CURRENT_METAMODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_ADD_METAMODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DELETE_METAMODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_PRODUCT_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_PRODUCT_ID);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_SERIAL_NUMBER);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_BUILD_DATE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_FIRMWARE_VERSION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_FIRMWARE_REVISION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_HARDWARE_VERSION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VCSC_HARDWARE_REVISION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_MOVE_METAMODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VALID_HORIZ_SYNC_RANGES);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_VALID_VERT_REFRESH_RANGES);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_XINERAMA_SCREEN_INFO);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_NVIDIA_XINERAMA_INFO_ORDER);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_SLI_MODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_PERFORMANCE_MODES);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GVIO_VIDEO_FORMAT_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_3D_VISION_PRO_GLASSES_NAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_TYPE_BASENAME);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_TYPE_ID);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_DP_GUID);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_EDID_HASH);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_TARGET_INDEX);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_DISPLAY_NAME_RANDR);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GPU_UUID);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GPU_UTILIZATION);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_MULTIGPU_MODE);
#undef MAKE_STRING_SIGNAL

#if NV_CTRL_STRING_LAST_ATTRIBUTE != NV_CTRL_STRING_MULTIGPU_MODE
#warning "There are attributes that do not emit signals!"
#endif


    /* make signals for binary attribute */
    for (i = 0; i <= NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE; i++) binary_signals[i] = 0;

#define MAKE_BINARY_SIGNAL(x)                                              \
    binary_signals[x] = g_signal_new(("CTK_EVENT_"  #x),                   \
                                     G_OBJECT_CLASS_TYPE(ctk_event_class), \
                                     G_SIGNAL_RUN_LAST, 0, NULL, NULL,     \
                                     g_cclosure_marshal_VOID__POINTER,     \
                                     G_TYPE_NONE, 1, G_TYPE_POINTER);

    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_MODELINES);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_XSCREENS_USING_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPUS_USING_FRAMELOCK);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAY_VIEWPORT);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_FRAMELOCKS_USED_BY_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPUS_USING_VCSC);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_VCSCS_USED_BY_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_COOLERS_USED_BY_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_THERMAL_SENSORS_USED_BY_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAY_TARGETS);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAYS_CONNECTED_TO_GPU);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_METAMODES_VERSION_2);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAYS_ASSIGNED_TO_XSCREEN);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPU_FLAGS);
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU);
#undef MAKE_BINARY_SIGNAL

#if NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE != NV_CTRL_BINARY_DATA_DISPLAYS_ON_GPU
#warning "There are attributes that do not emit signals!"
#endif

    /* Make XRandR signal */
    signal_RRScreenChangeNotify =
        g_signal_new("CTK_EVENT_RRScreenChangeNotify",
                     G_OBJECT_CLASS_TYPE(ctk_event_class),
                     G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                     g_cclosure_marshal_VOID__POINTER,
                     G_TYPE_NONE, 1, G_TYPE_POINTER);


} /* ctk_event_class_init */



static CtkEventSource* find_event_source(NvCtrlEventHandle *event_handle)
{
    CtkEventSource *event_source = event_sources;
    while (event_source) {
        if (event_source->event_handle == event_handle) {
            break;
        }
        event_source = event_source->next;
    }
    return event_source;
}



/* - ctk_event_register_source()
 *
 * Keep track of event sources globally to support
 * dispatching events on an event handle to multiple CtkEvent
 * objects.  Since the driver only sends out one event
 * notification per event handle (client), there should only be one
 * event source attached per unique event handle.  When an event
 * is received, the dispatching function should then
 * emit a signal to every CtkEvent object that
 * requests event notification from the event handle for the
 * given target type/id (X screen, GPU etc).
 */
static void ctk_event_register_source(CtkEvent *ctk_event)
{
    CtrlTarget *ctrl_target = ctk_event->ctrl_target;
    NvCtrlEventHandle *event_handle = NvCtrlGetEventHandle(ctrl_target);
    CtkEventSource *event_source;
    CtkEventNode *event_node;

    if (!event_handle) {
        return;
    }

    /* Do we already have an event source for this event handle? */
    event_source = find_event_source(event_handle);

    /* create a new input source */
    if (!event_source) {
        GSource *source;
        int event_fd;

        static GSourceFuncs ctk_source_funcs = {
            ctk_event_prepare,
            ctk_event_check,
            ctk_event_dispatch,
            NULL, /* finalize */
            NULL, /* closure_callback */
            NULL, /* closure_marshal */
        };

        source = g_source_new(&ctk_source_funcs, sizeof(CtkEventSource));
        event_source = (CtkEventSource *) source;
        if (!event_source) {
            return;
        }
        
        NvCtrlEventHandleGetFD(event_handle, &event_fd);
        event_source->event_handle = event_handle;
        event_source->event_poll_fd.fd = event_fd;
        event_source->event_poll_fd.events = G_IO_IN;
        
        /* add the input source to the glib main loop */
        
        g_source_add_poll(source, &event_source->event_poll_fd);
        g_source_attach(source, NULL);

        /* add the source to the global list of sources */

        event_source->next = event_sources;
        event_sources = event_source;
    }


    /* Add the ctk_event object to the source's list of event objects */

    event_node = (CtkEventNode *)g_malloc(sizeof(CtkEventNode));
    if (!event_node) {
        return;
    }
    event_node->ctk_event = ctk_event;
    event_node->target_type = NvCtrlGetTargetType(ctrl_target);
    event_node->target_id = NvCtrlGetTargetId(ctrl_target);
    event_node->next = event_source->ctk_events;
    event_source->ctk_events = event_node;

} /* ctk_event_register_source() */



/*
 * Unregister a previously registered CtkEvent from its corresponding event
 * source. If the event source becomes empty (no CtkEvents attached to it), this
 * function also destroys the event source and its corresponding event handle.
 */
static void ctk_event_unregister_source(CtkEvent *ctk_event)
{
    CtrlTarget *ctrl_target = ctk_event->ctrl_target;
    NvCtrlEventHandle *event_handle = NvCtrlGetEventHandle(ctrl_target);
    CtkEventSource *event_source;
    CtkEventNode *event_node;

    if (!event_handle) {
        return;
    }

    /* Do we have an event source for this event handle? */
    event_source = find_event_source(event_handle);

    if (!event_source) {
        return;
    }


    /* Remove the ctk_event object from the source's list of event objects */

    event_node = event_source->ctk_events;
    if (event_node->ctk_event == ctk_event) {
        event_source->ctk_events = event_node->next;
    }
    else {
        CtkEventNode *prev = event_node;
        event_node = event_node->next;
        while (event_node) {
            if (event_node->ctk_event == ctk_event) {
                prev->next = event_node->next;
                break;
            }
            prev = event_node;
            event_node = event_node->next;
        }
    }

    if (!event_node) {
        return;
    }

    g_free(event_node);


    /* destroy the event source if empty */

    if (event_source->ctk_events == NULL) {
        GSource *source = (GSource *)event_source;

        if (event_sources == event_source) {
            event_sources = event_source->next;
        }
        else {
            CtkEventSource *cur;
            for (cur = event_sources; cur; cur = cur->next) {
                if (cur->next == event_source) {
                    cur->next = event_source->next;
                    break;
                }
            }
        }

        NvCtrlCloseEventHandle(event_source->event_handle);
        g_source_remove_poll(source, &(event_source->event_poll_fd));
        g_source_destroy(source);
        g_source_unref(source);
    }
}



GObject *ctk_event_new(CtrlTarget *ctrl_target)
{
    GObject *object;
    CtkEvent *ctk_event;

    /* create the new object */

    object = g_object_new(CTK_TYPE_EVENT, NULL);

    ctk_event = CTK_EVENT(object);
    ctk_event->ctrl_target = ctrl_target;
    
    /* Register to receive (dpy) events */

    ctk_event_register_source(ctk_event);
    
    return G_OBJECT(ctk_event);

} /* ctk_event_new() */



void ctk_event_destroy(GObject *object)
{
    CtkEvent *ctk_event;

    if (object == NULL || !CTK_IS_EVENT(object)) {
        return;
    }

    ctk_event = CTK_EVENT(object);

    /* Unregister to stop receiving (dpy) events */

    ctk_event_unregister_source(ctk_event);

    /* Unref the CtkEvent object */

    g_object_unref(object);
}



static gboolean ctk_event_prepare(GSource *source, gint *timeout)
{
    ReturnStatus status;
    Bool pending;
    CtkEventSource *event_source = (CtkEventSource *) source;
    *timeout = -1;

    /*
     * Check if any events are pending on the event handle
     */
    status = NvCtrlEventHandlePending(event_source->event_handle, &pending);
    if (status == NvCtrlSuccess) {
        return pending;
    }

    return FALSE;
}



static gboolean ctk_event_check(GSource *source)
{
    ReturnStatus status;
    Bool pending;
    CtkEventSource *event_source = (CtkEventSource *) source;

    /*
     * XXX We could check for (event_source->event_poll_fd.revents & G_IO_IN),
     * but doing so caused some events to be missed as they came in with only
     * the G_IO_OUT flag set which is odd.
     */
    status = NvCtrlEventHandlePending(event_source->event_handle, &pending);
    if (status == NvCtrlSuccess) {
        return pending;
    }

    return FALSE;
}



#define CTK_EVENT_BROADCAST(ES, SIG, CEVT)             \
do {                                                   \
    CtkEventNode *e = (ES)->ctk_events;                \
    while  (e) {                                       \
        if (e->target_type == (CEVT)->target_type &&   \
            e->target_id == (CEVT)->target_id) {       \
            g_signal_emit(e->ctk_event, SIG, 0, CEVT); \
        }                                              \
        e = e->next;                                   \
    }                                                  \
} while (0)

static gboolean ctk_event_dispatch(GSource *source,
                                   GSourceFunc callback,
                                   gpointer user_data)
{
    ReturnStatus status;
    CtrlEvent event;
    CtkEventSource *event_source = (CtkEventSource *) source;

    /*
     * if ctk_event_dispatch() is called, then either
     * ctk_event_prepare() or ctk_event_check() returned TRUE, so we
     * know there is an event pending
     */
    status = NvCtrlEventHandleNextEvent(event_source->event_handle, &event);
    if (status != NvCtrlSuccess) {
        return FALSE;
    }

    if (event.type != CTRL_EVENT_TYPE_UNKNOWN) {

        /* 
         * Handle the CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE event
         */
        if (event.type == CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {

            /* make sure the attribute is in our signal array */
            if ((event.int_attr.attribute <= NV_CTRL_LAST_ATTRIBUTE) &&
                (signals[event.int_attr.attribute] != 0)) {

                /*
                 * XXX Is emitting a signal with g_signal_emit() really
                 * the "correct" way of dispatching the event?
                 */
                CTK_EVENT_BROADCAST(event_source,
                                    signals[event.int_attr.attribute],
                                    &event);
            }
        }
        
        /* 
         * Handle the CTRL_EVENT_TYPE_STRING_ATTRIBUTE event
         */
        else if (event.type == CTRL_EVENT_TYPE_STRING_ATTRIBUTE) {

            /* make sure the attribute is in our string signal array */

            if ((event.str_attr.attribute <= NV_CTRL_STRING_LAST_ATTRIBUTE) &&
                (string_signals[event.str_attr.attribute] != 0)) {

                /*
                 * XXX Is emitting a signal with g_signal_emit() really
                 * the "correct" way of dispatching the event
                 */
                CTK_EVENT_BROADCAST(event_source,
                                    string_signals[event.str_attr.attribute],
                                    &event);
            }
        }

        /*
         * Handle the CTRL_EVENT_TYPE_BINARY_ATTRIBUTE event
         */
        else if (event.type == CTRL_EVENT_TYPE_BINARY_ATTRIBUTE) {

            /* make sure the attribute is in our binary signal array */
            if ((event.bin_attr.attribute <= NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE) &&
                (binary_signals[event.bin_attr.attribute] != 0)) {

                /*
                 * XXX Is emitting a signal with g_signal_emit() really
                 * the "correct" way of dispatching the event
                 */
                CTK_EVENT_BROADCAST(event_source,
                                    binary_signals[event.bin_attr.attribute],
                                    &event);
            }
        }

        /*
         * Handle the CTRL_EVENT_TYPE_SCREEN_CHANGE event
         */
        else if (event.type == CTRL_EVENT_TYPE_SCREEN_CHANGE) {

            /* make sure the target_id is valid */
            if (event.target_id >= 0) {
                CTK_EVENT_BROADCAST(event_source,
                                    signal_RRScreenChangeNotify,
                                    &event);
            }
        }
    }
    
    return TRUE;

} /* ctk_event_dispatch() */



/* ctk_event_emit() - Emits signal(s) on a registered ctk_event object.
 * This function is primarily used to simulate NV-CONTROL events such
 * that various parts of nvidia-settings can communicate (internally)
 */
void ctk_event_emit(CtkEvent *ctk_event,
                    unsigned int mask,
                    int attrib,
                    int value)
{
    CtrlEvent event;
    CtkEventSource *source;
    CtrlTarget *ctrl_target = ctk_event->ctrl_target;
    NvCtrlEventHandle *event_handle = NvCtrlGetEventHandle(ctrl_target);


    if (attrib > NV_CTRL_LAST_ATTRIBUTE) return;


    /* Find the event source */
    source = event_sources;
    while (source) {
        if (source->event_handle == event_handle) {
            break;
        }
        source = source->next;
    }
    if (!source) return;


    /* Broadcast event to all relevant ctk_event objects */
    memset(&event, 0, sizeof(CtrlEvent));

    event.type        = CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE;
    event.target_type = NvCtrlGetTargetType(ctrl_target);
    event.target_id   = NvCtrlGetTargetId(ctrl_target);

    event.int_attr.attribute = attrib;
    event.int_attr.value     = value;

    CTK_EVENT_BROADCAST(source, signals[attrib], &event);

} /* ctk_event_emit() */



/* ctk_event_emit_string() - Emits signal(s) on a registered ctk_event object.
 * This function is primarily used to simulate NV-CONTROL events such
 * that various parts of nvidia-settings can communicate (internally)
 */
void ctk_event_emit_string(CtkEvent *ctk_event,
                           unsigned int mask,
                           int attrib)
{
    CtrlEvent event;
    CtkEventSource *source;
    CtrlTarget *ctrl_target = ctk_event->ctrl_target;
    NvCtrlEventHandle *event_handle = NvCtrlGetEventHandle(ctrl_target);


    if (attrib > NV_CTRL_STRING_LAST_ATTRIBUTE) return;


    /* Find the event source */
    source = event_sources;
    while (source) {
        if (source->event_handle == event_handle) {
            break;
        }
        source = source->next;
    }
    if (!source) return;


    /* Broadcast event to all relevant ctk_event objects */
    memset(&event, 0, sizeof(CtrlEvent));

    event.type        = CTRL_EVENT_TYPE_STRING_ATTRIBUTE;
    event.target_type = NvCtrlGetTargetType(ctrl_target);
    event.target_id   = NvCtrlGetTargetId(ctrl_target);

    event.str_attr.attribute = attrib;

    CTK_EVENT_BROADCAST(source, signals[attrib], &event);

} /* ctk_event_emit_string() */

