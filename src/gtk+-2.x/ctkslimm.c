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
#include "NvCtrlAttributes.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "msg.h"
#include "parse.h"
#include "common-utils.h"

#include "ctkbanner.h"

#include "ctkslimm.h"
#include "ctkdisplayconfig-utils.h"
#include "ctkhelp.h"
#include "ctkutils.h"
#include "ctkdropdownmenu.h"


/* Static function declarations */
static void setup_display_refresh_dropdown(CtkMMDialog *ctk_object);
static void setup_display_resolution_dropdown(CtkMMDialog *ctk_object);
static void setup_total_size_label(CtkMMDialog *ctk_object);
static void display_refresh_changed(GtkWidget *widget, gpointer user_data);
static void display_resolution_changed(GtkWidget *widget, gpointer user_data);
static void display_config_changed(GtkWidget *widget, gpointer user_data);
static void txt_overlap_activated(GtkWidget *widget, gpointer user_data);
static void validate_screen_size(CtkMMDialog *ctk_object);
static nvDisplayPtr find_active_display(nvLayoutPtr layout);
static nvDisplayPtr intersect_modelines_list(CtkMMDialog *ctk_mmdialog,
                                             nvLayoutPtr layout);
static void remove_duplicate_modelines_from_list(CtkMMDialog *ctk_mmdialog);
static Bool other_displays_have_modeline(nvLayoutPtr layout, 
                                         nvDisplayPtr display,
                                         nvModeLinePtr modeline);
static void populate_dropdown(CtkMMDialog *ctk_mmdialog);


typedef struct DpyLocRec { // Display Location
    int x;
    int y;

} DpyLoc;



static void set_overlap_controls_status(CtkMMDialog *ctk_object)
{
    CtkDropDownMenu *menu;
    gint config_idx, x_displays, y_displays;

    menu = CTK_DROP_DOWN_MENU(ctk_object->mnu_display_config);
    config_idx = ctk_drop_down_menu_get_current_value(menu);

    /* Get grid configuration values from index */
    if (config_idx < ctk_object->num_grid_configs) {
        x_displays = ctk_object->grid_configs[config_idx].columns;
        y_displays = ctk_object->grid_configs[config_idx].rows;
    } else {
        x_displays = y_displays = 0;
    }

    gtk_widget_set_sensitive(ctk_object->spbtn_hedge_overlap,
                             x_displays > 1 ? True : False);
    gtk_widget_set_sensitive(ctk_object->spbtn_vedge_overlap,
                             y_displays > 1 ? True : False);
}

static Bool compute_screen_size_details(CtkMMDialog *ctk_object,
                                        gint rows, gint cols,
                                        gint *width, gint *height)
{
    int h_overlap = 0;
    int v_overlap = 0;

    if (!ctk_object->cur_modeline) {
        return FALSE;
    }

    h_overlap = gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(ctk_object->spbtn_hedge_overlap));
    v_overlap = gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(ctk_object->spbtn_vedge_overlap));

    /* Total X Screen Size Calculation */
    *width  = cols * ctk_object->cur_modeline->data.hdisplay -
              (cols - 1) * h_overlap;
    *height = rows * ctk_object->cur_modeline->data.vdisplay -
              (rows - 1) * v_overlap;

    return TRUE;
}


static Bool compute_valid_screen_size(CtkMMDialog *ctk_object,
                                      int rows, int cols)
{
    gint width, height;

    if (!compute_screen_size_details(ctk_object,
                                     rows, cols,
                                     &width, &height)) {
        return FALSE;
    }

    return (width  < ctk_object->max_screen_width &&
            height < ctk_object->max_screen_height);
}



static Bool compute_screen_size(CtkMMDialog *ctk_object, gint *width,
                                gint *height)
{
    gint config_idx;
    gint x_displays,y_displays;

    CtkDropDownMenu *menu;

    menu = CTK_DROP_DOWN_MENU(ctk_object->mnu_display_config);
    config_idx = ctk_drop_down_menu_get_current_value(menu);

    /* Get grid configuration values from index */
    if (config_idx < ctk_object->num_grid_configs) {
        x_displays = ctk_object->grid_configs[config_idx].columns;
        y_displays = ctk_object->grid_configs[config_idx].rows;
    } else {
        x_displays = y_displays = 0;
    }

    return compute_screen_size_details(ctk_object,
                                       y_displays, x_displays,
                                       width, height);
}



static void validate_screen_size(CtkMMDialog *ctk_object)
{
    gint width, height;
    Bool error = FALSE;
    gchar *err_msg = NULL;
    GtkWidget *button;

    button = ctk_dialog_get_widget_for_response(GTK_DIALOG(ctk_object->dialog),
                                                GTK_RESPONSE_APPLY);

    /* Make sure the screen size is acceptable */
    if (!compute_screen_size(ctk_object, &width, &height)) {
        error = TRUE;
        err_msg = g_strdup("Unknown screen size!");

    } else if ((width > ctk_object->max_screen_width) ||
               (height > ctk_object->max_screen_height)) {
        error = TRUE;
        err_msg = g_strdup_printf("The configured X screen size of %dx%d is \n"
                                  "too large.  The maximum supported size is\n"
                                  "%dx%d.",
                                  width, height,
                                  ctk_object->max_screen_width,
                                  ctk_object->max_screen_height);
    }

    if (button) {
        gtk_widget_set_sensitive(button, !error);
    }

    if (error) {
        GtkWidget *dlg;
        GtkWidget *parent;

        parent = ctk_get_parent_window(GTK_WIDGET(ctk_object));

        dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_WARNING,
             GTK_BUTTONS_OK,
             "%s", err_msg);
            
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        g_free(err_msg);
        return;
    }
}



static void txt_overlap_activated(GtkWidget *widget, gpointer user_data)
{
    CtkMMDialog *ctk_object = (CtkMMDialog *)(user_data);
    /* Update total size label */
    setup_total_size_label(ctk_object);

    ctk_object->ctk_config->pending_config |=
        CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG;
}

static void display_config_changed(GtkWidget *widget, gpointer user_data)
{
    CtkMMDialog *ctk_object = (CtkMMDialog *)(user_data);
    /* Update total size label */
    setup_total_size_label(ctk_object);

    /* Update the sensitivity of the overlap controls */
    set_overlap_controls_status(ctk_object);
}


