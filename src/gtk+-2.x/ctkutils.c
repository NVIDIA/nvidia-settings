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

#include <stdlib.h>
#include <gtk/gtk.h>
#ifdef CTK_GTK3
#include <gio/gio.h>
#endif
#include <NvCtrlAttributes.h>
#include "NVCtrlLib.h"
#include "ctkutils.h"
#include "msg.h"

/*
 * GTK 2/3 util functions - These functions are used in nvidia-settings to hide
 * function calls that differ between GTK 2 and GTK 3. The naming convention
 * used is the name of the GTK 3 function, if applicable, with a ctk_ prefix
 * instead of gtk_.
 */

gboolean ctk_widget_is_sensitive(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.18 */
    return gtk_widget_is_sensitive(w);
#else
    /* deprecated in 2.20, removed in 3.0 */
    return GTK_WIDGET_IS_SENSITIVE(w);
#endif
}

gboolean ctk_widget_get_sensitive(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.18 */
    return gtk_widget_get_sensitive(w);
#else
    /* deprecated in 2.20, removed in 3.0 */
    return GTK_WIDGET_SENSITIVE(w);
#endif
}

gboolean ctk_widget_get_visible(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.18 */
    return gtk_widget_get_visible(w);
#else
    /* deprecated in 2.20, removed in 3.0 */
    return GTK_WIDGET_VISIBLE(w);
#endif
}

void ctk_widget_set_visible(GtkWidget *w, gboolean visible)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.18 */
    gtk_widget_set_visible(w, visible);
#else
    if (visible) {
        gtk_widget_show(w);
    } else {
        gtk_widget_hide(w);
    }
#endif
}

gboolean ctk_widget_is_drawable(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.18 */
    return gtk_widget_is_drawable(w);
#else
    /* deprecated in 2.20, removed in 3.0 */
    return GTK_WIDGET_DRAWABLE(w);
#endif
}

GdkWindow *ctk_widget_get_window(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_widget_get_window(w);
#else
    /* direct access removed in 3.0 */
    return w->window;
#endif
}

void ctk_widget_get_allocation(GtkWidget *w, GtkAllocation *a)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    gtk_widget_get_allocation(w, a);
#else
    /* direct access removed in 3.0 */
    a->x = w->allocation.x;
    a->y = w->allocation.y;
    a->width  = w->allocation.width;
    a->height = w->allocation.height;
#endif
}

void ctk_widget_get_preferred_size(GtkWidget *w, GtkRequisition *r)
{
#ifdef CTK_GTK3
    /* GTK function added in 3.0 */
    GtkRequisition min_req;
    gtk_widget_get_preferred_size(w, &min_req, r);
#else
    /* deprecated in 3.0 */
    gtk_widget_size_request(w, r);
#endif
}

gchar *ctk_widget_get_tooltip_text(GtkWidget *w)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.12 */
    return gtk_widget_get_tooltip_text(w);
#else
    /* direct access removed in 3.0 */
    GtkTooltipsData *td = gtk_tooltips_data_get(GTK_WIDGET(w));
    return g_strdup(td->tip_text);
#endif
}

GtkWidget *ctk_dialog_get_content_area(GtkDialog *d)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_dialog_get_content_area(d);
#else
    /* direct access removed in 3.0 */
    return d->vbox;
#endif
}

gdouble ctk_adjustment_get_page_increment(GtkAdjustment *a)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_adjustment_get_page_increment(a);
#else
    /* direct access removed in 3.0 */
    return a->page_increment;
#endif
}

gdouble ctk_adjustment_get_step_increment(GtkAdjustment *a)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_adjustment_get_step_increment(a);
#else
    /* direct access removed in 3.0 */
    return a->step_increment;
#endif
}

gdouble ctk_adjustment_get_page_size(GtkAdjustment *a)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_adjustment_get_page_size(a);
#else
    /* direct access removed in 3.0 */
    return a->page_size;
#endif
}

