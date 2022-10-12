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
 * config-file.c - this source file contains functions for processing
 * the NVIDIA Settings control panel configuration file.
 *
 * The configuration file is simply a newline-separated list of
 * attribute strings, where the syntax of an attribute string is
 * described in the comments of parse.h
 *
 * The pound sign ('#') signifies the beginning of a comment; comments
 * continue until a newline.
 */


#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>

#include "NvCtrlAttributes.h"

#include "config-file.h"
#include "query-assign.h"
#include "parse.h"
#include "msg.h"


typedef struct {
    ParsedAttribute a;
    int line;
    CtrlSystem *system;
} ParsedAttributeWrapper;


static ParsedAttributeWrapper *parse_config_file(char *buf,
                                                 const char *file,
                                                 const int length,
                                                 ConfigProperties *);

static int process_config_file_attributes(const Options *op,
                                          const char *file,
                                          ParsedAttributeWrapper *w,
                                          const char *display_name,
                                          CtrlSystemList *system_list);

static void save_gui_parsed_attributes(ParsedAttributeWrapper *w,
                                       ParsedAttribute *p);

static float get_color_value(int attr,
                             float c[3], float b[3], float g[3]);

static int parse_config_property(const char *file, const char *line,
                                 ConfigProperties *conf);

static void write_config_properties(FILE *stream, const ConfigProperties *conf,
                                    char *locale);

static char *create_display_device_target_string(CtrlTarget *t,
                                                 const ConfigProperties *conf);

/*
 * set_dynamic_verbosity() - Sets the __dynamic_verbosity variable which
 * allows temporary toggling of the verbosity level to hide some output
 * messages when verbosity has not been explicitly set by the user.
 */

static int __dynamic_verbosity = NV_TRUE;

void set_dynamic_verbosity(int dynamic)
{
    __dynamic_verbosity = dynamic;
}

/*
 * nv_read_config_file() - read the specified config file, building a
 * list of attributes to send.  Once all attributes are read, send
 * them to the X server.
 *
 * mmap(2) the file into memory for easier manipulation.
 *
 * If an error occurs while parsing the configuration file, an error
 * message is printed to stderr, NV_FALSE is returned, and nothing is
 * sent to the X server.
 *
 * NOTE: The conf->locale should have already been setup by calling
 *       init_config_properties() prior to calling this function.
 *
 * XXX should we do any sort of versioning to handle compatibility
 * problems in the future?
 */

int nv_read_config_file(const Options *op, const char *file,
                        const char *display_name,
                        ParsedAttribute *p, ConfigProperties *conf,
                        CtrlSystemList *systems)
{
    int fd = -1;
    int ret = NV_FALSE;
    int length;
    struct stat stat_buf;
    char *buf;
    char *locale;
    ParsedAttributeWrapper *w = NULL;

    if (!file) {
        /*
         * file is NULL, likely because tilde_expansion() failed and
         * returned NULL; silently fail
         */
        goto done;
    }

    /* open the file */

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        /*
         * It's OK if the file doesn't exist... but should we print a
         * warning?
         */
        goto done;
    }

    /* get the size of the file */

    if (fstat(fd, &stat_buf) == -1) {
        nv_error_msg("Unable to determine size of file '%s' (%s).",
                     file, strerror(errno));
        goto done;
    }

    length = stat_buf.st_size;

    if (length == 0) {
        nv_warning_msg("File '%s' has zero size; not reading.", file);
        ret = NV_TRUE;
        goto done;
    }

    /* map the file into memory */

    buf = mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);
    if (buf == (void *) -1) {
        nv_error_msg("Unable to mmap file '%s' for reading (%s).",
                     file, strerror(errno));
        goto done;
    }

    /*
     * save the current locale, parse the actual text in the file
     * and restore the saved locale (could be changed).
     */

    locale = strdup(conf->locale);

    w = parse_config_file(buf, file, length, conf);

    setlocale(LC_NUMERIC, locale);
    free(locale);

    /* unmap and close the file */

    if (munmap(buf, length) == -1) {
        nv_error_msg("Unable to unmap file '%s' after reading (%s).",
                     file, strerror(errno));
        goto done;
    }

    if (!w) {
        goto done;
    }

    /* process the parsed attributes */

    ret = process_config_file_attributes(op, file, w, display_name, systems);

    /*
     * add any relevant parsed attributes back to the list to be
     * passed to the gui
     */

    save_gui_parsed_attributes(w, p);

 done:
    free(w);
    close(fd);

    return ret;

} /* nv_read_config_file() */



