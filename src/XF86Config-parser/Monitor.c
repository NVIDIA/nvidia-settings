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

static XConfigSymTabRec MonitorTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {VENDOR, "vendorname"},
    {MODEL, "modelname"},
    {USEMODES, "usemodes"},
    {MODELINE, "modeline"},
    {DISPLAYSIZE, "displaysize"},
    {HORIZSYNC, "horizsync"},
    {VERTREFRESH, "vertrefresh"},
    {MODE, "mode"},
    {GAMMA, "gamma"},
    {OPTION, "option"},
    {-1, ""},
};

static XConfigSymTabRec ModesTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {MODELINE, "modeline"},
    {MODE, "mode"},
    {-1, ""},
};

static XConfigSymTabRec TimingTab[] =
{
    {TT_INTERLACE, "interlace"},
    {TT_PHSYNC, "+hsync"},
    {TT_NHSYNC, "-hsync"},
    {TT_PVSYNC, "+vsync"},
    {TT_NVSYNC, "-vsync"},
    {TT_CSYNC, "composite"},
    {TT_PCSYNC, "+csync"},
    {TT_NCSYNC, "-csync"},
    {TT_DBLSCAN, "doublescan"},
    {TT_HSKEW, "hskew"},
    {TT_BCAST, "bcast"},
    {TT_VSCAN, "vscan"},
    {TT_CUSTOM, "CUSTOM"},
    {-1, ""},
};

static XConfigSymTabRec ModeTab[] =
{
    {DOTCLOCK, "dotclock"},
    {HTIMINGS, "htimings"},
    {VTIMINGS, "vtimings"},
    {FLAGS, "flags"},
    {HSKEW, "hskew"},
    {BCAST, "bcast"},
    {VSCAN, "vscan"},
    {ENDMODE, "endmode"},
    {-1, ""},
};

#define CLEANUP xconfigFreeModeLineList

XConfigModeLinePtr
xconfigParseModeLine (void)
{
    int token;
    PARSE_PROLOGUE (XConfigModeLinePtr, XConfigModeLineRec)

    /* Identifier */
    if (xconfigGetSubToken (&(ptr->comment)) != STRING)
        Error ("ModeLine identifier expected", NULL);
    ptr->identifier = val.str;

    /* DotClock */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine dotclock expected", NULL);
    ptr->clock = (int) (val.realnum * 1000.0 + 0.5);

    /* HDisplay */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine Hdisplay expected", NULL);
    ptr->hdisplay = val.num;

    /* HSyncStart */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine HSyncStart expected", NULL);
    ptr->hsyncstart = val.num;

    /* HSyncEnd */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine HSyncEnd expected", NULL);
    ptr->hsyncend = val.num;

    /* HTotal */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine HTotal expected", NULL);
    ptr->htotal = val.num;

    /* VDisplay */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine Vdisplay expected", NULL);
    ptr->vdisplay = val.num;

    /* VSyncStart */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine VSyncStart expected", NULL);
    ptr->vsyncstart = val.num;

    /* VSyncEnd */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine VSyncEnd expected", NULL);
    ptr->vsyncend = val.num;

    /* VTotal */
    if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
        Error ("ModeLine VTotal expected", NULL);
    ptr->vtotal = val.num;

    token = xconfigGetSubTokenWithTab (&(ptr->comment), TimingTab);
    while ((token == TT_INTERLACE) || (token == TT_PHSYNC) ||
           (token == TT_NHSYNC) || (token == TT_PVSYNC) ||
           (token == TT_NVSYNC) || (token == TT_CSYNC) ||
           (token == TT_PCSYNC) || (token == TT_NCSYNC) ||
           (token == TT_DBLSCAN) || (token == TT_HSKEW) ||
           (token == TT_VSCAN) || (token == TT_BCAST))
    {
        switch (token)
        {

        case TT_INTERLACE:
            ptr->flags |= XCONFIG_MODE_INTERLACE;
            break;
        case TT_PHSYNC:
            ptr->flags |= XCONFIG_MODE_PHSYNC;
            break;
        case TT_NHSYNC:
            ptr->flags |= XCONFIG_MODE_NHSYNC;
            break;
        case TT_PVSYNC:
            ptr->flags |= XCONFIG_MODE_PVSYNC;
            break;
        case TT_NVSYNC:
            ptr->flags |= XCONFIG_MODE_NVSYNC;
            break;
        case TT_CSYNC:
            ptr->flags |= XCONFIG_MODE_CSYNC;
            break;
        case TT_PCSYNC:
            ptr->flags |= XCONFIG_MODE_PCSYNC;
            break;
        case TT_NCSYNC:
            ptr->flags |= XCONFIG_MODE_NCSYNC;
            break;
        case TT_DBLSCAN:
            ptr->flags |= XCONFIG_MODE_DBLSCAN;
            break;
        case TT_HSKEW:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "Hskew");
            ptr->hskew = val.num;
            ptr->flags |= XCONFIG_MODE_HSKEW;
            break;
        case TT_BCAST:
            ptr->flags |= XCONFIG_MODE_BCAST;
            break;
        case TT_VSCAN:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "Vscan");
            ptr->vscan = val.num;
            ptr->flags |= XCONFIG_MODE_VSCAN;
            break;
        case TT_CUSTOM:
            ptr->flags |= XCONFIG_MODE_CUSTOM;
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
            break;
        }
        token = xconfigGetSubTokenWithTab (&(ptr->comment), TimingTab);
    }
    xconfigUnGetToken (token);

    return (ptr);
}

