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
 * app-profiles.c - this source file contains functions for querying and
 * assigning application profile settings, as well as parsing and saving
 * application profile configuration files.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include "common-utils.h"
#include "app-profiles.h"
#include "msg.h"

/*
 * Define a wrapper around json_object_foreach() for compatibility with older
 * versions of libjansson.
 */
#if JANSSON_VERSION_HEX < 0x020200
# error "nvidia-settings requires jansson version 2.2 or later.  Please update"
# error "your version of jansson, or set NV_USE_BUNDLED_LIBJANSSON=1 to build"
# error "with the version of jansson included with the nvidia-settings source"
# error "code."
#elif JANSSON_VERSION_HEX < 0x020300
# define NV_JSON_OBJECT_FOREACH(object, key, value) \
    for(key = json_object_iter_key(json_object_iter(object)); \
        key && (value = json_object_iter_value(json_object_iter_at(object, key))); \
        key = json_object_iter_key(json_object_iter_next(object, json_object_iter_at(object, key))))
#else
# define NV_JSON_OBJECT_FOREACH(object, key, value) json_object_foreach(object, key, value)
#endif

static char *slurp(FILE *fp)
{
    int eof = FALSE;
    char *text = strdup("");
    char *new_text;

    while (text && !eof) {
        char *line = fget_next_line(fp, &eof);

        if (line && *line != '\0' && *line != '\n') {
            new_text = nvstrcat(text, "\n", line, NULL);
            free(text);
            text = new_text;
        }
        free(line);
    }

    return text;
}

static void splice_string(char **s, size_t b, size_t e, const char *replace)
{
    char *tail = strdup(*s + e);
    *s = realloc(*s, b + strlen(replace) + strlen(tail) + 1);
    if (!*s) {
        return;
    }
    sprintf(*s + b, "%s%s", replace, tail);
    free(tail);
}

#define HEX_DIGITS "0123456789abcdefABCDEF"

char *nv_app_profile_file_syntax_to_json(const char *orig_s)
{
    char *s = strdup(orig_s);

    int quoted = FALSE;
    char *tok;
    size_t start, end, size;
    unsigned long long val;
    char *old_substr = NULL;
    char *endptr;
    char *new_substr = NULL;

    tok = s;
    while ((tok = strpbrk(tok, "\\\"#" HEX_DIGITS))) {
        switch (*tok) {
        case '\"':
            // Quotation mark
            quoted = !quoted;
            tok++;
            break;
        case '\\':
            // Escaped character
            tok++;
            if (*tok) {
                tok++;
            }
            break;
        case '#':
            // Comment
            if (!quoted) {
                char *end_tok = nvstrchrnul(tok, '\n');
                start = tok - s;
                end = end_tok - s;
                splice_string(&s, start, end, "");
                if (!s) {
                    goto fail;
                }
                tok = s + start;
            } else {
                tok++;
            }
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
            // Numeric value
            size = strspn(tok, "Xx." HEX_DIGITS);
            if ((tok[0] == '0') &&
                (tok[1] == 'x' || tok[1] == 'X' || isdigit(tok[1])) &&
                !quoted) {
                old_substr = nvstrndup(tok, size);
                if (!old_substr) {
                    goto fail;
                }
                errno = 0;
                val = strtoull(old_substr, &endptr, 0);
                if (errno || (endptr - old_substr != strlen(old_substr))) {
                    // Invalid conversion, skip this string
                    tok += size;
                    free(old_substr); old_substr = NULL;
                } else {
                    new_substr = nvasprintf("%llu", val);
                    if (!new_substr) {
                        goto fail;
                    } else {
                        start = tok - s;
                        end = tok - s + size;
                        splice_string(&s, start, end, new_substr);
                        free(new_substr); new_substr = NULL;
                        free(old_substr); old_substr = NULL;
                        tok = s + start;
                    }
                }
            } else {
                // Not hex or octal; let the JSON parser deal with it
                tok += size;
            }
            break;
        default:
            assert(!"Unhandled character");
            break;
        }
    }

    assert(!new_substr);
    assert(!old_substr);
    return s;

fail:
    free(old_substr);
    free(new_substr);
    free(s);
    return NULL;
}

static int open_and_stat(const char *filename, const char *perms, FILE **fp, struct stat *stat_buf)
{
    int ret;
    *fp = fopen(filename, perms);
    if (!*fp) {
        if (errno != ENOENT) {
            nv_error_msg("Could not open file %s (%s)", filename, strerror(errno));
        }
        return -1;
    }

    ret = fstat(fileno(*fp), stat_buf);
    if (ret == -1) {
        nv_error_msg("Could not stat file %s (%s)", filename, strerror(errno));
        fclose(*fp);
    }
    return ret;
}

static json_t *app_profile_config_insert_file_object(AppProfileConfig *config, json_t *new_file)
{
    json_t *json_filename, *json_new_filename;
    char *dirname;
    const char *filename, *new_filename;
    json_t *file;
    json_t *order, *file_order;
    size_t new_file_major, new_file_minor, file_order_major, file_order_minor;
    size_t i;
    size_t num_files;

    json_new_filename = json_object_get(new_file, "filename");

    assert(json_new_filename);
    new_filename = json_string_value(json_new_filename);

    assert(nv_app_profile_config_check_valid_source_file(config,
                                                         new_filename,
                                                         NULL));

    // Determine the correct location of the file in the search path
    dirname = NULL;

    new_file_major = -1;
    for (i = 0; i < config->search_path_count; i++) {
        if (!strcmp(new_filename, config->search_path[i])) {
            new_file_major = i;
            break;
        } else {
            if (!dirname) {
                dirname = nv_dirname(new_filename);
            }
            if (!strcmp(dirname, config->search_path[i])) {
                new_file_major = i;
                break;
            }
        }
    }
    free(dirname);

    new_file_minor = 0;
    num_files = json_array_size(config->parsed_files);

    for (i = 0; i < num_files; i++) {
        file = json_array_get(config->parsed_files, i);
        file_order = json_object_get(file, "order");
        file_order_major = json_integer_value(json_object_get(file_order, "major"));
        file_order_minor = json_integer_value(json_object_get(file_order, "minor"));
        json_filename = json_object_get(file, "filename");
        assert(json_filename);
        filename = json_string_value(json_filename);
        if (file_order_major < new_file_major) {
        } else if (file_order_major == new_file_major) {
            if (strcoll(filename, new_filename) > 0) {
                break;
            }
            new_file_minor++;
        } else {
            break;
        }
    }

    // Mark the order of the file
    order = json_object_get(new_file, "order");
    json_object_set_new(order, "major", json_integer(new_file_major));
    json_object_set_new(order, "minor", json_integer(new_file_minor));

    // Add the new file
    json_array_insert(config->parsed_files, i, new_file);

    // Bump up minor for files after this one with the same major
    num_files = json_array_size(config->parsed_files);

    for ( ; i < num_files; i++) {
        file = json_array_get(config->parsed_files, i);
        file_order = json_object_get(file, "order");
        file_order_major = json_integer_value(json_object_get(order, "major"));
        file_order_minor = json_integer_value(json_object_get(order, "minor"));
        if (file_order_major > new_file_major) {
            break;
        }
        json_object_set_new(file_order, "minor", json_integer(file_order_minor+1));
    }

    return new_file;
}


/*
 * Create a new empty file object and adds it to the configuration.
 */
static json_t *app_profile_config_new_file(AppProfileConfig *config,
                                           const char *filename)
{
    json_t *new_file = json_object();

    json_object_set_new(new_file, "filename", json_string(filename));
    json_object_set_new(new_file, "rules", json_array());
    json_object_set_new(new_file, "profiles", json_object());
    json_object_set_new(new_file, "dirty", json_false());
    json_object_set_new(new_file, "new", json_true());
    // order is set by app_profile_config_insert_file_object() below

    new_file = app_profile_config_insert_file_object(config, new_file);

    return new_file;
}

static char *rule_id_to_key_string(int id)
{
    char *key;
    key = nvasprintf("%d", id);
    return key;
}

/*
 * Constructs a profile name that is guaranteed to be unique to this
 * configuration. This is used to handle the case where there are multiple
 * profiles with the same name (an invalid configuration).
 */
