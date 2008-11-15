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
#include <string.h>

extern LexRec val;

static XConfigSymTabRec LayoutTab[] =
{
    {ENDSECTION, "endsection"},
    {SCREEN, "screen"},
    {IDENTIFIER, "identifier"},
    {INACTIVE, "inactive"},
    {INPUTDEVICE, "inputdevice"},
    {OPTION, "option"},
    {-1, ""},
};

static XConfigSymTabRec AdjTab[] =
{
    {RIGHTOF, "rightof"},
    {LEFTOF, "leftof"},
    {ABOVE, "above"},
    {BELOW, "below"},
    {RELATIVE, "relative"},
    {ABSOLUTE, "absolute"},
    {-1, ""},
};


static int addImpliedLayout(XConfigPtr config, const char *screenName);


#define CLEANUP xconfigFreeLayoutList

XConfigLayoutPtr
xconfigParseLayoutSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigLayoutPtr, XConfigLayoutRec)

    while ((token = xconfigGetToken (LayoutTab)) != ENDSECTION)
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
        case INACTIVE:
            {
                XConfigInactivePtr iptr;

                iptr = calloc (1, sizeof (XConfigInactiveRec));
                iptr->next = NULL;
                if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                    Error (INACTIVE_MSG, NULL);
                iptr->device_name = val.str;
                xconfigAddListItem((GenericListPtr *)(&ptr->inactives),
                                   (GenericListPtr) iptr);
            }
            break;
        case SCREEN:
            {
                XConfigAdjacencyPtr aptr;
                int absKeyword = 0;

                aptr = calloc (1, sizeof (XConfigAdjacencyRec));
                aptr->next = NULL;
                aptr->scrnum = -1;
                aptr->where = CONF_ADJ_OBSOLETE;
                aptr->x = 0;
                aptr->y = 0;
                aptr->refscreen = NULL;
                if ((token = xconfigGetSubToken (&(ptr->comment))) == NUMBER)
                    aptr->scrnum = val.num;
                else
                    xconfigUnGetToken (token);
                token = xconfigGetSubToken(&(ptr->comment));
                if (token != STRING)
                    Error (SCREEN_MSG, NULL);
                aptr->screen_name = val.str;

                token = xconfigGetSubTokenWithTab(&(ptr->comment), AdjTab);
                switch (token)
                {
                case RIGHTOF:
                    aptr->where = CONF_ADJ_RIGHTOF;
                    break;
                case LEFTOF:
                    aptr->where = CONF_ADJ_LEFTOF;
                    break;
                case ABOVE:
                    aptr->where = CONF_ADJ_ABOVE;
                    break;
                case BELOW:
                    aptr->where = CONF_ADJ_BELOW;
                    break;
                case RELATIVE:
                    aptr->where = CONF_ADJ_RELATIVE;
                    break;
                case ABSOLUTE:
                    aptr->where = CONF_ADJ_ABSOLUTE;
                    absKeyword = 1;
                    break;
                case EOF_TOKEN:
                    Error (UNEXPECTED_EOF_MSG, NULL);
                    break;
                default:
                    xconfigUnGetToken (token);
                    token = xconfigGetSubToken(&(ptr->comment));
                    if (token == STRING)
                        aptr->where = CONF_ADJ_OBSOLETE;
                    else
                        aptr->where = CONF_ADJ_ABSOLUTE;
                }
                switch (aptr->where)
                {
                case CONF_ADJ_ABSOLUTE:
                    if (absKeyword) 
                        token = xconfigGetSubToken(&(ptr->comment));
                    if (token == NUMBER)
                    {
                        aptr->x = val.num;
                        token = xconfigGetSubToken(&(ptr->comment));
                        if (token != NUMBER)
                            Error(INVALID_SCR_MSG, NULL);
                        aptr->y = val.num;
                    } else {
                        if (absKeyword)
                            Error(INVALID_SCR_MSG, NULL);
                        else
                            xconfigUnGetToken (token);
                    }
                    break;
                case CONF_ADJ_RIGHTOF:
                case CONF_ADJ_LEFTOF:
                case CONF_ADJ_ABOVE:
                case CONF_ADJ_BELOW:
                case CONF_ADJ_RELATIVE:
                    token = xconfigGetSubToken(&(ptr->comment));
                    if (token != STRING)
                        Error(INVALID_SCR_MSG, NULL);
                    aptr->refscreen = val.str;
                    if (aptr->where == CONF_ADJ_RELATIVE)
                    {
                        token = xconfigGetSubToken(&(ptr->comment));
                        if (token != NUMBER)
                            Error(INVALID_SCR_MSG, NULL);
                        aptr->x = val.num;
                        token = xconfigGetSubToken(&(ptr->comment));
                        if (token != NUMBER)
                            Error(INVALID_SCR_MSG, NULL);
                        aptr->y = val.num;
                    }
                    break;
                case CONF_ADJ_OBSOLETE:
                    /* top */
                    aptr->top_name = val.str;

                    /* bottom */
                    if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                        Error (SCREEN_MSG, NULL);
                    aptr->bottom_name = val.str;

                    /* left */
                    if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                        Error (SCREEN_MSG, NULL);
                    aptr->left_name = val.str;

                    /* right */
                    if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                        Error (SCREEN_MSG, NULL);
                    aptr->right_name = val.str;

                }
                xconfigAddListItem((GenericListPtr *)(&ptr->adjacencies),
                                   (GenericListPtr) aptr);
            }
            break;
        case INPUTDEVICE:
            {
                XConfigInputrefPtr iptr;

                iptr = calloc (1, sizeof (XConfigInputrefRec));
                iptr->next = NULL;
                iptr->options = NULL;
                if (xconfigGetSubToken(&(ptr->comment)) != STRING)
                    Error (INPUTDEV_MSG, NULL);
                iptr->input_name = val.str;
                while ((token = xconfigGetSubToken(&(ptr->comment))) == STRING) {
                    xconfigAddNewOption(&iptr->options, val.str, NULL);
                }
                xconfigUnGetToken(token);
                xconfigAddListItem((GenericListPtr *)(&ptr->inputs),
                                   (GenericListPtr) iptr);
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

    if (!has_ident)
        Error (NO_IDENT_MSG, NULL);

    return ptr;
}

