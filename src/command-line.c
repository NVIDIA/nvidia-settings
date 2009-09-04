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

#include "command-line.h"
#include "query-assign.h"
#include "msg.h"
#include "nvgetopt.h"
#include "glxinfo.h"

#include "NvCtrlAttributes.h"


#define TAB    "  "
#define BIGTAB "      "

/* local prototypes */

static void print_assign_help(void);
static void print_query_help(void);
static void print_attribute_help(char *attr);
static void print_help(void);
static char *tilde_expansion(char *str);
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
    nv_msg(NULL, "");
    nv_msg(TAB, "Copyright (C) 2004 - 2008 NVIDIA Corporation.");
    nv_msg(NULL, "");
    
} /* print_version() */


/*
 * Options table; the fields are:
 *
 * name - this is the long option name
 *
 * shortname - this is the one character short option name
 *
 * flags - bitmask; possible values are NVGETOPT_HAS_ARGUMENT and
 * NVGETOPT_IS_BOOLEAN
 *
 * description function - function to call to display description
 *                        through nv_msg()
 *
 * description - text for use by print_help() to describe the option
 */

#define CONFIG_FILE_OPTION 1

static const NVGetoptOption __options[] = {
    { "version", 'v', 0, NULL,
      "Print the nvidia-settings version and exit." },
    
    { "help", 'h', 0, NULL,
      "Print usage information and exit." },
    
    { "config", CONFIG_FILE_OPTION, NVGETOPT_HAS_ARGUMENT, NULL,
      "Use the configuration file [CONFIG] rather than the "
      "default " DEFAULT_RC_FILE },

    { "ctrl-display", 'c', NVGETOPT_HAS_ARGUMENT, NULL,
      "Control the specified X display.  If this option is not given, then "
      "nvidia-settings will control the display specifed by '--display'.  If "
      "that is not given, then the $DISPLAY environment variable is used." },
    
    { "load-config-only", 'l', 0, NULL,
      "Load the configuration file, send the values specified therein to "
      "the X server, and exit.  This mode of operation is useful to place "
      "in your .xinitrc file, for example." },

    { "no-config", 'n', 0, NULL,
      "Do not load the configuration file.  This mode of operation is useful "
      "if nvidia-settings has difficulties starting due to problems with "
      "applying settings in the configuration file." },

    { "rewrite-config-file", 'r', 0, NULL,
      "Write the X server configuration to the configuration file, and exit, "
      "without starting the graphical user interface." },

    { "verbose", 'V', NVGETOPT_HAS_ARGUMENT|NVGETOPT_ARGUMENT_IS_OPTIONAL, NULL,
      "Controls how much information is printed.  Valid values are 'errors' "
      "(print error messages), 'warnings' (print error and warning messages), "
      "and 'all' (print error, warning and other informational messages).  By "
      "default, only errors are printed." },

    { "assign", 'a', NVGETOPT_HAS_ARGUMENT, print_assign_help, NULL },

    { "query", 'q', NVGETOPT_HAS_ARGUMENT, print_query_help, NULL },

    { "terse", 't', 0, NULL,
      "When querying attribute values with the '--query' commandline option, "
      "only print the current value, rather than the more verbose description "
      "of the attribute, its valid values, and its current value." },

    { "display-device-string", 'd', 0, NULL,
      "When printing attribute values in response to the '--query' option, "
      "if the attribute value is a display device mask, print the value "
      "as a list of display devices (e.g., \"CRT-0, DFP-0\"), rather than "
      "a hexidecimal bitmask (e.g., 0x00010001)." },

    { "glxinfo", 'g', 0, NULL,
      "Print GLX Information for the X display and exit." },
    
    { "describe", 'e', NVGETOPT_HAS_ARGUMENT, NULL,
      "Prints information about a particular attribute.  Specify 'all' to "
      "list the descriptions of all attributes.  Specify 'list' to list the "
      "attribute names without a descriptions." },

    { NULL,               0, 0, 0                   },
};



/*
 * print_assign_help() - print help information for the assign option.
 */

