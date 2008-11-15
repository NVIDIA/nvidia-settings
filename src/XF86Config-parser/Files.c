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

#include <strings.h>

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static XConfigSymTabRec FilesTab[] =
{
    {ENDSECTION, "endsection"},
    {FONTPATH, "fontpath"},
    {RGBPATH, "rgbpath"},
    {MODULEPATH, "modulepath"},
    {INPUTDEVICES, "inputdevices"},
    {LOGFILEPATH, "logfile"},
    {-1, ""},
};

static char *
prependRoot (char *pathname)
{
    return pathname;
}

#define CLEANUP xconfigFreeFiles

XConfigFilesPtr
xconfigParseFilesSection (void)
{
    int i, j;
    int k, l;
    char *str;
    int token;
    PARSE_PROLOGUE (XConfigFilesPtr, XConfigFilesRec)

    while ((token = xconfigGetToken (FilesTab)) != ENDSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case FONTPATH:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "FontPath");
            j = FALSE;
            str = prependRoot (val.str);
            if (ptr->fontpath == NULL)
            {
                ptr->fontpath = malloc (1);
                ptr->fontpath[0] = '\0';
                i = strlen (str) + 1;
            }
            else
            {
                i = strlen (ptr->fontpath) + strlen (str) + 1;
                if (ptr->fontpath[strlen (ptr->fontpath) - 1] != ',')
                {
                    i++;
                    j = TRUE;
                }
            }
            ptr->fontpath = realloc (ptr->fontpath, i);
            if (j)
                strcat (ptr->fontpath, ",");

            strcat (ptr->fontpath, str);
            free (val.str);
            break;
        case RGBPATH:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "RGBPath");
            ptr->rgbpath = val.str;
            break;
        case MODULEPATH:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "ModulePath");
            l = FALSE;
            str = prependRoot (val.str);
            if (ptr->modulepath == NULL)
            {
                ptr->modulepath = malloc (1);
                ptr->modulepath[0] = '\0';
                k = strlen (str) + 1;
            }
            else
            {
                k = strlen (ptr->modulepath) + strlen (str) + 1;
                if (ptr->modulepath[strlen (ptr->modulepath) - 1] != ',')
                {
                    k++;
                    l = TRUE;
                }
            }
            ptr->modulepath = realloc (ptr->modulepath, k);
            if (l)
                strcat (ptr->modulepath, ",");

            strcat (ptr->modulepath, str);
            free (val.str);
            break;
        case INPUTDEVICES:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "InputDevices");
            l = FALSE;
            str = prependRoot (val.str);
            if (ptr->inputdevs == NULL)
            {
                ptr->inputdevs = malloc (1);
                ptr->inputdevs[0] = '\0';
                k = strlen (str) + 1;
            }
            else
            {
                k = strlen (ptr->inputdevs) + strlen (str) + 1;
                if (ptr->inputdevs[strlen (ptr->inputdevs) - 1] != ',')
                {
                    k++;
                    l = TRUE;
                }
            }
            ptr->inputdevs = realloc (ptr->inputdevs, k);
            if (l)
                strcat (ptr->inputdevs, ",");

            strcat (ptr->inputdevs, str);
            free (val.str);
            break;
        case LOGFILEPATH:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "LogFile");
            ptr->logfile = val.str;
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
xconfigPrintFileSection (FILE * cf, XConfigFilesPtr ptr)
{
    char *p, *s;

    if (ptr == NULL)
        return;

    if (ptr->comment)
        fprintf (cf, "%s", ptr->comment);
    if (ptr->logfile)
        fprintf (cf, "    LogFile         \"%s\"\n", ptr->logfile);
    if (ptr->rgbpath)
        fprintf (cf, "    RgbPath         \"%s\"\n", ptr->rgbpath);
    if (ptr->modulepath)
    {
        s = ptr->modulepath;
        p = index (s, ',');
        while (p)
        {
            *p = '\000';
            fprintf (cf, "    ModulePath      \"%s\"\n", s);
            *p = ',';
            s = p;
            s++;
            p = index (s, ',');
        }
        fprintf (cf, "    ModulePath      \"%s\"\n", s);
    }
    if (ptr->inputdevs)
    {
        s = ptr->inputdevs;
        p = index (s, ',');
        while (p)
        {
            *p = '\000';
            fprintf (cf, "    InputDevices      \"%s\"\n", s);
            *p = ',';
            s = p;
            s++;
            p = index (s, ',');
        }
        fprintf (cf, "    InputDevices      \"%s\"\n", s);
    }
    if (ptr->fontpath)
    {
        s = ptr->fontpath;
        p = index (s, ',');
        while (p)
        {
            *p = '\000';
            fprintf (cf, "    FontPath        \"%s\"\n", s);
            *p = ',';
            s = p;
            s++;
            p = index (s, ',');
        }
        fprintf (cf, "    FontPath        \"%s\"\n", s);
    }
}

void
xconfigFreeFiles (XConfigFilesPtr *p)
{
    if (p == NULL || *p == NULL)
        return;

    TEST_FREE ((*p)->logfile);
    TEST_FREE ((*p)->rgbpath);
    TEST_FREE ((*p)->modulepath);
    TEST_FREE ((*p)->inputdevs);
    TEST_FREE ((*p)->fontpath);
    TEST_FREE ((*p)->comment);

    free (*p);
    *p = NULL;
}
