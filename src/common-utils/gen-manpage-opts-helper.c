/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "nvgetopt.h"
#include "gen-manpage-opts-helper.h"
#include "common-utils.h"

static void print_option(const NVGetoptOption *o)
{
    char scratch[64], *s;
    int j, len;

    int italics, bold, omitWhiteSpace, firstchar;

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
     * '\n': resets the first character flag
     * '.' : must not be the first character of a line
     * '\'': must not be the first character of a line
     *
     * Trailing whitespace is omitted when italics or bold is on
     */

    italics = FALSE;
    bold = FALSE;
    omitWhiteSpace = FALSE;
    firstchar = TRUE;

    for (s = o->description; s && *s; s++) {

        switch (*s) {
          case '&':
              if (italics) {
                  printf("\n");
              } else {
                  printf("\n.I ");
              }
              omitWhiteSpace = italics;
              firstchar = italics;
              italics = !italics;
              break;
          case '^':
              if (bold) {
                  printf("\n");
              } else {
                  printf("\n.B ");
              }
              omitWhiteSpace = bold;
              firstchar = bold;
              bold = !bold;
              break;
          case '-':
              printf("\\-");
              omitWhiteSpace = FALSE;
              firstchar = FALSE;
              break;
          case ' ':
              if (!omitWhiteSpace) {
                  printf(" ");
                  firstchar = FALSE;
              }
              break;
          case '\n':
              printf("\n");
              omitWhiteSpace = FALSE;
              firstchar = TRUE;
              break;
          case '.':
              if (firstchar) {
                  fprintf(stderr, "Error: *roff can't start a line with '.' "
                          "If you used '&' or '^' to format text in the "
                          "description of the '%s' option, please add some "
                          "text before the end of the sentence, so that a "
                          "valid manpage can be generated.\n", o->name);
                  exit(1);
              }
              /* fall through */
          case '\'':
              if (firstchar) {
                  fprintf(stderr, "Error: *roff can't start a line with '\''. "
                          "If you started a line with '\'' in the description "
                          "of the '%s' option, please add some text at the "
                          "beginning of the sentence, so that a valid manpage "
                          "can be generated.\n", o->name);
                  exit(1);
              }
              /* fall through */
          default:
              printf("%c", *s);
              omitWhiteSpace = FALSE;
              firstchar = FALSE;
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
