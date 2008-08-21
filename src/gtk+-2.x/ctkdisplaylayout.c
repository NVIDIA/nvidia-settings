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

#include <stdlib.h> /* malloc */
#include <string.h> /* strlen */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkdisplaylayout.h"




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
    int i;

    
    /* Clear the list */
    if (ctk_object->Zorder) {
        free(ctk_object->Zorder);
        ctk_object->Zorder = NULL;
    }
    ctk_object->Zcount = 0;
    ctk_object->Zselected = 0;


    /* Count the number of displays in the layout */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        ctk_object->Zcount += gpu->num_displays;
    }


    /* If there are no displays, we're done */
    if (!ctk_object->Zcount) {
        return;
    }


    /* Create the Z order list buffer */
    ctk_object->Zorder =
        (nvDisplayPtr *)calloc(1, ctk_object->Zcount *sizeof(nvDisplayPtr));
    if (!ctk_object->Zorder) {
        ctk_object->Zcount = 0;
        return;
    }


    /* Populate the Z order list */
    i = 0;
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        nvDisplayPtr display;
        for (display = gpu->displays; display; display = display->next) {
            ctk_object->Zorder[i++] = display;            
        }
    }

} /* zorder_layout() */



/** get_selected() ***************************************************
 *
 * Returns the currently selected display.  The selected display
 * device should always be at the top of the Z-order.
 *
 **/

static nvDisplayPtr get_selected(CtkDisplayLayout *ctk_object)
{
    if (!(ctk_object->Zselected) || !(ctk_object->Zcount)) {
        return NULL;
    }

    return ctk_object->Zorder[0];

} /*  get_selected() */



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



/** sync_modify() ****************************************************
 *
 * When the user is moving/panning a display device around, a
 * temporary dimension buffer is used to allow for snapping to other
 * displays.
 *
 * This function sets up the temporary buffer by copying the actual
 * dimensions of the selected display device to the temporary buffer.
 *
 **/

