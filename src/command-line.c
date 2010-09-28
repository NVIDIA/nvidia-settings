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

#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "option-table.h"
#include "query-assign.h"
#include "msg.h"
#include "nvgetopt.h"
#include "glxinfo.h"

#include "NvCtrlAttributes.h"


/* local prototypes */

static void print_attribute_help(char *attr);
static void print_help(void);
static char *nvstrcat(const char *str, ...);

/*
 * verbosity, controls output of errors, warnings and other
 * information (used by msg.c).
 */

int __verbosity = VERBOSITY_DEFAULT;
int __terse = NV_FALSE;
int __display_device_string = NV_FALSE;
/*
 * print_version() - print version information
 */

extern const char *pNV_ID;

static void print_version(void)
{
    nv_msg(NULL, "");
    nv_msg(NULL, pNV_ID);
    nv_msg(TAB, "The NVIDIA X Server Settings tool.");
    nv_msg(NULL, "");
    nv_msg(TAB, "This program is used to configure the "
           "NVIDIA Linux graphics driver.");
    nv_msg(TAB, "For more detail, please see the nvidia-settings(1) "
           "man page.");
    nv_msg(NULL, "");
    nv_msg(TAB, "Copyright (C) 2004 - 2010 NVIDIA Corporation.");
    nv_msg(NULL, "");
    
} /* print_version() */

/*
 * print_attribute_help() - print information about the specified attribute.
 */

static void print_attribute_help(char *attr)
{
    AttributeTableEntry *entry;
    int found = 0;
    int list_all = 0;
    int show_desc = 1;


    if (!strcasecmp(attr, "all")) {
        list_all = 1;

    } else if (!strcasecmp(attr, "list")) {
        list_all = 1;
        show_desc = 0;
    }

    nv_msg(NULL, "");

    for (entry = attributeTable; entry->name; entry++) {

        if (list_all || !strcasecmp(attr, entry->name)) {

            if (show_desc) {
                nv_msg(NULL, "Attribute '%s':", entry->name);

                if (entry->flags & NV_PARSER_TYPE_FRAMELOCK)
                    nv_msg(NULL, "  - Is Frame Lock attribute.");
                if (entry->flags & NV_PARSER_TYPE_NO_CONFIG_WRITE)
                    nv_msg(NULL, "  - Attribute is not written to the rc file.");
                if (entry->flags & NV_PARSER_TYPE_GUI_ATTRIBUTE)
                    nv_msg(NULL, "  - Is GUI attribute.");
                if (entry->flags & NV_PARSER_TYPE_XVIDEO_ATTRIBUTE)
                    nv_msg(NULL, "  - Is X Video attribute.");
                if (entry->flags & NV_PARSER_TYPE_PACKED_ATTRIBUTE)
                    nv_msg(NULL, "  - Attribute value is packed integer.");
                if (entry->flags & NV_PARSER_TYPE_VALUE_IS_DISPLAY)
                    nv_msg(NULL, "  - Attribute value is a display mask.");
                if (entry->flags & NV_PARSER_TYPE_NO_QUERY_ALL)
                    nv_msg(NULL, "  - Attribute not queried in 'query all'.");
                if (entry->flags & NV_PARSER_TYPE_NO_ZERO_VALUE)
                    nv_msg(NULL, "  - Attribute cannot be zero.");
                if (entry->flags & NV_PARSER_TYPE_100Hz)
                    nv_msg(NULL, "  - Attribute value is in units of Centihertz (1/100Hz).");
                if (entry->flags & NV_PARSER_TYPE_1000Hz)
                    nv_msg(NULL, "  - Attribute value is in units of Milihertz (1/1000 Hz).");
                if (entry->flags & NV_PARSER_TYPE_STRING_ATTRIBUTE)
                    nv_msg(NULL, "  - Attribute value is string.");
                if (entry->flags & NV_PARSER_TYPE_SDI)
                    nv_msg(NULL, "  - Is SDI attribute.");
                if (entry->flags & NV_PARSER_TYPE_VALUE_IS_SWITCH_DISPLAY)
                    nv_msg(NULL, "  - Attribute value is switch display.");

                nv_msg(TAB, entry->desc);
                nv_msg(NULL, "");
            } else {
                nv_msg(NULL, "%s", entry->name);
            }

            found = 1;
            if (!list_all) break;
        }
    }

    if (!found && !list_all) {
        nv_error_msg("Unrecognized attribute name '%s'.\n", attr);
    }

} /* print_attribute_help() */



