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

#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include "common-utils.h"

/*
 * Forward declaration to break circular dependency with query-assign.h
 */
struct _CtrlSystemList;

#define CONFIG_FILE_OPTION 1
#define DISPLAY_OPTION 2

/*
 * Options structure -- stores the parameters specified on the
 * commandline.
 */

typedef struct {
    
    char *ctrl_display;  /*
                          * The name of the display to control
                          * (doesn't have to be the same as the
                          * display on which the gui is shown
                          */
    
    char *config;        /*
                          * The name of the configuration file (to be
                          * read from, and to be written to); defaults
                          * to $XDG_CONFIG_HOME/nvidia/settings-rc or
                          * ~/.nvidia-settings-rc if the latter exists.
                          */

    char **assignments;  /*
                          * Dynamically allocated array of assignment
                          * strings specified on the commandline.
                          */
    
    int num_assignments; /*
                          * Number of assignment strings in the
                          * assignment array.
                          */
    
    char **queries;      /*
                          * Dynamically allocated array of query
                          * strings specified on the commandline.
                          */
    
    int num_queries;     /*
                          * Number of query strings in the query
                          * array.
                          */
    
    int only_load;       /*
                          * If true, just read the configuration file,
                          * send the attributes to the X server, and
                          * exit.
                          */

    int no_load;         /*
                          * If true, do not load the configuration file.
                          * The attributes are not sent to the X Server.
                          */

    int rewrite;         /*
                          * If true, write the X server configuration
                          * to the configuration file and exit.
                          */

    char *page;          /*
                          * The default page to display in the GUI
                          * when started.
                          */

    int list_targets;    /*
                          * If true, list resolved targets of operations
                          * (from query/assign or rc file) and exit.
                          */

    int terse;           /*
                          * If true, output minimal information to query
                          * operations.
                          */

    int dpy_string;      /*
                          * If true, output the display device mask as a list
                          * of display device names instead of a number.
                          */

    int write_config;    /*
                          * If true, write out the configuration file on exit.
                          */

    int use_gtk2;        /*
                          * If true, use GTK+ 2 user interface library
                          */

    char *gtk_lib_path;  /*
                          * Path to the user interface library to use or to the
                          * directory containing the library to use. In the
                          * former case, the value of the use_gtk2 option is
                          * ignored.
                          */

} Options;

const char *locate_default_rc_file(void);
Options *parse_command_line(int argc, char *argv[],
                            struct _CtrlSystemList *systems);

#endif /* __COMMAND_LINE_H__ */
