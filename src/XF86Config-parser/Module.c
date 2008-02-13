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

static XConfigSymTabRec SubModuleTab[] =
{
    {ENDSUBSECTION, "endsubsection"},
    {OPTION, "option"},
    {-1, ""},
};

static XConfigSymTabRec ModuleTab[] =
{
    {ENDSECTION, "endsection"},
    {LOAD, "load"},
    {LOAD_DRIVER, "loaddriver"},
    {SUBSECTION, "subsection"},
    {-1, ""},
};

#define CLEANUP xconfigFreeModules

XConfigLoadPtr
xconfigParseModuleSubSection (XConfigLoadPtr head, char *name)
{
    int token;
    PARSE_PROLOGUE (XConfigLoadPtr, XConfigLoadRec)

    ptr->name = name;
    ptr->type = XCONFIG_LOAD_MODULE;
    ptr->opt  = NULL;
    ptr->next = NULL;

    while ((token = xconfigGetToken (SubModuleTab)) != ENDSUBSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case OPTION:
            ptr->opt = xconfigParseOption(ptr->opt);
            break;
        case EOF_TOKEN:
            xconfigErrorMsg(ParseErrorMsg, UNEXPECTED_EOF_MSG);
            free(ptr);
            return NULL;
        default:
            xconfigErrorMsg(ParseErrorMsg, INVALID_KEYWORD_MSG,
                         xconfigTokenString());
            free(ptr);
            return NULL;
            break;
        }

    }

    return ((XConfigLoadPtr) xconfigAddListItem ((GenericListPtr) head,
                                           (GenericListPtr) ptr));
}

XConfigModulePtr
xconfigParseModuleSection (void)
{
    int token;
    PARSE_PROLOGUE (XConfigModulePtr, XConfigModuleRec)

    while ((token = xconfigGetToken (ModuleTab)) != ENDSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case LOAD:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Load");
            ptr->loads =
                xconfigAddNewLoadDirective (ptr->loads, val.str,
                                         XCONFIG_LOAD_MODULE, NULL, TRUE);
            break;
        case LOAD_DRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "LoadDriver");
            ptr->loads =
                xconfigAddNewLoadDirective (ptr->loads, val.str,
                                         XCONFIG_LOAD_DRIVER, NULL, TRUE);
            break;
        case SUBSECTION:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                        Error (QUOTE_MSG, "SubSection");
            ptr->loads =
                xconfigParseModuleSubSection (ptr->loads, val.str);
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

void
xconfigPrintModuleSection (FILE * cf, XConfigModulePtr ptr)
{
    XConfigLoadPtr lptr;

    if (ptr == NULL)
        return;

    if (ptr->comment)
        fprintf(cf, "%s", ptr->comment);
    for (lptr = ptr->loads; lptr; lptr = lptr->next)
    {
        switch (lptr->type)
        {
        case XCONFIG_LOAD_MODULE:
            if( lptr->opt == NULL ) {
                fprintf (cf, "    Load           \"%s\"", lptr->name);
                if (lptr->comment)
                    fprintf(cf, "%s", lptr->comment);
                else
                    fputc('\n', cf);
            }
            else
            {
                fprintf (cf, "    SubSection     \"%s\"\n", lptr->name);
                if (lptr->comment)
                    fprintf(cf, "%s", lptr->comment);
                xconfigPrintOptionList(cf, lptr->opt, 2);
                fprintf (cf, "    EndSubSection\n");
            }
            break;
        case XCONFIG_LOAD_DRIVER:
            fprintf (cf, "    LoadDriver     \"%s\"", lptr->name);
                if (lptr->comment)
                    fprintf(cf, "%s", lptr->comment);
                else
                    fputc('\n', cf);
            break;
#if 0
        default:
            fprintf (cf, "#    Unknown type  \"%s\"\n", lptr->name);
            break;
#endif
        }
    }
}

XConfigLoadPtr
xconfigAddNewLoadDirective (XConfigLoadPtr head, char *name, int type,
                            XConfigOptionPtr opts, int do_token)
{
    XConfigLoadPtr new;
    int token;

    new = calloc (1, sizeof (XConfigLoadRec));
    new->name = name;
    new->type = type;
    new->opt  = opts;
    new->next = NULL;

    if (do_token) {
        if ((token = xconfigGetToken(NULL)) == COMMENT)
            new->comment = xconfigAddComment(new->comment, val.str);
        else
            xconfigUnGetToken(token);
    }

    return ((XConfigLoadPtr) xconfigAddListItem ((GenericListPtr) head,
                                           (GenericListPtr) new));
}

XConfigLoadPtr
xconfigRemoveLoadDirective(XConfigLoadPtr head, XConfigLoadPtr load)
{
    XConfigLoadPtr prev = NULL;
    XConfigLoadPtr l = head;

    while (l) {
        if (l == load) {
            if (prev) prev->next = load->next;
            if (head == load) head = load->next;
            TEST_FREE(load->name);
            TEST_FREE(load->comment);
            xconfigOptionListFree(load->opt);
            free(load);
            break;
        }
        
        prev = l;
        l = l->next;
    }

    return head;
}

void
xconfigFreeModules (XConfigModulePtr ptr)
{
    XConfigLoadPtr lptr;
    XConfigLoadPtr prev;

    if (ptr == NULL)
        return;
    lptr = ptr->loads;
    while (lptr)
    {
        TEST_FREE (lptr->name);
        TEST_FREE (lptr->comment);
        prev = lptr;
        lptr = lptr->next;
        free (prev);
    }
    TEST_FREE (ptr->comment);
    free (ptr);
}