static char *app_profile_config_unique_profile_name(AppProfileConfig *config,
                                                    const char *orig_name,
                                                    const char *filename,
                                                    int do_warn,
                                                    int *needs_dirty)
{
    json_t *json_gold_filename = json_object_get(config->profile_locations, orig_name);

    if (json_gold_filename) {
        int i = 0;
        char *new_name = NULL;
        do {
            free(new_name);
            new_name = nvasprintf("%s_duplicate_%d", orig_name, i++);
        } while (new_name && json_object_get(config->profile_locations, new_name));
        if (do_warn) {
            nv_error_msg("The profile \"%s\" in the file \"%s\" has the same name "
                         "as a profile defined in the file \"%s\", and will be renamed to \"%s\".",
                         orig_name, filename, json_string_value(json_gold_filename), new_name);
        }
        if (needs_dirty) {
            *needs_dirty = TRUE;
        }
        return new_name;
    } else {
        return strdup(orig_name);
    }
}


char *nv_app_profile_config_get_unused_profile_name(AppProfileConfig *config)
{
    char *temp_name, *unique_name;
    int salt = rand();

    temp_name = nvasprintf("profile_%x", salt);
    unique_name = app_profile_config_unique_profile_name(config,
                                                         temp_name,
                                                         NULL,
                                                         FALSE,
                                                         NULL);

    free(temp_name);
    return unique_name;
}

static json_t *json_settings_parse(json_t *old_settings, const char *filename)
{
    int uses_setting_objects;
    size_t i, size;
    json_t *old_setting;
    json_t *new_settings, *new_setting;
    json_t *json_key, *json_value;

    if (!json_is_array(old_settings)) {
        return NULL;
    }

    new_settings = json_array();

    uses_setting_objects = json_array_size(old_settings) &&
                           json_is_object(json_array_get(old_settings, 0));

    for (i = 0, size = json_array_size(old_settings); i < size; ) {
        old_setting = json_array_get(old_settings, i++);
        if (uses_setting_objects) {
            json_key = json_object_get(old_setting, "key");
            if (!json_key) {
                json_key = json_object_get(old_setting, "k");
            }
            json_value = json_object_get(old_setting, "value");
            if (!json_value) {
                json_value = json_object_get(old_setting, "v");
            }
        } else {
            if (i >= size) {
                nv_error_msg("App profile parse error in %s: Key/value array of odd length\n", filename);
                json_decref(new_settings);
                return NULL;
            }
            json_key = old_setting;
            json_value = json_array_get(old_settings, i++);
        }

        if (!json_is_string(json_key)) {
            nv_error_msg("App profile parse error in %s: Invalid key detected in settings array\n", filename);
            json_decref(new_settings);
            return NULL;
        }

        if (!json_is_integer(json_value) &&
            !json_is_real(json_value) &&
            !json_is_true(json_value) &&
            !json_is_false(json_value) &&
            !json_is_string(json_value)) {
            nv_error_msg("App profile parse error in %s: Invalid value detected in settings array\n", filename);
            json_decref(new_settings);
            return NULL;
        }
        new_setting = json_object();
        json_object_set(new_setting, "key", json_key);
        json_object_set(new_setting, "value", json_value);
        json_array_append_new(new_settings, new_setting);
    }

    return new_settings;
}

/*
 * Load app profile key documentation from file.
 */
json_t *nv_app_profile_key_documentation_load(const char *key_docs_file)
{
    json_t *key_docs = NULL;
    int ret, i;
    struct stat stat_buf;
    FILE *fp = NULL;

    char *orig_text = NULL;
    char *json_text = NULL;
    json_t *orig_file = NULL;
    json_t *orig_json_keys = NULL;
    json_error_t error;

    if (!key_docs_file) {
        goto done;
    }

    ret = open_and_stat(key_docs_file, "r", &fp, &stat_buf);
    if (ret < 0) {
        goto done;
    }

    orig_text = slurp(fp);

    if (!orig_text) {
        nv_error_msg("Could not read from file %s", key_docs_file);
        goto done;
    }

    // Convert the file contents to JSON
    json_text = nv_app_profile_file_syntax_to_json(orig_text);

    if (!json_text) {
        nv_error_msg("App profile parse error in %s: "
                     "text is not valid app profile key "
                     "documentation syntax", key_docs_file);
        goto done;
    }

    // Parse the resulting JSON
    orig_file = json_loads(json_text, 0, &error);

    if (!orig_file) {
        nv_error_msg("App profile parse error in %s: %s on %s, line %d\n",
                       key_docs_file, error.text, error.source, error.line);
        goto done;
    }

    // Process the array of key objects within the top level object
    orig_json_keys = json_object_get(orig_file, "registry_keys");

    if (orig_json_keys) {
        int size = json_array_size(orig_json_keys);

        key_docs = json_array();

        for (i = 0; i < size; i++) {
            json_t *json_name, *json_description, *json_type;
            json_t *json_key_object = json_array_get(orig_json_keys, i);

            if (!json_is_object(json_key_object)) {
                nv_error_msg("App profile parse error in %s: "
                             "Object expected in 'registry_keys' array "
                             "at position %d",
                             key_docs_file, i);
                continue;
            }

            json_name        = json_object_get(json_key_object, "key");
            json_description = json_object_get(json_key_object, "description");
            json_type        = json_object_get(json_key_object, "type");

            /*
             * Any invalid and non-string type for any fields per key will
             * cause the key's data to not be added.
             */
            if (json_is_string(json_name) &&
                json_is_string(json_description) &&
                json_is_string(json_type)) {

                json_t *new_json_key_object = json_object();

                json_object_set(new_json_key_object, "key", json_name);
                json_object_set(new_json_key_object, "description",
                                json_description);
                json_object_set(new_json_key_object, "type", json_type);

                json_array_append_new(key_docs, new_json_key_object);
            }
        }
    }


done:
    if (json_array_size(key_docs) == 0) {
        json_decref(key_docs);
        key_docs = NULL;
    }

    free(orig_text);
    free(json_text);
    json_decref(orig_file);

    if (fp) {
        fclose(fp);
    }

    return key_docs;
}

/*
 * Load app profile settings from an already-open file. This operation is
 * atomic: either all of the settings from the file are added to the
 * configuration, or none are.
 */
