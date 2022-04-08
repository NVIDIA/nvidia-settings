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

#include <gtk/gtk.h>
#include <stdint.h>
#include "ctkutils.h"
#include "ctkapcrulemodel.h"

static GObjectClass *parent_class = NULL;

// Forward declarations
GType ctk_apc_rule_model_get_type(void);
static void apc_rule_model_class_init(CtkApcRuleModelClass *klass, gpointer);
static void apc_rule_model_init(CtkApcRuleModel *rule_model, gpointer);
static void apc_rule_model_finalize(GObject *object);
static void apc_rule_model_tree_model_init(GtkTreeModelIface *iface, gpointer);
static void apc_rule_model_drag_source_init(GtkTreeDragSourceIface *iface, gpointer);
static void apc_rule_model_drag_dest_init(GtkTreeDragDestIface *iface, gpointer);
static GtkTreeModelFlags apc_rule_model_get_flags(GtkTreeModel *tree_model);
static gint apc_rule_model_get_n_columns(GtkTreeModel *tree_model);
static GType apc_rule_model_get_column_type(GtkTreeModel *tree_model, gint index);
static gboolean apc_rule_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *apc_rule_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter);
static void apc_rule_model_get_value(GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value);
static gboolean apc_rule_model_iter_next(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);
static gboolean apc_rule_model_iter_children(GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             GtkTreeIter *parent);
static gboolean apc_rule_model_iter_has_child(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter);
static gint apc_rule_model_iter_n_children(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter);
static gboolean apc_rule_model_iter_nth_child(GtkTreeModel *tree_model,
                                              GtkTreeIter  *iter,
                                              GtkTreeIter  *parent,
                                              gint         n);
static gboolean apc_rule_model_iter_parent(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter,
                                           GtkTreeIter *child);
CtkApcRuleModel *ctk_apc_rule_model_new(AppProfileConfig *config);
int ctk_apc_rule_model_create_rule(CtkApcRuleModel *rule_model,
                                   const char *filename,
                                   json_t *new_rule);
void ctk_apc_rule_model_update_rule(CtkApcRuleModel *rule_model,
                                    const char *filename,
                                    int id,
                                    json_t *rule);
void ctk_apc_rule_model_delete_rule(CtkApcRuleModel *rule_model, int id);
static void apc_rule_model_post_set_rule_priority_common(CtkApcRuleModel *rule_model,
                                                         int id);
void ctk_apc_rule_model_set_abs_rule_priority(CtkApcRuleModel *rule_model,
                                              int id, size_t pri);
void ctk_apc_rule_model_change_rule_priority(CtkApcRuleModel *rule_model,
                                            int id, int delta);
static gboolean apc_rule_model_row_draggable(GtkTreeDragSource *drag_source, GtkTreePath *path);
static gboolean apc_rule_model_drag_data_get(GtkTreeDragSource *drag_source, GtkTreePath *path, GtkSelectionData *selection_data);
static gboolean apc_rule_model_drag_data_delete(GtkTreeDragSource *drag_source, GtkTreePath *path);
static gboolean apc_rule_model_drag_data_received(GtkTreeDragDest *drag_dest,
                                                  GtkTreePath *dest,
                                                  GtkSelectionData *selection_data);
static gboolean apc_rule_model_row_drop_possible(GtkTreeDragDest *drag_dest,
                                                 GtkTreePath *dest_path,
                                                 GtkSelectionData *selection_data);


