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

#include <stdlib.h> /* malloc */
#include <string.h> /* strlen */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkdisplaylayout.h"
#include "ctkdisplayconfig-utils.h"




/* GUI look and feel */

#define DEFAULT_SNAP_STRENGTH 100

#define MAX_LAYOUT_WIDTH   0x00007FFF /* 16 bit signed int (32767) */
#define MAX_LAYOUT_HEIGHT  0x00007FFF

#define LAYOUT_IMG_OFFSET           2 /* Border + White trimming */
#define LAYOUT_IMG_BORDER_PADDING   8

#define LAYOUT_IMG_FG_COLOR         "black"
#define LAYOUT_IMG_BG_COLOR         "#AAAAAA"
#define LAYOUT_IMG_SELECT_COLOR     "#FF8888"


/* Device (GPU) Coloring */

#define BG_SCR_ON              0 /* Screen viewing area (Has modeline set) */
#define BG_PAN_ON              1 /* Screen panning area (Has modeline set) */
#define BG_SCR_OFF             2 /* Screen viewing area (Off/Disabled) */
#define BG_PAN_OFF             3 /* Screen panning area (Off/Disabled) */

#define NUM_COLOR_PALETTES     MAX_DEVICES /* One palette for each possible Device/GPU */
#define NUM_COLORS_PER_PALETTE 4 /* Number of colors in a device's palette */
#define NUM_COLORS ((NUM_COLOR_PALETTES) * (NUM_COLORS_PER_PALETTE))

#if MAX_DEVICES != 8
#warning "Each GPU needs a color palette!"
#endif

/* Each device will need a unique color palette */
char *__palettes_color_names[NUM_COLORS] = {

    /* Blue */
    "#D9DBF4", /* View    - Has modeline set */
    "#C9CBE4", /* Panning - Has modeline set */
    "#BABCD5", /* View    - Off/Disabled */
    "#A3A5BE", /* Panning - OFf/Disabled */

    /* Orange */
    "#FFDB94",
    "#E8C47D",
    "#C9A55E",
    "#A6823B",

    /* Purple */
    "#E2D4F0",
    "#CFC1DD",
    "#B7A9C5",
    "#9D8FAB",

    /* Beige */
    "#EAF1C9",
    "#CBD2AA",
    "#ADB48C",
    "#838A62",

    /* Green */
    "#96E562",
    "#70BF3C",
    "#5BAA27",
    "#3C8B08",

    /* Pink */
    "#FFD6E9",
    "#E1B8CB",
    "#C79EB1",
    "#A87F92",

    /* Yellow */
    "#EEEE7E",
    "#E0E070",
    "#D5D565",
    "#C4C454",

    /* Teal */
    "#C9EAF1",
    "#A2C3CA",
    "#8DAEB5",
    "#76979E"
    };




/*** P R O T O T Y P E S *****************************************************/


static gboolean expose_event_callback (GtkWidget *widget,
                                       GdkEventExpose *event,
                                       gpointer data);

static gboolean configure_event_callback (GtkWidget *widget,
                                          GdkEventConfigure *event,
                                          gpointer data);

static gboolean motion_event_callback (GtkWidget *widget,
                                       GdkEventMotion *event,
                                       gpointer data);

static gboolean button_press_event_callback (GtkWidget *widget,
                                             GdkEventButton *event,
                                             gpointer data);

static gboolean button_release_event_callback (GtkWidget *widget,
                                               GdkEventButton *event,
                                               gpointer data);


static void calc_metamode(nvScreenPtr screen, nvMetaModePtr metamode);




/*** F U N C T I O N S *******************************************************/


/** queue_layout_redraw() ********************************************
 *
 * Queues an expose event to happen on ourselves so we know to
 * redraw later.
 *
 **/

static void queue_layout_redraw(CtkDisplayLayout *ctk_object)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GtkAllocation *allocation = &(drawing_area->allocation);
    GdkRectangle rect;


    if (!drawing_area->window) {
        return;
    }

    /* Queue an expose event */
    rect.x = allocation->x;
    rect.y = allocation->x;
    rect.width = allocation->width;
    rect.height = allocation->height;

    gdk_window_invalidate_rect(drawing_area->window, &rect, TRUE);

} /* queue_layout_redraw() */



/** zorder_layout() **************************************************
 *
 * In order to draw and allow selecting display devices, we need to
 * keep them in a Z-ordered list.  This function creates the initial
 * Z-order list for the given layout based on the devices it has.
 *
 **/

static void zorder_layout(CtkDisplayLayout *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvDisplayPtr display;
    int z;


    /* Clean up */
    if (ctk_object->Zorder) {
        free(ctk_object->Zorder);
        ctk_object->Zorder = NULL;
    }
    ctk_object->Zcount = 0;
    ctk_object->selected_display = NULL;
    ctk_object->selected_screen = NULL;


    /* Count the number of Z-orderable elements in the layout */
    ctk_object->Zcount = layout->num_screens;
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        ctk_object->Zcount += gpu->num_displays;
    }


    /* If there is nothing Z-orderable, we're done */
    if (!ctk_object->Zcount) {
        return;
    }


    /* Create the Z-order buffer */
    ctk_object->Zorder = calloc(ctk_object->Zcount, sizeof(ZNode));
    if (!ctk_object->Zorder) {
        ctk_object->Zcount = 0;
        return;
    }


    /* Populate the Z-order list */
    z = 0;

    /* Add screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        /* Add displays that belong to the screen */
        for (display = screen->displays;
             display;
             display = display->next_in_screen) {
            ctk_object->Zorder[z].type = ZNODE_TYPE_DISPLAY;
            ctk_object->Zorder[z].u.display = display;
            z++;
        }

        /* Add the screen */
        ctk_object->Zorder[z].type = ZNODE_TYPE_SCREEN;
        ctk_object->Zorder[z].u.screen = screen;
        z++;
    }

    /* Add displays that don't have screens */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->screen) continue;
            ctk_object->Zorder[z].type = ZNODE_TYPE_DISPLAY;
            ctk_object->Zorder[z].u.display = display;
            z++;
        }
    }

} /* zorder_layout() */



/** get_metamode() ***************************************************
 *
 * Returns a screen's metamode_idx'th metamode, clamping to the last
 * available metamode in the list.
 *
 **/

static nvMetaModePtr get_metamode(nvScreenPtr screen, int metamode_idx)
{
    nvMetaModePtr metamode = screen->metamodes;

    while (metamode && metamode->next && metamode_idx) {
        metamode = metamode->next;
        metamode_idx--;
    }

    return metamode;

} /* get_metamode() */



/** get_mode() *******************************************************
 *
 * Returns a display device's mode_idx'th mode.
 *
 **/

static nvModePtr get_mode(nvDisplayPtr display, int mode_idx)
{
    nvModePtr mode = display->modes;

    while (mode && mode->next && mode_idx) {
        mode = mode->next;
        mode_idx--;
    }

    return mode;

} /* get_mode() */



/** get_screen_dim ***************************************************
 *
 * Returns the dimension array to use as the screen's dimensions.
 *
 **/

static int *get_screen_dim(nvScreenPtr screen, Bool edim)
{
    if (!screen) return NULL;

    if (screen->no_scanout || !screen->cur_metamode) {
        return screen->dim;
    }

    return edim ? screen->cur_metamode->edim : screen->cur_metamode->dim;

} /* get_screen_dim() */



/** get_modify_info() ************************************************
 *
 * Gather information prior to moving/panning.
 *
 * Returns TRUE if something is selected and movable.
 *
 **/

static Bool get_modify_info(CtkDisplayLayout *ctk_object)
{
    ModifyInfo *info = &(ctk_object->modify_info);
    Bool use_screen_instead;
    int *sdim;


    info->screen = ctk_object->selected_screen;
    info->display = ctk_object->selected_display;


    /* There must be an associated screen to move */
    if (!info->screen) {
        info->display = NULL;
        return FALSE;
    }

    /* Don't allow modifying displays without modes */
    if (info->display && !info->display->cur_mode) {
        info->screen = NULL;
        info->display = NULL;
        return FALSE;
    }


    /* Gather the initial screen dimensions */
    sdim = get_screen_dim(info->screen, 0);
    info->orig_screen_dim[X] = sdim[X];
    info->orig_screen_dim[Y] = sdim[Y];
    info->orig_screen_dim[W] = sdim[W];
    info->orig_screen_dim[H] = sdim[H];


    /* If a display device is being moved (not panned) and
     * it is the only display device in the screen to use
     * absolute positioning, then really we want to move
     * its associated screen.
     */
    if (!ctk_object->modify_info.modify_panning &&
        info->display &&
        info->display->cur_mode->position_type == CONF_ADJ_ABSOLUTE) {

        nvDisplayPtr display;

        /* Make sure all other displays in the screen use
         * relative positioning.
         */
        use_screen_instead = TRUE;
        for (display = info->display->screen->displays;
             display;
             display = display->next_in_screen) {
            if (display == info->display) continue;
            if (!display->cur_mode) continue;
            if (display->cur_mode->position_type == CONF_ADJ_ABSOLUTE) {
                use_screen_instead = FALSE;
            }
        }
        if (use_screen_instead) {
            info->display = NULL;
        }
    }


    /* Gather the initial state of what is being moved */
    if (info->display) {
        info->target_position_type =
            &(info->display->cur_mode->position_type);
        if (ctk_object->modify_info.modify_panning) {
            info->target_dim = info->display->cur_mode->pan;
        } else {
            info->target_dim = info->display->cur_mode->dim;
        }
        info->gpu = info->display->gpu;
    } else {
        info->target_position_type = &(info->screen->position_type);
        info->target_dim = sdim;
        info->gpu = info->screen->gpu;
    }
    info->orig_position_type = *(info->target_position_type);
    info->orig_dim[X] = info->target_dim[X];
    info->orig_dim[Y] = info->target_dim[Y];
    info->orig_dim[W] = info->target_dim[W];
    info->orig_dim[H] = info->target_dim[H];


    /* Initialize where we moved to */
    info->dst_dim[X] = info->orig_dim[X];
    info->dst_dim[Y] = info->orig_dim[Y];
    info->dst_dim[W] = info->orig_dim[W];
    info->dst_dim[H] = info->orig_dim[H];


    /* Initialize snapping */
    info->best_snap_v = ctk_object->snap_strength +1;
    info->best_snap_h = ctk_object->snap_strength +1;


    /* Make sure the modify dim is up to date */
    if (info->modify_dirty) {
        info->modify_dim[X] = info->orig_dim[X];
        info->modify_dim[Y] = info->orig_dim[Y];
        info->modify_dim[W] = info->orig_dim[W];
        info->modify_dim[H] = info->orig_dim[H];
        info->modify_dirty = 0;
    }

    return TRUE;

} /* get_modify_info() */



/** sync_scaling() ***************************************************
 *
 * Computes the scaling required to display the layout image.
 *
 **/

static void sync_scaling(CtkDisplayLayout *ctk_object)
{
    int *dim = ctk_object->layout->dim;
    float wscale;
    float hscale;

    wscale = (float)(ctk_object->img_dim[W]) / (float)(dim[W]);
    hscale = (float)(ctk_object->img_dim[H]) / (float)(dim[H]);

    if (wscale * dim[H] > ctk_object->img_dim[H]) {
        ctk_object->scale = hscale;
    } else {
        ctk_object->scale = wscale;
    }

} /* sync_scaling() */



/** point_in_dim() ***************************************************
 *
 * Determines if a point lies within the given dimensions
 *
 **/

static int point_in_dim(int *dim, int x, int y)
{
    if (x > dim[X] && x < (dim[X] + dim[W]) &&
        y > dim[Y] && y < (dim[Y] + dim[H])) {
        return 1;
    }
    
    return 0;

} /*  point_in_dim() */



/** get_point_relative_position() ************************************
 *
 * Determines the relative position of a point to the given dimensions
 * of a box.
 *
 **/

static int get_point_relative_position(int *dim, int x, int y)
{
    float m1, b1;
    float m2, b2;
    float l1, l2;


    /* Point insize dim */
    if ((x >= dim[X]) && (x <= dim[X] + dim[W]) &&
        (y >= dim[Y]) && (y <= dim[Y] + dim[H])) {
        return CONF_ADJ_RELATIVE;
    }
    
    /* Compute cross lines of dimensions */
    m1 = ((float) dim[H]) / ((float) dim[W]);
    b1 = ((float) dim[Y]) - (m1 * ((float) dim[X]));
    
    m2 = -m1;
    b2 = ((float) dim[Y]) + ((float) dim[H]) - (m2 * ((float) dim[X]));
    
    /* Compute where point is relative to cross lines */
    l1 = m1 * ((float) x) + b1 - ((float) y);
    l2 = m2 * ((float) x) + b2 - ((float) y);
    
    if (l1 > 0.0f) {
        if (l2 > 0.0f) { 
            return CONF_ADJ_ABOVE;
        } else {
            return CONF_ADJ_RIGHTOF;
        }
    } else {
        if (l2 > 0.0f) { 
            return CONF_ADJ_LEFTOF;
        } else {
            return CONF_ADJ_BELOW;
        }
    }

} /* get_point_relative_position() */



/** offset functions *************************************************
 *
 * Offsetting functions
 *
 * These functions do the dirty work of actually moving display
 * devices around in the layout.
 *
 **/

/* Offset a single mode */
static void offset_mode(nvModePtr mode, int x, int y)
{
    mode->dim[X] += x;
    mode->dim[Y] += y;
    mode->pan[X] = mode->dim[X];
    mode->pan[Y] = mode->dim[Y];
}

/* Offset a display by offsetting the current mode */
static void offset_display(nvDisplayPtr display, int x, int y)
{
    nvModePtr mode;
    for (mode = display->modes; mode; mode = mode->next) {
        offset_mode(mode, x, y);
    }
}

/* Offsets an X screen */
static void offset_screen(nvScreenPtr screen, int x, int y)
{
    nvMetaModePtr metamode;

    screen->dim[X] += x;
    screen->dim[Y] += y;
    
    for (metamode = screen->metamodes; metamode; metamode = metamode->next) {
        metamode->dim[X] += x;
        metamode->dim[Y] += y;
        metamode->edim[X] += x;
        metamode->edim[Y] += y;
    }
}

/* Offsets the entire layout by offsetting its X screens and display devices */
static void offset_layout(nvLayoutPtr layout, int x, int y)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvDisplayPtr display;

    layout->dim[X] += x;
    layout->dim[Y] += y;

    /* Offset screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        offset_screen(screen, x, y);
    }

    /* Offset displays */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            offset_display(display, x, y);
        }
    }

} /* offset functions */