static void sync_modify(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr display = get_selected(ctk_object);

    if (display && display->cur_mode) {
        ctk_object->modify_dim[X] = display->cur_mode->pan[X];
        ctk_object->modify_dim[Y] = display->cur_mode->pan[Y];
        ctk_object->modify_dim[W] = display->cur_mode->pan[W];
        ctk_object->modify_dim[H] = display->cur_mode->pan[H];
    }

} /* sync_modify() */



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

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {

        /* Offset screens */
        for (screen = gpu->screens; screen; screen = screen->next) {
            offset_screen(screen, x, y);
        }

        /* Offset displays */
        for (display = gpu->displays; display; display = display->next) {
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

static void resolve_displays_in_screen(nvScreenPtr screen, int all_modes)
{
    nvDisplayPtr display;
    int pos[4];
    int first_idx;
    int last_idx;
    int mode_idx;

    if (all_modes) {
        first_idx = 0;
        last_idx = screen->num_metamodes -1;
    } else {
        first_idx = screen->cur_metamode_idx;
        last_idx = first_idx;
    }

    /* Resolve the current mode of each display in the screen */
    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue;

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
    nvMetaModePtr metamode = screen->cur_metamode;
    int relative_pos[4];
    
    if (!metamode) return 0;


    /* Set the dimensions */
    pos[W] = metamode->dim[W];
    pos[H] = metamode->dim[H];


    /* Find the position */
    switch (screen->position_type) {
    case CONF_ADJ_ABSOLUTE:
        pos[X] = metamode->dim[X];
        pos[Y] = metamode->dim[Y];
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

    
    /* Resolve the current screen location */
    if (resolve_screen(screen, pos)) {

        /* Move the screen and the displays by offsetting */
    
        x = pos[X] - screen->cur_metamode->dim[X];
        y = pos[Y] - screen->cur_metamode->dim[Y];

        offset_screen(screen, x, y);

        for (display = screen->gpu->displays;
             display;
             display = display->next) {

            if (display->screen != screen) continue;

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
    nvGpuPtr gpu;
    nvScreenPtr screen;

    /* First, resolve TwinView relationships */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            resolve_displays_in_screen(screen, 0);
        }
    }

    /* Next, resolve X screen relationships */
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            resolve_screen_in_layout(screen);
        }
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
    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue;

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


    if (!screen) return;

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
 * the smallest bounding box that holds all the gpu's X screen's
 * display device's (current) modes in the layout.  (Bounding box of
 * all the current metamodes of all X screens.)
 *
 * As a side effect, all other screen/metamode dimensions are
 * calculated.
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

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {

        for (screen = gpu->screens; screen; screen = screen->next) {
            int *sdim;
            
            calc_screen(screen);
            sdim = screen->cur_metamode->dim;
            
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
    }

    dim[W] = dim[W] - dim[X];
    dim[H] = dim[H] - dim[Y];


    /* Position disabled display devices off to the top right */
    x = dim[W] + dim[X];
    y = dim[Y];
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (display = gpu->displays; display; display = display->next) {
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

    for (display = screen->gpu->displays; display; display = display->next) {
        nvModePtr mode;
        
        if (display->screen != screen) continue;

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
    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue; /* Display not in screen */

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
    nvGpuPtr gpu;
    nvScreenPtr screen;
    int real_metamode_idx;
    int metamode_idx;
    
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {

        for (screen = gpu->screens; screen; screen = screen->next) {
            
            real_metamode_idx = screen->cur_metamode_idx;

            for (metamode_idx = 0;
                 metamode_idx < screen->num_metamodes;
                 metamode_idx++) {
                
                if (metamode_idx == real_metamode_idx) continue;

                set_screen_metamode(layout, screen, metamode_idx);
            }

            set_screen_metamode(layout, screen, real_metamode_idx);
        }
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

void reposition_screen(nvScreenPtr screen, int advanced)
{
    int orig_screen_x = screen->dim[X];
    int orig_screen_y = screen->dim[Y];
            
    /* Resolve new relative positions.  In basic mode,
     * relative position changes apply to all modes of a
     * display so we should resolve all modes (since they
     * were all changed.)
     */
    resolve_displays_in_screen(screen, !advanced);

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
        if (dist <= snap_strength && dist < *best_vert) {
            dst[Y] = snap[Y];
            *best_vert = dist;
        }
        
        /* Snap top side to bottom side */
        dist = abs((snap[Y] + snap[H]) - src[Y]);
        if (dist <= snap_strength && dist < *best_vert) {
            dst[Y] = snap[Y] + snap[H];
            *best_vert = dist;
        }
        
        /* Snap bottom side to top side */
        dist = abs(snap[Y] - (src[Y] + src[H]));
        if (dist <= snap_strength && dist < *best_vert) {
            dst[Y] = snap[Y] - src[H];
            *best_vert = dist;
        }
        
        /* Snap bottom side to bottom side */
        dist = abs((snap[Y] + snap[H]) - (src[Y] + src[H]));
        if (dist <= snap_strength && dist < *best_vert) {
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
            if (dist <= snap_strength && dist < *best_vert) {
                dst[Y] = snap[Y] + snap[H]/2 - src[H]/2;
                *best_vert = dist;
            }
        }
    }


    /* Snap horizontally */
    if (best_horz) {
        
        /* Snap left side to left side */
        dist = abs(snap[X] - src[X]);
        if (dist <= snap_strength && dist < *best_horz) {
            dst[X] = snap[X];
            *best_horz = dist;
        }
        
        /* Snap left side to right side */
        dist = abs((snap[X] + snap[W]) - src[X]);
        if (dist <= snap_strength && dist < *best_horz) {
            dst[X] = snap[X] + snap[W];
            *best_horz = dist;
        }
        
        /* Snap right side to left side */
        dist = abs(snap[X] - (src[X] + src[W]));
        if (dist <= snap_strength && dist < *best_horz) {
            dst[X] = snap[X] - src[W];
            *best_horz = dist;
        }
        
        /* Snap right side to right side */
        dist = abs((snap[X] + snap[W]) - (src[X]+src[W]));
        if (dist <= snap_strength && dist < *best_horz) {
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
            if (dist <= snap_strength && dist < *best_horz) {
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

static void snap_side_to_dim(int *dst, int *src, int *snap, int snap_strength,
                             int *best_vert, int *best_horz)
{
    int dist;
 

    /* Snap vertically */
    if (best_vert) {

        /* Snap side to top side */
        dist = abs(snap[Y] - (src[Y] + src[H]));
        if (dist  <= snap_strength && dist < *best_vert) {
            dst[H] = snap[Y] - src[Y];
            *best_vert = dist;
        }
    
        /* Snap side to bottom side */
        dist = abs((snap[Y] + snap[H]) - (src[Y] + src[H]));
        if (dist <= snap_strength && dist < *best_vert) {
            dst[H] = snap[Y] + snap[H] - src[Y];
            *best_vert = dist;
        }
    }


    /* Snap horizontally */
    if (best_horz) {

        /* Snap side to left side */
        dist = abs(snap[X] - (src[X] + src[W]));
        if (dist <= snap_strength && dist < *best_horz) {
            dst[W] = snap[X] - src[X];
            *best_horz = dist;
        }
        
        /* Snap side to right side */
        dist = abs((snap[X] + snap[W]) - (src[X] + src[W]));
        if (dist  <= snap_strength && dist < *best_horz) {
            dst[W] = snap[X] + snap[W] - src[X];
            *best_horz = dist;
        }
    }

} /* snap_side_to_dim() */



/** snap_move() *****************************************************
 *
 * Snaps the given dimensions 'dim' (viewport or panning domain of
 * a dislay) to other display devices' viewport and/or panning
 * domains in the layout due to a move.
 *
 * Returns 1 if a snap occured, 0 if not.
 *
 **/

static int snap_move(CtkDisplayLayout *ctk_object, int *dim)
{
    int dim_orig[4];
    int best_vert;
    int best_horz;
    int *bv;
    int *bh;
    int z;
    int dist;
    nvDisplayPtr display;
    nvDisplayPtr other;
    nvGpuPtr gpu;
    nvScreenPtr screen;


    display = get_selected(ctk_object);
    gpu = display->gpu;
    screen = display->screen;


    /* We will know that a snap occured if
     * 'best_xxxx' <= ctk_object->snap_strength, so we
     * initialize both directions to not having snapped here.
     */
    best_vert = ctk_object->snap_strength +1;
    best_horz = ctk_object->snap_strength +1;


    /* Copy the original dimensions (Always snap from this reference) */
    dim_orig[X] = dim[X];
    dim_orig[Y] = dim[Y];
    dim_orig[W] = dim[W];
    dim_orig[H] = dim[H];


    /* Snap to other display's modes */
    for (z = 1; z < ctk_object->Zcount; z++) {

        other = ctk_object->Zorder[z];

        /* Other display must have a mode */
        if (!other || !other->cur_mode || !other->screen) continue;


        bv = &best_vert;
        bh = &best_horz;

        /* We shouldn't snap horizontally to the other display if
         * we are in a right-of/left-of relative relationship with
         * the other display/display's screen.
         */
        if (((other->cur_mode->position_type == CONF_ADJ_RIGHTOF) ||
             (other->cur_mode->position_type == CONF_ADJ_LEFTOF)) &&
            (other->cur_mode->relative_to == display)) {
            bh = NULL;
        }
        if (((display->cur_mode->position_type == CONF_ADJ_RIGHTOF) ||
             (display->cur_mode->position_type == CONF_ADJ_LEFTOF)) &&
            (display->cur_mode->relative_to == other)) {
            bh = NULL;
        }
        if (display->screen && other->screen &&
            ((other->screen->position_type == CONF_ADJ_RIGHTOF) ||
             (other->screen->position_type == CONF_ADJ_LEFTOF)) &&
            (other->screen->relative_to == display->screen)) {
            bh = NULL;
        }
        if (display->screen && other->screen &&
            ((display->screen->position_type == CONF_ADJ_RIGHTOF) ||
             (display->screen->position_type == CONF_ADJ_LEFTOF)) &&
            (display->screen->relative_to == other->screen)) {
            bh = NULL;
        }

        /* If we aren't snapping horizontally with the other display,
         * we shouldn't snap vertically either if this is the top-most
         * display in the screen.
         */
        if (!bh && display->cur_mode->dim[Y] == display->screen->dim[Y]) {
            bv = NULL;
        }


        /* We shouldn't snap vertically to the other display if
         * we are in a above/below relative relationship with
         * the other display/display's screen.
         */
        if (((other->cur_mode->position_type == CONF_ADJ_ABOVE) ||
             (other->cur_mode->position_type == CONF_ADJ_BELOW)) &&
            (other->cur_mode->relative_to == display)) {
            bv = NULL;
        }
        if (((display->cur_mode->position_type == CONF_ADJ_ABOVE) ||
             (display->cur_mode->position_type == CONF_ADJ_BELOW)) &&
            (display->cur_mode->relative_to == other)) {
            bv = NULL;
        }
        
        if (display->screen && other->screen &&
            ((other->screen->position_type == CONF_ADJ_ABOVE) ||
             (other->screen->position_type == CONF_ADJ_BELOW)) &&
            (other->screen->relative_to == display->screen)) {
            bv = NULL;
        }
        if (display->screen && other->screen &&
            ((display->screen->position_type == CONF_ADJ_ABOVE) ||
             (display->screen->position_type == CONF_ADJ_BELOW)) &&
            (display->screen->relative_to == other->screen)) {
            bv = NULL;
        }

        /* If we aren't snapping vertically with the other display,
         * we shouldn't snap horizontally either if this is the left-most
         * display in the screen.
         */
        if (!bv && display->cur_mode->dim[X] == display->screen->dim[X]) {
            bh = NULL;
        }


        /* Snap to other display's panning dimensions */
        snap_dim_to_dim(dim,
                        dim_orig,
                        ctk_object->Zorder[z]->cur_mode->pan,
                        ctk_object->snap_strength,
                        bv, bh);

        /* Snap to other display's dimensions */
        snap_dim_to_dim(dim,
                        dim_orig,
                        ctk_object->Zorder[z]->cur_mode->dim,
                        ctk_object->snap_strength,
                        bv, bh);
    }


    /* Snap to the maximum screen dimensions */
    dist = abs(screen->dim[X] + gpu->max_width - dim_orig[W] - dim_orig[X]);
    if (dist <= ctk_object->snap_strength && dist < best_horz) {
        dim[X] = screen->dim[X] + gpu->max_width - dim_orig[W];
        best_horz = dist;
    }
    dist = abs(screen->dim[Y] + gpu->max_height - dim_orig[H] - dim_orig[Y]);
    if (dist <= ctk_object->snap_strength && dist < best_vert) {
        dim[Y] = screen->dim[Y] + gpu->max_height - dim_orig[H];
        best_vert = dist;
    }


    return (best_vert <= ctk_object->snap_strength ||
            best_horz <= ctk_object->snap_strength);

} /* snap_move() */



/** snap_pan() ******************************************************
 *
 * Snaps the bottom right of the panning domain given 'pan' of the
 * currently selected display to other display devices' viewport
 * and/or panning domains in the layout due to a panning domain
 * resize.
 *
 * Returns 1 if a snap happened, 0 if not.
 *
 **/

static int snap_pan(CtkDisplayLayout *ctk_object, int *pan, int *dim)
{
    int pan_orig[4];
    int best_vert;
    int best_horz;
    int *bv;
    int *bh;
    int z;
    int dist;
    nvDisplayPtr display;
    nvDisplayPtr other;
    nvGpuPtr gpu;
    nvScreenPtr screen;


    display = get_selected(ctk_object);
    gpu = display->gpu;
    screen = display->screen;


    /* We will know that a snap occured if
     * 'best_xxxx' <= ctk_object->snap_strength, so we
     * initialize both directions to not having snapped here.
     */
    best_vert = ctk_object->snap_strength +1;
    best_horz = ctk_object->snap_strength +1;


    /* Copy the original dimensions (Always snap from this reference) */
    pan_orig[X] = pan[X];
    pan_orig[Y] = pan[Y];
    pan_orig[W] = pan[W];
    pan_orig[H] = pan[H];


    /* Snap to other display's modes */
    for (z = 1; z < ctk_object->Zcount; z++) {

        other = ctk_object->Zorder[z];
        
        /* Other display must have a mode */
        if (!other || !other->cur_mode || !other->screen) continue;

        bv = &best_vert;
        bh = &best_horz;


        /* Don't snap to other displays that are positioned right of
         * the display being panned.
         */
        if ((other->cur_mode->position_type == CONF_ADJ_RIGHTOF) &&
            other->cur_mode->relative_to == display) {
            bh = NULL;
        }
        if ((display->cur_mode->position_type == CONF_ADJ_LEFTOF) &&
            display->cur_mode->relative_to == other) {
            bh = NULL;
        }
        if (display->screen && other->screen &&
            (other->screen->position_type == CONF_ADJ_RIGHTOF) &&
            (other->screen->relative_to == display->screen)) {
            bh = NULL;
        }
        if (display->screen && other->screen &&
            (display->screen->position_type == CONF_ADJ_LEFTOF) &&
            (display->screen->relative_to == other->screen)) {
            bh = NULL;
        }

        /* Don't snap to other displays that are positioned below of
         * the display being panned.
         */
        if ((other->cur_mode->position_type == CONF_ADJ_BELOW) &&
            other->cur_mode->relative_to == display) {
            bv = NULL;
        }
        if ((display->cur_mode->position_type == CONF_ADJ_ABOVE) &&
            display->cur_mode->relative_to == other) {
            bv = NULL;
        }
        if (display->screen && other->screen &&
            (other->screen->position_type == CONF_ADJ_BELOW) &&
            (other->screen->relative_to == display->screen)) {
            bv = NULL;
        }
        if (display->screen && other->screen &&
            (display->screen->position_type == CONF_ADJ_ABOVE) &&
            (display->screen->relative_to == other->screen)) {
            bv = NULL;
        }

        /* Snap to other screen panning dimensions */
        snap_side_to_dim(pan,
                         pan_orig,
                         ctk_object->Zorder[z]->cur_mode->pan,
                         ctk_object->snap_strength,
                         bv, bh);

        /* Snap to other screen dimensions */
        snap_side_to_dim(pan,
                         pan_orig,
                         ctk_object->Zorder[z]->cur_mode->dim,
                         ctk_object->snap_strength,
                         bv, bh);
    }


    /* Snap to multiples of the display's dimensions */
    dist = pan_orig[W] % dim[W];
    if (dist <= ctk_object->snap_strength && dist < best_horz) {
        pan[W] = dim[W] * (int)(pan_orig[W] / dim[W]);
        best_horz = dist;
    }
    dist = dim[W] - (pan_orig[W] % dim[W]);
    if (dist <= ctk_object->snap_strength && dist < best_horz) {
        pan[W] = dim[W] * (1 + (int)(pan_orig[W] / dim[W]));
        best_horz = dist;
    }
    dist = abs(pan_orig[H] % dim[H]);
    if (dist <= ctk_object->snap_strength && dist < best_vert) {
        pan[H] = dim[H] * (int)(pan_orig[H] / dim[H]);
        best_vert = dist;
    }
    dist = dim[H] - (pan_orig[H] % dim[H]);
    if (dist <= ctk_object->snap_strength && dist < best_vert) {
        pan[H] = dim[H] * (1 + (int)(pan_orig[H] / dim[H]));
        best_vert = dist;
    }


    /* Snap to the maximum screen dimensions */
    dist = abs((screen->dim[X] + gpu->max_width)
               -(pan_orig[X] + pan_orig[W]));
    if (dist <= ctk_object->snap_strength && dist < best_horz) {
        pan[W] = screen->dim[X] + gpu->max_width - pan_orig[X];
        best_horz = dist;
    }
    dist = abs((screen->dim[Y] + gpu->max_height)
               -(pan_orig[Y] + pan_orig[H]));
    if (dist <= ctk_object->snap_strength && dist < best_vert) {
        pan[H] = screen->dim[Y] + gpu->max_height - pan_orig[Y];
        best_vert = dist;
    }


    return (best_vert <= ctk_object->snap_strength ||
            best_horz <= ctk_object->snap_strength);

} /* snap_pan() */



/** move_selected() **************************************************
 *
 * Moves the selected display device by the given offset, optionally
 * snapping to other displays.
 *
 * If something actually moved, this function returns 1.  Otherwise
 * 0 is returned.
 *
 **/

static int move_selected(CtkDisplayLayout *ctk_object, int x, int y, int snap)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvDisplayPtr display;
    nvGpuPtr gpu;
    nvScreenPtr screen;
    int *pan;
    int *dim;
    int orig_display_position_type;
    int orig_display_pos[2] = { 0, 0 };
    int orig_screen_position_type;
    int orig_metamode_dim[4] = { 0, 0, 0, 0 };
    int post_snap_display_pos[2];
    int snap_dim[4];


    /* Get the dimensions of the display to move */
    display = get_selected(ctk_object);
    if (!display || !display->screen || !display->cur_mode ||
        !display->screen->cur_metamode) {
        return 0;
    }


    /* Keep track of original state to report changes */
    gpu = display->gpu;
    screen = display->screen;

    orig_display_position_type = display->cur_mode->position_type;
    dim = display->cur_mode->dim;
    pan = display->cur_mode->pan;
    orig_display_pos[X] = dim[X];
    orig_display_pos[Y] = dim[Y];

    orig_screen_position_type = screen->position_type;
    orig_metamode_dim[X] = screen->cur_metamode->dim[X];
    orig_metamode_dim[Y] = screen->cur_metamode->dim[Y];
    orig_metamode_dim[W] = screen->cur_metamode->dim[W];
    orig_metamode_dim[H] = screen->cur_metamode->dim[H];


    /* Process TwinView relative position moves */
    if (display->cur_mode->position_type != CONF_ADJ_ABSOLUTE) {
        nvDisplayPtr other = display->cur_mode->relative_to;
        int p_x;
        int p_y;

        if (!other) return 0; // Oops?

        p_x = (ctk_object->mouse_x - ctk_object->img_dim[X]) /
            ctk_object->scale;

        p_y = (ctk_object->mouse_y - ctk_object->img_dim[Y]) /
            ctk_object->scale;

        display->cur_mode->position_type =
            get_point_relative_position(other->cur_mode->dim, p_x, p_y);

        /* In basic mode, make the position apply to all metamodes */
        if (!ctk_object->advanced_mode) {
            nvModePtr mode;
            for (mode = display->modes; mode; mode = mode->next) {
                mode->position_type = display->cur_mode->position_type;
            }
        }

        post_snap_display_pos[X] = dim[X];
        post_snap_display_pos[Y] = dim[Y];

        /* Make sure the screen position does not change */
        reposition_screen(display->screen, ctk_object->advanced_mode);

        goto finish;
    }


    /* Move the display */
    ctk_object->modify_dim[X] += x;
    ctk_object->modify_dim[Y] += y;
    snap_dim[X] = ctk_object->modify_dim[X]; /* Snap from move dim */
    snap_dim[Y] = ctk_object->modify_dim[Y];
    snap_dim[W] = dim[W];
    snap_dim[H] = dim[H];


    /* Snap */
    if (snap && ctk_object->snap_strength) {

        /* Snap our viewport to other screens */
        snap_move(ctk_object, snap_dim);

        /* Snap our panning domain to other screens */
        snap_dim[W] = pan[W];
        snap_dim[H] = pan[H];
        snap_move(ctk_object, snap_dim);
    }
    dim[X] = snap_dim[X];
    dim[Y] = snap_dim[Y];
    pan[X] = dim[X];
    pan[Y] = dim[Y];


    /* Prevent layout from growing too big */
    x = MAX_LAYOUT_WIDTH - pan[W];
    if (dim[X] > x) {
        ctk_object->modify_dim[X] += x - dim[X];
        dim[X] = x;
    }
    y = MAX_LAYOUT_HEIGHT - pan[H];
    if (dim[Y] > y) {
        ctk_object->modify_dim[Y] += y - dim[Y];
        dim[Y] = y;
    }
    x = layout->dim[W] - MAX_LAYOUT_WIDTH;
    if (dim[X] < x) {
        ctk_object->modify_dim[X] += x - dim[X];
        dim[X] = x;
    }
    y = layout->dim[H] - MAX_LAYOUT_HEIGHT;
    if (dim[Y] < y) {
        ctk_object->modify_dim[Y] += y - dim[Y];
        dim[Y] = y;
    }


    /* Prevent screen from growing too big */
    x = screen->cur_metamode->edim[X] + gpu->max_width - pan[W]; 
    if (dim[X] > x) {
        ctk_object->modify_dim[X] += x - dim[X];
        dim[X] = x;
    }
    y = screen->cur_metamode->edim[Y] + gpu->max_height -pan[H];
    if (dim[Y] > y) {
        ctk_object->modify_dim[Y] += y - dim[Y];
        dim[Y] = y;
    }
    x = screen->cur_metamode->edim[X] + screen->cur_metamode->edim[W]
        - gpu->max_width;
    if (dim[X] < x) {
        ctk_object->modify_dim[X] += x - dim[X];
        dim[X] = x;
    }
    y = screen->cur_metamode->edim[Y] + screen->cur_metamode->edim[H]
        - gpu->max_height;
    if (dim[Y] < y) {
        ctk_object->modify_dim[Y] += y - dim[Y];
        dim[Y] = y;
    }


    /* Sync panning position */
    pan[X] = dim[X];
    pan[Y] = dim[Y];


    /* Get the post-snap display position.  If calculating the
     * layout changes the display's position, the modify dim
     * (used to mode the display) should be offset as well.
     */
    post_snap_display_pos[X] = dim[X];
    post_snap_display_pos[Y] = dim[Y];

   
    /* If the display's screen is using absolute positioning, we should
     * check to see if the position of the metamode has changed and if
     * so, offset other metamodes on the screen (hence moving the screen's
     * position.)
     *
     * If the screen is using relative positioning, don't offset
     * metamodes since the screen's position is based on another
     * screen.
     */ 
    if (screen->position_type == CONF_ADJ_ABSOLUTE) {
        resolve_displays_in_screen(screen, 0);
        calc_metamode(screen, screen->cur_metamode);
        x = screen->cur_metamode->dim[X] - orig_metamode_dim[X];
        y = screen->cur_metamode->dim[Y] - orig_metamode_dim[Y];
        
        if (x || y) {
            nvDisplayPtr other;
            nvModePtr mode;
            
            for (other = display->gpu->displays; other; other = other->next) {
                
                /* Other display must be in the same screen */
                if (other->screen != screen) continue;

                for (mode = other->modes; mode; mode = mode->next) {

                    /* Only move non-current modes */
                    if (mode == other->cur_mode) continue;

                    /* Don't move modes that are relative */
                    if (mode->position_type != CONF_ADJ_ABSOLUTE) continue;
                    
                    mode->dim[X] += x;
                    mode->dim[Y] += y;
                    mode->pan[X] = mode->dim[X];
                    mode->pan[Y] = mode->dim[Y];
                }
            }
        }


    /* Process X Screen Relative screen position moves */
    } else if (screen->num_displays == 1) {
        nvScreenPtr other = screen->relative_to;
        int p_x;
        int p_y;
        int type;
        
        p_x = (ctk_object->mouse_x - ctk_object->img_dim[X]) /
            ctk_object->scale;
        
        p_y = (ctk_object->mouse_y - ctk_object->img_dim[Y]) /
            ctk_object->scale;
        
        type = get_point_relative_position(other->cur_metamode->dim, p_x, p_y);
        if (type != CONF_ADJ_RELATIVE) {
            screen->position_type = type;
        }
    }


 finish:

    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);


    /* Update the modify dim if the position of the selected display changed */
    if ((post_snap_display_pos[X] != dim[X] ||
         post_snap_display_pos[Y] != dim[Y])) {
        ctk_object->modify_dim[X] += dim[X] - post_snap_display_pos[X];
        ctk_object->modify_dim[Y] += dim[Y] - post_snap_display_pos[Y];
    }


    /* Did the actual position of the display device change? */
    if ((orig_display_position_type != display->cur_mode->position_type ||
         orig_display_pos[X] != dim[X] ||
         orig_display_pos[Y] != dim[Y] ||
         orig_screen_position_type != screen->position_type ||
         orig_metamode_dim[X] != screen->cur_metamode->dim[X] ||
         orig_metamode_dim[Y] != screen->cur_metamode->dim[Y] ||
         orig_metamode_dim[W] != screen->cur_metamode->dim[W] ||
         orig_metamode_dim[H] != screen->cur_metamode->dim[H])) {
        return 1;
    }

    return 0;

} /* move_selected() */



/** pan_selected() ***************************************************
 *
 * Changes the size of the panning domain of the selected display.
 *
 **/

static int pan_selected(CtkDisplayLayout *ctk_object, int x, int y, int snap)
{
    nvLayoutPtr layout = ctk_object->layout;
    nvDisplayPtr display;
    nvScreenPtr screen;
    nvGpuPtr gpu;
    int *dim;
    int *pan;
    int orig_dim[4];



    /* Get the dimensions of the display to pan */
    display = get_selected(ctk_object);
    if (!display || !display->screen || !display->cur_mode) {
        return 0;
    }
    gpu = display->gpu;
    screen = display->screen;
    dim = display->cur_mode->dim;
    pan = display->cur_mode->pan;
    orig_dim[W] = pan[W];
    orig_dim[H] = pan[H];


    /* Resize the panning */
    ctk_object->modify_dim[W] += x;
    ctk_object->modify_dim[H] += y;


    /* Panning can never be smaller then the display viewport */
    if (ctk_object->modify_dim[W] < dim[W]) {
        ctk_object->modify_dim[W] = dim[W];
    }
    if (ctk_object->modify_dim[H] < dim[H]) {
        ctk_object->modify_dim[H] = dim[H];
    }

    pan[W] = ctk_object->modify_dim[W]; /* Snap from panning dimensions */
    pan[H] = ctk_object->modify_dim[H];


    /* Snap to other screens and dimensions */
    if (snap && ctk_object->snap_strength) {
        snap_pan(ctk_object, pan, dim);
    }


    /* Panning should not cause us to exceed the maximum layout dimensions */
    x = MAX_LAYOUT_WIDTH - pan[X];
    if (pan[W] > x) {
        ctk_object->modify_dim[W] += x - pan[W];
        pan[W] = x;
    }
    y = MAX_LAYOUT_HEIGHT - pan[Y];
    if (pan[H] > y) {
        ctk_object->modify_dim[H] += y - pan[H];
        pan[H] = y;
    }


    /* Panning should not cause us to exceed the maximum screen dimensions */
    x = screen->cur_metamode->edim[X] + gpu->max_width - pan[X];
    if (pan[W] > x) {
        ctk_object->modify_dim[W] += x - pan[W];
        pan[W] = x;
    }
    y = screen->cur_metamode->edim[Y] + gpu->max_height - pan[Y];
    if (pan[H] > y) {
        ctk_object->modify_dim[H] += y - pan[H];
        pan[H] = y;
    }


    /* Panning can never be smaller then the display viewport */
    if (pan[W] < dim[W]) {
        pan[W] = dim[W];
    }
    if (pan[H] < dim[H]) {
        pan[H] = dim[H];
    }


    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);


    if (orig_dim[W] != pan[W] || orig_dim[H] != pan[H]) {
        return 1;
    }

    return 0;

} /* pan_selected() */



/** select_display() *************************************************
 *
 * Moves the specified display to the top of the Zorder.
 *
 **/

static void select_display(CtkDisplayLayout *ctk_object, nvDisplayPtr display)
{
    int z;

    for (z = 0; z < ctk_object->Zcount; z++) {

        /* Find the display in question */
        if (ctk_object->Zorder[z] == display) {
            
            /* Bubble it to the top */
            while (z > 0) {
                ctk_object->Zorder[z] = ctk_object->Zorder[z-1];
                z--;
            }
            ctk_object->Zorder[0] = display;
            ctk_object->Zselected = 1;
            break;
        }
    }

} /* select_display() */



/** select_default_display() *****************************************
 *
 * Select the top left most display
 *
 */

#define DIST_SQR(D) (((D)[X] * (D)[X]) + ((D)[Y] * (D)[Y]))

static void select_default_display(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr display = NULL;
    int z;
    
    for (z = 0; z < ctk_object->Zcount; z++) {

        if (!ctk_object->Zorder[z]->cur_mode) continue;

        if (!display ||
            (DIST_SQR(display->cur_mode->dim) >
             DIST_SQR(ctk_object->Zorder[z]->cur_mode->dim))) {
            display = ctk_object->Zorder[z];
        }
    }

    if (display) {
        select_display(ctk_object, display);
    }

} /* select_default_display() */



/** pushback_display() ***********************************************
 *
 * Moves the specified display to the end of the Zorder
 *
 **/

static void pushback_display(CtkDisplayLayout *ctk_object,
                             nvDisplayPtr display)
{
    int z;

    if (!ctk_object->Zcount) {
        return;
    }

    for (z = 0; z < ctk_object->Zcount; z++) {

        /* Find the display */
        if (ctk_object->Zorder[z] == display) {

            /* Bubble it down */
            while (z < ctk_object->Zcount -1) {
                ctk_object->Zorder[z] = ctk_object->Zorder[z+1];
                z++;
            }
            ctk_object->Zorder[ctk_object->Zcount -1] = display;
            break;
        }
    }

} /* pushback_display() */



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

static char *get_display_tooltip(CtkDisplayLayout *ctk_object,
                                 nvDisplayPtr display)
{ 
    char *tip;


    /* No display given */
    if (!display) {
        return NULL;
    }


    /* Display does not have a screen (not configured) */
    if (!(display->screen)) {
        tip = g_strdup_printf("%s : Disabled (GPU: %s)",
                              display->name, display->gpu->name);


    /* Basic view */
    } else if (!ctk_object->advanced_mode) {
        
        /* Display has no mode */
        if (!display->cur_mode) {
            tip = g_strdup_printf("%s", display->name);
            
            
        /* Display does not have a current modeline (Off) */
        } else if (!(display->cur_mode->modeline)) {
            tip = g_strdup_printf("%s : Off",
                                  display->name);
            
        /* Display has mode/modeline */
        } else {
            float ref = display->cur_mode->modeline->refresh_rate;
            tip = g_strdup_printf("%s : %dx%d @ %.0f Hz",
                                  display->name,
                                  display->cur_mode->modeline->data.hdisplay,
                                  display->cur_mode->modeline->data.vdisplay,
                                  ref);
        }
        

    /* Advanced view */
    } else {


        /* Display has no mode */
        if (!display->cur_mode) {
            tip = g_strdup_printf("%s : (Screen: %d) (GPU: %s)",
                                  display->name,
                                  display->screen->scrnum,
                                  display->gpu->name);
            
        /* Display does not have a current modeline (Off) */
        } else if (!(display->cur_mode->modeline)) {
            tip = g_strdup_printf("%s : Off (Screen: %d) (GPU: %s)",
                                  display->name,
                                  display->screen->scrnum,
                                  display->gpu->name);
            
            /* Display has mode/modeline */
        } else {
            float ref = display->cur_mode->modeline->refresh_rate;
            tip = g_strdup_printf("%s : %dx%d @ %.0f Hz (Screen: %d) "
                                  "(GPU: %s)",
                                  display->name,
                                  display->cur_mode->modeline->data.hdisplay,
                                  display->cur_mode->modeline->data.vdisplay,
                                  ref,
                                  display->screen->scrnum,
                                  display->gpu->name);
        }
    }
            
    return tip;

} /* get_display_tooltip() */



/** get_display_tooltip_under_mouse() ********************************
 *
 * Returns the tooltip text that should be used to give information
 * about the display under the mouse at x, y.
 *
 * The caller should free the string that is returned.
 *
 **/

static char *get_display_tooltip_under_mouse(CtkDisplayLayout *ctk_object,
                                             int x, int y)
{
    int                 z;
    static nvDisplayPtr last_display = NULL;
    
 
    /* Scale and offset x & y so they reside in clickable area */
    x = (x -ctk_object->img_dim[X]) / ctk_object->scale;
    y = (y -ctk_object->img_dim[Y]) / ctk_object->scale;


    /* Find the first display under the cursor */
    z = 0;
    while (z < ctk_object->Zcount) {
        if (ctk_object->Zorder[z]->cur_mode &&
            point_in_dim(ctk_object->Zorder[z]->cur_mode->dim, x, y)) {
            if (ctk_object->Zorder[z] == last_display) {
                return NULL;
            }
            last_display = ctk_object->Zorder[z];
            return get_display_tooltip(ctk_object, ctk_object->Zorder[z]);
        }
        z++;
    }

    /* Check display pannings as a last resort */
    z = 0;
    while (z < ctk_object->Zcount) {
        if (ctk_object->Zorder[z]->cur_mode &&
            point_in_dim(ctk_object->Zorder[z]->cur_mode->pan, x, y)) {
            if (ctk_object->Zorder[z] == last_display) {
                return NULL;
            }
            last_display = ctk_object->Zorder[z];
            return get_display_tooltip(ctk_object, ctk_object->Zorder[z]);
        }
        z++;
    }


    if (last_display) {
        last_display = NULL;
        return g_strdup("*** No Display ***");
    }

    return NULL;

} /* get_display_tooltip_under_mouse() */



/** click_layout() ***************************************************
 *
 * Preforms a click in the layout, possibly selecting a display.
 *
 **/

static int click_layout(CtkDisplayLayout *ctk_object, int x, int y)
{
    int z;
    nvDisplayPtr cur_selected;


    /* Make sure there is something to click */
    if (!ctk_object->Zcount) {
        return 0;
    }


    /* Keep track of the currently selected display */
    cur_selected = ctk_object->Zorder[0];

    /* Assume user didn't actually click inside a display for now */
    ctk_object->clicked_outside = 1;

    /* Push selected screen to the back of Z order */
    ctk_object->Zselected = 0;
    if (ctk_object->select_next) {
        pushback_display(ctk_object, ctk_object->Zorder[0]);
    }

    /* Find the first display under the cursor */
    z = 0;
    while (z < ctk_object->Zcount) {
        if (ctk_object->Zorder[z]->cur_mode &&
            point_in_dim(ctk_object->Zorder[z]->cur_mode->dim, x, y)) {
            select_display(ctk_object, ctk_object->Zorder[z]);
            ctk_object->clicked_outside = 0;
            break;
        }
        z++;
    }

    /* Check if we clicked on a panning domain */
    if (!(ctk_object->Zselected)) {
        z = 0;
        while (z < ctk_object->Zcount) {
            if (ctk_object->Zorder[z]->cur_mode &&
                point_in_dim(ctk_object->Zorder[z]->cur_mode->pan, x, y)) {
                select_display(ctk_object, ctk_object->Zorder[z]);
                ctk_object->clicked_outside = 0;
                break;
            }
            z++;
        }
    }

    /* Reselect the last display */
    if (ctk_object->clicked_outside && cur_selected) {
        select_display(ctk_object, cur_selected);
    }

    /* Sync modify dimensions to the newly selected display */
    sync_modify(ctk_object);

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
                                  int height,
                                  ctk_display_layout_selected_callback selected_callback,
                                  void *selected_callback_data,
                                  ctk_display_layout_modified_callback modified_callback,
                                  void *modified_callback_data)
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

    /* Setup widget properties */
    ctk_object->ctk_config = ctk_config;
    ctk_object->selected_callback = selected_callback;
    ctk_object->selected_callback_data = selected_callback_data;
    ctk_object->modified_callback = modified_callback;
    ctk_object->modified_callback_data = modified_callback_data;

    ctk_object->handle = handle;
    ctk_object->layout = layout;
    calc_layout(layout);
    sync_scaling(ctk_object);
    zorder_layout(ctk_object);
    select_default_display(ctk_object);

    
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
    ctk_object->color_palettes = 
        (GdkColor *)calloc(1, NUM_COLORS * sizeof(GdkColor));
    for (i = 0; i < NUM_COLORS; i++) {
        gdk_color_parse(__palettes_color_names[i],
                        &(ctk_object->color_palettes[i]));
    }


    /* Setup the layout state variables */
    ctk_object->snap_strength = DEFAULT_SNAP_STRENGTH;
    ctk_object->need_swap     = 0;
    ctk_object->select_next   = 0;


    /* Make the drawing area */
    tmp = gtk_drawing_area_new();
    gtk_widget_add_events(tmp,
                          GDK_EXPOSURE_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

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



/** do_swap() ********************************************************
 *
 * Preforms a swap from the back buffer if one is needed.
 *
 **/

static void do_swap(CtkDisplayLayout *ctk_object)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);

    if (ctk_object->need_swap && drawing_area->window && fg_gc) {

        gdk_draw_pixmap(drawing_area->window,
                        fg_gc,
                        ctk_object->pixmap,
                        0,0,
                        0,0,
                        ctk_object->width,
                        ctk_object->height);

        ctk_object->need_swap = 0;
    }

} /* do_swap() */



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

    ctk_object->need_swap = 1;

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

        ctk_object->need_swap = 1;
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

        ctk_object->need_swap = 1;
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
                   display->name,
                   tmp_str);
    g_free(tmp_str);

} /* draw_display() */



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
    nvGpuPtr gpu;
    nvScreenPtr screen;
    int z;


    /* Draw the metamode's effective size */
    gdk_color_parse("#888888", &bg_color);
    gdk_color_parse("#777777", &bd_color);

    gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_ON_OFF_DASH,
                               GDK_CAP_NOT_LAST, GDK_JOIN_ROUND);

    for (gpu = ctk_object->layout->gpus; gpu; gpu = gpu->next) {
        for (screen = gpu->screens; screen; screen = screen->next) {
            draw_rect(ctk_object, screen->cur_metamode->edim, &bg_color, 1);
            draw_rect(ctk_object, screen->cur_metamode->edim,
                      &(ctk_object->fg_color), 0);
         }
    }

    gdk_gc_set_line_attributes(fg_gc, 1, GDK_LINE_SOLID, GDK_CAP_NOT_LAST,
                               GDK_JOIN_ROUND);


    /* Draw display devices from back to front */
    for (z = ctk_object->Zcount - 1; z >= 0; z--) {
        draw_display(ctk_object, ctk_object->Zorder[z]);
        ctk_object->need_swap = 1;
    }


    /* Hilite the selected display device */
    if (ctk_object->Zselected && ctk_object->Zcount) {
        int w, h;
        int size; /* Hilite line size */
        int offset; /* Hilite box offset */
        int *dim;

        dim = ctk_object->Zorder[0]->cur_mode->dim;

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
        if (ctk_object->Zorder[0]->screen) {
            
            // Shows the screen dimensions used to write to the X config file
            gdk_color_parse("#00FF00", &bg_color);
            draw_rect(ctk_object,
                      ctk_object->Zorder[0]->screen->dim,
                      &(bg_color), 0);

            // Shows the effective screen dimensions used in conjunction with
            // display devices that are "off"
            gdk_color_parse("#0000FF", &bg_color);
            draw_rect(ctk_object,
                      ctk_object->Zorder[0]->screen->cur_metamode->dim,
                      &(bg_color), 0);

            // Shows the current screen dimensions used for relative
            // positioning of the screen (w/o displays that are "off")
            gdk_color_parse("#FF00FF", &bg_color);
            draw_rect(ctk_object,
                      ctk_object->Zorder[0]->screen->cur_metamode->edim,
                      &(bg_color), 0);
        }
        */

        /* Uncomment to show unsnapped dimensions */
        //gdk_color_parse("#DD4444", &bg_color);
        //draw_rect(ctk_object, ctk_object->modify_dim, &bg_color, 0);

        ctk_object->need_swap = 1;
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
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkColor color;


    /* Clear to background color */
    gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->bg_color));

    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       TRUE,
                       2,
                       2,
                       drawing_area->allocation.width  -4,
                       drawing_area->allocation.height -4);


    /* Add white trim */
    gdk_color_parse("white", &color);
    gdk_gc_set_rgb_fg_color(fg_gc, &color);
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       FALSE,
                       1,
                       1,
                       drawing_area->allocation.width  -3,
                       drawing_area->allocation.height -3);


    /* Add layout border */
    gdk_gc_set_rgb_fg_color(fg_gc, &(ctk_object->fg_color));
    gdk_draw_rectangle(ctk_object->pixmap,
                       fg_gc,
                       FALSE,
                       0,
                       0,
                       drawing_area->allocation.width  -1,
                       drawing_area->allocation.height -1);

    ctk_object->need_swap = 1;

} /* clear_layout() */