GType ctk_apc_rule_model_get_type(void)
{
    static GType apc_rule_model_type = 0;
    if (!apc_rule_model_type) {
        static const GTypeInfo apc_rule_model_info = {
            sizeof (CtkApcRuleModelClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) apc_rule_model_class_init, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkApcRuleModel),
            0,    /* n_preallocs */
            (GInstanceInitFunc) apc_rule_model_init, /* instance_init */
            NULL  /* value_table */
        };
        static const GInterfaceInfo tree_model_info =
        {
            (GInterfaceInitFunc) apc_rule_model_tree_model_init, /* interface_init */
            NULL, /* interface_finalize */
            NULL  /* interface_data */
        };
        static const GInterfaceInfo drag_source_info =
        {
            (GInterfaceInitFunc) apc_rule_model_drag_source_init, /* interface_init */
            NULL, /* interface_finalize */
            NULL  /* interface_data */
        };
        static const GInterfaceInfo drag_dest_info =
        {
            (GInterfaceInitFunc) apc_rule_model_drag_dest_init, /* interface_init */
            NULL, /* interface_finalize */
            NULL  /* interface_data */
        };

        apc_rule_model_type =
            g_type_register_static(G_TYPE_OBJECT, "CtkApcRuleModel",
                                   &apc_rule_model_info, 0);

        g_type_add_interface_static(apc_rule_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
        g_type_add_interface_static(apc_rule_model_type, GTK_TYPE_TREE_DRAG_SOURCE, &drag_source_info);
        g_type_add_interface_static(apc_rule_model_type, GTK_TYPE_TREE_DRAG_DEST, &drag_dest_info);
    }

    return apc_rule_model_type;
}

static void apc_rule_model_class_init(CtkApcRuleModelClass *klass,
                                      gpointer class_data)
{
    GObjectClass *object_class;

    parent_class = (GObjectClass *)g_type_class_peek_parent(klass);
    object_class = (GObjectClass *)klass;

    object_class->finalize = apc_rule_model_finalize;
}

static void apc_rule_model_init(CtkApcRuleModel *rule_model, gpointer g_class)
{
    rule_model->stamp = g_random_int(); // random int to catch iterator type mismatches
    rule_model->config = NULL;
    rule_model->rules = g_array_new(FALSE, FALSE, sizeof(gint));
}

static void apc_rule_model_finalize(GObject *object)
{
    CtkApcRuleModel *rule_model = CTK_APC_RULE_MODEL(object);
    g_array_free(rule_model->rules, TRUE);
    parent_class->finalize(object);
}

static void apc_rule_model_tree_model_init(GtkTreeModelIface *iface,
                                           gpointer iface_data)
{
    iface->get_flags       = apc_rule_model_get_flags;
    iface->get_n_columns   = apc_rule_model_get_n_columns;
    iface->get_column_type = apc_rule_model_get_column_type;
    iface->get_iter        = apc_rule_model_get_iter;
    iface->get_path        = apc_rule_model_get_path;
    iface->get_value       = apc_rule_model_get_value;
    iface->iter_next       = apc_rule_model_iter_next;
    iface->iter_children   = apc_rule_model_iter_children;
    iface->iter_has_child  = apc_rule_model_iter_has_child;
    iface->iter_n_children = apc_rule_model_iter_n_children;
    iface->iter_nth_child  = apc_rule_model_iter_nth_child;
    iface->iter_parent     = apc_rule_model_iter_parent;
}

static void apc_rule_model_drag_source_init(GtkTreeDragSourceIface *iface,
                                            gpointer iface_data)
{
    iface->row_draggable = apc_rule_model_row_draggable;
    iface->drag_data_get = apc_rule_model_drag_data_get;
    iface->drag_data_delete = apc_rule_model_drag_data_delete;
}

static void apc_rule_model_drag_dest_init(GtkTreeDragDestIface *iface,
                                          gpointer iface_data)
{
    iface->drag_data_received = apc_rule_model_drag_data_received;
    iface->row_drop_possible  = apc_rule_model_row_drop_possible;
}

static gboolean apc_rule_model_row_draggable(GtkTreeDragSource *drag_source, GtkTreePath *path)
{
    return TRUE;
}

static gboolean apc_rule_model_drag_data_get(GtkTreeDragSource *drag_source, GtkTreePath *path, GtkSelectionData *selection_data)
{
    if (gtk_tree_set_row_drag_data(selection_data,
                                   GTK_TREE_MODEL(drag_source),
                                   path)) {
        return TRUE;
    } else {
        // XXX text targets?
        return FALSE;
    }
}

