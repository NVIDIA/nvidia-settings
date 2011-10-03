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

#ifndef __COMMAND_LINE_H__
#define __COMMAND_LINE_H__

#include <NvCtrlAttributes.h>

#define DEFAULT_RC_FILE "~/.nvidia-settings-rc"
#define CONFIG_FILE_OPTION 1


#define VERBOSITY_NONE     0 /* nothing */
#define VERBOSITY_ERROR    1 /* errors only */
#define VERBOSITY_WARNING  2 /* errors and warnings */
#define VERBOSITY_ALL      3 /* errors, warnings and other info */

#define VERBOSITY_DEFAULT  VERBOSITY_ERROR

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
                          * read from, and to be writen to); defaults
                          * to the value of the constant
                          * DEFAULT_RC_FILE.
                          */

    char **assignments;  /*
                          * Dyanamically allocated array of assignment
                          * strings specified on the commandline.
                          */
    
    int num_assignments; /*
                          * Number of assignment strings in the
                          * assignment array.
                          */
    
    char **queries;      /*
                          * Dyanamically allocated array of query
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

} Options;


Options *parse_command_line(int argc, char *argv[], char *dpy);

#endif /* __COMMAND_LINE_H__ */
