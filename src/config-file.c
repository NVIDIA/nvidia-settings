/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

/*
 * config-file.c - this source file contains functions for processing
 * the NVIDIA X Server control panel configuration file.
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

#include "NvCtrlAttributes.h"

#include "config-file.h"
#include "query-assign.h"
#include "parse.h"
#include "msg.h"

#define MAX_CONFIG_FILE_LINE_LEN 256


typedef struct {
    ParsedAttribute a;
    int line;
    CtrlHandles *h;
} ParsedAttributeWrapper;


static ParsedAttributeWrapper *parse_config_file(char *buf,
                                                 const char *file,
                                                 const int length,
                                                 ConfigProperties *);

static int process_config_file_attributes(const char *file,
                                          ParsedAttributeWrapper *w,
                                          const char *display_name);

static void save_gui_parsed_attributes(ParsedAttributeWrapper *w,
                                       ParsedAttribute *p);

static float get_color_value(int attr,
                             float c[3], float b[3], float g[3]);

static int parse_config_properties(const char *line, ConfigProperties *conf);

static void init_config_properties(ConfigProperties *conf);

static void write_config_properties(FILE *stream, ConfigProperties *conf);


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
 * XXX should we do any sort of versioning to handle compatibility
 * problems in the future?
 */

int nv_read_config_file(const char *file, const char *display_name,
                        ParsedAttribute *p, ConfigProperties *conf)
{
    int fd, ret, length;
    struct stat stat_buf;
    char *buf;
    ParsedAttributeWrapper *w = NULL;
    
    /* initialize the ConfigProperties */

    init_config_properties(conf);

    /* open the file */

    fd = open(file, O_RDONLY);
    if (fd == -1) {
        /*
         * It's OK if the file doesn't exist... but should we print a
         * warning?
         */
        return NV_FALSE;
    }
    
    /* get the size of the file */

    ret = fstat(fd, &stat_buf);
    if (ret == -1) {
        nv_error_msg("Unable to determine size of file '%s' (%s).",
                     file, strerror(errno));
        return NV_FALSE;
    }

    if (stat_buf.st_size == 0) {
        nv_warning_msg("File '%s' has zero size; not reading.", file);
        close(fd);
        return NV_TRUE;
    }

    length = stat_buf.st_size;

    /* map the file into memory */

    buf = mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);
    if (buf == (void *) -1) {
        nv_error_msg("Unable to mmap file '%s' for reading (%s).",
                     file, strerror(errno));
        return NV_FALSE;
    }
    
    /* parse the actual text in the file */

    w = parse_config_file(buf, file, length, conf);

    if (munmap (buf, stat_buf.st_size) == -1) {
        nv_error_msg("Unable to unmap file '%s' after reading (%s).",
                     file, strerror(errno));
        return NV_FALSE;
    }

    /* close the file */

    close(fd);
    
    if (!w) return NV_FALSE;

    /* process the parsed attributes */

    ret = process_config_file_attributes(file, w, display_name);

    /*
     * add any relevant parsed attributes back to the list to be
     * passed to the gui
     */

    save_gui_parsed_attributes(w, p);
    
    if (w) free(w);

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

