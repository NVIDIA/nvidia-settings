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

#include <gtk/gtk.h>
#include <stdint.h>
#include "ctkapcprofilemodel.h"
#include "ctkappprofile.h"
#include <string.h>
#include <stdlib.h>

static GObjectClass *parent_class = NULL;

// Forward declarations
static void apc_profile_model_init(CtkApcProfileModel *prof_model, gpointer);
static void apc_profile_model_finalize(GObject *object);
static void apc_profile_model_tree_model_init(GtkTreeModelIface *iface, gpointer);
static GtkTreeModelFlags apc_profile_model_get_flags(GtkTreeModel *tree_model);
static gint apc_profile_model_get_n_columns(GtkTreeModel *tree_model);
static GType apc_profile_model_get_column_type(GtkTreeModel *tree_model, gint index);
static gboolean apc_profile_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
static GtkTreePath *apc_profile_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter);
static gboolean apc_profile_model_iter_next(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter);
static gboolean apc_profile_model_iter_children(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *parent);
static gboolean apc_profile_model_iter_has_child(GtkTreeModel *tree_model,
                                                 GtkTreeIter *iter);
static gint apc_profile_model_iter_n_children(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter);
static gboolean
apc_profile_model_iter_nth_child(GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreeIter  *parent,
                                 gint          n);
static gboolean apc_profile_model_iter_parent(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter,
                                              GtkTreeIter *child);
static void apc_profile_model_class_init(CtkApcProfileModelClass *klass, gpointer);
static void apc_profile_model_get_value(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        gint column,
                                        GValue *value);


static void
apc_profile_model_tree_sortable_init(GtkTreeSortableIface *iface, gpointer);

static gboolean apc_profile_model_get_sort_column_id(GtkTreeSortable *sortable,
                                                     gint *sort_column_id,
                                                     GtkSortType *order);
static void apc_profile_model_set_sort_column_id(GtkTreeSortable *sortable,
                                                 gint sort_column_id,
                                                 GtkSortType order);
static void apc_profile_model_set_sort_func(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkTreeIterCompareFunc sort_func,
                                            gpointer user_data,
                                            GDestroyNotify destroy);
static void apc_profile_model_set_default_sort_func(GtkTreeSortable *sortable,
                                                    GtkTreeIterCompareFunc sort_func,
                                                    gpointer user_data,
                                                    GDestroyNotify destroy);
static gboolean apc_profile_model_has_default_sort_func(GtkTreeSortable *sortable);


GType ctk_apc_profile_model_get_type(void)
{
    static GType apc_profile_model_type = 0;
    if (!apc_profile_model_type) {
        static const GTypeInfo apc_profile_model_info = {
            sizeof (CtkApcProfileModelClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) apc_profile_model_class_init, /* constructor */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkApcProfileModel),
            0,    /* n_preallocs */
            (GInstanceInitFunc) apc_profile_model_init, /* instance_init */
            NULL  /* value_table */
        };
        static const GInterfaceInfo tree_model_info =
        {
            (GInterfaceInitFunc) apc_profile_model_tree_model_init, /* interface_init */
            NULL, /* interface_finalize */
            NULL  /* interface_data */
        };
        static const GInterfaceInfo tree_sortable_info =
        {
            (GInterfaceInitFunc) apc_profile_model_tree_sortable_init, /* interface_init */
            NULL, /* interface_finalize */
            NULL  /* interface_data */
        };

        apc_profile_model_type =
            g_type_register_static(G_TYPE_OBJECT, "CtkApcProfileModel",
                                   &apc_profile_model_info, 0);

        g_type_add_interface_static(apc_profile_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
        g_type_add_interface_static(apc_profile_model_type, GTK_TYPE_TREE_SORTABLE, &tree_sortable_info);
    }

    return apc_profile_model_type;
}

static void apc_profile_model_class_init(CtkApcProfileModelClass *klass, gpointer class_data)
{
    GObjectClass *object_class;

    parent_class = (GObjectClass *)g_type_class_peek_parent(klass);
    object_class = (GObjectClass *)klass;

    object_class->finalize = apc_profile_model_finalize;
}

static gint apc_profile_model_sort_name(GtkTreeModel *model,
                                        GtkTreeIter *a,
                                        GtkTreeIter *b,
                                        gpointer user_data)
{
    gint result;
    gchar *profile_name_a, *profile_name_b;
    gtk_tree_model_get(model, a, CTK_APC_PROFILE_MODEL_COL_NAME, &profile_name_a, -1);
    gtk_tree_model_get(model, b, CTK_APC_PROFILE_MODEL_COL_NAME, &profile_name_b, -1);
    result = strcmp(profile_name_a, profile_name_b);
    free(profile_name_a);
    free(profile_name_b);
    return result;
}

