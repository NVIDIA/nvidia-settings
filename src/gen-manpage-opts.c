/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2010 NVIDIA Corporation.
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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "nvgetopt.h"
#include "option-table.h"

#define NV_FMT_BUF_LEN 512

/*
 * Prints the option help in a form that is suitable to include in the manpage.
 */

static void print_option(const NVGetoptOption *o)
{
    int omitWhiteSpace;

    /* if we are going to need the argument, process it now */

    printf(".TP\n.BI \"");
    /* Print the name of the option */
    /* XXX We should backslashify the '-' characters in o->name. */

    if (isalpha(o->val)) {
        /* '\-c  \-\-name' */
        printf("\\-%c, \\-\\-%s", o->val, o->name);
    } else {
        printf("\\-\\-%s", o->name);
    }

    /* '=" "ARG' */
    if (o->flags & NVGETOPT_HAS_ARGUMENT) {
        int len, j;
        char tbuf[32];
        len = strlen(o->name);
        for (j = 0; j < len; j++)
            tbuf[j] = toupper(o->name[j]);
        tbuf[len] = '\0';
        printf("=\" \"%s", tbuf);
    }

    printf("\"\n");

    /* Print the option description */
    /* XXX Each sentence should be on its own line! */

    /*
     * Print the option description:  write each character one at a
     * time (ugh) so that we can special-case a few characters:
     *
     * "^" --> "\n.I "
     * "<" --> "\n.B "
     * ">" --> "\n"
     *
     * '^' is used to mark the text as underlined till it is turned off with '>'.
     * '<' is used to mark the text as bold till it is turned off with '>'.
     *
     * XXX Each sentence should be on its own line!
     */

    if (o->description) {
        char *buf = NULL, *s = NULL, *pbuf = NULL;

        buf = calloc(1, NV_FMT_BUF_LEN + strlen(o->description));
        if (!buf) {
            /* XXX There should be better message than this */
            printf("Not enough memory\n");
            return;
        }
        pbuf = buf;

        omitWhiteSpace = 0;
        for (s = o->description; s && *s; s++) {
            switch (*s) {
            case '<':
                sprintf(pbuf, "\n.B ");
                pbuf = pbuf + 4;
                omitWhiteSpace = 0;
                break;
            case '^':
                sprintf(pbuf, "\n.I ");
                pbuf = pbuf + 4;
                omitWhiteSpace = 0;
                break;
            case '>':
                *pbuf = '\n';
                pbuf++;
                omitWhiteSpace = 1;
                break;
            case ' ':
                if (!omitWhiteSpace) {
                    *pbuf = *s;
                    pbuf++;
                }
                break;
            default:
                *pbuf = *s;
                pbuf++;
                omitWhiteSpace = 0;
                break;
            }
        }
        printf("%s", buf);
        free(buf);
    }

    printf("\n");
}

int main(int argc, char* argv[])
{
    int i;
    const NVGetoptOption *o;

    printf(".SH OPTIONS\n");
    for (i = 0; __options[i].name; i++) {
        o = &__options[i];
        print_option(o);
    }

    return 0;
}
