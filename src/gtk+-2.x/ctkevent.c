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
    Display *dpy;
    GPollFD event_poll_fd;
    int event_base;
    int randr_event_base;

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
        };

        ctk_event_type = g_type_register_static
            (GTK_TYPE_OBJECT, "CtkEvent", &ctk_event_info, 0);
    }

    return ctk_event_type;
    
} /* ctk_event_get_type() */


static void ctk_event_class_init(CtkEventClass *ctk_event_class)
{
    GObjectClass *gobject_class;
    gint i;

    gobject_class = (GObjectClass *) ctk_event_class;

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
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_ALPHA);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_RED);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_GREEN);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_BLUE);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_X_OFFSET);
    MAKE_SIGNAL(NV_CTRL_CURSOR_SHADOW_Y_OFFSET);
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
    MAKE_SIGNAL(NV_CTRL_GVO_DISPLAY_X_SCREEN);
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
    MAKE_SIGNAL(NV_CTRL_GVO_X_SCREEN_PAN_X);
    MAKE_SIGNAL(NV_CTRL_GVO_X_SCREEN_PAN_Y);
    MAKE_SIGNAL(NV_CTRL_GPU_OVERCLOCKING_STATE);
    MAKE_SIGNAL(NV_CTRL_GPU_2D_CLOCK_FREQS);
    MAKE_SIGNAL(NV_CTRL_GPU_3D_CLOCK_FREQS);
    MAKE_SIGNAL(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS);
    MAKE_SIGNAL(NV_CTRL_GPU_OPTIMAL_CLOCK_FREQS_DETECTION_STATE);
    MAKE_SIGNAL(NV_CTRL_USE_HOUSE_SYNC);
    MAKE_SIGNAL(NV_CTRL_IMAGE_SETTINGS);
    MAKE_SIGNAL(NV_CTRL_XINERAMA_STEREO);
    MAKE_SIGNAL(NV_CTRL_BUS_RATE);
    MAKE_SIGNAL(NV_CTRL_SHOW_SLI_HUD);
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
    MAKE_SIGNAL(NV_CTRL_DYNAMIC_TWINVIEW);
    MAKE_SIGNAL(NV_CTRL_MULTIGPU_DISPLAY_OWNER);
    MAKE_SIGNAL(NV_CTRL_GPU_SCALING);
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
    MAKE_SIGNAL(NV_CTRL_ONDEMAND_VBLANK_INTERRUPTS);
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
    MAKE_SIGNAL(NV_CTRL_GVI_NUM_PORTS);
    MAKE_SIGNAL(NV_CTRL_FRAMELOCK_SYNC_DELAY_RESOLUTION);
    MAKE_SIGNAL(NV_CTRL_GPU_POWER_MIZER_MODE);

#undef MAKE_SIGNAL
    
    /*
     * When new integer attributes are added to NVCtrl.h, a
     * MAKE_SIGNAL() line should be added above.  The below #if should
     * also be updated to indicate the last attribute that ctkevent.c
     * knows about.
     */

#if NV_CTRL_LAST_ATTRIBUTE != NV_CTRL_GPU_POWER_MIZER_MODE
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
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_SLI_MODE);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_PERFORMANCE_MODES);
    MAKE_STRING_SIGNAL(NV_CTRL_STRING_GVIO_VIDEO_FORMAT_NAME);
#undef MAKE_STRING_SIGNAL

#if NV_CTRL_STRING_LAST_ATTRIBUTE != NV_CTRL_STRING_GVO_VIDEO_FORMAT_NAME
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
    MAKE_BINARY_SIGNAL(NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN);

#undef MAKE_BINARY_SIGNAL
    
#if NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE != NV_CTRL_BINARY_DATA_GPUS_USED_BY_LOGICAL_XSCREEN
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



/* - ctk_event_register_source()
 *
 * Keep track of event sources globally to support
 * dispatching events on a dpy to multiple CtkEvent
 * objects.  Since the driver only sends out one event
 * notification per dpy (client), there should only be one
 * event source attached per unique dpy.  When an event
 * is received, the dispatching function should then
 * emit a signal to every CtkEvent object that
 * requests event notification from the dpy for the
 * given target type/id (X screen, GPU etc).
 */