static gboolean apc_rule_model_drag_data_delete(GtkTreeDragSource *drag_source, GtkTreePath *path)
{
    CtkApcRuleModel *rule_model;
    gint *indices, depth;

    rule_model = CTK_APC_RULE_MODEL(drag_source);
    indices = gtk_tree_path_get_indices(path);
    depth   = gtk_tree_path_get_depth(path);
    if (depth != 1) {
        return FALSE;
    }
    if ((indices[0] < 0) || (indices[0] > rule_model->rules->len)) {
        return FALSE;
    }

    // XXX the actual deletion is handled in the data_received() callback
    // If we ever use targets other than the view itself, this will need to have
    // a real implementation.
    return TRUE;
}

static gboolean apc_rule_model_drag_data_received(GtkTreeDragDest *drag_dest,
                                                  GtkTreePath *dest,
                                                  GtkSelectionData *selection_data)
{
    CtkApcRuleModel *rule_model;
    GtkTreeModel *tree_model = GTK_TREE_MODEL(drag_dest);
    GtkTreeModel *src_model = NULL;
    GtkTreeIter iter;
    GtkTreePath *src = NULL;
    gint *dest_indices, dest_depth;
    gint src_depth;
    gint dest_n;
    GValue id = G_VALUE_INIT;

    if (gtk_tree_get_row_drag_data(selection_data, &src_model, &src) &&
        (src_model == tree_model)) {
        // Move the given row
        rule_model = CTK_APC_RULE_MODEL(drag_dest);

        dest_indices = gtk_tree_path_get_indices(dest);
        dest_depth = gtk_tree_path_get_depth(dest);
        assert(dest_depth == 1);
        (void)(dest_depth);

        dest_n = dest_indices[0];

        src_depth = gtk_tree_path_get_depth(src);
        assert(src_depth == 1);
        (void)(src_depth);

        gtk_tree_model_get_iter(tree_model,
                                &iter, src);
        gtk_tree_model_get_value(tree_model,
                                 &iter, CTK_APC_RULE_MODEL_COL_ID, &id);

        // Move the rule to the right location
        ctk_apc_rule_model_set_abs_rule_priority(rule_model,
                                                 g_value_get_int(&id),
                                                 dest_n);
        gtk_tree_path_free(src);
    } else {
        // XXX text targets?
    }

    return TRUE;
}

static gboolean apc_rule_model_row_drop_possible(GtkTreeDragDest *drag_dest,
                                                 GtkTreePath *dest_path,
                                                 GtkSelectionData *selection_data)
{
    CtkApcRuleModel *rule_model;
    gint n, *indices, depth;

    rule_model = CTK_APC_RULE_MODEL(drag_dest);
    indices = gtk_tree_path_get_indices(dest_path);
    depth   = gtk_tree_path_get_depth(dest_path);

    assert(depth >= 1);
    n = indices[0];

    return (depth == 1) && (n >= 0) && (n <= rule_model->rules->len);
}

static GtkTreeModelFlags apc_rule_model_get_flags(GtkTreeModel *tree_model)
{
    return GTK_TREE_MODEL_LIST_ONLY;
}

static gint apc_rule_model_get_n_columns(GtkTreeModel *tree_model)
{
    return CTK_APC_RULE_MODEL_N_COLUMNS;
}

static GType apc_rule_model_get_column_type(GtkTreeModel *tree_model, gint index)
{
    switch (index) {
    case CTK_APC_RULE_MODEL_COL_ID:
        return G_TYPE_INT;
    case CTK_APC_RULE_MODEL_COL_FEATURE:
        return G_TYPE_STRING;
    case CTK_APC_RULE_MODEL_COL_MATCHES:
        return G_TYPE_STRING;
    case CTK_APC_RULE_MODEL_COL_PROFILE_NAME:
        return G_TYPE_STRING;
    case CTK_APC_RULE_MODEL_COL_FILENAME:
        return G_TYPE_STRING;
    default:
        assert(0);
        return G_TYPE_INVALID;
    }
}

