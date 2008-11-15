/*
 * Copyright 2004 Red Hat Inc., Raleigh, North Carolina.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL RED HAT AND/OR THEIR SUPPLIERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <kem@redhat.com>
 *
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static XConfigSymTabRec ExtensionsTab[] =
{
    {ENDSECTION, "endsection"},
    {OPTION,     "option"},
    {-1,         ""},
};

#define CLEANUP xconfigFreeExtensions

XConfigExtensionsPtr
xconfigParseExtensionsSection (void)
{
    int token;
    
    PARSE_PROLOGUE (XConfigExtensionsPtr, XConfigExtensionsRec);

    while ((token = xconfigGetToken (ExtensionsTab)) != ENDSECTION) {
        switch (token) {
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        default:
            Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
            break;
        }
    }

    return ptr;
}

#undef CLEANUP

void
xconfigPrintExtensionsSection (FILE * cf, XConfigExtensionsPtr ptr)
{
    XConfigOptionPtr p;

    if (ptr == NULL || ptr->options == NULL)
        return;

    p = ptr->options;
    fprintf (cf, "Section \"Extensions\"\n");
    if (ptr->comment) fprintf (cf, "%s", ptr->comment);
    xconfigPrintOptionList(cf, p, 1);
    fprintf (cf, "EndSection\n\n");
}

void
xconfigFreeExtensions (XConfigExtensionsPtr *ptr)
{
    if (ptr == NULL || *ptr == NULL)
        return;

    xconfigFreeOptionList (&((*ptr)->options));
    TEST_FREE ((*ptr)->comment);
    free (*ptr);
    *ptr = NULL;
}