#undef CLEANUP

void
xconfigPrintLayoutSection (FILE * cf, XConfigLayoutPtr ptr)
{
    XConfigAdjacencyPtr aptr;
    XConfigInactivePtr iptr;
    XConfigInputrefPtr inptr;
    XConfigOptionPtr optr;

    while (ptr)
    {
        fprintf (cf, "Section \"ServerLayout\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);

        for (aptr = ptr->adjacencies; aptr; aptr = aptr->next)
        {
            fprintf (cf, "    Screen     ");
            if (aptr->scrnum >= 0)
                fprintf (cf, "%2d", aptr->scrnum);
            else
                fprintf (cf, "  ");
            fprintf (cf, "  \"%s\"", aptr->screen_name);
            switch(aptr->where)
            {
            case CONF_ADJ_OBSOLETE:
                fprintf (cf, " \"%s\"", aptr->top_name);
                fprintf (cf, " \"%s\"", aptr->bottom_name);
                fprintf (cf, " \"%s\"", aptr->right_name);
                fprintf (cf, " \"%s\"\n", aptr->left_name);
                break;
            case CONF_ADJ_ABSOLUTE:
                if (aptr->x != -1)
                    fprintf (cf, " %d %d\n", aptr->x, aptr->y);
                else
                    fprintf (cf, "\n");
                break;
            case CONF_ADJ_RIGHTOF:
                fprintf (cf, " RightOf \"%s\"\n", aptr->refscreen);
                break;
            case CONF_ADJ_LEFTOF:
                fprintf (cf, " LeftOf \"%s\"\n", aptr->refscreen);
                break;
            case CONF_ADJ_ABOVE:
                fprintf (cf, " Above \"%s\"\n", aptr->refscreen);
                break;
            case CONF_ADJ_BELOW:
                fprintf (cf, " Below \"%s\"\n", aptr->refscreen);
                break;
            case CONF_ADJ_RELATIVE:
                fprintf (cf, " Relative \"%s\" %d %d\n", aptr->refscreen,
                         aptr->x, aptr->y);
                break;
            }
        }
        for (iptr = ptr->inactives; iptr; iptr = iptr->next)
            fprintf (cf, "    Inactive       \"%s\"\n", iptr->device_name);
        for (inptr = ptr->inputs; inptr; inptr = inptr->next)
        {
            fprintf (cf, "    InputDevice    \"%s\"", inptr->input_name);
            for (optr = inptr->options; optr; optr = optr->next)
            {
                fprintf(cf, " \"%s\"", optr->name);
            }
            fprintf(cf, "\n");
        }
        xconfigPrintOptionList(cf, ptr->options, 1);
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigFreeLayoutList (XConfigLayoutPtr *ptr)
{
    XConfigLayoutPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->comment);
        xconfigFreeAdjacencyList (&((*ptr)->adjacencies));
        xconfigFreeInputrefList (&((*ptr)->inputs));
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

void
xconfigFreeAdjacencyList (XConfigAdjacencyPtr *ptr)
{
    XConfigAdjacencyPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->screen_name);
        TEST_FREE ((*ptr)->top_name);
        TEST_FREE ((*ptr)->bottom_name);
        TEST_FREE ((*ptr)->left_name);
        TEST_FREE ((*ptr)->right_name);

        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }

}

void
xconfigFreeInputrefList (XConfigInputrefPtr *ptr)
{
    XConfigInputrefPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->input_name);
        xconfigFreeOptionList (&((*ptr)->options));
        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }

}