static gboolean apc_rule_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
    CtkApcRuleModel *rule_model;
    gint depth, *indices;
    intptr_t n;

    assert(path);
    rule_model = CTK_APC_RULE_MODEL(tree_model);

    indices = gtk_tree_path_get_indices(path);
    depth   = gtk_tree_path_get_depth(path);

    assert(depth == 1);
    (void)(depth);

    n = indices[0];

    if (n >= rule_model->rules->len || n < 0) {
        return FALSE;
    }

    iter->stamp = rule_model->stamp;

    iter->user_data = (gpointer)n;
    iter->user_data2 = NULL; // unused
    iter->user_data3 = NULL; // unused

    return TRUE;
}

static GtkTreePath *apc_rule_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
    GtkTreePath *path;
    intptr_t n = (intptr_t)iter->user_data;

    g_return_val_if_fail(iter, NULL);

    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, n);

    return path;
}

static void apc_rule_model_get_value(GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value)
{
    const char *filename;
    CtkApcRuleModel *rule_model;
    const json_t *rule;
    const json_t *rule_pattern;
    int rule_id;
    intptr_t n;

    g_value_init(value, apc_rule_model_get_column_type(tree_model, column));
    rule_model = CTK_APC_RULE_MODEL(tree_model);

    n = (intptr_t)iter->user_data;
    rule_id = g_array_index(rule_model->rules, gint, n);

    rule = nv_app_profile_config_get_rule(rule_model->config,
                                          rule_id);
    rule_pattern = json_object_get(rule, "pattern");

    switch (column) {
    case CTK_APC_RULE_MODEL_COL_ID:
        g_value_set_int(value, rule_id);
        break;
    case CTK_APC_RULE_MODEL_COL_FEATURE:
        g_value_set_string(value, json_string_value(json_object_get(rule_pattern, "feature")));
        break;
    case CTK_APC_RULE_MODEL_COL_MATCHES:
        g_value_set_string(value, json_string_value(json_object_get(rule_pattern, "matches")));
        break;
    case CTK_APC_RULE_MODEL_COL_PROFILE_NAME:
        g_value_set_string(value, json_string_value(json_object_get(rule, "profile")));
        break;
    case CTK_APC_RULE_MODEL_COL_FILENAME:
        filename = nv_app_profile_config_get_rule_filename(rule_model->config, rule_id);
        assert(filename);
        g_value_set_string(value, filename);
        break;
    default:
        assert(0);
        break;
    }
}

static gboolean apc_rule_model_iter_next(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter)
{
    CtkApcRuleModel *rule_model;
    intptr_t n;

    rule_model = CTK_APC_RULE_MODEL(tree_model);

    if (!iter) {
        return FALSE;
    }

    n = (intptr_t)iter->user_data;
    n++;

    if (n >= rule_model->rules->len) {
        return FALSE;
    }

    iter->user_data = (gpointer)n;
    
    return TRUE;
}

static gboolean apc_rule_model_iter_children(GtkTreeModel *tree_model,
                                             GtkTreeIter *iter,
                                             GtkTreeIter *parent)
{
    CtkApcRuleModel *rule_model = CTK_APC_RULE_MODEL(tree_model);

    if (parent) {
        return FALSE;
    }

    // (parent == NULL) => return first profile

    if (!rule_model->rules->len) {
        return FALSE;
    }

    iter->stamp = rule_model->stamp;
    iter->user_data = (gpointer)0;
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;

    return TRUE;
}

static gboolean apc_rule_model_iter_has_child(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter)
{
    return FALSE;
}

static gint apc_rule_model_iter_n_children(GtkTreeModel *tree_model,
                                           GtkTreeIter *iter)
{
    CtkApcRuleModel *rule_model = CTK_APC_RULE_MODEL(tree_model);

    return iter ? 0 : rule_model->rules->len;
}

static gboolean
apc_rule_model_iter_nth_child(GtkTreeModel *tree_model,
                              GtkTreeIter *iter,
                              GtkTreeIter  *parent,
                              gint          n_in)
{
    CtkApcRuleModel *rule_model = CTK_APC_RULE_MODEL(tree_model);
    intptr_t n = (intptr_t)n_in;

    if (parent ||
        (n < 0) ||
        (n >= rule_model->rules->len)) {
        return FALSE;
    }

    iter->stamp = rule_model->stamp;
    iter->user_data = (gpointer)n;
    iter->user_data2 = NULL; // unused
    iter->user_data3 = NULL; // unused

    return TRUE;
}