static void display_refresh_changed(GtkWidget *widget, gpointer user_data)
{
    CtkMMDialog *ctk_object = (CtkMMDialog *)(user_data);
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);
    gint idx;

    /* Get the modeline and display to set */
    idx = ctk_drop_down_menu_get_current_value(menu);

    /* Select the new modeline as current modeline */
    ctk_object->cur_modeline = ctk_object->refresh_table[idx];

    ctk_object->ctk_config->pending_config |=
        CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG;
}


static void display_resolution_changed(GtkWidget *widget, gpointer user_data)
{
    CtkMMDialog *ctk_object = (CtkMMDialog *)(user_data);
    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(widget);

    gint idx;
    nvModeLinePtr modeline;

    /* Get the modeline and display to set */
    idx = ctk_drop_down_menu_get_current_value(menu);
    modeline = ctk_object->resolution_table[idx];

    /* Ignore selecting same resolution */
    if (ctk_object->cur_modeline == modeline ||
        (ctk_object->cur_modeline && modeline &&
         ctk_object->cur_modeline->data.hdisplay == modeline->data.hdisplay &&
         ctk_object->cur_modeline->data.vdisplay == modeline->data.vdisplay)) {
        return;
    }

    /* Select the new modeline as current modeline */
    ctk_object->cur_modeline = modeline;

    /* Adjust H and V overlap maximums and redraw total size label */
    gtk_spin_button_set_range(GTK_SPIN_BUTTON(ctk_object->spbtn_hedge_overlap),
                              -modeline->data.hdisplay,
                              modeline->data.hdisplay);

    gtk_spin_button_set_range(GTK_SPIN_BUTTON(ctk_object->spbtn_vedge_overlap),
                              -modeline->data.vdisplay,
                              modeline->data.vdisplay);

    /* Show size warning if detected before rebuilding grid config options */
    validate_screen_size(ctk_object);

    populate_dropdown(ctk_object);

    setup_total_size_label(ctk_object);

    /* Regenerate the refresh menu */
    setup_display_refresh_dropdown(ctk_object);

    ctk_object->ctk_config->pending_config |=
        CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG;
}



/** setup_total_size_label() *********************************
 *
 * Generates and sets the label showing total X Screen size of all displays
 * combined.
 *
 **/

static void setup_total_size_label(CtkMMDialog *ctk_object)
{
    gint width, height;
    gchar *xscreen_size;


    if (!compute_screen_size(ctk_object, &width, &height)) {
        return;
    }

    xscreen_size = g_strdup_printf("%d x %d", width, height);
    gtk_label_set_text(GTK_LABEL(ctk_object->lbl_total_size), xscreen_size);
    g_free(xscreen_size);

    validate_screen_size(ctk_object);
}



/** setup_display_refresh_dropdown() *********************************
 *
 * Generates the refresh rate dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_refresh_dropdown(CtkMMDialog *ctk_object)
{
    CtkDropDownMenu *menu;
    nvModeLinePtr modeline, m;
    nvModeLineItemPtr item, i;
    float cur_rate; /* Refresh Rate */
    int cur_idx = 0; /* Currently selected modeline */
    int idx;
    int match_cur_modeline;

    gchar *name; /* Modeline's label for the dropdown menu */

    /* Get selection information */
    if (!ctk_object->cur_modeline) {
        goto fail;
    }


    cur_rate = ctk_object->cur_modeline->refresh_rate;


    /* Create the menu index -> modeline pointer lookup table */
    if (ctk_object->refresh_table) {
        free(ctk_object->refresh_table);
        ctk_object->refresh_table_len = 0;
    }
    ctk_object->refresh_table =
        calloc(ctk_object->num_modelines, sizeof(nvModeLinePtr));
    if (!ctk_object->refresh_table) {
        goto fail;
    }


    /* Generate the refresh dropdown */
    menu = CTK_DROP_DOWN_MENU(ctk_object->mnu_display_refresh);
    
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_refresh),
         G_CALLBACK(display_refresh_changed), (gpointer) ctk_object);

    ctk_drop_down_menu_reset(menu);

    /* Make sure cur_modeline can possibly match resolution table */
    match_cur_modeline = 0;
    for (idx = 0; idx < ctk_object->resolution_table_len; idx++) {
        m = ctk_object->resolution_table[idx];
        if (m->data.hdisplay == ctk_object->cur_modeline->data.hdisplay &&
            m->data.vdisplay == ctk_object->cur_modeline->data.vdisplay) {
            match_cur_modeline = 1;
            break;
        }
    }

    /* Generate the refresh rate dropdown from the modelines list */
    for (item = ctk_object->modelines; item; item = item->next) {

        float modeline_rate;
        int count_ref; /* # modelines with similar refresh rates */
        int num_ref;   /* Modeline # in a group of similar refresh rates */

        gchar *extra = NULL;
        gchar *tmp;

        modeline = item->modeline;

        /* Ignore modelines of different resolution than the selected */
        m = NULL;
        if (match_cur_modeline) {
            m = ctk_object->cur_modeline;
        } else {
            m = ctk_object->resolution_table[
                    ctk_object->cur_resolution_table_idx];
        }

        if (m && (modeline->data.hdisplay != m->data.hdisplay ||
                  modeline->data.vdisplay != m->data.vdisplay)) {
            continue;
        }

        modeline_rate = modeline->refresh_rate;

        name = g_strdup_printf("%.0f Hz", modeline_rate);


        /* Get a unique number for this modeline */
        count_ref = 0; /* # modelines with similar refresh rates */
        num_ref = 0;   /* Modeline # in a group of similar refresh rates */
        for (i = ctk_object->modelines; i; i = i->next) {
            nvModeLinePtr m = i->modeline;
            float m_rate = m->refresh_rate;
            gchar *tmp = g_strdup_printf("%.0f Hz", m_rate);

            if (m->data.hdisplay == modeline->data.hdisplay &&
                m->data.vdisplay == modeline->data.vdisplay &&
                !g_ascii_strcasecmp(tmp, name)) {

                count_ref++;
                /* Modelines with similar refresh rates get a unique # (num_ref) */
                if (m == modeline) {
                    num_ref = count_ref; /* This modeline's # */
                }
            }
            g_free(tmp);
        }

        if (num_ref > 1) {
            continue;
        }

        /* Add "DoubleScan" and "Interlace" information */
        
        if (modeline->data.flags & V_DBLSCAN) {
            extra = g_strdup_printf("DoubleScan");
        }

        if (modeline->data.flags & V_INTERLACE) {
            if (extra) {
                tmp = g_strdup_printf("%s, Interlace", extra);
                g_free(extra);
                extra = tmp;
            } else {
                extra = g_strdup_printf("Interlace");
            }
        }

        if (extra) {
            tmp = g_strdup_printf("%s (%s)", name, extra);
            g_free(extra);
            g_free(name);
            name = tmp;
        }
        

        /* Keep track of the selected modeline */
        if (ctk_object->cur_modeline == modeline) {
            cur_idx = ctk_object->refresh_table_len;

        /* Find a close match  to the selected modeline */
        } else if (ctk_object->refresh_table_len &&
                   ctk_object->refresh_table[cur_idx] != ctk_object->cur_modeline) {

            /* Resolution must match */
            if (modeline->data.hdisplay == ctk_object->cur_modeline->data.hdisplay &&
                modeline->data.vdisplay == ctk_object->cur_modeline->data.vdisplay) {

                float prev_rate = ctk_object->refresh_table[cur_idx]->refresh_rate;
                float rate = modeline->refresh_rate;

                /* Found better resolution */
                if (ctk_object->refresh_table[cur_idx]->data.hdisplay != 
                    ctk_object->cur_modeline->data.hdisplay ||
                    ctk_object->refresh_table[cur_idx]->data.vdisplay != 
                    ctk_object->cur_modeline->data.vdisplay) {
                    cur_idx = ctk_object->refresh_table_len;
                }

                /* Found a better refresh rate */
                if (rate == cur_rate && prev_rate != cur_rate) {
                    cur_idx = ctk_object->refresh_table_len;
                }
            }
        }


        /* Add the modeline entry to the dropdown */
        ctk_drop_down_menu_append_item(menu, name, ctk_object->refresh_table_len);
        g_free(name);
        ctk_object->refresh_table[ctk_object->refresh_table_len++] = modeline;
    }

    /* Setup the menu and select the current mode */
    ctk_drop_down_menu_set_current_value(menu, cur_idx);
    gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, True);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_refresh),
         G_CALLBACK(display_refresh_changed), (gpointer) ctk_object);

    return;


    /* Handle failures */
 fail:
    gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, False);


} /* setup_display_refresh_dropdown() */



