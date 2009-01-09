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

static XConfigSymTabRec DisplayTab[] =
{
    {ENDSUBSECTION, "endsubsection"},
    {MODES, "modes"},
    {VIEWPORT, "viewport"},
    {VIRTUAL, "virtual"},
    {VISUAL, "visual"},
    {BLACK_TOK, "black"},
    {WHITE_TOK, "white"},
    {DEPTH, "depth"},
    {BPP, "fbbpp"},
    {WEIGHT, "weight"},
    {OPTION, "option"},
    {-1, ""},
};

#define CLEANUP xconfigFreeDisplayList

static int addImpliedScreen(XConfigPtr config);

XConfigDisplayPtr
xconfigParseDisplaySubSection (void)
{
    int token;
    PARSE_PROLOGUE (XConfigDisplayPtr, XConfigDisplayRec)

    ptr->black.red = ptr->black.green = ptr->black.blue = -1;
    ptr->white.red = ptr->white.green = ptr->white.blue = -1;
    ptr->frameX0 = ptr->frameY0 = -1;
    while ((token = xconfigGetToken (DisplayTab)) != ENDSUBSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case VIEWPORT:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (VIEWPORT_MSG, NULL);
            ptr->frameX0 = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (VIEWPORT_MSG, NULL);
            ptr->frameY0 = val.num;
            break;
        case VIRTUAL:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (VIRTUAL_MSG, NULL);
            ptr->virtualX = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (VIRTUAL_MSG, NULL);
            ptr->virtualY = val.num;
            break;
        case DEPTH:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "Display");
            ptr->depth = val.num;
            break;
        case BPP:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "Display");
            ptr->bpp = val.num;
            break;
        case VISUAL:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Display");
            ptr->visual = val.str;
            break;
        case WEIGHT:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WEIGHT_MSG, NULL);
            ptr->weight.red = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WEIGHT_MSG, NULL);
            ptr->weight.green = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WEIGHT_MSG, NULL);
            ptr->weight.blue = val.num;
            break;
        case BLACK_TOK:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (BLACK_MSG, NULL);
            ptr->black.red = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (BLACK_MSG, NULL);
            ptr->black.green = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (BLACK_MSG, NULL);
            ptr->black.blue = val.num;
            break;
        case WHITE_TOK:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WHITE_MSG, NULL);
            ptr->white.red = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WHITE_MSG, NULL);
            ptr->white.green = val.num;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (WHITE_MSG, NULL);
            ptr->white.blue = val.num;
            break;
        case MODES:
            {
                XConfigModePtr mptr;

                while ((token =
                        xconfigGetSubTokenWithTab(&(ptr->comment),
                                                  DisplayTab)) == STRING)
                {
                    mptr = calloc (1, sizeof (XConfigModeRec));
                    mptr->mode_name = val.str;
                    mptr->next = NULL;
                    xconfigAddListItem((GenericListPtr *)(&ptr->modes),
                                       (GenericListPtr) mptr);
                }
                xconfigUnGetToken (token);
            }
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

static XConfigSymTabRec ScreenTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OBSDRIVER, "driver"},
    {MDEVICE, "device"},
    {MONITOR, "monitor"},
    {VIDEOADAPTOR, "videoadaptor"},
    {SCREENNO, "screenno"},
    {SUBSECTION, "subsection"},
    {DEFAULTDEPTH, "defaultcolordepth"},
    {DEFAULTDEPTH, "defaultdepth"},
    {DEFAULTBPP, "defaultbpp"},
    {DEFAULTFBBPP, "defaultfbbpp"},
    {OPTION, "option"},
    {-1, ""},
};