/*
 * nv_write_config_file() - write a configuration file to the
 * specified filename.
 *
 * XXX how should this be handled?  Currently, we just query all
 * writable attributes, writing their current value to file.
 *
 * XXX should query first, and only once we know we can't fail, then
 * write the file (to avoid deleting the existing file).
 */

int nv_write_config_file(const char *filename, const CtrlSystem *system,
                         const ParsedAttribute *p, const ConfigProperties *conf)
{
    int ret, entry, val, randr_gamma_available;
    FILE *stream;
    time_t now;
    ReturnStatus status;
    CtrlAttributePerms perms;
    CtrlTargetNode *node;
    CtrlTarget *t;
    char *prefix, scratch[4];
    char *locale = "C";

    if (!filename) {
        nv_error_msg("Unable to open configuration file for writing.");
        return NV_FALSE;
    }

    stream = fopen(filename, "w");
    if (!stream) {
        nv_error_msg("Unable to open file '%s' for writing.", filename);
        return NV_FALSE;
    }
    
    /* write header */
    
    now = time(NULL);
    
    fprintf(stream, "#\n");
    fprintf(stream, "# %s\n", filename);
    fprintf(stream, "#\n");
    fprintf(stream, "# Configuration file for nvidia-settings - the NVIDIA "
            "Settings utility\n");

    /* NOTE: ctime(3) generates a new line */
    
    fprintf(stream, "# Generated on %s", ctime(&now));
    fprintf(stream, "#\n");
    
    /*
     * set the locale to "C" before writing the configuration file to
     * reduce the risk of locale related parsing problems.  Restore
     * the original locale before exiting this function.
     */

    if (setlocale(LC_NUMERIC, "C") == NULL) {
        nv_warning_msg("Error writing configuration file '%s': could "
                       "not set the locale 'C'.", filename);
        locale = conf->locale;
    }

    /* write the values in ConfigProperties */

    write_config_properties(stream, conf, locale);

    /* for each screen, query each attribute in the table */

    fprintf(stream, "\n");
    fprintf(stream, "# Attributes:\n");
    fprintf(stream, "\n");

    /*
     * Note: we only save writable attributes addressable by X screen here
     * followed by attributes for display target types.
     */

    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        t = node->t;

        /* skip it if we don't have a handle for this screen */

        if (!t->h) continue;

        /*
         * construct the prefix that will be printed in the config
         * file in front of each attribute on this screen; this will
         * either be "[screen]" or "[displayname]".
         */

        if (conf->booleans &
            CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE) {
            prefix = t->name;
        } else {
            snprintf(scratch, 4, "%d", NvCtrlGetTargetId(t));
            prefix = scratch;
        }

        /* loop over all the entries in the table */

        for (entry = 0; entry < attributeTableLen; entry++) {
            const AttributeTableEntry *a = &attributeTable[entry];

            /*
             * skip all attributes that are not supposed to be written
             * to the config file
             */

            if (a->flags.no_config_write) {
                continue;
            }

            /*
             * special case the color attributes because we want to
             * print floats
             */

            if (a->type == CTRL_ATTRIBUTE_TYPE_COLOR) {
                float c[3], b[3], g[3];

                /*
                 * if we are using RandR gamma, skip saving the color info
                 */

                status = NvCtrlGetAttribute(t,
                                            NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE,
                                            &val);
                if (status == NvCtrlSuccess && val) continue;

                status = NvCtrlGetColorAttributes(t, c, b, g);
                if (status != NvCtrlSuccess) continue;
                fprintf(stream, "%s%c%s=%f\n",
                        prefix, DISPLAY_NAME_SEPARATOR, a->name,
                        get_color_value(a->attr, c, b, g));
                continue;
            }

            /* Only write out integer attributes, string attributes
             * aren't written here.
             */
            if (a->type != CTRL_ATTRIBUTE_TYPE_INTEGER) {
                continue;
            }

            /*
             * Ignore display attributes (they are written later on) and only
             * write attributes that can be written for an X screen target
             */

            status = NvCtrlGetAttributePerms(t, a->type, a->attr, &perms);
            if (status != NvCtrlSuccess || !(perms.write) ||
                !(perms.valid_targets & CTRL_TARGET_PERM_BIT(X_SCREEN_TARGET)) ||
                (perms.valid_targets & CTRL_TARGET_PERM_BIT(DISPLAY_TARGET))) {
                continue;
            }

            status = NvCtrlGetAttribute(t, a->attr, &val);
            if (status != NvCtrlSuccess) {
                continue;
            }

            if (a->f.int_flags.is_display_id) {
                const char *name = NvCtrlGetDisplayConfigName(system, val);
                if (name) {
                    fprintf(stream, "%s%c%s=%s\n", prefix,
                            DISPLAY_NAME_SEPARATOR, a->name, name);
                }
                continue;
            }

            fprintf(stream, "%s%c%s=%d\n", prefix,
                    DISPLAY_NAME_SEPARATOR, a->name, val);

        } /* entry */

    } /* screen */

    /*
     * Write attributes addressable to display targets
     */

    for (node = system->targets[DISPLAY_TARGET]; node; node = node->next) {

        t = node->t;

        /* skip it if we don't have a handle for this display */

        if (!t->h) continue;

        /* 
         * check to see if we have RANDR gamma available. We may
         * skip writing attributes if it is missing. 
         */

        status = NvCtrlGetAttribute(t, 
                                    NV_CTRL_ATTR_RANDR_GAMMA_AVAILABLE,
                                    &randr_gamma_available);
        if (status != NvCtrlSuccess) {
            randr_gamma_available = 0;
        }

        /* Get the prefix we want to use for the display device target */

        prefix = create_display_device_target_string(t, conf);

        /* loop over all the entries in the table */

        for (entry = 0; entry < attributeTableLen; entry++) {
            const AttributeTableEntry *a = &attributeTable[entry];

            /*
             * skip all attributes that are not supposed to be written
             * to the config file
             */

            if (a->flags.no_config_write) {
                continue;
            }

            /*
             * for the display target we only write color attributes for now
             */

            if (a->type == CTRL_ATTRIBUTE_TYPE_COLOR) {
                float c[3], b[3], g[3];

                if (!randr_gamma_available) continue;

                status = NvCtrlGetColorAttributes(t, c, b, g);
                if (status != NvCtrlSuccess) continue;

                fprintf(stream, "%s%c%s=%f\n",
                        prefix, DISPLAY_NAME_SEPARATOR, a->name,
                        get_color_value(a->attr, c, b, g));
                continue;
            }

            /* Only write out integer attributes, string attributes
             * aren't written here.
             */
            if (a->type != CTRL_ATTRIBUTE_TYPE_INTEGER) {
                continue;
            }

            /* Make sure this is a display and writable attribute */

            status = NvCtrlGetAttributePerms(t, a->type, a->attr, &perms);
            if (status != NvCtrlSuccess || !(perms.write) ||
                !(perms.valid_targets & CTRL_TARGET_PERM_BIT(DISPLAY_TARGET))) {
                continue;
            }

            status = NvCtrlGetAttribute(t, a->attr, &val);
            if (status == NvCtrlSuccess) {
                fprintf(stream, "%s%c%s=%d\n", prefix,
                        DISPLAY_NAME_SEPARATOR, a->name, val);
            }
        }

        free(prefix);
    }

    /*
     * Write attributes addressable to GPU targets
     */

    for (node = system->targets[GPU_TARGET]; node; node = node->next) {
        char *target_str = NULL;

        t = node->t;

        /* skip it if we don't have a handle for this gpu */

        if (!t->h) {
            continue;
        }

        /*
         * Construct the prefix that will be printed in the config
         * file in front of each attribute.
         */

        target_str = nvasprintf("[gpu:%d]", NvCtrlGetTargetId(t));
        nvstrtoupper(target_str);

        /* loop over all the entries in the table */

        for (entry = 0; entry < attributeTableLen; entry++) {
            const AttributeTableEntry *a = &attributeTable[entry];

            /*
             * skip all attributes that are not supposed to be written
             * to the config file
             */

            if (a->flags.no_config_write) {
                continue;
            }

            /* Only write out integer attributes, string attributes
             * aren't written here.
             */
            if (a->type != CTRL_ATTRIBUTE_TYPE_INTEGER) {
                continue;
            }

            /*
             * Only write attributes that can be written for a GPU target
             */

            status = NvCtrlGetAttributePerms(t, a->type, a->attr, &perms);
            if (status != NvCtrlSuccess || !(perms.write) ||
                !(perms.valid_targets & CTRL_TARGET_PERM_BIT(GPU_TARGET)))  {
                continue;
            }

            status = NvCtrlGetAttribute(t, a->attr, &val);
            if (status != NvCtrlSuccess) {
                continue;
            }

            fprintf(stream, "%s%c%s=%d\n", target_str,
                    DISPLAY_NAME_SEPARATOR, a->name, val);
        }

        free(target_str);
    }

    /*
     * loop the ParsedAttribute list, writing the attributes to file.
     * note that we ignore conf->include_display_name_in_config_file
     * when writing these parsed attributes; this is because parsed
     * attributes (like the framelock properties) require a display
     * name be specified (since there are multiple X servers
     * involved).
     */

    while (p) {
        char target_str[64];
        const AttributeTableEntry *a = p->attr_entry;

        if (!p->next) {
            p = p->next;
            continue;
        }

        /*
         * if the parsed attribute has a target specification, and a
         * target type other than an X screen, include a target
         * specification in what we write to the .rc file.
         */

        target_str[0] = '\0';

        if (p->parser_flags.has_target &&
            (p->target_type != X_SCREEN_TARGET)) {

            const CtrlTargetTypeInfo *targetTypeInfo;

            /* Find the target name of the target type */
            targetTypeInfo = NvCtrlGetTargetTypeInfo(p->target_type);
            if (targetTypeInfo) {
                snprintf(target_str, 64, "[%s:%d]",
                         targetTypeInfo->parsed_name, p->target_id);
            }
        }

        if (a->flags.hijack_display_device) {
            fprintf(stream, "%s%s%c%s[0x%08x]=%d\n", p->display, target_str,
                    DISPLAY_NAME_SEPARATOR, a->name,
                    p->display_device_mask,
                    p->val.i);
        } else {
            fprintf(stream, "%s%s%c%s=%d\n", p->display, target_str,
                    DISPLAY_NAME_SEPARATOR, a->name, p->val.i);
        }


        p = p->next;
    }

    setlocale(LC_NUMERIC, conf->locale);

    /* close the configuration file */

    ret = fclose(stream);
    if (ret != 0) {
        nv_error_msg("Failure while closing file '%s'.", filename);
        return NV_FALSE;
    }
    
    return NV_TRUE;
    
} /* nv_write_config_file() */