static void ctk_event_register_source(CtkEvent *ctk_event)
{
    Display *dpy = NvCtrlGetDisplayPtr(ctk_event->handle);
    CtkEventSource *event_source;
    CtkEventNode *event_node;

    if (!dpy) {
        return;
    }

    /* Do we already have an event source for this dpy? */
    event_source = event_sources;
    while (event_source) {
        if (event_source->dpy == dpy) {
            break;
        }
        event_source = event_source->next;
    }

    /* create a new input source */
    if (!event_source) {
        GSource *source;

        static GSourceFuncs ctk_source_funcs = {
            ctk_event_prepare,
            ctk_event_check,
            ctk_event_dispatch,
            NULL
        };

        source = g_source_new(&ctk_source_funcs, sizeof(CtkEventSource));
        event_source = (CtkEventSource *) source;
        if (!event_source) {
            return;
        }
        
        event_source->dpy = dpy;
        event_source->event_poll_fd.fd = ConnectionNumber(dpy);
        event_source->event_poll_fd.events = G_IO_IN;
        event_source->event_base = NvCtrlGetEventBase(ctk_event->handle);
        event_source->randr_event_base =
            NvCtrlGetXrandrEventBase(ctk_event->handle);
        
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
    event_node->target_type = NvCtrlGetTargetType(ctk_event->handle);
    event_node->target_id = NvCtrlGetTargetId(ctk_event->handle);
    event_node->next = event_source->ctk_events;
    event_source->ctk_events = event_node;
    /*
     * This next bit of code is to make sure that the randr_event_base
     * for this event source is valid in the case where a NON X Screen
     * target type handle is used to create the initial event source
     * (Resulting in randr_event_base being == -1), followed by an
     * X Screen target type handle registering itself to receive
     * XRandR events on the existing dpy/event source.
     */
    if (event_source->randr_event_base == -1 &&
        event_node->target_type == NV_CTRL_TARGET_TYPE_X_SCREEN) {
        event_source->randr_event_base =
            NvCtrlGetXrandrEventBase(ctk_event->handle);
    }

} /* ctk_event_register_source() */



GtkObject *ctk_event_new(NvCtrlAttributeHandle *handle)
{
    GObject *object;
    CtkEvent *ctk_event;

    /* create the new object */

    object = g_object_new(CTK_TYPE_EVENT, NULL);

    ctk_event = CTK_EVENT(object);
    ctk_event->handle = handle;
    
    /* Register to receive (dpy) events */

    ctk_event_register_source(ctk_event);
    
    return GTK_OBJECT(ctk_event);

} /* ctk_event_new() */



static gboolean ctk_event_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    
    /*
     * XXX We could check if any events are pending on the Display
     * connection
     */
    
    return FALSE;
}



static gboolean ctk_event_check(GSource *source)
{
    CtkEventSource *event_source = (CtkEventSource *) source;

    /*
     * XXX We could check for (event_source->event_poll_fd.revents & G_IO_IN),
     * but doing so caused some events to be missed as they came in with only
     * the G_IO_OUT flag set which is odd.
     */
    return XPending(event_source->dpy);
}



static int get_screen_of_root(Display *dpy, Window root)
{
    int screen = -1;

    /* Find the screen the window belongs to */
    screen = XScreenCount(dpy);

    while (screen > 0) {
        screen--;
        if (root == RootWindow(dpy, screen)) {
            break;
        }
    }
    
    return screen;
}



#define CTK_EVENT_BROADCAST(ES, SIG, PTR, TYPE, ID)   \
do {                                                  \
    CtkEventNode *e = (ES)->ctk_events;               \
    while  (e) {                                      \
        if (e->target_type == (TYPE) &&               \
            e->target_id == (ID)) {                   \
            g_signal_emit(e->ctk_event, SIG, 0, PTR); \
        }                                             \
        e = e->next;                                  \
    }                                                 \
} while (0)

