/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */


/* View/edit this file with tab stops set to 4 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static XConfigSymTabRec VendorSubTab[] =
{
    {ENDSUBSECTION, "endsubsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {-1, ""},
};

#define CLEANUP xconfigFreeVendorSubList

XConfigVendSubPtr
xconfigParseVendorSubSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigVendSubPtr, XConfigVendSubRec)

    while ((token = xconfigGetToken (VendorSubTab)) != ENDSUBSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case IDENTIFIER:
            if (xconfigGetSubToken (&(ptr->comment)))
                Error (QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error (MULTIPLE_MSG, "Identifier");
            ptr->identifier = val.str;
            has_ident = TRUE;
            break;
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;

        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
            break;
        }
    }

    return ptr;
}

#undef CLEANUP

static XConfigSymTabRec VendorTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {SUBSECTION, "subsection"},
    {-1, ""},
};

#define CLEANUP xconfigFreeVendorList

XConfigVendorPtr
xconfigParseVendorSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigVendorPtr, XConfigVendorRec)

    while ((token = xconfigGetToken (VendorTab)) != ENDSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case IDENTIFIER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error (MULTIPLE_MSG, "Identifier");
            ptr->identifier = val.str;
            has_ident = TRUE;
            break;
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;
        case SUBSECTION:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "SubSection");
            {
                HANDLE_LIST (subs, xconfigParseVendorSubSection,
                            XConfigVendSubPtr);
            }
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
            break;
        }

    }

    if (!has_ident)
        Error (NO_IDENT_MSG, NULL);

    return ptr;
}

#undef CLEANUP

void
xconfigPrintVendorSection (FILE * cf, XConfigVendorPtr ptr)
{
    XConfigVendSubPtr pptr;

    while (ptr)
    {
        fprintf (cf, "Section \"Vendor\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);

        xconfigPrintOptionList(cf, ptr->options, 1);
        for (pptr = ptr->subs; pptr; pptr = pptr->next)
        {
            fprintf (cf, "    SubSection \"Vendor\"\n");
            if (pptr->comment)
                fprintf (cf, "%s", pptr->comment);
            if (pptr->identifier)
                fprintf (cf, "        Identifier \"%s\"\n", pptr->identifier);
            xconfigPrintOptionList(cf, pptr->options, 2);
            fprintf (cf, "    EndSubSection\n");
        }
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigFreeVendorList (XConfigVendorPtr *p)
{
    if (p == NULL || *p == NULL)
        return;

    xconfigFreeVendorSubList (&((*p)->subs));
    TEST_FREE ((*p)->identifier);
    TEST_FREE ((*p)->comment);
    xconfigFreeOptionList (&((*p)->options));
    free (*p);
    *p = NULL;
}

void
xconfigFreeVendorSubList (XConfigVendSubPtr *ptr)
{
    XConfigVendSubPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->name);
        TEST_FREE ((*ptr)->comment);
        xconfigFreeOptionList (&((*ptr)->options));
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

XConfigVendorPtr
xconfigFindVendor (const char *name, XConfigVendorPtr list)
{
    while (list)
    {
        if (xconfigNameCompare (list->identifier, name) == 0)
            return (list);
        list = list->next;
    }
    return (NULL);
}

