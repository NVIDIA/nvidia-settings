/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2013 NVIDIA Corporation.
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

#ifndef __CTK_APP_PROFILE_H__
#define __CTK_APP_PROFILE_H__

#include "app-profiles.h"
#include "ctkevent.h"
#include "ctkconfig.h"
#include "ctkapcprofilemodel.h"
#include "ctkapcrulemodel.h"
#include "ctkdropdownmenu.h"

G_BEGIN_DECLS

#define CTK_TYPE_APP_PROFILE (ctk_app_profile_get_type())

#define CTK_APP_PROFILE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APP_PROFILE, CtkAppProfile))

#define CTK_APP_PROFILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APP_PROFILE, CtkAppProfileClass))

#define CTK_IS_APP_PROFILE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APP_PROFILE))

#define CTK_IS_APP_PROFILE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APP_PROFILE))

#define CTK_APP_PROFILE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APP_PROFILE, CtkAppProfileClass))


typedef struct _CtkAppProfile       CtkAppProfile;
typedef struct _CtkAppProfileClass  CtkAppProfileClass;

typedef struct _EditRuleDialog {
    GtkWidget *parent;
    GtkWidget *top_window;

    gboolean new_rule;
    gint rule_id;

    // Canonical dialog box values
    GString *source_file;
    gint feature;
    GString *matches;
    GString *profile_name;

    // Widgets
    GtkWidget *source_file_combo;
    GtkWidget *feature_menu;
    GtkEntry *matches_entry;
    GtkWidget *profile_name_combo;
    GtkListStore *profile_settings_store;

    GtkWidget *add_edit_rule_button;

    // Data for constructing the help text for this dialog
    GList *help_data;

    // Signals
    gulong rule_profile_name_changed_signal;
    gulong feature_changed_signal;
} EditRuleDialog;

typedef struct _EditProfileDialog {
    GtkWidget *parent;

    // For convenience the profile dialog box can be opened
    // from the main window *or* the rule dialog box. Track
    // which is the caller here.
    GtkWidget *caller;

    GtkWidget *top_window;

    gboolean new_profile;

    // Canonical dialog box values
    GString *name;
    GString *orig_name; // The original name, before any editing took place
    GString *source_file;
    json_t *settings;

    // Widgets
    GtkWidget *name_entry;
    GtkWidget *generate_name_button;

    GtkWidget *source_file_combo;

    GtkWidget *add_edit_profile_button;

    GtkWidget *registry_key_combo;

    // Used in the special case where a currently edited row
    // will be deleted, in which case we don't want to update
    // the model.
    gboolean setting_update_canceled;

    CtkStatusBar error_statusbar;

    // Data for constructing the help text for this dialog
    GList *top_help_data;
    GList *setting_column_help_data;
    GList *setting_toolbar_help_data;
    GList *bottom_help_data;

    GtkTreeView *settings_view;
    GtkListStore *settings_store;

} EditProfileDialog;

typedef struct _SaveAppProfileChangesDialog {
    GtkWidget *parent;
    GtkWidget *top_window;

    gboolean show_preview;

    // Canonical dialog box values
    json_t *updates;

    // Widgets
    GtkWidget *preview_button;
    GtkWidget *preview_backup_entry;
    GtkWidget *preview_text_view;
    GtkWidget *preview_file_menu;
    GtkWidget *preview_vbox;
    GtkWidget *backup_check_button;

    // Data for constructing the help text for this dialog
    GList *help_data;

    // Signals
    gulong preview_changed_signal;
} SaveAppProfileChangesDialog;

struct _CtkAppProfile
{
    GtkVBox parent;
    CtkConfig *ctk_config;

    AppProfileConfig *gold_config, *cur_config;
    json_t *key_docs;

    // Interfaces layered on top of the config object for use with GtkTreeView
    CtkApcProfileModel *apc_profile_model;
    CtkApcRuleModel    *apc_rule_model;

    // Widgets
    GtkTreeView *main_profile_view;
    GtkTreeView *main_rule_view;
    GtkWidget *notebook;
    GtkWidget *enable_check_button;

    // Dialog boxes
    EditRuleDialog *edit_rule_dialog;
    EditProfileDialog *edit_profile_dialog;
    SaveAppProfileChangesDialog *save_app_profile_changes_dialog;

    // Data for constructing the help text for this page
    GList *global_settings_help_data;

    GList *rules_help_data;
    GList *rules_columns_help_data;

    GList *profiles_help_data;
    GList *profiles_columns_help_data;

    GList *save_reload_help_data;

    // TODO: provide undo functionality
};

struct _CtkAppProfileClass
{
    GtkVBoxClass parent_class;
};

GType          ctk_app_profile_get_type    (void) G_GNUC_CONST;
GtkWidget*     ctk_app_profile_new         (CtrlTarget *, CtkConfig *);
GtkTextBuffer* ctk_app_profile_create_help (CtkAppProfile *, GtkTextTagTable *);

char *serialize_settings(const json_t *settings, gboolean add_markup);

G_END_DECLS

#endif /* __CTK_APP_PROFILE_H__ */