static gint apc_profile_model_sort_filename(GtkTreeModel *model,
                                            GtkTreeIter *a,
                                            GtkTreeIter *b,
                                            gpointer user_data)
{
    gint result;
    gchar *filename_a, *filename_b;
    gtk_tree_model_get(model, a, CTK_APC_PROFILE_MODEL_COL_FILENAME, &filename_a, -1);
    gtk_tree_model_get(model, b, CTK_APC_PROFILE_MODEL_COL_FILENAME, &filename_b, -1);
    result = strcmp(filename_a, filename_b);
    free(filename_a);
    free(filename_b);
    return result;
}

static gint apc_profile_model_sort_settings(GtkTreeModel *model,
                                            GtkTreeIter *a,
                                            GtkTreeIter *b,
                                            gpointer user_data)
{
    gint result;
    json_t *settings_a, *settings_b;
    char *settings_string_a, *settings_string_b;
    gtk_tree_model_get(model, a, CTK_APC_PROFILE_MODEL_COL_SETTINGS, &settings_a, -1);
    gtk_tree_model_get(model, b, CTK_APC_PROFILE_MODEL_COL_SETTINGS, &settings_b, -1);
    settings_string_a = serialize_settings(settings_a, FALSE);
    settings_string_b = serialize_settings(settings_b, FALSE);
    result = strcmp(settings_string_a, settings_string_b);
    free(settings_string_a);
    free(settings_string_b);
    json_decref(settings_a);
    json_decref(settings_b);
    return result;
}

static void apc_profile_model_init(CtkApcProfileModel *prof_model, gpointer g_class)
{
    prof_model->stamp = g_random_int(); // random int to catch iterator type mismatches
    prof_model->config = NULL;

    prof_model->profiles = g_array_new(FALSE, TRUE, sizeof(char *));
    prof_model->sort_column_id = CTK_APC_PROFILE_MODEL_DEFAULT_SORT_COL;
    prof_model->order = GTK_SORT_DESCENDING;

    prof_model->sort_funcs[CTK_APC_PROFILE_MODEL_COL_NAME] = apc_profile_model_sort_name;
    prof_model->sort_funcs[CTK_APC_PROFILE_MODEL_COL_FILENAME] = apc_profile_model_sort_filename;
    prof_model->sort_funcs[CTK_APC_PROFILE_MODEL_COL_SETTINGS] = apc_profile_model_sort_settings;

    memset(prof_model->sort_user_data, 0, sizeof(prof_model->sort_user_data));
    memset(prof_model->sort_destroy_notify, 0, sizeof(prof_model->sort_destroy_notify));
}

static void apc_profile_model_finalize(GObject *object)
{
    guint i;
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(object);

    for (i = 0; i < prof_model->profiles->len; i++) {
        char *profile_name = g_array_index(prof_model->profiles, char*, i);
        free(profile_name);
    }

    for (i = 0; i < CTK_APC_PROFILE_MODEL_N_COLUMNS; i++) {
        if (prof_model->sort_destroy_notify[i]) {
            (*prof_model->sort_destroy_notify[i])(
                    prof_model->sort_user_data[i]
            );
        }
    }
    
    g_array_free(prof_model->profiles, TRUE);
    parent_class->finalize(object);
}

static void
apc_profile_model_tree_model_init(GtkTreeModelIface *iface,
                                  gpointer iface_data)
{
    iface->get_flags       = apc_profile_model_get_flags;
    iface->get_n_columns   = apc_profile_model_get_n_columns;
    iface->get_column_type = apc_profile_model_get_column_type;
    iface->get_iter        = apc_profile_model_get_iter;
    iface->get_path        = apc_profile_model_get_path;
    iface->get_value       = apc_profile_model_get_value;
    iface->iter_next       = apc_profile_model_iter_next;
    iface->iter_children   = apc_profile_model_iter_children;
    iface->iter_has_child  = apc_profile_model_iter_has_child;
    iface->iter_n_children = apc_profile_model_iter_n_children;
    iface->iter_nth_child  = apc_profile_model_iter_nth_child;
    iface->iter_parent     = apc_profile_model_iter_parent;
}