/** draw_all() *******************************************************
 *
 * Clears and redraws the entire layout.
 *
 **/

static void draw_all(CtkDisplayLayout *ctk_object)
{
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkGCValues old_gc_values;

    if (!fg_gc) return;

    /* Redraw everything */
    gdk_gc_get_values(fg_gc, &old_gc_values);

    clear_layout(ctk_object);

    draw_layout(ctk_object);

    gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);

} /* draw_all() */



/** ctk_display_layout_redraw() **************************************
 *
 * Redraw everything in the layout and makes sure the widget is
 * updated.
 *
 **/

void ctk_display_layout_redraw(CtkDisplayLayout *ctk_object)
{
    nvLayoutPtr layout = ctk_object->layout;

    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);
    sync_modify(ctk_object);

    /* Redraw */
    draw_all(ctk_object);

    /* Refresh GUI */
    do_swap(ctk_object);

} /* ctk_display_layout_redraw() */



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
    select_default_display(ctk_object);
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);
    sync_modify(ctk_object);

    /* Redraw */
    draw_all(ctk_object);

    /* Refresh GUI */
    do_swap(ctk_object);

} /* ctk_display_layout_set_layout() */



/** ctk_display_layout_get_selected_display() ************************
 *
 * Returns the selected display.
 *
 **/

