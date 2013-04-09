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

// Tree model implementation for operating on profiles in an AppProfileConfig

#ifndef __CTK_APC_PROFILE_MODEL_H__
#define __CTK_APC_PROFILE_MODEL_H__

#include <gtk/gtk.h>
#include <assert.h>
#include "app-profiles.h"

G_BEGIN_DECLS

#define CTK_TYPE_APC_PROFILE_MODEL (ctk_apc_profile_model_get_type())

#define CTK_APC_PROFILE_MODEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APC_PROFILE_MODEL, CtkApcProfileModel))

#define CTK_APC_PROFILE_MODEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APC_PROFILE_MODEL, CtkApcProfileModelClass))

#define CTK_IS_APC_PROFILE_MODEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APC_PROFILE_MODEL))

#define CTK_IS_APC_PROFILE_MODEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APC_PROFILE_MODEL))

#define CTK_APC_PROFILE_MODEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APC_PROFILE_MODEL, CtkApcProfileModelClass))

enum {
    CTK_APC_PROFILE_MODEL_COL_NAME = 0,
    CTK_APC_PROFILE_MODEL_COL_FILENAME,
    CTK_APC_PROFILE_MODEL_COL_SETTINGS,
    CTK_APC_PROFILE_MODEL_N_COLUMNS,
    CTK_APC_PROFILE_MODEL_DEFAULT_SORT_COL = CTK_APC_PROFILE_MODEL_COL_NAME
};

typedef struct _CtkApcProfileModel CtkApcProfileModel;
typedef struct _CtkApcProfileModelClass CtkApcProfileModelClass;

#define CTK_APC_PROFILE_MODEL_MAX_ITERS 16

struct _CtkApcProfileModel
{
    GObject parent;
    gint stamp;

    AppProfileConfig *config;

    // A sortable array of profile names cached from the config, used for
    // presentation and iteration.
    GArray *profiles;
    gint sort_column_id;
    GtkSortType order;
    
    GtkTreeIterCompareFunc sort_funcs[CTK_APC_PROFILE_MODEL_N_COLUMNS];
    gpointer sort_user_data[CTK_APC_PROFILE_MODEL_N_COLUMNS];
    GDestroyNotify sort_destroy_notify[CTK_APC_PROFILE_MODEL_N_COLUMNS];
};

struct _CtkApcProfileModelClass
{
    GObjectClass parent_class;
};

GType ctk_apc_profile_model_class_get_type (void) G_GNUC_CONST;
GType ctk_apc_profile_model_get_type(void) G_GNUC_CONST;
CtkApcProfileModel *ctk_apc_profile_model_new (AppProfileConfig *config);

void ctk_apc_profile_model_update_profile(CtkApcProfileModel *prof_model,
                                          const char *filename,
                                          const char *profile_name,
                                          json_t *profile);

void ctk_apc_profile_model_delete_profile(CtkApcProfileModel *prof_model,
                                          const char *profile_name);

void ctk_apc_profile_model_attach(CtkApcProfileModel *prof_model, AppProfileConfig *config);

// Thin wrapper around nv_app_profile_config_get_profile() to promote
// modularity (all requests for config data should go through the models).
static inline const json_t *ctk_apc_profile_model_get_profile(CtkApcProfileModel *prof_model,
                                                              const char *profile_name)
{
    return nv_app_profile_config_get_profile(prof_model->config, profile_name);
}

G_END_DECLS

#endif
