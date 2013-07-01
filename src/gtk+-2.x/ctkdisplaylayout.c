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
#include "ctkutils.h"




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

static Bool sync_layout(CtkDisplayLayout *ctk_object);




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



/** get_screen_rect **************************************************
 *
 * Returns the dimension array to use as the screen's dimensions.
 *
 **/

static GdkRectangle *get_screen_rect(nvScreenPtr screen, Bool edim)
{
    if (!screen) return NULL;

    if (screen->no_scanout || !screen->cur_metamode) {
        return &(screen->dim);
    }

    return edim ? &(screen->cur_metamode->edim) : &(screen->cur_metamode->dim);
}



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
    GdkRectangle *screen_rect;


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
    screen_rect = get_screen_rect(info->screen, 0);
    info->orig_screen_dim = *(screen_rect);


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
        info->target_dim = &(info->display->cur_mode->pan);
    } else {
        info->target_position_type = &(info->screen->position_type);
        info->target_dim = screen_rect;
    }
    info->orig_position_type = *(info->target_position_type);
    info->orig_dim = *(info->target_dim);

    /* Initialize where we moved to */
    info->dst_dim = info->orig_dim;

    /* Initialize snapping */
    info->best_snap_v = ctk_object->snap_strength +1;
    info->best_snap_h = ctk_object->snap_strength +1;


    /* Make sure the modify dim is up to date */
    if (info->modify_dirty) {
        info->modify_dim = info->orig_dim;
        info->modify_dirty = 0;
    }

    return TRUE;

} /* get_modify_info() */



/** sync_scaling() ***************************************************
 *
 * Computes the scaling required to display the layout image.
 *
 **/

static Bool sync_scaling(CtkDisplayLayout *ctk_object)
{
    GdkRectangle *dim = &(ctk_object->layout->dim);
    float wscale;
    float hscale;
    float prev_scale = ctk_object->scale;

    wscale = (float)(ctk_object->img_dim.width) / (float)(dim->width);
    hscale = (float)(ctk_object->img_dim.height) / (float)(dim->height);

    if (wscale * dim->height > ctk_object->img_dim.height) {
        ctk_object->scale = hscale;
    } else {
        ctk_object->scale = wscale;
    }

    return (prev_scale != ctk_object->scale);

} /* sync_scaling() */



/** point_in_dim() ***************************************************
 *
 * Determines if a point lies within the given dimensions
 *
 **/

static int point_in_rect(const GdkRectangle *rect, int x, int y)
{
    if (x > rect->x && x < (rect->x + rect->width) &&
        y > rect->y && y < (rect->y + rect->height)) {
        return 1;
    }

    return 0;
}

static int point_in_display(nvDisplayPtr display, int x, int y)
{
    if (!display->cur_mode) {
        return 0;
    }

    return point_in_rect(&(display->cur_mode->pan), x, y);
}

static int point_in_screen(nvScreenPtr screen, int x, int y)
{
    GdkRectangle *screen_rect = get_screen_rect(screen, 1);

    return point_in_rect(screen_rect, x, y);
}



/** get_point_relative_position() ************************************
 *
 * Returns where the point (x, y) is, relative to the given rectangle
 * as: above, below, left-of, right-of, inside/clones.
 *
 **/

static int get_point_relative_position(GdkRectangle *rect, int x, int y)
{
    float m1, b1;
    float m2, b2;
    float l1, l2;


    /* Point insize dim */
    if (point_in_rect(rect, x, y)) {
        return CONF_ADJ_RELATIVE;
    }

    /* Compute cross lines of dimensions */
    m1 = ((float) rect->height) / ((float) rect->width);
    b1 = ((float) rect->y) - (m1 * ((float) rect->x));

    m2 = -m1;
    b2 = ((float) rect->y) + ((float) rect->height) -
        (m2 * ((float) rect->x));

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

static void offset_mode(nvModePtr mode, int x, int y)
{
    mode->pan.x += x;
    mode->pan.y += y;
}

static void offset_display(nvDisplayPtr display, int x, int y)
{
    nvModePtr mode;
    for (mode = display->modes; mode; mode = mode->next) {
        offset_mode(mode, x, y);
    }
}

static void offset_metamode(nvScreenPtr screen, nvMetaModePtr metamode, int idx,
                            int x, int y)
{
    nvDisplayPtr display;

    metamode->dim.x += x;
    metamode->dim.y += y;
    metamode->edim.x += x;
    metamode->edim.y += y;

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        nvModePtr mode = get_mode(display, idx);
        if (mode) {
            offset_mode(mode, x, y);
        }
    }
}

static void offset_screen(nvScreenPtr screen, int x, int y)
{
    nvMetaModePtr metamode;
    nvDisplayPtr display;

    screen->dim.x += x;
    screen->dim.y += y;

    for (metamode = screen->metamodes; metamode; metamode = metamode->next) {
        metamode->dim.x += x;
        metamode->dim.y += y;
        metamode->edim.x += x;
        metamode->edim.y += y;
    }

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {
        offset_display(display, x, y);
    }
}