/*
 * parse_config_file() - scan through the buffer; skipping comment
 * lines.  Non-comment lines with non-whitespace characters are passed
 * on to nv_parse_attribute_string for parsing.
 *
 * If an error occurs, an error message is printed and NULL is
 * returned.  If successful, a malloced array of
 * ParsedAttributeWrapper structs is returned.  The last
 * ParsedAttributeWrapper in the array has line == -1.  It is the
 * caller's responsibility to free the array.
 */

static ParsedAttributeWrapper *parse_config_file(char *buf, const char *file,
                                                 const int length,
                                                 ConfigProperties *conf)
{
    int line, has_data, current_tmp_len, len, n, ret;
    char *cur, *c, *comment, *tmp;
    ParsedAttributeWrapper *w;
    
    cur = buf;
    line = 1;
    current_tmp_len = 0;
    n = 0;
    w = NULL;
    tmp = NULL;

    while (cur) {
        c = cur;
        comment = NULL;
        has_data = NV_FALSE;;
        
        while (((c - buf) < length) &&
               (*c != '\n') &&
               (*c != '\0')) {
            if (comment) { c++; continue; }
            if (*c == '#') { comment = c; continue; }
            if (!isspace(*c)) has_data = NV_TRUE;
            c++;
        }
        
        if (has_data) {
            if (!comment) comment = c;
            len = comment - cur;
            
            /* grow the tmp buffer if it's too small */
            
            if (len >= current_tmp_len) {
                current_tmp_len = len + 1;
                if (tmp) {
                    free(tmp);
                }
                tmp = nvalloc(sizeof(char) * current_tmp_len);
            }

            strncpy (tmp, cur, len);
            tmp[len] = '\0';

            /* first, see if this line is a config property */

            if (!parse_config_property(file, tmp, conf)) {
                
                w = nvrealloc(w, sizeof(ParsedAttributeWrapper) * (n+1));
            
                ret = nv_parse_attribute_string(tmp,
                                                NV_PARSER_ASSIGNMENT,
                                                &w[n].a);
                if (ret != NV_PARSER_STATUS_SUCCESS) {
                    nv_error_msg("Error parsing configuration file '%s' on "
                                 "line %d: '%s' (%s).",
                                 file, line, tmp, nv_parse_strerror(ret));
                    goto failed;
                }
            
                w[n].line = line;
                n++;
            }
        }

        if (((c - buf) >= length) || (*c == '\0')) cur = NULL;
        else cur = c + 1;

        line++;
    }
    free(tmp);
    /* mark the end of the array */

    w = nvrealloc(w, sizeof(ParsedAttributeWrapper) * (n+1));
    w[n].line = -1;
    
    return w;

 failed:
    if (w) free(w);
    free(tmp);
    return NULL;

} /* parse_config_file() */