#define CLEANUP xconfigFreeScreenList
XConfigScreenPtr
xconfigParseScreenSection (void)
{
    int has_ident = FALSE;
    int has_driver= FALSE;
    int token;

    PARSE_PROLOGUE (XConfigScreenPtr, XConfigScreenRec)

        while ((token = xconfigGetToken (ScreenTab)) != ENDSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case IDENTIFIER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Identifier");
            ptr->identifier = val.str;
            if (has_ident || has_driver)
                Error (ONLY_ONE_MSG,"Identifier or Driver");
            has_ident = TRUE;
            break;
        case OBSDRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Driver");
            ptr->obsolete_driver = val.str;
            if (has_ident || has_driver)
                Error (ONLY_ONE_MSG,"Identifier or Driver");
            has_driver = TRUE;
            break;
        case DEFAULTDEPTH:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "DefaultDepth");
            ptr->defaultdepth = val.num;
            break;
        case DEFAULTBPP:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "DefaultBPP");
            ptr->defaultbpp = val.num;
            break;
        case DEFAULTFBBPP:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "DefaultFbBPP");
            ptr->defaultfbbpp = val.num;
            break;
        case MDEVICE:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Device");
            ptr->device_name = val.str;
            break;
        case MONITOR:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Monitor");
            ptr->monitor_name = val.str;
            break;
        case VIDEOADAPTOR:
            {
                XConfigAdaptorLinkPtr aptr;

                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "VideoAdaptor");

                /* Don't allow duplicates */
                for (aptr = ptr->adaptors; aptr; 
                    aptr = (XConfigAdaptorLinkPtr) aptr->next)
                    if (xconfigNameCompare (val.str, aptr->adaptor_name) == 0)
                        break;

                if (aptr == NULL)
                {
                    aptr = calloc (1, sizeof (XConfigAdaptorLinkRec));
                    aptr->next = NULL;
                    aptr->adaptor_name = val.str;
                    xconfigAddListItem ((GenericListPtr *)(&ptr->adaptors),
                                        (GenericListPtr) aptr);
                }
            }
            break;
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;
        case SUBSECTION:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "SubSection");
            {
                free(val.str);
                HANDLE_LIST (displays, xconfigParseDisplaySubSection,
                             XConfigDisplayPtr);
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

    if (!has_ident && !has_driver)
        Error (NO_IDENT_MSG, NULL);

    return ptr;
}

