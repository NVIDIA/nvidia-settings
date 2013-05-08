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

#ifndef __CTK_DISPLAYCONFIG_H__
#define __CTK_DISPLAYCONFIG_H__

#include "ctkevent.h"
#include "ctkconfig.h"
#include "ctkdisplaylayout.h"
#include "ctkdisplayconfig-utils.h"



G_BEGIN_DECLS

#define CTK_TYPE_DISPLAY_CONFIG (ctk_display_config_get_type())

#define CTK_DISPLAY_CONFIG(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_DISPLAY_CONFIG, \
                                 CtkDisplayConfig))

#define CTK_DISPLAY_CONFIG_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_DISPLAY_CONFIG, \
                              CtkDisplayConfigClass))

#define CTK_IS_DISPLAY_CONFIG(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_DISPLAY_CONFIG))

#define CTK_IS_DISPLAY_CONFIG_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_DISPLAY_CONFIG))

#define CTK_DISPLAY_CONFIG_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_DISPLAY_CONFIG, \
                                CtkDisplayConfigClass))

typedef enum {
    SELECTABLE_ITEM_SCREEN,
    SELECTABLE_ITEM_DISPLAY
} SelectableItemType;


typedef struct SelectableItemRec {
    SelectableItemType type;
    union {
        nvDisplayPtr display;
        nvScreenPtr screen;
    } u;
} SelectableItem;


typedef struct _CtkDisplayConfig
{
    GtkVBox parent;

    NvCtrlAttributeHandle *handle;
    CtkConfig *ctk_config;

    gboolean page_selected; /* Is the display config page selected in the UI */

    /* Layout */
    nvLayoutPtr layout;
    GtkWidget *obj_layout;
    GtkWidget *label_layout;

    GtkWidget *chk_xinerama_enabled;
    GtkWidget *chk_primary_display;
    gboolean primary_display_changed;

    GtkWidget *mnu_selected_item;
    SelectableItem *selected_item_table;
    int selected_item_table_len;

    GtkWidget *notebook; /* Tabbed notebook for display and X screen pages */

    /* Display - Info */
    GtkWidget *display_page;

    GtkWidget *txt_display_gpu;

    GtkWidget *box_display_config;
    GtkWidget *mnu_display_config;
    GtkWidget *mnu_display_config_disabled;
    GtkWidget *mnu_display_config_xscreen;
    GtkWidget *mnu_display_config_twinview;

    /* Display - Settings */
    GtkWidget *box_screen_drag_info_display;

    GtkWidget *box_display_resolution;
    GtkWidget *mnu_display_resolution;
    nvSelectedModePtr *resolution_table;
    int resolution_table_len;

    GtkWidget *mnu_display_refresh;
    nvModeLinePtr *refresh_table; /* Lookup table for refresh menu */
    int refresh_table_len;

    GtkWidget *box_display_modename;
    GtkWidget *txt_display_modename;

    GtkWidget *box_display_stereo;
    GtkWidget *mnu_display_stereo;

    GtkWidget *box_display_orientation;
    GtkWidget *mnu_display_rotation;
    GtkWidget *mnu_display_reflection;

    GtkWidget *box_display_viewport;

    GtkWidget *box_display_viewport_in;
    GtkWidget *txt_display_viewport_in;

    GtkWidget *box_display_viewport_out;
    GtkWidget *txt_display_viewport_out;


    GtkWidget *box_display_position;
    GtkWidget *mnu_display_position_type;     /* Absolute, Right of... */
    GtkWidget *mnu_display_position_relative; /* List of available devices */
    nvDisplayPtr *display_position_table; /* Lookup table for relative display position */
    int display_position_table_len;
    GtkWidget *txt_display_position_offset;   /* Absolute: +0+0 */

    GtkWidget *box_display_panning;
    GtkWidget *txt_display_panning;


    /* X Screen - Info */
    GtkWidget *screen_page;

    /* X Screen - Settings */
    GtkWidget *box_screen_drag_info_screen;

    GtkWidget *box_screen_virtual_size;
    GtkWidget *txt_screen_virtual_size;

    GtkWidget *box_screen_depth;
    GtkWidget *mnu_screen_depth;

    GtkWidget *box_screen_stereo;
    GtkWidget *mnu_screen_stereo;

    GtkWidget *box_screen_position;
    GtkWidget *mnu_screen_position_type;     /* Absolute, Right of... */
    GtkWidget *mnu_screen_position_relative; /* List of available devices */
    nvScreenPtr *screen_position_table;
    int screen_position_table_len;
    GtkWidget *txt_screen_position_offset;   /* Absolute: +0+0 */

    GtkWidget *box_screen_metamode;
    GtkWidget *btn_screen_metamode;
    GtkWidget *btn_screen_metamode_add;
    GtkWidget *btn_screen_metamode_delete;

    int *screen_depth_table;
    int screen_depth_table_len;

    /* Dialogs */
    GtkWidget *dlg_display_disable;
    GtkWidget *txt_display_disable;
    GtkWidget *btn_display_disable_off;
    GtkWidget *btn_display_disable_cancel;

    GtkWidget *dlg_validation_override;
    GtkTextBuffer *buf_validation_override;
    GtkWidget *btn_validation_override_cancel;
    GtkWidget *box_validation_override_details;
    GtkWidget *btn_validation_override_show; /* Show details */

    GtkWidget *dlg_validation_apply;

    GtkWidget *dlg_reset_confirm;
    GtkWidget *btn_reset_cancel;

    GtkWidget *dlg_display_confirm;
    GtkWidget *txt_display_confirm;
    GtkWidget *btn_display_apply_cancel;
    guint display_confirm_timer;
    int display_confirm_countdown; /* Timeout to reset display config */

    SaveXConfDlg *save_xconfig_dlg;

    /* Buttons */
    GtkWidget *btn_apply;
    gboolean apply_possible; /* True if all modifications are applicable */

    gboolean reset_required; /* Reser required to apply */
    gboolean forced_reset_allowed; /* OK to reset layout w/o user input */
    gboolean notify_user_of_reset; /* User was notified of reset requirement */
    gboolean ignore_reset_events; /* Ignore reset-causing events */

    GdkPoint cur_screen_pos; /* Keep track of the selected X screen's position */

    GtkWidget *btn_save;
    GtkWidget *btn_probe;

    GtkWidget *btn_advanced;
    gboolean   advanced_mode;

    GtkWidget *btn_reset;

    int last_resolution_idx;

} CtkDisplayConfig;

typedef struct _CtkDisplayConfigClass
{
    GtkVBoxClass parent_class;
} CtkDisplayConfigClass;


GType       ctk_display_config_get_type  (void) G_GNUC_CONST;
GtkWidget*  ctk_display_config_new       (NvCtrlAttributeHandle *,
                                          CtkConfig *);

GtkTextBuffer *ctk_display_config_create_help(GtkTextTagTable *,
                                              CtkDisplayConfig *);

void ctk_display_config_selected(GtkWidget *);
void ctk_display_config_unselected(GtkWidget *);


G_END_DECLS

#endif /* __CTK_DISPLAYCONFIG_H__ */