nvDisplayPtr ctk_display_layout_get_selected_display(CtkDisplayLayout *ctk_object)
{
    return get_selected(ctk_object);

} /* ctk_display_layout_get_selected_display() */



/** ctk_display_layout_get_selected_screen() *************************
 *
 * Returns the selected screen.
 *
 **/

nvScreenPtr ctk_display_layout_get_selected_screen(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr display = get_selected(ctk_object);
    if (display) {
        return display->screen;
    }
    return NULL;

} /* ctk_display_layout_get_selected_screen() */



/** ctk_display_layout_get_selected_gpu() ****************************
 *
 * Returns the selected gpu.
 *
 **/

nvGpuPtr ctk_display_layout_get_selected_gpu(CtkDisplayLayout *ctk_object)
{
    nvDisplayPtr display = get_selected(ctk_object);
    if (display) {
        return display->gpu;
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
    sync_modify(ctk_object);

    /* Redraw the layout */
    ctk_display_layout_redraw(ctk_object);

} /* ctk_display_layout_set_screen_metamode() */



/** ctk_display_layout_add_screen_metamode() *************************
 *
 * Adds a new metamode to the screen.
 *
 **/

void ctk_display_layout_add_screen_metamode(CtkDisplayLayout *ctk_object,
                                            nvScreenPtr screen)
{
    nvDisplayPtr display = get_selected(ctk_object);
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
    for (display = screen->gpu->displays; display; display = display->next) {
        nvModePtr mode;

        if (display->screen != screen) continue; /* Display not in screen */
        
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
                                               int metamode_idx)
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
    for (display = screen->gpu->displays; display; display = display->next) {

        if (display->screen != screen) continue;
        
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
    ctk_display_layout_set_screen_metamode
        (ctk_object, screen, screen->cur_metamode_idx);

} /* ctk_display_layout_delete_screen_metamode() */



/** ctk_display_layout_set_mode_modeline() ***************************
 *
 * Sets which modeline the mode should use.
 *
 **/

void ctk_display_layout_set_mode_modeline(CtkDisplayLayout *ctk_object,
                                          nvModePtr mode,
                                          nvModeLinePtr modeline)
{
    nvLayoutPtr layout = ctk_object->layout;
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

    /* The metamode that this mode belongs to should now be
     * considered a user metamode.
     */
    if (mode->metamode) {
        mode->metamode->source = METAMODE_SOURCE_NVCONTROL;
    }

    /* Recalculate layout dimensions and scaling */
    calc_layout(layout);
    offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
    recenter_layout(layout);
    sync_scaling(ctk_object);
    sync_modify(ctk_object);


    /* Redraw the layout */
    ctk_display_layout_redraw(ctk_object);

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
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkGCValues old_gc_values;
    int modified = 0;
    nvLayoutPtr layout = ctk_object->layout;


    if (!display) return;

    if (position_type != CONF_ADJ_ABSOLUTE && !relative_to) return;


    /* Backup the foreground color and clear the layout */
    if (fg_gc) {
        gdk_gc_get_values(fg_gc, &old_gc_values);
        
        clear_layout(ctk_object);
    }


    /* XXX When configuring a relative position, make sure
     * all displays that are relative to us become absolute.
     * This is to avoid relationship loops.  Eventually, we'll want
     * to be able to handle weird loops since X does this.
     */

    if (position_type != CONF_ADJ_ABSOLUTE) {
        
        nvDisplayPtr other;

        for (other = display->gpu->displays; other; other = other->next) {

            if (other->screen != display->screen) continue;

            if (!other->cur_mode) continue;

            if (other->cur_mode->relative_to == display) {
                other->cur_mode->position_type = CONF_ADJ_ABSOLUTE;
                other->cur_mode->relative_to = NULL;
            }
        }
    }


    /* Set the new positioning type */
    if (ctk_object->advanced_mode) {
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
        sync_modify(ctk_object);
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
        reposition_screen(display->screen, ctk_object->advanced_mode);

        /* Recalculate the layout */
        calc_layout(layout);
        offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
        recenter_layout(layout);
        sync_scaling(ctk_object);
        sync_modify(ctk_object);
        break;
    }

    
    /* Redraw the layout and reset the foreground color */
    if (fg_gc) {
        draw_layout(ctk_object);

        gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);
        
        do_swap(ctk_object);
    }

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
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkGCValues old_gc_values;
    int modified = 0;


    if (!display) return;


    /* Backup the foreground color */
    if (fg_gc) {
        gdk_gc_get_values(fg_gc, &old_gc_values);

        clear_layout(ctk_object);
    }


    /* Change the panning */
    sync_modify(ctk_object);
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
    

    /* Redraw layout and reset the foreground color */
    if (fg_gc) {
        draw_layout(ctk_object);

        gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);

        do_swap(ctk_object);
    }

} /* ctk_display_layout_set_display_panning() */