static gboolean ctk_event_dispatch(GSource *source,
                                   GSourceFunc callback, gpointer user_data)
{
    XEvent event;
    CtkEventSource *event_source = (CtkEventSource *) source;
    CtkEventStruct event_struct;

    /*
     * if ctk_event_dispatch() is called, then either
     * ctk_event_prepare() or ctk_event_check() returned TRUE, so we
     * know there is an event pending
     */
    
    XNextEvent(event_source->dpy, &event);

    /* 
     * Handle the ATTRIBUTE_CHANGED_EVENT NV-CONTROL event
     */

    if (event_source->event_base != -1 &&
        (event.type == (event_source->event_base + ATTRIBUTE_CHANGED_EVENT))) {

        XNVCtrlAttributeChangedEvent *nvctrlevent =
            (XNVCtrlAttributeChangedEvent *) &event;

        /* make sure the attribute is in our signal array */

        if ((nvctrlevent->attribute >= 0) &&
            (nvctrlevent->attribute <= NV_CTRL_LAST_ATTRIBUTE) &&
            (signals[nvctrlevent->attribute] != 0)) {
            
            event_struct.attribute    = nvctrlevent->attribute;
            event_struct.value        = nvctrlevent->value;
            event_struct.display_mask = nvctrlevent->display_mask;
            event_struct.availability = TRUE;

            /*
             * XXX Is emitting a signal with g_signal_emit() really
             * the "correct" way of dispatching the event?
             */

            CTK_EVENT_BROADCAST(event_source,
                                signals[nvctrlevent->attribute],
                                &event_struct,
                                NV_CTRL_TARGET_TYPE_X_SCREEN,
                                nvctrlevent->screen);
        }

    /* 
     * Handle the TARGET_ATTRIBUTE_CHANGED_EVENT NV-CONTROL event
     */

    } else if (event_source->event_base != -1 &&
               (event.type == (event_source->event_base
                               +TARGET_ATTRIBUTE_CHANGED_EVENT))) {

        XNVCtrlAttributeChangedEventTarget *nvctrlevent =
            (XNVCtrlAttributeChangedEventTarget *) &event;

        /* make sure the attribute is in our signal array */

        if ((nvctrlevent->attribute >= 0) &&
            (nvctrlevent->attribute <= NV_CTRL_LAST_ATTRIBUTE) &&
            (signals[nvctrlevent->attribute] != 0)) {
            
            event_struct.attribute    = nvctrlevent->attribute;
            event_struct.value        = nvctrlevent->value;
            event_struct.display_mask = nvctrlevent->display_mask;
            event_struct.availability = TRUE;
            
            /*
             * XXX Is emitting a signal with g_signal_emit() really
             * the "correct" way of dispatching the event?
             */

            CTK_EVENT_BROADCAST(event_source,
                                signals[nvctrlevent->attribute],
                                &event_struct,
                                nvctrlevent->target_type,
                                nvctrlevent->target_id);
        }

        /*
         * Handle the TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT
         * NV-CONTROL event.
         */

    } else if (event_source->event_base != -1 &&
               (event.type == (event_source->event_base
                               + TARGET_ATTRIBUTE_AVAILABILITY_CHANGED_EVENT))) {

        XNVCtrlAttributeChangedEventTargetAvailability *nvctrlevent =
            (XNVCtrlAttributeChangedEventTargetAvailability *) &event;

        /* make sure the attribute is in our signal array */

        if ((nvctrlevent->attribute >= 0) &&
            (nvctrlevent->attribute <= NV_CTRL_LAST_ATTRIBUTE) &&
            (signals[nvctrlevent->attribute] != 0)) {
            
            event_struct.attribute    = nvctrlevent->attribute;
            event_struct.value        = nvctrlevent->value;
            event_struct.display_mask = nvctrlevent->display_mask;
            event_struct.availability = nvctrlevent->availability;
            
            /*
             * XXX Is emitting a signal with g_signal_emit() really
             * the "correct" way of dispatching the event?
             */

            CTK_EVENT_BROADCAST(event_source,
                                signals[nvctrlevent->attribute],
                                &event_struct,
                                nvctrlevent->target_type,
                                nvctrlevent->target_id);
        }
        /*
         * Handle the TARGET_STRING_ATTRIBUTE_CHANGED_EVENT
         * NV-CONTROL event.
         */
    } else if (event_source->event_base != -1 &&
               (event.type == (event_source->event_base
                               + TARGET_STRING_ATTRIBUTE_CHANGED_EVENT))) {
        XNVCtrlStringAttributeChangedEventTarget *nvctrlevent =
            (XNVCtrlStringAttributeChangedEventTarget *) &event;

        /* make sure the attribute is in our signal array */
        
        if ((nvctrlevent->attribute >= 0) &&
            (nvctrlevent->attribute <= NV_CTRL_STRING_LAST_ATTRIBUTE) &&
            (string_signals[nvctrlevent->attribute] != 0)) {

            event_struct.attribute    = nvctrlevent->attribute;
            event_struct.value        = 0;
            event_struct.display_mask = nvctrlevent->display_mask;
            event_struct.availability = TRUE;
            /*
             * XXX Is emitting a signal with g_signal_emit() really
             * the "correct" way of dispatching the event
             */

            CTK_EVENT_BROADCAST(event_source,
                                string_signals[nvctrlevent->attribute],
                                &event_struct,
                                nvctrlevent->target_type,
                                nvctrlevent->target_id);
        }
         /*
          * Handle the TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT
          * NV-CONTROL event.
          */
    } else if (event_source->event_base != -1 &&
               (event.type == (event_source->event_base
                               + TARGET_BINARY_ATTRIBUTE_CHANGED_EVENT))) {
        XNVCtrlBinaryAttributeChangedEventTarget *nvctrlevent =
            (XNVCtrlBinaryAttributeChangedEventTarget *) &event;

        /* make sure the attribute is in our signal array */
        if ((nvctrlevent->attribute >= 0) &&
            (nvctrlevent->attribute <= NV_CTRL_BINARY_DATA_LAST_ATTRIBUTE) &&
            (binary_signals[nvctrlevent->attribute] != 0)) {

            event_struct.attribute    = nvctrlevent->attribute;
            event_struct.value        = 0;
            event_struct.display_mask = nvctrlevent->display_mask;
            event_struct.availability = TRUE;
            /*
             * XXX Is emitting a signal with g_signal_emit() really
             * the "correct" way of dispatching the event
             */

            CTK_EVENT_BROADCAST(event_source,
                                binary_signals[nvctrlevent->attribute],
                                &event_struct,
                                nvctrlevent->target_type,
                                nvctrlevent->target_id);
        }


        /*
         * Also handle XRandR events.
         */

    } else if (event_source->randr_event_base != -1 &&
               (event.type ==
                (event_source->randr_event_base + RRScreenChangeNotify))) {
        
        XRRScreenChangeNotifyEvent *xrandrevent =
            (XRRScreenChangeNotifyEvent *)&event;
        int screen;
        
        /* Find the screen the window belongs to */
        screen = get_screen_of_root(xrandrevent->display, xrandrevent->root);
        if (screen >= 0) {
            CTK_EVENT_BROADCAST(event_source,
                                signal_RRScreenChangeNotify,
                                &event,
                                NV_CTRL_TARGET_TYPE_X_SCREEN,
                                screen);
        }

    /*
     * Trap events that get registered but are not handled
     * properly.
     */

    } else {
        nv_warning_msg("Unknown event type %d.", event.type);
    }
    
    return TRUE;

} /* ctk_event_dispatch() */