static void app_profile_config_load_file(AppProfileConfig *config,
                                         const char *filename,
                                         struct stat *stat_buf,
                                         FILE *fp)
{
    char *json_text = NULL;
    char *orig_text = NULL;
    size_t i, size;
    json_error_t error;
    json_t *orig_file = NULL;
    json_t *orig_json_profiles, *orig_json_rules;
    int next_free_rule_id = config->next_free_rule_id;
    int dirty = FALSE;
    json_t *new_file = NULL;
    json_t *new_json_profiles = NULL;
    json_t *new_json_rules = NULL;

    if (!S_ISREG(stat_buf->st_mode)) {
        // Silently ignore all but regular files
        goto done;
    }

    orig_text = slurp(fp);

    if (!orig_text) {
        nv_error_msg("Could not read from file %s", filename);
        goto done;
    }

    // Convert the file contents to JSON
    json_text = nv_app_profile_file_syntax_to_json(orig_text);

    if (!json_text) {
        nv_error_msg("App profile parse error in %s: text is not valid app profile configuration syntax", filename);
        goto done;
    }

    new_file = json_object();

    json_object_set_new(new_file, "dirty", json_false());
    json_object_set_new(new_file, "filename", json_string(filename));

    new_json_profiles = json_object();
    new_json_rules = json_array();

    // Parse the resulting JSON
    orig_file = json_loads(json_text, 0, &error);

    if (!orig_file) {
        nv_error_msg("App profile parse error in %s: %s on %s, line %d\n",
                       filename, error.text, error.source, error.line);
        goto done;
    }

    if (!json_is_object(orig_file)) {
        nv_error_msg("App profile parse error in %s: top-level config not an object!\n", filename);
        goto done;
    }

    orig_json_profiles = json_object_get(orig_file, "profiles");

    if (orig_json_profiles) {
        /*
         * Note: we store profiles internally as members of an object, but the
         * config file syntax uses an array to store profiles.
         */
        if (!json_is_array(orig_json_profiles)) {
            nv_error_msg("App profile parse error in %s: profiles value is not an array\n", filename);
            goto done;
        }

        size = json_array_size(orig_json_profiles);
        for (i = 0; i < size; i++) {
            const char *new_name;
            json_t *orig_json_profile, *orig_json_name, *orig_json_settings;
            json_t *new_json_profile, *new_json_settings;

            new_json_profile = json_object();

            orig_json_profile = json_array_get(orig_json_profiles, i);
            if (!json_is_object(orig_json_profile)) {
                goto done;
            }

            orig_json_name = json_object_get(orig_json_profile, "name");
            if (!json_is_string(orig_json_name)) {
                goto done;
            }

            orig_json_settings = json_object_get(orig_json_profile, "settings");
            new_json_settings = json_settings_parse(orig_json_settings, filename);
            if (!new_json_settings) {
                goto done;
            }

            new_name = app_profile_config_unique_profile_name(config,
                                                              json_string_value(orig_json_name),
                                                              filename,
                                                              TRUE,
                                                              &dirty);
            json_object_set_new(new_json_profile, "settings", new_json_settings);

            json_object_set_new(new_json_profiles, new_name, new_json_profile);
        }
    }

    orig_json_rules = json_object_get(orig_file, "rules");

    if (orig_json_rules) {
        if (!json_is_array(orig_json_rules)) {
            nv_error_msg("App profile parse error in %s: rules value is not an array\n", filename);
            goto done;
        }

        size = json_array_size(orig_json_rules);
        for (i = 0; i < size; i++) {
            int new_id;
            char *profile_name;
            json_t *orig_json_rule, *orig_json_pattern, *orig_json_profile;
            json_t *new_json_rule, *new_json_pattern;
            orig_json_rule = json_array_get(orig_json_rules, i);

            if (!json_is_object(orig_json_rule)) {
                goto done;
            }

            new_id = next_free_rule_id++;

            new_json_rule = json_object();
            new_json_pattern = json_object();

            orig_json_pattern = json_object_get(orig_json_rule, "pattern");
            if (json_is_object(orig_json_pattern)) {
                // pattern object
                json_t *orig_json_feature, *orig_json_matches;
                orig_json_feature = json_object_get(orig_json_pattern, "feature");
                if (!json_is_string(orig_json_feature)) {
                    json_decref(new_json_rule);
                    json_decref(new_json_pattern);
                    goto done;
                }
                orig_json_matches = json_object_get(orig_json_pattern, "matches");
                if (!json_is_string(orig_json_matches)) {
                    json_decref(new_json_rule);
                    json_decref(new_json_pattern);
                    goto done;
                }
                json_object_set(new_json_pattern, "feature", orig_json_feature);
                json_object_set(new_json_pattern, "matches", orig_json_matches);
            } else if (json_is_string(orig_json_pattern)) {
                // procname
                json_object_set_new(new_json_pattern, "feature", json_string("procname"));
                json_object_set(new_json_pattern, "matches", orig_json_pattern);
            } else {
                json_decref(new_json_rule);
                json_decref(new_json_pattern);
                goto done;
            }

            json_object_set_new(new_json_rule, "pattern", new_json_pattern);

            orig_json_profile = json_object_get(orig_json_rule, "profile");
            if (json_is_object(orig_json_profile) || json_is_array(orig_json_profile)) {
                // inline profile object
                json_t *new_json_profile;
                json_t *orig_json_settings, *orig_json_name;
                json_t *new_json_settings;

                if (json_is_object(orig_json_profile)) {
                    orig_json_settings = json_object_get(orig_json_profile, "settings");
                    orig_json_name = json_object_get(orig_json_profile, "name");
                } else {
                    // must be array
                    orig_json_settings = orig_json_profile;
                    orig_json_name = NULL;
                }

                if (json_is_string(orig_json_name)) {
                    profile_name = app_profile_config_unique_profile_name(config,
                                                                          json_string_value(orig_json_name),
                                                                          filename,
                                                                          TRUE,
                                                                          &dirty);
                } else if (!orig_json_name) {
                    char *profile_name_template;
                    profile_name_template = nvasprintf("inline_%d", new_id);
                    profile_name = app_profile_config_unique_profile_name(config,
                                                                          profile_name_template,
                                                                          filename,
                                                                          FALSE,
                                                                          &dirty);
                    free(profile_name_template);
                } else {
                    json_decref(new_json_rule);
                    goto done;
                }

                new_json_settings = json_settings_parse(orig_json_settings, filename); 

                if (!profile_name || !new_json_settings) {
                    free(profile_name);
                    json_decref(new_json_settings);
                    json_decref(new_json_rule);
                    goto done;
                }

                new_json_profile = json_object();
                json_object_set_new(new_json_profile, "settings", new_json_settings);

                json_object_set_new(new_json_profiles, profile_name, new_json_profile);
            } else if (json_is_string(orig_json_profile)) {
                // named profile
                profile_name = strdup(json_string_value(orig_json_profile));
            } else {
                json_decref(new_json_rule);
                goto done;
            }

            json_object_set_new(new_json_rule, "profile", json_string(profile_name));
            free(profile_name);

            json_object_set_new(new_json_rule, "id", json_integer(new_id));

            json_array_append_new(new_json_rules, new_json_rule);
        }
    }

    json_object_set(new_file, "profiles", new_json_profiles);
    json_object_set(new_file, "rules", new_json_rules);
    json_object_set_new(new_file, "dirty", dirty ? json_true() : json_false());
    json_object_set_new(new_file, "new", json_false());

    // Don't use the atime in the stat_buf; instead measure it here
    json_object_set_new(new_file, "atime", json_integer(time(NULL)));

    // If we didn't fail anywhere above, add the file to our configuration
    app_profile_config_insert_file_object(config, new_file);

    // Add the profiles in this file to the global profiles list
    {
        const char *key;
        json_t *value;
        NV_JSON_OBJECT_FOREACH(new_json_profiles, key, value) {
            json_object_set_new(config->profile_locations, key, json_string(filename));
        }
    }

    // Add the rules in this file to the global rules list
    size = json_array_size(new_json_rules);
    for (i = 0; i < size; i++) {
        char *key;
        json_t *new_rule;

        new_rule = json_array_get(new_json_rules, i);
        key = rule_id_to_key_string(json_integer_value(json_object_get(new_rule, "id")));
        json_object_set_new(config->rule_locations, key, json_string(filename));
        free(key);
    }
    config->next_free_rule_id = next_free_rule_id;

done:
    json_decref(orig_file);
    json_decref(new_file);
    json_decref(new_json_rules);
    json_decref(new_json_profiles);
    free(json_text);
    free(orig_text);
}

// Load app profile settings from a directory
static void app_profile_config_load_files_from_directory(AppProfileConfig *config,
                                                         const char *dirname)
{
    FILE *fp;
    struct stat stat_buf;
    struct dirent **namelist;
    int i, n, ret;

    n = scandir(dirname, &namelist, NULL, alphasort);

    if (n < 0) {
        nv_error_msg("Failed to open directory \"%s\"", dirname);
        return;
    }

    for (i = 0; i < n; i++) {
        char *d_name = namelist[i]->d_name;
        char *full_path;

        // Skip "." and ".."
        if ((d_name[0] == '.') &&
            ((d_name[1] == '\0') ||
             ((d_name[1] == '.') && (d_name[2] == '\0')))) {
            continue;
        }

        full_path = nvstrcat(dirname, "/", d_name, NULL);
        ret = open_and_stat(full_path, "r", &fp, &stat_buf);

        if (ret < 0) {
            free(full_path);
            free(namelist[i]);
            continue;
        }

        app_profile_config_load_file(config,
                                     full_path,
                                     &stat_buf,
                                     fp);
        fclose(fp);
        free(full_path);
        free(namelist[i]);
    }

    free(namelist);
}

