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

/*
 * app-profiles.h - prototypes for querying and assigning application profile
 * settings, as well as parsing and saving application profile configuration
 * files.
 */
#ifndef __APP_PROFILES_H__
#define __APP_PROFILES_H__

#include <assert.h>
#include <jansson.h>

/*
 * A comment on the format of parsed configuration files:
 *
 * Each parsed file is described by a JSON object, with the following members:
 *     filename: This is the name of the file.
 *     rules: this is the same as the rules array in the original configuration file,
 *            except inline profiles are moved into the profiles array. Also see
 *            below.
 *     profiles: this is a JSON object which is used as a per-file hashtable
 *               mapping profile names to profiles. Note this differs from the
 *               format of the original configuration file, which stores
 *               profiles in an array.
 *     order: this is a JSON object which describes the priority of the file
 *            in the search path. This contains the following members:
 *         major: the index of the top-level search path entry
 *         minor: 0 if the file is in the top-level search path, or otherwise the order
 *                of the file within the top-level directory as determined by
 *                strcoll(3).
 *     dirty: a flag indicating this file should be overwritten even if
 *     validation does not detect any changes (used to handle invalid
 *     configuration, e.g. duplicate profile names).
 *     new: indicates whether the file object is new to the configuration or
 *     loaded from disk
 *     atime: time of last access as determined by time(2) during the
 *     nv_app_profile_config_load() call. Used to check if our local copy
 *     of the config file is stale. Undefined for new files
 *
 * Each rule in the configuration contains the following attributes:
 *     id: This is a unique integer that identifies the rule.
 *     pattern: This is the same as the pattern attribute in the rule read in
 *              from the original configuration.
 *     profile: This is a string describing the name of the profile applied by
 *              the rule.
 *
 * The priority of the rule is implicitly determined by its position in the
 * configuration.
 *
 * Each profile in the configuration contains the following attributes:
 *     settings: This is an array of setting objects, and is the same as
 *               the settings array of the original profile read in from the
 *               original configuration.
 *
 * The name of the profile is implicitly determined by its key in its container
 * JSON object.
 */

/*
 * The AppProfileConfig struct contains the current profile configuration
 * as determined by nvidia-settings. This configuration contains a list
 * of files which contain rules and profiles.
 */
typedef struct AppProfileConfigRec {
    /*
     * JSON object containing our global app profile options. Currently
     * contains the following members:
     * enabled: boolean indicating whether app profiles are enabled.
     */
    json_t *global_options;

    /*
     * JSON array of parsed files, each containing a rules and profiles array
     * along with other metadata.
     */
    json_t *parsed_files;

    /*
     * We maintain secondary hashtables of profile and rule locations stored as
     * JSON objects for quicker lookup of individual profiles and rules. This is
     * also used to ensure that globally there are no two profiles with the same
     * name (regardless of their file of origin).
     */
    json_t *profile_locations;
    json_t *rule_locations;
    size_t next_free_rule_id;

    /*
     * Copy of the global configuration filename
     */
    char *global_config_file;

    /*
     * Copy of the search path. This is needed to determine the order of files
     * in the parsed_files array above
     */
    char **search_path;
    size_t search_path_count;
} AppProfileConfig;

/*
 * Save configuration specified by the JSON array updates to disk. See
 * nv_app_profile_config_validate() below for the format of this array.
 * backup indicates whether this should also make backups of the original files
 * before saving.
 * Returns 0 if successful, or a negative integer if an error was encountered.
 * If error_str is non-NULL, *error_str is set to NULL on success, or a
 * dynamically-allocated string if an error occurred.
 */
int nv_app_profile_config_save_updates(AppProfileConfig *config,
                                       json_t *updates, int backup,
                                       char **error_str);

/*
 * Load an application profile configuration from disk, using a list of files specified by search_path.
 */
AppProfileConfig *nv_app_profile_config_load(const char *global_config_file,
                                             char **search_path,
                                             size_t search_path_count);

/*
 * Load the registry keys documentation from the installed file.
 */
json_t *nv_app_profile_key_documentation_load(const char *key_docs_file);

/*
 * Duplicate the configuration; the copy can then be edited and compared against
 * the original.
 */
AppProfileConfig *nv_app_profile_config_dup(AppProfileConfig *old_config);

/*
 * Toggle whether application profiles are enabled for this user.
 */
void nv_app_profile_config_set_enabled(AppProfileConfig *config,
                                       int enabled);

/*
 * Destroy the configuration and free its backing memory.
 */
void nv_app_profile_config_free(AppProfileConfig *config);

/*
 * Validate the configuration specified by new_config against the pristine copy
 * specified by old_config, and generate a list of changes needed in order to
 * achieve the new configuration.
 *
 * This returns a JSON array which must be freed via json_decref(), containing
 * a list of update objects. Each update object contains the following
 * attributes:
 *
 * filename (string): the filename of the file that changed
 * text (string): the new JSON that should be used
 *
 * This array can be passed directly to nv_app_profile_config_save_updates() to
 * be saved to disk.
 */
json_t *nv_app_profile_config_validate(AppProfileConfig *new_config,
                                       AppProfileConfig *old_config);

#define INVALID_RULE_ID -1

// Accessor functions
const json_t *nv_app_profile_config_get_profile(AppProfileConfig *config,
                                                const char *profile_name);

const json_t *nv_app_profile_config_get_rule(AppProfileConfig *config,
                                             int id);