/** setup_display_resolution_dropdown() ******************************
 *
 * Generates the resolution dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_resolution_dropdown(CtkMMDialog *ctk_object)
{
    CtkDropDownMenu *menu;

    nvModeLineItemPtr item;
    nvModeLinePtr  modeline;
    nvModeLinePtr  cur_modeline = ctk_object->cur_modeline;

    int cur_idx = 0;  /* Currently selected modeline (resolution) */

    /* Create the modeline lookup table for the dropdown */
    if (ctk_object->resolution_table) {
        free(ctk_object->resolution_table);
        ctk_object->resolution_table_len = 0;
    }
    ctk_object->resolution_table =
        calloc((ctk_object->num_modelines + 1), sizeof(nvModeLinePtr));
    if (!ctk_object->resolution_table) {
        goto fail;
    }

    /* Start the menu generation */
    menu = CTK_DROP_DOWN_MENU(ctk_object->mnu_display_resolution);


    item = ctk_object->modelines;
    cur_idx = 0;

    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

    ctk_drop_down_menu_reset(menu);

    /* Generate the resolution menu */
    while (item) {
        nvModeLineItemPtr i;
        nvModeLinePtr m;
        gchar *name;

        modeline = item->modeline;

        /* Find the first resolution that matches the current res W & H */
        i = ctk_object->modelines;
        m = i->modeline;
        while (m != modeline) {
            m = i->modeline;
            if (modeline->data.hdisplay == m->data.hdisplay &&
                modeline->data.vdisplay == m->data.vdisplay) {
                break;
            }
            i = i->next;
        }

        /* Add resolution if it is the first of its kind */
        if (m == modeline) {

            /* Set the current modeline idx if not already set by default */
            if (cur_modeline) {
                if (!IS_NVIDIA_DEFAULT_MODE(cur_modeline) &&
                    cur_modeline->data.hdisplay == modeline->data.hdisplay &&
                    cur_modeline->data.vdisplay == modeline->data.vdisplay) {
                    cur_idx = ctk_object->resolution_table_len;
                }
            }

            name = g_strdup_printf("%dx%d", modeline->data.hdisplay,
                                   modeline->data.vdisplay);
            ctk_drop_down_menu_append_item(menu, name,
                                           ctk_object->resolution_table_len);
            g_free(name);
            ctk_object->resolution_table[ctk_object->resolution_table_len++] =
                modeline;
        }
        item = item->next;
    }

    /* Setup the menu and select the current mode */

    ctk_drop_down_menu_set_current_value(menu, cur_idx);
    ctk_object->cur_resolution_table_idx = cur_idx;

    /* If dropdown has only one item, disable menu selection */
    if (ctk_object->resolution_table_len > 1) {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, True);
    } else {
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, False);
    }

    g_signal_handlers_unblock_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

    return;

    /* Handle failures */
 fail:

    gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, False);

} /* setup_display_resolution_dropdown() */



// Adds the value to the array if it does not already exist
static Bool add_array_value(int array[][2], int max_len, int *cur_len, int val)
{
    int i;

    /* Find the value */
    for (i = 0; i < *cur_len; i++) {
        if (array[i][0] == val) {
            array[i][1]++;
            return TRUE;
        }
    }

    /* Add the value */
    if (*cur_len < max_len) {
        array[*cur_len][0] = val;
        array[*cur_len][1] = 1;
        (*cur_len)++;
        return TRUE;
    }

    /* Value not found and array is full */
    return FALSE;
}