void
xconfigPrintScreenSection (FILE * cf, XConfigScreenPtr ptr)
{
    XConfigAdaptorLinkPtr aptr;
    XConfigDisplayPtr dptr;
    XConfigModePtr mptr;

    while (ptr)
    {
        fprintf (cf, "Section \"Screen\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);
        if (ptr->obsolete_driver)
            fprintf (cf, "    Driver         \"%s\"\n", ptr->obsolete_driver);
        if (ptr->device_name)
            fprintf (cf, "    Device         \"%s\"\n", ptr->device_name);
        if (ptr->monitor_name)
            fprintf (cf, "    Monitor        \"%s\"\n", ptr->monitor_name);
        if (ptr->defaultdepth)
            fprintf (cf, "    DefaultDepth    %d\n",
                     ptr->defaultdepth);
        if (ptr->defaultbpp)
            fprintf (cf, "    DefaultBPP      %d\n",
                     ptr->defaultbpp);
        if (ptr->defaultfbbpp)
            fprintf (cf, "    DefaultFbBPP    %d\n",
                     ptr->defaultfbbpp);
        xconfigPrintOptionList(cf, ptr->options, 1);
        for (aptr = ptr->adaptors; aptr; aptr = aptr->next)
        {
            fprintf (cf, "    VideoAdaptor   \"%s\"\n", aptr->adaptor_name);
        }
        for (dptr = ptr->displays; dptr; dptr = dptr->next)
        {
            fprintf (cf, "    SubSection     \"Display\"\n");
            if (dptr->comment)
                fprintf (cf, "%s", dptr->comment);
            if (dptr->frameX0 >= 0 || dptr->frameY0 >= 0)
            {
                fprintf (cf, "        Viewport    %d %d\n",
                         dptr->frameX0, dptr->frameY0);
            }
            if (dptr->virtualX != 0 || dptr->virtualY != 0)
            {
                fprintf (cf, "        Virtual     %d %d\n",
                         dptr->virtualX, dptr->virtualY);
            }
            if (dptr->depth)
            {
                fprintf (cf, "        Depth       %d\n", dptr->depth);
            }
            if (dptr->bpp)
            {
                fprintf (cf, "        FbBPP       %d\n", dptr->bpp);
            }
            if (dptr->visual)
            {
                fprintf (cf, "        Visual     \"%s\"\n", dptr->visual);
            }
            if (dptr->weight.red != 0)
            {
                fprintf (cf, "        Weight      %d %d %d\n",
                     dptr->weight.red, dptr->weight.green, dptr->weight.blue);
            }
            if (dptr->black.red != -1)
            {
                fprintf (cf, "        Black       0x%04x 0x%04x 0x%04x\n",
                      dptr->black.red, dptr->black.green, dptr->black.blue);
            }
            if (dptr->white.red != -1)
            {
                fprintf (cf, "        White       0x%04x 0x%04x 0x%04x\n",
                      dptr->white.red, dptr->white.green, dptr->white.blue);
            }
            if (dptr->modes)
            {
                fprintf (cf, "        Modes     ");
            }
            for (mptr = dptr->modes; mptr; mptr = mptr->next)
            {
                fprintf (cf, " \"%s\"", mptr->mode_name);
            }
            if (dptr->modes)
            {
                fprintf (cf, "\n");
            }
            xconfigPrintOptionList(cf, dptr->options, 2);
            fprintf (cf, "    EndSubSection\n");
        }
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }

}

void
xconfigFreeScreenList (XConfigScreenPtr *ptr)
{
    XConfigScreenPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->monitor_name);
        TEST_FREE ((*ptr)->device_name);
        TEST_FREE ((*ptr)->comment);
        xconfigFreeOptionList (&((*ptr)->options));
        xconfigFreeAdaptorLinkList (&((*ptr)->adaptors));
        xconfigFreeDisplayList (&((*ptr)->displays));
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

void
xconfigFreeAdaptorLinkList (XConfigAdaptorLinkPtr *ptr)
{
    XConfigAdaptorLinkPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->adaptor_name);
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

void
xconfigFreeDisplayList (XConfigDisplayPtr *ptr)
{
    XConfigDisplayPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        xconfigFreeModeList (&((*ptr)->modes));
        xconfigFreeOptionList (&((*ptr)->options));
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

void
xconfigFreeModeList (XConfigModePtr *ptr)
{
    XConfigModePtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->mode_name);
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

int
xconfigValidateScreen (XConfigPtr p)
{
    XConfigScreenPtr screen = p->screens;
    XConfigMonitorPtr monitor;
    XConfigDevicePtr device;
    XConfigAdaptorLinkPtr adaptor;

    /*
     * if we do not have a screen, just return TRUE; we'll add a
     * screen later during the Sanitize step
     */

    if (!screen) return TRUE;

    while (screen)
    {
        if (screen->obsolete_driver && !screen->identifier)
            screen->identifier = screen->obsolete_driver;

        monitor = xconfigFindMonitor (screen->monitor_name, p->monitors);
        if (screen->monitor_name)
        {
            if (!monitor)
            {
                xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_MONITOR_MSG,
                             screen->monitor_name, screen->identifier);
                return (FALSE);
            }
            else
            {
                screen->monitor = monitor;
                if (!xconfigValidateMonitor(p, screen))
                    return (FALSE);
            }
        }

        device = xconfigFindDevice (screen->device_name, p->devices);
        if (!device)
        {
            xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_DEVICE_MSG,
                         screen->device_name, screen->identifier);
            return (FALSE);
        }
        else
            screen->device = device;

        adaptor = screen->adaptors;
        while (adaptor) {
            adaptor->adaptor = xconfigFindVideoAdaptor(adaptor->adaptor_name,
                                                       p->videoadaptors);
            if (!adaptor->adaptor) {
                xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_ADAPTOR_MSG,
                             adaptor->adaptor_name,
                             screen->identifier);
                return (FALSE);
            } else if (adaptor->adaptor->fwdref) {
                xconfigErrorMsg(ValidationErrorMsg, ADAPTOR_REF_TWICE_MSG,
                             adaptor->adaptor_name,
                             adaptor->adaptor->fwdref);
                return (FALSE);
            }
            
            adaptor->adaptor->fwdref = xconfigStrdup(screen->identifier);
            adaptor = adaptor->next;
        }

        screen = screen->next;
    }

    return (TRUE);
}

