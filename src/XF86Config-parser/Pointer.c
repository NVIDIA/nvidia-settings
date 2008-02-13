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

static XConfigSymTabRec PointerTab[] =
{
    {PROTOCOL, "protocol"},
    {EMULATE3, "emulate3buttons"},
    {EM3TIMEOUT, "emulate3timeout"},
    {ENDSUBSECTION, "endsubsection"},
    {ENDSECTION, "endsection"},
    {PDEVICE, "device"},
    {PDEVICE, "port"},
    {BAUDRATE, "baudrate"},
    {SAMPLERATE, "samplerate"},
    {CLEARDTR, "cleardtr"},
    {CLEARRTS, "clearrts"},
    {CHORDMIDDLE, "chordmiddle"},
    {PRESOLUTION, "resolution"},
    {DEVICE_NAME, "devicename"},
    {ALWAYSCORE, "alwayscore"},
    {PBUTTONS, "buttons"},
    {ZAXISMAPPING, "zaxismapping"},
    {-1, ""},
};

static XConfigSymTabRec ZMapTab[] =
{
    {XAXIS, "x"},
    {YAXIS, "y"},
    {-1, ""},
};

#define CLEANUP xconfigFreeInputList

XConfigInputPtr
xconfigParsePointerSection (void)
{
    char *s, *s1, *s2;
    int l;
    int token;
    PARSE_PROLOGUE (XConfigInputPtr, XConfigInputRec)

    while ((token = xconfigGetToken (PointerTab)) != ENDSECTION)
    {
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
        case PROTOCOL:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Protocol");
            ptr->options = xconfigAddNewOption(ptr->options,
                                            "Protocol", val.str);
            break;
        case PDEVICE:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Device");
            ptr->options = xconfigAddNewOption(ptr->options,
                                            "Device", val.str);
            break;
        case EMULATE3:
            ptr->options =
                xconfigAddNewOption(ptr->options, "Emulate3Buttons", NULL);
            break;
        case EM3TIMEOUT:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER || val.num < 0)
                Error (POSITIVE_INT_MSG, "Emulate3Timeout");
            s = xconfigULongToString(val.num);
            ptr->options =
                xconfigAddNewOption(ptr->options, "Emulate3Timeout", s);
            TEST_FREE(s);
            break;
        case CHORDMIDDLE:
            ptr->options = xconfigAddNewOption(ptr->options, "ChordMiddle",
                                               NULL);
            break;
        case PBUTTONS:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER || val.num < 0)
                Error (POSITIVE_INT_MSG, "Buttons");
            s = xconfigULongToString(val.num);
            ptr->options = xconfigAddNewOption(ptr->options, "Buttons", s);
            TEST_FREE(s);
            break;
        case BAUDRATE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER || val.num < 0)
                Error (POSITIVE_INT_MSG, "BaudRate");
            s = xconfigULongToString(val.num);
            ptr->options =
                xconfigAddNewOption(ptr->options, "BaudRate", s);
            TEST_FREE(s);
            break;
        case SAMPLERATE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER || val.num < 0)
                Error (POSITIVE_INT_MSG, "SampleRate");
            s = xconfigULongToString(val.num);
            ptr->options =
                xconfigAddNewOption(ptr->options, "SampleRate", s);
            TEST_FREE(s);
            break;
        case PRESOLUTION:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER || val.num < 0)
                Error (POSITIVE_INT_MSG, "Resolution");
            s = xconfigULongToString(val.num);
            ptr->options =
                xconfigAddNewOption(ptr->options, "Resolution", s);
            TEST_FREE(s);
            break;
        case CLEARDTR:
            ptr->options =
                xconfigAddNewOption(ptr->options, "ClearDTR", NULL);
            break;
        case CLEARRTS:
            ptr->options =
                xconfigAddNewOption(ptr->options, "ClearRTS", NULL);
            break;
        case ZAXISMAPPING:
            switch (xconfigGetToken(ZMapTab)) {
            case NUMBER:
                if (val.num < 0)
                    Error (ZAXISMAPPING_MSG, NULL);
                s1 = xconfigULongToString(val.num);
                if (xconfigGetSubToken (&(ptr->comment)) != NUMBER ||
                    val.num < 0)
                    Error (ZAXISMAPPING_MSG, NULL);
                s2 = xconfigULongToString(val.num);
                l = strlen(s1) + 1 + strlen(s2) + 1;
                s = malloc(l);
                sprintf(s, "%s %s", s1, s2);
                free(s1);
                free(s2);
                break;
            case XAXIS:
                s = xconfigStrdup("x");
                break;
            case YAXIS:
                s = xconfigStrdup("y");
                break;
            default:
                Error (ZAXISMAPPING_MSG, NULL);
                break;
            }
            ptr->options =
                xconfigAddNewOption(ptr->options, "ZAxisMapping", s);
            TEST_FREE(s);
            break;
        case ALWAYSCORE:
            break;
        case EOF_TOKEN:
            Error (UNEXPECTED_EOF_MSG, NULL);
            break;
        default:
            Error (INVALID_KEYWORD_MSG, xconfigTokenString ());
            break;
        }
    }

    ptr->identifier = xconfigStrdup(CONF_IMPLICIT_POINTER);
    ptr->driver = xconfigStrdup("mouse");
    ptr->options = xconfigAddNewOption(ptr->options, "CorePointer", NULL);

    return ptr;
}

#undef CLEANUP