static Bool parse_slimm_layout(CtkMMDialog *ctk_mmdialog,
                               nvLayoutPtr layout,
                               int *hoverlap,
                               int *voverlap)
{
    CtrlTarget *ctrl_target = ctk_mmdialog->ctrl_target;
    ReturnStatus ret;
    char *metamode_str = NULL;
    char *str;
    const char *mode_str;
    const char *tmp;
    char *mode_name = NULL;
    gchar *err_msg = NULL;

    const int max_locs = ctk_mmdialog->num_displays;
    int row_loc[max_locs][2]; // As position, count
    int col_loc[max_locs][2]; // As position, count
    DpyLoc locs[max_locs];    // Location of displays
    int loc_idx;
    int num_locs;
    int rows;
    int cols;

    int i;
    int found;
    nvModeLinePtr *cur_modeline; // Used to assign the current modeline
    
    nvDisplayPtr display = find_active_display(layout);
    if (display == NULL) {
        err_msg = "Active display not found.";
        goto fail;
    }

    /* Point at the display's current modeline so we can patch it */
    cur_modeline = &(display->cur_mode->modeline);
    *cur_modeline = NULL;

    /* Get the current metamode string */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_CURRENT_METAMODE,
                                   &metamode_str);
    if ((ret != NvCtrlSuccess) || !metamode_str) {
        err_msg = "Error querying current MetaMode.";
        goto fail;
    }


    /* Point to the start of the metamodes, skipping any tokens */
    str = strstr(metamode_str, "::");
    if (str) {
        str += 2;
    } else {
        str = metamode_str;
    }

    /* Parse each metamode */
    num_locs = 0;
    mode_str = strtok(str, ",");
    while (mode_str) {

        /* Parse each mode */
        mode_str = parse_skip_whitespace(mode_str);


        /* Skip the display name */
        tmp = strstr(mode_str, ":");
        if (tmp) tmp++;
        tmp = parse_skip_whitespace(tmp);


        /* Read the mode name */
        tmp = parse_read_name(tmp, &mode_name, 0);
        if (!tmp || !mode_name) {
            err_msg = "Failed to parse mode name from MetaMode.";
            goto fail;
        }

        if (!(*cur_modeline)) {
            /* Match the mode name to one of the modelines */
            *cur_modeline = display->modelines;
            while (*cur_modeline) {
                if (!strcmp(mode_name, (*cur_modeline)->data.identifier)) {
                    break;
                }
                *cur_modeline = (*cur_modeline)->next;
            }
        } else if (strcmp(mode_name, (*cur_modeline)->data.identifier)) {
            /* Modes don't all have the same mode name */
            free(mode_name);
            err_msg = "MetaMode using mismatched modes.";
            goto fail;
        }
        free(mode_name);


        /* Read mode for position information */
        found = 0;
        while (*tmp && !found) {
            if (*tmp == '+') {
                if (num_locs >= max_locs) {
                    /* Too many displays, not supported */
                    err_msg = "Too many displays in MetaMode.";
                    goto fail;
                }
                tmp++;
                tmp = parse_read_integer_pair(tmp, 0,
                                              &(locs[num_locs].x),
                                              &(locs[num_locs].y));
                num_locs++;
                found = 1;
            } else {
                tmp++;
            }

            /* Catch errors */
            if (!tmp) {
                err_msg = "Failed to parse location information from "
                    "MetaMode.";
                goto fail;
            }
        }

        /* Assume 0,0 positioning if position info not found */
        if (!found) {
            if (num_locs >= max_locs) {
                /* Too many displays, not supported */
                err_msg = "Too many displays in MetaMode.";
                goto fail;
            }
            tmp++;
            tmp = parse_read_integer_pair(tmp, 0,
                                          &(locs[num_locs].x),
                                          &(locs[num_locs].y));
            num_locs++;
        }
 
        /* Parse next mode */
        mode_str = strtok(NULL, ",");
    }


    /* Make sure we were able to find the current modeline */
    if ( !(*cur_modeline)) {
        err_msg = "Unable to identify current resolution and refresh rate.";
        goto fail;
    }


    // Now that we've parsed all the points, count the number of rows/cols.
    rows = 0;
    cols = 0;

    for (loc_idx = 0; loc_idx < num_locs; loc_idx++) {
        if (!add_array_value(row_loc, max_locs, &rows, locs[loc_idx].y)) {
            err_msg = "Too many rows.";
            goto fail;
        }
        if (!add_array_value(col_loc, max_locs, &cols, locs[loc_idx].x)) {
            err_msg = "Too many columns.";
            goto fail;
        }
    }

    /* Make sure that each row has the same number of columns,
     *  and that each column has the same number of rows
     */
    for (i = 0; i < rows; i++) {
        if (row_loc[i][1] != cols) {
            err_msg = "Rows have varying number of columns.";
            goto fail;
        }
    }
    for (i = 0; i < cols; i++) {
        if (col_loc[i][1] != rows) {
            err_msg = "Columns have varying number of rows.";
            goto fail;
        }
    }

    /* Calculate row overlap */
    *voverlap = 0;
    found = 0;
    if (rows > 1) {
        int loc = row_loc[0][0];
        int best_dist = 0; // Best overlap distance
        for (i = 1; i < rows; i++) {
            int overlap = (row_loc[i][0] - loc);
            int dist = (overlap >= 0) ? overlap : -overlap;
            if (!found || dist < best_dist) {
                best_dist = dist;
                *voverlap = overlap;
                found = 1;
            }
        }
    }
    if (*voverlap > 0) {
        *voverlap = (*cur_modeline)->data.vdisplay - *voverlap;
    } else if (*voverlap < 0) {
        *voverlap += (*cur_modeline)->data.vdisplay;
    }


    /* Calculate column overlap */
    *hoverlap = 0;
    found = 0;
    if (cols > 1) {
        int loc = col_loc[0][0];
        int best_dist = 0; // Best overlap distance
        for (i = 1; i < cols; i++) {
            int overlap = (col_loc[i][0] - loc);
            int dist = (overlap >= 0) ? overlap : -overlap;
            if (!found || dist < best_dist) {
                best_dist = dist;
                *hoverlap = overlap;
                found = 1;
            }
        }
    }
    if (*hoverlap > 0) {
        *hoverlap = (*cur_modeline)->data.hdisplay - *hoverlap;
    } else if (*hoverlap < 0) {
        *hoverlap += (*cur_modeline)->data.hdisplay;
    }

    ctk_mmdialog->parsed_rows = rows;
    ctk_mmdialog->parsed_cols = cols;

    free(metamode_str);
    return TRUE;


 fail:
    *hoverlap = 0;
    *voverlap = 0;

    if (err_msg) {
        nv_warning_msg("Unable to determine current SLI Mosaic Mode "
                       "configuration (will fall back to default): %s\n",
                       err_msg);
    }

    free(metamode_str);

    return FALSE;
}



static void remove_duplicate_modelines_from_list(CtkMMDialog *ctk_mmdialog)
{
    nvModeLineItemPtr iter = ctk_mmdialog->modelines;
    nvModeLineItemPtr tmp;
    if (!iter) {
        return;
    }

    /* Remove nvidia-auto-select modeline first */
    if (IS_NVIDIA_DEFAULT_MODE(iter->modeline)) {
        ctk_mmdialog->modelines = iter->next;
        free(iter);
        ctk_mmdialog->num_modelines--;
    }

    /* Remove duplicate modelines in active display - assuming sorted order*/
    for (iter = ctk_mmdialog->modelines; iter;) {
        if (!iter->next) break;

        if (modelines_match(iter->modeline, iter->next->modeline)) {
            /* next is a duplicate - remove it. */
            tmp = iter->next;
            iter->next = iter->next->next;
            free(tmp);
            ctk_mmdialog->num_modelines--;
        }
        else {
            iter = iter->next;
        }
    }

}