/*
 * process_config_file_attributes() - process the list of
 * attributes to be assigned that we acquired in parsing the config
 * file.
 */

static int process_config_file_attributes(const Options *op,
                                          const char *file,
                                          ParsedAttributeWrapper *w,
                                          const char *display_name,
                                          CtrlSystemList *systems)
{
    int i;
    
    NvVerbosity old_verbosity = nv_get_verbosity();

    /* Override the verbosity in the default behavior so
     * nvidia-settings isn't so alarmist when loading the RC file.
     */
    if (__dynamic_verbosity) {
        nv_set_verbosity(NV_VERBOSITY_NONE);
    }

    /*
     * make sure that all ParsedAttributes have displays (this will do
     * nothing if we already have a display name
     */

    for (i = 0; w[i].line != -1; i++) {
        nv_assign_default_display(&w[i].a, display_name);
    }

    /* connect to all the systems referenced in the config file */

    for (i = 0; w[i].line != -1; i++) {
        w[i].system = NvCtrlConnectToSystem(w[i].a.display, systems);
    }

    /* now process each attribute, passing in the correct system */

    for (i = 0; w[i].line != -1; i++) {

        nv_process_parsed_attribute(op, &w[i].a, w[i].system, NV_TRUE, NV_FALSE,
                                    "on line %d of configuration file "
                                    "'%s'", w[i].line, file);
        /*
         * We do not fail if processing the attribute failed.  If the
         * GPU or the X config changed (for example stereo is
         * disabled), some attributes written in the config file may
         * not be advertised by the NVCTRL extension (for example the
         * control to force stereo)
         */
    }
    
    /* Reset the default verbosity */

    if (__dynamic_verbosity) {
        nv_set_verbosity(old_verbosity);
    }

    return NV_TRUE;
    
} /* process_config_file_attributes() */



