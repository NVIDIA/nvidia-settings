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

/*
 * nvgetopt.c - portable getopt_long() replacement; removes the need
 * for the stupid optstring argument.  Also adds support for the
 * "-feature"/"+feature" syntax.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nvgetopt.h"



/*
 * nvgetopt() - see the glibc getopt_long(3) manpage for usage
 * description.  Options can be prepended with any of "--", "-", or
 * "+".
 *
 * A global variable stores the current index into the argv array, so
 * subsequent calls to nvgetopt() will advance through argv[].
 *
 * On success, the matching NVGetoptOption.val is returned.
 *
 * On failure, an error is printed to stderr, and 0 is returned.
 *
 * When there are no more options to parse, -1 is returned.
 */

int nvgetopt(int argc, char *argv[], const NVGetoptOption *options,
             char **strval, int *boolval)
{
    char *c, *a, *arg, *name = NULL, *argument=NULL;
    int i, found = NVGETOPT_FALSE, ret = 0, val = NVGETOPT_FALSE;
    const NVGetoptOption *o = NULL;
    static int argv_index = 0;
    
    argv_index++;

    /* if no more options, return -1 */

    if (argv_index >= argc) return -1;
    
    /* get the argument in question */

    arg = strdup(argv[argv_index]);
    
    /* look for "--", "-", or "+" */
    
    if ((arg[0] == '-') && (arg[1] == '-')) {
        name = arg + 2;
        val = NVGETOPT_INVALID;
    } else if (arg[0] == '-') {
        name = arg + 1;
        val = NVGETOPT_FALSE;
    } else if (arg[0] == '+') {
        name = arg + 1;
        val = NVGETOPT_TRUE;
    } else {
        fprintf(stderr, "%s: invalid option: \"%s\"\n", argv[0], arg);
        goto done;
    }

    /*
     * if there is an "=" in the string, then assign argument and zero
     * out the equal sign so that name will match what is in the
     * option table.
     */

    c = name;
    while (*c) {
        if (*c == '=') { argument = c + 1; *c = '\0'; break; }
        c++;
    }
    
    /*
     * if the string is terminated after one character, interpret it
     * as a short option.  Otherwise, interpret it as a long option.
     */

    if (name[1] == '\0') { /* short option */
        for (i = 0; options[i].name; i++) {
            if (options[i].val == name[0]) {
                o = &options[i];
                break;
            }
        }
    } else { /* long option */
        for (i = 0; options[i].name; i++) {
            if (strcmp(options[i].name, name) == 0) {
                o = &options[i];
                break;
            }
        }
    }

    /*
     * if we didn't find a match, maybe this is multiple short options
     * stored together; is each character a short option?
     */

    if (!o) {
        for (c = name; *c; c++) {
            found = NVGETOPT_FALSE;
            for (i = 0; options[i].name; i++) {
                if (options[i].val == *c) {
                    found = NVGETOPT_TRUE;
                    break;
                }
            }
            if (!found) break;
        }
        
        if (found) {

            /*
             * all characters individually are short options, so
             * interpret them that way
             */

            for (i = 0; options[i].name; i++) {
                if (options[i].val == name[0]) {
                    
                    /*
                     * don't allow options with arguments to be
                     * processed in this way
                     */
                    
                    if (options[i].flags & NVGETOPT_HAS_ARGUMENT) break;
                    
                    /*
                     * remove the first short option from
                     * argv[argv_index]
                     */
                    
                    a = argv[argv_index];
                    if (a[0] == '-') a++;
                    if (a[0] == '-') a++;
                    if (a[0] == '+') a++;
                    
                    while(a[0]) { a[0] = a[1]; a++; }
                    
                    /*
                     * decrement argv_index so that we process this
                     * entry again
                     */
                    
                    argv_index--;
                    
                    o = &options[i];
                    break;
                }
            }
        }
    }
    
    /* if we didn't find an option, return */

    if (!o) {
        fprintf(stderr, "%s: unrecognized option: \"%s\"\n", argv[0], arg);
        goto done;
    }

    if (o->flags & NVGETOPT_IS_BOOLEAN) {

        /*
         * if this option is boolean, then make sure it wasn't
         * prepended with "--"
         */

        if (val == NVGETOPT_INVALID) {
            fprintf(stderr, "%s: incorrect usage: \"%s\".  The option \"%s\" "
                    "should be prepended with either one \"-\" (to disable "
                    "%s) or one \"+\" (to enable %s)\n",
                    argv[0], arg, o->name, o->name, o->name);
            goto done;
        }

        /* assign boolval */
        
        if (boolval) *boolval = val;
    }

    /*
     * if this option takes an argument, then we either need to use
     * what was after the "=" in this argv[] entry, ot we need to pull
     * the next entry off of argv[]
     */

    if (o->flags & NVGETOPT_HAS_ARGUMENT) {
        if (argument) {
            if (!argument[0]) {
                fprintf(stderr, "%s: option \"%s\" requires an argument.\n",
                        argv[0], arg);
                goto done;
            }
            if (strval) *strval = strdup(argument);
        } else {
            argv_index++;
            if (argv_index >= argc) {
                fprintf(stderr, "%s: option \"%s\" requires an argument.\n",
                        argv[0], arg);
                goto done;
            }
            if (strval) *strval = argv[argv_index];
        } 
    }
    
    ret = o->val;

 done:

    free(arg);
    return ret;

} /* nvgetopt() */