static Bool other_displays_have_modeline(nvLayoutPtr layout,
                                         nvDisplayPtr display,
                                         nvModeLinePtr modeline)
{
    nvGpuPtr gpu;
    nvDisplayPtr d;

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (d = gpu->displays; d; d = d->next_on_gpu) {
            if (display == d) continue;
            if (d->modelines == NULL) continue;
            if (!display_has_modeline(d, modeline)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


static nvDisplayPtr find_active_display(nvLayoutPtr layout)
{
    nvGpuPtr gpu;
    nvDisplayPtr display;

    for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
        for (display = gpu->displays;
             display;
             display = display->next_on_gpu) {
            if (display->modelines) return display;
        }
    }
    return NULL;
}



static void add_modeline_to_list(CtkMMDialog *ctk_mmdialog, nvModeLinePtr m)
{
    nvModeLineItemPtr item;

    if (!m) return;

    item = calloc(1,sizeof(nvModeLineItem));
    item->modeline = m;

    if (!ctk_mmdialog->modelines) {
        ctk_mmdialog->modelines = item;
        ctk_mmdialog->num_modelines = 1;
    } else {
        nvModeLineItemPtr iter = ctk_mmdialog->modelines;
        while (iter->next) {
            iter = iter->next;
        }
        iter->next = item;
        ctk_mmdialog->num_modelines++;
    }
}



static void delete_modelines_list(CtkMMDialog *ctk_mmdialog)
{
    nvModeLineItemPtr item, next;

    if (ctk_mmdialog->modelines == NULL || ctk_mmdialog->num_modelines == 0) {
        return;
    }

    item = ctk_mmdialog->modelines;

    while (item) {
        next = item->next;
        free(item);
        item = next;
    }

    ctk_mmdialog->num_modelines = 0;
    ctk_mmdialog->modelines = NULL;
}



static nvDisplayPtr intersect_modelines_list(CtkMMDialog *ctk_mmdialog,
                                             const nvLayoutPtr layout)
{
    nvDisplayPtr display;
    nvModeLinePtr m;

    /**
     *
     * Only need to go through one active display, and eliminate all modelines
     * in this display that do not exist in other displays (being driven by
     * this or any other GPU)
     *
     */
    display = find_active_display(layout);
    if (display == NULL) return NULL;

    delete_modelines_list(ctk_mmdialog);

    m = display->modelines;
    while (m) {
        if (other_displays_have_modeline(layout, display, m)) {
            add_modeline_to_list(ctk_mmdialog, m);
        }
        m = m->next;
    }

    remove_duplicate_modelines_from_list(ctk_mmdialog);

    return display;
}

#define STEREO_IS_3D_VISION(stereo) \
    (((stereo) == NV_CTRL_STEREO_3D_VISION) || \
     ((stereo) == NV_CTRL_STEREO_3D_VISION_PRO))

static int get_display_stereo_mode(nvDisplayPtr display)
{
    if ((display->screen == NULL) ||
        (display->screen->stereo_supported == FALSE)) {
        return NV_CTRL_STEREO_OFF;
    } else {
        return display->screen->stereo;
    }
}



/*
 * Return number of configs, and assign 'configs' if non-NULL.
 */
static int generate_configs_helper(const int num_displays,
                                   gboolean only_max,
                                   GridConfig *configs)
{
    int i, j, c = 0;

    for (i = 1; i <= num_displays; i++) {
        if (only_max) {
            if (num_displays % i == 0) {
                if (configs) {
                    configs[c].rows = i;
                    configs[c].columns = num_displays / i;
                }
                c++;
            }
        } else {
            for (j = 1; j * i <= num_displays; j++) {
                if (i == 1 && j == 1) {
                    continue;
                }
                if (configs) {
                    configs[c].rows = i;
                    configs[c].columns = j;
                }
                c++;
            }
        }
    }

    return c;
}



static void generate_configs(CtkMMDialog *ctk_mmdialog, gboolean only_max)
{
    int n_configs = generate_configs_helper(ctk_mmdialog->num_displays, only_max,
                                            NULL);

    ctk_mmdialog->grid_configs = calloc(n_configs, sizeof(GridConfig));
    ctk_mmdialog->num_grid_configs = n_configs;

    generate_configs_helper(ctk_mmdialog->num_displays, only_max,
                            ctk_mmdialog->grid_configs);
}



static void populate_dropdown(CtkMMDialog *ctk_mmdialog)
{
    int iter;
    int rows, cols;
    int cur_rows, cur_cols;
    char *tmp;
    gboolean only_max;

    CtkDropDownMenu *menu = CTK_DROP_DOWN_MENU(ctk_mmdialog->mnu_display_config);
    int grid_config_id = ctk_drop_down_menu_get_current_value(menu);

    if (ctk_mmdialog->grid_configs && grid_config_id >= 0) {
        cur_rows = ctk_mmdialog->grid_configs[grid_config_id].rows;
        cur_cols = ctk_mmdialog->grid_configs[grid_config_id].columns;
    } else {
        cur_rows = ctk_mmdialog->parsed_rows;
        cur_cols = ctk_mmdialog->parsed_cols;
    }

    if (ctk_mmdialog->grid_configs) {
        free(ctk_mmdialog->grid_configs);
        ctk_mmdialog->num_grid_configs = 0;
    }
    ctk_drop_down_menu_reset(menu);

    only_max = FALSE;
    if (ctk_mmdialog->chk_all_displays != NULL) {
        only_max = gtk_toggle_button_get_active(
                       GTK_TOGGLE_BUTTON(ctk_mmdialog->chk_all_displays));
    }

    generate_configs(ctk_mmdialog, only_max);


    grid_config_id = 0;
    for (iter = 0; iter < ctk_mmdialog->num_grid_configs; iter++) {
        rows = ctk_mmdialog->grid_configs[iter].rows;
        cols = ctk_mmdialog->grid_configs[iter].columns;

        if (!compute_valid_screen_size(ctk_mmdialog, rows, cols)) {
            /* Skip configs that exceed the max screen size */
            continue;
        }

        tmp = g_strdup_printf("%d x %d grid", rows, cols);

        g_signal_handlers_block_by_func(
            G_OBJECT(ctk_mmdialog->mnu_display_config),
            G_CALLBACK(display_config_changed), (gpointer) ctk_mmdialog);

        ctk_drop_down_menu_append_item(menu, tmp, iter);

        g_signal_handlers_unblock_by_func(
            G_OBJECT(ctk_mmdialog->mnu_display_config),
            G_CALLBACK(display_config_changed), (gpointer) ctk_mmdialog);

        if (cur_rows == rows && cur_cols == cols) {
            grid_config_id = iter;
        }
    }

    ctk_drop_down_menu_set_current_value(menu, grid_config_id);
}



static void restrict_display_config_changed(GtkWidget *widget,
                                            gpointer user_data)
{
    CtkMMDialog *ctk_mmdialog = (CtkMMDialog *)(user_data);

    update_mosaic_dialog_ui(ctk_mmdialog, NULL);
}



static void print_error_string(gchar *err_str)
{
    if (!err_str) {
        nv_error_msg("Unable to load SLI Mosaic Mode Settings dialog.");
    } else {
        nv_error_msg("Unable to load SLI Mosaic Mode Settings "
                     "dialog:\n\n%s", err_str);
    }
}



static nvDisplayPtr setup_display(CtkMMDialog *ctk_mmdialog)
{
    nvDisplayPtr display;
    nvLayoutPtr layout = ctk_mmdialog->layout;

    display = intersect_modelines_list(ctk_mmdialog, layout);

    if (display == NULL) {
        print_error_string("Unable to find active display with "
                           "intersected modelines.");
        free(ctk_mmdialog);
        return NULL;

    } else if ((ctk_mmdialog->modelines == NULL)) {
        /* The modepool for the active display did not have any modes in
         * its modepool matching any of the modes on the modepool of any
         * other display in the layout, causing intersect_modelines to
         * remove every mode from the list of available modes for SLI mosaic
         * mode.
         *
         * This can happen if one display had its modepool trimmed and modified
         * to support 3D vision, while other displays (either on X screens
         * without stereo currently enabled, or on screenless GPUs) did not.
         * Find if that is the case, and display an informative message if so.
         */
        nvGpuPtr gpu;
        nvDisplayPtr d;
        int stereo = get_display_stereo_mode(display);

        for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
            for (d = gpu->displays; d; d = d->next_on_gpu) {
                int other_stereo;

                if (display == d) {
                    continue;
                }

                other_stereo = get_display_stereo_mode(d);

                if ((STEREO_IS_3D_VISION(stereo) &&
                    !STEREO_IS_3D_VISION(other_stereo)) ||
                    (!STEREO_IS_3D_VISION(stereo) &&
                     STEREO_IS_3D_VISION(other_stereo))) {

                    print_error_string("Unable to find common modelines between\n"
                                       "all connected displays due to 3D vision\n"
                                       "being enabled on some displays and not\n"
                                       "others. Please make sure that 3D vision\n"
                                       "is enabled on all connected displays\n"
                                       "before enabling SLI mosaic mode.");

                    free(ctk_mmdialog);
                    return NULL;
                }
            }
        }

        /* The intersected modepool was empty, but not because of a mismatch
         * in 3D Vision settings.
         */
        print_error_string("Unable to find find common modelines between "
                           "all connected displays.");

        free(ctk_mmdialog);
        return NULL;
    }

    if (display) {

        /* Extract modelines and cur_modeline */
        if (display->cur_mode->modeline) {
            ctk_mmdialog->cur_modeline = display->cur_mode->modeline;
        } else if (ctk_mmdialog->num_modelines > 0) {
            ctk_mmdialog->cur_modeline = ctk_mmdialog->modelines->modeline;
        }
    }

    return display;
}



void update_mosaic_dialog_ui(CtkMMDialog *ctk_mmdialog, nvLayoutPtr layout)
{
    nvModeLineItemPtr iter;
    char *id;

    if (ctk_mmdialog == NULL) {
        return;
    }

    if (layout) {
        ctk_mmdialog->layout = layout;
    }

    parse_slimm_layout(ctk_mmdialog,
                       ctk_mmdialog->layout,
                       &ctk_mmdialog->h_overlap_parsed,
                       &ctk_mmdialog->v_overlap_parsed);

    id = g_strdup(ctk_mmdialog->cur_modeline->data.identifier);

    setup_display(ctk_mmdialog);

    iter = ctk_mmdialog->modelines;
    while (iter->next) {
        if (strcmp(id, iter->modeline->data.identifier) == 0) {
            ctk_mmdialog->cur_modeline = iter->modeline;
            break;
        } else {
            iter = iter->next;
        }
    }
    g_free(id);

    populate_dropdown(ctk_mmdialog);

    setup_display_resolution_dropdown(ctk_mmdialog);
    setup_display_refresh_dropdown(ctk_mmdialog);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctk_mmdialog->spbtn_hedge_overlap),
                              ctk_mmdialog->h_overlap_parsed);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ctk_mmdialog->spbtn_vedge_overlap),
                              ctk_mmdialog->v_overlap_parsed);
    setup_total_size_label(ctk_mmdialog);
}



