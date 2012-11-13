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

#ifndef __CTK_DISPLAYLAYOUT_H__
#define __CTK_DISPLAYLAYOUT_H__

#include "ctkevent.h"
#include "ctkconfig.h"

#include "XF86Config-parser/xf86Parser.h"


G_BEGIN_DECLS

#define CTK_TYPE_DISPLAY_LAYOUT (ctk_display_layout_get_type())

#define CTK_DISPLAY_LAYOUT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DISPLAY_LAYOUT, \
                                 CtkDisplayLayout))

#define CTK_DISPLAY_LAYOUT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DISPLAY_LAYOUT, \
                              CtkDisplayLayoutClass))

#define CTK_IS_DISPLAY_LAYOUT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DISPLAY_LAYOUT))

#define CTK_IS_DISPLAY_LAYOUT_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DISPLAY_LAYOUT))

#define CTK_DISPLAY_LAYOUT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DISPLAY_LAYOUT, \
                                CtkDisplayLayoutClass))

#define CTK_DISPLAY_LAYOUT_TOOLTIP_WIDGET(obj) \
    ((CTK_SCALE(obj))->tooltip_widget)



/* Maximums */
#define MAX_DEVICES 8  /* Max number of GPUs */


/* Rectangle/Dim data positions */
#define LEFT         0
#define TOP          1
#define WIDTH        2
#define HEIGHT       3
#define X         LEFT
#define Y          TOP
#define W        WIDTH
#define H       HEIGHT


/* XF86VIDMODE */
#define V_PHSYNC        0x0001 
#define V_NHSYNC        0x0002
#define V_PVSYNC        0x0004
#define V_NVSYNC        0x0008
#define V_INTERLACE     0x0010 
#define V_DBLSCAN       0x0020
#define V_CSYNC         0x0040
#define V_PCSYNC        0x0080
#define V_NCSYNC        0x0100
#define V_HSKEW         0x0200  /* hskew provided */
#define V_BCAST         0x0400
#define V_CUSTOM        0x0800  /* timing numbers customized by editor */
#define V_VSCAN         0x1000


/* NV-CONTROL modeline sources */
#define MODELINE_SOURCE_XSERVER   0x001
#define MODELINE_SOURCE_XCONFIG   0x002
#define MODELINE_SOURCE_BUILTIN   0x004
#define MODELINE_SOURCE_VESA      0x008
#define MODELINE_SOURCE_EDID      0x010
#define MODELINE_SOURCE_NVCONTROL 0x020

#define MODELINE_SOURCE_USER  \
  ((MODELINE_SOURCE_XCONFIG)|(MODELINE_SOURCE_NVCONTROL))




/*** M A C R O S *************************************************************/


#define NV_MIN(A, B) ((A)<(B)?(A):(B))
#define NV_MAX(A, B) ((A)>(B)?(A):(B))


/* Determines if the mode is the nvidia-auto-select mode. */
#define IS_NVIDIA_DEFAULT_MODE(m)                        \
(!strcmp(( m )->data.identifier, "nvidia-auto-select"))


/* Calculates the horizontal refresh rate (sync) of the modeline in kHz */
#define GET_MODELINE_HSYNC(m)                                        \
(((double)((m)->data.clock)) / (2.0f * (double)((m)->data.htotal)))

/* Determines if the metamode was created/modified by the user */
#define IS_METAMODE_SOURCE_USER(s)      \
(((s) == METAMODE_SOURCE_XCONFIG) ||    \
 ((s) == METAMODE_SOURCE_NVCONTROL) ||  \
 ((s) == METAMODE_SOURCE_RANDR))



/*** T Y P E   D E F I N I T I O N S *****************************************/

typedef enum {
    PASSIVE_STEREO_EYE_NONE = 0,
    PASSIVE_STEREO_EYE_LEFT,
    PASSIVE_STEREO_EYE_RIGHT,
} PassiveStereoEye;

typedef enum {
    ROTATION_0 = 0,
    ROTATION_90,
    ROTATION_180,
    ROTATION_270,
} Rotation;

typedef enum {
    REFLECTION_NONE = 0,
    REFLECTION_X,
    REFLECTION_Y,
    REFLECTION_XY,
} Reflection;

typedef enum {
    METAMODE_SOURCE_XCONFIG = 0,
    METAMODE_SOURCE_IMPLICIT,
    METAMODE_SOURCE_NVCONTROL,
    METAMODE_SOURCE_RANDR,
} MetaModeSource;

typedef struct nvModeLineRec {
    struct nvModeLineRec *next;

    XConfigModeLineRec data; /* Modeline information */

    double refresh_rate; /* in Hz */

    /* Extra information */
    unsigned int source;
    char *xconfig_name;

} nvModeLine, *nvModeLinePtr;



