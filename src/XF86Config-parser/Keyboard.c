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
#include "ctype.h"

extern LexRec val;

static XConfigSymTabRec KeyboardTab[] =
{
    {ENDSECTION, "endsection"},
    {KPROTOCOL, "protocol"},
    {AUTOREPEAT, "autorepeat"},
    {XLEDS, "xleds"},
    {PANIX106, "panix106"},
    {XKBKEYMAP, "xkbkeymap"},
    {XKBCOMPAT, "xkbcompat"},
    {XKBTYPES, "xkbtypes"},
    {XKBKEYCODES, "xkbkeycodes"},
    {XKBGEOMETRY, "xkbgeometry"},
    {XKBSYMBOLS, "xkbsymbols"},
    {XKBDISABLE, "xkbdisable"},
    {XKBRULES, "xkbrules"},
    {XKBMODEL, "xkbmodel"},
    {XKBLAYOUT, "xkblayout"},
    {XKBVARIANT, "xkbvariant"},
    {XKBOPTIONS, "xkboptions"},
    /* The next two have become ServerFlags options */
    {VTINIT, "vtinit"},
    {VTSYSREQ, "vtsysreq"},
    /* Obsolete keywords */
    {SERVERNUM, "servernumlock"},
    {LEFTALT, "leftalt"},
    {RIGHTALT, "rightalt"},
    {RIGHTALT, "altgr"},
    {SCROLLLOCK_TOK, "scrolllock"},
    {RIGHTCTL, "rightctl"},
    {-1, ""},
};

/* Obsolete */
static XConfigSymTabRec KeyMapTab[] =
{
    {CONF_KM_META, "meta"},
    {CONF_KM_COMPOSE, "compose"},
    {CONF_KM_MODESHIFT, "modeshift"},
    {CONF_KM_MODELOCK, "modelock"},
    {CONF_KM_SCROLLLOCK, "scrolllock"},
    {CONF_KM_CONTROL, "control"},
    {-1, ""},
};

#define CLEANUP xconfigFreeInputList

XConfigInputPtr
xconfigParseKeyboardSection (void)
{
    char *s, *s1, *s2;
    int l;
    int token, ntoken;
    PARSE_PROLOGUE (XConfigInputPtr, XConfigInputRec)

        while ((token = xconfigGetToken (KeyboardTab)) != ENDSECTION)
        {
            switch (token)
            {
            case COMMENT:
                ptr->comment = xconfigAddComment(ptr->comment, val.str);
                break;
            case KPROTOCOL:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "Protocol");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("Protocol"),
                                                   val.str);
                break;
            case AUTOREPEAT:
                if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                    Error (AUTOREPEAT_MSG, NULL);
                s1 = xconfigULongToString(val.num);
                if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                    Error (AUTOREPEAT_MSG, NULL);
                s2 = xconfigULongToString(val.num);
                l = strlen(s1) + 1 + strlen(s2) + 1;
                s = malloc(l);
                sprintf(s, "%s %s", s1, s2);
                free(s1);
                free(s2);
                ptr->options =
                    xconfigAddNewOption(ptr->options,
                                        xconfigStrdup("AutoRepeat"), s);
                break;
            case XLEDS:
                if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                    Error (XLEDS_MSG, NULL);
                s = xconfigULongToString(val.num);
                l = strlen(s) + 1;
                while ((token = xconfigGetSubToken(&(ptr->comment))) == NUMBER)
                {
                    s1 = xconfigULongToString(val.num);
                    l += (1 + strlen(s1));
                    s = realloc(s, l);
                    strcat(s, " ");
                    strcat(s, s1);
                    free(s1);
                }
                xconfigUnGetToken (token);
                break;
            case SERVERNUM:
                xconfigErrorMsg(ParseWarningMsg, OBSOLETE_MSG,
                                xconfigTokenString());
                break;
            case LEFTALT:
            case RIGHTALT:
            case SCROLLLOCK_TOK:
            case RIGHTCTL:
                xconfigErrorMsg(ParseWarningMsg, OBSOLETE_MSG,
                                xconfigTokenString());
                break;
                ntoken = xconfigGetToken (KeyMapTab);
                switch (ntoken)
                {
                case EOF_TOKEN:
                    xconfigErrorMsg(ParseErrorMsg, UNEXPECTED_EOF_MSG);
                    CLEANUP (ptr);
                    return (NULL);
                    break;
                    
                default:
                    Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
                    break;
                }
                break;
            case VTINIT:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "VTInit");
                xconfigErrorMsg(ParseWarningMsg, MOVED_TO_FLAGS_MSG, "VTInit");
                break;
            case VTSYSREQ:
                xconfigErrorMsg(ParseWarningMsg,
                                MOVED_TO_FLAGS_MSG, "VTSysReq");
                break;
            case XKBDISABLE:
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbDisable"),
                                                   NULL);
                break;
            case XKBKEYMAP:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBKeymap");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbKeymap"),
                                                   val.str);
                break;
            case XKBCOMPAT:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBCompat");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbCompat"),
                                                   val.str);
                break;
            case XKBTYPES:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBTypes");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbTypes"),
                                                   val.str);
                break;
            case XKBKEYCODES:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBKeycodes");
                ptr->options =
                    xconfigAddNewOption(ptr->options,
                                        xconfigStrdup("XkbKeycodes"),
                                        val.str);
                break;
            case XKBGEOMETRY:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBGeometry");
                ptr->options =
                    xconfigAddNewOption(ptr->options,
                                        xconfigStrdup("XkbGeometry"),
                                        val.str);
                break;
            case XKBSYMBOLS:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBSymbols");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbSymbols"),
                                                   val.str);
                break;
            case XKBRULES:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBRules");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbRules"),
                                                   val.str);
                break;
            case XKBMODEL:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBModel");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbModel"),
                                                   val.str);
                break;
            case XKBLAYOUT:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBLayout");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbLayout"),
                                                   val.str);
                break;
            case XKBVARIANT:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBVariant");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbVariant"),
                                                   val.str);
                break;
            case XKBOPTIONS:
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (QUOTE_MSG, "XKBOptions");
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("XkbOptions"),
                                                   val.str);
                break;
            case PANIX106:
                ptr->options = xconfigAddNewOption(ptr->options,
                                                   xconfigStrdup("Panix106"),
                                                   NULL);
                break;
            case EOF_TOKEN:
                Error (UNEXPECTED_EOF_MSG, NULL);
                break;
            default:
                Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
                break;
            }
        }
    
    ptr->identifier = xconfigStrdup(CONF_IMPLICIT_KEYBOARD);
    ptr->driver = xconfigStrdup("keyboard");
    ptr->options = xconfigAddNewOption(ptr->options,
                                       xconfigStrdup("CoreKeyboard"), NULL);
    
    return ptr;
}

