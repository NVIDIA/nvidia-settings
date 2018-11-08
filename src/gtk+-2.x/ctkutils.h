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

#ifndef __CTK_UTILS_H__
#define __CTK_UTILS_H__

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkconfig.h"

G_BEGIN_DECLS

// GValues must be initialized before they are used. This macro is
// only available since glib 2.30
#ifndef G_VALUE_INIT
# define G_VALUE_INIT { 0, { { 0 } } }
#endif

#ifdef CTK_GTK3
#define CTK_STOCK_ADD             "list-add"
#define CTK_STOCK_APPLY           "_Apply"  /* No icon recommended */
#define CTK_STOCK_CANCEL          "_Cancel" /* No icon recommended */
#define CTK_STOCK_CLOSE           "window-close"
#define CTK_STOCK_DIALOG_ERROR    "dialog-error"
#define CTK_STOCK_DIALOG_INFO     "dialog-information"
#define CTK_STOCK_DIALOG_QUESTION "dialog-question"
#define CTK_STOCK_DIALOG_WARNING  "dialog-warning"
#define CTK_STOCK_EDIT             NULL /* Nothing recommended */
#define CTK_STOCK_EXECUTE         "system-run"
#define CTK_STOCK_GO_DOWN         "go-down"
#define CTK_STOCK_GO_UP           "go-up"
#define CTK_STOCK_HELP            "help-browser"
#define CTK_STOCK_NO               NULL /* Nothing recommended */
#define CTK_STOCK_OK              "_OK" /* No icon recommended */
#define CTK_STOCK_OPEN            "document-open"
#define CTK_STOCK_PREFERENCES     "preferences-system"
#define CTK_STOCK_QUIT            "application-exit"
#define CTK_STOCK_REFRESH         "view-refresh"
#define CTK_STOCK_REMOVE          "list-remove"
#define CTK_STOCK_SAVE            "document-save"
#define CTK_STOCK_STOP            "process-stop"
#define CTK_STOCK_YES              NULL /* Nothing recommended */
#else
#define CTK_STOCK_ADD             GTK_STOCK_ADD
#define CTK_STOCK_APPLY           GTK_STOCK_APPLY
#define CTK_STOCK_CANCEL          GTK_STOCK_CANCEL
#define CTK_STOCK_CLOSE           GTK_STOCK_CLOSE
#define CTK_STOCK_DIALOG_ERROR    GTK_STOCK_DIALOG_ERROR
#define CTK_STOCK_DIALOG_INFO     GTK_STOCK_DIALOG_INFO
#define CTK_STOCK_DIALOG_QUESTION GTK_STOCK_DIALOG_QUESTION
#define CTK_STOCK_DIALOG_WARNING  GTK_STOCK_DIALOG_WARNING
#define CTK_STOCK_EDIT            GTK_STOCK_EDIT
#define CTK_STOCK_EXECUTE         GTK_STOCK_EXECUTE
#define CTK_STOCK_GO_DOWN         GTK_STOCK_GO_DOWN
#define CTK_STOCK_GO_UP           GTK_STOCK_GO_UP
#define CTK_STOCK_HELP            GTK_STOCK_HELP
#define CTK_STOCK_NO              GTK_STOCK_NO
#define CTK_STOCK_OK              GTK_STOCK_OK
#define CTK_STOCK_OPEN            GTK_STOCK_OPEN
#define CTK_STOCK_PREFERENCES     GTK_STOCK_PREFERENCES
#define CTK_STOCK_QUIT            GTK_STOCK_QUIT
#define CTK_STOCK_REFRESH         GTK_STOCK_REFRESH
#define CTK_STOCK_REMOVE          GTK_STOCK_REMOVE
#define CTK_STOCK_SAVE            GTK_STOCK_SAVE
#define CTK_STOCK_STOP            GTK_STOCK_STOP
#define CTK_STOCK_YES             GTK_STOCK_YES
#endif

/*
 * GTK 2/3 util functions
 */

