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
#include "NvCtrlAttributes.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "msg.h"
#include "parse.h"

#include "ctkbanner.h"

#include "ctkslimm.h"
#include "ctkdisplayconfig-utils.h"
#include "ctkhelp.h"
#include "ctkutils.h"


/* Static function declarations */
static void setup_display_refresh_dropdown(CtkSLIMM *ctk_object);
static void setup_display_resolution_dropdown(CtkSLIMM *ctk_object);
static void setup_total_size_label(CtkSLIMM *ctk_object);
static void display_refresh_changed(GtkWidget *widget, gpointer user_data);
static void display_resolution_changed(GtkWidget *widget, gpointer user_data);
static void display_config_changed(GtkWidget *widget, gpointer user_data);
static void txt_overlap_activated(GtkWidget *widget, gpointer user_data);
static void slimm_checkbox_toggled(GtkWidget *widget, gpointer user_data);
static void save_xconfig_button_clicked(GtkWidget *widget, gpointer user_data);
static void add_slimm_options(XConfigPtr xconf, gchar *metamode_str);
static void remove_slimm_options(XConfigPtr xconf);
static nvDisplayPtr find_active_display(nvLayoutPtr layout);
static nvDisplayPtr intersect_modelines(nvLayoutPtr layout);
static void remove_duplicate_modelines(nvDisplayPtr display);
static Bool other_displays_have_modeline(nvLayoutPtr layout, 
                                         nvDisplayPtr display,
                                         nvModeLinePtr modeline);


typedef struct GridConfigRec {
    int rows;
    int columns;
    Bool valid; // Is this layout valid

} GridConfig;

typedef struct DpyLocRec { // Display Location
    int x;
    int y;

} DpyLoc;



/** 
 * The gridConfigs array enumerates the display grid configurations
 * that are presently supported.
 *
 **/

static GridConfig gridConfigs[] = {
    {2, 2, FALSE}, // rows, columns, valid
    {2, 3, FALSE},
    {2, 4, FALSE},
    {3, 1, FALSE},
    {3, 2, FALSE},
    {1, 3, FALSE},
    {2, 1, FALSE},
    {1, 2, FALSE},
    {4, 1, FALSE},
    {1, 4, FALSE},
    {0, 0, FALSE}
};