/** ctk_display_layout_select_display() ***********************
 *
 * Updates the currently selected display.
 *
 **/

void ctk_display_layout_select_display(CtkDisplayLayout *ctk_object,
                                       nvDisplayPtr display)
{
    if (display) {
        /* Select the new display */
        select_display(ctk_object, display);
    } else if (ctk_object->Zcount) {
        /* Select the new topmost display */
        select_display(ctk_object, ctk_object->Zorder[0]);
    }

} /* ctk_display_layout_select_display() */



/** ctk_display_layout_update_display_count() ************************
 *
 * Updates the number of displays shown in the layout by re-building
 * the Zorder list.
 *
 **/

void ctk_display_layout_update_display_count(CtkDisplayLayout *ctk_object,
                                             nvDisplayPtr display)
{
    /* Update the Z order */
    zorder_layout(ctk_object);

    /* Select the previously selected display */
    ctk_display_layout_select_display(ctk_object, display);

} /* ctk_display_layout_update_display_count() */



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
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkGCValues old_gc_values;
    int modified = 0;
    nvLayoutPtr layout = ctk_object->layout;
    nvGpuPtr gpu;


    if (!screen) return;

    if (position_type != CONF_ADJ_ABSOLUTE && !relative_to) return;


    /* Backup the foreground color and clear the layout */
    if (fg_gc) {
        gdk_gc_get_values(fg_gc, &old_gc_values);
        clear_layout(ctk_object);
    }


    /* XXX When configuring a relative position, make sure
     * all screens that are relative to us become absolute.
     * This is to avoid relationship loops.  Eventually, we'll want
     * to be able to handle weird loops since X does this.
     */

    if (position_type != CONF_ADJ_ABSOLUTE) {
        
        nvScreenPtr other;

        for (gpu = layout->gpus; gpu; gpu = gpu->next) {
            for (other = gpu->screens; other; other = other->next) {
                if (other->relative_to == screen) {
                    /* Make this screen use absolute positioning */
                    switch_screen_to_absolute(other);
                }
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

            /* Make sure this screen use absolute positioning */
            switch_screen_to_absolute(screen);

            /* Do the move by offsetting */
            offset_screen(screen, x_offset, y_offset);
            for (other = screen->gpu->displays; other; other = other->next) {
                if (other->screen != screen) continue;
                offset_display(other, x_offset, y_offset);
            }
            
            /* Recalculate the layout */
            calc_layout(layout);
            offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
            recenter_layout(layout);
            sync_scaling(ctk_object);
            sync_modify(ctk_object);
                        
            /* Report back result of move */
            if (ctk_object->modified_callback &&
                (modified ||
                 x != screen->cur_metamode->edim[X] ||
                 y != screen->cur_metamode->edim[Y])) {
                ctk_object->modified_callback
                    (ctk_object->layout, ctk_object->modified_callback_data);
            }
        }
        break;

    case CONF_ADJ_RELATIVE:
        
        /* Fall Through */
        screen->x_offset = x;
        screen->y_offset = y;
 
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
        calc_layout(layout);
        offset_layout(layout, -layout->dim[X], -layout->dim[Y]);
        recenter_layout(layout);
        sync_scaling(ctk_object);
        sync_modify(ctk_object);
        break;
    }


    /* Redraw the layout and reset the foreground color */
    if (fg_gc) {
        draw_layout(ctk_object);
        
        gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);
        
        do_swap(ctk_object);
    }

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