/* ctk_event_emit() - Emits signal(s) on a registered ctk_event object.
 * This function is primarly used to simulate NV-CONTROL events such
 * that various parts of nvidia-settings can communicate (internally)
 */
void ctk_event_emit(CtkEvent *ctk_event,
                    unsigned int mask, int attrib, int value)
{
    CtkEventStruct event;
    CtkEventSource *source;
    Display *dpy = NvCtrlGetDisplayPtr(ctk_event->handle);


    if (attrib > NV_CTRL_LAST_ATTRIBUTE) return;


    /* Find the event source */
    source = event_sources;
    while (source) {
        if (source->dpy == dpy) {
            break;
        }
        source = source->next;
    }
    if (!source) return;


    /* Broadcast event to all relavent ctk_event objects */
    event.attribute = attrib;
    event.value = value;
    event.display_mask = mask;

    CTK_EVENT_BROADCAST(source,
                        signals[attrib],
                        &event,
                        NvCtrlGetTargetType(ctk_event->handle),
                        NvCtrlGetTargetId(ctk_event->handle));

} /* ctk_event_emit() */



/* ctk_event_emit_string() - Emits signal(s) on a registered ctk_event object.
 * This function is primarly used to simulate NV-CONTROL events such
 * that various parts of nvidia-settings can communicate (internally)
 */
void ctk_event_emit_string(CtkEvent *ctk_event,
                    unsigned int mask, int attrib)
{
    CtkEventStruct event;
    CtkEventSource *source;
    Display *dpy = NvCtrlGetDisplayPtr(ctk_event->handle);


    if (attrib > NV_CTRL_STRING_LAST_ATTRIBUTE) return;


    /* Find the event source */
    source = event_sources;
    while (source) {
        if (source->dpy == dpy) {
            break;
        }
        source = source->next;
    }
    if (!source) return;


    /* Broadcast event to all relavent ctk_event objects */

    event.attribute = attrib;
    event.value = 0;
    event.display_mask = mask;

    CTK_EVENT_BROADCAST(source,
                        string_signals[attrib],
                        &event,
                        NvCtrlGetTargetType(ctk_event->handle),
                        NvCtrlGetTargetId(ctk_event->handle));

} /* ctk_event_emit_string() */