GType ctk_slimm_get_type()
{
    static GType ctk_slimm_type = 0;

    if (!ctk_slimm_type) {
        static const GTypeInfo info_ctk_slimm = {
            sizeof (CtkSLIMMClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkSLIMM),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_slimm_type =
            g_type_register_static(GTK_TYPE_VBOX,
                                   "CtkSLIMM", &info_ctk_slimm, 0);
    }
    
    return ctk_slimm_type;
}

static void remove_slimm_options(XConfigPtr xconf)
{
    /* Remove SLI Mosaic Option */
    xconfigRemoveNamedOption(&xconf->layouts->adjacencies->screen->options,
                             "SLI", NULL);

    /* Remove MetaMode Option */
    xconfigRemoveNamedOption(&xconf->layouts->adjacencies->screen->options,
                             "MetaModes", NULL);
}



/* get_ith_valid_grid_config()
 * Returns valid grid configuration from gridConfig list.
 */

static GridConfig *get_ith_valid_grid_config(int idx)
{
    int i, count = 0;
    for (i = 0; gridConfigs[i].rows; i++) {
        if (!gridConfigs[i].valid) continue;
        if (count == idx) return &gridConfigs[i];
        count++;
    }
    return NULL;
} /* get_ith_valid_grid_config() */



static void add_slimm_options(XConfigPtr xconf, gchar *metamode_str)
{
    XConfigAdjacencyPtr adj;

    /* Make sure there is only one screen specified in the main layout */
    adj = xconf->layouts->adjacencies;
    while (adj->next) {
        xconfigRemoveListItem((GenericListPtr *)(&adj),
                              (GenericListPtr)adj->next);
    }

    /* 
     * Now fix up the screen in the Device section (to prevent failure with
     * seperate x screen config
     *
    */
    xconf->layouts->adjacencies->screen->device->screen = -1;

    /* Write out SLI Mosaic Option */
    xconfigAddNewOption(&(xconf->layouts->adjacencies->screen->options),
                        "SLI", "Mosaic");

    /* Write out MetaMode Option */
    xconfigAddNewOption(&(xconf->layouts->adjacencies->screen->options),
                        "MetaModes", metamode_str);
}



static XConfigPtr xconfig_generate(XConfigPtr xconfCur,
                                   Bool merge,
                                   Bool *merged,
                                   void *callback_data)
{
    CtkSLIMM *ctk_object = (CtkSLIMM *)callback_data;

    gint idx;

    gint xctr,yctr;

    gint x_displays,y_displays;
    gint h_overlap, v_overlap;

    gint x_total, y_total;

    gchar *metamode_str = NULL;
    gchar *tmpstr;

    gint checkbox_state = 
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ctk_object->cbtn_slimm_enable));


    /* Make sure we're being asked to merge */
    if (!xconfCur || !merge) {
        *merged = FALSE;
        return NULL;
    }


    if (checkbox_state) {
        GridConfig *grid_config;
        /* SLI MM needs to be enabled */
        idx = gtk_option_menu_get_history(GTK_OPTION_MENU(ctk_object->mnu_display_config));

        /* Get grid configuration values from index */

        grid_config = get_ith_valid_grid_config(idx);
        if (grid_config) {
            x_displays = grid_config->columns;
            y_displays = grid_config->rows;
        } else {
            x_displays = y_displays = 0;
        }

        h_overlap = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ctk_object->spbtn_hedge_overlap));
        v_overlap = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ctk_object->spbtn_vedge_overlap));

        /* Total X Screen Size Calculation */
        x_total = x_displays * ctk_object->cur_modeline->data.hdisplay - 
                  (x_displays - 1) * h_overlap;
        y_total = y_displays * ctk_object->cur_modeline->data.vdisplay - 
                  (y_displays - 1) * v_overlap;

        for (yctr = 0; yctr < y_displays;yctr++) {
            for (xctr = 0; xctr < x_displays;xctr++) {
                tmpstr = g_strdup_printf("%s +%d+%d",
                                         ctk_object->cur_modeline->data.identifier,
                                         ctk_object->cur_modeline->data.hdisplay * xctr - 
                                         h_overlap * xctr,
                                         ctk_object->cur_modeline->data.vdisplay * yctr - 
                                         v_overlap * yctr);
                if (metamode_str) {
                    metamode_str = g_strconcat(metamode_str, ", ", tmpstr, NULL);
                    g_free(tmpstr);
                } else {
                    metamode_str = tmpstr;
                }
            }
        }

        add_slimm_options(xconfCur, metamode_str);
    } else {
        /* SLI MM needs to be disabled */

        remove_slimm_options(xconfCur);
    }

    *merged = TRUE;

    return xconfCur;
}



static void save_xconfig_button_clicked(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data); 

    /* Run the dialog */
    run_save_xconfig_dialog(ctk_object->save_xconfig_dlg);

}



static void txt_overlap_activated(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data);
    /* Update total size label */
    setup_total_size_label(ctk_object);
}

static void display_config_changed(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data);
    /* Update total size label */
    setup_total_size_label(ctk_object);
}


static void display_refresh_changed(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data);
    gint idx;

    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));

    /* Select the new modeline as current modeline */
    ctk_object->cur_modeline = ctk_object->refresh_table[idx];
}


static void display_resolution_changed(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data);

    gint idx;
    nvModeLinePtr modeline;

    /* Get the modeline and display to set */
    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(widget));
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

    setup_total_size_label(ctk_object);

    /* Regenerate the refresh menu */
    setup_display_refresh_dropdown(ctk_object);
}



static void slimm_checkbox_toggled(GtkWidget *widget, gpointer user_data)
{
    CtkSLIMM *ctk_object = CTK_SLIMM(user_data);

    gint enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

    if (enabled) {
        if (ctk_object->mnu_refresh_disabled) {
            ctk_object->mnu_refresh_disabled = False;
            gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, True);
        }
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, True);
        gtk_widget_set_sensitive(ctk_object->mnu_display_config, True);
        gtk_widget_set_sensitive(ctk_object->spbtn_hedge_overlap, True);
        gtk_widget_set_sensitive(ctk_object->spbtn_vedge_overlap, True);
        gtk_widget_set_sensitive(ctk_object->box_total_size, True);
    } else {
        if (GTK_WIDGET_SENSITIVE(ctk_object->mnu_display_refresh)) {
            ctk_object->mnu_refresh_disabled = True;
            gtk_widget_set_sensitive(ctk_object->mnu_display_refresh, False);
        }
        gtk_widget_set_sensitive(ctk_object->mnu_display_resolution, False);
        gtk_widget_set_sensitive(ctk_object->mnu_display_config, False);
        gtk_widget_set_sensitive(ctk_object->spbtn_hedge_overlap, False);
        gtk_widget_set_sensitive(ctk_object->spbtn_vedge_overlap, False);
        gtk_widget_set_sensitive(ctk_object->box_total_size, False);
    }
}



