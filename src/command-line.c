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

#include "common-utils.h"

/* local prototypes */

static void print_attribute_help(char *attr);
static void print_help(void);

/*
 * verbosity, controls output of errors, warnings and other
 * information (used by msg.c).
 */

int __verbosity = VERBOSITY_DEFAULT;
int __terse = NV_FALSE;
int __display_device_string = NV_FALSE;
int __verbosity_level_changed = NV_FALSE;

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
    const AttributeTableEntry *entry;
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


static void print_help_helper(const char *name, const char *description)
{
    nv_msg(TAB, name);
    nv_msg_preserve_whitespace(BIGTAB, description);
    nv_msg(NULL, "");
}

/*
 * print_help() - loop through the __options[] table, and print the
 * description of each option.
 */

void print_help(void)
{
    print_version();

    nv_msg(NULL, "");
    nv_msg(NULL, "nvidia-settings [options]");
    nv_msg(NULL, "");

    nvgetopt_print_help(__options, 0, print_help_helper);
}


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

Options *parse_command_line(int argc, char *argv[], char *dpy, 
                            CtrlHandlesArray *handles_array)
{
    Options *op;
    int n, c;
    char *strval;

    op = nvalloc(sizeof(Options));
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
        case 'p': op->page = strval; break;
        case 'V':
            __verbosity = VERBOSITY_DEFAULT;
            if (!strval) {
                /* user didn't give argument, assume "all" */
                __verbosity = VERBOSITY_ALL;
            } else if (nv_strcasecmp(strval, "none") == NV_TRUE) {
                __verbosity = VERBOSITY_NONE;
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
            __verbosity_level_changed = NV_TRUE;
            break;
        case 'a':
            n = op->num_assignments;
            op->assignments = nvrealloc(op->assignments,
                                        sizeof(char *) * (n+1));
            op->assignments[n] = strval;
            op->num_assignments++;
            break;
        case 'q':
            n = op->num_queries;
            op->queries = nvrealloc(op->queries, sizeof(char *) * (n+1));
            op->queries[n] = strval;
            op->num_queries++;
            break;
        case CONFIG_FILE_OPTION: op->config = strval; break;
        case 'g': print_glxinfo(NULL, handles_array); exit(0); break;
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