static json_t *app_profile_config_load_global_options(const char *global_config_file)
{
    json_error_t error;
    json_t *options = json_object();
    int ret;
    FILE *fp;
    struct stat stat_buf;
    char *option_text;
    json_t *options_from_file;
    json_t *option;

    // By default, app profiles are enabled
    json_object_set_new(options, "enabled", json_true());

    if (!global_config_file) {
        return options;
    }

    ret = open_and_stat(global_config_file, "r", &fp, &stat_buf);
    if ((ret < 0) || !S_ISREG(stat_buf.st_mode)) {
        if (ret >= 0) {
            fclose(fp);
        }
        return options;
    }

    option_text = slurp(fp);
    fclose(fp);

    options_from_file = json_loads(option_text, 0, &error);
    free(option_text);

    if (!options_from_file) {
        nv_error_msg("App profile parse error in %s: %s on %s, line %d\n",
                     global_config_file, error.text, error.source, error.line);
        return options;
    }

    // Load the "enabled" option
    option = json_object_get(options_from_file, "enabled");
    if (option && (json_is_true(option) || json_is_false(option))) {
        json_object_set(options, "enabled", option);
    }

    json_decref(options_from_file);

    return options;
}

AppProfileConfig *nv_app_profile_config_load(const char *global_config_file,
                                             char **search_path,
                                             size_t search_path_count)
{
    size_t i;
    AppProfileConfig *config = malloc(sizeof(AppProfileConfig));

    if (!config) {
        return NULL;
    }

    // Initialize the config
    config->next_free_rule_id = 0;

    config->parsed_files = json_array();
    config->profile_locations = json_object();
    config->rule_locations = json_object();

    if (global_config_file) {
        config->global_config_file = nvstrdup(global_config_file);
    } else {
        config->global_config_file = NULL;
    }

    config->global_options = app_profile_config_load_global_options(global_config_file);

    config->search_path = malloc(sizeof(char *) * search_path_count);
    config->search_path_count = search_path_count;

    for (i = 0; i < search_path_count; i++) {
        config->search_path[i] = strdup(search_path[i]);
    }

    for (i = 0; i < search_path_count; i++) {
        int ret;
        struct stat stat_buf;
        const char *filename = search_path[i];
        FILE *fp;

        ret = open_and_stat(filename, "r", &fp, &stat_buf);
        if (ret < 0) {
            continue;
        }

        if (S_ISDIR(stat_buf.st_mode)) {
            // Parse files in the directory
            fclose(fp);
            app_profile_config_load_files_from_directory(config, filename);
        } else {
            // Load the individual file
            app_profile_config_load_file(config, filename, &stat_buf, fp);
            fclose(fp);
            continue;
        }
    }

    return config;
}

static int file_in_search_path(AppProfileConfig *config, const char *filename)
{
    size_t i;
    for (i = 0; i < config->search_path_count; i++) {
        if (!strcmp(filename, config->search_path[i])) {
            return TRUE;
        }
    }

    return FALSE;
}

// Print an error message and optionally capture the error string for later use
// Note: this assumes fmt is a string literal!
#define LOG_ERROR(error_str, fmt, ...) do {                   \
    if (error_str) {                                          \
        nv_append_sprintf(error_str, fmt "\n", __VA_ARGS__);  \
    }                                                         \
    nv_error_msg(fmt, __VA_ARGS__);                           \
} while (0)

/*
 * Create parent directories as needed and handle error messages
 */
static int nv_mkdirp(const char *dirname, char **error_str)
{
    char *tmp_error_str = NULL;
    int success;

    success = nv_mkdir_recursive(dirname, 0777, &tmp_error_str, NULL);
    if (tmp_error_str) {
        LOG_ERROR(error_str, "%s", tmp_error_str);
        free(tmp_error_str);
    }

    return success ? 0 : -1;
}

char *nv_app_profile_config_get_backup_filename(AppProfileConfig *config, const char *filename)
{
    char *basename = NULL;
    char *dirname = NULL;
    char *backup_name = NULL;

    if ((config->global_config_file &&
         !strcmp(config->global_config_file, filename)) ||
        file_in_search_path(config, filename)) {
        // Files in the top-level search path, and the global config file, can
        // be renamed from "$FILE" to "$FILE.backup" without affecting the
        // configuration
        backup_name = nvasprintf("%s.backup", filename);
    } else {
        // Files in a search path directory *cannot* be renamed from "$FILE" to
        // "$FILE.backup" without affecting the configuration due to the search
        // path rules. Instead, attempt to move them to a subdirectory called
        // ".backup".
        dirname = nv_dirname(filename);
        basename = nv_basename(filename);
        assert(file_in_search_path(config, dirname));
        backup_name = nvasprintf("%s/.backup/%s", dirname, basename);
    }

    free(dirname);
    free(basename);
    return backup_name;
}

static int app_profile_config_backup_file(AppProfileConfig *config,
                                          const char *filename,
                                          char **error_str)
{
    int ret;
    char *backup_name = nv_app_profile_config_get_backup_filename(config, filename);
    char *backup_dirname = nv_dirname(backup_name);

    ret = nv_mkdirp(backup_dirname, error_str);
    if (ret < 0) {
        LOG_ERROR(error_str, "Could not create backup directory \"%s\" (%s)",
                  backup_name, strerror(errno));
        goto done;
    }

    ret = rename(filename, backup_name);
    if (ret < 0) {
        if (errno == ENOENT) {
            // Clear the error; the file does not exist
            ret = 0;
        } else {
            LOG_ERROR(error_str, "Could not rename file \"%s\" to \"%s\" for backup (%s)",
                      filename, backup_name, strerror(errno));
        }
    }

    nv_info_msg("", "Backing up configuration file \"%s\" as \"%s\"\n", filename, backup_name);

done:
    free(backup_dirname);
    free(backup_name);
    return ret;
}


static int app_profile_config_save_updates_to_file(AppProfileConfig *config,
                                                   const char *filename,
                                                   const char *update_text,
                                                   int backup,
                                                   char **error_str)
{
    int file_is_new = FALSE;
    struct stat stat_buf;
    char *dirname = NULL;
    FILE *fp;
    int ret;

    ret = stat(filename, &stat_buf);

    if ((ret < 0) && (errno != ENOENT)) {
        LOG_ERROR(error_str, "Could not stat file \"%s\" (%s)",
                  filename, strerror(errno));
        goto done;
    } else if ((ret < 0) && (errno == ENOENT)) {
        file_is_new = TRUE;
        // Check if the prefix is in the search path
        dirname = nv_dirname(filename);

        if (file_in_search_path(config, dirname)) {
            // This file is in a directory in the search path
            ret = stat(dirname, &stat_buf);
            if ((ret < 0) && (errno != ENOENT)) {
                LOG_ERROR(error_str, "Could not stat file \"%s\" (%s)",
                          dirname, strerror(errno));
                goto done;
            } else if ((ret < 0) && errno == ENOENT) {
                // Attempt to create the directory in the search path
                ret = nv_mkdirp(dirname, error_str);
                if (ret < 0) {
                    goto done;
                }
            } else if (S_ISREG(stat_buf.st_mode)) {
                // If the search path entry is currently a regular file,
                // unlink it and create a directory instead
                if (backup) {
                    ret = app_profile_config_backup_file(config, dirname,
                                                         error_str);
                    if (ret < 0) {
                        goto done;
                    }
                }
                ret = unlink(dirname);
                if (ret < 0) {
                    LOG_ERROR(error_str,
                              "Could not remove the file \"%s\" (%s)",
                              dirname, strerror(errno));
                    goto done;
                }
                ret = nv_mkdirp(dirname, error_str);
                if (ret < 0) {
                    goto done;
                }
            }
        } else {
            // Attempt to create parent directories for this file
            ret = nv_mkdirp(dirname, error_str);
            if (ret < 0) {
                goto done;
            }
        }
    } else if (!S_ISREG(stat_buf.st_mode)) {
        // XXX: if this is a directory, we could recursively remove files here,
        // but that seems a little dangerous. Instead, complain and bail out
        // here.
        ret = -1;
        LOG_ERROR(error_str,
                  "Refusing to write to file \"%s\" "
                  "since it is not a regular file", filename);
        goto done;
    }

    if (!file_is_new && backup) {
        ret = app_profile_config_backup_file(config, filename,
                                             error_str);
        if (ret < 0) {
            goto done;
        }
    }
    ret = open_and_stat(filename, "w", &fp, &stat_buf);
    if (ret < 0) {
        LOG_ERROR(error_str, "Could not write to the file \"%s\" (%s)",
                  filename, strerror(errno));
        goto done;
    }
    nv_info_msg("", "Writing to configuration file \"%s\"\n", filename);
    fprintf(fp, "%s\n", update_text);
    fclose(fp);

done:
    free(dirname);
    return ret;
}