static void
apc_profile_model_tree_sortable_init(GtkTreeSortableIface *iface,
                                     gpointer iface_data)
{
    iface->get_sort_column_id = apc_profile_model_get_sort_column_id;
    iface->set_sort_column_id = apc_profile_model_set_sort_column_id;
    iface->set_sort_func = apc_profile_model_set_sort_func;
    iface->set_default_sort_func = apc_profile_model_set_default_sort_func;
    iface->has_default_sort_func = apc_profile_model_has_default_sort_func;
}

static GtkTreeModelFlags apc_profile_model_get_flags(GtkTreeModel *tree_model)
{
    return GTK_TREE_MODEL_LIST_ONLY;
}

static gint apc_profile_model_get_n_columns(GtkTreeModel *tree_model)
{
    return CTK_APC_PROFILE_MODEL_N_COLUMNS;
}

static GType apc_profile_model_get_column_type(GtkTreeModel *tree_model, gint index)
{
    switch (index) {
    case CTK_APC_PROFILE_MODEL_COL_NAME:
        return G_TYPE_STRING;
    case CTK_APC_PROFILE_MODEL_COL_FILENAME:
        return G_TYPE_STRING;
    case CTK_APC_PROFILE_MODEL_COL_SETTINGS:
        return G_TYPE_POINTER;
    default:
        assert(0);
        return G_TYPE_INVALID;
    }
}

static inline void set_iter_to_index(GtkTreeIter *iter, intptr_t idx)
{
    iter->user_data = (gpointer)idx;
    iter->user_data2 = NULL; // unused
    iter->user_data3 = NULL; // unused
}

static gboolean apc_profile_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
    CtkApcProfileModel *prof_model;
    gint depth, *indices;
    intptr_t n;

    assert(path);
    prof_model = CTK_APC_PROFILE_MODEL(tree_model);

    indices = gtk_tree_path_get_indices(path);
    depth   = gtk_tree_path_get_depth(path);

    assert(depth == 1);
    (void)(depth);

    n = indices[0];

    if (n >= prof_model->profiles->len || n < 0) {
        return FALSE;
    }

    iter->stamp = prof_model->stamp;
    set_iter_to_index(iter, n);

    return TRUE;
}

static GtkTreePath *apc_profile_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
    GtkTreePath *path;
    intptr_t n;

    g_return_val_if_fail(iter, NULL);

    n = (intptr_t)iter->user_data;

    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, n);

    return path;
}

static void apc_profile_model_get_value(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter,
                                        gint column,
                                        GValue *value)
{
    const char *profile_name;
    const char *filename;
    CtkApcProfileModel *prof_model;
    const json_t *profile;
    json_t *settings;
    intptr_t n;

    g_value_init(value, apc_profile_model_get_column_type(tree_model, column));
    prof_model = CTK_APC_PROFILE_MODEL(tree_model);
    
    n = (intptr_t)iter->user_data;
    profile_name = g_array_index(prof_model->profiles, char*, n);

    switch (column) {
    case CTK_APC_PROFILE_MODEL_COL_NAME:
        g_value_set_string(value, profile_name);
        break;
    case CTK_APC_PROFILE_MODEL_COL_FILENAME:
        filename = nv_app_profile_config_get_profile_filename(prof_model->config,
                                                              profile_name);
        assert(filename);
        g_value_set_string(value, filename);
        break;
    case CTK_APC_PROFILE_MODEL_COL_SETTINGS:
        profile = nv_app_profile_config_get_profile(prof_model->config,
                                                    profile_name);
        settings = json_deep_copy(json_object_get(profile, "settings"));
        assert(settings);

        g_value_set_pointer(value, settings);
        break;
    default:
        assert(0);
        break;
    }
}

static gboolean apc_profile_model_iter_next(GtkTreeModel *tree_model,
                                        GtkTreeIter *iter)
{
    CtkApcProfileModel *prof_model;
    intptr_t n;

    prof_model = CTK_APC_PROFILE_MODEL(tree_model);

    if (!iter) {
        return FALSE;
    }

    n = (intptr_t)iter->user_data;
    n++;

    if (n >= prof_model->profiles->len) {
        return FALSE;
    }

    set_iter_to_index(iter, n);
    
    return TRUE;
}

static gboolean apc_profile_model_iter_children(GtkTreeModel *tree_model,
                                                GtkTreeIter *iter,
                                                GtkTreeIter *parent)
{
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(tree_model);

    if (parent) {
        return FALSE;
    }

    // (parent == NULL) => return first profile

    if (!prof_model->profiles->len) {
        return FALSE;
    }

    iter->stamp = prof_model->stamp;
    set_iter_to_index(iter, 0);

    return TRUE;
}