/*
 * print_help() - loop through the __options[] table, and print the
 * description of each option.
 */

void print_help(void)
{
    int i, j, len;
    char *msg, *tmp, scratch[64];
    const NVGetoptOption *o;
    
    print_version();

    nv_msg(NULL, "");
    nv_msg(NULL, "nvidia-settings [options]");
    nv_msg(NULL, "");
    
    for (i = 0; __options[i].name; i++) {
        o = &__options[i];
        if (isalpha(o->val)) {
            sprintf(scratch, "%c", o->val);
            msg = nvstrcat("-", scratch, ", --", o->name, NULL);
        } else {
            msg = nvstrcat("--", o->name, NULL);
        }
        if (o->flags & NVGETOPT_HAS_ARGUMENT) {
            len = strlen(o->name);
            for (j = 0; j < len; j++) scratch[j] = toupper(o->name[j]);
            scratch[len] = '\0';
            tmp = nvstrcat(msg, "=[", scratch, "]", NULL);
            free(msg);
            msg = tmp;
        }
        nv_msg(TAB, msg);
        free(msg);

        if (o->description) {
            char *buf = NULL, *pbuf = NULL, *s = NULL;

            buf = calloc(1, 1 + strlen(o->description));
            if (!buf) {
                /* XXX There should be better message than this */
                nv_error_msg("Not enough memory\n");
                return;
            }
            pbuf = buf;

            for (s = o->description; s && *s; s++) {
                switch (*s) {
                case '<':
                case '>':
                case '^':
                    break;
                default:
                    *pbuf = *s;
                    pbuf++;
                    break;
                }
            }
            *pbuf = '\0';
            nv_msg_preserve_whitespace(BIGTAB, buf);
            free(buf);
        }

        nv_msg(NULL, "");
    }
} /* print_help() */


/*
 * parse_command_line() - malloc an Options structure, initialize it
 * with defaults, and fill in any pertinent data from the commandline
 * arguments.  This must be called after the gui is initialized (so
 * that the gui can remove its commandline arguments from argv).
 *
 * XXX it's unfortunate that we need to init the gui before calling
 * this, because many of the commandline options will cause us to not
 * even use the gui.
 */