int xconfigSanitizeScreen(XConfigPtr p)
{
    XConfigScreenPtr screen = p->screens;
    XConfigMonitorPtr monitor;

    if (!addImpliedScreen(p)) {
        return FALSE;
    }
   
    while (screen) {
        
        /*
         * if no monitor for this screen (either the monitor name, or
         * the actual monitor pointer), find a monitor: resolve
         * discrepancies between screen->monitor_name and
         * screen->monitor; otherwise use the first monitor in the
         * config; if we still don't have a monitor, add a new one
         */

        if (!screen->monitor_name || !screen->monitor) {
        
            monitor = NULL;

            if (!monitor && screen->monitor) {
                monitor = screen->monitor;
            }

            if (!monitor && screen->monitor_name) {
                monitor = xconfigFindMonitor(screen->monitor_name,
                                             p->monitors);
            }
            
            if (!monitor && p->monitors) {
                monitor = p->monitors;
            }

            if (!monitor) {
                monitor = xconfigAddMonitor(p, 0);
            }
            
            if (monitor) {
                screen->monitor = monitor;
                
                if (screen->monitor_name) {
                    free(screen->monitor_name);
                }
                
                screen->monitor_name = xconfigStrdup(monitor->identifier);
                
                if (!xconfigValidateMonitor(p, screen)) {
                    return FALSE;
                }
            }
        }
        
        screen = screen->next;        
    }
    
    return TRUE;
}



XConfigScreenPtr
xconfigFindScreen (const char *ident, XConfigScreenPtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

XConfigModePtr
xconfigFindMode (const char *name, XConfigModePtr p)
{
    while (p)
    {
        if (xconfigNameCompare (name, p->mode_name) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

void
xconfigAddMode(XConfigModePtr *pHead, const char *name)
{
    XConfigModePtr mode;
    
    mode = xconfigAlloc(sizeof(XConfigModeRec));
    mode->mode_name = xconfigStrdup(name);

    mode->next = *pHead;
    *pHead = mode;
}


void
xconfigRemoveMode(XConfigModePtr *pHead, const char *name)
{
    XConfigModePtr p = *pHead;
    XConfigModePtr last = NULL;

    while (p) {
        if (xconfigNameCompare(p->mode_name, name) == 0) {
            if (last) {
                last->next = p->next;
            } else {
                *pHead = p->next;
            }
            free(p->mode_name);
            free(p);
            return;
        }
        last = p;
        p = p->next;
    }
}


static int addImpliedScreen(XConfigPtr config)
{
    XConfigScreenPtr screen;
    XConfigDevicePtr device;
    XConfigMonitorPtr monitor;

    if (config->screens) return TRUE;

    xconfigErrorMsg(WarnMsg, "No Screen specified, constructing implicit "
                    "screen section.\n");

    /* allocate the new screen section */

    screen = calloc(1, sizeof(XConfigScreenRec));
    if (!screen) return FALSE;
    
    screen->identifier = xconfigStrdup("Default Screen");

    /*
     * Use the first device section if there is one.
     */
    if (config->devices) {
        device = config->devices;
        screen->device_name = xconfigStrdup(device->identifier);
        screen->device = device;
    }

    /*
     * Use the first monitor section if there is one.
     */
    if (config->monitors) {
        monitor = config->monitors;
        screen->monitor_name = xconfigStrdup(monitor->identifier);
        screen->monitor = monitor;
    }

    config->screens = screen;

    return TRUE;
}