int nv_app_profile_config_save_updates(AppProfileConfig *config,
                                       json_t *updates,
                                       int backup,
                                       char **error_str)
{
    json_t *update;
    const char *filename;
    const char *update_text;
    size_t i, size;
    int ret = 0;
    int all_ret = 0;

    if (error_str) {
        *error_str = NULL;
    }

    for (i = 0, size = json_array_size(updates); i < size; i++) {
        update = json_array_get(updates, i);
        filename = json_string_value(json_object_get(update, "filename"));
        update_text = json_string_value(json_object_get(update, "text"));
        ret = app_profile_config_save_updates_to_file(config,
                                                      filename,
                                                      update_text,
                                                      backup,
                                                      error_str);
        if (ret < 0) {
            all_ret = -1;
        }
    }

    assert(all_ret <= 0);

    // This asserts an error string is set iff we are returning an error
    assert(!error_str ||
           (!(*error_str) && (all_ret == 0)) ||
           ((*error_str) && (all_ret < 0)));

    return all_ret;
}

AppProfileConfig *nv_app_profile_config_dup(AppProfileConfig *config)
{
    size_t i;
    AppProfileConfig *new_config;

    new_config = malloc(sizeof(AppProfileConfig));
    new_config->parsed_files = json_deep_copy(config->parsed_files);
    new_config->profile_locations = json_deep_copy(config->profile_locations);
    new_config->rule_locations = json_deep_copy(config->rule_locations);
    new_config->next_free_rule_id = config->next_free_rule_id;

    new_config->global_config_file =
        config->global_config_file ? strdup(config->global_config_file) : NULL;
    new_config->global_options = json_deep_copy(config->global_options);

    new_config->search_path = malloc(sizeof(char *) * config->search_path_count);
    new_config->search_path_count = config->search_path_count;

    for (i = 0; i < config->search_path_count; i++) {
        new_config->search_path[i] = strdup(config->search_path[i]);
    }

    return new_config;
}

void nv_app_profile_config_set_enabled(AppProfileConfig *config,
                                       int enabled)
{
    json_t *global_options = config->global_options;

    json_object_set_new(global_options, "enabled",
                        enabled ? json_true() : json_false());
}

int nv_app_profile_config_get_enabled(AppProfileConfig *config)
{
    json_t *global_options = config->global_options;
    json_t *enabled_json;

    enabled_json = json_object_get(global_options, "enabled");
    assert(enabled_json);

    return json_is_true(enabled_json);
}

void nv_app_profile_config_free(AppProfileConfig *config)
{
    size_t i;
    json_decref(config->global_options);
    json_decref(config->parsed_files);
    json_decref(config->profile_locations);
    json_decref(config->rule_locations);

    for (i = 0; i < config->search_path_count; i++) {
        free(config->search_path[i]);
    }

    free(config->search_path);
    free(config->global_config_file);

    free(config);
}

static json_t *app_profile_config_lookup_file(AppProfileConfig *config, const char *filename)
{
    size_t i, size;
    json_t *json_file, *json_filename;

    size = json_array_size(config->parsed_files);

    for (i = 0; i < size; i++) {
        json_file = json_array_get(config->parsed_files, i);
        json_filename = json_object_get(json_file, "filename");
        if (!strcmp(json_string_value(json_filename), filename)) {
            return json_file;
        }
    }

    return NULL;
}

static void app_profile_config_delete_file(AppProfileConfig *config, const char *filename)
{
    size_t i, size;
    json_t *json_file, *json_filename;

    size = json_array_size(config->parsed_files);

    for (i = 0; i < size; i++) {
        json_file = json_array_get(config->parsed_files, i);
        json_filename = json_object_get(json_file, "filename");
        if (!strcmp(json_string_value(json_filename), filename)) {
            json_array_remove(config->parsed_files, i);
            return;
        }
    }
}

static void app_profile_config_get_per_file_config(AppProfileConfig *config,
                                                   const char *filename,
                                                   json_t **file,
                                                   json_t **rules,
                                                   json_t **profiles)
{
    *file = app_profile_config_lookup_file(config, filename);

    if (!*file) {
        *rules = NULL;
        *profiles = NULL;
    } else {
        *rules = json_object_get(*file, "rules");
        *profiles = json_object_get(*file, "profiles");
    }
}

/*
 * Convert the internal representation of an application profile to a
 * representation suitable for writing to disk.
 */
static json_t *app_profile_config_profile_output(const char *profile_name, const json_t *orig_profile)
{
    json_t *new_profile = json_object();

    json_object_set_new(new_profile, "name", json_string(profile_name));
    json_object_set(new_profile, "settings", json_object_get(orig_profile, "settings"));

    return new_profile;
}

static json_t *app_profile_config_rule_output(const json_t *orig_rule)
{
    json_t *new_rule = json_object();

    json_object_set(new_rule, "pattern", json_object_get(orig_rule, "pattern"));
    json_object_set(new_rule, "profile", json_object_get(orig_rule, "profile"));

    return new_rule;
}

static char *config_to_cfg_file_syntax(json_t *old_rules, json_t *old_profiles)
{
    char *output = NULL;
    const char *profile_name;
    json_t *root, *rules_array, *profiles_array;
    json_t *old_rule, *old_profile;
    json_t *new_rule, *new_profile;
    size_t i, size;

    root = json_object();
    if (!root) {
        goto fail;
    }

    rules_array = json_array();
    if (!rules_array) {
        goto fail;
    }
    json_object_set_new(root, "rules", rules_array);

    profiles_array = json_array();
    if (!profiles_array) {
        goto fail;
    }
    json_object_set_new(root, "profiles", profiles_array);

    if (old_rules) {
        size = json_array_size(old_rules);
        for (i = 0; i < size; i++) {
            old_rule = json_array_get(old_rules, i);
            new_rule = app_profile_config_rule_output(old_rule);
            json_array_append_new(rules_array, new_rule);
        }
    }

    if (old_profiles) {
        NV_JSON_OBJECT_FOREACH(old_profiles, profile_name, old_profile) {
            new_profile = app_profile_config_profile_output(profile_name, old_profile);
            json_array_append_new(profiles_array, new_profile);
        }
    }

    output = json_dumps(root, JSON_ENSURE_ASCII | JSON_INDENT(4));

fail:
    json_decref(root);
    return output;
}

static void add_files_from_config(AppProfileConfig *config, json_t *all_files, json_t *changed_files)
{
    json_t *file, *filename;
    size_t i, size;
    for (i = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        file = json_array_get(config->parsed_files, i);
        filename = json_object_get(file, "filename");
        json_object_set_new(all_files, json_string_value(filename), json_true());
        if (json_is_true(json_object_get(file, "dirty"))) {
            json_object_set_new(changed_files, json_string_value(filename), json_true());
        }
    }
}

static json_t *app_profile_config_validate_global_options(AppProfileConfig *new_config,
                                                          AppProfileConfig *old_config)
{
    json_t *update = NULL;
    char *option_text;

    assert((!new_config->global_config_file && !old_config->global_config_file) ||
           (!strcmp(new_config->global_config_file, old_config->global_config_file)));

    if (new_config->global_config_file &&
        !json_equal(new_config->global_options, old_config->global_options)) {
        update = json_object();
        json_object_set_new(update, "filename", json_string(new_config->global_config_file));
        option_text = json_dumps(new_config->global_options, JSON_ENSURE_ASCII | JSON_INDENT(4));
        json_object_set_new(update, "text", json_string(option_text));
        free(option_text);
    }

    return update;
}

