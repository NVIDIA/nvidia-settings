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

#ifndef __CTK_WINDOW_H__
#define __CTK_WINDOW_H__

#include <gtk/gtk.h>

#include "NvCtrlAttributes.h"

#include "parse.h"
#include "config-file.h"
#include "ctkconfig.h"

G_BEGIN_DECLS

#define CTK_TYPE_WINDOW (ctk_window_get_type())

#define CTK_WINDOW(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_WINDOW, CtkWindow))

#define CTK_WINDOW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_WINDOW, CtkWindowClass))

#define CTK_IS_WINDOW(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_WINDOW))

#define CTK_IS_WINDOW_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_WINDOW))

#define CTK_WINDOW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_WINDOW, CtkWindowClass))


#define CTK_DISPLAY_DEVICE_CRT_MASK 0x000000FF
#define CTK_DISPLAY_DEVICE_TV_MASK  0x0000FF00
#define CTK_DISPLAY_DEVICE_DFP_MASK 0x00FF0000


typedef struct _CtkWindow       CtkWindow;
typedef struct _CtkWindowClass  CtkWindowClass;

struct _CtkWindow
{
    GtkWindow               parent;

    GtkTreeStore           *tree_store;
    GtkTreeView            *treeview;

    GtkWidget              *page_viewer;
    GtkWidget              *page;

    CtkConfig              *ctk_config;
    GtkWidget              *ctk_help;
    
    GtkWidget              *quit_dialog;

    ParsedAttribute        *attribute_list;

    GtkTreeIter             iter;
    GtkWidget              *widget;

    GtkTextTagTable        *help_tag_table;
    GtkTextBuffer          *help_text_buffer;
};

struct _CtkWindowClass
{
    GtkWindowClass parent_class;
};

GType       ctk_window_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_window_new       (NvCtrlAttributeHandle**, gint,
                                  NvCtrlAttributeHandle**, gint,
                                  NvCtrlAttributeHandle**, gint,
                                  ParsedAttribute *, ConfigProperties *conf,
                                  CtrlHandles *pCtrlHandles);

void add_special_config_file_attributes(CtkWindow *ctk_window);

G_END_DECLS

#endif /* __CTK_WINDOW_H__ */