/** resolve_display() ************************************************
 *
 * Figures out where the current mode of the given display should be
 * placed in relation to the layout.
 *
 * XXX This function assumes there are no relationship loops
 *
 **/

static int resolve_display(nvDisplayPtr display, int mode_idx,
                           int pos[4])
{
    nvModePtr mode = get_mode(display, mode_idx);
    int relative_pos[4];
    
    if (!mode) return 0;


    /* Set the dimensions */
    pos[W] = mode->pan[W];
    pos[H] = mode->pan[H];


    /* Find the position */
    switch (mode->position_type) {
    case CONF_ADJ_ABSOLUTE:
        pos[X] = mode->pan[X];
        pos[Y] = mode->pan[Y];
        break;

    case CONF_ADJ_RIGHTOF:
        resolve_display(mode->relative_to, mode_idx, relative_pos);
        pos[X] = relative_pos[X] + relative_pos[W];
        pos[Y] = relative_pos[Y];
        break;

    case CONF_ADJ_LEFTOF:
        resolve_display(mode->relative_to, mode_idx, relative_pos);
        pos[X] = relative_pos[X] - pos[W];
        pos[Y] = relative_pos[Y];
        break;

    case CONF_ADJ_BELOW:
        resolve_display(mode->relative_to, mode_idx, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y] + relative_pos[H];
        break;

    case CONF_ADJ_ABOVE:
        resolve_display(mode->relative_to, mode_idx, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y] - pos[H];
        break;

    case CONF_ADJ_RELATIVE: /* Clone */
        resolve_display(mode->relative_to, mode_idx, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y];
        break;

    default:
        return 0;
    }
    
    return 1;

} /* resolve_display() */



/** resolve_displays_in_screen() *************************************
 *
 * Resolves relative display positions into absolute positions for
 * the currently selected metamode of the screen.
 *
 **/

static void resolve_displays_in_screen(nvScreenPtr screen,
                                       int resolve_all_modes)
{
    nvDisplayPtr display;
    int pos[4];
    int first_idx;
    int last_idx;
    int mode_idx;

    if (resolve_all_modes) {
        first_idx = 0;
        last_idx = screen->num_metamodes -1;
    } else {
        first_idx = screen->cur_metamode_idx;
        last_idx = first_idx;
    }

    /* Resolve the current mode of each display in the screen */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        for (mode_idx = first_idx; mode_idx <= last_idx; mode_idx++) {
            if (resolve_display(display, mode_idx, pos)) {
                nvModePtr mode = get_mode(display, mode_idx);
                mode->dim[X] = pos[X];
                mode->dim[Y] = pos[Y];
                mode->pan[X] = pos[X];
                mode->pan[Y] = pos[Y];
            }
        }
    }

    /* Get the new position of the metamode(s) */
    for (mode_idx = first_idx; mode_idx <= last_idx; mode_idx++) {
        calc_metamode(screen, get_metamode(screen, mode_idx));
    }

} /* resolve_displays_in_screen() */



/** resolve_screen() *************************************************
 *
 * Figures out where the current metamode of the given screen should be
 * placed in relation to the layout.
 *
 * XXX This function assumes there are no relationship loops
 *
 **/

static int resolve_screen(nvScreenPtr screen, int pos[4])
{
    int *sdim = get_screen_dim(screen, 0);
    int relative_pos[4];
    

    if (!sdim) return 0;


    /* Set the dimensions */
    pos[W] = sdim[W];
    pos[H] = sdim[H];


    /* Find the position */
    switch (screen->position_type) {
    case CONF_ADJ_ABSOLUTE:
        pos[X] = sdim[X];
        pos[Y] = sdim[Y];
        break;

    case CONF_ADJ_RIGHTOF:
        resolve_screen(screen->relative_to, relative_pos);
        pos[X] = relative_pos[X] + relative_pos[W];
        pos[Y] = relative_pos[Y];
        break;

    case CONF_ADJ_LEFTOF:
        resolve_screen(screen->relative_to, relative_pos);
        pos[X] = relative_pos[X] - pos[W];
        pos[Y] = relative_pos[Y];
        break;

    case CONF_ADJ_BELOW:
        resolve_screen(screen->relative_to, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y] + relative_pos[H];
        break;

    case CONF_ADJ_ABOVE:
        resolve_screen(screen->relative_to, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y] - pos[H];
        break;

    case CONF_ADJ_RELATIVE: /* Clone */
        resolve_screen(screen->relative_to, relative_pos);
        pos[X] = relative_pos[X];
        pos[Y] = relative_pos[Y];
        break;

    default:
        return 0;
    }
    
    return 1;

} /* resolve_screen() */



/* resolve_screen_in_layout() ***************************************
 *
 * Resolves relative screen positions into absolute positions for
 * the currently selected metamode of the screen.
 *
 **/

static void resolve_screen_in_layout(nvScreenPtr screen)
{
    nvDisplayPtr display;
    int pos[4];
    int x, y;
    int *sdim;


    /* Resolve the current screen location */
    if (resolve_screen(screen, pos)) {

        /* Move the screen and the displays by offsetting */
        sdim = get_screen_dim(screen, 0);

        x = pos[X] - sdim[X];
        y = pos[Y] - sdim[Y];

        offset_screen(screen, x, y);

        for (display = screen->displays;
             display;
             display = display->next_in_screen) {
            offset_mode(display->cur_mode, x, y);
        }
    }

} /* resolve_screen_in_layout() */



/** resolve_layout() *************************************************
 *
 * Resolves relative positions into absolute positions for the
 * the *current* layout.
 *
 **/

static void resolve_layout(nvLayoutPtr layout)
{
    nvScreenPtr screen;

    /* First, resolve TwinView relationships */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        resolve_displays_in_screen(screen, 0);
    }

    /* Next, resolve X screen relationships */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        resolve_screen_in_layout(screen);
    }

} /* resolve_layout() */



/** calc_metamode() **************************************************
 *
 * Calculates the dimensions of a metamode.
 *
 * - Calculates the smallest bounding box that can hold the given
 *   metamode of the X screen.
 *
 **/

static void calc_metamode(nvScreenPtr screen, nvMetaModePtr metamode)
{
    nvDisplayPtr display;
    nvModePtr mode;
    int init = 1;
    int einit = 1;
    int *dim;  // Bounding box for all modes, including NULL modes.
    int *edim; // Bounding box for non-NULL modes.


    if (!screen || !metamode) {
        return;
    }

    dim = metamode->dim;
    edim = metamode->edim;

    dim[X] = edim[X] = 0;
    dim[Y] = edim[Y] = 0;
    dim[W] = edim[W] = 0;
    dim[H] = edim[H] = 0;

    /* Calculate its dimensions */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        /* Get the display's mode that is part of the metamode. */
        for (mode = display->modes; mode; mode = mode->next) {
            if (mode->metamode == metamode) break;
        }
        if (!mode) continue;
        
        if (init) {
            dim[X] = mode->pan[X];
            dim[Y] = mode->pan[Y];
            dim[W] = mode->pan[X] +mode->pan[W];
            dim[H] = mode->pan[Y] +mode->pan[H];
            init = 0;
        } else {
            dim[X] = MIN(dim[X], mode->dim[X]);
            dim[Y] = MIN(dim[Y], mode->dim[Y]);
            dim[W] = MAX(dim[W], mode->dim[X] +mode->pan[W]);
            dim[H] = MAX(dim[H], mode->dim[Y] +mode->pan[H]);
        }        

        /* Don't include NULL modes in the effective dimension calculation */
        if (!mode->modeline) continue;

        if (einit) {
            edim[X] = mode->pan[X];
            edim[Y] = mode->pan[Y];
            edim[W] = mode->pan[X] +mode->pan[W];
            edim[H] = mode->pan[Y] +mode->pan[H];
            einit = 0;
        } else {
            edim[X] = MIN(edim[X], mode->dim[X]);
            edim[Y] = MIN(edim[Y], mode->dim[Y]);
            edim[W] = MAX(edim[W], mode->dim[X] +mode->pan[W]);
            edim[H] = MAX(edim[H], mode->dim[Y] +mode->pan[H]);
        }
    }

    dim[W] = dim[W] - dim[X];
    dim[H] = dim[H] - dim[Y];

    edim[W] = edim[W] - edim[X];
    edim[H] = edim[H] - edim[Y];

} /* calc_metamode() */



/** calc_screen() ****************************************************
 *
 * Calculates the dimensions of an X screen
 *
 * - Calculates the smallest bounding box that can hold all of the
 *   metamodes of the X screen.
 *
 **/

static void calc_screen(nvScreenPtr screen)
{
    nvMetaModePtr metamode;
    int *dim;


    if (!screen || screen->no_scanout) return;

    dim = screen->dim;
    metamode = screen->metamodes;

    if (!metamode) {
        dim[X] = 0;
        dim[Y] = 0;
        dim[W] = 0;
        dim[H] = 0;
        return;
    }

    calc_metamode(screen, metamode);
    dim[X] = metamode->dim[X];
    dim[Y] = metamode->dim[Y];
    dim[W] = metamode->dim[X] +metamode->dim[W];
    dim[H] = metamode->dim[Y] +metamode->dim[H];
   
    for (metamode = metamode->next;
         metamode;
         metamode = metamode->next) {

        calc_metamode(screen, metamode);
        dim[X] = MIN(dim[X], metamode->dim[X]);
        dim[Y] = MIN(dim[Y], metamode->dim[Y]);
        dim[W] = MAX(dim[W], metamode->dim[X] +metamode->dim[W]);
        dim[H] = MAX(dim[H], metamode->dim[Y] +metamode->dim[H]);
    }

    dim[W] = dim[W] - dim[X];
    dim[H] = dim[H] - dim[Y];

} /* calc_screen() */



/** calc_layout() ****************************************************
 *
 * Calculates the dimensions (width & height) of the layout.  This is
 * the smallest bounding box that holds all the metamodes of all X
 * screens as well as dummy modes for disabled displays.

 * As a side effect, the dimensions of all metamodes for all X
 * screens are (re)calculated.
 *
 **/

static void calc_layout(nvLayoutPtr layout)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvDisplayPtr display;
    int init = 1;
    int *dim;
    int x, y;


    if (!layout) return;

    resolve_layout(layout);

    dim = layout->dim;
    dim[X] = 0;
    dim[Y] = 0;
    dim[W] = 0;
    dim[H] = 0;

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        int *sdim;

        calc_screen(screen);
        sdim = get_screen_dim(screen, 0);

        if (init) {
            dim[X] = sdim[X];
            dim[Y] = sdim[Y];
            dim[W] = sdim[X] +sdim[W];
            dim[H] = sdim[Y] +sdim[H];
            init = 0;
            continue;
        }

        dim[X] = MIN(dim[X], sdim[X]);
        dim[Y] = MIN(dim[Y], sdim[Y]);
        dim[W] = MAX(dim[W], sdim[X] +sdim[W]);
        dim[H] = MAX(dim[H], sdim[Y] +sdim[H]);
    }

    dim[W] = dim[W] - dim[X];
    dim[H] = dim[H] - dim[Y];


    /* Position disabled display devices off to the top right */
    x = dim[W] + dim[X];
    y = dim[Y];
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->screen) continue;

            display->cur_mode->dim[X] = x;
            display->cur_mode->pan[X] = x;
            display->cur_mode->dim[Y] = y;
            display->cur_mode->pan[Y] = y;

            x += display->cur_mode->dim[W];
            dim[W] += display->cur_mode->dim[W];
            dim[H] = MAX(dim[H], display->cur_mode->dim[H]);
        }
    }

} /* calc_layout() */



/** recenter_screen() ************************************************
 *
 * Makes sure that all the metamodes in the screen have the same top
 * left corner.  This is done by offsetting metamodes back to the
 * screen's bounding box top left corner.
 *
 **/

static void recenter_screen(nvScreenPtr screen)
{
    nvDisplayPtr display;

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        nvModePtr mode;

        for (mode = display->modes; mode; mode = mode->next) {
            int offset_x = (screen->dim[X] - mode->metamode->dim[X]);
            int offset_y = (screen->dim[Y] - mode->metamode->dim[Y]);
            offset_mode(mode, offset_x, offset_y);
        }
    }

    /* Recalculate the screen's dimensions */
    calc_screen(screen);

} /* recenter_screen() */



/** set_screen_metamode() ********************************************
 *
 * Updates the layout structure to make the screen and each of its
 * displays point to the correct metamode/mode.
 *
 **/

static void set_screen_metamode(nvLayoutPtr layout, nvScreenPtr screen,
                                int new_metamode_idx)
{
    nvDisplayPtr display;


    /* Set which metamode the screen is pointing to */
    screen->cur_metamode_idx = new_metamode_idx;
    screen->cur_metamode = get_metamode(screen, new_metamode_idx);

    /* Make each display within the screen point to the new mode */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        display->cur_mode = get_mode(display, new_metamode_idx);
    }

    /* Recalculate the layout dimensions */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);

} /* set_screen_metamode() */



/** recenter_layout() ************************************************
 *
 * Recenters all metamodes of all screens in the layout.  (Makes
 * sure that the top left corner of each screen's metamode is (0,0)
 * if possible.)
 *
 **/

static void recenter_layout(nvLayoutPtr layout)
{
    nvScreenPtr screen;
    int real_metamode_idx;
    int metamode_idx;

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        real_metamode_idx = screen->cur_metamode_idx;

        for (metamode_idx = 0;
             metamode_idx < screen->num_metamodes;
             metamode_idx++) {

            if (metamode_idx == real_metamode_idx) continue;

            set_screen_metamode(layout, screen, metamode_idx);
        }

        set_screen_metamode(layout, screen, real_metamode_idx);
    }

} /* recenter_layout() */



/** reposition_screen() **********************************************
 *
 * Call this after the relative position of a display has changed
 * to make sure the display's screen's absolute position does not
 * change as a result.  (This function should be called before
 * calling calc_layout() such that the screen's top left position
 * can be preserved correctly.)
 *
 **/

static void reposition_screen(nvScreenPtr screen, int resolve_all_modes)
{
    int orig_screen_x = screen->dim[X];
    int orig_screen_y = screen->dim[Y];

    /* Resolve new relative positions.  In basic mode,
     * relative position changes apply to all modes of a
     * display so we should resolve all modes (since they
     * were all changed.)
     */
    resolve_displays_in_screen(screen, resolve_all_modes);

    /* Reestablish the screen's original position */
    screen->dim[X] = orig_screen_x;
    screen->dim[Y] = orig_screen_y;
    recenter_screen(screen);

} /* reposition_screen() */