/*
 * save_gui_parsed_attributes() - scan through the parsed attribute
 * wrappers, and save any relevant attributes to the attribute list to
 * be passed to the gui.
 */

static void save_gui_parsed_attributes(ParsedAttributeWrapper *w,
                                       ParsedAttribute *p_list)
{
    int i;

    for (i = 0; w[i].line != -1; i++) {
        ParsedAttribute *p = &(w[i].a);
        if (p->attr_entry->flags.is_gui_attribute) {
            nv_parsed_attribute_add(p_list, p);
        }
    }
}



static float get_color_value(int attr, float c[3], float b[3], float g[3])
{
    switch (attr & (ALL_VALUES | ALL_CHANNELS)) {
    case (CONTRAST_VALUE | RED_CHANNEL):     return c[RED_CHANNEL_INDEX];
    case (CONTRAST_VALUE | GREEN_CHANNEL):   return c[GREEN_CHANNEL_INDEX];
    case (CONTRAST_VALUE | BLUE_CHANNEL):    return c[BLUE_CHANNEL_INDEX];
    case (BRIGHTNESS_VALUE | RED_CHANNEL):   return b[RED_CHANNEL_INDEX];
    case (BRIGHTNESS_VALUE | GREEN_CHANNEL): return b[GREEN_CHANNEL_INDEX];
    case (BRIGHTNESS_VALUE | BLUE_CHANNEL):  return b[BLUE_CHANNEL_INDEX];
    case (GAMMA_VALUE | RED_CHANNEL):        return g[RED_CHANNEL_INDEX];
    case (GAMMA_VALUE | GREEN_CHANNEL):      return g[GREEN_CHANNEL_INDEX];
    case (GAMMA_VALUE | BLUE_CHANNEL):       return g[BLUE_CHANNEL_INDEX];
    default:                                 return 0.0;
    }
} /* get_color_value() */