/** setup_total_size_label() *********************************
 *
 * Generates and sets the label showing total X Screen size of all displays
 * combined.
 *
 **/

static void setup_total_size_label(CtkSLIMM *ctk_object)
{
    gint idx;
    gint x_displays,y_displays;
    gint h_overlap, v_overlap;
    gchar *xscreen_size;
    gint x_total, y_total;
    GridConfig *grid_config;

    if (!ctk_object->cur_modeline) {
        return;
    }

    idx = gtk_option_menu_get_history(GTK_OPTION_MENU(ctk_object->mnu_display_config));

    /* Get grid configuration values from index */
    grid_config = get_ith_valid_grid_config(idx);
    if (grid_config) {
        x_displays = grid_config->columns;
        y_displays = grid_config->rows;
    } else {
        x_displays = y_displays = 0;
    }
    
    gtk_widget_set_sensitive(ctk_object->spbtn_hedge_overlap,
                             x_displays > 1 ? True : False);
    gtk_widget_set_sensitive(ctk_object->spbtn_vedge_overlap,
                             y_displays > 1 ? True : False);

    h_overlap = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ctk_object->spbtn_hedge_overlap));
    v_overlap = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ctk_object->spbtn_vedge_overlap));

    /* Total X Screen Size Calculation */
    x_total = x_displays * ctk_object->cur_modeline->data.hdisplay - 
              (x_displays - 1) * h_overlap;
    y_total = y_displays * ctk_object->cur_modeline->data.vdisplay - 
              (y_displays - 1) * v_overlap;

    xscreen_size = g_strdup_printf("%d x %d", x_total, y_total);
    gtk_label_set_text(GTK_LABEL(ctk_object->lbl_total_size), xscreen_size);
    g_free(xscreen_size);

}

/** setup_display_refresh_dropdown() *********************************
 *
 * Generates the refresh rate dropdown based on the currently selected
 * display.
 *
 **/