static gboolean
apc_rule_model_iter_parent(GtkTreeModel *tree_model,
                           GtkTreeIter *iter,
                           GtkTreeIter *child)
{
    return FALSE;
}

void ctk_apc_rule_model_attach(CtkApcRuleModel *rule_model, AppProfileConfig *config)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    json_t *rule;
    AppProfileConfigRuleIter *rule_iter;
    gint i;
    gint id;

    rule_model->config = config;

    // Clear existing rules from the model
    path = gtk_tree_path_new_from_indices(0, -1);
    for (i = 0; i < rule_model->rules->len; i++) {
        // Emit a "row-deleted" signal for each deleted rule
        // (we can just keep calling this on row 0)
        gtk_tree_model_row_deleted(GTK_TREE_MODEL(rule_model), path);
    }
    gtk_tree_path_free(path);
    g_array_set_size(rule_model->rules, 0);
    
    // Load rules from the config into the model
    for (rule_iter = nv_app_profile_config_rule_iter(config), i = 0;
         rule_iter;
         rule_iter = nv_app_profile_config_rule_iter_next(rule_iter)) {
        rule = nv_app_profile_config_rule_iter_val(rule_iter);
        id = (int)json_integer_value(json_object_get(rule, "id"));
        g_array_append_val(rule_model->rules, id);

        // Emit a "row-inserted" signal for each new rule
        path = gtk_tree_path_new_from_indices(i++, -1);
        apc_rule_model_get_iter(GTK_TREE_MODEL(rule_model), &iter, path);
        gtk_tree_model_row_inserted(GTK_TREE_MODEL(rule_model), path, &iter);
        gtk_tree_path_free(path);
    }
}

CtkApcRuleModel *ctk_apc_rule_model_new(AppProfileConfig *config)
{
    CtkApcRuleModel *rule_model;

    rule_model = CTK_APC_RULE_MODEL(g_object_new(CTK_TYPE_APC_RULE_MODEL, NULL));
    assert(rule_model);

    ctk_apc_rule_model_attach(rule_model, config);

    return rule_model;
}

int ctk_apc_rule_model_create_rule(CtkApcRuleModel *rule_model,
                                   const char *filename,
                                   json_t *new_rule)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    gint n;
    int rule_id;

    rule_id = nv_app_profile_config_create_rule(rule_model->config,
                                                filename,
                                                new_rule);

    n = (gint)nv_app_profile_config_get_rule_priority(rule_model->config,
                                                      rule_id);
    g_array_insert_val(rule_model->rules, n, rule_id);

    // Emit a "row-inserted" signal
    path = gtk_tree_path_new_from_indices(n, -1);
    apc_rule_model_get_iter(GTK_TREE_MODEL(rule_model), &iter, path);
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(rule_model), path, &iter);
    gtk_tree_path_free(path);

    return rule_id;
}


static gint find_index_of_rule(CtkApcRuleModel *rule_model, int id)
{
    gint i;
    for (i = 0; i < rule_model->rules->len; i++) {
        if (g_array_index(rule_model->rules, gint, i) == id) {
            return i;
        }
    }

    return -1;
}