/* Offsets the entire layout by offsetting its X screens and display devices */
static void offset_layout(nvLayoutPtr layout, int x, int y)
{
    nvGpuPtr gpu;
    nvScreenPtr screen;
    nvDisplayPtr display;

    layout->dim.x += x;
    layout->dim.y += y;

    /* Offset screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        offset_screen(screen, x, y);
    }

    /* Offset disabled displays */
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->screen) {
                continue;
            }
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
                           GdkRectangle *pos)
{
    nvModePtr mode = get_mode(display, mode_idx);
    GdkRectangle relative_pos;

    if (!mode) return 0;


    /* Set the dimensions */
    pos->width  = mode->pan.width;
    pos->height = mode->pan.height;


    /* Find the position */
    switch (mode->position_type) {
    case CONF_ADJ_ABSOLUTE:
        pos->x = mode->pan.x;
        pos->y = mode->pan.y;
        break;

    case CONF_ADJ_RIGHTOF:
        resolve_display(mode->relative_to, mode_idx, &relative_pos);
        pos->x = relative_pos.x + relative_pos.width;
        pos->y = relative_pos.y;
        break;

    case CONF_ADJ_LEFTOF:
        resolve_display(mode->relative_to, mode_idx, &relative_pos);
        pos->x = relative_pos.x - pos->width;
        pos->y = relative_pos.y;
        break;

    case CONF_ADJ_BELOW:
        resolve_display(mode->relative_to, mode_idx, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y + relative_pos.height;
        break;

    case CONF_ADJ_ABOVE:
        resolve_display(mode->relative_to, mode_idx, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y - pos->height;
        break;

    case CONF_ADJ_RELATIVE: /* Clone */
        resolve_display(mode->relative_to, mode_idx, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y;
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
    GdkRectangle rect;
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
            if (resolve_display(display, mode_idx, &rect)) {
                nvModePtr mode = get_mode(display, mode_idx);
                mode->pan.x = rect.x;
                mode->pan.y = rect.y;
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

static int resolve_screen(nvScreenPtr screen, GdkRectangle *pos)
{
    GdkRectangle *screen_rect = get_screen_rect(screen, 0);
    GdkRectangle relative_pos;

    if (!screen_rect) return 0;


    /* Set the dimensions */
    pos->width  = screen_rect->width;
    pos->height = screen_rect->height;


    /* Find the position */
    switch (screen->position_type) {
    case CONF_ADJ_ABSOLUTE:
        pos->x = screen_rect->x;
        pos->y = screen_rect->y;
        break;

    case CONF_ADJ_RIGHTOF:
        resolve_screen(screen->relative_to, &relative_pos);
        pos->x = relative_pos.x + relative_pos.width;
        pos->y = relative_pos.y;
        break;

    case CONF_ADJ_LEFTOF:
        resolve_screen(screen->relative_to, &relative_pos);
        pos->x = relative_pos.x - pos->width;
        pos->y = relative_pos.y;
        break;

    case CONF_ADJ_BELOW:
        resolve_screen(screen->relative_to, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y + relative_pos.height;
        break;

    case CONF_ADJ_ABOVE:
        resolve_screen(screen->relative_to, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y - pos->height;
        break;

    case CONF_ADJ_RELATIVE: /* Clone */
        resolve_screen(screen->relative_to, &relative_pos);
        pos->x = relative_pos.x;
        pos->y = relative_pos.y;
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
    GdkRectangle pos;
    int x, y;
    GdkRectangle *screen_rect;


    /* Resolve the current screen location */
    if (resolve_screen(screen, &pos)) {

        /* Move the screen and the displays by offsetting */
        screen_rect = get_screen_rect(screen, 0);

        x = pos.x - screen_rect->x;
        y = pos.y - screen_rect->y;

        offset_screen(screen, x, y);
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
    GdkRectangle *dim;  // Bounding box for all modes, including NULL modes.
    GdkRectangle *edim; // Bounding box for non-NULL modes.


    if (!screen || !metamode) {
        return;
    }

    dim = &(metamode->dim);
    edim = &(metamode->edim);

    memset(dim, 0, sizeof(*dim));
    memset(edim, 0, sizeof(*edim));

    for (display = screen->displays;
         display;
         display = display->next_in_screen) {

        /* Get the display's mode that is part of the metamode. */
        for (mode = display->modes; mode; mode = mode->next) {
            if (mode->metamode == metamode) break;
        }
        if (!mode) continue;

        if (init) {
            *dim = mode->pan;
            init = 0;
        } else {
            gdk_rectangle_union(dim, &(mode->pan), dim);
        }

        /* Don't include NULL modes in the effective dimension calculation */
        if (!mode->modeline) continue;

        if (einit) {
            *edim = mode->pan;
            einit = 0;
        } else {
            gdk_rectangle_union(edim, &(mode->pan), edim);
        }
    }
}



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
    GdkRectangle *dim;


    if (!screen || screen->no_scanout) return;

    dim = &(screen->dim);
    metamode = screen->metamodes;

    if (!metamode) {
        memset(dim, 0, sizeof(*dim));
        return;
    }

    /* Init screen dimensions to size of first metamode */
    calc_metamode(screen, metamode);
    *dim = metamode->dim;

    for (metamode = metamode->next;
         metamode;
         metamode = metamode->next) {

        calc_metamode(screen, metamode);
        gdk_rectangle_union(dim, &(metamode->dim), dim);
    }
}



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
    GdkRectangle *dim;
    int x, y;


    if (!layout) return;

    resolve_layout(layout);

    dim = &(layout->dim);
    memset(dim, 0, sizeof(*dim));

    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        GdkRectangle *screen_rect;

        calc_screen(screen);
        screen_rect = get_screen_rect(screen, 0);

        if (init) {
            *dim = *screen_rect;
            init = 0;
            continue;
        }
        gdk_rectangle_union(dim, screen_rect, dim);
    }

    /* Position disabled display devices off to the top right */
    x = dim->x + dim->width;
    y = dim->y;
    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->screen) continue;

            display->cur_mode->pan.x = x;
            display->cur_mode->pan.y = y;

            x += display->cur_mode->pan.width;
            dim->width += display->cur_mode->pan.width;
            dim->height = NV_MAX(dim->height, display->cur_mode->pan.height);
        }
    }
}



/** realign_screen() *************************************************
 *
 * Makes sure that all the top left corners of all the screen's metamodes
 * coincide. This is done by offsetting metamodes back to the screen's
 * bounding box top left corner.
 *
 **/

static Bool realign_screen(nvScreenPtr screen)
{
    nvMetaModePtr metamode;
    int idx;
    Bool modified = FALSE;

    /* Calculate dimensions of screen and all metamodes */
    calc_screen(screen);

    /* Offset metamodes back to screen's top left corner */
    for (metamode = screen->metamodes, idx = 0;
         metamode;
         metamode = metamode->next, idx++) {
        int offset_x = (screen->dim.x - metamode->dim.x);
        int offset_y = (screen->dim.y - metamode->dim.y);

        if (offset_x || offset_y) {
            offset_metamode(screen, metamode, idx, offset_x, offset_y);
            modified = TRUE;
        }
    }

    return modified;
}



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
}



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
    int orig_screen_x = screen->dim.x;
    int orig_screen_y = screen->dim.y;

    /* Resolve new relative positions.  In basic mode,
     * relative position changes apply to all modes of a
     * display so we should resolve all modes (since they
     * were all changed.)
     */
    resolve_displays_in_screen(screen, resolve_all_modes);

    /* Reestablish the screen's original position */
    screen->dim.x = orig_screen_x;
    screen->dim.y = orig_screen_y;
    realign_screen(screen);

} /* reposition_screen() */



/** switch_screen_to_absolute() **************************************
 *
 * Prepare a screen for using absolute positioning.  This is needed
 * since screens using relative positioning may not have all their
 * metamodes's top left corner coincide.  This function makes sure
 * that all metamodes in the screen have the same top left corner by
 * offsetting the modes of metamodes that are offset from the screen's
 * bounding box top left corner.
 *
 **/

static void switch_screen_to_absolute(nvScreenPtr screen)
{
    screen->position_type = CONF_ADJ_ABSOLUTE;
    screen->relative_to   = NULL;

    realign_screen(screen);

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

static void snap_dim_to_dim(GdkRectangle *dst, GdkRectangle *src,
                            GdkRectangle *snap, int snap_strength,
                            int *best_vert, int *best_horz)
{
    int dist;


    /* Snap vertically */
    if (best_vert) {

        /* Snap top side to top side */
        dist = abs(snap->y - src->y);
        if (dist < *best_vert) {
            dst->y = snap->y;
            *best_vert = dist;
        }

        /* Snap top side to bottom side */
        dist = abs((snap->y + snap->height) - src->y);
        if (dist < *best_vert) {
            dst->y = snap->y + snap->height;
            *best_vert = dist;
        }

        /* Snap bottom side to top side */
        dist = abs(snap->y - (src->y + src->height));
        if (dist < *best_vert) {
            dst->y = snap->y - src->height;
            *best_vert = dist;
        }

        /* Snap bottom side to bottom side */
        dist = abs((snap->y + snap->height) - (src->y + src->height));
        if (dist < *best_vert) {
            dst->y = snap->y + snap->height - src->height;
            *best_vert = dist;
        }

        /* Snap midlines */
        if (/* Top of 'src' is above bottom of 'snap' */
            (src->y <= snap->y + snap->height + snap_strength) &&
            /* Bottom of 'src' is below top of 'snap' */
            (src->y + src->height >= snap->y - snap_strength)) {

            /* Snap vertically */
            dist = abs((snap->y + snap->height/2) - (src->y + src->height/2));
            if (dist < *best_vert) {
                dst->y = snap->y + snap->height/2 - src->height/2;
                *best_vert = dist;
            }
        }
    }


    /* Snap horizontally */
    if (best_horz) {

        /* Snap left side to left side */
        dist = abs(snap->x - src->x);
        if (dist < *best_horz) {
            dst->x = snap->x;
            *best_horz = dist;
        }

        /* Snap left side to right side */
        dist = abs((snap->x + snap->width) - src->x);
        if (dist < *best_horz) {
            dst->x = snap->x + snap->width;
            *best_horz = dist;
        }

        /* Snap right side to left side */
        dist = abs(snap->x - (src->x + src->width));
        if (dist < *best_horz) {
            dst->x = snap->x - src->width;
            *best_horz = dist;
        }

        /* Snap right side to right side */
        dist = abs((snap->x + snap->width) - (src->x + src->width));
        if (dist < *best_horz) {
            dst->x = snap->x + snap->width - src->width;
            *best_horz = dist;
        }

        /* Snap midlines */
        if (/* Left of 'src' is before right of 'snap' */
            (src->x <= snap->x + snap->width + snap_strength) &&
            /* Right of 'src' is after left of 'snap' */
            (src->x + src->width >= snap->x - snap_strength)) {

            /* Snap vertically */
            dist = abs((snap->x + snap->width/2) - (src->x + src->width/2));
            if (dist < *best_horz) {
                dst->x = snap->x + snap->width/2 - src->width/2;
                *best_horz = dist;
            }
        }
    }

} /* snap_dim_to_dim() */



/** snap_side_to_dim() **********************************************
 *
 * Snaps the sides of src to snap and stores the result in dst
 *
 * Returns 1 if a snap occurred.
 *
 **/

static void snap_side_to_dim(GdkRectangle *dst, GdkRectangle *src,
                             GdkRectangle *snap,
                             int *best_vert, int *best_horz)
{
    int dist;


    /* Snap vertically */
    if (best_vert) {

        /* Snap side to top side */
        dist = abs(snap->y - (src->y + src->height));
        if (dist < *best_vert) {
            dst->height = snap->y - src->y;
            *best_vert = dist;
        }

        /* Snap side to bottom side */
        dist = abs((snap->y + snap->height) - (src->y + src->height));
        if (dist < *best_vert) {
            dst->height = snap->y + snap->height - src->y;
            *best_vert = dist;
        }
    }


    /* Snap horizontally */
    if (best_horz) {

        /* Snap side to left side */
        dist = abs(snap->x - (src->x + src->width));
        if (dist < *best_horz) {
            dst->width = snap->x - src->x;
            *best_horz = dist;
        }

        /* Snap side to right side */
        dist = abs((snap->x + snap->width) - (src->x + src->width));
        if (dist < *best_horz) {
            dst->width = snap->x + snap->width - src->x;
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
    GdkRectangle *screen_rect;


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

            /* NOTE: When the display devices' screens are relative to each
             *       other, we may still want to allow snapping of the non-
             *       related edges.  This is useful, for example, when two
             *       screens have a right of/left of relationship and
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
            snap_dim_to_dim(&(info->dst_dim),
                            &(info->src_dim),
                            &(other->cur_mode->pan),
                            ctk_object->snap_strength, bv, bh);

            /* Snap to other display's dimensions */
            {
                GdkRectangle rect;
                get_viewportin_rect(other->cur_mode, &rect);
                snap_dim_to_dim(&(info->dst_dim),
                                &(info->src_dim),
                                &rect,
                                ctk_object->snap_strength, bv, bh);
            }
        }

    } /* Done snapping to other displays */


    /* Snap to dimensions of other X screens */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {

        if (screen == info->screen) continue;

        /* NOTE: When the (display devices') screens are relative to
         *       each other, we may still want to allow snapping of the
         *       non-related edges.  This is useful, for example, when
         *       two screens have a right of/left of relationship and
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
            info->display->cur_mode->pan.y == info->screen->dim.y) {
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
            info->display->cur_mode->pan.x == info->screen->dim.x) {
            bh = NULL;
        }

        screen_rect = get_screen_rect(screen, 0);
        snap_dim_to_dim(&(info->dst_dim),
                        &(info->src_dim),
                        screen_rect,
                        ctk_object->snap_strength, bv, bh);
    }

    /* Snap to the maximum screen dimensions */
    bv = &info->best_snap_v;
    bh = &info->best_snap_h;

    if (info->display) {
        dist = abs( (info->screen->dim.x + info->screen->max_width)
                   -(info->src_dim.x + info->src_dim.width));
        if (dist < *bh) {
            info->dst_dim.x = info->screen->dim.x + info->screen->max_width -
                info->src_dim.width;
            *bh = dist;
        }
        dist = abs( (info->screen->dim.y + info->screen->max_height)
                   -(info->src_dim.y + info->src_dim.height));
        if (dist < *bv) {
            info->dst_dim.y = info->screen->dim.y + info->screen->max_height -
                info->src_dim.height;
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
 * device as well as setting a (no-scanout) screen's virtual size.
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
    GdkRectangle *screen_rect;


    if (info->display) {
        /* Snap to multiples of the display's dimensions */
        const nvSize *cur_mode_size = &(info->display->cur_mode->viewPortIn);

        bh = &(info->best_snap_h);
        bv = &(info->best_snap_v);

        dist = info->src_dim.width % cur_mode_size->width;
        if (dist < *bh) {
            info->dst_dim.width = cur_mode_size->width *
                (int)(info->src_dim.width / cur_mode_size->width);
            *bh = dist;
        }
        dist = cur_mode_size->width -
            (info->src_dim.width % cur_mode_size->width);
        if (dist < *bh) {
            info->dst_dim.width = cur_mode_size->width *
                (1 + (int)(info->src_dim.width / cur_mode_size->width));
            *bh = dist;
        }
        dist = abs(info->src_dim.height % cur_mode_size->height);
        if (dist < *bv) {
            info->dst_dim.height = cur_mode_size->height *
                (int)(info->src_dim.height / cur_mode_size->height);
            *bv = dist;
        }
        dist = cur_mode_size->height -
            (info->src_dim.height % cur_mode_size->height);
        if (dist < *bv) {
            info->dst_dim.height = cur_mode_size->height *
                (1 + (int)(info->src_dim.height / cur_mode_size->height));
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
         *       displays have a right of/left of relationship and
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
        snap_side_to_dim(&(info->dst_dim),
                         &(info->src_dim),
                         &(other->cur_mode->pan),
                         bv, bh);

        /* Snap to other display dimensions */
        {
            GdkRectangle rect;
            get_viewportin_rect(other->cur_mode, &rect);
            snap_side_to_dim(&(info->dst_dim),
                             &(info->src_dim),
                             &rect,
                             bv, bh);
        }
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

        screen_rect = get_screen_rect(screen, 0);
        snap_side_to_dim(&(info->dst_dim),
                         &(info->src_dim),
                         screen_rect,
                         bv, bh);
    }

    bh = &(info->best_snap_h);
    bv = &(info->best_snap_v);

    /* Snap to the maximum screen width */
    dist = abs((info->screen->dim.x + info->screen->max_width)
               -(info->src_dim.x + info->src_dim.width));
    if (dist < *bh) {
        info->dst_dim.width = info->screen->dim.x + info->screen->max_width -
            info->src_dim.x;
        *bh = dist;
    }

    /* Snap to the maximum screen height */
    dist = abs((info->screen->dim.y + info->screen->max_height)
               -(info->src_dim.y + info->src_dim.height));
    if (dist < *bv) {
        info->dst_dim.height = info->screen->dim.y + info->screen->max_height -
            info->src_dim.y;
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

    GdkRectangle *dim; /* Temp dimensions */
    GdkRectangle *sdim; /* Temp screen dimensions */


    info->modify_panning = 0;
    if (!get_modify_info(ctk_object)) return 0;


    /* Should we snap */
    info->snap = snap;


    /* Moving something that is using relative positioning can be done
     * fairly cleanly with common code, so do that here.
     */
    if (info->orig_position_type != CONF_ADJ_ABSOLUTE) {
        int p_x = (ctk_object->mouse_x - ctk_object->img_dim.x) /
            ctk_object->scale;
        int p_y = (ctk_object->mouse_y - ctk_object->img_dim.y) /
            ctk_object->scale;

        if (info->display) {
            dim = &(info->display->cur_mode->relative_to->cur_mode->pan);
        } else {
            dim = get_screen_rect(info->screen->relative_to, 0);
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
        info->modify_dim.x += x;
        info->modify_dim.y += y;

        info->dst_dim = info->modify_dim;


        /* Snap to other screens and displays */
        if (snap && ctk_object->snap_strength) {

            info->src_dim = info->dst_dim;
            snap_move(ctk_object);

            if (info->display) {

                /* Also snap display's panning box to other screens/displays */
                info->src_dim.width  = info->display->cur_mode->pan.width;
                info->src_dim.height = info->display->cur_mode->pan.height;

                info->dst_dim.width  = info->src_dim.width;
                info->dst_dim.height = info->src_dim.height;

                snap_move(ctk_object);
            }
        }

        /* Get the bounding dimensions of what is being moved */
        dim = info->target_dim;
        sdim = get_screen_rect(info->screen, 1);

        /* Prevent moving out of the max layout bounds */
        x = MAX_LAYOUT_WIDTH - dim->width;
        if (info->dst_dim.x > x) {
            info->modify_dim.x += x - info->dst_dim.x;
            info->dst_dim.x = x;
        }
        y = MAX_LAYOUT_HEIGHT - dim->height;
        if (info->dst_dim.y > y) {
            info->modify_dim.y += y - info->dst_dim.y;
            info->dst_dim.y = y;
        }
        x = layout->dim.width - MAX_LAYOUT_WIDTH;
        if (info->dst_dim.x < x) {
            info->modify_dim.x += x - info->dst_dim.x;
            info->dst_dim.x = x;
        }
        y = layout->dim.height - MAX_LAYOUT_HEIGHT;
        if (info->dst_dim.y < y) {
            info->modify_dim.y += y - info->dst_dim.y;
            info->dst_dim.y = y;
        }

        /* Prevent screen from growing too big */
        x = sdim->x + info->screen->max_width - dim->width;
        if (info->dst_dim.x > x) {
            info->modify_dim.x += x - info->dst_dim.x;
            info->dst_dim.x = x;
        }
        y = sdim->y + info->screen->max_height - dim->height;
        if (info->dst_dim.y > y) {
            info->modify_dim.y += y - info->dst_dim.y;
            info->dst_dim.y = y;
        }
        x = sdim->x + sdim->width - info->screen->max_width;
        if (info->dst_dim.x < x) {
            info->modify_dim.x += x - info->dst_dim.x;
            info->dst_dim.x = x;
        }
        y = sdim->y + sdim->height - info->screen->max_height;
        if (info->dst_dim.y < y) {
            info->modify_dim.y += y - info->dst_dim.y;
            info->dst_dim.y = y;
        }


        /* Apply the move */
        if (!info->display) {
            /* Move the screen */
            x = info->dst_dim.x - info->orig_dim.x;
            y = info->dst_dim.y - info->orig_dim.y;

            offset_screen(info->screen, x, y);

        } else {
            /* Move the display to its destination */
            info->display->cur_mode->pan.x = info->dst_dim.x;
            info->display->cur_mode->pan.y = info->dst_dim.y;

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
                x = info->screen->cur_metamode->dim.x - info->orig_screen_dim.x;
                y = info->screen->cur_metamode->dim.y - info->orig_screen_dim.y;

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
    if (sync_layout(ctk_object)) {
        modified = 1;
    }

    /* If what we moved required the layout to be shifted, offset
     * the modify dim (used for snapping) by the same displacement.
     */
    x = info->target_dim->x - info->dst_dim.x;
    y = info->target_dim->y - info->dst_dim.y;
    if (x || y) {
        info->modify_dim.x += x;
        info->modify_dim.y += y;
    }

    /* Check if the item being moved has a new position */
    if (*(info->target_position_type) != info->orig_position_type ||
        info->target_dim->x != info->orig_dim.x ||
        info->target_dim->y != info->orig_dim.y) {
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
    ModifyInfo *info = &(ctk_object->modify_info);
    int modified = 0;

    GdkRectangle *dim;
    int extra;


    info->modify_panning = 1;
    if (!get_modify_info(ctk_object)) return 0;


    /* Only allow changing the panning of displays and the size
     * of no-scanout screens.
     */
    if (!info->display && !info->screen->no_scanout) return 0;

    info->snap = snap;


    /* Compute pre-snap dimensions */
    info->modify_dim.width += x;
    info->modify_dim.height += y;

    /* Don't allow the thing being modified to get too small */
    if (info->display) {
        clamp_rect_to_viewportin(&(info->modify_dim), info->display->cur_mode);
    } else if (info->screen->no_scanout) {
        clamp_screen_size_rect(&(info->modify_dim));
    }

    info->dst_dim.width = info->modify_dim.width;
    info->dst_dim.height = info->modify_dim.height;


    /* Snap to other screens and dimensions */
    if (snap && ctk_object->snap_strength) {
        info->src_dim = info->dst_dim;
        snap_pan(ctk_object);
    }

    /* Make sure no-scanout virtual screen width is at least a multiple of 8 */
    if (info->screen->no_scanout) {
        extra = (info->dst_dim.width % 8);
        if (extra > 0) {
            info->dst_dim.width += (8 - extra);
        }
    }

    /* Panning should not cause us to exceed the maximum layout dimensions */
    x = MAX_LAYOUT_WIDTH - info->dst_dim.x;
    if (info->dst_dim.width > x) {
        info->modify_dim.width += x - info->dst_dim.width;
        info->dst_dim.width = x;
    }
    y = MAX_LAYOUT_HEIGHT - info->dst_dim.y;
    if (info->dst_dim.height > y) {
        info->modify_dim.height += y - info->dst_dim.height;
        info->dst_dim.height = y;
    }

    /* Panning should not cause us to exceed the maximum screen dimensions */
    dim = get_screen_rect(info->screen, 1);
    x = dim->x + info->screen->max_width - info->dst_dim.x;
    if (info->dst_dim.width > x) {
        info->modify_dim.width += x - info->dst_dim.width;
        info->dst_dim.width = x;
    }
    y = dim->y + info->screen->max_height - info->dst_dim.y;
    if (info->dst_dim.height > y) {
        info->modify_dim.height += y - info->dst_dim.height;
        info->dst_dim.height = y;
    }

    /* Panning domain can never be smaller then the display ViewPortIn */
    if (info->display) {
        clamp_rect_to_viewportin(&(info->dst_dim), info->display->cur_mode);
    } else if (info->screen->no_scanout) {
        clamp_screen_size_rect(&(info->dst_dim));
    }

    /* Assign the new size */
    info->target_dim->width = info->dst_dim.width;
    info->target_dim->height = info->dst_dim.height;


    /* Recalculate layout dimensions and scaling */
    if (sync_layout(ctk_object)) {
        modified = 1;
    }

    /* Check if the item being moved has a new position */
    /* Report if anything changed */
    if (info->target_dim->width != info->orig_dim.width ||
        info->target_dim->height != info->orig_dim.height) {
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

#define DIST_SQR(D) (((D).x * (D).x) + ((D).y * (D).y))

static void select_default_item(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr sel_display = NULL;
    nvScreenPtr sel_screen = NULL;
    nvScreenPtr screen;
    nvDisplayPtr display;
    int i;
    int best_dst = -1; // Distance squared to element.
    int dst;

    /* Clear the selection */
    ctk_object->selected_display = NULL;
    ctk_object->selected_screen = NULL;

    for (i = 0; i < ctk_object->Zcount; i++) {

        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;

            /* Ignore disabled displays */
            if (!display->cur_mode) continue;

            dst = DIST_SQR(display->cur_mode->pan);
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
 * screen.  The caller should free the string that is returned.
 *
 **/

static char *get_screen_tooltip(nvScreenPtr screen)
{
    char *tip;


    /* No display given */
    if (!screen) {
        return NULL;
    }

    tip = g_strdup_printf("X Screen %d%s", screen->scrnum,
                          screen->no_scanout ? " : No Scanout" : "");

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


    /* Scale and offset x & y so they reside in clickable area */
    x = (x -ctk_object->img_dim.x) / ctk_object->scale;
    y = (y -ctk_object->img_dim.y) / ctk_object->scale;


    /* Go through the Z-order looking for what we are under */
    for (i = 0; i < ctk_object->Zcount; i++) {

        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;
            if (point_in_display(display, x, y)) {
                screen = NULL;
                if (display == last_display) {
                    goto found;
                }
                tip = get_display_tooltip(display, ctk_object->advanced_mode);
                goto found;
            }

        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            screen = ctk_object->Zorder[i].u.screen;
            if (point_in_screen(screen, x, y)) {
                display = NULL;
                if (screen == last_screen) {
                    goto found;
                }
                tip = get_screen_tooltip(screen);
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
    GdkModifierType state;


    /* Assume user didn't actually click inside a display for now */
    ctk_object->clicked_outside = 1;
    ctk_object->selected_display = NULL;
    ctk_object->selected_screen = NULL;

    gdk_window_get_pointer
        (GTK_WIDGET(ctk_get_parent_window(ctk_object->drawing_area))->window,
         NULL, NULL, &state);

    /* Look through the Z-order for the next element */
    for (i = 0; i < ctk_object->Zcount; i++) {
        if (ctk_object->Zorder[i].type == ZNODE_TYPE_DISPLAY) {
            display = ctk_object->Zorder[i].u.display;
            if (point_in_display(display, x, y)) {
                select_display(ctk_object, display);
                ctk_object->clicked_outside = 0;
                break;
            }
        } else if (ctk_object->Zorder[i].type == ZNODE_TYPE_SCREEN) {
            screen = ctk_object->Zorder[i].u.screen;
            if (point_in_screen(screen, x, y)) {
                select_screen(ctk_object, screen);
                ctk_object->clicked_outside = 0;
                break;
            }
        }
    }

    /* Select display's X screen when CTRL is held down on click */
    if (ctk_object->selected_screen && (state & GDK_CONTROL_MASK)) {
        ctk_object->selected_display = NULL;
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

    sync_layout(ctk_object);
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
                      GdkRectangle *rect,
                      GdkColor *color,
                      int fill)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);

    /* Setup color to use */
    gdk_gc_set_rgb_fg_color(fg_gc, color);

    /* Draw the rectangle */
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       fill,
                       ctk_object->img_dim.x + ctk_object->scale * rect->x,
                       ctk_object->img_dim.y + ctk_object->scale * rect->y,
                       ctk_object->scale * rect->width,
                       ctk_object->scale * rect->height);

} /* draw_rect() */



/** draw_rect_strs() *************************************************
 *
 * Draws possibly 2 rows of text in the middle of a bounding,
 * scaled rectangle.  If the text does not fit, it is not drawn.
 *
 **/

static void draw_rect_strs(CtkDisplayLayout *ctk_object,
                           GdkRectangle *rect,
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

        if (txt_w +8 <= ctk_object->scale * rect->width &&
            txt_h +8 <= ctk_object->scale * rect->height) {
            draw_1 = 1;
        }
    }

    if (str_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_2, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        if (txt_w +8 <= ctk_object->scale * rect->width &&
            txt_h +8 <= ctk_object->scale * rect->height) {
            draw_2 = 1;
        }

        str = g_strconcat(str_1, "\n", str_2, NULL);

        pango_layout_set_text(ctk_object->pango_layout, str, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        if (draw_1 && draw_2 &&
            txt_h +8 > ctk_object->scale * rect->height) {
            draw_2 = 0;
        }

        g_free(str);
    }

    if (draw_1 && !draw_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_1, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x1 = ctk_object->scale*(rect->x + rect->width / 2) - (txt_w / 2);
        txt_y1 = ctk_object->scale*(rect->y + rect->height / 2) - (txt_h / 2);

        /* Write name */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim.x + txt_x1,
                        ctk_object->img_dim.y + txt_y1,
                        ctk_object->pango_layout);
    }

    else if (!draw_1 && draw_2) {
        pango_layout_set_text(ctk_object->pango_layout, str_2, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x2 = ctk_object->scale*(rect->x + rect->width / 2) - (txt_w / 2);
        txt_y2 = ctk_object->scale*(rect->y + rect->height / 2) - (txt_h / 2);

        /* Write dimensions */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim.x + txt_x2,
                        ctk_object->img_dim.y + txt_y2,
                        ctk_object->pango_layout);
    }

    else if (draw_1 && draw_2) {
        str = g_strconcat(str_1, "\n", str_2, NULL);

        pango_layout_set_text(ctk_object->pango_layout, str, -1);
        pango_layout_get_pixel_size(ctk_object->pango_layout, &txt_w, &txt_h);

        txt_x = ctk_object->scale*(rect->x + rect->width / 2) - (txt_w / 2);
        txt_y = ctk_object->scale*(rect->y + rect->height / 2) - (txt_h / 2);

        /* Write both */
        gdk_gc_set_rgb_fg_color(fg_gc, color);

        gdk_draw_layout(ctk_object->pixmap,
                        fg_gc,
                        ctk_object->img_dim.x + txt_x,
                        ctk_object->img_dim.y + txt_y,
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
    GdkRectangle rect;

    if (!display || !(display->cur_mode)) {
        return;
    }

    mode = display->cur_mode;
    base_color_idx =
        NUM_COLORS_PER_PALETTE * NvCtrlGetTargetId(display->gpu->handle);


    /* Draw panning */
    color_idx = base_color_idx + ((mode->modeline) ? BG_PAN_ON : BG_PAN_OFF);
    draw_rect(ctk_object, &(mode->pan),
              &(ctk_object->color_palettes[color_idx]), 1);
    draw_rect(ctk_object, &(mode->pan), &(ctk_object->fg_color), 0);


    /* Draw ViewPortIn */
    get_viewportin_rect(mode, &rect);
    color_idx = base_color_idx + ((mode->modeline) ? BG_SCR_ON : BG_SCR_OFF);
    draw_rect(ctk_object, &rect, &(ctk_object->color_palettes[color_idx]), 1);
    draw_rect(ctk_object, &rect, &(ctk_object->fg_color), 0);


    /* Draw text information */
    if (!mode->display->screen) {
        tmp_str = g_strdup("(Disabled)");
    } else if (mode->modeline) {
        tmp_str = g_strdup_printf("%dx%d", mode->viewPortIn.width,
                                  mode->viewPortIn.height);
    } else {
        tmp_str = g_strdup("(Off)");
    }
    draw_rect_strs(ctk_object,
                   &rect,
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

    GdkRectangle *sdim; /* Screen dimensions */
    GdkColor bg_color; /* Background color */
    GdkColor bd_color; /* Border color */
    char *tmp_str;


    if (!screen)  return;


    /* Draw the screen effective size */
    gdk_color_parse("#888888", &bg_color);
    gdk_color_parse("#777777", &bd_color);

    sdim = get_screen_rect(screen, 1);

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
                       &(screen->dim),
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
        GdkRectangle vpiRect;
        GdkRectangle *rect;

        if (ctk_object->selected_display) {
            get_viewportin_rect(ctk_object->selected_display->cur_mode,
                                &vpiRect);
            rect = &vpiRect;
        } else {
            rect = get_screen_rect(ctk_object->selected_screen, 0);
        }

        /* Draw red selection border */
        w  = (int)(ctk_object->scale * rect->width);
        h  = (int)(ctk_object->scale * rect->height);

        /* Setup color to use */
        gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->select_color));

        /* If dislay is too small, just color the whole thing */
        size = 3;
        offset = (size/2) +1;

        if ((w -(2* offset) < 0) || (h -(2 *offset) < 0)) {
            draw_rect(ctk_object, rect, &(ctk_object->select_color), 1);
            draw_rect(ctk_object, rect, &(ctk_object->fg_color), 0);

        } else {
            gdk_gc_set_line_attributes(fg_gc, size, GDK_LINE_SOLID,
                                       GDK_CAP_ROUND, GDK_JOIN_ROUND);

            gdk_draw_rectangle(ctk_object->pixmap,
                               fg_gc,
                               0,
                               ctk_object->img_dim.x +(ctk_object->scale * rect->x) +offset,
                               ctk_object->img_dim.y +(ctk_object->scale * rect->y) +offset,
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
                          &(ctk_object->selected_screen->cur_metamode->viewPortIn),
                          &(bg_color), 0);

                // Shows the current screen dimensions used for relative
                // positioning of the screen (w/o displays that are "off")
                gdk_color_parse("#FF00FF", &bg_color);
                draw_rect(ctk_object,
                          &(ctk_object->selected_screen->cur_metamode->edim),
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



/** sync_layout() ****************************************************
 *
 * Recalculates the X screen positions in the layout such that the
 * top-left most X screen is at 0x0.
 *
 **/

static Bool sync_layout(CtkDisplayLayout *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvScreenPtr screen;
    Bool modified = FALSE;


    /* Align all metamodes of each screen */
    for (screen = layout->screens; screen; screen = screen->next_in_layout) {
        if (realign_screen(screen)) {
            modified = TRUE;
        }
    }

    /* Resolve final screen positions */
    calc_layout(layout);

    /* Offset layout back to (0,0) */
    if (layout->dim.x || layout->dim.y) {
        offset_layout(layout, -layout->dim.x, -layout->dim.y);
        modified = TRUE;
    }

    if (sync_scaling(ctk_object)) {
        modified = TRUE;
    }

    return modified;
}



/** ctk_display_layout_update() **************************************
 *
 * Causes a recalculation of the layout.
 *
 **/

void ctk_display_layout_update(CtkDisplayLayout *ctk_object)
{
    /* Recalculate layout dimensions and scaling */
    sync_layout(ctk_object);
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

    sync_layout(ctk_object);
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



/** ctk_display_layout_set_screen_metamode() *************************
 *
 * Sets which metamode the screen should use.
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


    if (!screen) return;


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

        /* Duplicate the currently selected mode */
        if (display->cur_mode) {
            memcpy(mode, display->cur_mode, sizeof(*mode));
        }

        /* Link the mode to the metamode */
        mode->metamode = metamode;

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


    if (!screen || metamode_idx >= screen->num_metamodes) {
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
        if (screen == ctk_object->selected_screen) {
            ctk_object->selected_screen = NULL;
        }

        /* Make sure screen numbers are consistent */
        renumber_xscreens(ctk_object->layout);
    }

    /* Add the fake mode to the display */
    gpu_add_screenless_modes_to_displays(display->gpu);

    /* Re-select the display to sync the loss of the screen */
    if (display == ctk_object->selected_display) {
        select_display(ctk_object, display);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_disable_display() */



/** ctk_display_layout_set_mode_modeline() ***************************
 *
 * Sets which modeline, ViewPortIn and ViewPortOut the mode should use.
 *
 **/

void ctk_display_layout_set_mode_modeline(CtkDisplayLayout *ctk_object,
                                          nvModePtr mode,
                                          nvModeLinePtr modeline,
                                          const nvSize *viewPortIn,
                                          const GdkRectangle *viewPortOut)
{
    nvModeLinePtr old_modeline;

    if (!mode) {
        return;
    }

    /* Set the new modeline */
    old_modeline = mode->modeline;

    mode_set_modeline(mode, modeline, viewPortIn, viewPortOut);

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


/*!
 * Sets the ViewPortIn for the given mode.
 *
 * If a modification occurs, this function will call the modified_callback
 * handler registered, if any.
 *
 * \param[in]  ctk_object  The Display Layout object
 * \param[in]  mode        The mode to be modified
 * \param[in]  w           The width of the ViewPortIn to set
 * \param[in]  h           The height of the ViewPortIn to set
 */
void ctk_display_layout_set_mode_viewport_in(CtkDisplayLayout *ctk_object,
                                             nvModePtr mode,
                                             int w, int h,
                                             Bool update_panning_size)
{
    Bool modified = TRUE;

    if (!mode || !mode->modeline) {
        return;
    }

    if (w < 1) {
        w = 1;
    }
    if (h < 1) {
        h = 1;
    }

    mode->viewPortIn.width  = w;
    mode->viewPortIn.height = h;

    if (update_panning_size) {
        mode->pan.width  = w;
        mode->pan.height = h;
    }

    clamp_mode_panning(mode);

    if (modified) {
        /* Update the layout */
        ctk_display_layout_update(ctk_object);

        /* Notify the modification */
        if (ctk_object->modified_callback) {
            ctk_object->modified_callback(ctk_object->layout,
                                          ctk_object->modified_callback_data);
        }
    }
}


/*!
 * Sets the ViewPortOut for the given mode.
 *
 * If a modification occurs, this function will call the modified_callback
 * handler registered, if any.
 *
 * \param[in]  ctk_object  The Display Layout object
 * \param[in]  mode        The mode to be modified
 * \param[in]  x           The X offset of the ViewPortOut to set
 * \param[in]  y           The Y offset of the ViewPortOut to set
 * \param[in]  w           The width of the ViewPortOut to set
 * \param[in]  h           The height of the ViewPortOut to set
 */
void ctk_display_layout_set_mode_viewport_out(CtkDisplayLayout *ctk_object,
                                              nvModePtr mode,
                                              int x, int y, int w, int h)
{
    Bool modified = TRUE;
    int extra;

    if (!mode || !mode->modeline) {
        return;
    }

    /* Clamp ViewPortOut to raster size.  If the ViewPortOut extends past the
     * raster size, reduce the ViewPortOut offset before reducing the
     * dimensions
     */
    extra = (x + w) - mode->modeline->data.hdisplay;
    if (extra > 0) {
        if (extra > x) {
            w = mode->modeline->data.hdisplay;
            x = 0;
        } else {
            x -= extra;
        }
    }

    extra = (y + h) - mode->modeline->data.vdisplay;
    if (extra > 0) {
        if (extra > y) {
            h = mode->modeline->data.vdisplay;
            y = 0;
        } else {
            y -= extra;
        }
    }

    if (w < 1) {
        w = 1;
    }
    if (h < 1) {
        h = 1;
    }
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }

    mode->viewPortOut.x      = x;
    mode->viewPortOut.y      = y;
    mode->viewPortOut.width  = w;
    mode->viewPortOut.height = h;

    if (modified) {
        /* Update the layout */
        ctk_display_layout_update(ctk_object);

        /* Notify the modification */
        if (ctk_object->modified_callback) {
            ctk_object->modified_callback(ctk_object->layout,
                                          ctk_object->modified_callback_data);
        }
    }
}



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
                                 x - display->cur_mode->pan.x,
                                 y - display->cur_mode->pan.y,
                                 0);

        /* Report back result of move */
        if (ctk_object->modified_callback &&
            (modified ||
             x != display->cur_mode->pan.x ||
             y != display->cur_mode->pan.y)) {
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
                            width  - display->cur_mode->pan.width,
                            height - display->cur_mode->pan.height,
                            0);

    /* Report back result of move */
    if (ctk_object->modified_callback &&
        (modified ||
         width  != display->cur_mode->pan.width ||
         height != display->cur_mode->pan.height)) {
        ctk_object->modified_callback(ctk_object->layout,
                                      ctk_object->modified_callback_data);
    }

    queue_layout_redraw(ctk_object);

} /* ctk_display_layout_set_display_panning() */



/*!
 * Sets the rotation orientation for the display.
 *
 * In basic mode, this function will make all modes on the display have the
 * same rotation.  In advanced mode, only the current mode will have its
 * rotation orientation modified.
 *
 * If a modification occurs, this function will call the modified_callback
 * handler registered, if any.
 *
 * \param[in]  ctk_object  The Display Layout object
 * \param[in]  display     The display who's modes are to be modified
 * \param[in]  rotation    The rotation to set
 */
void ctk_display_layout_set_display_rotation(CtkDisplayLayout *ctk_object,
                                             nvDisplayPtr display,
                                             Rotation rotation)
{
    Bool modified;


    if (!display->cur_mode ||
        !display->cur_mode->modeline) {
        return;
    }

    if (ctk_object->advanced_mode) {
        /* In advanced mode, only set the rotation of the current mode */
        modified = mode_set_rotation(display->cur_mode, rotation);
    } else {
        /* In basic mode, make all the modes have the same rotation */
        modified = display_set_modes_rotation(display, rotation);
    }

    if (modified) {
        /* Update the layout */
        ctk_display_layout_update(ctk_object);

        /* Notify the modification */
        if (ctk_object->modified_callback) {
            ctk_object->modified_callback(ctk_object->layout,
                                          ctk_object->modified_callback_data);
        }
    }
}



/*!
 * Sets the reflection orientation for the display.
 *
 * In basic mode, this function will make all modes on the display have the
 * same reflection.  In advanced mode, only the current mode will have its
 * reflection orientation modified.
 *
 * If a modification occurs, this function will call the modified_callback
 * handler registered, if any.
 *
 * \param[in]  ctk_object  The Display Layout object
 * \param[in]  display     The display who's modes are to be modified
 * \param[in]  reflection  The reflection to set
 */
void ctk_display_layout_set_display_reflection(CtkDisplayLayout *ctk_object,
                                               nvDisplayPtr display,
                                               Reflection reflection)
{
    Bool modified;


    if (!display->cur_mode ||
        !display->cur_mode->modeline) {
        return;
    }

    if (ctk_object->advanced_mode) {
        /* In advanced mode, only set the reflection of the current mode */
        if (display->cur_mode->reflection != reflection) {
            modified = TRUE;
        }
        display->cur_mode->reflection = reflection;
    } else {
        nvModePtr mode;

        /* In basic mode, make all the modes have the same reflection */
        for (mode = display->modes; mode; mode = mode->next) {
            if (mode->reflection != reflection) {
                mode->reflection = reflection;
                modified = TRUE;
            }
        }
    }

    if (modified) {
        /* Update the layout */
        ctk_display_layout_update(ctk_object);

        /* Notify the modification */
        if (ctk_object->modified_callback) {
            ctk_object->modified_callback(ctk_object->layout,
                                          ctk_object->modified_callback_data);
        }
    }
}



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
                            width - screen->dim.width,
                            height - screen->dim.height,
                            0);

    if (ctk_object->modified_callback &&
        (modified ||
         width != screen->dim.width ||
         height != screen->dim.height)) {
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
            int x_offset = x - screen->dim.x;
            int y_offset = y - screen->dim.y;
            GdkRectangle *sdim;

            /* Make sure this screen use absolute positioning */
            switch_screen_to_absolute(screen);

            /* Do the move by offsetting */
            offset_screen(screen, x_offset, y_offset);

            /* Recalculate the layout */
            ctk_display_layout_update(ctk_object);

            /* Report back result of move */
            sdim = get_screen_rect(screen, 1);
            if (x != sdim->x || y != sdim->y) {
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
 * bells and whistles.
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
 * Sets up callbacks so users of the display layout can receive
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

    ctk_object->img_dim.x      = LAYOUT_IMG_OFFSET + LAYOUT_IMG_BORDER_PADDING;
    ctk_object->img_dim.y      = LAYOUT_IMG_OFFSET + LAYOUT_IMG_BORDER_PADDING;
    ctk_object->img_dim.width  = width  -2*(ctk_object->img_dim.x);
    ctk_object->img_dim.height = height -2*(ctk_object->img_dim.y);

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
    if (ctk_object->advanced_mode && (state & GDK_SHIFT_MASK)) {
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
    int x = (event->x -ctk_object->img_dim.x) / ctk_object->scale;
    int y = (event->y -ctk_object->img_dim.y) / ctk_object->scale;

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

    /* XXX Ignore triple clicks */
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
