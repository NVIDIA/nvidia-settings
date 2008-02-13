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

#include "NvCtrlAttributes.h"

#include "command-line.h"
#include "config-file.h"
#include "query-assign.h"
#include "msg.h"

#include "ctkui.h"

#include <stdlib.h>


int main(int argc, char **argv)
{
    ConfigProperties conf;
    ParsedAttribute *p;
    CtrlHandles *h;
    Options *op;
    int ret;
    
    /*
     * initialize the ui
     *
     * XXX it would be nice if we didn't do this up front, since we
     * may not even use the gui, but we want the toolkit to have a
     * chance to parse the commandline before we do... we should
     * investigate gtk_init_check().
     */
    
    ctk_init(&argc, &argv);
    
    /* parse the commandline */
    
    op = parse_command_line(argc, argv, ctk_get_display());

    /* process any query or assignment commandline options */

    if (op->num_assignments || op->num_queries) {
        ret = nv_process_assignments_and_queries(op);
        return ret ? 0 : 1;
    }
    
    /* initialize the parsed attribute list */

    p = nv_parsed_attribute_init();

    /* upload the data from the config file */
    
    ret = nv_read_config_file(op->config, op->ctrl_display, p, &conf);

    /*
     * if the user requested that we only load the config file, then
     * exit now
     */
    
    if (op->load) {
        return ret ? 0 : 1;
    }

    /* allocate the CtrlHandles for this X screen */

    h = nv_alloc_ctrl_handles(op->ctrl_display);
    
    if (!h || !h->dpy) {
        return 1;
    }

    /* pass control to the gui */

    ctk_main(h->h, h->num_screens, p, &conf);

    /* write the configuration file */

    nv_write_config_file(op->config, h, p, &conf);

    /* cleanup */

    nv_free_ctrl_handles(h);
    nv_parsed_attribute_free(p);

    return 0;
    
} /* main() */