void ctk_apc_rule_model_update_rule(CtkApcRuleModel *rule_model,
                                    const char *filename,
                                    int id,
                                    json_t *rule)
{
    int rule_moved;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint n;
    size_t new_pri, old_pri;
    gint *new_order;
    GArray *new_rules;
    int cur_id;

    rule_moved = nv_app_profile_config_update_rule(rule_model->config,
                                                   filename,
                                                   id,
                                                   rule);

    if (rule_moved) {
        // Compute the new ordering
        new_rules = g_array_new(FALSE, FALSE, sizeof(gint));
        new_order = malloc(sizeof(gint) * rule_model->rules->len);

        for (old_pri = 0; old_pri < rule_model->rules->len; old_pri++) {
            cur_id = g_array_index(rule_model->rules, gint, old_pri);
            new_pri = nv_app_profile_config_get_rule_priority(rule_model->config, cur_id);
            new_order[new_pri] = old_pri;
        }

        for (new_pri = 0; new_pri < rule_model->rules->len; new_pri++) {
            cur_id = g_array_index(rule_model->rules, gint, new_order[new_pri]);
            g_array_append_val(new_rules, cur_id);
        }

        g_array_free(rule_model->rules, TRUE);
        rule_model->rules = new_rules;

        // emit a "rows-reordered" signal
        path = gtk_tree_path_new();
        gtk_tree_model_rows_reordered(GTK_TREE_MODEL(rule_model),
                                      path, NULL, new_order);
        gtk_tree_path_free(path);
        free(new_order);
    } else {
        // emit a "row-changed" signal
        n = find_index_of_rule(rule_model, id);
        path = gtk_tree_path_new_from_indices(n, -1);
        apc_rule_model_get_iter(GTK_TREE_MODEL(rule_model), &iter, path);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(rule_model), path, &iter);
        gtk_tree_path_free(path);
    }
}

void ctk_apc_rule_model_delete_rule(CtkApcRuleModel *rule_model, int id)
{
    GtkTreePath *path;
    gint n;

    n = find_index_of_rule(rule_model, id);
    assert(n >= 0);

    nv_app_profile_config_delete_rule(rule_model->config, id);
    g_array_remove_index(rule_model->rules, n);

    // emit a "row-deleted" signal
    path = gtk_tree_path_new_from_indices(n, -1);
    gtk_tree_model_row_deleted(GTK_TREE_MODEL(rule_model), path);
    gtk_tree_path_free(path);
}

static void apc_rule_model_post_set_rule_priority_common(CtkApcRuleModel *rule_model,
                                                         int id)
{
    gint *new_order;
    size_t old_pri, new_pri;
    int cur_id;
    gint n;
    GArray *new_rules;
    GtkTreePath *path;
    GtkTreeIter iter;

    // Compute the new ordering
    new_rules = g_array_new(FALSE, FALSE, sizeof(gint));
    new_order = malloc(sizeof(gint) * rule_model->rules->len);

    for (old_pri = 0; old_pri < rule_model->rules->len; old_pri++) {
        cur_id = g_array_index(rule_model->rules, gint, old_pri);
        new_pri = nv_app_profile_config_get_rule_priority(rule_model->config, cur_id);
        new_order[new_pri] = old_pri;
    }

    for (new_pri = 0; new_pri < rule_model->rules->len; new_pri++) {
        cur_id = g_array_index(rule_model->rules, gint, new_order[new_pri]);
        g_array_append_val(new_rules, cur_id);
    }

    g_array_free(rule_model->rules, TRUE);
    rule_model->rules = new_rules;

    // emit a "rows-reordered" signal
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(GTK_TREE_MODEL(rule_model),
                                  path, NULL, new_order);
    gtk_tree_path_free(path);
    free(new_order);
    
    // emit a "row-changed" signal for the rule whose priority has changed
    n = find_index_of_rule(rule_model, id);

    path = gtk_tree_path_new_from_indices(n, -1);
    apc_rule_model_get_iter(GTK_TREE_MODEL(rule_model), &iter, path);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(rule_model), path, &iter);
    gtk_tree_path_free(path);
}

void ctk_apc_rule_model_set_abs_rule_priority(CtkApcRuleModel *rule_model,
                                              int id, size_t pri)
{
    nv_app_profile_config_set_abs_rule_priority(rule_model->config, id, pri);
    apc_rule_model_post_set_rule_priority_common(rule_model, id);
}

void ctk_apc_rule_model_change_rule_priority(CtkApcRuleModel *rule_model,
                                            int id, int delta)
{
    nv_app_profile_config_change_rule_priority(rule_model->config, id, delta);
    apc_rule_model_post_set_rule_priority_common(rule_model, id);
}