static gchar *count_displays_and_parse_layout(CtkMMDialog *ctk_mmdialog)
{
    nvLayoutPtr layout = ctk_mmdialog->layout;
    gchar *err_str = NULL;

    if (layout) {

        nvGpuPtr gpu;
        int num_displays = 0;

        for (gpu = layout->gpus; gpu; gpu = gpu->next_in_layout) {
            num_displays += gpu->num_displays;
        }

        ctk_mmdialog->num_displays = num_displays;


        /* Make sure we have enough displays for the minimum config */
        if (num_displays < 2) {
            err_str = g_strdup_printf("Not enough display devices to "
                                      "configure SLI Mosaic Mode.\nYou must "
                                      "have at least 2 Displays connected, "
                                      "but only %d Display%s detected.",
                                      num_displays,
                                      (num_displays != 1) ? "s were" : " was");
        } else {
            parse_slimm_layout(ctk_mmdialog,
                               layout,
                               &ctk_mmdialog->h_overlap_parsed,
                               &ctk_mmdialog->v_overlap_parsed);
        }
    }

    return err_str;
}



CtkMMDialog *create_mosaic_dialog(GtkWidget *parent,
                                  CtrlTarget *ctrl_target,
                                  CtkConfig *ctk_config,
                                  const nvLayoutPtr layout)
{
    GtkWidget *dialog_obj;
    CtkMMDialog *ctk_mmdialog;
    GtkWidget *content;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *checkbutton;
    GtkWidget *hseparator;
    GtkWidget *table;

    GtkWidget *spinbutton;
    CtkDropDownMenu *menu;

    gchar *err_str = NULL;
    gchar *tmp;
    gchar *sli_mode = NULL;
    ReturnStatus ret;
    ReturnStatus ret1;
    int major = 0, minor = 0;
    gint val;

    nvDisplayPtr display;

    Bool trust_slimm_available = FALSE;
    Bool only_max;


    /* now, create the dialog */

    ctk_mmdialog = calloc(1, sizeof(CtkMMDialog));
    if (!ctk_mmdialog) {
        return NULL;
    }

    ctk_mmdialog->ctrl_target = ctrl_target;
    ctk_mmdialog->ctk_config = ctk_config;

    ctk_mmdialog->parent = parent;
    ctk_mmdialog->layout = layout;

    ctk_mmdialog->is_active = FALSE;

    /*
     * Check for NV-CONTROL protocol version.
     * This is used to not trust old X drivers which always reported
     * it available (on NV50+).
     */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_ATTR_NV_MAJOR_VERSION, &major);
    ret1 = NvCtrlGetAttribute(ctrl_target,
                              NV_CTRL_ATTR_NV_MINOR_VERSION, &minor);

    if ((ret == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        ((major > 1) || ((major == 1) && (minor > 23)))) {
        trust_slimm_available = TRUE;
    }

    /* return on old X drivers. */
    if (!trust_slimm_available) {
        free(ctk_mmdialog);
        return NULL;
    }

    /* Check if this screen supports SLI Mosaic Mode */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE, &val);
    if ((ret == NvCtrlSuccess) &&
        (val == NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE_FALSE)) {
        /* Mosaic not supported */
        free(ctk_mmdialog);
        return NULL;
    }

    /* Query the maximum screen sizes */
    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_MAX_SCREEN_WIDTH,
                             &ctk_mmdialog->max_screen_width);
    if (ret != NvCtrlSuccess) {
        free(ctk_mmdialog);
        return NULL;
    }

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_MAX_SCREEN_HEIGHT,
                             &ctk_mmdialog->max_screen_height);
    if (ret != NvCtrlSuccess) {
        free(ctk_mmdialog);
        return NULL;
    }

    /*
     * Create the display configuration widgets
     *
     */

    err_str = count_displays_and_parse_layout(ctk_mmdialog);


    /* If we failed to load, tell the user why */
    if (err_str || !layout) {
        print_error_string(err_str);
        g_free(err_str);
        free(ctk_mmdialog);
        return NULL;
    }

    display = setup_display(ctk_mmdialog);

    if (display == NULL) {
        return NULL;
    }

    /* Create the dialog */
    dialog_obj = gtk_dialog_new_with_buttons
        ("Configure SLI Mosaic Layout",
         GTK_WINDOW(gtk_widget_get_parent(GTK_WIDGET(parent))),
         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
         "_Apply to Layout",
         GTK_RESPONSE_APPLY,
         GTK_STOCK_CANCEL,
         GTK_RESPONSE_CANCEL,
         NULL);
    ctk_mmdialog->dialog = dialog_obj;

    gtk_dialog_set_default_response(GTK_DIALOG(dialog_obj),
                                    GTK_RESPONSE_REJECT);


    /* set container properties of the object */

    content = ctk_dialog_get_content_area(GTK_DIALOG(dialog_obj));
    gtk_box_set_spacing(GTK_BOX(content), 10);
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(content), vbox);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Display Configuration (rows x columns)");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


    hbox = gtk_hbox_new(FALSE, 0);

    /* Option menu for Display Grid Configuration */
    menu = (CtkDropDownMenu *)
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    ctk_mmdialog->mnu_display_config = GTK_WIDGET(menu);

    only_max = (ctk_mmdialog->parsed_rows * ctk_mmdialog->parsed_cols ==
                ctk_mmdialog->num_displays);

    checkbutton = gtk_check_button_new_with_label("Only show configurations "
                                                  "using all displays");
    ctk_mmdialog->chk_all_displays = checkbutton;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), only_max);
    g_signal_connect(G_OBJECT(checkbutton), "toggled",
                     G_CALLBACK(restrict_display_config_changed),
                     (gpointer) ctk_mmdialog);

    populate_dropdown(ctk_mmdialog);

    g_signal_connect(G_OBJECT(ctk_mmdialog->mnu_display_config), "changed",
                     G_CALLBACK(display_config_changed),
                     (gpointer) ctk_mmdialog);

    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(menu), TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(checkbutton), FALSE, FALSE, 5);

    table = gtk_table_new(20, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);
    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Resolution (per display)");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 2, 3, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Refresh Rate");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 2, 3, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);


    /* Option menu for resolutions */
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_mmdialog->mnu_display_resolution =
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);

    /* Create a drop down menu */
    setup_display_resolution_dropdown(ctk_mmdialog);
    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_mmdialog->mnu_display_resolution, 
                     TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);
    g_signal_connect(G_OBJECT(ctk_mmdialog->mnu_display_resolution), "changed",
                     G_CALLBACK(display_resolution_changed),
                     (gpointer) ctk_mmdialog);


    /* Option menu for refresh rates */
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_mmdialog->mnu_display_refresh =
        ctk_drop_down_menu_new(CTK_DROP_DOWN_MENU_FLAG_READONLY);
    setup_display_refresh_dropdown(ctk_mmdialog);
    g_signal_connect(G_OBJECT(ctk_mmdialog->mnu_display_refresh), "changed",
                     G_CALLBACK(display_refresh_changed),
                     (gpointer) ctk_mmdialog);

    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_mmdialog->mnu_display_refresh, TRUE, TRUE, 0);

    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 3, 4, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    /* Edge Overlap section */
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Edge Overlap");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 8, 9, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Total Size");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 8, 9, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);


    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Horizontal:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    spinbutton = gtk_spin_button_new_with_range(
                     -ctk_mmdialog->cur_modeline->data.hdisplay,
                     ctk_mmdialog->cur_modeline->data.hdisplay,
                     1);

    ctk_mmdialog->spbtn_hedge_overlap = spinbutton;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton),
                              ctk_mmdialog->h_overlap_parsed);

    g_signal_connect(G_OBJECT(ctk_mmdialog->spbtn_hedge_overlap), "value-changed",
                     G_CALLBACK(txt_overlap_activated),
                     (gpointer) ctk_mmdialog);

    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 5);

    label = gtk_label_new("pixels");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 9, 10, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);


    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Vertical:    ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    spinbutton = gtk_spin_button_new_with_range(-ctk_mmdialog->cur_modeline->data.vdisplay, 
                                                ctk_mmdialog->cur_modeline->data.vdisplay, 
                                                1);
    ctk_mmdialog->spbtn_vedge_overlap = spinbutton;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton),
                              ctk_mmdialog->v_overlap_parsed);

    g_signal_connect(G_OBJECT(ctk_mmdialog->spbtn_vedge_overlap), "value-changed",
                     G_CALLBACK(txt_overlap_activated),
                     (gpointer) ctk_mmdialog);

    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 5);

    label = gtk_label_new("pixels");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 10, 11, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    label = gtk_label_new("NULL");
    ctk_mmdialog->lbl_total_size = label;
    setup_total_size_label(ctk_mmdialog);

    hbox = gtk_hbox_new(FALSE, 0);
    ctk_mmdialog->box_total_size = hbox;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 9, 10, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Maximum Size");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 10, 11, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    tmp = g_strdup_printf("%dx%d", ctk_mmdialog->max_screen_width,
                          ctk_mmdialog->max_screen_height);
    label = gtk_label_new(tmp);
    g_free(tmp);
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 11, 12, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);


    /* If current SLI Mode != Mosaic, disable UI elements initially */
    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_SLI_MODE,
                                   &sli_mode);

    set_overlap_controls_status(ctk_mmdialog);

    ctk_mmdialog->ctk_config->pending_config &=
        ~CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG;

    free(sli_mode);

    gtk_widget_show_all(content);
    return ctk_mmdialog;
}