static void print_assign_help(void)
{
    nv_msg(BIGTAB, "The ASSIGN argument to the '--assign' commandline option "
           "is of the form:");
    
    nv_msg(NULL, "");
    
    nv_msg(BIGTAB TAB, "{DISPLAY}/{attribute name}[{display devices}]"
           "={value}");
    
    nv_msg(NULL, "");

    nv_msg(BIGTAB, "This assigns the attribute {attribute name} to the value "
           "{value} on the X Display {DISPLAY}.  {DISPLAY} follows the usual "
           "{host}:{display}.{screen} syntax of the DISPLAY environment "
           "variable and is optional; when it is not specified, then it is "
           "implied following the same rule as the --ctrl-display option.  "
           "If the X screen is not specified, then the assignment is made to "
           "all X screens.  Note that the '/' is only required when {DISPLAY} "
           "is present.");

    nv_msg(NULL, "");

    nv_msg(BIGTAB, "{DISPLAY} can additionally include a target "
           "specification to direct an assignment to something other than "
           "an X screen.  A target specification is contained within brackets "
           "and consists of a target type name, a colon, and the "
           "target id.  The target type name can be one of \"screen\", "
           "\"gpu\", \"framelock\", \"vcs\", \"gvi\", or \"fan\"; the target "
           "id is the index into the "
           "list of targets (for that target type).  The target specification "
           "can be used in {DISPLAY} wherever an X screen can be used, "
           "following the syntax {host}:{display}[{target_type}:"
           "{target_id}].  See the output of `nvidia-settings -q all` for "
           "information on which target types can be used with which "
           "attributes.  See the output of `nvidia-settings -q screens "
           "-q gpus -q framelocks -q vcs -q gvis -q fans` for lists of targets "
           "for each target type.");
    
    nv_msg(NULL, "");

    nv_msg(BIGTAB, "The [{display devices}] portion is also optional; "
           "if it is not specified, then the attribute is assigned to all "
           "display devices.");

    nv_msg(NULL, "");
 
    nv_msg(BIGTAB, "Some examples:");
    
    nv_msg(NULL, "");
    
    nv_msg(BIGTAB TAB, "-a FSAA=5");
    nv_msg(BIGTAB TAB, "-a localhost:0.0/DigitalVibrance[CRT-0]=0");
    nv_msg(BIGTAB TAB, "--assign=\"SyncToVBlank=1\"");
    nv_msg(BIGTAB TAB, "-a [gpu:0]/DigitalVibrance[DFP-1]=63");
    
} /* print_assign_help() */



/*
 * print_query_help() - print help information for the query option.
 */

static void print_query_help(void)
{
    nv_msg(BIGTAB, "The QUERY argument to the '--query' commandline option "
           "is of the form:");

    nv_msg(NULL, "");

    nv_msg(BIGTAB TAB, "{DISPLAY}/{attribute name}[{display devices}]");
    
    nv_msg(NULL, "");

    nv_msg(BIGTAB, "This queries the current value of the attribute "
           "{attribute name} on the X Display {DISPLAY}.  The format is "
           "the same as that for the '--assign' option, without "
           "'={value}'.  Specify '-q screens', '-q gpus', '-q framelocks', "
           "'-q vcs', '-q gvis', or '-q fans' to query a list of X screens, "
           "GPUs, Frame Lock devices, Visual Computing Systems, SDI Input "
           "Devices, or Fans, respectively, that are present on the X Display "
           "{DISPLAY}.  Specify '-q all' to query all attributes.");

} /* print_query_help() */



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
        if (o->description) nv_msg(BIGTAB, o->description);
        if (o->print_description) (*(o->print_description))();
        nv_msg(NULL, "");
        free(msg);
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
        c = nvgetopt(argc, argv, __options, &strval, NULL);

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

static char *tilde_expansion(char *str)
{
    char *prefix = NULL;
    char *replace, *user, *ret;
    struct passwd *pw;
    int len;

    if ((!str) || (str[0] != '~')) return str;
    
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

    if (!prefix) return str;
    
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