Options *parse_command_line(int argc, char *argv[], char *dpy)
{
    Options *op;
    int n, c;
    char *strval;

    op = (Options *) malloc(sizeof (Options));
    memset(op, 0, sizeof (Options));
    
    op->config = DEFAULT_RC_FILE;
    
    /*
     * initialize the controlled display to the gui display name
     * passed in.
     */
    
    op->ctrl_display = dpy;
    
    while (1) {
        c = nvgetopt(argc, argv, __options, &strval,
                     NULL,  /* boolval */
                     NULL,  /* intval */
                     NULL,  /* doubleval */
                     NULL); /* disable_val */

        if (c == -1)
            break;
        
        switch (c) {
        case 'v': print_version(); exit(0); break;
        case 'h': print_help(); exit(0); break;
        case 'l': op->only_load = 1; break;
        case 'n': op->no_load = 1; break;
        case 'r': op->rewrite = 1; break;
        case 'c': op->ctrl_display = strval; break;
        case 'V':
            __verbosity = VERBOSITY_DEFAULT;
            if (!strval) {
                /* user didn't give argument, assume "all" */
                __verbosity = VERBOSITY_ALL;
            } else if (nv_strcasecmp(strval, "errors") == NV_TRUE) {
                __verbosity = VERBOSITY_ERROR;
            } else if (nv_strcasecmp(strval, "warnings") == NV_TRUE) {
                __verbosity = VERBOSITY_WARNING;
            } else if (nv_strcasecmp(strval, "all") == NV_TRUE) {
                __verbosity = VERBOSITY_ALL;
            } else {
                nv_error_msg("Invalid verbosity level '%s'.  Please run "
                             "`%s --help` for usage information.\n",
                             strval, argv[0]);
                exit(0);
            }
            break;
        case 'a':
            n = op->num_assignments;
            op->assignments = realloc(op->assignments, sizeof(char *) * (n+1));
            op->assignments[n] = strval;
            op->num_assignments++;
            break;
        case 'q':
            n = op->num_queries;
            op->queries = realloc(op->queries, sizeof(char *) * (n+1));
            op->queries[n] = strval;
            op->num_queries++;
            break;
        case CONFIG_FILE_OPTION: op->config = strval; break;
        case 'g': print_glxinfo(NULL); exit(0); break;
        case 't': __terse = NV_TRUE; break;
        case 'd': __display_device_string = NV_TRUE; break;
        case 'e': print_attribute_help(strval); exit(0); break;
        default:
            nv_error_msg("Invalid commandline, please run `%s --help` "
                         "for usage information.\n", argv[0]);
            exit(0);
        }
    }

    /* do tilde expansion on the config file path */

    op->config = tilde_expansion(op->config);
    
    return op;

} /* parse_command_line() */



/*
 * tilde_expansion() - do tilde expansion on the given path name;
 * based loosely on code snippets found in the comp.unix.programmer
 * FAQ.  The tilde expansion rule is: if a tilde ('~') is alone or
 * followed by a '/', then substitute the current user's home
 * directory; if followed by the name of a user, then substitute that
 * user's home directory.
 */

char *tilde_expansion(const char *str)
{
    char *prefix = NULL;
    const char *replace;
    char *user, *ret;
    struct passwd *pw;
    int len;

    if ((!str) || (str[0] != '~')) return strdup(str);
    
    if ((str[1] == '/') || (str[1] == '\0')) {

        /* expand to the current user's home directory */

        prefix = getenv("HOME");
        if (!prefix) {
            
            /* $HOME isn't set; get the home directory from /etc/passwd */
            
            pw = getpwuid(getuid());
            if (pw) prefix = pw->pw_dir;
        }
        
        replace = str + 1;
        
    } else {
    
        /* expand to the specified user's home directory */

        replace = strchr(str, '/');
        if (!replace) replace = str + strlen(str);

        len = replace - str;
        user = malloc(len + 1);
        strncpy(user, str+1, len-1);
        user[len] = '\0';
        pw = getpwnam(user);
        if (pw) prefix = pw->pw_dir;
        free (user);
    }

    if (!prefix) return strdup(str);
    
    ret = malloc(strlen(prefix) + strlen(replace) + 1);
    strcpy(ret, prefix);
    strcat(ret, replace);
    
    return ret;

} /* tilde_expansion() */


/* XXX useful utility function... where should this go? */

/*
 * nvstrcat() - allocate a new string, copying all given strings
 * into it.  taken from glib
 */

static char *nvstrcat(const char *str, ...)
{
    unsigned int l;
    va_list args;
    char *s;
    char *concat;
  
    l = 1 + strlen(str);
    va_start(args, str);
    s = va_arg(args, char *);

    while (s) {
        l += strlen(s);
        s = va_arg(args, char *);
    }
    va_end(args);
  
    concat = malloc(l);
    concat[0] = 0;
  
    strcat(concat, str);
    va_start(args, str);
    s = va_arg(args, char *);
    while (s) {
        strcat(concat, s);
        s = va_arg(args, char *);
    }
    va_end(args);
  
    return concat;

} /* nvstrcat() */