/** switch_screen_to_absolute() **************************************
 *
 * Prepair a screen for using absolute positioning.  This is needed
 * since screens using relative positioning may not have all their
 * metamodes's top left corner coincideat the same place.  This
 * function makes sure that all metamodes in the screen have the
 * same top left corner by offsetting the modes of metamodes that
 * are offset from the screen's bounding box top left corner.
 *
 **/

static void switch_screen_to_absolute(nvScreenPtr screen)
{
    screen->position_type = CONF_ADJ_ABSOLUTE;
    screen->relative_to   = NULL;

    recenter_screen(screen);

} /* switch_screen_to_absolute() */



/** snap_dim_to_dim() ***********************************************
 *
 * Snaps the sides of two rectangles together.  
 *
 * Snaps the dimensions of "src" to those of "snap" if any part
 * of the "src" rectangle is within "snap_strength" of the "snap"
 * rectangle.  The resulting, snapped, rectangle is returned in
 * "dst", along with the deltas (how far we needed to jump in order
 * to produce a snap) in the vertical and horizontal directions.
 *
 * No vertically snapping occurs if 'best_vert' is NULL.
 * No horizontal snapping occurs if 'best_horz' is NULL.
 *
 **/

static void snap_dim_to_dim(int *dst, int *src, int *snap, int snap_strength,
                            int *best_vert, int *best_horz)
{
    int dist;


    /* Snap vertically */
    if (best_vert) {

        /* Snap top side to top side */
        dist = abs(snap[Y] - src[Y]);
        if (dist < *best_vert) {
            dst[Y] = snap[Y];
            *best_vert = dist;
        }
        
        /* Snap top side to bottom side */
        dist = abs((snap[Y] + snap[H]) - src[Y]);
        if (dist < *best_vert) {
            dst[Y] = snap[Y] + snap[H];
            *best_vert = dist;
        }
        
        /* Snap bottom side to top side */
        dist = abs(snap[Y] - (src[Y] + src[H]));
        if (dist < *best_vert) {
            dst[Y] = snap[Y] - src[H];
            *best_vert = dist;
        }
        
        /* Snap bottom side to bottom side */
        dist = abs((snap[Y] + snap[H]) - (src[Y] + src[H]));
        if (dist < *best_vert) {
            dst[Y] = snap[Y] + snap[H] - src[H];
            *best_vert = dist;
        }
        
        /* Snap midlines */
        if (/* Top of 'src' is above bottom of 'snap' */
            (src[Y]          <= snap[Y] + snap[H] + snap_strength) &&
            /* Bottom of 'src' is below top of 'snap' */
            (src[Y] + src[H] >= snap[Y] - snap_strength)) {
            
            /* Snap vertically */
            dist = abs((snap[Y] + snap[H]/2) - (src[Y]+src[H]/2));
            if (dist < *best_vert) {
                dst[Y] = snap[Y] + snap[H]/2 - src[H]/2;
                *best_vert = dist;
            }
        }
    }


    /* Snap horizontally */
    if (best_horz) {
        
        /* Snap left side to left side */
        dist = abs(snap[X] - src[X]);
        if (dist < *best_horz) {
            dst[X] = snap[X];
            *best_horz = dist;
        }
        
        /* Snap left side to right side */
        dist = abs((snap[X] + snap[W]) - src[X]);
        if (dist < *best_horz) {
            dst[X] = snap[X] + snap[W];
            *best_horz = dist;
        }
        
        /* Snap right side to left side */
        dist = abs(snap[X] - (src[X] + src[W]));
        if (dist < *best_horz) {
            dst[X] = snap[X] - src[W];
            *best_horz = dist;
        }
        
        /* Snap right side to right side */
        dist = abs((snap[X] + snap[W]) - (src[X]+src[W]));
        if (dist < *best_horz) {
            dst[X] = snap[X] + snap[W] - src[W];
            *best_horz = dist;
        }
        
        /* Snap midlines */
        if (/* Left of 'src' is before right of 'snap' */
            (src[X]          <= snap[X] + snap[W] + snap_strength) &&
            /* Right of 'src' is after left of 'snap' */
            (src[X] + src[W] >= snap[X] - snap_strength)) {
            
            /* Snap vertically */
            dist = abs((snap[X] + snap[W]/2) - (src[X]+src[W]/2));
            if (dist < *best_horz) {
                dst[X] = snap[X] + snap[W]/2 - src[W]/2;
                *best_horz = dist;
            }
        }
    }

} /* snap_dim_to_dim() */



/** snap_side_to_dim() **********************************************
 *
 * Snaps the sides of src to snap and stores the result in dst
 *
 * Returns 1 if a snap occured.
 *
 **/

static void snap_side_to_dim(int *dst, int *src, int *snap,
                             int *best_vert, int *best_horz)
{
    int dist;
 

    /* Snap vertically */
    if (best_vert) {

        /* Snap side to top side */
        dist = abs(snap[Y] - (src[Y] + src[H]));
        if (dist < *best_vert) {
            dst[H] = snap[Y] - src[Y];
            *best_vert = dist;
        }
    
        /* Snap side to bottom side */
        dist = abs((snap[Y] + snap[H]) - (src[Y] + src[H]));
        if (dist < *best_vert) {
            dst[H] = snap[Y] + snap[H] - src[Y];
            *best_vert = dist;
        }
    }


    /* Snap horizontally */
    if (best_horz) {

        /* Snap side to left side */
        dist = abs(snap[X] - (src[X] + src[W]));
        if (dist < *best_horz) {
            dst[W] = snap[X] - src[X];
            *best_horz = dist;
        }
        
        /* Snap side to right side */
        dist = abs((snap[X] + snap[W]) - (src[X] + src[W]));
        if (dist < *best_horz) {
            dst[W] = snap[X] + snap[W] - src[X];
            *best_horz = dist;
        }
    }

} /* snap_side_to_dim() */



/** snap_move() *****************************************************
 *
 * Snaps the modify info's source dimensions (src_dim) to other
 * displays/screens by moving the top left coord of the src_dim
 * such that one or two of the edges of the src_dim line up
 * with the closest other screen/display's dimensions.  The results
 * of the snap are placed into the destination dimensions (dst_dim).
 *
 **/

static void snap_move(CtkDisplayLayout *ctk_object)
{
    ModifyInfo *info = &(ctk_object->modify_info);
    int *bv;
    int *bh;
    int i;
    int dist;
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvDisplayPtr other;
    int *sdim;


    /* Snap to other display's modes */
    if (info->display) {
        for (i = 0; i < ctk_object->Zcount; i++) {
            
            if (ctk_object->Zorder[i].type != ZNODE_TYPE_DISPLAY) continue;
            
            other = ctk_object->Zorder[i].u.display;

            /* Other display must have a mode */
            if (!other || !other->cur_mode || !other->screen ||
                other == info->display) continue;

            /* Don't snap to displays that are somehow related.
             * XXX Check for nested relations.
             */
            if (((other->cur_mode->position_type != CONF_ADJ_ABSOLUTE) &&
                 (other->cur_mode->relative_to == info->display)) ||
                ((info->display->cur_mode->position_type != CONF_ADJ_ABSOLUTE) &&
                 (info->display->cur_mode->relative_to == other))) {
                continue;
            }

            /* NOTE: When the display devices's screens are relative to each
             *       other, we may still want to allow snapping of the non-
             *       related edges.  This is useful, for example, when two
             *       screens have a right of/left of relationtship and
             *       one of them is taller.
             */
            bv = &info->best_snap_v;
            bh = &info->best_snap_h;

            if (((other->screen->position_type == CONF_ADJ_RIGHTOF) ||
                 (other->screen->position_type == CONF_ADJ_LEFTOF)) &&
                (other->screen->relative_to == info->screen)) {
                bh = NULL;
            }
            if (((info->screen->position_type == CONF_ADJ_RIGHTOF) ||
                 (info->screen->position_type == CONF_ADJ_LEFTOF)) &&
                (info->screen->relative_to == other->screen)) {
                bh = NULL;
            }

            if (((other->screen->position_type == CONF_ADJ_ABOVE) ||
                 (other->screen->position_type == CONF_ADJ_BELOW)) &&
                (other->screen->relative_to == info->screen)) {
                bv = NULL;
            }
            if (((info->screen->position_type == CONF_ADJ_ABOVE) ||
                 (info->screen->position_type == CONF_ADJ_BELOW)) &&
                (info->screen->relative_to == other->screen)) {
                bv = NULL;
            }
            
            /* Snap to other display's panning dimensions */
            snap_dim_to_dim(info->dst_dim,
                            info->src_dim,
                            other->cur_mode->pan,
                            ctk_object->snap_strength, bv, bh);

            /* Snap to other display's dimensions */
            snap_dim_to_dim(info->dst_dim,
                            info->src_dim,
                            other->cur_mode->dim,
                            ctk_object->snap_strength, bv, bh);
        }

    } /* Done snapping to other displays */


    /* Snap to dimensions of other X screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        if (screen == info->screen) continue;

        /* NOTE: When the (display devices's) screens are relative to
         *       each other, we may still want to allow snapping of the
         *       non-related edges.  This is useful, for example, when
         *       two screens have a right of/left of relationtship and
         *       one of them is taller.
         */

        bv = &info->best_snap_v;
        bh = &info->best_snap_h;

        if (((screen->position_type == CONF_ADJ_RIGHTOF) ||
             (screen->position_type == CONF_ADJ_LEFTOF)) &&
            (screen->relative_to == info->screen)) {
            bh = NULL;
        }
        if (((info->screen->position_type == CONF_ADJ_RIGHTOF) ||
             (info->screen->position_type == CONF_ADJ_LEFTOF)) &&
            (info->screen->relative_to == screen)) {
            bh = NULL;
        }

        /* If we aren't snapping horizontally with the other screen,
         * we shouldn't snap vertically either if we are moving the
         * top-most display in the screen.
         */
        if (!bh &&
            info->display &&
            info->display->cur_mode->dim[Y] == info->screen->dim[Y]) {
            bv = NULL;
        }

        if (((screen->position_type == CONF_ADJ_ABOVE) ||
             (screen->position_type == CONF_ADJ_BELOW)) &&
            (screen->relative_to == info->screen)) {
            bv = NULL;
        }
        if (((info->screen->position_type == CONF_ADJ_ABOVE) ||
             (info->screen->position_type == CONF_ADJ_BELOW)) &&
            (info->screen->relative_to == screen)) {
            bv = NULL;
        }

        /* If we aren't snapping vertically with the other screen,
         * we shouldn't snap horizontally either if this is the
         * left-most display in the screen.
         */
        if (!bv &&
            info->display &&
            info->display->cur_mode->dim[X] == info->screen->dim[X]) {
            bh = NULL;
        }

        sdim = get_screen_dim(screen, 0);
        snap_dim_to_dim(info->dst_dim,
                        info->src_dim,
                        sdim,
                        ctk_object->snap_strength, bv, bh);
    }

    /* Snap to the maximum screen dimensions */
    bv = &info->best_snap_v;
    bh = &info->best_snap_h;

    if (info->display) {
        dist = abs( (info->screen->dim[X] + info->gpu->max_width)
                   -(info->src_dim[X] + info->src_dim[W]));
        if (dist < *bh) {
            info->dst_dim[X] =
                info->screen->dim[X] + info->gpu->max_width - info->src_dim[W];
            *bh = dist;
        }
        dist = abs( (info->screen->dim[Y] + info->gpu->max_height)
                   -(info->src_dim[Y] + info->src_dim[H]));
        if (dist < *bv) {
            info->dst_dim[Y] =
                info->screen->dim[Y] + info->gpu->max_height - info->src_dim[H];
            *bv = dist;
        }
    }

} /* snap_move() */



/** snap_pan() ******************************************************
 *
 * Snaps the modify info's source dimensions (src_dim) bottom right
 * edge(s) to other displays/screens by growing/shrinking the
 * size of the src_dim such that the edge(s) of the src_dim line up
 * with the closest other screen/display's dimensions.  The results
 * of the snap are placed into the destination dimensions (dst_dim).
 *
 * This is used for changing both the panning domain of a display
 * device as well as setting a (no-scannout) screen's virtual size.
 *
 **/