static void setup_display_refresh_dropdown(CtkSLIMM *ctk_object)
{
    GtkWidget *menu;
    GtkWidget *menu_item;
    nvModeLinePtr modeline;
    float cur_rate; /* Refresh Rate */
    int cur_idx = 0; /* Currently selected modeline */

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
        (nvModeLinePtr *)calloc(ctk_object->num_modelines, sizeof(nvModeLinePtr));
    if (!ctk_object->refresh_table) {
        goto fail;
    }


    /* Generate the refresh dropdown */
    menu = gtk_menu_new();

    /* Generate the refresh rate dropdown from the modelines list */
    for (modeline = ctk_object->modelines; modeline; modeline = modeline->next) {

        float modeline_rate;
        nvModeLinePtr m;
        int count_ref; /* # modelines with similar refresh rates */
        int num_ref;   /* Modeline # in a group of similar refresh rates */
        int is_doublescan;
        int is_interlaced;

        gchar *extra = NULL;
        gchar *tmp;

        /* Ignore modelines of different resolution */
        if (modeline->data.hdisplay != ctk_object->cur_modeline->data.hdisplay ||
            modeline->data.vdisplay != ctk_object->cur_modeline->data.vdisplay) {
            continue;
        }

        modeline_rate = modeline->refresh_rate;
        is_doublescan = (modeline->data.flags & V_DBLSCAN);
        is_interlaced = (modeline->data.flags & V_INTERLACE);

        name = g_strdup_printf("%.0f Hz", modeline_rate);


        /* Get a unique number for this modeline */
        count_ref = 0; /* # modelines with similar refresh rates */
        num_ref = 0;   /* Modeline # in a group of similar refresh rates */
        for (m = ctk_object->modelines; m; m = m->next) {
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
        menu_item = gtk_menu_item_new_with_label(name);
        g_free(name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        gtk_widget_show(menu_item);
        ctk_object->refresh_table[ctk_object->refresh_table_len++] = modeline;
    }

    /* Setup the menu and select the current mode */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_refresh),
         G_CALLBACK(display_refresh_changed), (gpointer) ctk_object);

    ctk_object->cur_modeline = ctk_object->refresh_table[cur_idx];

    gtk_option_menu_set_menu(GTK_OPTION_MENU(ctk_object->mnu_display_refresh), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(ctk_object->mnu_display_refresh), cur_idx);
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

static void setup_display_resolution_dropdown(CtkSLIMM *ctk_object)
{
    GtkWidget *menu;
    GtkWidget *menu_item;

    nvModeLinePtr  modeline;
    nvModeLinePtr  cur_modeline = ctk_object->cur_modeline;

    int cur_idx = 0;  /* Currently selected modeline (resolution) */

    /* Create the modeline lookup table for the dropdown */
    if (ctk_object->resolution_table) {
        free(ctk_object->resolution_table);
        ctk_object->resolution_table_len = 0;
    }
    ctk_object->resolution_table =
        (nvModeLinePtr *)calloc((ctk_object->num_modelines + 1),
                                sizeof(nvModeLinePtr));
    if (!ctk_object->resolution_table) {
        goto fail;
    }

    /* Start the menu generation */
    menu = gtk_menu_new();

    modeline = ctk_object->modelines;
    cur_idx = 0;

    /* Generate the resolution menu */

    while (modeline) {
        nvModeLinePtr m;
        gchar *name;

        /* Find the first resolution that matches the current res W & H */
        m = ctk_object->modelines;
        while (m != modeline) {
            if (modeline->data.hdisplay == m->data.hdisplay &&
                modeline->data.vdisplay == m->data.vdisplay) {
                break;
            }
            m = m->next;
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
            menu_item = gtk_menu_item_new_with_label(name);
            g_free(name);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            gtk_widget_show(menu_item);
            ctk_object->resolution_table[ctk_object->resolution_table_len++] =
                modeline;
        }
        modeline = modeline->next;
    }

    /* Setup the menu and select the current mode */
    g_signal_handlers_block_by_func
        (G_OBJECT(ctk_object->mnu_display_resolution),
         G_CALLBACK(display_resolution_changed), (gpointer) ctk_object);

    gtk_option_menu_set_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution), menu);

    gtk_option_menu_set_history
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution), cur_idx);

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

    gtk_option_menu_remove_menu
        (GTK_OPTION_MENU(ctk_object->mnu_display_resolution));

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