static gboolean apc_profile_model_iter_has_child(GtkTreeModel *tree_model,
                                                 GtkTreeIter *iter)
{
    return FALSE;
}

static gint apc_profile_model_iter_n_children(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter)
{
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(tree_model);

    return iter ? 0 : (gint)prof_model->profiles->len;
}

static gboolean
apc_profile_model_iter_nth_child(GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter,
                                 GtkTreeIter  *parent,
                                 gint          n_in)
{
    intptr_t n = (intptr_t)n_in;
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(tree_model);

    if (parent ||
        (n < 0) ||
        (n >= prof_model->profiles->len)) {
        return FALSE;
    }

    iter->stamp = prof_model->stamp;
    set_iter_to_index(iter, n);

    return TRUE;
}

static gboolean
apc_profile_model_iter_parent(GtkTreeModel *tree_model,
                              GtkTreeIter *iter,
                              GtkTreeIter *child)
{
    return FALSE;
}

void ctk_apc_profile_model_attach(CtkApcProfileModel *prof_model, AppProfileConfig *config)
{
    GtkTreePath *path;
    GtkTreeIter iter;
    AppProfileConfigProfileIter *prof_iter;
    const char *profile_name, *dup_profile_name;
    gint i;
    prof_model->config = config;

    // Clear existing profiles from the model
    path = gtk_tree_path_new_from_indices(0, -1);
    for (i = 0; i < prof_model->profiles->len; i++) {
        // Emit a "row-deleted" signal for each deleted profile 
        // (we can just keep calling this on row 0)
        gtk_tree_model_row_deleted(GTK_TREE_MODEL(prof_model), path);
    }
    gtk_tree_path_free(path);
    g_array_set_size(prof_model->profiles, 0);

    // Load the profiles from the config into the model
    for (prof_iter = nv_app_profile_config_profile_iter(config), i = 0;
         prof_iter;
         prof_iter = nv_app_profile_config_profile_iter_next(prof_iter)) {
        profile_name = nv_app_profile_config_profile_iter_name(prof_iter);
        dup_profile_name = strdup(profile_name);
        g_array_append_val(prof_model->profiles, dup_profile_name);

        // emit a "row-inserted" signal for each new profile
        path = gtk_tree_path_new_from_indices(i++, -1);
        apc_profile_model_get_iter(GTK_TREE_MODEL(prof_model), &iter, path);
        gtk_tree_model_row_inserted(GTK_TREE_MODEL(prof_model), path, &iter);
        gtk_tree_path_free(path);
    }
}

CtkApcProfileModel *ctk_apc_profile_model_new(AppProfileConfig *config)
{
    CtkApcProfileModel *prof_model;

    prof_model = CTK_APC_PROFILE_MODEL(g_object_new(CTK_TYPE_APC_PROFILE_MODEL, NULL));
    assert(prof_model);

    ctk_apc_profile_model_attach(prof_model, config);

    return prof_model;
}

static gint find_index_of_profile(CtkApcProfileModel *prof_model, const char *profile_name)
{
    gint i;
    for (i = 0; i < prof_model->profiles->len; i++) {
        if (!strcmp(g_array_index(prof_model->profiles, char*, i), profile_name)) {
            return i;
        }
    }

    return -1;
}

void ctk_apc_profile_model_update_profile(CtkApcProfileModel *prof_model,
                                          const char *filename,
                                          const char *profile_name,
                                          json_t *profile)
{
    int profile_created;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint n;

    profile_created = nv_app_profile_config_update_profile(prof_model->config,
                                                           filename,
                                                           profile_name,
                                                           profile);

    if (profile_created) {
        char *dup_profile_name = strdup(profile_name);
        n = prof_model->profiles->len;
        g_array_append_val(prof_model->profiles, dup_profile_name);

        // emit a "row-inserted" signal
        path = gtk_tree_path_new_from_indices(n, -1);
        apc_profile_model_get_iter(GTK_TREE_MODEL(prof_model), &iter, path);
        gtk_tree_model_row_inserted(GTK_TREE_MODEL(prof_model), path, &iter);
        gtk_tree_path_free(path);

        // TODO resort the array if necessary
    } else {
        // emit a "row-changed" signal
        n = find_index_of_profile(prof_model, profile_name);
        path = gtk_tree_path_new_from_indices(n, -1);
        apc_profile_model_get_iter(GTK_TREE_MODEL(prof_model), &iter, path);
        gtk_tree_model_row_changed(GTK_TREE_MODEL(prof_model), path, &iter);
        gtk_tree_path_free(path);
    }
}