json_t *nv_app_profile_config_validate(AppProfileConfig *new_config,
                                       AppProfileConfig *old_config)
{
    json_t *all_files, *changed_files;
    json_t *new_file, *new_rules, *old_rules;
    json_t *old_file, *new_profiles, *old_profiles;
    json_t *updates, *update;
    const char *filename;
    char *update_text;
    json_t *unused;

    updates = json_array();

    // Determine if the global config file needs to be updated
    update = app_profile_config_validate_global_options(new_config, old_config);
    if (update) {
        json_array_append_new(updates, update);
    }

    // Build a set of files to examine: this is the union of files specified
    // by the old configuration and the new.
    all_files = json_object();
    changed_files = json_object();
    add_files_from_config(new_config, all_files, changed_files);
    add_files_from_config(old_config, all_files, changed_files);

    // For each file in the set, determine if it needs to be updated
    NV_JSON_OBJECT_FOREACH(all_files, filename, unused) {
        app_profile_config_get_per_file_config(new_config, filename, &new_file, &new_rules, &new_profiles);
        app_profile_config_get_per_file_config(old_config, filename, &old_file, &old_rules, &old_profiles);

        // Simply compare the JSON objects
        if (!json_equal(old_rules, new_rules) || !json_equal(old_profiles, new_profiles)) {
            json_object_set_new(changed_files, filename, json_true());
        }
    }

    // For each file that changed, generate an update record with the new JSON
    NV_JSON_OBJECT_FOREACH(changed_files, filename, unused) {
        update = json_object();

        json_object_set_new(update, "filename", json_string(filename));
        app_profile_config_get_per_file_config(new_config, filename, &new_file, &new_rules, &new_profiles);

        update_text = config_to_cfg_file_syntax(new_rules, new_profiles);
        json_object_set_new(update, "text", json_string(update_text));

        json_array_append_new(updates, update);
        free(update_text);
    }

    json_decref(all_files);
    json_decref(changed_files);

    return updates;
}

static int file_object_is_empty(const json_t *file)
{
    const json_t *rules;
    const json_t *profiles;

    rules = json_object_get(file, "rules");
    profiles = json_object_get(file, "profiles");

    return (!json_array_size(rules) && !json_object_size(profiles));
}

/*
 * Checks whether the given file is "empty" (contains no rules and profiles)
 * and "new" (created in the configuration and not loaded from disk), and
 * removes it from the configuration if both criteria are satisfied.
 */
static void app_profile_config_prune_empty_file(AppProfileConfig *config, const json_t *file)
{
    char *filename;
    if (json_is_true(json_object_get(file, "new")) && file_object_is_empty(file)) {
        filename = strdup(json_string_value(json_object_get(file, "filename")));
        app_profile_config_delete_file(config, filename);
        free(filename);
    }
}

int nv_app_profile_config_update_profile(AppProfileConfig *config,
                                         const char *filename,
                                         const char *profile_name,
                                         json_t *new_profile)
{
    json_t *file;
    json_t *old_file = NULL;
    json_t *file_profiles;
    const char *old_filename;

    old_filename = json_string_value(json_object_get(config->profile_locations, profile_name));

    if (old_filename) {
        // Existing profile
        old_file = app_profile_config_lookup_file(config, old_filename);
        assert(old_file);
    }

    // If there is an existing profile with a differing filename, delete it first
    if (old_filename && (strcmp(filename, old_filename) != 0)) {
        file = app_profile_config_lookup_file(config, old_filename);
        file_profiles = json_object_get(file, "profiles");
        if (file) {
            json_object_del(file_profiles, profile_name);
        }
    }

    file = app_profile_config_lookup_file(config, filename);
    if (!file) {
        file = app_profile_config_new_file(config, filename);
    }

    file_profiles = json_object_get(file, "profiles");
    json_object_set(file_profiles, profile_name, new_profile);
    json_object_set(config->profile_locations, profile_name, json_string(filename));

    if (old_file) {
        app_profile_config_prune_empty_file(config, old_file);
    }

    return !old_filename;
}

void nv_app_profile_config_delete_profile(AppProfileConfig *config,
                                          const char *profile_name)
{
    json_t *file = NULL;
    const char *filename = json_string_value(json_object_get(config->profile_locations, profile_name));

    if (filename) {
        file = app_profile_config_lookup_file(config, filename);
        if (file) {
            json_object_del(json_object_get(file, "profiles"), profile_name);
        }
    }

    json_object_del(config->profile_locations, profile_name);

    if (file) {
        app_profile_config_prune_empty_file(config, file);
    }
}

int nv_app_profile_config_create_rule(AppProfileConfig *config,
                                      const char *filename,
                                      json_t *new_rule)
{
    char *key;
    json_t *file, *file_rules;
    json_t *new_rule_copy;
    int new_id;

    file = app_profile_config_lookup_file(config, filename);
    if (!file) {
        file = app_profile_config_new_file(config, filename);
    }

    file_rules = json_object_get(file, "rules");

    // Add the rule to the head of the per-file list
    json_array_append(file_rules, new_rule);
    new_rule_copy = json_array_get(file_rules, json_array_size(file_rules) - 1);

    new_id = config->next_free_rule_id++;
    json_object_set_new(new_rule_copy, "id", json_integer(new_id));

    key = rule_id_to_key_string(new_id);
    json_object_set(config->rule_locations, key, json_string(filename));
    free(key);

    return new_id;
}

static int lookup_rule_index_in_array(json_t *rules, int id)
{
    json_t *rule, *rule_id;
    size_t i, size;
    for (i = 0, size = json_array_size(rules); i < size; i++) {
        rule = json_array_get(rules, i);
        rule_id = json_object_get(rule, "id");
        if (json_integer_value(rule_id) == id) {
            return i;
        }
    }

    return -1;
}

int nv_app_profile_config_update_rule(AppProfileConfig *config,
                                      const char *filename,
                                      int id,
                                      json_t *new_rule)
{
    json_t *old_file, *new_file;
    json_t *old_file_rules, *new_file_rules;
    json_t *new_rule_copy;
    const char *old_filename;
    char *key;
    int idx;
    int rule_moved;

    key = rule_id_to_key_string(id);
    old_filename = json_string_value(json_object_get(config->rule_locations, key));
    assert(old_filename);

    old_file = app_profile_config_lookup_file(config, old_filename);
    assert(old_file);

    old_file_rules = json_object_get(old_file, "rules");

    if (filename && (strcmp(filename, old_filename) != 0)) {
        // If the rule has a new file, delete the rule and re-add it
        new_file = app_profile_config_lookup_file(config, filename);
        rule_moved = TRUE;
        if (!new_file) {
            new_file = app_profile_config_new_file(config, filename);
        }

        new_file_rules = json_object_get(new_file, "rules");

        idx = lookup_rule_index_in_array(old_file_rules, id);
        if (idx != -1) {
            json_array_remove(old_file_rules, idx);
        }
        json_array_insert(new_file_rules, 0, new_rule);
        new_rule_copy = json_array_get(new_file_rules, 0);
        json_object_set_new(new_rule_copy, "id", json_integer(id));

        json_object_set_new(config->rule_locations, key, json_string(filename));
    } else {
        // Otherwise, just edit the existing rule
        rule_moved = FALSE;
        idx = lookup_rule_index_in_array(old_file_rules, id);
        if (idx != -1) {
            json_array_set(old_file_rules, idx, new_rule);
            new_rule_copy = json_array_get(old_file_rules, idx);
            json_object_set_new(new_rule_copy, "id", json_integer(id));
        }
    }

    free(key);

    app_profile_config_prune_empty_file(config, old_file);

    return rule_moved;
}


void nv_app_profile_config_delete_rule(AppProfileConfig *config, int id)
{
    json_t *file, *file_rules;
    const char *filename;
    char *key;
    int idx;

    key = rule_id_to_key_string(id);

    filename = json_string_value(json_object_get(config->rule_locations, key));
    assert(filename);

    file = app_profile_config_lookup_file(config, filename);
    assert(file);

    file_rules = json_object_get(file, "rules");

    idx = lookup_rule_index_in_array(file_rules, id);
    if (idx != -1) {
        json_array_remove(file_rules, idx);
    }

    json_object_del(config->rule_locations, key);
    free(key);
}

size_t nv_app_profile_config_count_rules(AppProfileConfig *config)
{
    return json_object_size(config->rule_locations);
}

static size_t app_profile_config_count_rules_before(AppProfileConfig *config, const char *filename)
{
    size_t i, size;
    size_t num_rules = 0;
    json_t *cur_file, *cur_filename;

    for (i = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        cur_file = json_array_get(config->parsed_files, i);
        cur_filename = json_object_get(cur_file, "filename");
        if (!strcmp(filename, json_string_value(cur_filename))) {
            break;
        }
        num_rules += json_array_size(json_object_get(cur_file, "rules"));
    }

    return num_rules;
}