gboolean ctk_widget_is_sensitive(GtkWidget *w);
gboolean ctk_widget_get_sensitive(GtkWidget *w);
gboolean ctk_widget_get_visible(GtkWidget *w);
gboolean ctk_widget_is_drawable(GtkWidget *w);
GdkWindow *ctk_widget_get_window(GtkWidget *w);
void ctk_widget_get_allocation(GtkWidget *w, GtkAllocation *a);
gchar *ctk_widget_get_tooltip_text(GtkWidget *w);

void ctk_widget_get_preferred_size(GtkWidget *w, GtkRequisition *r);
GtkWidget *ctk_dialog_get_content_area(GtkDialog *d);

gdouble ctk_adjustment_get_page_increment(GtkAdjustment *a);
gdouble ctk_adjustment_get_step_increment(GtkAdjustment *a);
gdouble ctk_adjustment_get_page_size(GtkAdjustment *a);
gdouble ctk_adjustment_get_upper(GtkAdjustment *a);
void ctk_adjustment_set_upper(GtkAdjustment *a, gdouble x);
void ctk_adjustment_set_lower(GtkAdjustment *a, gdouble x);

GtkWidget *ctk_scrolled_window_get_vscrollbar(GtkScrolledWindow *sw);
void ctk_scrolled_window_add(GtkScrolledWindow *sw, GtkWidget *child);
GtkWidget *ctk_statusbar_get_message_area(GtkStatusbar *statusbar);
void ctk_cell_renderer_set_alignment(GtkCellRenderer *widget,
                                     gfloat x, gfloat y);

GtkWidget *ctk_file_chooser_dialog_new(const gchar *title, GtkWindow *parent,
                                       GtkFileChooserAction action);
void ctk_file_chooser_set_filename(GtkWidget *widget, const gchar *filename);
gchar *ctk_file_chooser_get_filename(GtkWidget *widget);
void ctk_file_chooser_set_extra_widget(GtkWidget *widget, GtkWidget *extra);

GtkWidget *ctk_combo_box_text_new(void);
GtkWidget *ctk_combo_box_text_new_with_entry(void);
void ctk_combo_box_text_append_text(GtkWidget *widget, const gchar *text);

void ctk_g_object_ref_sink(gpointer obj);

GtkWidget *ctk_image_new_from_str(const gchar *str, GtkIconSize size);
GdkPixbuf *ctk_widget_render_icon(GtkWidget *widget, const gchar *stock_id,
                                  GtkIconSize size, const gchar *detail);

/* end of GTK 2/3 util functions */

gchar *get_pcie_generation_string(CtrlTarget *ctrl_target);

gchar *get_pcie_link_width_string(CtrlTarget *ctrl_target, gint attribute);

gchar *get_pcie_link_speed_string(CtrlTarget *ctrl_target, gint attribute);

gchar *get_nvidia_driver_version(CtrlTarget *ctrl_target);

gchar* create_gpu_name_string(CtrlTarget *ctrl_target);

gchar* create_display_name_list_string(CtrlTarget *ctrl_target,
                                       unsigned int attr);

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
                                        const gchar *value);

GtkWidget *add_table_row(GtkWidget *, const gint,
                         const gfloat, const gfloat, const gchar *,
                         const gfloat, const gfloat, const gchar *);

GtkWidget * ctk_get_parent_window(GtkWidget *child);

void ctk_display_error_msg(GtkWidget *parent, gchar *msg);

void ctk_display_warning_msg(GtkWidget *parent, gchar *msg);

void ctk_empty_container(GtkWidget *);

void update_display_enabled_flag(CtrlTarget *ctrl_target,
                                 gboolean *display_enabled);

gboolean ctk_check_min_gtk_version(guint required_major,
                                   guint required_minor,
                                   guint required_micro);

void ctk_force_text_colors_on_widget(GtkWidget *widget);

gchar *ctk_get_filename_from_dialog(const gchar* title,
                                    GtkWindow *parent,
                                    const gchar* initial_filename);
GdkPixbuf *ctk_pixbuf_from_data(const char *start, const char *end);
#define CTK_LOAD_PIXBUF(art) ctk_pixbuf_from_data(_binary_##art##_png_start, _binary_##art##_png_end)

G_END_DECLS

#endif /* __CTK_UTILS_H__ */