gdouble ctk_adjustment_get_upper(GtkAdjustment *a)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    return gtk_adjustment_get_upper(a);
#else
    /* direct access removed in 3.0 */
    return a->upper;
#endif
}

void ctk_adjustment_set_upper(GtkAdjustment *a, gdouble x)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    gtk_adjustment_set_upper(a, x);
#else
    /* direct access removed in 3.0 */
    a->upper = x;
#endif
}

void ctk_adjustment_set_lower(GtkAdjustment *a, gdouble x)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.14 */
    gtk_adjustment_set_lower(a, x);
#else
    /* direct access removed in 3.0 */
    a->lower = x;
#endif
}

GtkWidget *ctk_scrolled_window_get_vscrollbar(GtkScrolledWindow *sw)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.8 */
    return gtk_scrolled_window_get_vscrollbar(sw);
#else
    /* direct access removed in 3.0 */
    return sw->vscrollbar;
#endif
}

void ctk_scrolled_window_add(GtkScrolledWindow *sw, GtkWidget *child)
{
#ifdef CTK_GTK3
    gtk_container_add(GTK_CONTAINER(sw), child);
#else
    gtk_scrolled_window_add_with_viewport(sw, child);
#endif
}

GtkWidget *ctk_statusbar_get_message_area(GtkStatusbar *statusbar)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.20 */
    return gtk_statusbar_get_message_area(statusbar);
#else
    /* direct access removed in 3.0 */
    return statusbar->label;
#endif
}

void ctk_g_object_ref_sink(gpointer obj)
{
#ifdef CTK_GTK3
    /* GTK function added in 2.10 */
    g_object_ref_sink(obj);
#else
    /* function removed in 2.10 */
    gtk_object_sink(GTK_OBJECT(obj));
#endif
}

GtkWidget *ctk_combo_box_text_new(void)
{
#ifdef CTK_GTK3
    /* added in 2.24 */
    return gtk_combo_box_text_new();
#else
    /* added in 2.4, deprecated in 2.24, removed in 3.0 */
    return gtk_combo_box_new_text();
#endif
}

GtkWidget *ctk_combo_box_text_new_with_entry()
{
#ifdef CTK_GTK3
    /* added in 2.24 */
    return gtk_combo_box_text_new_with_entry();
#else
    /* added in 2.4, deprecated in 2.24, removed in 3.0 */
    return gtk_combo_box_entry_new_text();
#endif
}

void ctk_combo_box_text_append_text(GtkWidget *widget, const gchar *text)
{
#ifdef CTK_GTK3
    /* added in 2.24 */
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), text);
#else
    /* added in 2.4, deprecated in 2.24, removed in 3.0 */
    gtk_combo_box_append_text(GTK_COMBO_BOX(widget), text);
#endif
}

GtkWidget *ctk_image_new_from_str(const gchar *str, GtkIconSize size)
{
    if (!str) {
        return NULL;
    }

#ifdef CTK_GTK3
    /* added in 2.6 */
    return gtk_image_new_from_icon_name(str, size);
#else
    /* deprecated in 3.10 */
    return gtk_image_new_from_stock(str, size);
#endif
}

GdkPixbuf *ctk_widget_render_icon(GtkWidget *widget, const gchar *stock_id,
                                  GtkIconSize size, const gchar *detail)
{
    if (!stock_id) {
        return NULL;
    }

#ifdef CTK_GTK3
    /* added in 3.0 and deprecated in 3.10 */
    return gtk_widget_render_icon_pixbuf(widget, stock_id, size);
#else
    /* deprecated in 3.0 */
    return gtk_widget_render_icon(widget, stock_id, size, detail);
#endif
}

void ctk_cell_renderer_set_alignment(GtkCellRenderer *renderer,
                                     gfloat x, gfloat y)
{
#ifdef CTK_GTK3
    /* added in 2.18 */
    gtk_cell_renderer_set_alignment(renderer, x, y);
#else
    /* direct access removed in 3.0 */
    renderer->xalign = x;
    renderer->yalign = y;
#endif
}