static void app_profile_config_insert_rule(AppProfileConfig *config,
                                           json_t *rule,
                                           size_t new_pri,
                                           const char *old_filename)
{
    size_t i, j, size;
    size_t num_rules = 0;
    char *key;
    const char *filename;
    json_t *file, *file_rules;
    json_t *target[2];
    size_t rules_before_target[2];

    for (i = 0, j = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        file = json_array_get(config->parsed_files, i);
        file_rules = json_object_get(file, "rules");
        if ((num_rules <= new_pri) &&
            (num_rules + json_array_size(file_rules) >= new_pri)) {
            // Potential target file for this rule
            rules_before_target[j] = num_rules;
            target[j++] = file;
            if (j >= 2) {
                break;
            }
        }
        num_rules += json_array_size(file_rules);
    }

    assert((j > 0) && (j <= 2));

    // If possible, we prefer to keep the rule in the same file as before
    for (i = 0; i < j; i++) {
        filename = json_string_value(json_object_get(target[i], "filename"));
        if (!strcmp(filename, old_filename)) {
            break;
        }
    }
    i = (i == j) ? 0 : i;

    file_rules = json_object_get(target[i], "rules");
    json_array_insert_new(file_rules, new_pri - rules_before_target[i], rule);
    // Update the hashtable to point to the new file
    key = rule_id_to_key_string(json_integer_value(json_object_get(rule, "id")));
    filename = json_string_value(json_object_get(target[i], "filename"));
    json_object_set_new(config->rule_locations, key, json_string(filename));
    free(key);
}

size_t nv_app_profile_config_get_rule_priority(AppProfileConfig *config,
                                               int id)
{
    json_t *file, *file_rules;
    const char *filename;
    int idx;
    char *key;

    key = rule_id_to_key_string(id);

    filename = json_string_value(json_object_get(config->rule_locations, key));
    assert(filename);

    file = app_profile_config_lookup_file(config, filename);
    assert(file);

    file_rules = json_object_get(file, "rules");

    idx = lookup_rule_index_in_array(file_rules, id);

    free(key);

    return app_profile_config_count_rules_before(config, filename) + idx;
}

static void app_profile_config_set_abs_rule_priority_internal(AppProfileConfig *config,
                                                              int id,
                                                              size_t new_pri,
                                                              size_t current_pri,
                                                              size_t lowest_pri)
{
    json_t *rule, *rule_copy;
    json_t *file, *file_rules;
    const char *filename;
    int idx;
    char *key;

    if (new_pri == current_pri) {
        return;
    } else if (new_pri >= lowest_pri) {
        new_pri = lowest_pri - 1;
    }

    key = rule_id_to_key_string(id);

    filename = json_string_value(json_object_get(config->rule_locations, key));
    assert(filename);

    file = app_profile_config_lookup_file(config, filename);
    assert(file);

    file_rules = json_object_get(file, "rules");
    idx = lookup_rule_index_in_array(file_rules, id);
    assert(idx >= 0);
    rule = json_array_get(file_rules, idx);

    rule_copy = json_deep_copy(rule);
    json_array_remove(file_rules, idx);

    app_profile_config_insert_rule(config, rule_copy, new_pri, filename);

    app_profile_config_prune_empty_file(config, file);

    free(key);
}

void nv_app_profile_config_set_abs_rule_priority(AppProfileConfig *config,
                                                 int id, size_t new_pri)
{
    size_t current_pri = nv_app_profile_config_get_rule_priority(config, id);
    size_t lowest_pri = nv_app_profile_config_count_rules(config);
    app_profile_config_set_abs_rule_priority_internal(config, id, new_pri, current_pri, lowest_pri);
}

void nv_app_profile_config_change_rule_priority(AppProfileConfig *config,
                                                int id,
                                                int delta)
{
    size_t lowest_pri = nv_app_profile_config_count_rules(config);
    size_t current_pri = nv_app_profile_config_get_rule_priority(config, id);
    size_t new_pri;
    if ((delta < 0) && (((size_t)-delta) > current_pri)) {
        new_pri = 0;
    } else {
        new_pri = current_pri + delta;
    }
    app_profile_config_set_abs_rule_priority_internal(config, id, new_pri, current_pri, lowest_pri);
}

const json_t *nv_app_profile_config_get_profile(AppProfileConfig *config,
                                                const char *profile_name)
{
    json_t *file, *file_profiles;
    json_t *filename = json_object_get(config->profile_locations, profile_name);

    if (!filename) {
        return NULL;
    }

    file = app_profile_config_lookup_file(config, json_string_value(filename));
    file_profiles = json_object_get(file, "profiles");

    return json_object_get(file_profiles, profile_name);
}


const json_t *nv_app_profile_config_get_rule(AppProfileConfig *config,
                                             int id)
{
    char *key = rule_id_to_key_string(id);
    json_t *file, *rule, *filename;
    json_t *file_rules;
    int idx;

    filename = json_object_get(config->rule_locations, key);

    if (!filename) {
        free(key);
        return NULL;
    }

    file = app_profile_config_lookup_file(config, json_string_value(filename));
    file_rules = json_object_get(file, "rules");

    idx = lookup_rule_index_in_array(file_rules, id);
    if (idx != -1) {
        rule = json_array_get(file_rules, idx);
    } else {
        assert(0);
        rule = NULL;
    }

    free(key);
    return rule;
}

struct AppProfileConfigProfileIterRec {
    AppProfileConfig *config;
    size_t file_idx;
    json_t *profiles;
    void *profile_iter;
};

AppProfileConfigProfileIter *nv_app_profile_config_profile_iter(AppProfileConfig *config)
{
    AppProfileConfigProfileIter *iter = malloc(sizeof(AppProfileConfigProfileIter));

    iter->config = config;
    iter->file_idx = 0;
    iter->profile_iter = NULL;

    return nv_app_profile_config_profile_iter_next(iter);
}

AppProfileConfigProfileIter *nv_app_profile_config_profile_iter_next(AppProfileConfigProfileIter *iter)
{
    AppProfileConfig *config = iter->config;
    json_t *file;
    int advance = TRUE;
    size_t size;

    size = json_array_size(config->parsed_files);
    while ((iter->file_idx < size) &&
           !iter->profile_iter) {
        file = json_array_get(config->parsed_files, iter->file_idx);
        iter->profiles = json_object_get(file, "profiles");
        iter->profile_iter = json_object_iter(iter->profiles);
        iter->file_idx++;
        advance = FALSE;
    }

    if (!iter->profile_iter) {
        free(iter);
        return NULL;
    }

    if (advance) {
        iter->profile_iter = json_object_iter_next(iter->profiles, iter->profile_iter);
    }

    while ((iter->file_idx < size) &&
           !iter->profile_iter) {
        file = json_array_get(config->parsed_files, iter->file_idx);
        iter->profiles = json_object_get(file, "profiles");
        iter->profile_iter = json_object_iter(iter->profiles);
        iter->file_idx++;
    }

    if (!iter->profile_iter) {
        free(iter);
        return NULL;
    }

    return iter;
}


struct AppProfileConfigRuleIterRec {
    AppProfileConfig *config;
    size_t file_idx;
    int rule_idx;
    json_t *rules;
};

AppProfileConfigRuleIter *nv_app_profile_config_rule_iter(AppProfileConfig *config)
{
    AppProfileConfigRuleIter *iter = malloc(sizeof(AppProfileConfigRuleIter));

    iter->file_idx = 0;
    iter->rule_idx = -1;
    iter->config = config;

    return nv_app_profile_config_rule_iter_next(iter);
}

AppProfileConfigRuleIter *nv_app_profile_config_rule_iter_next(AppProfileConfigRuleIter *iter)
{
    AppProfileConfig *config = iter->config;
    json_t *file;
    int advance = TRUE;
    size_t size;

    size = json_array_size(config->parsed_files);
    while ((iter->file_idx < size) &&
           (iter->rule_idx == -1)) {
        file = json_array_get(config->parsed_files, iter->file_idx);
        iter->rules = json_object_get(file, "rules");
        if (json_array_size(iter->rules)) {
            iter->rule_idx = 0;
        }
        iter->file_idx++;
        advance = FALSE;
    }

    if (iter->rule_idx == -1) {
        free(iter);
        return NULL;
    }

    if (advance) {
        iter->rule_idx++;
        if (iter->rule_idx >= json_array_size(iter->rules)) {
            iter->rule_idx = -1;
        }
    }

    while ((iter->file_idx < size) &&
           (iter->rule_idx == -1)) {
        file = json_array_get(config->parsed_files, iter->file_idx);
        iter->rules = json_object_get(file, "rules");
        if (json_array_size(iter->rules)) {
            iter->rule_idx = 0;
        }
        iter->file_idx++;
    }

    if (iter->rule_idx == -1) {
        free(iter);
        return NULL;
    }

    return iter;
}