/* Mode (A particular configuration for a display within an X screen)
 *
 * NOTE: When metamodes are duplicated, the modes are memcpy'ed over, so
 *       if new variables are added to the nvModeRec that shouldn't (just)
 *       be copied, be sure to update this in
 *       ctk_display_layout_add_screen_metamode().
 */
typedef struct nvModeRec {
    struct nvModeRec *next;

    /* Defines a single mode for a dispay device as part of an X screen's
     * metamode.
     *
     * "WxH_Hz +x+y @WxH"
     *
     * "modeline_reference_name  +offset @panning"
     */

    struct nvDisplayRec *display;       /* Display device mode belongs to */
    struct nvMetaModeRec *metamode;     /* Metamode the mode is in */
    struct nvModeLineRec *modeline;     /* Modeline this mode references */
    int dummy;                          /* Dummy mode, don't print out */

    int viewPortIn[4];                  /* Viewport In (absolute) */
    int pan[4];                         /* Panning Domain (absolute) */

    int viewPortOut[4];                 /* ViewPort Out (WH) */

    int position_type;                  /* Relative, Absolute, etc. */
    struct nvDisplayRec *relative_to;   /* Display Relative/RightOf etc */

    PassiveStereoEye passive_stereo_eye; /* Stereo mode 4 per-dpy setting */


    Rotation rotation;
    Reflection reflection;

} nvMode, *nvModePtr;



/* Display Device (CRT, DFP, TV, Projector ...) */
typedef struct nvDisplayRec {
    struct nvDisplayRec *next_on_gpu;
    struct nvDisplayRec *next_in_screen;
    XConfigMonitorPtr    conf_monitor;

    NvCtrlAttributeHandle *handle;      /* NV-CONTROL handle to device */

    struct nvGpuRec    *gpu;            /* GPU the display belongs to */
    struct nvScreenRec *screen;         /* X screen the display is tied to */

    char               *logName;        /* Display name (from NV-CONTROL) */
    char               *typeBaseName;   /* e.g. "CRT", "DFP", "TV" */
    char               *typeIdName;     /* e.g. "DFP-1", "TV-0" */
    char               *dpGuidName;     /* e.g. "DP-GUID-11111111-1111-1111-1111-111111111111" */
    char               *edidHashName;   /* e.g. "DPY-EDID-11111111-1111-1111-1111-111111111111" */
    char               *targetIdName;   /* e.g. "DPY-3" */
    char               *randrName;      /* e.g. "VGA-1", "DVI-I-2" */

    Bool                is_sdi;         /* Is an SDI display */

    nvModeLinePtr       modelines;      /* Modelines validated by X */
    int                 num_modelines;

    nvModePtr           modes;          /* List of modes this display uses */
    int                 num_modes;
    nvModePtr           cur_mode;       /* Current mode display uses */

} nvDisplay, *nvDisplayPtr;



/* MetaMode (A particular configuration for an X screen) */
typedef struct nvMetaModeRec {
    struct nvMetaModeRec *next;

    int id;     /* Magic id */
    MetaModeSource source; /* Source of the metamode */
    Bool switchable; /* Can the metamode be accessed through Ctrl Alt +- */

    // Used for drawing & moving metamode boxes
    int dim[4]; /* Bounding box of all modes */

    // Used for applying and generating metamodes (effective dimensions)
    int edim[4]; /* Bounding box of all non-NULL modes */

    char *string; /* Temp string used for modifying the metamode list */

} nvMetaMode, *nvMetaModePtr;



/* X Screen */
typedef struct nvScreenRec {
    struct nvScreenRec *next_in_layout;

    XConfigScreenPtr conf_screen;
    XConfigDevicePtr conf_device;

    /* An X screen may have one or more displays connected to it
     * if TwinView is on.
     *
     * If NoScanout is enabled, the X screen will not make use
     * of display device(s).
     *
     */

    NvCtrlAttributeHandle *handle;  /* NV-CONTROL handle to X screen */
    CtkEvent *ctk_event;
    int scrnum;

    struct nvLayoutRec *layout; /* Layout this X screen belongs to */
    struct nvGpuRec *gpu;  /* GPU driving this X screen */

    int depth;      /* Depth of the screen */
    int stereo;     /* Stereo mode enabled on this screen */

    nvDisplayPtr displays; /* List of displays using this screen */
    int num_displays; /* # of displays using this screen */

    nvMetaModePtr metamodes;     /* List of metamodes */
    int num_metamodes;           /* # modes per display device */
    nvMetaModePtr cur_metamode;  /* Current metamode to display */
    int cur_metamode_idx;        /* Current metamode to display */
    nvDisplayPtr primaryDisplay;
    // Used for generating metamode strings.
    int dim[4]; /* Bounding box of all metamodes (Absolute coords) */

    int position_type;                /* Relative, Absolute, etc. */
    struct nvScreenRec *relative_to;  /* Screen Relative/RightOf etc */
    int x_offset;                     /* Offsets for relative positioning */
    int y_offset;

    Bool sli;
    Bool dynamic_twinview;  /* This screen supports dynamic twinview */
    Bool no_scanout;        /* This screen has no display devices */
    Bool stereo_supported;  /* Can stereo be configured on this screen */

} nvScreen, *nvScreenPtr;