void ctk_mmdialog_insert_help(GtkTextBuffer *b, GtkTextIter *i)
{
    ctk_help_heading(b, i, "Configure SLI Mosaic Layout Dialog");

    ctk_help_para(b, i, "This dialog allows easy configuration "
                  "of SLI Mosaic Mode.");
    
    ctk_help_heading(b, i, "Display Configuration");
    ctk_help_para(b, i, "This drop down menu allows selection of the display grid "
                  "configuration for SLI Mosaic Mode; the possible configurations "
                  "are described as rows x columns. Configurations that exceed "
                  "the maximum screen dimensions will be omitted from the list "
                  "of options.");
    
    ctk_help_heading(b, i, "Resolution");
    ctk_help_para(b, i, "This drop down menu allows selection of the resolution to "
                  "use for each of the displays in SLI Mosaic Mode.  Note that only "
                  "the resolutions that are available for each display will be "
                  "shown here.");
    
    ctk_help_heading(b, i, "Refresh Rate");
    ctk_help_para(b, i, "This drop down menu allows selection of the refresh rate "
                  "to use for each of the displays in SLI Mosaic Mode.  By default "
                  "the highest refresh rate each of the displays can achieve at "
                  "the selected resolution is chosen.  This combo box gets updated "
                  "when a new resolution is picked.");

    ctk_help_heading(b, i, "Edge Overlap");
    ctk_help_para(b, i, "These two controls allow the user to specify the "
                  "Horizontal and Vertical Edge Overlap values.  The displays "
                  "will overlap by the specified number of pixels when forming "
                  "the grid configuration.  For example, 4 flat panel displays "
                  "forming a 2 x 2 grid in SLI Mosaic Mode with a resolution of "
                  "1600x1200 and a Horizontal and Vertical Edge overlap of 50 "
                  "will generate the following MetaMode: \"1600x1200+0+0,"
                  "1600x1200+1550+0,1600x1200+0+1150,1600x1200+1550+1150\".");

    ctk_help_heading(b, i, "Total Size");
    ctk_help_para(b, i, "This is the total size of the X screen formed using all "
                  "displays in SLI Mosaic Mode.");

    ctk_help_heading(b, i, "Maximum Size");
    ctk_help_para(b, i, "This is the maximum allowable size of the X screen "
                  "formed using all displays in SLI Mosaic Mode.");

}