static Bool parse_slimm_layout(CtkSLIMM *ctk_slimm,
                               nvLayoutPtr layout,
                               int *hoverlap,
                               int *voverlap,
                               int *grid_config_id)
{
    ReturnStatus ret;
    char *metamode_str = NULL;
    char *str;
    const char *mode_str;
    const char *tmp;
    char *mode_name = NULL;
    gchar *err_msg = NULL;

    static DpyLoc *locs = NULL;  // Location of displays
    static int max_locs = 0; // Maximum number of supported displays basically
    static int max_rows = 0;
    static int max_cols = 0;
    int loc_idx;
    int num_locs;
    int rows;
    int cols;

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


    /* Make space for the display location array */
    if (!locs) {
        for (loc_idx = 0; gridConfigs[loc_idx].rows; loc_idx++ ) {
            if ( max_rows < gridConfigs[loc_idx].rows) {
                max_rows = gridConfigs[loc_idx].rows;
            }
            if ( max_cols < gridConfigs[loc_idx].columns) {
                max_cols = gridConfigs[loc_idx].columns;
            }
            if (max_locs < 
                (gridConfigs[loc_idx].rows * gridConfigs[loc_idx].columns)) {
                max_locs =
                    gridConfigs[loc_idx].rows * gridConfigs[loc_idx].columns;
            }
        }
        locs = (DpyLoc *)malloc(max_locs * sizeof(DpyLoc));
        if (!locs) {
            err_msg = "Out of memory.";
            goto fail;
        }
    }


    /* Get the current metamode string */
    ret = NvCtrlGetStringAttribute(ctk_slimm->handle,
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
    {
        int row_loc[max_rows][2]; // As position, count
        int col_loc[max_cols][2]; // As position, count
        int i;
        int found;

        rows = 0;
        cols = 0;

        for (loc_idx = 0; loc_idx < num_locs; loc_idx++) {
            if (!add_array_value(row_loc, max_rows, &rows,
                                 locs[loc_idx].y)) {
                err_msg = "Too many rows.";
                goto fail;
            }
            if (!add_array_value(col_loc, max_cols, &cols,
                                 locs[loc_idx].x)) {
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
        
        /* Make sure this is a known/supported grid config */
        found = 0;
        for (i = 0; gridConfigs[i].rows; i++) {
            if ((gridConfigs[i].rows == rows) &&
                (gridConfigs[i].columns == cols)) {
                *grid_config_id = i;
                found = 1;
                break;
            }
        }
        if (!found) {
            err_msg = "Unknown grid configuration.";
            return FALSE;
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
        if (*voverlap > 0) { *voverlap -= (*cur_modeline)->data.vdisplay; }
        if (*voverlap < 0) { *voverlap += (*cur_modeline)->data.vdisplay; }


        /* Calculte column overlap */
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
        if (*hoverlap > 0) { *hoverlap -= (*cur_modeline)->data.hdisplay; }
        if (*hoverlap < 0) { *hoverlap += (*cur_modeline)->data.hdisplay; }
    }
    
    XFree(metamode_str);
    return TRUE;


 fail:
    *hoverlap = 0;
    *voverlap = 0;
    *grid_config_id = 0;
    
    if (err_msg) {
        nv_warning_msg("Unable to determine current SLI Mosaic Mode "
                       "configuration (will fall back to default): %s\n",
                       err_msg);
    }

    if (metamode_str) {
        XFree(metamode_str);
    }

    return FALSE;
}



static void remove_duplicate_modelines(nvDisplayPtr display)
{
    nvModeLinePtr m, nextm;
    m = display->modelines;
    if (!m) {
        return;
    }

    /* Remove nvidia-auto-select modeline first */
    if (IS_NVIDIA_DEFAULT_MODE(m)) {
        display->modelines = m->next;
        if (m == display->cur_mode->modeline) {
            display->cur_mode->modeline = m->next;
        }
        modeline_free(m);
        display->num_modelines--;
    }
    
    /* Remove duplicate modelines in active display - assuming sorted order*/
    for (m = display->modelines; m;) {
        nextm = m->next;
        if (!nextm) break; 
            
        if (modelines_match(m, nextm)) {
            /* nextm is a duplicate - remove it. */
            m->next = nextm->next;
            if (nextm == display->cur_mode->modeline) {
                display->cur_mode->modeline = m;
            }
            modeline_free(nextm);
            display->num_modelines--;
        }
        else {
            m = nextm;
        }
    }

}


static Bool other_displays_have_modeline(nvLayoutPtr layout, 
                                         nvDisplayPtr display,
                                         nvModeLinePtr modeline)
{
    nvGpuPtr gpu;
    nvDisplayPtr d;

    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (d = gpu->displays; d; d = d->next) {
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
    for (gpu = layout->gpus; gpu; gpu = gpu->next) {
        for (display = gpu->displays; display; display = display->next) {
            if (display->modelines) return display;
        }
    }
    return NULL;
}


static nvDisplayPtr intersect_modelines(nvLayoutPtr layout)
{
    nvDisplayPtr display;
    nvModeLinePtr m, prev;

    /** 
     * 
     * Only need to go through one active display, and eliminate all modelines
     * in this display that do not exist in other displays (being driven by
     * this or any other GPU)
     *
     */
    display = find_active_display(layout);
    if (display == NULL) return NULL;

    prev = NULL;
    m = display->modelines;
    while (m) {
        if (!other_displays_have_modeline(layout, display, m)) {
            if (prev) {
                /* Remove past beginning */
                prev->next = m->next;
            } else {
                /* Remove first entry */
                display->modelines = m->next;
            }

            if (m == display->cur_mode->modeline) {
                display->cur_mode->modeline = 0;
            }
            modeline_free(m);
            display->num_modelines--;

            if (prev) {
                m = prev->next;
            } else {
                m = display->modelines;
            }
        } else {
            prev = m;
            m = m->next;
        }
    }

    remove_duplicate_modelines(display);

    return display;
}


GtkWidget* ctk_slimm_new(NvCtrlAttributeHandle *handle,
                          CtkEvent *ctk_event, CtkConfig *ctk_config)
{
    GObject *object;
    CtkSLIMM *ctk_slimm;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *checkbutton;
    GtkWidget *hseparator;
    GtkWidget *table;
    GtkWidget *button;

    GtkWidget *optionmenu, *menu, *menuitem, *spinbutton;
    CtkSLIMM *ctk_object;

    gchar *err_str = NULL;
    gchar *tmp;
    gchar *sli_mode = NULL;
    ReturnStatus ret;
    gint val;

    nvLayoutPtr layout;
    nvDisplayPtr display;

    int iter;
    int grid_menu_selected_id;
    int count;

    Bool valid_layout = FALSE;

    int hoverlap = 0;
    int voverlap = 0;
    int grid_config_id = 0;

    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_SLIMM, NULL);
    ctk_slimm = CTK_SLIMM(object);

    /* cache the attribute handle */

    ctk_slimm->handle = handle;
    ctk_slimm->ctk_config = ctk_config;
    ctk_object = ctk_slimm;
    
    /* Check if this screen supports SLI Mosaic Mode */
    ret = NvCtrlGetAttribute(ctk_object->handle,
                             NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE, &val);
    if ((ret == NvCtrlSuccess) && 
        (val == NV_CTRL_SLI_MOSAIC_MODE_AVAILABLE_FALSE)) {
        /* Mosaic not supported */
        return NULL;
    }

    /*
     * Create the display configuration widgets
     *
     */

    /* Load the layout structure from the X server */
    layout = layout_load_from_server(handle, &err_str);

    if (!err_str && layout) {
        nvGpuPtr gpu;
        int num_displays = 0;
        int min_displays = 0;
        GridConfig *grid;
        int num_valid = 0;

        for (gpu = layout->gpus; gpu; gpu = gpu->next) {
            num_displays += gpu->num_displays;
        }

        /* Mark configs that have enough displays as valid */
        for (grid = gridConfigs; grid->rows; grid++) {
            if (!min_displays ||
                (min_displays > (grid->rows * grid->columns))) {
                min_displays = grid->rows * grid->columns;
            }
            if (num_displays >= (grid->rows * grid->columns)) {
                grid->valid = TRUE;
                num_valid++;
            }
        }
        
        /* Make sure we have enough dislays for the minimum config */
        if (num_valid <= 0) {
            err_str = g_strdup_printf("Not enough display devices to "
                                      "configure SLI Mosaic Mode.\nYou must "
                                      "have at least %d Display%s connected, "
                                      "but only %d Display%s detected.",
                                      min_displays, 
                                      (min_displays != 1) ? "s" : "",
                                      num_displays, 
                                      (num_displays != 1) ? "s were" : " was");
            layout_free(layout);
            layout = NULL;

        } else {
            valid_layout = parse_slimm_layout(ctk_slimm,
                                              layout,
                                              &hoverlap,
                                              &voverlap,
                                              &grid_config_id);
        }
    }
    

    /* If we failed to load, tell the user why */
    if (err_str || !layout) {
        gchar *str;

        if (!err_str) {
            str = g_strdup("Unable to load SLI Mosaic Mode Settings page.");
        } else {
            str = g_strdup_printf("Unable to load SLI Mosaic Mode Settings "
                                  "page:\n\n%s", err_str);
            g_free(err_str);
        }

        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_container_add(GTK_CONTAINER(object), label);

        /* Show the GUI */
        gtk_widget_show_all(GTK_WIDGET(ctk_object));

        return GTK_WIDGET(ctk_object);
    }

    display = intersect_modelines(layout);

    if (display == NULL) {
        gchar *str = g_strdup("Unable to find active display with "
                              "intersected modelines.");
        label = gtk_label_new(str);
        g_free(str);
        gtk_label_set_selectable(GTK_LABEL(label), TRUE);
        gtk_container_add(GTK_CONTAINER(object), label);

        /* Show the GUI */
        gtk_widget_show_all(GTK_WIDGET(ctk_object));

        return GTK_WIDGET(ctk_object);
    }


    /* Extract modelines and cur_modeline and free layout structure */
    ctk_object->modelines = display->modelines;
    if (display->cur_mode->modeline) {
        ctk_object->cur_modeline = display->cur_mode->modeline; 
    } else if (ctk_object->modelines) {
        ctk_object->cur_modeline = ctk_object->modelines;
    } else {
        /* This is an error. */
        return NULL;
    }
    ctk_object->num_modelines = display->num_modelines;

    /* XXX Since we've hijacked the layout's modelines,
     *     we can stub out the layout's pointer and free it.
     */
    display->modelines = NULL;
    display->cur_mode->modeline = NULL;
    display->num_modelines = 0;
    layout_free(layout);
    layout = NULL;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_slimm), 10);

    /* banner */

    banner = ctk_banner_image_new(BANNER_ARTWORK_SLIMM);
    gtk_box_pack_start(GTK_BOX(ctk_slimm), banner, FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_slimm), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    checkbutton = gtk_check_button_new_with_label("Use SLI Mosaic Mode");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), TRUE);
    ctk_slimm->cbtn_slimm_enable = checkbutton;
    g_signal_connect(G_OBJECT(checkbutton), "toggled", 
                     G_CALLBACK(slimm_checkbox_toggled),
                     (gpointer) ctk_object);
    gtk_box_pack_start(GTK_BOX(hbox), checkbutton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0); 
 
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Display Configuration (rows x columns)");
    hseparator = gtk_hseparator_new();
    gtk_widget_show(hseparator);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    

    hbox = gtk_hbox_new(FALSE, 0);

    /* Option menu for Display Grid Configuration */
    optionmenu = gtk_option_menu_new();
    ctk_slimm->mnu_display_config = optionmenu;
    menu = gtk_menu_new();

    grid_menu_selected_id = 0;
    count = 0;
    for (iter = 0; gridConfigs[iter].rows; iter++) {
        /* Don't show invalid configs */
        if (!gridConfigs[iter].valid) continue;
        tmp = g_strdup_printf("%d x %d grid",
                              gridConfigs[iter].rows,
                              gridConfigs[iter].columns);

        menuitem = gtk_menu_item_new_with_label(tmp);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
        gtk_widget_show(menuitem);

        /* Update grid_config_id to set menu history */
        if (iter == grid_config_id) {
            grid_menu_selected_id = count;
        }
        count++;
    }

    gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);
    gtk_option_menu_set_history(GTK_OPTION_MENU(ctk_slimm->mnu_display_config),
                                grid_menu_selected_id);

    g_signal_connect(G_OBJECT(ctk_object->mnu_display_config), "changed",
                     G_CALLBACK(display_config_changed),
                     (gpointer) ctk_object);

    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), optionmenu, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
 
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
    optionmenu = gtk_option_menu_new();
    ctk_slimm->mnu_display_resolution = optionmenu;

    /* Create a drop down menu */
    setup_display_resolution_dropdown(ctk_object);
    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), ctk_slimm->mnu_display_resolution, 
                     TRUE, TRUE, 0);
    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 3, 4, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_resolution), "changed",
                     G_CALLBACK(display_resolution_changed),
                     (gpointer) ctk_object);


    /* Option menu for refresh rates */
    optionmenu = gtk_option_menu_new();
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_slimm->mnu_display_refresh = optionmenu;
    setup_display_refresh_dropdown(ctk_object);
    g_signal_connect(G_OBJECT(ctk_object->mnu_display_refresh), "changed",
                     G_CALLBACK(display_refresh_changed),
                     (gpointer) ctk_object);

    label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), optionmenu, TRUE, TRUE, 0);

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

    spinbutton = gtk_spin_button_new_with_range(-ctk_object->cur_modeline->data.hdisplay, 
                                                ctk_object->cur_modeline->data.hdisplay, 
                                                1);

    ctk_slimm->spbtn_hedge_overlap = spinbutton;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), hoverlap);

    g_signal_connect(G_OBJECT(ctk_object->spbtn_hedge_overlap), "value-changed",
                     G_CALLBACK(txt_overlap_activated),
                     (gpointer) ctk_object);

    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 5);

    label = gtk_label_new("pixels");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 9, 10, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);


    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("Vertical:    ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);

    spinbutton = gtk_spin_button_new_with_range(-ctk_object->cur_modeline->data.vdisplay, 
                                                ctk_object->cur_modeline->data.vdisplay, 
                                                1);
    ctk_slimm->spbtn_vedge_overlap = spinbutton;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spinbutton), voverlap);

    g_signal_connect(G_OBJECT(ctk_object->spbtn_vedge_overlap), "value-changed",
                     G_CALLBACK(txt_overlap_activated),
                     (gpointer) ctk_object);

    gtk_box_pack_start(GTK_BOX(hbox), spinbutton, FALSE, FALSE, 5);

    label = gtk_label_new("pixels");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

    gtk_table_attach(GTK_TABLE(table), hbox, 0, 1, 10, 11, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    label = gtk_label_new("NULL");
    ctk_slimm->lbl_total_size = label;
    setup_total_size_label(ctk_slimm);

    hbox = gtk_hbox_new(FALSE, 0);
    ctk_slimm->box_total_size = hbox;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 10);
    gtk_table_attach(GTK_TABLE(table), hbox, 1, 2, 9, 10, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0.5, 0.5);

    label = gtk_label_new("Save to X Configuration File");
    hbox = gtk_hbox_new(FALSE, 0);
    button = gtk_button_new();
    ctk_slimm->btn_save_config = button; 
    g_signal_connect(G_OBJECT(ctk_object->btn_save_config), "clicked",
                     G_CALLBACK(save_xconfig_button_clicked),
                     (gpointer) ctk_object);

    ctk_slimm->save_xconfig_dlg =
        create_save_xconfig_dialog(GTK_WIDGET(ctk_slimm),
                                   FALSE, // Merge toggleable
                                   xconfig_generate,
                                   (void *)ctk_slimm);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(button), hbox);

    gtk_table_attach(GTK_TABLE(table), button, 1, 2, 19, 20, GTK_EXPAND | GTK_FILL,
                     GTK_EXPAND | GTK_FILL, 0, 0);

    /* If current SLI Mode != Mosaic, disable UI elements initially */
    ret = NvCtrlGetStringAttribute(ctk_slimm->handle,
                                   NV_CTRL_STRING_SLI_MODE,
                                   &sli_mode);

    if ((ret != NvCtrlSuccess) ||
        (ret == NvCtrlSuccess && g_ascii_strcasecmp(sli_mode, "Mosaic"))) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), FALSE);
        slimm_checkbox_toggled(ctk_slimm->cbtn_slimm_enable, (gpointer) ctk_slimm);
    }

    if (sli_mode) {
        XFree(sli_mode);
    }

    gtk_widget_show_all(GTK_WIDGET(object));    

    return GTK_WIDGET(object);
}


    
GtkTextBuffer *ctk_slimm_create_help(GtkTextTagTable *table,
                                      const gchar *slimm_name)
{
    GtkTextIter i;
    GtkTextBuffer *b;

    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "SLI Mosaic Mode Settings Help");

    ctk_help_para(b, &i, "This page allows easy configuration "
                  "of SLI Mosaic Mode.");
    
    ctk_help_heading(b, &i, "Use SLI Mosaic Mode");
    ctk_help_para(b, &i, "This checkbox controls whether SLI Mosaic Mode is enabled "
                  "or disabled.");
    
    ctk_help_heading(b, &i, "Display Configuration");
    ctk_help_para(b, &i, "This drop down menu allows selection of the display grid "
                  "configuration for SLI Mosaic Mode; the possible configurations "
                  "are described as rows x columns.");
    
    ctk_help_heading(b, &i, "Resolution");
    ctk_help_para(b, &i, "This drop down menu allows selection of the resolution to "
                  "use for each of the displays in SLI Mosaic Mode.  Note that only "
                  "the resolutions that are available for each display will be "
                  "shown here.");
    
    ctk_help_heading(b, &i, "Refresh Rate");
    ctk_help_para(b, &i, "This drop down menu allows selection of the refresh rate "
                  "to use for each of the displays in SLI Mosaic Mode.  By default "
                  "the highest refresh rate each of the displays can achieve at "
                  "the selected resolution is chosen.  This combo box gets updated "
                  "when a new resolution is picked.");

    ctk_help_heading(b, &i, "Edge Overlap");
    ctk_help_para(b, &i, "These two controls allow the user to specify the "
                  "Horizontal and Vertical Edge Overlap values.  The displays "
                  "will overlap by the specified number of pixels when forming "
                  "the grid configuration. For example, 4 flat panel displays "
                  "forming a 2 x 2 grid in SLI Mosaic Mode with a resolution of "
                  "1600x1200 and a Horizontal and Vertical Edge overlap of 50 "
                  "will generate the following MetaMode: \"1600x1200+0+0,"
                  "1600x1200+1550+0,1600x1200+0+1150,1600x1200+1550+1150\".");

    ctk_help_heading(b, &i, "Total Size");
    ctk_help_para(b, &i, "This is the total size of the X screen formed using all "
                  "displays in SLI Mosaic Mode.");

    ctk_help_heading(b, &i, "Save to X Configuration File");
    ctk_help_para(b, &i, "Clicking this button saves the selected SLI Mosaic Mode "
                  "settings into the X Configuration File.");

    ctk_help_finish(b);

    return b;
}


