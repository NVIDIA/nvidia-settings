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

// Tree model implementation for operating on rules in an AppProfileConfig

#ifndef __CTK_APC_RULE_MODEL_H__
#define __CTK_APC_RULE_MODEL_H__

#include <gtk/gtk.h>
#include <assert.h>
#include "app-profiles.h"

G_BEGIN_DECLS

#define CTK_TYPE_APC_RULE_MODEL (ctk_apc_rule_model_get_type())

#define CTK_APC_RULE_MODEL(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_APC_RULE_MODEL, CtkApcRuleModel))

#define CTK_APC_RULE_MODEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_APC_RULE_MODEL, CtkApcRuleModelClass))

#define CTK_IS_APC_RULE_MODEL(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_APC_RULE_MODEL))

#define CTK_IS_APC_RULE_MODEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_APC_RULE_MODEL))

#define CTK_APC_RULE_MODEL_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_APC_RULE_MODEL, CtkApcRuleModelClass))

enum {
    CTK_APC_RULE_MODEL_COL_ID = 0,
    CTK_APC_RULE_MODEL_COL_FEATURE,
    CTK_APC_RULE_MODEL_COL_MATCHES,
    CTK_APC_RULE_MODEL_COL_PROFILE_NAME,
    CTK_APC_RULE_MODEL_COL_FILENAME,
    CTK_APC_RULE_MODEL_N_COLUMNS
};

typedef struct _CtkApcRuleModel CtkApcRuleModel;
typedef struct _CtkApcRuleModelClass CtkApcRuleModelClass;

struct _CtkApcRuleModel
{
    GObject parent;
    gint stamp;

    AppProfileConfig *config;

    // A sortable array of rule IDs (int) cached from the config,
    // used for presentation and iteration.
    GArray *rules;
};

struct _CtkApcRuleModelClass
{
    GObjectClass parent_class;
};

GType ctk_apc_rule_model_class_get_type (void) G_GNUC_CONST;
CtkApcRuleModel *ctk_apc_rule_model_new (AppProfileConfig *config);

int ctk_apc_rule_model_create_rule(CtkApcRuleModel *rule_model,
                                   const char *filename,
                                   json_t *new_rule);
void ctk_apc_rule_model_update_rule(CtkApcRuleModel *rule_model,
                                    const char *filename,
                                    int id,
                                    json_t *rule);
void ctk_apc_rule_model_delete_rule(CtkApcRuleModel *rule_model, int id);
void ctk_apc_rule_model_set_abs_rule_priority(CtkApcRuleModel *rule_model,
                                              int id, size_t pri);
void ctk_apc_rule_model_change_rule_priority(CtkApcRuleModel *rule_model,
                                            int id, int delta);

void ctk_apc_rule_model_attach(CtkApcRuleModel *rule_model, AppProfileConfig *config);

G_END_DECLS

#endif