void ctk_apc_profile_model_delete_profile(CtkApcProfileModel *prof_model,
                                          const char *profile_name)
{
    GtkTreePath *path;
    gint n;
    
    n = find_index_of_profile(prof_model, profile_name);
    assert(n >= 0);

    nv_app_profile_config_delete_profile(prof_model->config, profile_name);
    g_array_remove_index(prof_model->profiles, n);
    
    // emit a "row-deleted" signal
    path = gtk_tree_path_new_from_indices(n, -1);
    gtk_tree_model_row_deleted(GTK_TREE_MODEL(prof_model), path);
    gtk_tree_path_free(path);
}

// XXX Not available in GTK+-2.2.1
#ifndef GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID
# define GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID -2
#endif

static gboolean apc_profile_model_get_sort_column_id(GtkTreeSortable *sortable,
                                                     gint *sort_column_id,
                                                     GtkSortType *order)
{
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(sortable);
    if (sort_column_id) {
        *sort_column_id = prof_model->sort_column_id;
    }
    if (order) {
        *order = prof_model->order;
    }

    return (prof_model->sort_column_id != GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID) &&
           (prof_model->sort_column_id != GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID);
}

static gint compare_profiles(gconstpointer a, gconstpointer b, gpointer user_data)
{
    gint result;
    GtkTreeIter iter_a, iter_b;
    intptr_t ia, ib;
    char * const *buf;
    gint sort_column_id;
    CtkApcProfileModel *prof_model = (CtkApcProfileModel *)user_data;

    sort_column_id = prof_model->sort_column_id;
    if (sort_column_id <= GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID) {
        return 0;
    }

    // XXX hack: get the position of these pointers in the container array
    buf = (char * const *)prof_model->profiles->data;
    ia = ((char * const *)a - buf);
    ib = ((char * const *)b - buf);

    // Build iterators to be used by the comparison function
    iter_a.stamp = prof_model->stamp;
    set_iter_to_index(&iter_a, ia);

    iter_b.stamp = prof_model->stamp;
    set_iter_to_index(&iter_b, ib);

    result = (*prof_model->sort_funcs[sort_column_id])(GTK_TREE_MODEL(prof_model),
                                                       &iter_a,
                                                       &iter_b,
                                                       prof_model->sort_user_data[sort_column_id]);

    return (prof_model->order == GTK_SORT_DESCENDING) ? -result : result;
}

static void apc_profile_model_resort(CtkApcProfileModel *prof_model)
{
    // Emit the "sort-column-changed" signal
    gtk_tree_sortable_sort_column_changed(GTK_TREE_SORTABLE(prof_model));

    // Reorder the model based on the sort func
    g_array_sort_with_data(prof_model->profiles, compare_profiles, (gpointer)prof_model);
}

static void apc_profile_model_set_sort_column_id(GtkTreeSortable *sortable,
                                                 gint sort_column_id,
                                                 GtkSortType order)
{
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(sortable);

    if ((prof_model->sort_column_id != sort_column_id) ||
        (prof_model->order != order)) {
        prof_model->sort_column_id = sort_column_id;
        prof_model->order = order;

        apc_profile_model_resort(prof_model);
    }
}

static void apc_profile_model_set_sort_func(GtkTreeSortable *sortable,
                                            gint sort_column_id,
                                            GtkTreeIterCompareFunc sort_func,
                                            gpointer user_data,
                                            GDestroyNotify destroy)
{
    CtkApcProfileModel *prof_model = CTK_APC_PROFILE_MODEL(sortable);

    g_return_if_fail((sort_column_id >= 0) &&
                     (sort_column_id < CTK_APC_PROFILE_MODEL_N_COLUMNS));

    prof_model->sort_funcs[sort_column_id] = sort_func;

    if (prof_model->sort_destroy_notify[sort_column_id]) {
        (*prof_model->sort_destroy_notify[sort_column_id])(
            prof_model->sort_user_data[sort_column_id]
        );
    }

    prof_model->sort_user_data[sort_column_id] = user_data;
    prof_model->sort_destroy_notify[sort_column_id] = destroy;

    if (sort_column_id == prof_model->sort_column_id) {
        apc_profile_model_resort(prof_model);
    }
}
static void apc_profile_model_set_default_sort_func(GtkTreeSortable *sortable,
                                                    GtkTreeIterCompareFunc sort_func,
                                                    gpointer user_data,
                                                    GDestroyNotify destroy)
{
    // do nothing
}

static gboolean apc_profile_model_has_default_sort_func(GtkTreeSortable *sortable)
{
    return FALSE;
}