static void snap_pan(CtkDisplayLayout *ctk_object)
{
    ModifyInfo *info = &(ctk_object->modify_info);
    int *bv;
    int *bh;
    int i;
    int dist;
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    nvDisplayPtr other;
    int *sdim;


    if (info->display) {
        /* Snap to multiples of the display's dimensions */
        bh = &(info->best_snap_h);
        bv = &(info->best_snap_v);

        dist = info->src_dim[W] % info->display->cur_mode->dim[W];
        if (dist < *bh) {
            info->dst_dim[W] = info->display->cur_mode->dim[W] *
                (int)(info->src_dim[W] / info->display->cur_mode->dim[W]);
            *bh = dist;
        }
        dist = info->display->cur_mode->dim[W] -
            (info->src_dim[W] % info->display->cur_mode->dim[W]);
        if (dist < *bh) {
            info->dst_dim[W] = info->display->cur_mode->dim[W] *
                (1 + (int)(info->src_dim[W] / info->display->cur_mode->dim[W]));
            *bh = dist;
        }
        dist = abs(info->src_dim[H] % info->display->cur_mode->dim[H]);
        if (dist < *bv) {
            info->dst_dim[H] = info->display->cur_mode->dim[H] *
                (int)(info->src_dim[H] / info->display->cur_mode->dim[H]);
            *bv = dist;
        }
        dist = info->display->cur_mode->dim[H] -
            (info->src_dim[H] % info->display->cur_mode->dim[H]);
        if (dist < *bv) {
            info->dst_dim[H] = info->display->cur_mode->dim[H] *
                (1 + (int)(info->src_dim[H] / info->display->cur_mode->dim[H]));
            *bv = dist;
        }
    }


    /* Snap to other display's modes */
    for (i = 0; i < ctk_object->Zcount; i++) {

        if (ctk_object->Zorder[i].type != ZNODE_TYPE_DISPLAY) continue;

        other = ctk_object->Zorder[i].u.display;

        /* Other display must have a mode */
        if (!other || !other->cur_mode || !other->screen ||
            other == info->display) continue;


        /* NOTE: When display devices are relative to each other,
         *       we may still want to allow snapping of the non-related
         *       edges.  This is useful, for example, when two
         *       displays have a right of/left of relationtship and
         *       one of the displays is taller.
         */
        bv = &info->best_snap_v;
        bh = &info->best_snap_h;

        /* Don't snap horizontally to other displays that are somehow
         * related on the right edge of the display being panned.
         */
        if (info->display) {
            if ((other->cur_mode->position_type == CONF_ADJ_RIGHTOF) &&
                other->cur_mode->relative_to == info->display) {
                bh = NULL;
            }
            if ((info->display->cur_mode->position_type == CONF_ADJ_LEFTOF) &&
                info->display->cur_mode->relative_to == other) {
                bh = NULL;
            }
        }
        if ((other->screen->position_type == CONF_ADJ_RIGHTOF) &&
            other->screen->relative_to == info->screen) {
            bh = NULL;
        }
        if ((info->screen->position_type == CONF_ADJ_LEFTOF) &&
            info->screen->relative_to == other->screen) {
            bh = NULL;
        }

        /* Don't snap vertically to other displays that are somehow
         * related on the bottom edge of the display being panned.
         */
        if (info->display) {
            if ((other->cur_mode->position_type == CONF_ADJ_BELOW) &&
                other->cur_mode->relative_to == info->display) {
                bv = NULL;
            }
            if ((info->display->cur_mode->position_type == CONF_ADJ_ABOVE) &&
                info->display->cur_mode->relative_to == other) {
                bv = NULL;
            }
        }
        if ((other->screen->position_type == CONF_ADJ_BELOW) &&
            other->screen->relative_to == info->screen) {
            bv = NULL;
        }
        if ((info->screen->position_type == CONF_ADJ_ABOVE) &&
            info->screen->relative_to == other->screen) {
            bv = NULL;
        }

        /* Snap to other display panning dimensions */
        snap_side_to_dim(info->dst_dim,
                         info->src_dim,
                         other->cur_mode->pan,
                         bv, bh);
        
        /* Snap to other display dimensions */
        snap_side_to_dim(info->dst_dim,
                         info->src_dim,
                         other->cur_mode->dim,
                         bv, bh);
    }


    /* Snap to dimensions of other X screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        if (screen == info->screen) continue;

        bv = &info->best_snap_v;
        bh = &info->best_snap_h;

        /* Don't snap horizontally to other screens that are somehow
         * related on the right edge of the (display's) screen being
         * panned.
         */
        if ((screen->position_type == CONF_ADJ_RIGHTOF) &&
            (screen->relative_to == info->screen)) {
            bh = NULL;
        }
        if ((info->screen->position_type == CONF_ADJ_LEFTOF) &&
            (info->screen->relative_to == screen)) {
            bh = NULL;
        }

        /* Don't snap vertically to other screens that are somehow
         * related on the bottom edge of the (display's) screen being
         * panned.
         */
        if ((screen->position_type == CONF_ADJ_BELOW) &&
            (screen->relative_to == info->screen)) {
            bv = NULL;
        }
        if ((info->screen->position_type == CONF_ADJ_ABOVE) &&
            (info->screen->relative_to == screen)) {
            bv = NULL;
        }

        sdim = get_screen_dim(screen, 0);
        snap_side_to_dim(info->dst_dim,
                         info->src_dim,
                         sdim,
                         bv, bh);
    }

    bh = &(info->best_snap_h);
    bv = &(info->best_snap_v);

    /* Snap to the maximum screen width */
    dist = abs((info->screen->dim[X] + info->gpu->max_width)
               -(info->src_dim[X] + info->src_dim[W]));
    if (dist < *bh) {
        info->dst_dim[W] = info->screen->dim[X] + info->gpu->max_width -
            info->src_dim[X];
        *bh = dist;
    }

    /* Snap to the maximum screen height */
    dist = abs((info->screen->dim[Y] + info->gpu->max_height)
               -(info->src_dim[Y] + info->src_dim[H]));
    if (dist < *bv) {
        info->dst_dim[H] = info->screen->dim[Y] + info->gpu->max_height -
            info->src_dim[Y];
        *bv = dist;
    }

} /* snap_pan() */



/** move_selected() **************************************************
 *
 * Moves whatever is selected by the given x and y offsets.  This
 * function handles movement of relative and absolute positions as
 * well as snapping.
 *
 * Returns 1 if the layout was modified
 *
 **/

static int move_selected(CtkDisplayLayout *ctk_object, int x, int y, int snap)
{
    nvLayoutPtr layout = ctk_object->layout;
    ModifyInfo *info = &(ctk_object->modify_info);
    int modified = 0;

    int *dim; /* Temp dimensions */
    int *sdim; /* Temp screen dimensions */


    info->modify_panning = 0;
    if (!get_modify_info(ctk_object)) return 0;


    /* Should we snap */
    info->snap = snap;


    /* Moving something that is using relative positioning can be done
     * fairly cleanly with common code, so do that here.
     */
    if (info->orig_position_type != CONF_ADJ_ABSOLUTE) {
        int p_x = (ctk_object->mouse_x - ctk_object->img_dim[X]) /
            ctk_object->scale;
        int p_y = (ctk_object->mouse_y - ctk_object->img_dim[Y]) /
            ctk_object->scale;

        if (info->display) {
            dim = info->display->cur_mode->relative_to->cur_mode->dim;
        } else {
            dim = get_screen_dim(info->screen->relative_to, 0);
        }

        if (dim) {
            /* Compute the new orientation based on the mouse position */
            *(info->target_position_type) =
                get_point_relative_position(dim, p_x, p_y);

            /* For displays, while in basic mode, make sure that the
             * relative position applies to all metamodes.
             */
            if (info->display) {

                if (!ctk_object->advanced_mode) {
                    nvModePtr mode;
                    for (mode = info->display->modes; mode;
                         mode = mode->next) {
                        mode->position_type = *(info->target_position_type);
                    }
                }

                /* Make sure the screen position does not change */
                reposition_screen(info->screen, !ctk_object->advanced_mode);
                /* Always update the modify dim for relative positioning */
                info->modify_dirty = 1;
            }
        }

    } else {
        /* Move via absolute positioning */

        /* Compute pre-snap dimensions */
        info->modify_dim[X] += x;
        info->modify_dim[Y] += y;

        info->dst_dim[X] = info->modify_dim[X];
        info->dst_dim[Y] = info->modify_dim[Y];
        info->dst_dim[W] = info->modify_dim[W];
        info->dst_dim[H] = info->modify_dim[H];


        /* Snap to other screens and displays */
        if (snap && ctk_object->snap_strength) {

            info->src_dim[X] = info->dst_dim[X];
            info->src_dim[Y] = info->dst_dim[Y];
            info->src_dim[W] = info->dst_dim[W];
            info->src_dim[H] = info->dst_dim[H];

            snap_move(ctk_object);

            if (info->display) {

                /* Also snap display's panning box to other screens/displays */
                info->src_dim[W] = info->display->cur_mode->pan[W];
                info->src_dim[H] = info->display->cur_mode->pan[H];
                info->dst_dim[W] = info->src_dim[W];
                info->dst_dim[H] = info->src_dim[H];

                snap_move(ctk_object);
            }
        }

        /* Get the bounding dimensions of what is being moved */
        if (info->display) {
            dim = info->display->cur_mode->pan;
        } else {
            dim = info->target_dim;
        }
        sdim = get_screen_dim(info->screen, 1);

        /* Prevent moving out of the max layout bounds */
        x = MAX_LAYOUT_WIDTH - dim[W];
        if (info->dst_dim[X] > x) {
            info->modify_dim[X] += x - info->dst_dim[X];
            info->dst_dim[X] = x;
        }
        y = MAX_LAYOUT_HEIGHT - dim[H];
        if (info->dst_dim[Y] > y) {
            info->modify_dim[Y] += y - info->dst_dim[Y];
            info->dst_dim[Y] = y;
        }
        x = layout->dim[W] - MAX_LAYOUT_WIDTH;
        if (info->dst_dim[X] < x) {
            info->modify_dim[X] += x - info->dst_dim[X];
            info->dst_dim[X] = x;
        }
        y = layout->dim[H] - MAX_LAYOUT_HEIGHT;
        if (info->dst_dim[Y] < y) {
            info->modify_dim[Y] += y - info->dst_dim[Y];
            info->dst_dim[Y] = y;
        }

        /* Prevent screen from growing too big */
        x = sdim[X] + info->gpu->max_width - dim[W]; 
        if (info->dst_dim[X] > x) {
            info->modify_dim[X] += x - info->dst_dim[X];
            info->dst_dim[X] = x;
        }
        y = sdim[Y] + info->gpu->max_height - dim[H];
        if (info->dst_dim[Y] > y) {
            info->modify_dim[Y] += y - info->dst_dim[Y];
            info->dst_dim[Y] = y;
        }
        x = sdim[X] + sdim[W] - info->gpu->max_width;
        if (info->dst_dim[X] < x) {
            info->modify_dim[X] += x - info->dst_dim[X];
            info->dst_dim[X] = x;
        }
        y = sdim[Y] + sdim[H] - info->gpu->max_height;
        if (info->dst_dim[Y] < y) {
            info->modify_dim[Y] += y - info->dst_dim[Y];
            info->dst_dim[Y] = y;
        }


        /* Apply the move */
        if (!info->display) {
            /* Move the screen */
            nvDisplayPtr display;

            x = info->dst_dim[X] - info->orig_dim[X];
            y = info->dst_dim[Y] - info->orig_dim[Y];

            /* Offset the screen and all its displays */
            offset_screen(info->screen, x, y);
            for (display = info->screen->displays;
                 display;
                 display = display->next_in_screen) {
                offset_display(display, x, y);
            }

        } else {
            /* Move the display to its destination */
            info->display->cur_mode->dim[X] = info->dst_dim[X];
            info->display->cur_mode->dim[Y] = info->dst_dim[Y];
            info->display->cur_mode->pan[X] = info->dst_dim[X];
            info->display->cur_mode->pan[Y] = info->dst_dim[Y];

            /* If the screen of the display that was moved is using absolute
             * positioning, we should check to see if the position of the
             * metamode has changed and if so, offset other metamodes on the
             * screen (hence moving the screen's position.)
             *
             * If the screen is using relative positioning, don't offset
             * metamodes since the screen's position is based on another
             * screen which will get resolved later.
             */
            if (info->screen->position_type == CONF_ADJ_ABSOLUTE &&
                info->screen->cur_metamode) {

                resolve_displays_in_screen(info->screen, 0);
                calc_metamode(info->screen, info->screen->cur_metamode);
                x = info->screen->cur_metamode->dim[X] - info->orig_screen_dim[X];
                y = info->screen->cur_metamode->dim[Y] - info->orig_screen_dim[Y];

                if (x || y) {
                    nvDisplayPtr other;
                    nvModePtr mode;
                    for (other = info->screen->displays;
                         other;
                         other = other->next_in_screen) {

                        for (mode = other->modes; mode; mode = mode->next) {

                            /* Only move non-current modes */
                            if (mode == other->cur_mode) continue;

                            /* Don't move modes that are relative */
                            if (mode->position_type != CONF_ADJ_ABSOLUTE) continue;

                            offset_mode(mode, x, y);
                        }
                    }
                }
            }
        }
    }

    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);

    /* If layout needs to offset, it was modified */
    if (layout->dim[X] || layout->dim[Y]) {
        offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
        modified = 1;
    }

    recenter_layout(layout);
    sync_scaling(ctk_object);


    /* If what we moved required the layout to be shifted, offset
     * the modify dim (used for snapping) by the same displacement.
     */
    x = info->target_dim[X] - info->dst_dim[X];
    y = info->target_dim[Y] - info->dst_dim[Y];
    if (x || y) {
        info->modify_dim[X] += x;
        info->modify_dim[Y] += y;
    }

    /* Check if the item being moved has a new position */
    if (*(info->target_position_type) != info->orig_position_type ||
        info->target_dim[X] != info->orig_dim[X] ||
        info->target_dim[Y] != info->orig_dim[Y]) {
        modified = 1;
    }

    // XXX Screen could have changed position due to display moving.

    return modified;

} /* move_selected() */



/** pan_selected() ***************************************************
 *
 * Changes the size of the panning domain of the selected display.
 *
 **/

static int pan_selected(CtkDisplayLayout *ctk_object, int x, int y, int snap)
{
    nvLayoutPtr layout = ctk_object->layout;
    ModifyInfo *info = &(ctk_object->modify_info);
    int modified = 0;

    int *dim;
    int extra;


    info->modify_panning = 1;
    if (!get_modify_info(ctk_object)) return 0;


    /* Only allow changing the panning of displays and the size
     * of no-scanout screens.
     */
    if (!info->display && !info->screen->no_scanout) return 0;

    info->snap = snap;


    /* Compute pre-snap dimensions */
    info->modify_dim[W] += x;
    info->modify_dim[H] += y;

    /* Don't allow the panning domain to get too small */
    if (info->display) {
        dim = info->display->cur_mode->dim;
        if (info->modify_dim[W] < dim[W]) {
            info->modify_dim[W] = dim[W];
        }
        if (info->modify_dim[H] < dim[H]) {
            info->modify_dim[H] = dim[H];
        }
    } else if (info->screen->no_scanout) {
        if (info->modify_dim[W] < 304) {
            info->modify_dim[W] = 304;
        }
        if (info->modify_dim[H] < 200) {
            info->modify_dim[H] = 200;
        }
    }

    info->dst_dim[W] = info->modify_dim[W];
    info->dst_dim[H] = info->modify_dim[H];


    /* Snap to other screens and dimensions */
    if (snap && ctk_object->snap_strength) {

        info->src_dim[X] = info->dst_dim[X];
        info->src_dim[Y] = info->dst_dim[Y];
        info->src_dim[W] = info->dst_dim[W];
        info->src_dim[H] = info->dst_dim[H];

        snap_pan(ctk_object);
    }

    /* Make sure no-scanout virtual screen width is at least a multiple of 8 */
    if (info->screen->no_scanout) {
        extra = (info->dst_dim[W] % 8);
        if (extra > 0) {
            info->dst_dim[W] += (8 - extra);
        }
    }

    /* Panning should not cause us to exceed the maximum layout dimensions */
    x = MAX_LAYOUT_WIDTH - info->dst_dim[X];
    if (info->dst_dim[W] > x) {
        info->modify_dim[W] += x - info->dst_dim[W];
        info->dst_dim[W] = x;
    }
    y = MAX_LAYOUT_HEIGHT - info->dst_dim[Y];
    if (info->dst_dim[H] > y) {
        info->modify_dim[H] += y - info->dst_dim[H];
        info->dst_dim[H] = y;
    }

    /* Panning should not cause us to exceed the maximum screen dimensions */
    dim = get_screen_dim(info->screen, 1);
    x = dim[X] + info->gpu->max_width - info->dst_dim[X];
    if (info->dst_dim[W] > x) {
        info->modify_dim[W] += x - info->dst_dim[W];
        info->dst_dim[W] = x;
    }
    y = dim[Y] + info->gpu->max_height - info->dst_dim[Y];
    if (info->dst_dim[H] > y) {
        info->modify_dim[H] += y - info->dst_dim[H];
        info->dst_dim[H] = y;
    }

    /* Panning domain can never be smaller then the display viewport */
    if (info->display) {
        dim = info->display->cur_mode->dim;
        if (info->dst_dim[W] < dim[W]) {   
            info->dst_dim[W] = dim[W];
        }
        if (info->dst_dim[H] < dim[H]) {
            info->dst_dim[H] = dim[H];
        }
    } else if (info->screen->no_scanout) {
        if (info->dst_dim[W] < 304) {
            info->dst_dim[W] = 304;
        }
        if (info->dst_dim[H] < 200) {
            info->dst_dim[H] = 200;
        }
    }

    /* Assign the new size */
    info->target_dim[W] = info->dst_dim[W];
    info->target_dim[H] = info->dst_dim[H];


    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);

    /* If layout needs to offset, something moved */
    if (layout->dim[X] || layout->dim[Y]) {
        offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
        modified = 1;
    }

    recenter_layout(layout);
    sync_scaling(ctk_object);


    /* Check if the item being moved has a new position */
    /* Report if anything changed */
    if (info->target_dim[W] != info->orig_dim[W] ||
        info->target_dim[H] != info->orig_dim[H]) {
        modified = 1;
    }

    // XXX Screen could have changed position due to display moving.

    return modified;

} /* pan_selected() */


