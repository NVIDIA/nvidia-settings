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
#include "version.h"

#include <dlfcn.h>
#include <sys/stat.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

static const char* library_names[] = {
    "libnvidia-gtk3.so." NVIDIA_VERSION,
    "libnvidia-gtk3.so",
    "libnvidia-gtk2.so." NVIDIA_VERSION,
    "libnvidia-gtk2.so",
};

typedef struct {
    void *gui_lib_handle;
    char *error_msg;

    int (*fn_ctk_init_check)(int *, char **[]);
    char *(*fn_ctk_get_display)(void);
    void (*fn_ctk_main)(ParsedAttribute *, ConfigProperties *,
                        CtrlSystem *, const char *);
} GtkLibraryData;


/*
 * get_full_library_name() - build the library name to use by selecting the
 * library name based on the index along with a possible path prefix.
 */

static char *get_full_library_name(const char *prefix, int index)
{
    int length = strlen(prefix);
    return nvstrcat(prefix,
                    (length > 0 && prefix[length-1] != '/') ? "/" : "",
                    library_names[index],
                    NULL);
}


/*
 * check_and_save_dlerror() - If an error occurs related to our dynamic loading,
 * save the error message. Since we can check for more than one library and will
 * fallback to another library name if the first is not found, we don't want to
 * print an error message unless we abort execution because of it.
 */

static int check_and_save_dlerror(char **error_msg)
{
    const char *e;
    char *new_error_msg;

    e = dlerror();
    if (e) {
        if (*error_msg) {
            new_error_msg = nvstrcat(*error_msg, "\n", e, NULL);
            free(*error_msg);
            *error_msg = new_error_msg;
        } else {
            *error_msg = nvstrdup(e);
        }
        return 1;
    }

    return 0;
}


/*
 * load_and_resolve_libdata() - Load the given shared object name and check that
 * it loads and all function pointers are resolved. If any error is detected,
 * close the object handle and save the resulting error messages.
 */

static void load_and_resolve_libdata(const char *gui_lib_name,
                                     GtkLibraryData *libdata)
{
    int symbol_error = 0;

    libdata->gui_lib_handle = dlopen(gui_lib_name, RTLD_NOW);

    if (libdata->gui_lib_handle) {
        dlerror(); // clear error msg

        libdata->fn_ctk_init_check = dlsym(libdata->gui_lib_handle,
                                           "ctk_init_check");
        symbol_error += check_and_save_dlerror(&libdata->error_msg);

        libdata->fn_ctk_get_display = dlsym(libdata->gui_lib_handle,
                                            "ctk_get_display");
        symbol_error += check_and_save_dlerror(&libdata->error_msg);

        libdata->fn_ctk_main = dlsym(libdata->gui_lib_handle, "ctk_main");
        symbol_error += check_and_save_dlerror(&libdata->error_msg);

        if (symbol_error > 0 ||
            libdata->fn_ctk_init_check == NULL ||
            libdata->fn_ctk_get_display == NULL ||
            libdata->fn_ctk_main == NULL) {

            dlclose(libdata->gui_lib_handle);
            libdata->gui_lib_handle = NULL;
        }
    } else {
        check_and_save_dlerror(&libdata->error_msg);
    }
}


/*
 * remove_flag_from_command_line() - Remove the "--" option and reindexes argv
 * and reindex the command line options so that gtk_init_check() can process its
 * options. If that flag is not found, no change to argv or argc is made.
 */

static void remove_flag_from_command_line(int *argc, char ***argv)
{
    int i, j;

    for (i=0, j=0; i<(*argc); i++) {
        if (i > j) {
            (*argv)[j] = (*argv)[i];
            j++;
        } else if (strcmp("--", (*argv)[i]) != 0) {
            j++;
        }
    }

    (*argc) = j;
    (*argv)[(*argc)] = NULL;
}


/*
 * load_ui_library() - Decide whether we need to build a library name or use one
 * already specified. If we build the name, iterate over our possible name
 * options and either open the library or return a NULL pointer on failure.
 */

