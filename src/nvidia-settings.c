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
    NvCtrlAttributeHandle **screen_handles = NULL;
    NvCtrlAttributeHandle **gpu_handles = NULL;
    NvCtrlAttributeHandle **vcsc_handles = NULL;
    Options *op;
    int ret, i, num_screen_handles, num_gpu_handles, num_vcsc_handles;
    
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

    /* allocate the CtrlHandles for this X screen */

    h = nv_alloc_ctrl_handles(op->ctrl_display);
    
    if (!h || !h->dpy) {
        return 1;
    }

    /*
     * pull the screen and gpu handles out of the CtrlHandles so that we can
     * pass them to the gui
     */
    
    num_screen_handles = h->targets[X_SCREEN_TARGET].n;

    if (num_screen_handles) {
        screen_handles =
            malloc(num_screen_handles * sizeof(NvCtrlAttributeHandle *));
        if (screen_handles) {
            for (i = 0; i < num_screen_handles; i++) {
                screen_handles[i] = h->targets[X_SCREEN_TARGET].t[i].h;
            }
        } else {
            num_screen_handles = 0;
        }
    }

    num_gpu_handles = h->targets[GPU_TARGET].n;

    if (num_gpu_handles) {
        gpu_handles =
            malloc(num_gpu_handles * sizeof(NvCtrlAttributeHandle *));
        if (gpu_handles) {
            for (i = 0; i < num_gpu_handles; i++) {
                gpu_handles[i] = h->targets[GPU_TARGET].t[i].h;
            }
        } else {
            num_gpu_handles = 0;
        }
    }

    num_vcsc_handles = h->targets[VCSC_TARGET].n;

    if (num_vcsc_handles) {
        vcsc_handles =
            malloc(num_vcsc_handles * sizeof(NvCtrlAttributeHandle *));
        if (vcsc_handles) {
            for (i = 0; i < num_vcsc_handles; i++) {
                vcsc_handles[i] = h->targets[VCSC_TARGET].t[i].h;
            }
        } else {
            num_vcsc_handles = 0;
        }
    }
    
    /* pass control to the gui */

    ctk_main(screen_handles, num_screen_handles,
             gpu_handles, num_gpu_handles,
             vcsc_handles, num_vcsc_handles,
             p, &conf);
    
    /* write the configuration file */

    nv_write_config_file(op->config, h, p, &conf);

    /* cleanup */

    if (screen_handles) free(screen_handles);
    if (gpu_handles) free(gpu_handles);
    nv_free_ctrl_handles(h);
    nv_parsed_attribute_free(p);

    return 0;
    
} /* main() */