void ctk_widget_set_halign_left(GtkWidget *w)
{
#ifdef CTK_GTK3
    gtk_widget_set_halign(w, GTK_ALIGN_START);
#else
    g_object_set(w, "xalign", 0.0, NULL);
#endif
}

GtkWidget *ctk_file_chooser_dialog_new(const gchar *title,
                                       GtkWindow *parent,
                                       GtkFileChooserAction action)
{
#ifdef CTK_GTK3
    /* Added in 2.4 */
    gboolean open = (action == GTK_FILE_CHOOSER_ACTION_OPEN);

    return gtk_file_chooser_dialog_new(title, parent, action,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       open ? GTK_STOCK_OPEN : GTK_STOCK_SAVE,
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);
#else
    /*
     * There is an issue on some platforms that causes the UI to hang when
     * creating a GtkFileChooserDialog from a shared library. For now, we will
     * continue to use the GtkFileSelection.
     *
     * Will return GTK_RESPONSE_OK if not cancelled
     *
     * Deprecated in 2.4 and removed in 3.0
     */
    return gtk_file_selection_new(title);
#endif
}

void ctk_file_chooser_set_filename(GtkWidget *widget, const gchar *filename)
{
#ifdef CTK_GTK3
    GtkFileChooserAction action =
        gtk_file_chooser_get_action(GTK_FILE_CHOOSER(widget));
    char *full_filename = tilde_expansion(filename);
    char *basename = g_path_get_basename(full_filename);
    char *dirname = NULL;

    if (action == GTK_FILE_CHOOSER_ACTION_SAVE &&
        (!g_file_test(full_filename, G_FILE_TEST_EXISTS) ||
         basename[0] == '.')) {

        char *dirname = g_path_get_dirname(full_filename);

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), dirname);
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(widget), basename);

    } else {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), full_filename);
    }

    free(dirname);
    free(basename);
    free(full_filename);
#else
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(widget), filename);
#endif
}

gchar *ctk_file_chooser_get_filename(GtkWidget *widget)
{
#ifdef CTK_GTK3
    return gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
#else
    return g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget)));
#endif
}

void ctk_file_chooser_set_extra_widget(GtkWidget *widget, GtkWidget *extra)
{
#ifdef CTK_GTK3
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(widget), extra);
#else
    gtk_box_pack_start(GTK_BOX(GTK_FILE_SELECTION(widget)->main_vbox),
                       extra, FALSE, FALSE, 15);
#endif
}

/* end of GTK2/3 util functions */


/*
 * This function checks that the GTK+ library in use is at least as new as the
 * given version number. Note this differs from gtk_check_version(), which
 * checks that the library in use is "compatible" with the given version (which
 * requires the library in use have both (1) a newer or equal version number and
 * (2) an equal major version number).
 */
gboolean ctk_check_min_gtk_version(guint required_major,
                                   guint required_minor,
                                   guint required_micro)
{
    return (NV_VERSION3(required_major, required_minor,
                        required_micro) <=
            NV_VERSION3(gtk_major_version, gtk_minor_version,
                        gtk_micro_version));
}


gchar *get_pcie_generation_string(CtrlTarget *ctrl_target)
{
    ReturnStatus ret;
    gchar *s = NULL;
    int tmp;

    ret = NvCtrlGetAttribute(ctrl_target, NV_CTRL_GPU_PCIE_GENERATION, &tmp);
    if (ret == NvCtrlSuccess) {
        s = g_strdup_printf("Gen%u", tmp);
    }
    return s;
}

gchar *get_pcie_link_width_string(CtrlTarget *ctrl_target, int attribute)
{
    ReturnStatus ret;
    gint tmp;
    gchar *s = NULL;

    ret = NvCtrlGetAttribute(ctrl_target, attribute, &tmp);
    if (ret != NvCtrlSuccess) {
        s = g_strdup_printf("Unknown");
    } else {
        s = g_strdup_printf("x%d", tmp);
    }

    return s;
}

