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

#ifndef __CTK_HELP_H__
#define __CTK_HELP_H__

#include <gtk/gtk.h>

#include "common-utils.h"


G_BEGIN_DECLS

#define CTK_TYPE_HELP (ctk_help_get_type())

#define CTK_HELP(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_HELP, CtkHelp))

#define CTK_HELP_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_HELP, CtkHelpClass))

#define CTK_IS_HELP(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_HELP))

#define CTK_IS_HELP_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_HELP))

#define CTK_HELP_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_HELP, CtkHelpClass))


typedef struct _CtkHelp       CtkHelp;
typedef struct _CtkHelpClass  CtkHelpClass;
typedef struct _CtkHelpDataItem CtkHelpDataItem;

struct _CtkHelp
{
    GtkWindow               parent;
    
    GtkWidget              *text_viewer;
    GtkTextBuffer          *default_help;
    GtkTextTagTable        *tag_table;

    GtkWidget              *toggle_button;
};

struct _CtkHelpDataItem
{
    // Header for the help section (usually corresponds to a label)
    gchar *label;

    // A brief summary of the contents
    gchar *help_text;

    // If non-NULL, elaborates on help_text above
    gchar *extended_help_text;
};

struct _CtkHelpClass
{
    GtkWindowClass parent_class;
};

GType             ctk_help_get_type          (void) G_GNUC_CONST;
GtkWidget        *ctk_help_new               (GtkWidget *, GtkTextTagTable *);
void              ctk_help_set_page          (CtkHelp *, GtkTextBuffer *);
GtkTextTagTable  *ctk_help_create_tag_table  (void);

void ctk_help_title   (GtkTextBuffer *, GtkTextIter *, const gchar *, ...) NV_ATTRIBUTE_PRINTF(3, 4);
void ctk_help_para    (GtkTextBuffer *, GtkTextIter *, const gchar *, ...) NV_ATTRIBUTE_PRINTF(3, 4);
void ctk_help_heading (GtkTextBuffer *, GtkTextIter *, const gchar *, ...) NV_ATTRIBUTE_PRINTF(3, 4);
void ctk_help_term    (GtkTextBuffer *, GtkTextIter *, const gchar *, ...) NV_ATTRIBUTE_PRINTF(3, 4);
void ctk_help_finish  (GtkTextBuffer *);

void ctk_help_reset_hardware_defaults(GtkTextBuffer *, GtkTextIter *, gchar *);
gchar *ctk_help_create_reset_hardware_defaults_text(gchar*, gchar *);

void ctk_help_data_list_prepend(GList **list,
                                const gchar *label,
                                const gchar *help_text,
                                const gchar *extended_help_text);
void ctk_help_data_list_free_full(GList *list);
void ctk_help_data_list_print_terms(GtkTextBuffer *b, GtkTextIter *i,
                                    GList *help_data_list);
void ctk_help_data_list_print_sections(GtkTextBuffer *b, GtkTextIter *i,
                                       GList *help_data_list);


#define CTK_HELP_TITLE_TAG                "title"
#define CTK_HELP_HEADING_TAG              "heading"
#define CTK_HELP_HEADING_NOT_EDITABLE_TAG "not_editable"
#define CTK_HELP_WORD_WRAP_TAG            "word_wrap"
#define CTK_HELP_MARGIN_TAG               "margin"
#define CTK_HELP_SINGLE_SPACE_TAG         "single-space"
#define CTK_HELP_BOLD_TAG                 "bold"


G_END_DECLS

#endif /* __CTK_HELP_H__ */
