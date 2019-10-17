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

/*
 * The CtkConfig widget controls configuration options of the control
 * panel itself (rather than configuration options of the NVIDIA
 * X/GLX driver).
 */

#ifndef __CTK_CONFIG_H__
#define __CTK_CONFIG_H__

#include <gtk/gtk.h>

#if GTK_MAJOR_VERSION >= 3
#define CTK_GTK3
#endif

#include "config-file.h"

#define CTK_CONFIG_PENDING_APPLY_DISPLAY_CONFIG    (1 << 0)
#define CTK_CONFIG_PENDING_WRITE_DISPLAY_CONFIG    (1 << 1)
#define CTK_CONFIG_PENDING_WRITE_MOSAIC_CONFIG     (1 << 2)
#define CTK_CONFIG_PENDING_WRITE_APP_PROFILES      (1 << 3)
#define CTK_CONFIG_PENDING_LAST_VALUE              (1 << 4)

G_BEGIN_DECLS

#define CTK_TYPE_CONFIG (ctk_config_get_type())

#define CTK_CONFIG(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_CONFIG, CtkConfig))

#define CTK_CONFIG_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_CONFIG, CtkConfigClass))

#define CTK_IS_CONFIG(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_CONFIG))

#define CTK_IS_CONFIG_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_CONFIG))

#define CTK_CONFIG_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_CONFIG, CtkConfigClass))


typedef struct _CtkConfig       CtkConfig;
typedef struct _CtkConfigClass  CtkConfigClass;

typedef struct _CtkStatusBar    CtkStatusBar;
typedef struct _CtkToolTips     CtkToolTips;

struct _CtkStatusBar
{
    GtkWidget *widget;
    guint prev_message_id;

    // determines if ctk_config_statusbar_message() will update the statusbar
    gboolean enabled;
};

#ifndef CTK_GTK3
struct _CtkToolTips
{
    GtkTooltips *object;
};
#endif


struct _CtkConfig
{
    GtkVBox parent;
    CtkStatusBar status_bar;
#ifndef CTK_GTK3
    CtkToolTips tooltips;
#endif
    GtkListStore *list_store;
    ConfigProperties *conf;
    GtkWidget *timer_list;
    GtkWidget *timer_list_box;
    GtkWidget *button_save_rc;
    gchar *rc_filename;
    gboolean timer_list_visible;
    CtrlSystem *pCtrlSystem;
    GList *help_data;
    guint pending_config;
};

struct _CtkConfigClass
{
    GtkVBoxClass parent_class;
};

GType      ctk_config_get_type            (void) G_GNUC_CONST;
GtkWidget* ctk_config_new                 (ConfigProperties *, CtrlSystem *);
void       ctk_config_statusbar_message   (CtkConfig *, const char *, ...) NV_ATTRIBUTE_PRINTF(2, 3);
GtkWidget* ctk_config_get_statusbar       (CtkConfig *);
void       ctk_config_set_tooltip         (CtkConfig *, GtkWidget *,
                                           const gchar *);
GtkTextBuffer *ctk_config_create_help     (CtkConfig *, GtkTextTagTable *);

void ctk_config_add_timer(CtkConfig *, guint, gchar *, GSourceFunc, gpointer);
void ctk_config_remove_timer(CtkConfig *, GSourceFunc);

void ctk_config_start_timer(CtkConfig *, GSourceFunc, gpointer);
void ctk_config_stop_timer(CtkConfig *, GSourceFunc, gpointer);

gboolean ctk_config_slider_text_entry_shown(CtkConfig *);

void ctk_config_set_tooltip_and_add_help_data(CtkConfig *config,
                                              GtkWidget *widget,
                                              GList **help_data_list,
                                              const gchar *label,
                                              const gchar *help_text,
                                              const gchar *extended_help_text);

// Helper functions for other components which use CtkStatusBar
void ctk_statusbar_init(CtkStatusBar *status_bar);
void ctk_statusbar_message(CtkStatusBar *status_bar, const gchar *str);
void ctk_statusbar_clear(CtkStatusBar *status_bar);

G_END_DECLS

#endif /* __CTK_CONFIG_H__ */
