/* DRI.c -- DRI Section in XF86Config file
 * Created: Fri Mar 19 08:40:22 1999 by faith@precisioninsight.com
 * Revised: Thu Jun 17 16:08:05 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static XConfigSymTabRec DRITab[] =
{
    {ENDSECTION, "endsection"},
    {GROUP,      "group"},
    {BUFFERS,    "buffers"},
    {MODE,       "mode"},
    {-1,         ""},
};

#define CLEANUP xconfigFreeBuffersList

XConfigBuffersPtr
xconfigParseBuffers (void)
{
    int token;
    PARSE_PROLOGUE (XConfigBuffersPtr, XConfigBuffersRec);

    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER) {
        Error("Buffers count expected", NULL);
    }
    ptr->count = val.num;

    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER) {
        Error("Buffers size expected", NULL);
    }
    ptr->size = val.num;

    if ((token = xconfigGetSubToken (&(ptr->comment))) == STRING) {
        ptr->flags = val.str;
        if ((token = xconfigGetToken (NULL)) == COMMENT)
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
        else
            xconfigUnGetToken(token);
    }

    return ptr;
}

#undef CLEANUP
    
#define CLEANUP xconfigFreeDRI

XConfigDRIPtr
xconfigParseDRISection (void)
{
    int token;
    PARSE_PROLOGUE (XConfigDRIPtr, XConfigDRIRec);

    /* Zero is a valid value for this. */
    ptr->group = -1;
    while ((token = xconfigGetToken (DRITab)) != ENDSECTION) {
    switch (token)
        {
        case GROUP:
        if ((token = xconfigGetSubToken (&(ptr->comment))) == STRING)
            ptr->group_name = val.str;
        else if (token == NUMBER)
            ptr->group = val.num;
        else
            Error (GROUP_MSG, NULL);
        break;
        case MODE:
        if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
            Error (NUMBER_MSG, "Mode");
        ptr->mode = val.num;
        break;
        case BUFFERS:
        HANDLE_LIST (buffers, xconfigParseBuffers,
                 XConfigBuffersPtr);
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
xconfigPrintDRISection (FILE * cf, XConfigDRIPtr ptr)
{
    /* we never need the DRI section for the NVIDIA driver */

    return;
}

void
xconfigFreeDRI (XConfigDRIPtr *ptr)
{
    if (ptr == NULL || *ptr == NULL)
        return;
    
    xconfigFreeBuffersList (&((*ptr)->buffers));
    TEST_FREE ((*ptr)->comment);
    free (*ptr);
    *ptr = NULL;
}

void
xconfigFreeBuffersList (XConfigBuffersPtr *ptr)
{
    XConfigBuffersPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr) {
        TEST_FREE ((*ptr)->flags);
        TEST_FREE ((*ptr)->comment);
        prev = *ptr;
        *ptr  = (*ptr)->next;
        free (prev);
    }
}