/*
 * Table of ConfigProperties (properties of the nvidia-settings
 * utilities itself, rather than properties of the X screen(s) that
 * nvidia-settings is configuring).  The table just binds string names
 * to the bitmask constants.
 */

typedef struct {
    char *name;
    unsigned int flag;
} ConfigPropertiesTableEntry;

ConfigPropertiesTableEntry configPropertyTable[] = {
    { "DisplayStatusBar", CONFIG_PROPERTIES_DISPLAY_STATUS_BAR },
    { "SliderTextEntries", CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES },
    { "IncludeDisplayNameInConfigFile",
      CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE },
    { "UpdateRulesOnProfileNameChange",
      CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE },
    { NULL, 0 }
};


    
/*
 * parse_config_property() - special case the config properties; if
 * the given line sets a config property, update conf as appropriate
 * and return NV_TRUE.  If the given line does not describe a config
 * property, return NV_FALSE.
 */

static int parse_config_property(const char *file, const char *line, ConfigProperties *conf)
{
    char *no_spaces, *s;
    char *locale;
    ConfigPropertiesTableEntry *t;
    char *timer, *token;
    TimerConfigProperty *c = NULL;
    int interval;
    int ret = NV_FALSE;
    int i;
    unsigned int flag;
    const char *ignoredProperties[] = {
        "TextureSharpen",
        "ToolTips",
        "ShowQuitDialog"
    };

    no_spaces = remove_spaces(line);

    if (!no_spaces) goto done;

    s = strchr(no_spaces, '=');

    if (!s) goto done;

    *s = '\0';

    for (i = 0; i < ARRAY_LEN(ignoredProperties); i++) {
        if (nv_strcasecmp(no_spaces, ignoredProperties[i])) {
            ret = NV_TRUE;
            goto done;
        }
    }

    if (nv_strcasecmp(no_spaces, "RcFileLocale")) {
        locale = ++s;
        if (setlocale(LC_NUMERIC, locale) == NULL) {
            nv_warning_msg("Error parsing configuration file '%s': could "
                           "not set the specified locale '%s'.",
                           file, locale);
        }
    } else if (nv_strcasecmp(no_spaces, "Timer")) {
        timer = ++s;

        token = strtok(timer, ",");
        if (!token)
            goto done;

        c = nvalloc(sizeof(TimerConfigProperty));

        c->description = replace_characters(token, '_', ' ');

        token = strtok(NULL, ",");
        if (!token)
            goto done;

        if (nv_strcasecmp(token, "Yes")) {
            c->user_enabled = 1;
        } else if (nv_strcasecmp(token, "No")) {
            c->user_enabled = 0;
        } else {
            goto done;
        }

        token = strtok(NULL, ",");
        if (!token)
            goto done;

        parse_read_integer(token, &interval);
        c->interval = interval;

        c->next = conf->timers;
        conf->timers = c;
    } else {
        for (t = configPropertyTable, flag = 0; t->name; t++) {
            if (nv_strcasecmp(no_spaces, t->name)) {
                flag = t->flag;
                break;
            }
        }

        if (!flag) goto done;

        s++;

        if (nv_strcasecmp(s, "yes")) {
            conf->booleans |= flag;
        } else if (nv_strcasecmp(s, "no")) {
            conf->booleans &= ~flag;
        } else {
            goto done;
        }
    }
    
    ret = NV_TRUE;

 done:

    if ((ret != NV_TRUE) && c) {
        if (c->description)
            free(c->description);
        free(c);
    }

    if (no_spaces) free(no_spaces);
    return ret;
    
} /* parse_config_property() */