XConfigModeLinePtr
xconfigParseVerboseMode (void)
{
    int token, token2;
    int had_dotclock = 0, had_htimings = 0, had_vtimings = 0;
    PARSE_PROLOGUE (XConfigModeLinePtr, XConfigModeLineRec)

        if (xconfigGetSubToken (&(ptr->comment)) != STRING)
        Error ("Mode name expected", NULL);
    ptr->identifier = val.str;
    while ((token = xconfigGetToken (ModeTab)) != ENDMODE)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case DOTCLOCK:
            if ((token = xconfigGetSubToken (&(ptr->comment))) != NUMBER)
                Error (NUMBER_MSG, "DotClock");
            ptr->clock = (int) (val.realnum * 1000.0 + 0.5);
            had_dotclock = 1;
            break;
        case HTIMINGS:
            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->hdisplay = val.num;
            else
                Error ("Horizontal display expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->hsyncstart = val.num;
            else
                Error ("Horizontal sync start expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->hsyncend = val.num;
            else
                Error ("Horizontal sync end expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->htotal = val.num;
            else
                Error ("Horizontal total expected", NULL);
            had_htimings = 1;
            break;
        case VTIMINGS:
            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->vdisplay = val.num;
            else
                Error ("Vertical display expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->vsyncstart = val.num;
            else
                Error ("Vertical sync start expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->vsyncend = val.num;
            else
                Error ("Vertical sync end expected", NULL);

            if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                ptr->vtotal = val.num;
            else
                Error ("Vertical total expected", NULL);
            had_vtimings = 1;
            break;
        case FLAGS:
            token = xconfigGetSubToken (&(ptr->comment));
            if (token != STRING)
                Error (QUOTE_MSG, "Flags");
            while (token == STRING)
            {
                token2 = xconfigGetStringToken (TimingTab);
                switch (token2)
                {
                case TT_INTERLACE:
                    ptr->flags |= XCONFIG_MODE_INTERLACE;
                    break;
                case TT_PHSYNC:
                    ptr->flags |= XCONFIG_MODE_PHSYNC;
                    break;
                case TT_NHSYNC:
                    ptr->flags |= XCONFIG_MODE_NHSYNC;
                    break;
                case TT_PVSYNC:
                    ptr->flags |= XCONFIG_MODE_PVSYNC;
                    break;
                case TT_NVSYNC:
                    ptr->flags |= XCONFIG_MODE_NVSYNC;
                    break;
                case TT_CSYNC:
                    ptr->flags |= XCONFIG_MODE_CSYNC;
                    break;
                case TT_PCSYNC:
                    ptr->flags |= XCONFIG_MODE_PCSYNC;
                    break;
                case TT_NCSYNC:
                    ptr->flags |= XCONFIG_MODE_NCSYNC;
                    break;
                case TT_DBLSCAN:
                    ptr->flags |= XCONFIG_MODE_DBLSCAN;
                    break;
                case TT_CUSTOM:
                    ptr->flags |= XCONFIG_MODE_CUSTOM;
                    break;
                case EOF_TOKEN:
                    Error (UNEXPECTED_EOF_MSG, NULL);
                    break;
                default:
                    Error ("Unknown flag string", NULL);
                    break;
                }
                token = xconfigGetSubToken (&(ptr->comment));
            }
            xconfigUnGetToken (token);
            break;
        case HSKEW:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error ("Horizontal skew expected", NULL);
            ptr->flags |= XCONFIG_MODE_HSKEW;
            ptr->hskew = val.num;
            break;
        case VSCAN:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error ("Vertical scan count expected", NULL);
            ptr->flags |= XCONFIG_MODE_VSCAN;
            ptr->vscan = val.num;
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            Error ("Unexepcted token in verbose \"Mode\" entry\n", NULL);
        }
    }
    if (!had_dotclock)
        Error ("the dotclock is missing", NULL);
    if (!had_htimings)
        Error ("the horizontal timings are missing", NULL);
    if (!had_vtimings)
        Error ("the vertical timings are missing", NULL);

    return (ptr);
}

#undef CLEANUP

#define CLEANUP xconfigFreeMonitorList

XConfigMonitorPtr
xconfigParseMonitorSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigMonitorPtr, XConfigMonitorRec)

        while ((token = xconfigGetToken (MonitorTab)) != ENDSECTION)
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
        case VENDOR:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Vendor");
            ptr->vendor = val.str;
            break;
        case MODEL:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "ModelName");
            ptr->modelname = val.str;
            break;
        case MODE:
            HANDLE_LIST (modelines, xconfigParseVerboseMode,
                         XConfigModeLinePtr);
            break;
        case MODELINE:
            HANDLE_LIST (modelines, xconfigParseModeLine,
                         XConfigModeLinePtr);
            break;
        case DISPLAYSIZE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (DISPLAYSIZE_MSG, NULL);
            ptr->width = val.realnum;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (DISPLAYSIZE_MSG, NULL);
            ptr->height = val.realnum;
            break;

        case HORIZSYNC:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (HORIZSYNC_MSG, NULL);
            do {
                ptr->hsync[ptr->n_hsync].lo = val.realnum;
                switch (token = xconfigGetSubToken (&(ptr->comment)))
                {
                    case COMMA:
                        ptr->hsync[ptr->n_hsync].hi =
                        ptr->hsync[ptr->n_hsync].lo;
                        break;
                    case DASH:
                        if (xconfigGetSubToken (&(ptr->comment)) != NUMBER ||
                            (float)val.realnum < ptr->hsync[ptr->n_hsync].lo)
                            Error (HORIZSYNC_MSG, NULL);
                        ptr->hsync[ptr->n_hsync].hi = val.realnum;
                        if ((token = xconfigGetSubToken (&(ptr->comment))) == COMMA)
                            break;
                        ptr->n_hsync++;
                        goto HorizDone;
                    default:
                        /* We cannot currently know if a '\n' was found,
                         * or this is a real error
                         */
                        ptr->hsync[ptr->n_hsync].hi =
                        ptr->hsync[ptr->n_hsync].lo;
                        ptr->n_hsync++;
                        goto HorizDone;
                }
                if (ptr->n_hsync >= CONF_MAX_HSYNC)
                    Error ("Sorry. Too many horizontal sync intervals.", NULL);
                ptr->n_hsync++;
            } while ((token = xconfigGetSubToken (&(ptr->comment))) == NUMBER);
HorizDone:
            xconfigUnGetToken (token);
            break;

        case VERTREFRESH:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (VERTREFRESH_MSG, NULL);
            do {
                ptr->vrefresh[ptr->n_vrefresh].lo = val.realnum;
                switch (token = xconfigGetSubToken (&(ptr->comment)))
                {
                    case COMMA:
                        ptr->vrefresh[ptr->n_vrefresh].hi =
                        ptr->vrefresh[ptr->n_vrefresh].lo;
                        break;
                    case DASH:
                        if (xconfigGetSubToken (&(ptr->comment)) != NUMBER ||
                            (float)val.realnum < ptr->vrefresh[ptr->n_vrefresh].lo)
                            Error (VERTREFRESH_MSG, NULL);
                        ptr->vrefresh[ptr->n_vrefresh].hi = val.realnum;
                        if ((token = xconfigGetSubToken (&(ptr->comment))) == COMMA)
                            break;
                        ptr->n_vrefresh++;
                        goto VertDone;
                    default:
                        /* We cannot currently know if a '\n' was found,
                         * or this is a real error
                         */
                        ptr->vrefresh[ptr->n_vrefresh].hi =
                        ptr->vrefresh[ptr->n_vrefresh].lo;
                        ptr->n_vrefresh++;
                        goto VertDone;
                }
                if (ptr->n_vrefresh >= CONF_MAX_VREFRESH)
                    Error ("Sorry. Too many vertical refresh intervals.", NULL);
                ptr->n_vrefresh++;
            } while ((token = xconfigGetSubToken (&(ptr->comment))) == NUMBER);
VertDone:
            xconfigUnGetToken (token);
            break;

        case GAMMA:
            if( xconfigGetSubToken (&(ptr->comment)) != NUMBER )
            {
                Error (INVALID_GAMMA_MSG, NULL);
            }
            else
            {
                ptr->gamma_red = ptr->gamma_green =
                    ptr->gamma_blue = val.realnum;
                if( xconfigGetSubToken (&(ptr->comment)) == NUMBER )
                {
                    ptr->gamma_green = val.realnum;
                    if( xconfigGetSubToken (&(ptr->comment)) == NUMBER )
                    {
                        ptr->gamma_blue = val.realnum;
                    }
                    else
                    {
                        Error (INVALID_GAMMA_MSG, NULL);
                    }
                }
                else
                    xconfigUnGetToken (token);
            }
            break;
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;
        case USEMODES:
                {
                XConfigModesLinkPtr mptr;

                if ((token = xconfigGetSubToken (&(ptr->comment))) != STRING)
                    Error (QUOTE_MSG, "UseModes");

                /* add to the end of the list of modes sections 
                   referenced here */
                mptr = calloc (1, sizeof (XConfigModesLinkRec));
                mptr->next = NULL;
                mptr->modes_name = val.str;
                mptr->modes = NULL;
                ptr->modes_sections = (XConfigModesLinkPtr)
                    xconfigAddListItem((GenericListPtr)ptr->modes_sections,
                            (GenericListPtr)mptr);
            }
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            xconfigErrorMsg(ParseErrorMsg, INVALID_KEYWORD_MSG,
                         xconfigTokenString());
            CLEANUP (ptr);
            return NULL;
            break;
        }
    }

    if (!has_ident)
        Error (NO_IDENT_MSG, NULL);

    return ptr;
}