#define CheckScreen(str, ptr)\
if (str[0] != '\0') \
{ \
screen = xconfigFindScreen (str, p->conf_screen_lst); \
if (!screen) \
{ \
    xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_SCREEN_MSG, \
                   str, layout->identifier); \
    return (FALSE); \
} \
else \
    ptr = screen; \
}

int
xconfigValidateLayout (XConfigPtr p)
{
    XConfigLayoutPtr layout = p->layouts;
    XConfigAdjacencyPtr adj;
    XConfigInactivePtr iptr;
    XConfigInputrefPtr inputRef;
    XConfigScreenPtr screen;
    XConfigDevicePtr device;
    XConfigInputPtr input;

    /*
     * if we do not have a layout, just return TRUE; we'll add a
     * layout later during the Sanitize step
     */

    if (!layout) return TRUE;

    while (layout)
    {
        adj = layout->adjacencies;
        while (adj)
        {
            /* the first one can't be "" but all others can */
            screen = xconfigFindScreen (adj->screen_name, p->screens);
            if (!screen)
            {
                xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_SCREEN_MSG,
                             adj->screen_name, layout->identifier);
                return (FALSE);
            }
            else
                adj->screen = screen;

#if 0
            CheckScreen (adj->top_name, adj->top);
            CheckScreen (adj->bottom_name, adj->bottom);
            CheckScreen (adj->left_name, adj->left);
            CheckScreen (adj->right_name, adj->right);
#endif

            adj = adj->next;
        }

        /* I not believe the "inactives" list is used for anything */
        
        iptr = layout->inactives;
        while (iptr)
        {
            device = xconfigFindDevice (iptr->device_name,
                                     p->devices);
            if (!device)
            {
                xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_DEVICE_MSG,
                             iptr->device_name, layout->identifier);
                return (FALSE);
            }
            else
                iptr->device = device;
            iptr = iptr->next;
        }

        /*
         * the layout->inputs list is also updated in
         * getCoreInputDevice() when no core input device is found in
         * the layout's input list
         */

        inputRef = layout->inputs;
        while (inputRef)
        {
            input = xconfigFindInput (inputRef->input_name,
                                   p->inputs);
            if (!input)
            {
                xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_INPUT_MSG,
                             inputRef->input_name, layout->identifier);
                return (FALSE);
            }
            else {
                inputRef->input = input;
            }
            inputRef = inputRef->next;
        }
        layout = layout->next;
    }
    return (TRUE);
}

int
xconfigSanitizeLayout(XConfigPtr p,
                      const char *screenName,
                      GenerateOptions *gop)
{
    XConfigLayoutPtr layout = p->layouts;
    
    /* add an implicit layout if none exist */

    if (!p->layouts) {
        if (!addImpliedLayout(p, screenName)) {
            return FALSE;
        }
    }

    /* check that input devices are assigned for each layout */
    
    for (layout = p->layouts; layout; layout = layout->next) {
        if (!xconfigCheckCoreInputDevices(gop, p, layout)) {
            return FALSE;
        }
    }

    return TRUE;
}

XConfigLayoutPtr
xconfigFindLayout (const char *name, XConfigLayoutPtr list)
{
    while (list)
    {
        if (xconfigNameCompare (list->identifier, name) == 0)
            return (list);
        list = list->next;
    }
    return (NULL);
}


static int addImpliedLayout(XConfigPtr config, const char *screenName)
{
    XConfigScreenPtr screen;
    XConfigLayoutPtr layout;
    XConfigAdjacencyPtr adj;

    if (config->layouts) return TRUE;

    /*
     * which screen section is the active one?
     *
     * If there is a -screen option, use that one, otherwise use the first
     * screen in the config's list.
     */
    
    if (screenName) {
        screen = xconfigFindScreen(screenName, config->screens);
        if (!screen) {
            xconfigErrorMsg(ErrorMsg, "No Screen section called \"%s\"\n",
                            screenName);
            return FALSE;
        }
    } else {
        screen = config->screens;
    }
    
    xconfigErrorMsg(WarnMsg, "No Layout specified, constructing implicit "
                    "layout section using screen \"%s\".\n",
                    screen->identifier);
    
    /* allocate the new layout section */
    
    layout = calloc(1, sizeof(XConfigLayoutRec));
    
    layout->identifier = xconfigStrdup("Default Layout");
    
    adj = calloc(1, sizeof(XConfigAdjacencyRec));
    adj->scrnum = -1;
    adj->screen = screen;
    adj->screen_name = xconfigStrdup(screen->identifier);
    
    layout->adjacencies = adj;

    config->layouts = layout;
    
    /* validate the Layout here to setup all the pointers */

    if (!xconfigValidateLayout(config)) return FALSE;

    return TRUE;
}
