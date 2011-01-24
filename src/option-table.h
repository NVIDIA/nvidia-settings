/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#ifndef __OPTION_TABLE_H__
#define __OPTION_TABLE_H__

#include "nvgetopt.h"
#include "command-line.h"

#define TAB    "  "
#define BIGTAB "      "

/*
 * Options table; see nvgetopt.h for a description of the fields, and
 * gen-manpage-opts.c:print_option() for a description of special
 * characters that are converted during manpage generation.
 */

static const NVGetoptOption __options[] = {
    { "version", 'v', 0, NULL,
      "Print the <nvidia-settings> version and exit." },

    { "help", 'h', 0, NULL,
      "Print usage information and exit." },

    { "config", CONFIG_FILE_OPTION, NVGETOPT_STRING_ARGUMENT, NULL,
      "Use the configuration file ^CONFIG> rather than the "
      "default ^" DEFAULT_RC_FILE ">" },

    { "ctrl-display", 'c', NVGETOPT_STRING_ARGUMENT, NULL,
      "Control the specified X display.  If this option is not given, then "
      "<nvidia-settings> will control the display specified by <'--display'>; "
      "if that is not given, then the ^$DISPLAY> environment variable is used." },

    { "load-config-only", 'l', 0, NULL,
      "Load the configuration file, send the values specified therein to "
      "the X server, and exit.  This mode of operation is useful to place "
      "in your xinitrc file, for example." },

    { "no-config", 'n', 0, NULL,
      "Do not load the configuration file.  This mode of operation is useful "
      "if <nvidia-settings> has difficulties starting due to problems with "
      "applying settings in the configuration file." },

    { "rewrite-config-file", 'r', 0, NULL,
      "Write the X server configuration to the configuration file, and exit, "
      "without starting the graphical user interface.  See EXAMPLES section." },

    { "verbose", 'V',
      NVGETOPT_STRING_ARGUMENT | NVGETOPT_ARGUMENT_IS_OPTIONAL, NULL,
      "Controls how much information is printed.  By default, the verbosity "
      "is <errors> and only error messages are printed.  Valid values are "
      "<'errors'> (print error messages), <'warnings'> (print error and "
      "warning messages), and <'all'> (print error, warning and other "
      "informational messages)." },

    { "assign", 'a', NVGETOPT_STRING_ARGUMENT, NULL,
      "The ^ASSIGN> argument to the <'--assign'> command line option is of the "
      "form:\n"
      "\n"
      TAB "{DISPLAY}/{attribute name}[{display devices}]={value}\n"
      "\n"
      "This assigns the attribute {attribute name} to the value {value} on the "
      "X Display {DISPLAY}.  {DISPLAY} follows the usual {host}:{display}."
      "{screen} syntax of the DISPLAY environment variable and is optional; "
      "when it is not specified, then it is implied following the same rule as "
      "the <--ctrl-display> option.  If the X screen is not specified, then the "
      "assignment is made to all X screens.  Note that the '/' is only required "
      "when {DISPLAY} is present.\n"
      "\n"
      "{DISPLAY} can additionally include a target specification to direct "
      "an assignment to something other than an X screen.  A target "
      "specification is contained within brackets and consists of a target "
      "type name, a colon, and the target id.  The target type name can be "
      "one of <\"screen\", \"gpu\", \"framelock\", \"vcs\", \"gvi\",> or "
      "<\"fan\";> the target id is the index into the list of targets "
      "(for that target type).  The target specification can be used in "
      "{DISPLAY} wherever an X screen can be used, following the syntax "
      "{host}:{display}[{target_type}:{target_id}].  See the output of\n"
      "\n"
      TAB "nvidia-settings -q all \n"
      "\n"
      "for information on which target types can be used with which "
      "attributes.  See the output of\n"
      "\n"
      TAB " nvidia-settings -q screens -q gpus -q framelocks -q vcs -q gvis "
      "-q fans \n"
      "\n"
      "for lists of targets for each target type.\n"
      "\n"
      "The [{display devices}] portion is also optional; if it is not "
      "specified, then the attribute is assigned to all display devices.\n"
      "\n"
      "Some examples:\n"
      "\n"
      TAB "-a FSAA=5\n"
      TAB "-a localhost:0.0/DigitalVibrance[CRT-0]=0\n"
      TAB "--assign=\"SyncToVBlank=1\"\n"
      TAB "-a [gpu:0]/DigitalVibrance[DFP-1]=63\n" },

    { "query", 'q', NVGETOPT_STRING_ARGUMENT, NULL,
      "The ^QUERY> argument to the <'--query'> command line option is of the "
      "form:\n"
      "\n"
      TAB "{DISPLAY}/{attribute name}[{display devices}]\n"
      "\n"
      "This queries the current value of the attribute {attribute name} on the "
      "X Display {DISPLAY}.  The syntax is the same as that for the "
      "<'--assign'> option, without '=<{value}'>; specify <'-q screens', "
      "'-q gpus', '-q framelocks', '-q vcs', '-q gvis', or '-q fans'> to "
      "query a list of X screens, GPUs, Frame Lock devices, Visual Computing "
      "Systems, SDI Input Devices, or Fans, respectively, that are present "
      "on the X Display {DISPLAY}.  Specify <'-q all'> to query all attributes." },

    { "terse", 't', 0, NULL,
      "When querying attribute values with the '--query' command line option, "
      "only print the current value, rather than the more verbose description "
      "of the attribute, its valid values, and its current value." },

    { "display-device-string", 'd', 0, NULL,
      "When printing attribute values in response to the '--query' option, "
      "if the attribute value is a display device mask, print the value "
      "as a list of display devices (e.g., \"CRT-0, DFP-0\"), rather than "
      "a hexadecimal bit mask (e.g., 0x00010001)." },

    { "glxinfo", 'g', 0, NULL,
      "Print GLX Information for the X display and exit." },

    { "describe", 'e', NVGETOPT_STRING_ARGUMENT, NULL,
      "Prints information about a particular attribute.  Specify 'all' to "
      "list the descriptions of all attributes.  Specify 'list' to list the "
      "attribute names without a descriptions." },

    { "page", 'p', NVGETOPT_STRING_ARGUMENT, NULL,
      "The ^PAGE> argument to the <'--page'> commandline option selects a "
      "particular page in the nvidia-settings user interface to display "
      "upon starting nvidia-settings.  Valid values are the page names "
      "in the tree view on the left side of the nvidia-settings user "
      "interface; e.g.,\n"
      "\n"
      TAB "--page=\"X Screen 0\"\n"
      "\n"
      "Because some page names are not unique (e.g., a \"PowerMizer\" page is "
      "present under each GPU), the page name can optionally be prepended "
      "with the name of the parent X Screen or GPU page, followed by a comma.  "
      "E.g.,\n"
      "\n"
      TAB "--page=\"GPU 0 - (Quadro 6000), PowerMizer\"\n"
      "\n"
      "The first page with a name matching the ^PAGE> argument will be used.  "
      "By default, the \"X Server Information\" page is displayed." },

    { NULL, 0, 0, NULL, NULL},
};

#endif //_OPTION_TABLE_H