const char *nv_app_profile_config_profile_iter_name(AppProfileConfigProfileIter *iter)
{
    return json_object_iter_key(iter->profile_iter);
}

json_t *nv_app_profile_config_profile_iter_val(AppProfileConfigProfileIter *iter)
{
    return json_object_iter_value(iter->profile_iter);
}

size_t nv_app_profile_config_rule_iter_pri(AppProfileConfigRuleIter *iter)
{
    json_t *rule = nv_app_profile_config_rule_iter_val(iter);
    return nv_app_profile_config_get_rule_priority(iter->config,
                json_integer_value(json_object_get(rule, "id")));
}

json_t *nv_app_profile_config_rule_iter_val(AppProfileConfigRuleIter *iter)
{
    return json_array_get(iter->rules, iter->rule_idx);
}

const char *nv_app_profile_config_profile_iter_filename(AppProfileConfigProfileIter *iter)
{
    json_t *file = json_array_get(iter->config->parsed_files, iter->file_idx - 1);
    return json_string_value(json_object_get(file, "filename"));
}

const char *nv_app_profile_config_rule_iter_filename(AppProfileConfigRuleIter *iter)
{
    json_t *file = json_array_get(iter->config->parsed_files, iter->file_idx - 1);
    return json_string_value(json_object_get(file, "filename"));
}

const char *nv_app_profile_config_get_rule_filename(AppProfileConfig *config,
                                                    int id)
{
    const char *filename;
    char *key;

    key = rule_id_to_key_string(id);
    filename = json_string_value(json_object_get(config->rule_locations, key));
    free(key);

    return filename;
}

const char *nv_app_profile_config_get_profile_filename(AppProfileConfig *config,
                                                       const char *profile_name)
{
    return json_string_value(json_object_get(config->profile_locations, profile_name));
}

static char *get_search_path_string(AppProfileConfig *config)
{
    size_t i;
    char *new_s;
    char *s = strdup("");
    for (i = 0; i < config->search_path_count; i++) {
        new_s = nvasprintf("%s\t\"%s\"\n",
                           s, config->search_path[i]);
        free(s);
        s = new_s;
    }

    return s;
}

static int app_profile_config_check_is_prefix(const char *filename1, const char *filename2)
{
    char *dirname1, *dirname2;
    int prefix_state;
    dirname1 = nv_dirname(filename1);
    dirname2 = nv_dirname(filename2);

    if (!strcmp(dirname1, filename2)) {
        prefix_state = 1;
    } else if (!strcmp(filename1, dirname2)) {
        prefix_state = -1;
    } else {
        prefix_state = 0;
    }

    free(dirname1);
    free(dirname2);

    return prefix_state;
}

int nv_app_profile_config_check_valid_source_file(AppProfileConfig *config,
                                                  const char *filename,
                                                  char **reason)
{
    const json_t *parsed_file;
    size_t i, size;
    const char *cur_filename;
    char *dirname;
    char *search_path_string;
    int prefix_state;

    // Check if the source file can be found in the search path
    dirname = NULL;
    for (i = 0; i < config->search_path_count; i++) {
        if (!strcmp(filename, config->search_path[i])) {
            break;
        } else {
            if (!dirname) {
                dirname = nv_dirname(filename);
            }
            if (!strcmp(dirname, config->search_path[i])) {
                break;
            }
        }
    }
    free(dirname);

    if (i == config->search_path_count) {
        search_path_string = get_search_path_string(config);
        if (reason) {
            *reason = nvasprintf("the filename is not valid in the search path:\n\n%s\n",
                                 search_path_string);
        }
        free(search_path_string);
        return FALSE;
    }

    // Check if the source file is a prefix of some other file in the configuration,
    // or vice versa
    for (i = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        parsed_file = json_array_get(config->parsed_files, i);
        cur_filename = json_string_value(json_object_get(parsed_file, "filename"));

        prefix_state = app_profile_config_check_is_prefix(filename, cur_filename);

        if (prefix_state) {
            if (prefix_state > 0) {
                if (reason) {
                    *reason = nvasprintf("the filename is a prefix of the existing file \"%s\".",
                                         cur_filename);
                }
            } else if (reason) {
                *reason = nvasprintf("the filename would be placed in the directory \"%s\", "
                                     "but that is already a regular file in the configuration.",
                                     cur_filename);
            }
            return FALSE;
        }
    }

    return TRUE;
}

int nv_app_profile_config_check_backing_files(AppProfileConfig *config)
{
    json_t *file;
    size_t i, size;
    const char *filename;
    FILE *fp;
    time_t saved_atime;
    struct stat stat_buf;
    int ret;
    int changed = FALSE;
    for (i = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        file = json_array_get(config->parsed_files, i);
        if (json_is_false(json_object_get(file, "new"))) {
            // Stat the file and compare our saved atime to the file's mtime
            filename = json_string_value(json_object_get(file, "filename"));
            ret = open_and_stat(filename, "r", &fp, &stat_buf);
            if (ret >= 0) {
                fclose(fp);
                saved_atime = (time_t)json_integer_value(json_object_get(file, "atime"));
                if (stat_buf.st_mtime > saved_atime) {
                    json_object_set_new(file, "dirty", json_true());
                    changed = TRUE;
                }
            } else {
                // I/O errors: assume something changed
                json_object_set_new(file, "dirty", json_true());
                changed = TRUE;
            }
        }
    }

    return changed;
}

/*
 * Filenames in the search path ending in "*.d" are directories by convention,
 * and should not be listed as valid default filenames.
 */
static inline int check_has_directory_suffix(const char *filename)
{
    size_t len = strlen(filename);

    return (len >= 2) &&
           (filename[len-2] == '.') &&
           (filename[len-1] == 'd');
}

json_t *nv_app_profile_config_get_source_filenames(AppProfileConfig *config)
{
    size_t i, j, size;
    const char *filename;
    json_t *filenames;
    json_t *file;
    int do_add_search_path_item;

    filenames = json_array();
    for (i = 0, size = json_array_size(config->parsed_files); i < size; i++) {
        file = json_array_get(config->parsed_files, i);
        json_array_append(filenames, json_object_get(file, "filename"));
    }

    for (i = 0; i < config->search_path_count; i++) {
        do_add_search_path_item =
            !check_has_directory_suffix(config->search_path[i]);
        for (j = 0; (j < size) && do_add_search_path_item; j++) {
            file = json_array_get(config->parsed_files, j);
            filename = json_string_value(json_object_get(file, "filename"));
            if (!strcmp(config->search_path[i], filename) ||
                app_profile_config_check_is_prefix(config->search_path[i],
                                                   filename)) {
                do_add_search_path_item = FALSE;
            }
        }
        if (do_add_search_path_item) {
            json_array_append_new(filenames,
                                  json_string(config->search_path[i]));
        }
    }

    return filenames;
}

int nv_app_profile_config_profile_name_change_fixup(AppProfileConfig *config,
                                                     const char *orig_name,
                                                     const char *new_name)
{
    int fixed_up = FALSE;
    size_t i, j;
    size_t num_files, num_rules;
    json_t *file, *rules, *rule, *rule_profile;
    const char *rule_profile_str;

    for (i = 0, num_files = json_array_size(config->parsed_files); i < num_files; i++) {
        file = json_array_get(config->parsed_files, i);
        rules = json_object_get(file, "rules");
        for (j = 0, num_rules = json_array_size(rules); j < num_rules; j++) {
            rule = json_array_get(rules, j);
            rule_profile = json_object_get(rule, "profile");
            assert(json_is_string(rule_profile));
            rule_profile_str = json_string_value(rule_profile);
            if (!strcmp(rule_profile_str, orig_name)) {
                json_object_set_new(rule, "profile", json_string(new_name));
                fixed_up = TRUE;
            }
        }
    }

    return fixed_up;
}