/** get_screen_zorder_move_data() ************************************
 *
 * Looks for the screen in the Z order, and if the position given
 * where the screen is to be moved to is different than there the
 * screen currently is, returns a backup buffer of the zorder of
 * the screen and its displays (also also returns the current zorder
 * position of the screen).
 *
 **/

static ZNode *get_screen_zorder_move_data(CtkDisplayLayout *ctk_object,
                                          nvScreenPtr screen,
                                          int move_to,
                                          int *screen_at)
{
    ZNode *tmpzo;
    int i;

    if (!screen) return NULL;

    for (i = 0; i < ctk_object->Zcount; i++) {

        if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN &&
            ctk_object->Zorder[i].u.screen == screen) {

            /* Only move screen if it is not moving to the same location */
            if (move_to != i) {

                tmpzo = malloc((1 + screen->num_displays) * sizeof(ZNode));
                if (!tmpzo) return NULL;
                
                memcpy(tmpzo,
                       ctk_object->Zorder + i - screen->num_displays,
                       (1 + screen->num_displays)*sizeof(ZNode)); 
                
                if (screen_at) {
                    *screen_at = i;
                }
                return tmpzo;
            }
            break;
        }
    }

    return NULL;

} /* get_screen_zorder_move_data() */


/** select_screen() **************************************************
 *
 * Selects the given screen (by moving it and all of its displays
 * to the top of the zorder).
 *
 **/

static void select_screen(CtkDisplayLayout *ctk_object, nvScreenPtr screen)
{
    int move_to = 0;
    int screen_at;
    ZNode *tmpzo;

    if (!screen) {
        goto done;
    }


     /* Move the screen and its displays to the top */
    move_to = 0 + screen->num_displays;

    tmpzo = get_screen_zorder_move_data(ctk_object, screen, move_to,
                                        &screen_at);
    if (!tmpzo) {
        goto done;
    }

    /* Move other nodes down to make room at the top */
    memmove(ctk_object->Zorder + 1 + screen->num_displays,
            ctk_object->Zorder,
            (screen_at - screen->num_displays)*sizeof(ZNode));

    /* Copy the screen and its displays to the top */
    memcpy(ctk_object->Zorder, tmpzo,
           (1 + screen->num_displays)*sizeof(ZNode));

    free(tmpzo);

 done:
    ctk_object->selected_screen = screen;

} /* select_screen() */



/** select_display() *************************************************
 *
 * Moves the specified display to the top of the Z-order.
 *
 **/

static void select_display(CtkDisplayLayout *ctk_object, nvDisplayPtr display)
{
    int i;

    if (!display) {
        select_screen(ctk_object, NULL);
        goto done;
    }

    /* Move the screen and its displays to the top of the Z order */
    select_screen(ctk_object, display->screen);

    /* Move the display to the top of the Z order */
    for (i = 0; i < ctk_object->Zcount; i++) {

        /* Find the display */
        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY &&
            ctk_object->Zorder[i].u.display == display) {

            /* Move all nodes above this one down by one location */
            if (i > 0) {
                memmove(ctk_object->Zorder + 1, ctk_object->Zorder,
                        i*sizeof(ZNode));

                /* Place the display at the top */
                ctk_object->Zorder[0].type = ZNODE_TYPE_DISPLAY;
                ctk_object->Zorder[0].u.display = display;
            }
            break;
        }
    }

 done:
    ctk_object->selected_display = display;

} /* select_display() */



/** select_default_item() ********************************************
 *
 * Select the top left most element (display/screen).
 *
 */

#define DIST_SQR(D) (((D)[X] * (D)[X]) + ((D)[Y] * (D)[Y]))

static void select_default_item(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr sel_display = NULL;
    nvScreenPtr sel_screen = NULL;
    nvScreenPtr screen;
    nvDisplayPtr display;
    int i;
    int best_dst = -1; // Distance squared to element.
    int dst;

    
    for (i = 0; i < ctk_object->Zcount; i++) {

        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;

            /* Ignore disabled displays */
            if (!display->cur_mode) continue;

            dst = DIST_SQR(display->cur_mode->dim);
            if (best_dst < 0 || dst < best_dst) {
                best_dst = dst;
                sel_display = display;
                sel_screen = NULL;
            }
            
        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            screen = ctk_object->Zorder[i].u.screen;

            /* Only select no-scanout screens */
            if (screen->num_displays > 0) continue;

            dst = DIST_SQR(screen->dim);
            if (best_dst < 0 || dst < best_dst) {
                best_dst = dst;
                sel_display = NULL;
                sel_screen = screen;
            }
        }
    }

    if (sel_display) {
        select_display(ctk_object, sel_display);
    } else if (sel_screen) {
        select_screen(ctk_object, sel_screen);
    }

} /* select_default_item() */



/** get_display_tooltip() ********************************************
 *
 * Returns the text to use for displaying a tooltip from the given
 * display:
 * 
 *   MONITOR NAME : WIDTHxHEIGHT @ HERTZ (GPU NAME)
 *
 * The caller should free the string that is returned.
 *
 **/

static char *get_display_tooltip(nvDisplayPtr display, Bool advanced)
{ 
    char *tip;


    /* No display given */
    if (!display) {
        return NULL;
    }


    /* Display does not have a screen (not configured) */
    if (!(display->screen)) {
        tip = g_strdup_printf("%s : Disabled (GPU: %s)",
                              display->logName, display->gpu->name);


    /* Basic view */
    } else if (!advanced) {
        
        /* Display has no mode */
        if (!display->cur_mode) {
            tip = g_strdup_printf("%s", display->logName);
            
            
        /* Display does not have a current modeline (Off) */
        } else if (!(display->cur_mode->modeline)) {
            tip = g_strdup_printf("%s : Off",
                                  display->logName);
            
        /* Display has mode/modeline */
        } else {
            float ref = display->cur_mode->modeline->refresh_rate;
            tip = g_strdup_printf("%s : %dx%d @ %.*f Hz",
                                  display->logName,
                                  display->cur_mode->modeline->data.hdisplay,
                                  display->cur_mode->modeline->data.vdisplay,
                                  (display->is_sdi ? 3 : 0),
                                  ref);
        }
        

    /* Advanced view */
    } else {


        /* Display has no mode */
        if (!display->cur_mode) {
            tip = g_strdup_printf("%s\n(X Screen %d)\n(GPU: %s)",
                                  display->logName,
                                  display->screen->scrnum,
                                  display->gpu->name);
            
        /* Display does not have a current modeline (Off) */
        } else if (!(display->cur_mode->modeline)) {
            tip = g_strdup_printf("%s : Off\n(X Screen %d)\n(GPU: %s)",
                                  display->logName,
                                  display->screen->scrnum,
                                  display->gpu->name);
            
            /* Display has mode/modeline */
        } else {
            float ref = display->cur_mode->modeline->refresh_rate;
            tip = g_strdup_printf("%s : %dx%d @ %.*f Hz\n(X Screen %d)\n"
                                  "(GPU: %s)",
                                  display->logName,
                                  display->cur_mode->modeline->data.hdisplay,
                                  display->cur_mode->modeline->data.vdisplay,
                                  (display->is_sdi ? 3 : 0),
                                  ref,
                                  display->screen->scrnum,
                                  display->gpu->name);
        }
    }
            
    return tip;

} /* get_display_tooltip() */



/** get_screen_tooltip() *********************************************
 *
 * Returns the text to use for displaying a tooltip from the given
 * screen:
 * 
 *   SCREEN NUMBER (GPU NAME)
 *
 * The caller should free the string that is returned.
 *
 **/

static char *get_screen_tooltip(nvScreenPtr screen, Bool advanced)
{ 
    char *tip;


    /* No display given */
    if (!screen) {
        return NULL;
    }


    /* Basic view */
    if (!advanced) {
        
        tip = g_strdup_printf("X Screen %d%s", screen->scrnum,
                              screen->no_scanout ? " : No Scanout" : ""
                              );

    /* Advanced view */
    } else {

        tip = g_strdup_printf("X Screen %d%s\n(GPU: %s)",
                              screen->scrnum,
                              screen->no_scanout ? " : No Scanout" : "",
                              screen->gpu->name);
    }
            
    return tip;

} /* get_screen_tooltip() */



/** get_tooltip_under_mouse() ****************************************
 *
 * Returns the tooltip text that should be used to give information
 * about the item under the mouse at x, y.
 *
 * The caller should free the string that is returned.
 *
 **/

static char *get_tooltip_under_mouse(CtkDisplayLayout *ctk_object,
                                             int x, int y)
{
    static nvDisplayPtr last_display = NULL;
    static nvScreenPtr  last_screen = NULL;
    int i;
    nvDisplayPtr display = NULL;
    nvScreenPtr screen = NULL;
    char *tip = NULL;
    int *sdim;
    
 
    /* Scale and offset x & y so they reside in clickable area */
    x = (x -ctk_object->img_dim[X]) / ctk_object->scale;
    y = (y -ctk_object->img_dim[Y]) / ctk_object->scale;


    /* Go through the Z-order looking for what we are under */
    for (i = 0; i < ctk_object->Zcount; i++) {
        
        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;
            if (display->cur_mode &&
                point_in_dim(display->cur_mode->pan, x, y)) {
                screen = NULL;
                if (display == last_display) {
                    goto found;
                }
                tip = get_display_tooltip(display, ctk_object->advanced_mode);
                goto found;
            }

        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            screen = ctk_object->Zorder[i].u.screen;
            sdim = get_screen_dim(screen, 1);
            if (point_in_dim(sdim, x, y)) {
                display = NULL;
                if (screen == last_screen) {
                    goto found;
                }
                tip = get_screen_tooltip(screen, ctk_object->advanced_mode);
                goto found;
            }
        }
    }

    /* Handle mouse over nothing for the first time */
    if (last_display || last_screen) {
        last_display = NULL;
        last_screen = NULL;
        return g_strdup("No Display");
    }

    return NULL;

 found:
    last_display = display;
    last_screen = screen;
    return tip;

} /* get_tooltip_under_mouse() */



/** click_layout() ***************************************************
 *
 * Preforms a click in the layout, possibly selecting a display.
 *
 **/

static int click_layout(CtkDisplayLayout *ctk_object, int x, int y)
{
    int i;
    nvDisplayPtr cur_selected_display = ctk_object->selected_display;
    nvScreenPtr cur_selected_screen = ctk_object->selected_screen;
    nvDisplayPtr display;
    nvScreenPtr screen;
    int *sdim;


    /* Assume user didn't actually click inside a display for now */
    ctk_object->clicked_outside = 1;
    ctk_object->selected_display = NULL;
    ctk_object->selected_screen = NULL;


    /* Look through the Z-order for the next element */
    for (i = 0; i < ctk_object->Zcount; i++) {
        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;
            if (display->cur_mode &&
                point_in_dim(display->cur_mode->pan, x, y)) {

                select_display(ctk_object, display);
                ctk_object->clicked_outside = 0;
                break;
            }
        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            screen = ctk_object->Zorder[i].u.screen;
            sdim = get_screen_dim(screen, 1);
            if (point_in_dim(sdim, x, y)) {
                select_screen(ctk_object, screen);
                ctk_object->clicked_outside = 0;
                break;
            }
        }
    }


    /* Don't allow clicking outside - reselect what was last selected */
    if (ctk_object->clicked_outside) {
        ctk_object->selected_display = cur_selected_display;
        ctk_object->selected_screen = cur_selected_screen;

    } else {

        /* Sync modify dimensions to what was newly selected */
        ctk_object->modify_info.modify_dirty = 1;
    }

    return 1;

} /* click_layout() */



/** ctk_display_layout_get_type() ************************************
 *
 * Returns the CtkDisplayLayout type.
 *
 **/