/* GVO Mode information */
typedef struct GvoModeDataRec {
    unsigned int id; /* NV-CONTROL ID */
    char *name;
    unsigned int rate; /* Refresh rate */
} GvoModeData;



/* GPU (Device) */
typedef struct nvGpuRec {
    struct nvGpuRec *next_in_layout; /* List of all GPUs */

    NvCtrlAttributeHandle *handle;  /* NV-CONTROL handle to GPU */
    CtkEvent *ctk_event;

    struct nvLayoutRec *layout; /* Layout this GPU belongs to */

    int max_width;
    int max_height;
    int max_displays;
    Bool allow_depth_30;

    char *name;  /* Name of the GPU */

    gchar *pci_bus_id;

    GvoModeData *gvo_mode_data; /* Information about GVO modes available */
    unsigned int num_gvo_modes;

    nvDisplayPtr displays; /* Linked list of displays connected to GPU */
    int num_displays;

} nvGpu, *nvGpuPtr;



/* Layout */
typedef struct nvLayoutRec {
    XConfigLayoutPtr conf_layout;
    char *filename;

    NvCtrlAttributeHandle *handle;

    nvGpuPtr gpus;  /* Linked list of GPUs (next_in_layout) */
    int num_gpus;

    nvScreenPtr screens; /* Linked list of X screens (next_in_layout) */
    int num_screens;

    /* Used for drawing the layout */
    int dim[4]; /* Bounding box of All X screens (Absolute coords) */

    int xinerama_enabled;

} nvLayout, *nvLayoutPtr;



typedef void (* ctk_display_layout_selected_callback) (nvLayoutPtr, void *);
typedef void (* ctk_display_layout_modified_callback) (nvLayoutPtr, void *);



/* Stores information about a screen or display  that
 * is being moved/panned.
 */
typedef struct ModifyInfoRec {

    /* What to move */
    nvDisplayPtr display;
    nvScreenPtr screen;
    int orig_screen_dim[4]; // Used when moding display = moding screen.
    nvGpuPtr gpu;

    int orig_position_type; // Original values of what is being
    int orig_dim[4];        // modified.

    int *target_position_type; // Pointers to values of thing that
    int *target_dim;           // is being modified.


    /* Snapping */
    int snap;         // Should we snap or not?
    int best_snap_v;
    int best_snap_h;

    int modify_dirty;   // Sync the modify_dim before moving/panning.
    int modify_panning; // Modifying panning (instead of position)
    int modify_dim[4];  // Dimensions to snap from

    int src_dim[4]; // Pre-snap (To allow snapping of pan box on move)
    int dst_dim[4]; // Post-snap position

} ModifyInfo;



// Something selectable/visible.
typedef struct _ZNode
{
    enum {
        ZNODE_TYPE_DISPLAY,
        ZNODE_TYPE_SCREEN,
    } type;

    union {
        nvDisplayPtr display;
        nvScreenPtr screen;
    } u;

} ZNode;


typedef struct _CtkDisplayLayout
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig             *ctk_config;

    GtkWidget   *drawing_area;   /* Drawing area */
    GtkWidget   *tooltip_area;   /* Tooltip area */
    GtkTooltips *tooltip_group;  /* Tooltip group */

    /* Layout configuration */
    nvLayoutPtr layout;

    /* Double buffering of layout image */
    GdkPixmap *pixmap;

    /* Image information */
    int        img_dim[4];      /* Dimensions used to draw in */
    float      scale;

    /* Colors */
    GdkColor  *color_palettes;  /* Colors to use to display screens */
    GdkColor   fg_color;
    GdkColor   bg_color;
    GdkColor   select_color;

    /* Pango layout for strings in layout image */

    PangoLayout *pango_layout;

    /* List of visible elements in the layout */
    ZNode *Zorder; /* Z ordering of visible elements in layout */
    int    Zcount; /* Count of visible elements in the z order */

    nvDisplayPtr  selected_display; /* Currently selected display */
    nvScreenPtr   selected_screen;  /* Selected screen */

    /* Settings */
    int        snap_strength;
    int        advanced_mode;   /* Allow advanced layout modifications: */
                                /* - panning */
                                /* - multiple modes */

    /* State */
    void      *first_selected_display; /* First thing clicked on (used to cycle) */
    void      *first_selected_screen;
    int        clicked_outside; /* User clicked outside displays, don't move */
    ModifyInfo modify_info;     /* Used to move/pan screens/displays */

    int        button1;
    int        button2;
    int        button3;
    int        mouse_x;
    int        mouse_y;
    int        last_mouse_x;
    int        last_mouse_y;

    ctk_display_layout_selected_callback  selected_callback;
    void                                 *selected_callback_data;
    ctk_display_layout_modified_callback  modified_callback;
    void                                 *modified_callback_data;

} CtkDisplayLayout;