/** expose_event_callback() ******************************************
 *
 * Handles expose events.
 *
 **/

static gboolean
expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    CtkDisplayLayout *ctk_object = CTK_DISPLAY_LAYOUT(data);


    if (event->count) {
        return TRUE;
    }

    /* Redraw the layout */
    ctk_display_layout_redraw(ctk_object);
    
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

    ctk_object->width = width;
    ctk_object->height = height;
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
    GtkWidget *drawing_area = ctk_object->drawing_area;
    GdkGC *fg_gc = get_widget_fg_gc(drawing_area);
    GdkGCValues old_gc_values;

    static int init = 1;
    static int __modify_panning;

    if (init) {
        init = 0;
        __modify_panning = (event->state & ShiftMask) ? 1 : 0;
    }


    if (ctk_object->last_mouse_x == event->x &&
        ctk_object->last_mouse_y == event->y) {
        return TRUE;
    }

    ctk_object->mouse_x = event->x;
    ctk_object->mouse_y = event->y;


    /* Mouse moved, allow user to reselect the current display */
    ctk_object->select_next = 0;


    /* Backup the foreground color */
    gdk_gc_get_values(fg_gc, &old_gc_values);


    /* Modify screen layout */
    if (ctk_object->button1 && !ctk_object->clicked_outside) {
        int modified = 0;
        int delta_x =
            (event->x - ctk_object->last_mouse_x) / ctk_object->scale;
        int delta_y =
            (event->y - ctk_object->last_mouse_y) / ctk_object->scale;


        clear_layout(ctk_object);

        /* Swap between panning and moving  */
        if (__modify_panning != ((event->state & ShiftMask) ? 1 : 0)) {
            __modify_panning = ((event->state & ShiftMask) ? 1 : 0);
            sync_modify(ctk_object);
        }
        if (!(event->state & ShiftMask) || !ctk_object->advanced_mode) {
            modified = move_selected(ctk_object, delta_x, delta_y, 1);
        } else {
            modified = pan_selected(ctk_object, delta_x, delta_y, 1);
        }

        if (modified && ctk_object->modified_callback) {
            ctk_object->modified_callback(ctk_object->layout,
                                       ctk_object->modified_callback_data);
        }

        draw_layout(ctk_object);


    /* Update the tooltip under the mouse */
    } else {
        char *tip =
            get_display_tooltip_under_mouse(ctk_object, event->x, event->y);

        if (tip) {
            gtk_tooltips_set_tip(ctk_object->tooltip_group,
                                 ctk_object->tooltip_area,
                                 tip, NULL);
            
            gtk_tooltips_force_window(ctk_object->tooltip_group);
            g_free(tip);
        }
    }


    ctk_object->last_mouse_x = event->x;
    ctk_object->last_mouse_y = event->y;


    /* Reset the foreground color */
    gdk_gc_set_values(fg_gc, &old_gc_values, GDK_GC_FOREGROUND);
    

    /* Refresh GUI */
    do_swap(ctk_object);

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
    nvDisplayPtr last_selected; /* Last display selected */
    nvDisplayPtr new_selected;

    /* Scale and offset x & y so they reside in the clickable area */
    int x = (event->x -ctk_object->img_dim[X]) / ctk_object->scale;
    int y = (event->y -ctk_object->img_dim[Y]) / ctk_object->scale;

    static Time time = 0;


    ctk_object->last_mouse_x = event->x;
    ctk_object->last_mouse_y = event->y;


    switch (event->button) {

    /* Select a display device */
    case Button1:
        ctk_object->button1 = 1;
        last_selected = get_selected(ctk_object);


        /* Do the click */
        click_layout(ctk_object, x, y);

        new_selected = get_selected(ctk_object);
        
        /* If the user just clicked on the currently selected display,
         * the next time they click here, we should instead select
         * the next display in the Z order.
         */
        ctk_object->select_next =
            (last_selected == new_selected)?1:0;
        
        if (ctk_object->selected_callback) {
            ctk_object->selected_callback(ctk_object->layout,
                                          ctk_object->selected_callback_data);
        }

        if (last_selected != new_selected) {
            /* Selected new display - Redraw */
            time = event->time;

        } else if (new_selected && time && (event->time - time < 500)) {
            /* Double clicked on display - XXX Could flash display here */
            time = 0;

        } else {
            /* Selected same display - Do noting */
            time = event->time;
        }

        /* Redraw everything */
        draw_all(ctk_object);

        break;

    default:
        break;
    }


    /* Refresh GUI */
    do_swap(ctk_object);
    
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
    

    /* Refresh GUI */
    do_swap(ctk_object);

    return TRUE;

} /* button_release_event_callback() */