/*
 * write_config_properties() - write the ConfigProperties to file;
 * this just amounts to looping through the table, and printing if
 * each property is enabled or disabled.
 */

static void write_config_properties(FILE *stream, const ConfigProperties *conf, char *locale)
{
    ConfigPropertiesTableEntry *t;
    TimerConfigProperty *c;
    char *description;

    fprintf(stream, "\n");
    fprintf(stream, "# ConfigProperties:\n");
    fprintf(stream, "\n");

    fprintf(stream, "RcFileLocale = %s\n", locale);

    for (t = configPropertyTable; t->name; t++) {
        fprintf(stream, "%s = %s\n", t->name,
                (t->flag & conf->booleans) ? "Yes" : "No");
    }

    for (c = conf->timers; (c != NULL); c = c->next) {
        description = replace_characters(c->description, ' ', '_');
        fprintf(stream, "Timer = %s,%s,%u\n",
                description, c->user_enabled ? "Yes" : "No",
                c->interval);
        free(description);
    }
} /* write_config_properties()*/



/*
 * init_config_properties() - initialize the ConfigProperties
 * structure.
 */

void init_config_properties(ConfigProperties *conf)
{
    memset(conf, 0, sizeof(ConfigProperties));

    conf->booleans = 
        (CONFIG_PROPERTIES_DISPLAY_STATUS_BAR |
         CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES |
         CONFIG_PROPERTIES_UPDATE_RULES_ON_PROFILE_NAME_CHANGE);

    conf->locale = strdup(setlocale(LC_NUMERIC, NULL));

} /* init_config_properties() */


/*
 * create_display_device_target_string() - create the string
 * to specify the display device target in the config file.
 */

static char *create_display_device_target_string(CtrlTarget *t,
                                                 const ConfigProperties *conf)
{
    char *target_name = NULL;
    char *target_prefix_name = NULL;
    char *display_name = NULL;
    char *s;

    if (t->protoNames[NV_DPY_PROTO_NAME_RANDR]) {
        target_name = t->protoNames[NV_DPY_PROTO_NAME_RANDR];
    }

    /* If we don't have a target name here, use the full name and return. */
    if (!target_name) {
        return nvstrdup(t->name);
    }

    /* Get the display name if the user requested it to be used. */
    if (conf->booleans &
        CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE) {
        display_name = NvCtrlGetDisplayName(t);
    }

    /* Get the target type prefix. */
    target_prefix_name = nvstrdup(t->targetTypeInfo->parsed_name);
    nvstrtoupper(target_prefix_name);

    /* Build the string */
    if (display_name && target_prefix_name) {
        s = nvasprintf("%s[%s:%s]", display_name,
                       target_prefix_name, target_name);
    } else if (target_prefix_name) {
        s = nvasprintf("[%s:%s]", target_prefix_name, target_name);
    } else if (display_name) {
        s = nvasprintf("%s[%s]", display_name, target_name);
    } else {
        s = nvasprintf("[%s]", target_name);
    }

    free(target_prefix_name);
    free(display_name);

    return s;
}