typedef struct _CtkDisplayLayoutClass
{
    GtkVBoxClass parent_class;
} CtkDisplayLayoutClass;



GType       ctk_display_layout_get_type  (void) G_GNUC_CONST;

GtkWidget*  ctk_display_layout_new       (NvCtrlAttributeHandle *,
                                          CtkConfig *,
                                          nvLayoutPtr, /* Layout to display */
                                          int,         /* Width of image */
                                          int          /* Height of image */
                                          );

void ctk_display_layout_update (CtkDisplayLayout *);

void ctk_display_layout_set_layout (CtkDisplayLayout *, nvLayoutPtr);

void ctk_display_layout_update_zorder(CtkDisplayLayout *ctk_object);

nvDisplayPtr ctk_display_layout_get_selected_display (CtkDisplayLayout *);
nvScreenPtr ctk_display_layout_get_selected_screen (CtkDisplayLayout *);
nvGpuPtr ctk_display_layout_get_selected_gpu (CtkDisplayLayout *);


void ctk_display_layout_set_mode_modeline (CtkDisplayLayout *,
                                           nvModePtr mode,
                                           nvModeLinePtr modeline);

void ctk_display_layout_set_mode_viewport_in(CtkDisplayLayout *ctk_object,
                                             nvModePtr mode,
                                             int w, int h);

void ctk_display_layout_set_mode_viewport_out(CtkDisplayLayout *ctk_object,
                                              nvModePtr mode,
                                              int x, int y, int w, int h);

void ctk_display_layout_set_display_position (CtkDisplayLayout *ctk_object,
                                              nvDisplayPtr display,
                                              int position_type,
                                              nvDisplayPtr relative_to,
                                              int x, int y);
void ctk_display_layout_set_display_panning (CtkDisplayLayout *ctk_object,
                                             nvDisplayPtr display,
                                             int width, int height);

void ctk_display_layout_set_display_rotation (CtkDisplayLayout *ctk_object,
                                              nvDisplayPtr display,
                                              Rotation rotation);

void ctk_display_layout_set_display_reflection (CtkDisplayLayout *ctk_object,
                                                nvDisplayPtr display,
                                                Reflection reflection);

void ctk_display_layout_select_display (CtkDisplayLayout *ctk_object,
                                        nvDisplayPtr display);
void ctk_display_layout_select_screen (CtkDisplayLayout *ctk_object,
                                       nvScreenPtr screen);
void ctk_display_layout_update_display_count (CtkDisplayLayout *,
                                              nvDisplayPtr);

void ctk_display_layout_set_screen_virtual_size (CtkDisplayLayout *ctk_object,
                                                 nvScreenPtr screen,
                                                 int width, int height);
void ctk_display_layout_set_screen_depth (CtkDisplayLayout *ctk_object,
                                          nvScreenPtr screen, int depth);
void ctk_display_layout_set_screen_position (CtkDisplayLayout *ctk_object,
                                             nvScreenPtr screen,
                                             int position_type,
                                             nvScreenPtr relative_to,
                                             int x, int y);
void ctk_display_layout_set_screen_metamode (CtkDisplayLayout *,
                                             nvScreenPtr screen,
                                             int new_mode);
void ctk_display_layout_add_screen_metamode (CtkDisplayLayout *, nvScreenPtr);
void ctk_display_layout_delete_screen_metamode (CtkDisplayLayout *,
                                                nvScreenPtr,
                                                int metamode_idx,
                                                Bool reselect);

void ctk_display_layout_disable_display (CtkDisplayLayout *ctk_object,
                                         nvDisplayPtr display);

void ctk_display_layout_set_advanced_mode (CtkDisplayLayout *ctk_object,
                                           int advanced_mode);

void ctk_display_layout_register_callbacks(CtkDisplayLayout *ctk_object,
                                           ctk_display_layout_selected_callback,
                                           void *selected_callback_data,
                                           ctk_display_layout_modified_callback,
                                           void *modified_callback_data);


G_END_DECLS

#endif /* __CTK_DISPLAYLAYOUT_H__ */