GType ctk_display_layout_get_type(void)
{
    static GType ctk_display_layout_type = 0;

    if (!ctk_display_layout_type) {
        static const GTypeInfo ctk_display_layout_info = {
            sizeof (CtkDisplayLayoutClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkDisplayLayout),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_display_layout_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkDisplayLayout", &ctk_display_layout_info, 0);
    }

    return ctk_display_layout_type;

} /* ctk_display_layout_get_type() */



/** ctk_display_layout_new() *****************************************
 *
 * CTK Display Layout widget creation.
 *
 */
GtkWidget* ctk_display_layout_new(NvCtrlAttributeHandle *handle,
                                  CtkConfig *ctk_config,
                                  nvLayoutPtr layout,
                                  int width,
                                  int height)
{
    GObject *object;
    CtkDisplayLayout *ctk_object;
    GtkWidget *tmp;
    PangoFontDescription *font_description;
    int i;


    /* Make sure we have a handle */
    g_return_val_if_fail(handle != NULL, NULL);


    /* Create the ctk object */
    object = g_object_new(CTK_TYPE_DISPLAY_LAYOUT, NULL);
    ctk_object = CTK_DISPLAY_LAYOUT(object);
    ctk_object->selected_callback = NULL;
    ctk_object->selected_callback_data = NULL;
    ctk_object->modified_callback = NULL;
    ctk_object->modified_callback_data = NULL;
    ctk_object->Zorder = NULL;
    ctk_object->Zcount = 0;

    /* Setup widget properties */
    ctk_object->ctk_config = ctk_config;

    ctk_object->handle = handle;
    ctk_object->layout = layout;

    calc_layout(layout);
    sync_scaling(ctk_object);
    zorder_layout(ctk_object);
    select_default_item(ctk_object);

    /* Setup Pango layout/font */
    ctk_object->pango_layout =
        gtk_widget_create_pango_layout(GTK_WIDGET(ctk_object), NULL);

    pango_layout_set_alignment(ctk_object->pango_layout, PANGO_ALIGN_CENTER);

    font_description = pango_font_description_new();
    pango_font_description_set_family(font_description, "Sans");
    pango_font_description_set_weight(font_description, PANGO_WEIGHT_BOLD);

    pango_layout_set_font_description(ctk_object->pango_layout,
                                      font_description);


    /* Setup colors */
    gdk_color_parse(LAYOUT_IMG_FG_COLOR,     &(ctk_object->fg_color));
    gdk_color_parse(LAYOUT_IMG_BG_COLOR,     &(ctk_object->bg_color));
    gdk_color_parse(LAYOUT_IMG_SELECT_COLOR, &(ctk_object->select_color));

    /* Parse the device color palettes */
    ctk_object->color_palettes = calloc(NUM_COLORS, sizeof(GdkColor));
    for (i = 0; i < NUM_COLORS; i++) {
        gdk_color_parse(__palettes_color_names[i],
                        &(ctk_object->color_palettes[i]));
    }


    /* Setup the layout state variables */
    ctk_object->snap_strength = DEFAULT_SNAP_STRENGTH;
    ctk_object->first_selected_display = NULL;
    ctk_object->first_selected_screen = NULL;


    /* Make the drawing area */
    tmp = gtk_drawing_area_new();
    gtk_widget_add_events(tmp,
                          GDK_EXPOSURE_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_POINTER_MOTION_HINT_MASK);

    g_signal_connect (G_OBJECT (tmp), "expose_event",  
                      G_CALLBACK (expose_event_callback),
                      (gpointer)(ctk_object));

    g_signal_connect (G_OBJECT (tmp), "configure_event",  
                      G_CALLBACK (configure_event_callback),
                      (gpointer)(ctk_object));

    g_signal_connect (G_OBJECT (tmp), "motion_notify_event",  
                      G_CALLBACK (motion_event_callback),
                      (gpointer)(ctk_object));

    g_signal_connect (G_OBJECT (tmp), "button_press_event",  
                      G_CALLBACK (button_press_event_callback),
                      (gpointer)(ctk_object));

    g_signal_connect (G_OBJECT (tmp), "button_release_event",  
                      G_CALLBACK (button_release_event_callback),
                      (gpointer)(ctk_object));

    GTK_WIDGET_SET_FLAGS(tmp, GTK_DOUBLE_BUFFERED);

    ctk_object->drawing_area = tmp;
    gtk_widget_set_size_request(tmp, width, height);


    /* Set container properties of the object */
    gtk_box_set_spacing(GTK_BOX(ctk_object), 0);

    ctk_object->tooltip_area  = gtk_event_box_new();
    ctk_object->tooltip_group = gtk_tooltips_new();

    gtk_tooltips_enable(ctk_object->tooltip_group);
    gtk_tooltips_set_tip(ctk_object->tooltip_group,
                         ctk_object->tooltip_area,
                         "*** No Display ***", NULL);

    gtk_container_add(GTK_CONTAINER(ctk_object->tooltip_area), tmp);
    gtk_box_pack_start(GTK_BOX(object), ctk_object->tooltip_area,
                       TRUE, TRUE, 0);

    return GTK_WIDGET(ctk_object);

} /* ctk_display_layout_new() */



/** get_widget_fg_gc() ***********************************************
 *
 * Returns the foreground graphics context of the given widget.  If
 * this function returns NULL, then drawing on this widget is not
 * currently possible and should be avoided.
 *
 **/

static GdkGC *get_widget_fg_gc(GtkWidget *widget)
{
    GtkStyle *style = gtk_widget_get_style(widget);

    if (!style) return NULL;

    return style->fg_gc[GTK_WIDGET_STATE(widget)];

} /* get_widget_fg_gc() */



/** draw_rect() ******************************************************
 *
 * Draws a solid or wireframe rectangle to scale of the given color
 * in the given widget.
 *
 **/

static void draw_rect(CtkDisplayLayout *ctk_object,
                      int *dim,
                      GdkColor *color,
                      int fill)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);

    int w = (int)(ctk_object->scale * (dim[X] + dim[W])) - (int)(ctk_object->scale * (dim[X]));
    int h = (int)(ctk_object->scale * (dim[Y] + dim[H])) - (int)(ctk_object->scale * (dim[Y]));

    /* Setup color to use */
    gdk_gc_set_rgb_fg_color(fg_gc, color);

    /* Draw the rectangle */
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       fill,
                       ctk_object->img_dim[X] + ctk_object->scale * dim[X],
                       ctk_object->img_dim[Y] + ctk_object->scale * dim[Y],
                       w,
                       h);

} /* draw_rect() */



/** draw_rect_strs() *************************************************
 *
 * Draws possibly 2 rows of text in the middle of a bounding,
 * scaled rectangle.  If the text does not fit, it is not drawn.
 *
 **/

static void draw_rect_strs(CtkDisplayLayout *ctk_object,
                           int *dim,
                           GdkColor *color,
                           const char *str_1,
                           const char *str_2)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    char *str;

    int txt_w;
    int txt_h;
    int txt_x, txt_x1, txt_x2;
    int txt_y, txt_y1, txt_y2;

    int draw_1 = 0;
    int draw_2 = 0;

    if (str_1) {
        pango_layout_set_text(ctk_object->pango_layout, str_1, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        if (txt_w +8 <= ctk_object->scale * dim[W] &&
            txt_h +8 <= ctk_object->scale * dim[H]) {
            draw_1 = 1;
        }
    }

    if (str_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_2, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        if (txt_w +8 <= ctk_object->scale * dim[W] &&
            txt_h +8 <= ctk_object->scale * dim[H]) {
            draw_2 = 1;
        }

        str = g_strconcat(str_1, "\n", str_2, NULL);

        pango_layout_set_text(ctk_object->pango_layout, str, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        if (draw_1 && draw_2 &&
            txt_h +8 > ctk_object->scale * dim[H]) {
            draw_2 = 0;
        }

        g_free(str);
    }

    if (draw_1 && !draw_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_1, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x1 = ctk_object->scale*(dim[X] + dim[W] / 2) - (txt_w / 2);
        txt_y1 = ctk_object->scale*(dim[Y] + dim[H] / 2) - (txt_h / 2);

        /* Write name */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim[X] + txt_x1,
                        ctk_object->img_dim[Y] + txt_y1,
                        ctk_object->pango_layout);
    }

    else if (!draw_1 && draw_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_2, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x2 = ctk_object->scale*(dim[X] + dim[W] / 2) - (txt_w / 2);
        txt_y2 = ctk_object->scale*(dim[Y] + dim[H] / 2) - (txt_h / 2);

        /* Write dimensions */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim[X] + txt_x2,
                        ctk_object->img_dim[Y] + txt_y2,
                        ctk_object->pango_layout);
    }

    else if (draw_1 && draw_2) {
        str = g_strconcat(str_1, "\n", str_2, NULL);

        pango_layout_set_text(ctk_object->pango_layout, str, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x = ctk_object->scale*(dim[X] + dim[W] / 2) - (txt_w / 2);
        txt_y = ctk_object->scale*(dim[Y] + dim[H] / 2) - (txt_h / 2);

        /* Write both */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim[X] + txt_x,
                        ctk_object->img_dim[Y] + txt_y,
                        ctk_object->pango_layout);

        g_free(str);
    }

} /* draw_rect_strs() */



/** draw_display() ***************************************************
 *
 * Draws a display to scale within the layout.
 *
 **/

static void draw_display(CtkDisplayLayout *ctk_object,
                         nvDisplayPtr display)
{
    nvModePtr mode;
    int base_color_idx;
    int color_idx;
    char *tmp_str;

    if (!display || !(display->cur_mode)) {
        return;
    }

    mode = display->cur_mode;
    base_color_idx =
        NUM_COLORS_PER_PALETTE * NvCtrlGetTargetId(display->gpu->handle);


    /* Draw panning */
    color_idx = base_color_idx + ((mode->modeline) ? BG_PAN_ON : BG_PAN_OFF);
    draw_rect(ctk_object, mode->pan, &(ctk_object->color_palettes[color_idx]),
              1);
    draw_rect(ctk_object, mode->pan, &(ctk_object->fg_color), 0);


    /* Draw viewport */
    color_idx = base_color_idx + ((mode->modeline) ? BG_SCR_ON : BG_SCR_OFF);
    draw_rect(ctk_object, mode->dim, &(ctk_object->color_palettes[color_idx]),
              1);
    draw_rect(ctk_object, mode->dim, &(ctk_object->fg_color), 0);


    /* Draw text information */
    if (!mode->display->screen) {
        tmp_str = g_strdup("(Disabled)");
    } else if (mode->modeline) {
        tmp_str = g_strdup_printf("%dx%d", mode->dim[W], mode->dim[H]);
    } else {
        tmp_str = g_strdup("(Off)");
    }
    draw_rect_strs(ctk_object,
                   mode->dim,
                   &(ctk_object->fg_color),
                   display->logName,
                   tmp_str);
    g_free(tmp_str);

} /* draw_display() */



/** draw_screen() ****************************************************
 *
 * Draws a screen to scale within the layout.
 *
 **/

static void draw_screen(CtkDisplayLayout *ctk_object,
                        nvScreenPtr screen)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);

    int *sdim; /* Screen dimensions */
    GdkColor bg_color; /* Background color */
    GdkColor bd_color; /* Border color */
    char *tmp_str;


    if (!screen)  return;


    /* Draw the screen effective size */
    gdk_color_parse("#888888", &bg_color);
    gdk_color_parse("#777777", &bd_color);

    sdim = get_screen_dim(screen, 1);

    /* Draw the screen background */
    draw_rect(ctk_object, sdim, &bg_color, 1);

    /* Draw the screen border with dashed lines */
    gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_ON_OFF_DASH,
                               GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);

    draw_rect(ctk_object, sdim, &(ctk_object->fg_color), 0);
    
    gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_SOLID,
                               GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);

    /* Show the name of the screen if no-scanout is selected */
    if (screen->no_scanout) {
        tmp_str = g_strdup_printf("X Screen %d", screen->scrnum);
        
        draw_rect_strs(ctk_object,
                       screen->dim,
                       &(ctk_object->fg_color),
                       tmp_str,
                       "(No Scanout)");
        g_free(tmp_str);
    }

} /* draw_screen() */



/** draw_layout() ****************************************************
 *
 * Draws a layout.
 *
 **/

static void draw_layout(CtkDisplayLayout *ctk_object)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);

    GdkColor bg_color; /* Background color */
    GdkColor bd_color; /* Border color */
    int i;


    gdk_color_parse("#888888", &bg_color);
    gdk_color_parse("#777777", &bd_color);

    /* Draw the Z-order back to front */
    for (i = ctk_object->Zcount - 1; i >= 0; i--) {
        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            draw_display(ctk_object, ctk_object->Zorder[i].u.display);
        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            draw_screen(ctk_object, ctk_object->Zorder[i].u.screen);
        }
    }

    /* Hilite the selected item */
    if (ctk_object->selected_display ||
        ctk_object->selected_screen) {

        int w, h;
        int size; /* Hilite line size */
        int offset; /* Hilite box offset */
        int *dim;

        if (ctk_object->selected_display) {
            dim = ctk_object->selected_display->cur_mode->dim;
        } else {
            dim = get_screen_dim(ctk_object->selected_screen, 0);
        }

        /* Draw red selection border */
        w  = (int)(ctk_object->scale * (dim[X] + dim[W])) - (int)(ctk_object->scale * (dim[X]));
        h  = (int)(ctk_object->scale * (dim[Y] + dim[H])) - (int)(ctk_object->scale * (dim[Y]));

        /* Setup color to use */
        gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->select_color));

        /* If dislay is too small, just color the whole thing */
        size = 3;
        offset = (size/2) +1;

        if ((w -(2* offset) < 0) || (h -(2 *offset) < 0)) {
            draw_rect(ctk_object, dim, &(ctk_object->select_color), 1);
            draw_rect(ctk_object, dim, &(ctk_object->fg_color), 0);

        } else {
            gdk_gc_set_line_attributes(fg_gc, size, GDK_LINE_SOLID,
                                       GDK_CAP_ROUND, GDK_JOIN_ROUND);

            gdk_draw_rectangle(ctk_object->pixmap,
                               fg_gc,
                               0,
                               ctk_object->img_dim[X] +(ctk_object->scale * dim[X]) +offset,
                               ctk_object->img_dim[Y] +(ctk_object->scale * dim[Y]) +offset,
                               w -(2 * offset),
                               h -(2 * offset));

            gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_SOLID, GDK_CAP_ROUND,
                                       GDK_JOIN_ROUND);
        }


        /* Uncomment to show bounding box of selected screen's metamodes */
        /*
        if (ctk_object->selected_screen) {

            // Shows the screen dimensions used to write to the X config file
            gdk_color_parse("#00FF00", &bg_color);
            draw_rect(ctk_object,
                      ctk_object->selected_screen->dim,
                      &(bg_color), 0);

            if (ctk_object->selected_screen->cur_metamode) {
                // Shows the effective screen dimensions used in conjunction
                // with display devices that are "off"
                gdk_color_parse("#0000FF", &bg_color);
                draw_rect(ctk_object,
                          ctk_object->selected_screen->cur_metamode->dim,
                          &(bg_color), 0);

                // Shows the current screen dimensions used for relative
                // positioning of the screen (w/o displays that are "off")
                gdk_color_parse("#FF00FF", &bg_color);
                draw_rect(ctk_object,
                          ctk_object->selected_screen->cur_metamode->edim,
                          &(bg_color), 0);
            }
        }
        //*/

        /* Uncomment to show unsnapped dimensions */
        /*
        {
            gdk_color_parse("#DD4444", &bg_color);
            if (ctk_object->modify_info.modify_dirty) {
                gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_ON_OFF_DASH,
                                           GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);
            }
            draw_rect(ctk_object, ctk_object->modify_info.modify_dim, &bg_color, 0);
            if (ctk_object->modify_info.modify_dirty) {
                gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_SOLID,
                                           GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);
            }
        }
        //*/
    }

} /* draw_layout() */