static void *load_ui_library(GtkLibraryData *libdata, Options *op)
{
    struct stat buf;
    char *gui_lib_name;
    int index = 0;
    int max_index = ARRAY_LEN(library_names);

    /*
     * If the user did not specify a path or the path specified is a directory,
     * we must attempt to open our default library names.
     */

    if (op->gtk_lib_path != NULL &&
        (stat(op->gtk_lib_path, &buf) == 0 && !S_ISDIR(buf.st_mode))) {

        /* user specified a non-directory file, try to dlopen it and return */

        gui_lib_name = op->gtk_lib_path;

        load_and_resolve_libdata(gui_lib_name, libdata);

    } else {

        while (!libdata->gui_lib_handle && index < max_index) {

            /*
             * If we fail to open the default library, work down the list of
             * known library names and versions.
             */

            gui_lib_name =
                get_full_library_name(op->gtk_lib_path ?
                                      op->gtk_lib_path : "", index);

            index++;

            if (op->use_gtk2 && strstr(gui_lib_name, "gtk2") == NULL) {
                continue;
            }

            load_and_resolve_libdata(gui_lib_name, libdata);
            nvfree(gui_lib_name);
        }
    }

    return libdata->gui_lib_handle;

}



/*
 * main() - nvidia-settings application start
 */

int main(int argc, char **argv)
{
    ConfigProperties conf;
    ParsedAttribute *p;
    CtrlSystem *system;
    CtrlSystemList systems;
    Options *op;
    int ret;
    int gui = 0;

    GtkLibraryData libdata;

    libdata.gui_lib_handle = NULL;
    libdata.error_msg = NULL;

    systems.n = 0;
    systems.array = NULL;

    nv_set_verbosity(NV_VERBOSITY_DEPRECATED);

    /* parse the commandline */

    op = parse_command_line(argc, argv, &systems);

    /*
     * Using the default library names, along with a possible path or name
     * specified by the user, attempt to dlopen the appropriate user interface
     * shared object.
     */

    load_ui_library(&libdata, op);

    if (libdata.gui_lib_handle) {
        /*
         * initialize the ui
         *
         * gui flag used to decide if ctk should be used or not, as
         * the user might just use control the display from a remote console
         * but for some reason cannot initialize the gtk gui. - TY 2005-05-27
         *
         * All options intented for gtk must come after the "--" option flag.
         * Since gtk will also stop parsing options when encountering this
         * flag, we must remove it from argv before calling ctk_init_check.
         */

        remove_flag_from_command_line(&argc, &argv);

        if (libdata.fn_ctk_init_check(&argc, &argv)) {
            if (!op->ctrl_display) {
                op->ctrl_display = libdata.fn_ctk_get_display();
            }
            gui = 1;
        }
    }

    /*
     * Quit here if the dynamic load above fails.
     */

    if (!libdata.gui_lib_handle) {
        nv_error_msg("%s", libdata.error_msg);
        nv_error_msg("A problem occured when loading the GUI library. Please "
                     "check your installation and library path. You may need "
                     "to specify this library when calling nvidia-settings. "
                     "Please run `%s --help` for usage information.\n",
                     argv[0]);
        return 1;
    }

    /* quit here if we don't have a ctrl_display - TY 2005-05-27 */

    if (op->ctrl_display == NULL) {
        nv_error_msg("The control display is undefined; please run "
                     "`%s --help` for usage information.\n", argv[0]);
        return 1;
    }

    /* Allocate handle for ctrl_display */

    NvCtrlConnectToSystem(op->ctrl_display, &systems);

    /* process any query or assignment commandline options */

    if (op->num_assignments || op->num_queries) {
        ret = nv_process_assignments_and_queries(op, &systems);
        NvCtrlFreeAllSystems(&systems);
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
        system = NvCtrlGetSystem(op->ctrl_display, &systems);
        if (!system || !system->dpy) {
            return 1;
        }
        ret = nv_write_config_file(op->config, system, p, &conf);
        NvCtrlFreeAllSystems(&systems);
        nv_parsed_attribute_free(p);
        free(op);
        op = NULL;
        return ret ? 0 : 1;
    }

    /* upload the data from the config file */
    
    if (!op->no_load) {
        ret = nv_read_config_file(op, op->config, op->ctrl_display,
                                  p, &conf, &systems);
    } else {
        ret = 1;
    }

    /*
     * if the user requested that we only load the config file, or that
     * we only list the resolved targets, then exit now.
     */

    if (op->only_load || op->list_targets) {
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

    /* Get the CtrlSystem for this X screen */

    system = NvCtrlGetSystem(op->ctrl_display, &systems);

    if (!system || !system->dpy) {
        return 1;
    }

    /* pass control to the gui */

    libdata.fn_ctk_main(p, &conf, system, op->page);

    /* write the configuration file */

    if (op->write_config) {
        nv_write_config_file(op->config, system, p, &conf);
    }

    /* cleanup */

    NvCtrlFreeAllSystems(&systems);
    nv_parsed_attribute_free(p);
    dlclose(libdata.gui_lib_handle);

    return 0;

} /* main() */