int nv_write_config_file(const char *filename, CtrlHandles *h,
                         ParsedAttribute *p, ConfigProperties *conf)
{
    int screen, ret, entry, bit, val;
    FILE *stream;
    time_t now;
    AttributeTableEntry *a;
    ReturnStatus status;
    NVCTRLAttributeValidValuesRec valid;
    uint32 mask;
    CtrlHandleTarget *t;
    char *tmp_d_str, *tmp, *prefix, scratch[4];

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
            "X Server Settings utility\n");

    /* NOTE: ctime(3) generates a new line */
    
    fprintf(stream, "# Generated on %s", ctime(&now));
    fprintf(stream, "#\n");
    
    /* write the values in ConfigProperties */

    write_config_properties(stream, conf);

    /* for each screen, query each attribute in the table */

    fprintf(stream, "\n");
    fprintf(stream, "# Attributes:\n");
    fprintf(stream, "\n");

    /*
     * Note: we only save writable attributes addressable by X screen
     * (i.e., we don't look at other target types, yet).
     */
    
    for (screen = 0; screen < h->targets[X_SCREEN_TARGET].n; screen++) {

        t = &h->targets[X_SCREEN_TARGET].t[screen];

        /* skip it if we don't have a handle for this screen */

        if (!t->h) continue;

        /*
         * construct the prefix that will be printed in the config
         * file infront of each attribute on this screen; this will
         * either be "[screen]" or "[displayname]".
         */

        if (conf->booleans &
            CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE) {
            prefix = t->name;
        } else {
            snprintf(scratch, 4, "%d", screen);
            prefix = scratch;
        }

        /* loop over all the entries in the table */

        for (entry = 0; attributeTable[entry].name; entry++) {

            a = &attributeTable[entry];
            
            /* 
             * skip all attributes that are not supposed to be written
             * to the config file
             */

            if (a->flags & NV_PARSER_TYPE_NO_CONFIG_WRITE) continue;

            /*
             * special case the color attributes because we want to
             * print floats
             */
            
            if (a->flags & NV_PARSER_TYPE_COLOR_ATTRIBUTE) {
                float c[3], b[3], g[3];
                status = NvCtrlGetColorAttributes(t->h, c, b, g);
                if (status != NvCtrlSuccess) continue;
                fprintf(stream, "%s%c%s=%f\n",
                        prefix, DISPLAY_NAME_SEPARATOR, a->name,
                        get_color_value(a->attr, c, b, g));
                continue;
            }
            
            for (bit = 0; bit < 24; bit++) {
                
                mask = 1 << bit;
                
                if ((t->d & mask) == 0x0) continue;

                status = NvCtrlGetValidDisplayAttributeValues
                    (t->h, mask, a->attr, &valid);

                if (status != NvCtrlSuccess) goto exit_bit_loop;
                
                if ((valid.permissions & ATTRIBUTE_TYPE_WRITE) == 0x0)
                    goto exit_bit_loop;
                
                status = NvCtrlGetDisplayAttribute(t->h, mask, a->attr, &val);
                
                if (status != NvCtrlSuccess) goto exit_bit_loop;
                
                if (valid.permissions & ATTRIBUTE_TYPE_DISPLAY) {

                    tmp_d_str =
                        display_device_mask_to_display_device_name(mask);

                    fprintf(stream, "%s%c%s[%s]=%d\n", prefix,
                            DISPLAY_NAME_SEPARATOR, a->name, tmp_d_str, val);
                    
                    free(tmp_d_str);
                    
                    continue;
                    
                } else {

                    fprintf(stream, "%s%c%s=%d\n", prefix,
                            DISPLAY_NAME_SEPARATOR, a->name, val);

                    /* fall through to exit_bit_loop */
                }
                
            exit_bit_loop:

                bit = 25; /* XXX force us out of the display device loop */
                
            } /* bit */
            
        } /* entry */
        
    } /* screen */
    
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

        if (!p->next) {
            p = p->next;
            continue;
        }

        tmp = nv_get_attribute_name(p->attr);
        if (!tmp) {
            nv_error_msg("Failure to save unknown attribute %d.", p->attr);
            p = p->next;
            continue;
        }

        /*
         * if the parsed attribute has a target specification, and a
         * target type other than an X screen, include a target
         * specification in what we write to the .rc file.
         */
        
        target_str[0] = '\0';
        
        if ((p->flags & NV_PARSER_HAS_TARGET) &&
            (p->target_type != NV_CTRL_TARGET_TYPE_X_SCREEN)) {
            
            int j;
            
            /* Find the target name of the target type */
            for (j = 0; targetTypeTable[j].name; j++) {
                if (targetTypeTable[j].nvctrl == p->target_type) {
                    snprintf(target_str, 64, "[%s:%d]",
                             targetTypeTable[j].parsed_name, p->target_id);
                    break;
                }
            }
        }
        
        if (p->display_device_mask) {
            
            tmp_d_str = display_device_mask_to_display_device_name
                (p->display_device_mask);
            
            fprintf(stream, "%s%s%c%s[%s]=%d\n", p->display, target_str,
                    DISPLAY_NAME_SEPARATOR, tmp, tmp_d_str, p->val);
            
            free(tmp_d_str);
            
        } else {
                
            fprintf(stream, "%s%s%c%s=%d\n", p->display, target_str,
                    DISPLAY_NAME_SEPARATOR, tmp, p->val);
        }
        
        p = p->next;
    }

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
    int line, has_data, len, n, ret;
    char *cur, *c, *comment, tmp[MAX_CONFIG_FILE_LINE_LEN];
    ParsedAttributeWrapper *w;
    
    cur = buf;
    line = 1;
    n = 0;
    w = NULL;

    while (cur) {
        c = cur;
        comment = NULL;
        has_data = NV_FALSE;;
        
        while (((c - buf) < length) &&
               (*c != '\n') &&
               (*c != '\0') &&
               (*c != EOF)) {
            if (comment) { c++; continue; }
            if (*c == '#') { comment = c; continue; }
            if (!isspace(*c)) has_data = NV_TRUE;
            c++;
        }

        if (has_data) {
            if (!comment) comment = c;
            len = comment - cur;
            if (len >= MAX_CONFIG_FILE_LINE_LEN) {
                nv_error_msg("Error parsing configuration file '%s' on "
                             "line %d: line length exceeds maximum "
                             "length of %d.",
                             file, line, MAX_CONFIG_FILE_LINE_LEN);
                goto failed;
            }

            strncpy (tmp, cur, len);
            tmp[len] = '\0';

            /* first, see if this line is a config property */

            if (!parse_config_properties(tmp, conf)) {
                
                w = realloc(w, sizeof(ParsedAttributeWrapper) * (n+1));
            
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

        if (((c - buf) >= length) || (*c == '\0') || (*c == EOF)) cur = NULL;
        else cur = c + 1;
        
        line++;
    }

    /* mark the end of the array */

    w = realloc(w, sizeof(ParsedAttributeWrapper) * (n+1));
    w[n].line = -1;
    
    return w;

 failed:
    if (w) free(w);
    return NULL;

} /* parse_config_file() */




/*
 * process_config_file_attributes() - process the list of
 * attributes to be assigned that we acquired in parsing the config
 * file.
 */

static int process_config_file_attributes(const char *file,
                                          ParsedAttributeWrapper *w,
                                          const char *display_name)
{
    int i, j, ret, found, n = 0;
    CtrlHandles **h = NULL;
    
    /*
     * make sure that all ParsedAttributes have displays (this will do
     * nothing if we already have a display name
     */

    for (i = 0; w[i].line != -1; i++) {
        nv_assign_default_display(&w[i].a, display_name);
    }
    
    /* build the list of CtrlHandles */
    
    for (i = 0; w[i].line != -1; i++) {
        found = NV_FALSE;
        for (j = 0; j < n; j++) {
            if (nv_strcasecmp(h[j]->display, w[i].a.display)) {
                w[i].h = h[j];
                found = NV_TRUE;
                break;
            }
        }

        /*
         * no handle found for this display, need to create a new
         * handle.
         *
         * XXX we should really just build a list of what ctrl_handles
         * we need, and what attributes on which ctrl_handles, so that
         * we don't have to pass NV_CTRL_ATTRIBUTES_ALL_SUBSYSTEMS to
         * NvCtrlAttributeInit (done in nv_alloc_ctrl_handles())
         * unless we really need it.
         */

        if (!found) {
            h = realloc(h, sizeof(CtrlHandles *) * (n + 1));
            h[n] = nv_alloc_ctrl_handles(w[i].a.display);
            w[i].h = h[n];
            n++;
        }
    }
    
    /* now process each attribute, passing in the correct CtrlHandles */

    for (i = 0; w[i].line != -1; i++) {

        ret = nv_process_parsed_attribute(&w[i].a, w[i].h, NV_TRUE, NV_FALSE,
                                          "on line %d of configuration file "
                                          "'%s'", w[i].line, file);
        /*
         * We do not fail if processing the attribute failed.  If the
         * GPU or the X config changed (for example stereo is
         * disabled), some attributes written in the config file may
         * not be advertised by the the NVCTRL extension (for example
         * the control to force stereo)
         */
    }
    
    /* free all the CtrlHandles we allocated */

    for (i = 0; i < n; i++) {
        nv_free_ctrl_handles(h[i]);
    }
    
    if (h) free(h);


    return NV_TRUE;
    
} /* process_config_file_attributes() */



/*
 * save_gui_parsed_attributes() - scan through the parsed attribute
 * wrappers, and save any relevant attributes to the attribute list to
 * be passed to the gui.
 */

static void save_gui_parsed_attributes(ParsedAttributeWrapper *w,
                                       ParsedAttribute *p)
{
    int i;

    for (i = 0; w[i].line != -1; i++) {
        if (w[i].a.flags & NV_PARSER_TYPE_GUI_ATTRIUBUTE) {
            nv_parsed_attribute_add(p, &w[i].a);            
        }
    }
} /* save_gui_parsed_attributes() */



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
    { "ToolTips", CONFIG_PROPERTIES_TOOLTIPS },
    { "DisplayStatusBar", CONFIG_PROPERTIES_DISPLAY_STATUS_BAR },
    { "SliderTextEntries", CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES },
    { "IncludeDisplayNameInConfigFile",
      CONFIG_PROPERTIES_INCLUDE_DISPLAY_NAME_IN_CONFIG_FILE },
    { "ShowQuitDialog", CONFIG_PROPERTIES_SHOW_QUIT_DIALOG },
    { NULL, 0 }
};


    
/*
 * parse_config_property() - special case the config properties; if
 * the given line sets a config property, update conf as appropriate
 * and return NV_TRUE.  If the given line does not describe a config
 * property, return NV_FALSE.
 */