gchar *get_pcie_link_speed_string(CtrlTarget *ctrl_target, int attribute)
{
    ReturnStatus ret;
    gint tmp;
    gchar *s = NULL;

    ret = NvCtrlGetAttribute(ctrl_target, attribute, &tmp);
    if (ret == NvCtrlSuccess) {
        s = g_strdup_printf("%.1f GT/s", tmp/1000.0);
    }

    return s;
}



gchar *get_nvidia_driver_version(CtrlTarget *ctrl_target)
{
    ReturnStatus ret;
    gchar *driver_version;

    if (ctrl_target == NULL  || ctrl_target->h == NULL) {
        return NULL;
    }

    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_NVIDIA_DRIVER_VERSION,
                                   &driver_version);
    if (ret != NvCtrlSuccess) {
        return NULL;
    }

    return driver_version;
}



/*
 * Used to check if current display enabled or disabled.
 */
void update_display_enabled_flag(CtrlTarget *ctrl_target,
                                 gboolean *display_enabled)
{
    ReturnStatus ret;
    int val;

    /* Is display enabled? */

    ret = NvCtrlGetAttribute(ctrl_target,
                             NV_CTRL_DISPLAY_ENABLED,
                             &val);

    *display_enabled =
        ((ret == NvCtrlSuccess) &&
         (val == NV_CTRL_DISPLAY_ENABLED_TRUE));

} /* update_display_enabled_flag() */



gchar* create_gpu_name_string(CtrlTarget *ctrl_target)
{
    gchar *gpu_name;
    gchar *gpu_product_name;
    ReturnStatus ret;

    ret = NvCtrlGetStringAttribute(ctrl_target,
                                   NV_CTRL_STRING_PRODUCT_NAME,
                                   &gpu_product_name);
    if (ret == NvCtrlSuccess && gpu_product_name) {
        gpu_name = g_strdup_printf("GPU %d - (%s)",
                                   NvCtrlGetTargetId(ctrl_target),
                                   gpu_product_name);
    } else {
        gpu_name = g_strdup_printf("GPU %d - (Unknown)",
                                   NvCtrlGetTargetId(ctrl_target));
    }
    g_free(gpu_product_name);

    return gpu_name;
}



gchar* create_display_name_list_string(CtrlTarget *ctrl_target,
                                       unsigned int attr)
{
    gchar *displays = NULL;
    ReturnStatus ret;
    int *pData;
    int len;
    int i;


    /* List of Display Device connected on GPU */

    ret = NvCtrlGetBinaryAttribute(ctrl_target, 0,
                                   attr,
                                   (unsigned char **)(&pData), &len);
    if (ret == NvCtrlNotSupported) {
        return NULL;
    }

    for (i = 0; ret == NvCtrlSuccess && i < pData[0]; i++) {
        CtrlTarget *ctrl_other;
        int display_id = pData[i+1];
        gchar *logName = NULL;
        gchar *typeIdName = NULL;
        gchar *tmp_str;

        ctrl_other = NvCtrlGetTarget(ctrl_target->system, DISPLAY_TARGET,
                                     display_id);
        if (ctrl_other) {
            ret = NvCtrlGetStringAttribute(ctrl_other,
                                           NV_CTRL_STRING_DISPLAY_DEVICE_NAME,
                                           &tmp_str);
            if (ret == NvCtrlSuccess) {
                logName = g_strdup(tmp_str);
                free(tmp_str);
            }
            ret = NvCtrlGetStringAttribute(ctrl_other,
                                           NV_CTRL_STRING_DISPLAY_NAME_RANDR,
                                           &tmp_str);
            if (ret == NvCtrlSuccess) {
                typeIdName = g_strdup(tmp_str);
                free(tmp_str);
            }
        }
        if (!logName) {
            logName = g_strdup("Unknown");
        }
        if (!typeIdName) {
            typeIdName = g_strdup_printf("DPY-%d", display_id);
        }

        tmp_str = g_strdup_printf("%s (%s)", logName, typeIdName);
        g_free(logName);
        g_free(typeIdName);

        if (displays) {
            logName = g_strdup_printf("%s,\n%s", tmp_str, displays);
            g_free(displays);
            g_free(tmp_str);
            displays = logName;
        } else {
            displays = tmp_str;
        }
    }

    if (!displays) {
        displays = g_strdup("None");
    }

    return displays;
}