/** clear_layout() ***************************************************
 *
 * Clears the layout.
 *
 **/

static void clear_layout(CtkDisplayLayout *ctk_object)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GtkAllocation *allocation = &(drawing_area->allocation);
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkColor color;


    /* Clear to background color */
    gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->bg_color));

    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       TRUE,
                       2,
                       2,
                       allocation->width  -4,
                       allocation->height -4);


    /* Add white trim */
    gdk_color_parse("white", &color);
    gdk_gc_set_rgb_fg_color(fg_gc, &color);
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       FALSE,
                       1,
                       1,
                       allocation->width  -3,
                       allocation->height -3);


    /* Add layout border */
    gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->fg_color));
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       FALSE,
                       0,
                       0,
                       allocation->width  -1,
                       allocation->height -1);

} /* clear_layout() */



/** ctk_display_layout_update() **************************************
 *
 * Causes a recalculation of the layout.
 *
 **/

void ctk_display_layout_update(CtkDisplayLayout *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;

    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);
    ctk_object->modify_info.modify_dirty = 1;

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_update() */



/** ctk_display_layout_set_layout() **********************************
 *
 * Configures the display layout widget to show the given layout.
 *
 **/

void ctk_display_layout_set_layout(CtkDisplayLayout *ctk_object,
                                   nvLayoutPtr layout)
{
    /* Setup for the new layout */
    ctk_object->layout = layout;
    zorder_layout(ctk_object);
    select_default_item(ctk_object);

    /* Update */
    ctk_display_layout_update(ctk_object);

} /* ctk_display_layout_set_layout() */



/** ctk_display_layout_update_zorder() *******************************
 *
 * Updates the layout by re-building the Z-order list.
 *
 **/

void ctk_display_layout_update_zorder(CtkDisplayLayout *ctk_object)
{
    zorder_layout(ctk_object);

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_update_zorder() */



/** ctk_display_layout_get_selected_display() ************************
 *
 * Returns the selected display.
 *
 **/

nvDisplayPtr ctk_display_layout_get_selected_display(CtkDisplayLayout *ctk_object)
{
    return ctk_object->selected_display;

} /* ctk_display_layout_get_selected_display() */



/** ctk_display_layout_get_selected_screen() *************************
 *
 * Returns the selected screen.
 *
 **/

nvScreenPtr ctk_display_layout_get_selected_screen(CtkDisplayLayout *ctk_object)
{
    return ctk_object->selected_screen;

} /* ctk_display_layout_get_selected_screen() */



/** ctk_display_layout_get_selected_gpu() ****************************
 *
 * Returns the selected gpu.
 *
 **/

nvGpuPtr ctk_display_layout_get_selected_gpu(CtkDisplayLayout *ctk_object)
{
    if (ctk_object->selected_display) {
        return ctk_object->selected_display->gpu;
    }

    if (ctk_object->selected_screen) {
        return ctk_object->selected_screen->gpu;
    }

    return NULL;

} /* ctk_display_layout_get_selected_gpu() */



/** ctk_display_layout_set_screen_metamode() *************************
 *
 * Sets which metamode the screen should be use.
 *
 **/

void ctk_display_layout_set_screen_metamode(CtkDisplayLayout *ctk_object,
                                            nvScreenPtr screen,
                                            int new_metamode_idx)
{
    if (!screen) return;

    /* Make sure the metamode exists */
    if (new_metamode_idx < 0) {
        new_metamode_idx = 0;
    } else if (new_metamode_idx >= screen->num_metamodes) {
        new_metamode_idx = screen->num_metamodes -1;
    }

    /* Select the new metamode and recalculate layout dimensions and scaling */
    set_screen_metamode(ctk_object->layout, screen, new_metamode_idx);
    recenter_layout(ctk_object->layout);
    sync_scaling(ctk_object);
    ctk_object->modify_info.modify_dirty = 1;

    /* Update the layout */
    ctk_display_layout_update(ctk_object);

} /* ctk_display_layout_set_screen_metamode() */



/** ctk_display_layout_add_screen_metamode() *************************
 *
 * Adds a new metamode to the screen.
 *
 **/

void ctk_display_layout_add_screen_metamode(CtkDisplayLayout *ctk_object,
                                            nvScreenPtr screen)
{
    nvDisplayPtr display;
    nvMetaModePtr metamode;


    if (!screen || !screen->gpu) return;


    /* Add a metamode to the screen */
    metamode = (nvMetaModePtr)calloc(1, sizeof(nvMetaMode));
    if (!metamode) return;

    /* Duplicate the currently selected metamode */
    metamode->id = -1;
    metamode->source = METAMODE_SOURCE_NVCONTROL;

    /* Add the metamode after the currently selected metamode */
    metamode->next = screen->cur_metamode->next;
    screen->cur_metamode->next = metamode;
    screen->num_metamodes++;


    /* Add a mode to each display */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        nvModePtr mode;

        /* Create the mode */
        mode = (nvModePtr)calloc(1, sizeof(nvMode));
        if (!mode) goto fail;

        /* Link the mode to the metamode */
        mode->metamode = metamode;

        /* Duplicate the currently selected mode */
        mode->display = display;
        if (display->cur_mode) {
            mode->modeline = display->cur_mode->modeline;
            mode->dim[X] = display->cur_mode->dim[X];
            mode->dim[Y] = display->cur_mode->dim[Y];
            mode->dim[W] = display->cur_mode->dim[W];
            mode->dim[H] = display->cur_mode->dim[H];
            mode->pan[X] = display->cur_mode->pan[X];
            mode->pan[Y] = display->cur_mode->pan[Y];
            mode->pan[W] = display->cur_mode->pan[W];
            mode->pan[H] = display->cur_mode->pan[H];
            mode->position_type = display->cur_mode->position_type;
            mode->relative_to = display->cur_mode->relative_to;
        }

        /* Add the mode after the currently selected mode */
        mode->next = display->cur_mode->next;
        display->cur_mode->next = mode;
        display->num_modes++;
    }

    /* Select the newly created metamode */
    ctk_display_layout_set_screen_metamode(ctk_object,
                                           screen,
                                           (screen->cur_metamode_idx+1));
    return;

 fail:
    /* XXX Need to bail better:
     * - Remove metamode from screen
     * - Remove any excess metamodes from the displays
     */
    queue_layout_redraw(ctk_object);
    return;

} /* ctk_display_layout_add_screen_metamode() */



/** ctk_display_layout_delete_screen_metamode() **********************
 *
 * Deletes a metamode from the screen (also deletes corresponding
 * modes from the screen's displays.)
 *
 **/