int run_mosaic_dialog(CtkMMDialog *ctk_mmdialog, GtkWidget *parent,
                      const nvLayoutPtr layout)
{
    gint response;
    gint x_displays, y_displays;
    CtkDropDownMenu *menu;
    gint idx;
    gint h_overlap;
    gint v_overlap;


    ctk_mmdialog->layout = layout;

    /* Show the save dialog */
    gtk_window_set_transient_for
        (GTK_WINDOW(ctk_mmdialog->dialog),
         GTK_WINDOW(gtk_widget_get_toplevel(parent)));

    gtk_window_resize(GTK_WINDOW(ctk_mmdialog->dialog), 350, 1);
    gtk_window_set_resizable(GTK_WINDOW(ctk_mmdialog->dialog), FALSE);
    gtk_widget_show(ctk_mmdialog->dialog);

    ctk_mmdialog->is_active = TRUE;
    response = gtk_dialog_run(GTK_DIALOG(ctk_mmdialog->dialog));
    ctk_mmdialog->is_active = FALSE;

    gtk_widget_hide(ctk_mmdialog->dialog);

    if (response == GTK_RESPONSE_ACCEPT ||
        response == GTK_RESPONSE_APPLY) {


        /* Grid width && Grid height */

        menu = CTK_DROP_DOWN_MENU(ctk_mmdialog->mnu_display_config);
        idx = ctk_drop_down_menu_get_current_value(menu);

        if (idx < ctk_mmdialog->num_grid_configs) {
            x_displays = ctk_mmdialog->grid_configs[idx].columns;
            y_displays = ctk_mmdialog->grid_configs[idx].rows;
        } else {
            x_displays = y_displays = 0;
        }

        ctk_mmdialog->x_displays = x_displays;
        ctk_mmdialog->y_displays = y_displays;


        /* Resolution */

        menu = CTK_DROP_DOWN_MENU(ctk_mmdialog->mnu_display_resolution);
        ctk_mmdialog->resolution_idx =
            ctk_drop_down_menu_get_current_value(menu);


        /* Refresh Rate */

        menu = CTK_DROP_DOWN_MENU(ctk_mmdialog->mnu_display_refresh);
        ctk_mmdialog->refresh_idx = ctk_drop_down_menu_get_current_value(menu);


        /* Edge Overlap */

        h_overlap = gtk_spin_button_get_value_as_int(
                        GTK_SPIN_BUTTON(ctk_mmdialog->spbtn_hedge_overlap));
        v_overlap = gtk_spin_button_get_value_as_int(
                        GTK_SPIN_BUTTON(ctk_mmdialog->spbtn_vedge_overlap));

        ctk_mmdialog->h_overlap = h_overlap;
        ctk_mmdialog->v_overlap = v_overlap;

    }

    return response != GTK_RESPONSE_CANCEL;

}


