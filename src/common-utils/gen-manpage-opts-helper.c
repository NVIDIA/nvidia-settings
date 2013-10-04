/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "nvgetopt.h"
#include "gen-manpage-opts-helper.h"

static void print_option(const NVGetoptOption *o)
{
    char scratch[64], *s;
    int j, len;

    int italics, bold, omitWhiteSpace;

    /* if we are going to need the argument, process it now */
    if (o->flags & NVGETOPT_HAS_ARGUMENT) {
        if (o->arg_name) {
            strcpy(scratch, o->arg_name);
        } else {
            len = strlen(o->name);
            for (j = 0; j < len; j++) scratch[j] = toupper(o->name[j]);
            scratch[len] = '\0';
        }
    }

    printf(".TP\n.BI \"");
    /* Print the name of the option */
    /* XXX We should backslashify the '-' characters in o->name. */

    if (isalpha(o->val)) {
        /* '\-c' */
        printf("\\-%c", o->val);

        if (o->flags & NVGETOPT_HAS_ARGUMENT) {
            /* ' " "ARG" "' */
            printf(" \" \"%s\" \"", scratch);
        }
        /* ', ' */
        printf(", ");
    }

    /* '\-\-name' */
    printf("\\-\\-%s", o->name);

    /* '=" "ARG' */
    if (o->flags & NVGETOPT_HAS_ARGUMENT) {
        printf("=\" \"%s", scratch);

        /* '" "' */
        if ((o->flags & NVGETOPT_IS_BOOLEAN) ||
            (o->flags & NVGETOPT_ALLOW_DISABLE)) {
            printf("\" \"");
        }
    }

    /* ', \-\-no\-name' */
    if (((o->flags & NVGETOPT_IS_BOOLEAN) &&
         !(o->flags & NVGETOPT_HAS_ARGUMENT)) ||
        (o->flags & NVGETOPT_ALLOW_DISABLE)) {
        printf(", \\-\\-no\\-%s", o->name);
    }

    printf("\"\n");

    /* Print the option description */
    /* XXX Each sentence should be on its own line! */

    /*
     * Print the option description:  write each character one at a
     * time (ugh) so that we can special-case a few characters:
     *
     * '&' : toggles italics on and off
     * '^' : toggles bold on and off
     * '-' : is backslashified: "\-"
     *
     * Whitespace is omitted when italics or bold is on
     */

    italics = 0;
    bold = 0;
    omitWhiteSpace = 0;

    for (s = o->description; s && *s; s++) {

        switch (*s) {
          case '&':
              if (italics) {
                  printf("\n");
              } else {
                  printf("\n.I ");
              }
              omitWhiteSpace = italics;
              italics = !italics;
              break;
          case '^':
              if (bold) {
                  printf("\n");
              } else {
                  printf("\n.B ");
              }
              omitWhiteSpace = bold;
              bold = !bold;
              break;
          case '-':
              printf("\\-");
              omitWhiteSpace = 0;
              break;
          case ' ':
              if (!omitWhiteSpace) {
                  printf(" ");
              }
              break;
          default:
              printf("%c", *s);
              omitWhiteSpace = 0;
              break;
        }
    }

    printf("\n");
}

void gen_manpage_opts_helper(const NVGetoptOption *options)
{
    int i;
    int has_advanced_options = 0;

    /* Print the "simple" options; i.e. the ones you get with --help. */
    printf(".SH OPTIONS\n");
    for (i = 0; options[i].name; i++) {
        const NVGetoptOption *o = &options[i];

        if (!o->description) {
            continue;
        }

        if (!(o->flags & NVGETOPT_HELP_ALWAYS)) {
            has_advanced_options = 1;
            continue;
        }

        print_option(o);
    }

    if (has_advanced_options) {
        /*
         * If any exist, print the advanced options; i.e., the ones
         * you get with --advanced-help
         */
        printf(".SH \"ADVANCED OPTIONS\"\n");
        for (i = 0; options[i].name; i++) {
            const NVGetoptOption *o = &options[i];

            if (!o->description) {
                continue;
            }

            if (o->flags & NVGETOPT_HELP_ALWAYS) {
                continue;
            }

            print_option(o);
        }
    }
}