const char *nv_app_profile_config_get_rule_filename(AppProfileConfig *config,
                                                    int id);

const char *nv_app_profile_config_get_profile_filename(AppProfileConfig *config,
                                                       const char *profile_name);

int nv_app_profile_config_get_enabled(AppProfileConfig *config);


// Opaque iterator functions
typedef struct AppProfileConfigProfileIterRec AppProfileConfigProfileIter;
typedef struct AppProfileConfigRuleIterRec AppProfileConfigRuleIter;

AppProfileConfigProfileIter *nv_app_profile_config_profile_iter(AppProfileConfig *config);
AppProfileConfigProfileIter *nv_app_profile_config_profile_iter_next(AppProfileConfigProfileIter *iter);
const char *nv_app_profile_config_profile_iter_filename(AppProfileConfigProfileIter *iter);
const char *nv_app_profile_config_profile_iter_name(AppProfileConfigProfileIter *iter);
json_t *nv_app_profile_config_profile_iter_val(AppProfileConfigProfileIter *iter);

AppProfileConfigRuleIter *nv_app_profile_config_rule_iter(AppProfileConfig *config);
AppProfileConfigRuleIter *nv_app_profile_config_rule_iter_next(AppProfileConfigRuleIter *iter);
json_t *nv_app_profile_config_rule_iter_val(AppProfileConfigRuleIter *iter);
size_t nv_app_profile_config_rule_iter_pri(AppProfileConfigRuleIter *iter);
const char *nv_app_profile_config_rule_iter_filename(AppProfileConfigRuleIter *iter);

size_t nv_app_profile_config_count_rules(AppProfileConfig *config);

/*
 * Update or create a new profile. Returns TRUE if this operation resulted in
 * the creation of a new profile.
 */
int nv_app_profile_config_update_profile(AppProfileConfig *config,
                                         const char *filename,
                                         const char *profile_name,
                                         json_t *new_profile);

/*
 * Delete a profile from the configuration.
 */
void nv_app_profile_config_delete_profile(AppProfileConfig *config,
                                          const char *profile_name);

/*
 * Create a new rule. The rule will be given priority over all rules defined in
 * the same file in the search path. Returns the ID of the newly-created rule.
 * The memory pointed to by rule must be allocated by malloc(), and should not
 * be freed explicitly by the caller.
 */
int nv_app_profile_config_create_rule(AppProfileConfig *config,
                                      const char *filename,
                                      json_t *new_rule);

/*
 * Update the attributes of an existing rule by its ID. If filename is
 * non-NULL and differs from the file in which the rule currently resides,
 * this operation will move the rule to the file specified by filename,
 * potentially altering the rule's priority in the process. Returns TRUE
 * if the rule is moved as a result of this operation.
 */
int nv_app_profile_config_update_rule(AppProfileConfig *config,
                                      const char *filename,
                                      int id,
                                      json_t *rule);

/*
 * Delete the rule with the given ID from the configuration.
 */
void nv_app_profile_config_delete_rule(AppProfileConfig *config, int id);

size_t nv_app_profile_config_get_rule_priority(AppProfileConfig *config,
                                               int id);

void nv_app_profile_config_set_abs_rule_priority(AppProfileConfig *config,
                                                 int id, size_t pri);

/*
 * Change the priority of the rule in the rule list. Existing rules with
 * equal or lower priority in the list will be moved down in the list to make
 * room. This operation also changes the source file of the rule as needed to
 * ensure the rule's priority will be consistent with its location in the search
 * path.
 */
void nv_app_profile_config_change_rule_priority(AppProfileConfig *config,
                                                int id, int delta);

/*
 * This function walks the list of parsed files, and marks any files which have
 * changed since the configuration was loaded on disk dirty for the validation
 * step to generate an update for them.
 *
 * This function returns TRUE if any files read in this configuration have
 * changed since this configuration was loaded from disk.
 */
int nv_app_profile_config_check_backing_files(AppProfileConfig *config);

/*
 * Utility function to strip comments and translate hex/octal values to decimal
 * so the JSON parser can understand.
 */
char *nv_app_profile_file_syntax_to_json(const char *orig_s);

int nv_app_profile_config_check_valid_source_file(AppProfileConfig *config,
                                                  const char *filename,
                                                  char **reason);

/*
 * Constructs a list of suggested filenames from the default search path and
 * parsed files list. If an item in the search path appears in the prefix of a
 * path to a parsed file, it will *not* appear in the list. The resulting json_t
 * object should be freed via json_decref() once the caller is done using it.
 */
json_t *nv_app_profile_config_get_source_filenames(AppProfileConfig *config);

/*
 * Given a valid filename in the search path, this function constructs the name
 * of the file that will be used as a backup file for display to the end user.
 * The returned string should be freed by the caller.
 */
char *nv_app_profile_config_get_backup_filename(AppProfileConfig *config, const char *filename);

/*
 * Constructs an unused profile name for use with a new profile. The returned
 * string should be freed by the caller.
 */
char *nv_app_profile_config_get_unused_profile_name(AppProfileConfig *config);

/*
 * This function looks for all rules referring to the profile name given by
 * orig_name and updates them to refer to the profile name given by new_name.
 * Returns TRUE if any rules have been altered as a result of this operation.
 */
int nv_app_profile_config_profile_name_change_fixup(AppProfileConfig *config,
                                                    const char *orig_name,
                                                    const char *new_name);

#endif // __APP_PROFILES_H__
