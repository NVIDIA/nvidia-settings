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
#include "config-file.h"

/* local prototypes */

static void print_attribute_help(const char *attr);
static void print_help(void);

/*
 * print_version() - print version information
 */

extern const char *pNV_ID;

static void print_version(void)
{
    nv_msg(NULL, "");
    nv_msg(NULL, "%s", pNV_ID);
    nv_msg(TAB, "The NVIDIA X Server Settings tool.");
    nv_msg(NULL, "");
    nv_msg(TAB, "This program is used to configure the "
           "NVIDIA Linux graphics driver.");
    nv_msg(TAB, "For more detail, please see the nvidia-settings(1) "
           "man page.");
    nv_msg(NULL, "");
    
} /* print_version() */

/*
 * print_attribute_help() - print information about the specified attribute.
 */

static void print_attribute_help(const char *attr)
{
    int i;
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

    for (i = 0; i < attributeTableLen; i++) {
        const AttributeTableEntry *entry = attributeTable + i;

        if (list_all || !strcasecmp(attr, entry->name)) {

            if (show_desc) {
                nv_msg(NULL, "Attribute '%s':", entry->name);

                /* Attribute type (value) information */

                switch (entry->type) {
                case CTRL_ATTRIBUTE_TYPE_INTEGER:
                    nv_msg(NULL, "  - Attribute value is an integer.");
                    break;
                case CTRL_ATTRIBUTE_TYPE_STRING:
                case CTRL_ATTRIBUTE_TYPE_STRING_OPERATION:
                    nv_msg(NULL, "  - Attribute value is a string.");
                    break;
                case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
                    nv_msg(NULL, "  - Attribute value is binary data.");
                    break;
                case CTRL_ATTRIBUTE_TYPE_COLOR:
                    nv_msg(NULL, "  - Attribute value is a color.");
                    break;
                case CTRL_ATTRIBUTE_TYPE_SDI_CSC:
                    nv_msg(NULL, "  - Attribute value is a SDI CSC matrix.");
                    break;
                }

                /* Attribute flags (common) */

                if (entry->flags.is_gui_attribute) {
                    nv_msg(NULL, "  - Is GUI attribute.");
                }
                if (entry->flags.is_framelock_attribute) {
                    nv_msg(NULL, "  - Is Frame Lock attribute.");
                }
                if (entry->flags.is_sdi_attribute) {
                    nv_msg(NULL, "  - Is SDI attribute.");
                }
                if (entry->flags.no_config_write) {
                    nv_msg(NULL, "  - Attribute is not written to the rc file.");
                }
                if (entry->flags.no_query_all) {
                    nv_msg(NULL, "  - Attribute not queried in 'query all'.");
                }

                /* Attribute type-specific flags */

                switch (entry->type) {
                case CTRL_ATTRIBUTE_TYPE_INTEGER:
                    if (entry->f.int_flags.is_100Hz) {
                        nv_msg(NULL, "  - Attribute value is in units of Centihertz "
                               "(1/100Hz).");
                    }
                    if (entry->f.int_flags.is_1000Hz) {
                        nv_msg(NULL, "  - Attribute value is in units of Milihertz "
                               "(1/1000 Hz).");
                    }
                    if (entry->f.int_flags.is_packed) {
                        nv_msg(NULL, "  - Attribute value is packed integer.");
                    }
                    if (entry->f.int_flags.is_display_mask) {
                        nv_msg(NULL, "  - Attribute value is a display mask.");
                    }
                    if (entry->f.int_flags.is_display_id) {
                        nv_msg(NULL, "  - Attribute value is a display ID.");
                    }
                    if (entry->f.int_flags.no_zero) {
                        nv_msg(NULL, "  - Attribute cannot be zero.");
                    }
                    if (entry->f.int_flags.is_switch_display) {
                        nv_msg(NULL, "  - Attribute value is switch display.");
                    }
                    break;
                case CTRL_ATTRIBUTE_TYPE_STRING:
                case CTRL_ATTRIBUTE_TYPE_COLOR:
                case CTRL_ATTRIBUTE_TYPE_SDI_CSC:
                case CTRL_ATTRIBUTE_TYPE_STRING_OPERATION:
                case CTRL_ATTRIBUTE_TYPE_BINARY_DATA:
                    /* Nothing specific to report for these */
                    break;
                }

                nv_msg(TAB, "%s", entry->desc);
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
    nv_msg(TAB, "%s", name);
    nv_msg_preserve_whitespace(BIGTAB, "%s", description);
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
 * arguments.  This must be called before the gui is initialized so
 * that the correct gui library can be used. Arguments for the gui must
 * follow a '--' marker. The marker will be removed before passing the
 * commandline arguments to the gui init function.
 */

Options *parse_command_line(int argc, char *argv[],
                            CtrlSystemList *systems)
{
    Options *op;
    int n, c;
    char *strval;
    int boolval;

    op = nvalloc(sizeof(Options));
    op->config = DEFAULT_RC_FILE;
    op->write_config = NV_TRUE;

    /*
     * initialize the controlled display to the gui display name
     * passed in.
     */

    while (1) {
        c = nvgetopt(argc, argv, __options, &strval,
                     &boolval,  /* boolval */
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
        case DISPLAY_OPTION:
            /*
             * --ctrl-display and --display can both be specified so only assign
             * --display to ctrl_display if it is not yet assigned.
             */
            if (!op->ctrl_display) {
                op->ctrl_display = strval;
            }
            break;
        case 'p': op->page = strval; break;
        case 'V':
            nv_set_verbosity(NV_VERBOSITY_DEFAULT);
            if (!strval) {
                /* user didn't give argument, assume "all" */
                nv_set_verbosity(NV_VERBOSITY_ALL);
            } else if (nv_strcasecmp(strval, "none") == NV_TRUE) {
                nv_set_verbosity(NV_VERBOSITY_NONE);
            } else if (nv_strcasecmp(strval, "errors") == NV_TRUE) {
                nv_set_verbosity(NV_VERBOSITY_ERROR);
            } else if (nv_strcasecmp(strval, "deprecations") == NV_TRUE) {
                nv_set_verbosity(NV_VERBOSITY_DEPRECATED);
            } else if (nv_strcasecmp(strval, "warnings") == NV_TRUE) {
                nv_set_verbosity(NV_VERBOSITY_WARNING);
            } else if (nv_strcasecmp(strval, "all") == NV_TRUE) {
                nv_set_verbosity(NV_VERBOSITY_ALL);
            } else {
                nv_error_msg("Invalid verbosity level '%s'.  Please run "
                             "`%s --help` for usage information.\n",
                             strval, argv[0]);
                exit(0);
            }
            set_dynamic_verbosity(NV_FALSE);
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
        case 'g': print_glxinfo(NULL, systems); exit(0); break;
        case 't': op->terse = NV_TRUE; break;
        case 'd': op->dpy_string = NV_TRUE; break;
        case 'e': print_attribute_help(strval); exit(0); break;
        case 'L': op->list_targets = NV_TRUE; break;
        case 'w': op->write_config = boolval; break;
        case 'i': op->use_gtk2 = NV_TRUE; break;
        case 'I': op->gtk_lib_path = strval; break;
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