void ctk_display_layout_delete_screen_metamode(CtkDisplayLayout *ctk_object,
                                               nvScreenPtr screen,
                                               int metamode_idx,
                                               Bool reselect)
{
    nvDisplayPtr display;
    nvMetaModePtr metamode;
    nvMetaModePtr metamode_prev;
    nvModePtr mode ;
    nvModePtr mode_prev;
    int i;


    if (!screen || !screen->gpu || metamode_idx >= screen->num_metamodes) {
        return;
    }


    /* Don't delete the last metamode */
    if (screen->num_metamodes <= 1) {
        return;
    }


    /* Find the metamode */
    metamode_prev = NULL;
    metamode = screen->metamodes;
    i = 0;
    while (metamode && i < metamode_idx) {
        metamode_prev = metamode;
        metamode = metamode->next;
        i++;
    }


    /* Remove the metamode from the list */
    if (!metamode_prev) {
        screen->metamodes = screen->metamodes->next;
    } else {
        metamode_prev->next = metamode->next;
    }
    screen->num_metamodes--;

    if (screen->cur_metamode == metamode) {
        screen->cur_metamode = metamode->next;
    }
    if (screen->cur_metamode_idx >= screen->num_metamodes) {
        screen->cur_metamode_idx = screen->num_metamodes -1;
    }

    free(metamode);


    /* Delete the mode from each display in the screen */
    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        /* Find the mode */
        mode_prev = NULL;
        mode = display->modes;
        for (i = 0; i != metamode_idx; i++) {
            mode_prev = mode;
            mode = mode->next;
        }

        /* Remove the mode from the list */
        if (!mode_prev) {
            display->modes = display->modes->next;
        } else {
            mode_prev->next = mode->next;
        }
        display->num_modes--;

        if (display->cur_mode == mode) {
            display->cur_mode = mode->next;
        }

        /* Delete the mode */
        free(mode);
    }


    /* Update which metamode should be selected */
    if (reselect) {
        ctk_display_layout_set_screen_metamode
            (ctk_object, screen, screen->cur_metamode_idx);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_delete_screen_metamode() */



/** ctk_display_layout_disable_display() *****************************
 *
 * Disables a display (removes it from its X screen.
 *
 **/

void ctk_display_layout_disable_display(CtkDisplayLayout *ctk_object,
                                        nvDisplayPtr display)
{
    nvScreenPtr screen = display->screen;

    /* Remove display from the X screen */
    screen_remove_display(display);

    /* If the screen is empty, remove it */
    if (!screen->num_displays) {
        layout_remove_and_free_screen(screen);

        /* Unselect the screen if it was selected */
        if (screen == ctk_object->first_selected_screen) {
            ctk_object->first_selected_screen = NULL;
        }
        if (screen == ctk_object->selected_screen) {
            ctk_object->selected_screen = NULL;
        }

        /* Make sure screen numbers are consistent */
        renumber_xscreens(ctk_object->layout);
    }

    /* Add the fake mode to the display */
    gpu_add_screenless_modes_to_displays(display->gpu);

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_disable_display() */



/** ctk_display_layout_set_mode_modeline() ***************************
 *
 * Sets which modeline the mode should use.
 *
 **/

void ctk_display_layout_set_mode_modeline(CtkDisplayLayout *ctk_object,
                                          nvModePtr mode,
                                          nvModeLinePtr modeline)
{
    nvModeLinePtr old_modeline;

    if (!mode) {
        return;
    }

    /* Set the new modeline */
    old_modeline = mode->modeline;
    mode->modeline = modeline;


    /* Setup the mode's dimensions based on the modeline */
    if (modeline) {
        mode->dim[W] = modeline->data.hdisplay;
        mode->dim[H] = modeline->data.vdisplay;

        if (mode->pan[W] < modeline->data.hdisplay) {
            mode->pan[W] = modeline->data.hdisplay;
        }
        if (mode->pan[H] < modeline->data.vdisplay) {
            mode->pan[H] = modeline->data.vdisplay;
        }

        /* If the old modeline did not have panning dimensions */
        if (!old_modeline ||
            (mode->pan[W] == old_modeline->data.hdisplay)) {
            mode->pan[W] = modeline->data.hdisplay;
        }
        if (!old_modeline ||
            (mode->pan[H] == old_modeline->data.vdisplay)) {
            mode->pan[H] = modeline->data.vdisplay;
        }

    } else if (mode->display) {
        /* Display is being turned off, set the default width/height */
        mode->dim[W] = mode->display->modelines->data.hdisplay;
        mode->dim[H] = mode->display->modelines->data.vdisplay;
        mode->pan[W] = mode->display->modelines->data.hdisplay;
        mode->pan[H] = mode->display->modelines->data.vdisplay;
    }

    /* In advanced mode, changing the resolution a display uses for a
     * particular metamode should make this metamode non-implicit.
     */
    if (ctk_object->advanced_mode &&
        (old_modeline != modeline) &&
        mode->metamode) {
        mode->metamode->source = METAMODE_SOURCE_NVCONTROL;
    }


    /* Update the layout */
    ctk_display_layout_update(ctk_object);

} /* ctk_display_layout_set_display_modeline() */



/** ctk_display_layout_set_display_position() ************************
 *
 * Sets the absolute/relative position of the display.
 *
 **/

void ctk_display_layout_set_display_position(CtkDisplayLayout *ctk_object,
                                             nvDisplayPtr display,
                                             int position_type,
                                             nvDisplayPtr relative_to,
                                             int x, int y)
{
    int modified = 0;
    int resolve_all_modes = !ctk_object->advanced_mode;


    if (!display) return;

    if (position_type != CONF_ADJ_ABSOLUTE && !relative_to) return;

    /* XXX When configuring a relative position, make sure
     * all displays that are relative to us become absolute.
     * This is to avoid relationship loops.  Eventually, we'll want
     * to be able to handle weird loops since X does this.
     */

    if (position_type != CONF_ADJ_ABSOLUTE) {

        nvDisplayPtr other;
        nvModePtr mode;

        for (other = display->screen->displays;
             other;
             other = other->next_in_screen) {

            if (!resolve_all_modes) {
                mode = other->cur_mode;
                if (mode && mode->relative_to == display) {
                    mode->position_type = CONF_ADJ_ABSOLUTE;
                    mode->relative_to = NULL;
                }

            } else {

                for (mode = other->modes; mode; mode = mode->next) {
                    if (mode->relative_to == display) {
                        mode->position_type = CONF_ADJ_ABSOLUTE;
                        mode->relative_to = NULL;
                    }
                }
            }
        }
    }


    /* Set the new positioning type */
    if (!resolve_all_modes) {
        display->cur_mode->position_type = position_type;
        display->cur_mode->relative_to = relative_to;

    } else {
        nvModePtr mode;

        for (mode = display->modes; mode; mode = mode->next) {
            mode->position_type = position_type;
            mode->relative_to = relative_to;
        }
    }


    switch (position_type) {
    case CONF_ADJ_ABSOLUTE:
        /* Do the move by offsetting */
        ctk_object->modify_info.modify_dirty = 1;
        modified = move_selected(ctk_object,
                                 x - display->cur_mode->dim[X],
                                 y - display->cur_mode->dim[Y],
                                 0);

        /* Report back result of move */
        if (ctk_object->modified_callback &&
            (modified ||
             x != display->cur_mode->dim[X] ||
             y != display->cur_mode->dim[Y])) {
            ctk_object->modified_callback
                (ctk_object->layout, ctk_object->modified_callback_data);
        }
        break;

    default:
        /* Make sure the screen position does not change */
        reposition_screen(display->screen, resolve_all_modes);

        /* Recalculate the layout */
        ctk_display_layout_update(ctk_object);
        break;
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_set_display_position() */



/** ctk_display_layout_set_display_panning() *************************
 *
 * Sets the panning domain of the display.
 *
 **/

void ctk_display_layout_set_display_panning(CtkDisplayLayout *ctk_object,
                                            nvDisplayPtr display,
                                            int width, int height)
{
    int modified = 0;


    if (!display) return;

    /* Change the panning */
    ctk_object->modify_info.modify_dirty = 1;
    modified = pan_selected(ctk_object,
                            width  - display->cur_mode->pan[W],
                            height - display->cur_mode->pan[H],
                            0);

    /* Report back result of move */
    if (ctk_object->modified_callback &&
        (modified ||
         width  != display->cur_mode->pan[W] ||
         height != display->cur_mode->pan[H])) {
        ctk_object->modified_callback(ctk_object->layout,
                                      ctk_object->modified_callback_data);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_set_display_panning() */



/** select_topmost_item() **************************************
 *
 * Select top item from Z order list.
 *
 **/

static void select_topmost_item(CtkDisplayLayout *ctk_object)
{
    if (ctk_object->Zorder[0].type == ZNODE_TYPE_DISPLAY) {
        select_display(ctk_object, ctk_object->Zorder[0].u.display);
    } else if (ctk_object->Zorder[0].type == ZNODE_TYPE_SCREEN) {
        select_screen(ctk_object, ctk_object->Zorder[0].u.screen);
    }
} /* select_topmost_item() */



/** ctk_display_layout_select_display() ***********************
 *
 * Updates the currently selected display.
 *
 **/

void ctk_display_layout_select_display(CtkDisplayLayout *ctk_object,
                                       nvDisplayPtr display)
{
    /* Select the new display */
    select_display(ctk_object, display);

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_select_display() */



/** ctk_display_layout_select_screen() ************************
 *
 * Makes the given screen the thing that is selected.
 *
 **/

void ctk_display_layout_select_screen(CtkDisplayLayout *ctk_object,
                                      nvScreenPtr screen)
{
    /* Select the new display */
    ctk_object->selected_display = NULL;
    select_screen(ctk_object, screen);

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_select_screen() */



/** ctk_display_layout_update_display_count() ************************
 *
 * Updates the number of displays shown in the layout by re-building
 * the Z-order list.
 *
 **/

void ctk_display_layout_update_display_count(CtkDisplayLayout *ctk_object,
                                             nvDisplayPtr display)
{
    /* Update the Z order */
    zorder_layout(ctk_object);

    /* Select the previously selected display */
    if (display) {
        ctk_display_layout_select_display(ctk_object, display);
    } else {
        select_topmost_item(ctk_object);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_update_display_count() */



/** ctk_display_layout_set_screen_virtual_size() *********************
 *
 * Sets the virtual size of the screen
 *
 **/

void ctk_display_layout_set_screen_virtual_size(CtkDisplayLayout *ctk_object,
                                                nvScreenPtr screen,
                                                int width, int height)
{
    int modified;

    if (!screen || !screen->no_scanout) return;

    /* Do the panning by offsetting */

    // XXX May want to pan non-selected screen,
    //     though right now this just works out
    //     since what we want to pan is always
    //     what is selected.
    ctk_object->modify_info.modify_dirty = 1;
    modified = pan_selected(ctk_object,
                            width - screen->dim[W],
                            height - screen->dim[H],
                            0);

    if (ctk_object->modified_callback &&
        (modified ||
         width != screen->dim[W] ||
         height != screen->dim[H])) {
        ctk_object->modified_callback(ctk_object->layout,
                                      ctk_object->modified_callback_data);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_set_screen_virtual_size() */



/** ctk_display_layout_set_screen_depth() ****************************
 *
 * Sets which modeline the screen should use.
 *
 **/

void ctk_display_layout_set_screen_depth(CtkDisplayLayout *ctk_object,
                                         nvScreenPtr screen,
                                         int depth)
{
    /* Setup screen's default depth */
    if (screen) {
        screen->depth = depth;
    }

} /* ctk_display_layout_set_screen_depth() */



/** ctk_display_layout_set_screen_position() *************************
 *
 * Sets the absolute/relative position of the screen.
 *
 **/

void ctk_display_layout_set_screen_position(CtkDisplayLayout *ctk_object,
                                            nvScreenPtr screen,
                                            int position_type,
                                            nvScreenPtr relative_to,
                                            int x, int y)
{
    int modified = 0;
    nvLayoutPtr layout = ctk_object->layout;


    if (!screen) return;

    if (position_type != CONF_ADJ_ABSOLUTE && !relative_to) return;


    /* XXX When configuring a relative position, make sure
     * all screens that are relative to us become absolute.
     * This is to avoid relationship loops.  Eventually, we'll want
     * to be able to handle weird loops since X does this.
     */

    if (position_type != CONF_ADJ_ABSOLUTE) {

        nvScreenPtr other;

        for (other = layout->screens;
             other;
             other = other->next_in_layout) {

            if (other->relative_to == screen) {
                switch_screen_to_absolute(other);
            }
        }
    }


    /* Set the new positioning type */
    switch (position_type) {
    case CONF_ADJ_ABSOLUTE:
        {
            nvDisplayPtr other;
            int x_offset = x - screen->dim[X];
            int y_offset = y - screen->dim[Y];
            int *sdim;

            /* Make sure this screen use absolute positioning */
            switch_screen_to_absolute(screen);

            /* Do the move by offsetting */
            offset_screen(screen, x_offset, y_offset);
            for (other = screen->displays;
                 other;
                 other = other->next_in_screen) {
                offset_display(other, x_offset, y_offset);
            }

            /* Recalculate the layout */
            ctk_display_layout_update(ctk_object);

            /* Report back result of move */
            sdim = get_screen_dim(screen, 1);
            if (x != sdim[X] || y != sdim[Y]) {
                modified = 1;
            }

            if (ctk_object->modified_callback && modified) {
                ctk_object->modified_callback
                    (ctk_object->layout, ctk_object->modified_callback_data);
            }
        }
        break;

    case CONF_ADJ_RELATIVE:
        screen->x_offset = x;
        screen->y_offset = y;

        /* Fall Through */
    default:
        /* Set the desired positioning */
        screen->relative_to = relative_to;
        screen->position_type = position_type;

        /* Other relative positioning */

        /* XXX Need to validate cases where displays are
         *     positioned relative to each other in a
         *     circular setup
         *
         *     eg. CRT-0  left of  CRT-1
         *         CRT-1  clones   CRT-0  <- Shouldn't allow this
         *
         *     also:
         *
         *  CRT-0 left of CRT-1
         *  CRT-1 left of CRT-2
         *  CRT-2 clones  CRT-0 ...  Eep!
         */

        /* Recalculate the layout */
        ctk_display_layout_update(ctk_object);
        break;
    }


    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_set_screen_position() */



/** ctk_display_layout_set_advanced_mode() ***************************
 *
 * Enables/Disables the user's ability to modify advanced layout
 * bells and whisles.
 *
 * In advanced mode the user has access to:
 *
 * - Per-display panning.
 * - Modeline timing modifications. (Add/Delete)
 * - Multiple metamodes. (Add/Delete)
 *
 *
 * In basic mode:
 *
 * - User can only modify the current metamode.
 *
 *
 **/

void ctk_display_layout_set_advanced_mode(CtkDisplayLayout *ctk_object,
                                          int advanced_mode)
{
    ctk_object->advanced_mode = advanced_mode;

} /* ctk_display_layout_set_allow_panning() */



/** ctk_display_layout_register_callbacks() **************************
 *
 * Sets up callbacks so users of the display layout can recieve
 * notifications.
 *
 **/

void ctk_display_layout_register_callbacks(CtkDisplayLayout *ctk_object,
                                           ctk_display_layout_selected_callback selected_callback,
                                           void *selected_callback_data,
                                           ctk_display_layout_modified_callback modified_callback,
                                           void *modified_callback_data)
{
    ctk_object->selected_callback = selected_callback;
    ctk_object->selected_callback_data = selected_callback_data;
    ctk_object->modified_callback = modified_callback;
    ctk_object->modified_callback_data = modified_callback_data;

} /* ctk_display_layout_register_callbacks() */



/** expose_event_callback() ******************************************
 *
 * Handles expose events.
 *
 **/

static gboolean
expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);
    GdkGC *fg_gc = get_widget_fg_gc(widget);
    GdkGCValues old_gc_values;


    if (event->count || !widget->window || !fg_gc) {
        return TRUE;
    }

    /* Redraw the layout */
    gdk_window_begin_paint_rect(widget->window, &event->area);

    gdk_gc_get_values(fg_gc, &old_gc_values);

    clear_layout(ctk_object);

    draw_layout(ctk_object);

    gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);

    gdk_draw_pixmap(widget->window,
                    fg_gc,
                    ctk_object->pixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);

    gdk_window_end_paint(widget->window);

    return TRUE;

} /* expose_event_callback() */



/** configure_event_callback() ***************************************
 *
 * Handles configure events.
 *
 **/

static gboolean
configure_event_callback(GtkWidget *widget, GdkEventConfigure *event,
                         gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);
    int width = widget->allocation.width;
    int height = widget->allocation.height;

    ctk_object->img_dim[X] = LAYOUT_IMG_OFFSET + LAYOUT_IMG_BORDER_PADDING;
    ctk_object->img_dim[Y] = LAYOUT_IMG_OFFSET + LAYOUT_IMG_BORDER_PADDING;
    ctk_object->img_dim[W] = width  -2*(ctk_object->img_dim[X]);
    ctk_object->img_dim[H] = height -2*(ctk_object->img_dim[Y]);

    sync_scaling(ctk_object);

    ctk_object->pixmap = gdk_pixmap_new(widget->window, width, height, -1);

    return TRUE;

} /* configure_event_callback() */



/** motion_event_callback() ******************************************
 *
 * Handles mouse motion events.
 *
 **/

static gboolean
motion_event_callback(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);
    static int init = 1;
    int modify_panning;
    int x;
    int y;
    GdkModifierType state;


    /* Handle hints so we don't get overwhelmed with motion events */
    if (event->is_hint) {
        gdk_window_get_pointer(event->window, &x, &y, &state);
    } else {
        x = event->x;
        y = event->y;
        state = event->state;
    }

    /* Swap between panning and moving  */
    if (ctk_object->advanced_mode && (state & ShiftMask)) {
        modify_panning = 1;
    } else {
        modify_panning = 0;
    }
    if ((ctk_object->modify_info.modify_panning != modify_panning) || init) {
        init = 0;
        ctk_object->modify_info.modify_dirty = 1;
    }

    /* Nothing to do if mouse didn't move */
    if (ctk_object->last_mouse_x == x &&
        ctk_object->last_mouse_y == y) {
        return TRUE;
    }

    ctk_object->mouse_x = x;
    ctk_object->mouse_y = y;


    /* If mouse moved, allow user to reselect the current display/screen */
    ctk_object->first_selected_display = NULL;
    ctk_object->first_selected_screen = NULL;


    /* Modify screen layout */
    if (ctk_object->button1 && !ctk_object->clicked_outside) {
        int modified = 0;
        int delta_x =
            (x - ctk_object->last_mouse_x) / ctk_object->scale;
        int delta_y =
            (y - ctk_object->last_mouse_y) / ctk_object->scale;

        if (!modify_panning) {
            modified = move_selected(ctk_object, delta_x, delta_y, 1);
        } else {
            modified = pan_selected(ctk_object, delta_x, delta_y, 1);
        }

        if (modified) {
            GtkWidget *drawing_area = ctk_object->drawing_area;

            if (ctk_object->modified_callback) {
                ctk_object->modified_callback(ctk_object->layout,
                                              ctk_object->modified_callback_data);
            }

            /* Queue and process expose event so we redraw ASAP */
            queue_layout_redraw(ctk_object);
            gdk_window_process_updates(drawing_area->window, TRUE);
        }

    /* Update the tooltip under the mouse */
    } else {
        char *tip = get_tooltip_under_mouse(ctk_object, x, y);
        if (tip) {
            gtk_tooltips_set_tip(ctk_object->tooltip_group,
                                 ctk_object->tooltip_area,
                                 tip, NULL);
            gtk_tooltips_force_window(ctk_object->tooltip_group);
            g_free(tip);
        }
    }

    ctk_object->last_mouse_x = x;
    ctk_object->last_mouse_y = y;

    return TRUE;

} /* motion_event_callback() */



/** button_press_event_callback() ************************************
 *
 * Handles mouse button press events.
 *
 **/

static gboolean
button_press_event_callback(GtkWidget *widget, GdkEventButton *event,
                            gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);

    /* Scale and offset x & y so they reside in the clickable area */
    int x = (event->x -ctk_object->img_dim[X]) / ctk_object->scale;
    int y = (event->y -ctk_object->img_dim[Y]) / ctk_object->scale;

    GdkEvent *next_event;


    ctk_object->last_mouse_x = event->x;
    ctk_object->last_mouse_y = event->y;


    /* Check to see if a double click event is pending
     * and ignore this click if that is the case.
     */
    next_event = gdk_event_peek();
    if (next_event) {
        if (next_event->type == GDK_2BUTTON_PRESS) {
            /* Double click event detected, ignore the
             * preceding GDK_BUTTON_PRESS
             */
            return TRUE;
        }
    }

    /* Handle double clicks */
    if (event->type == GDK_2BUTTON_PRESS) {
        /* XXX Flash the display or screen */
        return TRUE;
    }

    /* XXX Ignore tripple clicks */
    if (event->type != GDK_BUTTON_PRESS) return TRUE;

    switch (event->button) {

    /* Handle selection of displays/X screens */
    case Button1:
        ctk_object->button1 = 1;
        click_layout(ctk_object, x, y);

        /* Report back selection event */
        if (ctk_object->selected_callback) {
            ctk_object->selected_callback(ctk_object->layout,
                                          ctk_object->selected_callback_data);
        }

        queue_layout_redraw(ctk_object);
        break;

    default:
        break;
    }

    return TRUE;

} /* button_press_event_callback() */



/** button_release_event_callback() **********************************
 *
 * Handles mouse button release events.
 *
 **/

static gboolean
button_release_event_callback(GtkWidget *widget, GdkEventButton *event,
                              gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);


    switch (event->button) {

    case Button1:
        ctk_object->button1 = 0;
        break;

    case Button2:
        ctk_object->button2 = 0;
        break;

    case Button3:
        ctk_object->button3 = 0;
        break;

    default:
        break;
    }

    return TRUE;

} /* button_release_event_callback() */