#undef CLEANUP
#define CLEANUP xconfigFreeModesList

XConfigModesPtr
xconfigParseModesSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigModesPtr, XConfigModesRec)

    while ((token = xconfigGetToken (ModesTab)) != ENDSECTION)
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
        case MODE:
            HANDLE_LIST (modelines, xconfigParseVerboseMode,
                         XConfigModeLinePtr);
            break;
        case MODELINE:
            HANDLE_LIST (modelines, xconfigParseModeLine,
                         XConfigModeLinePtr);
            break;
        default:
            xconfigErrorMsg(ParseErrorMsg, INVALID_KEYWORD_MSG,
                         xconfigTokenString());
            CLEANUP (ptr);
            return NULL;
            break;
        }
    }

    if (!has_ident)
        Error (NO_IDENT_MSG, NULL);

    return ptr;
}

#undef CLEANUP

void
xconfigPrintMonitorSection (FILE * cf, XConfigMonitorPtr ptr)
{
    int i;
    XConfigModeLinePtr mlptr;
    XConfigModesLinkPtr mptr;

    while (ptr)
    {
        mptr = ptr->modes_sections;
        fprintf (cf, "Section \"Monitor\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);
        if (ptr->vendor)
            fprintf (cf, "    VendorName     \"%s\"\n", ptr->vendor);
        if (ptr->modelname)
            fprintf (cf, "    ModelName      \"%s\"\n", ptr->modelname);
        while (mptr) {
            fprintf (cf, "    UseModes       \"%s\"\n", mptr->modes_name);
            mptr = mptr->next;
        }
        if (ptr->width)
            fprintf (cf, "    DisplaySize     %d    %d\n",
                     ptr->width,
                     ptr->height);
        for (i = 0; i < ptr->n_hsync; i++)
        {
            fprintf (cf, "    HorizSync       %2.1f - %2.1f\n",
                     ptr->hsync[i].lo,
                     ptr->hsync[i].hi);
        }
        for (i = 0; i < ptr->n_vrefresh; i++)
        {
            if (ptr->vrefresh[i].lo == ptr->vrefresh[i].hi) {
                fprintf (cf, "    VertRefresh     %2.1f\n",
                         ptr->vrefresh[i].lo);
            } else {
                fprintf (cf, "    VertRefresh     %2.1f - %2.1f\n",
                         ptr->vrefresh[i].lo,
                         ptr->vrefresh[i].hi);
            }
        }
        if (ptr->gamma_red) {
            if (ptr->gamma_red == ptr->gamma_green
                && ptr->gamma_red == ptr->gamma_blue)
            {
                fprintf (cf, "    Gamma           %.4g\n",
                    ptr->gamma_red);
            } else {
                fprintf (cf, "    Gamma           %.4g %.4g %.4g\n",
                    ptr->gamma_red,
                    ptr->gamma_green,
                    ptr->gamma_blue);
            }
        }
        for (mlptr = ptr->modelines; mlptr; mlptr = mlptr->next)
        {
            fprintf (cf, "    ModeLine       \"%s\" %2.1f ",
                     mlptr->identifier, mlptr->clock / 1000.0);
            fprintf (cf, "%d %d %d %d %d %d %d %d",
                     mlptr->hdisplay, mlptr->hsyncstart,
                     mlptr->hsyncend, mlptr->htotal,
                     mlptr->vdisplay, mlptr->vsyncstart,
                     mlptr->vsyncend, mlptr->vtotal);
            if (mlptr->flags & XCONFIG_MODE_PHSYNC)
                fprintf (cf, " +hsync");
            if (mlptr->flags & XCONFIG_MODE_NHSYNC)
                fprintf (cf, " -hsync");
            if (mlptr->flags & XCONFIG_MODE_PVSYNC)
                fprintf (cf, " +vsync");
            if (mlptr->flags & XCONFIG_MODE_NVSYNC)
                fprintf (cf, " -vsync");
            if (mlptr->flags & XCONFIG_MODE_INTERLACE)
                fprintf (cf, " interlace");
            if (mlptr->flags & XCONFIG_MODE_CSYNC)
                fprintf (cf, " composite");
            if (mlptr->flags & XCONFIG_MODE_PCSYNC)
                fprintf (cf, " +csync");
            if (mlptr->flags & XCONFIG_MODE_NCSYNC)
                fprintf (cf, " -csync");
            if (mlptr->flags & XCONFIG_MODE_DBLSCAN)
                fprintf (cf, " doublescan");
            if (mlptr->flags & XCONFIG_MODE_HSKEW)
                fprintf (cf, " hskew %d", mlptr->hskew);
            if (mlptr->flags & XCONFIG_MODE_BCAST)
                fprintf (cf, " bcast");
            fprintf (cf, "\n");
        }
        xconfigPrintOptionList(cf, ptr->options, 1);
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigPrintModesSection (FILE * cf, XConfigModesPtr ptr)
{
    XConfigModeLinePtr mlptr;

    while (ptr)
    {
        fprintf (cf, "Section \"Modes\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier         \"%s\"\n", ptr->identifier);
        for (mlptr = ptr->modelines; mlptr; mlptr = mlptr->next)
        {
            fprintf (cf, "    ModeLine     \"%s\" %2.1f ",
                     mlptr->identifier, mlptr->clock / 1000.0);
            fprintf (cf, "%d %d %d %d %d %d %d %d",
                     mlptr->hdisplay, mlptr->hsyncstart,
                     mlptr->hsyncend, mlptr->htotal,
                     mlptr->vdisplay, mlptr->vsyncstart,
                     mlptr->vsyncend, mlptr->vtotal);
            if (mlptr->flags & XCONFIG_MODE_PHSYNC)
                fprintf (cf, " +hsync");
            if (mlptr->flags & XCONFIG_MODE_NHSYNC)
                fprintf (cf, " -hsync");
            if (mlptr->flags & XCONFIG_MODE_PVSYNC)
                fprintf (cf, " +vsync");
            if (mlptr->flags & XCONFIG_MODE_NVSYNC)
                fprintf (cf, " -vsync");
            if (mlptr->flags & XCONFIG_MODE_INTERLACE)
                fprintf (cf, " interlace");
            if (mlptr->flags & XCONFIG_MODE_CSYNC)
                fprintf (cf, " composite");
            if (mlptr->flags & XCONFIG_MODE_PCSYNC)
                fprintf (cf, " +csync");
            if (mlptr->flags & XCONFIG_MODE_NCSYNC)
                fprintf (cf, " -csync");
            if (mlptr->flags & XCONFIG_MODE_DBLSCAN)
                fprintf (cf, " doublescan");
            if (mlptr->flags & XCONFIG_MODE_HSKEW)
                fprintf (cf, " hskew %d", mlptr->hskew);
            if (mlptr->flags & XCONFIG_MODE_VSCAN)
                fprintf (cf, " vscan %d", mlptr->vscan);
            if (mlptr->flags & XCONFIG_MODE_BCAST)
                fprintf (cf, " bcast");
            if (mlptr->comment)
                fprintf (cf, "%s", mlptr->comment);
            else
                fprintf (cf, "\n");
        }
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigFreeMonitorList (XConfigMonitorPtr ptr)
{
    XConfigMonitorPtr prev;

    while (ptr)
    {
        TEST_FREE (ptr->identifier);
        TEST_FREE (ptr->vendor);
        TEST_FREE (ptr->modelname);
        TEST_FREE (ptr->comment);
        xconfigOptionListFree (ptr->options);
        xconfigFreeModeLineList (ptr->modelines);
        prev = ptr;
        ptr = ptr->next;
        free (prev);
    }
}

void
xconfigFreeModesList (XConfigModesPtr ptr)
{
    XConfigModesPtr prev;

    while (ptr)
    {
        TEST_FREE (ptr->identifier);
        TEST_FREE (ptr->comment);
        xconfigFreeModeLineList (ptr->modelines);
        prev = ptr;
        ptr = ptr->next;
        free (prev);
    }
}

void
xconfigFreeModeLineList (XConfigModeLinePtr ptr)
{
    XConfigModeLinePtr prev;
    while (ptr)
    {
        TEST_FREE (ptr->identifier);
        TEST_FREE (ptr->comment);
        prev = ptr;
        ptr = ptr->next;
        free (prev);
    }
}

void
xconfigFreeModesLinkList (XConfigModesLinkPtr ptr)
{
    XConfigModesLinkPtr prev;

    while (ptr)
    {
        TEST_FREE (ptr->modes_name);
        prev = ptr;
        ptr = ptr->next;
        free (prev);
    }
}

XConfigMonitorPtr
xconfigFindMonitor (const char *ident, XConfigMonitorPtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

XConfigModesPtr
xconfigFindModes (const char *ident, XConfigModesPtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

XConfigModeLinePtr
xconfigFindModeLine (const char *ident, XConfigModeLinePtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

int
xconfigValidateMonitor (XConfigPtr p, XConfigScreenPtr screen)
{
    XConfigMonitorPtr monitor = screen->monitor;
    XConfigModesLinkPtr modeslnk = monitor->modes_sections;
    XConfigModesPtr modes;
    while(modeslnk)
    {
        modes = xconfigFindModes (modeslnk->modes_name, p->modes);
        if (!modes)
        {
            xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_MODES_MSG, 
                         modeslnk->modes_name, screen->identifier);
            return (FALSE);
        }
        modeslnk->modes = modes;
        modeslnk = modeslnk->next;
    }
    return (TRUE);
}