static int parse_config_properties(const char *line, ConfigProperties *conf)
{
    char *no_spaces, *s;
    ConfigPropertiesTableEntry *t;
    int ret = NV_FALSE;
    unsigned int flag;
    
    no_spaces = remove_spaces(line);

    if (!no_spaces) goto done;

    s = strchr(no_spaces, '=');

    if (!s) goto done;

    *s = '\0';
    
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
    
    ret = NV_TRUE;

 done:

    if (no_spaces) free(no_spaces);
    return ret;
    
} /* parse_config_property() */



/*
 * write_config_properties() - write the ConfigProperties to file;
 * this just amounts to looping through the table, and printing if
 * each property is enabled or disabled.
 */

static void write_config_properties(FILE *stream, ConfigProperties *conf)
{
    ConfigPropertiesTableEntry *t;

    fprintf(stream, "\n");
    fprintf(stream, "# ConfigProperties:\n");
    fprintf(stream, "\n");

    for (t = configPropertyTable; t->name; t++) {
        fprintf(stream, "%s = %s\n", t->name,
                (t->flag & conf->booleans) ? "Yes" : "No");
    }
} /* write_config_properties()*/



/*
 * init_config_properties() - initialize the ConfigProperties
 * structure.
 */

static void init_config_properties(ConfigProperties *conf)
{
    memset(conf, 0, sizeof(ConfigProperties));

    conf->booleans = 
        (CONFIG_PROPERTIES_TOOLTIPS |
         CONFIG_PROPERTIES_DISPLAY_STATUS_BAR |
         CONFIG_PROPERTIES_SLIDER_TEXT_ENTRIES |
         CONFIG_PROPERTIES_SHOW_QUIT_DIALOG);

} /* init_config_properties() */