GtkWidget *add_table_row_with_help_text(GtkWidget *table,
                                        CtkConfig *ctk_config,
                                        const char *help,
                                        const gint row,
                                        const gint col,
                                        // 0 = left, 1 = right
                                        const gfloat name_xalign,
                                        // 0 = top, 1 = bottom
                                        const gfloat name_yalign,
                                        const gchar *name,
                                        const gfloat value_xalign,
                                        const gfloat value_yalign,
                                        const gchar *value)
{
    GtkWidget *label;

    label = gtk_label_new(name);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), name_xalign, name_yalign);
    gtk_table_attach(GTK_TABLE(table), label, col, col+1, row, row + 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

    if (value == NULL)
        label = gtk_label_new("Unknown");
    else
        label = gtk_label_new(value);
    gtk_label_set_selectable(GTK_LABEL(label), TRUE);
    gtk_misc_set_alignment(GTK_MISC(label), value_xalign, value_yalign);
    gtk_table_attach(GTK_TABLE(table), label, col+1, col+2, row, row + 1,
                     GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
    if ((help != NULL) || (ctk_config != NULL)) {
        ctk_config_set_tooltip(ctk_config, label, help);
    }

    return label;
}


GtkWidget *add_table_row(GtkWidget *table,
                         const gint row,
                         const gfloat name_xalign, // 0 = left, 1 = right
                         const gfloat name_yalign, // 0 = top, 1 = bottom
                         const gchar *name,
                         const gfloat value_xalign,
                         const gfloat value_yalign,
                         const gchar *value)
{
    return add_table_row_with_help_text(table,
                                        NULL, /* ctk_config */
                                        NULL, /* help */
                                        row,
                                        0, /* col */
                                        name_xalign,
                                        name_yalign,
                                        name,
                                        value_xalign,
                                        value_yalign,
                                        value);
}



/** ctk_get_parent_window() ******************************************
 *
 * Returns the parent window of a widget, if one exists
 *
 **/
GtkWidget * ctk_get_parent_window(GtkWidget *child)
{
    GtkWidget *parent = gtk_widget_get_parent(child);
    

    while (parent && !GTK_IS_WINDOW(parent)) {
        GtkWidget *last = parent;

        parent = gtk_widget_get_parent(last);
        if (!parent || parent == last) {
            /* GTK Error, can't find parent window! */
            parent = NULL;
            break;
        }
    }

    return parent;
    
} /* ctk_get_parent_window() */




/** ctk_display_error_msg() ******************************************
 *
 * Displays an error message.
 *
 **/
void ctk_display_error_msg(GtkWidget *parent, gchar * msg)
{
    GtkWidget *dlg;
    
    if (msg) {
        nv_error_msg("%s", msg);

        if (parent) {
            dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_ERROR,
             GTK_BUTTONS_OK,
             "%s", msg);
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }

} /* ctk_display_error_msg() */



/** ctk_display_warning_msg() ****************************************
 *
 * Displays a warning message.
 *
 **/
void ctk_display_warning_msg(GtkWidget *parent, gchar * msg)
{
    GtkWidget *dlg;
    
    if (msg) {
        nv_warning_msg("%s", msg);

        if (parent) {
            dlg = gtk_message_dialog_new
            (GTK_WINDOW(parent),
             GTK_DIALOG_DESTROY_WITH_PARENT,
             GTK_MESSAGE_WARNING,
             GTK_BUTTONS_OK,
             "%s", msg);
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }

} /* ctk_display_warning_msg() */



/** ctk_empty_container() ********************************************
 *
 * Removes all (non internal) children widgets from a container
 *
 * XXX It might be useful later on to allow a callback to be called
 *     for each widget that gets removed etc.
 *
 **/

void ctk_empty_container(GtkWidget *container)
{
    GList *list;
    GList *node;

    if (!container) {
        return;
    }

    list = gtk_container_get_children(GTK_CONTAINER(container));
    node = list;
    while (node) {
        gtk_container_remove(GTK_CONTAINER(container),
                             (GtkWidget *)(node->data));
        node = node->next;
    }
    g_list_free(list);

} /* ctk_empty_container() */


#ifndef CTK_GTK3
/* Updates the widget to use the text colors ('text' and 'base') for the
 * foreground and background colors.
 */
static void widget_use_text_colors_for_state(GtkWidget *widget,
                                             GtkStateType state)
{
    GtkStyle *style = gtk_widget_get_style(widget);

    gtk_widget_modify_fg(widget, state, &(style->text[state]));
    gtk_widget_modify_bg(widget, state, &(style->base[state]));

}


/* Callback for the "style-set" event, used to ensure that widgets that should
 * use the text colors for drawing still do so after a Theme update.
 */
static void force_text_colors_handler(GtkWidget *widget,
                                      GtkStyle *previous_style,
                                      gpointer user_data)
{
    g_signal_handlers_block_by_func
        (G_OBJECT(widget),
         G_CALLBACK(force_text_colors_handler), user_data);

    /* Use text colors for foreground and background */
    widget_use_text_colors_for_state(widget, GTK_STATE_NORMAL);
    widget_use_text_colors_for_state(widget, GTK_STATE_ACTIVE);
    widget_use_text_colors_for_state(widget, GTK_STATE_PRELIGHT);
    widget_use_text_colors_for_state(widget, GTK_STATE_SELECTED);
    widget_use_text_colors_for_state(widget, GTK_STATE_INSENSITIVE);

    g_signal_handlers_unblock_by_func
        (G_OBJECT(widget),
         G_CALLBACK(force_text_colors_handler), user_data);
}
#endif

void ctk_force_text_colors_on_widget(GtkWidget *widget)
{
#ifdef CTK_GTK3
    /* The GTK CSS theme engine will handle updates */
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class(context, "entry");
#else
    /* Set the initial state */
    force_text_colors_handler(widget, NULL, NULL);

    /* Ensure state is updated when style changes */
    g_signal_connect(G_OBJECT(widget),
                     "style-set", G_CALLBACK(force_text_colors_handler),
                     (gpointer) NULL);
#endif
}


/*
 * Create a dialog to allow the user to specify the name of the file to load
 * or save.
 */
char *ctk_get_filename_from_dialog(const gchar* title,
                                   GtkWindow *parent,
                                   const gchar* initial_filename)
{
    GtkWidget *dialog;
    gchar *filename = NULL;
    gint result;

    dialog =
        ctk_file_chooser_dialog_new(title, parent,
                                    GTK_FILE_CHOOSER_ACTION_SAVE);

    ctk_file_chooser_set_filename(dialog, initial_filename);

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_ACCEPT || result == GTK_RESPONSE_OK) {
        filename = ctk_file_chooser_get_filename(dialog);
    }

    gtk_widget_destroy(dialog);

    return filename;
}

GdkPixbuf *ctk_pixbuf_from_data(const char *start, const char *end)
{
    GdkPixbuf *pixbuf = NULL;
    size_t size = end - start;

#ifdef CTK_GTK3
    GInputStream *stream =
        g_memory_input_stream_new_from_data(start, size, NULL);

    if (stream) {
        pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, NULL);

        g_object_unref(stream);
    }
#else
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();

    if (loader) {
        gdk_pixbuf_loader_write(loader, (const void*)start, size, NULL);
        gdk_pixbuf_loader_close(loader, NULL);

        pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        if (pixbuf) {
            g_object_ref(pixbuf);
        }

        g_object_unref(loader);
    }
#endif
    return pixbuf;
}
