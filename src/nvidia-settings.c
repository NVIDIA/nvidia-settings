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
    char *dpy = NULL;
    int gui = 0;

    /*
     * initialize the ui
     *
     * XXX it would be nice if we didn't do this up front, since we
     * may not even use the gui, but we want the toolkit to have a
     * chance to parse the commandline before we do...
     *
     * gui flag used to decide if ctk should be used or not, as
     * the user might just use control the display from a remote console
     * but for some reason cannot initialize the gtk gui. - TY 2005-05-27
     */

    if (ctk_init_check(&argc, &argv)) {
        dpy = ctk_get_display();
        gui = 1;
    }
    
    /* parse the commandline */
    
    op = parse_command_line(argc, argv, dpy);

    /* quit here if we don't have a ctrl_display - TY 2005-05-27 */

    if (op->ctrl_display == NULL) {
        nv_error_msg("The control display is undefined; please run "
                     "`%s --help` for usage information.\n", argv[0]);
        return 1;
    }

    /* process any query or assignment commandline options */

    if (op->num_assignments || op->num_queries) {
        ret = nv_process_assignments_and_queries(op);
        return ret ? 0 : 1;
    }
    
    /* initialize the parsed attribute list */

    p = nv_parsed_attribute_init();

    /* initialize the ConfigProperties */

    init_config_properties(&conf);

    /*
     * Rewrite the X server settings to configuration file
     * and exit, without starting a Graphical User Interface.
     */

    if (op->rewrite) {
        nv_parsed_attribute_clean(p);
        h = nv_alloc_ctrl_handles(op->ctrl_display);
        if(!h || !h->dpy) return 1;
        ret = nv_write_config_file(op->config, h, p, &conf);
        nv_free_ctrl_handles(h);
        nv_parsed_attribute_free(p);
        free(op);
        op = NULL;
        return ret ? 0 : 1;
    }

    /* upload the data from the config file */
    
    if (!op->no_load) {
        ret = nv_read_config_file(op->config, op->ctrl_display, p, &conf);
    } else {
        ret = 1;
    }

    /*
     * if the user requested that we only load the config file, then
     * exit now
     */
    
    if (op->only_load) {
        return ret ? 0 : 1;
    }

    /*
     * past this point, we need to be able to create a gui; fail if
     * the gui isn't available; TY 2005-05-27
     */

    if (gui == 0) {
        nv_error_msg("Unable to create nvidia-settings GUI; please run "
                     "`%s --help` for usage information.\n", argv[0]);
        return 1;
    }

    /* allocate the CtrlHandles for this X screen */

    h = nv_alloc_ctrl_handles(op->ctrl_display);
    
    if (!h || !h->dpy) {
        return 1;
    }

    /* pass control to the gui */

    ctk_main(p, &conf, h, op->page);
    
    /* write the configuration file */

    nv_write_config_file(op->config, h, p, &conf);

    /* cleanup */

    nv_free_ctrl_handles(h);
    nv_parsed_attribute_free(p);

    return 0;
    
} /* main() */
