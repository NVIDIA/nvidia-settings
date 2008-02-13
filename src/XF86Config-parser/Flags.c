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
#include <math.h>

extern LexRec val;

static XConfigSymTabRec ServerFlagsTab[] =
{
    {ENDSECTION, "endsection"},
    {NOTRAPSIGNALS, "notrapsignals"},
    {DONTZAP, "dontzap"},
    {DONTZOOM, "dontzoom"},
    {DISABLEVIDMODE, "disablevidmodeextension"},
    {ALLOWNONLOCAL, "allownonlocalxvidtune"},
    {DISABLEMODINDEV, "disablemodindev"},
    {MODINDEVALLOWNONLOCAL, "allownonlocalmodindev"},
    {ALLOWMOUSEOPENFAIL, "allowmouseopenfail"},
    {OPTION, "option"},
    {BLANKTIME, "blanktime"},
    {STANDBYTIME, "standbytime"},
    {SUSPENDTIME, "suspendtime"},
    {OFFTIME, "offtime"},
    {DEFAULTLAYOUT, "defaultserverlayout"},
    {-1, ""},
};

#define CLEANUP xconfigFreeFlags

XConfigFlagsPtr
xconfigParseFlagsSection (void)
{
    int token;
    PARSE_PROLOGUE (XConfigFlagsPtr, XConfigFlagsRec)

    while ((token = xconfigGetToken (ServerFlagsTab)) != ENDSECTION)
    {
        int hasvalue = FALSE;
        int strvalue = FALSE;
        int tokentype;
        switch (token)
        {
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
            /* 
             * these old keywords are turned into standard generic options.
             * we fall through here on purpose
             */
        case DEFAULTLAYOUT:
            strvalue = TRUE;
        case BLANKTIME:
        case STANDBYTIME:
        case SUSPENDTIME:
        case OFFTIME:
            hasvalue = TRUE;
        case NOTRAPSIGNALS:
        case DONTZAP:
        case DONTZOOM:
        case DISABLEVIDMODE:
        case ALLOWNONLOCAL:
        case DISABLEMODINDEV:
        case MODINDEVALLOWNONLOCAL:
        case ALLOWMOUSEOPENFAIL:
            {
                int i = 0;
                while (ServerFlagsTab[i].token != -1)
                {
                    char *tmp;

                    if (ServerFlagsTab[i].token == token)
                    {
                        char *valstr = NULL;
                        /* can't use strdup because it calls malloc */
                        tmp = xconfigStrdup (ServerFlagsTab[i].name);
                        if (hasvalue)
                        {
                            tokentype = xconfigGetSubToken(&(ptr->comment));
                            if (strvalue) {
                                if (tokentype != STRING)
                                    Error (QUOTE_MSG, tmp);
                                valstr = val.str;
                            } else {
                                if (tokentype != NUMBER)
                                    Error (NUMBER_MSG, tmp);
                                valstr = malloc(16);
                                if (valstr)
                                    sprintf(valstr, "%d", val.num);
                            }
                        }
                        ptr->options = xconfigAddNewOption
                            (ptr->options, tmp, valstr);
                    }
                    i++;
                }
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

void
xconfigPrintServerFlagsSection (FILE * f, XConfigFlagsPtr flags)
{
    XConfigOptionPtr p;

    if ((!flags) || (!flags->options))
        return;
    p = flags->options;
    fprintf (f, "Section \"ServerFlags\"\n");
    if (flags->comment)
        fprintf (f, "%s", flags->comment);
    xconfigPrintOptionList(f, p, 1);
    fprintf (f, "EndSection\n\n");
}

static XConfigOptionPtr
addNewOption2 (XConfigOptionPtr head, char *name, char *val, int used)
{
    XConfigOptionPtr new, old = NULL;

    /* Don't allow duplicates */
    if (head != NULL && (old = xconfigFindOption(head, name)) != NULL) {
        TEST_FREE(old->name);
        TEST_FREE(old->val);
        new = old;
    } else {
        new = calloc (1, sizeof (XConfigOptionRec));
        new->next = NULL;
    }
    new->name = name;
    new->val = val;
    new->used = used;
    
    if (old == NULL)
        return ((XConfigOptionPtr) xconfigAddListItem ((GenericListPtr) head,
                                                       (GenericListPtr) new));
    else 
        return head;
}

XConfigOptionPtr
xconfigAddNewOption (XConfigOptionPtr head, char *name, char *val)
{
    return addNewOption2(head, name, val, 0);
}

void
xconfigFreeFlags (XConfigFlagsPtr flags)
{
    if (flags == NULL)
        return;
    xconfigOptionListFree (flags->options);
    TEST_FREE(flags->comment);
    free (flags);
}

XConfigOptionPtr
xconfigOptionListDup (XConfigOptionPtr opt)
{
    XConfigOptionPtr newopt = NULL;

    while (opt)
    {
        newopt = xconfigAddNewOption(newopt, xconfigStrdup(opt->name), 
                      xconfigStrdup(opt->val));
        newopt->used = opt->used;
        if (opt->comment)
            newopt->comment = xconfigStrdup(opt->comment);
        opt = opt->next;
    }
    return newopt;
}

void
xconfigOptionListFree (XConfigOptionPtr opt)
{
    XConfigOptionPtr prev;

    while (opt)
    {
        TEST_FREE (opt->name);
        TEST_FREE (opt->val);
        TEST_FREE (opt->comment);
        prev = opt;
        opt = opt->next;
        free (prev);
    }
}

char *
xconfigOptionName(XConfigOptionPtr opt)
{
    if (opt)
        return opt->name;
    return 0;
}

char *
xconfigOptionValue(XConfigOptionPtr opt)
{
    if (opt)
        return opt->val;
    return 0;
}

XConfigOptionPtr
xconfigNewOption(char *name, char *value)
{
    XConfigOptionPtr opt;

    opt = calloc(1, sizeof (XConfigOptionRec));
    if (!opt)
        return NULL;

    opt->used = 0;
    opt->next = 0;
    opt->name = name;
    opt->val = value;

    return opt;
}

XConfigOptionPtr
xconfigRemoveOption(XConfigOptionPtr list, XConfigOptionPtr opt)
{
    XConfigOptionPtr prev = NULL;
    XConfigOptionPtr p = list;

    while (p) {
        if (p == opt) {
            if (prev) prev->next = opt->next;
            if (list == opt) list = opt->next;

            TEST_FREE(opt->name);
            TEST_FREE(opt->val);
            TEST_FREE(opt->comment);
            free(opt);
            break;
        }
        prev = p;
        p = p->next;
    }

    return list;
}

XConfigOptionPtr
xconfigNextOption(XConfigOptionPtr list)
{
    if (!list)
        return NULL;
    return list->next;
}

/*
 * this function searches the given option list for the named option and
 * returns a pointer to the option rec if found. If not found, it returns
 * NULL
 */

XConfigOptionPtr
xconfigFindOption (XConfigOptionPtr list, const char *name)
{
    while (list)
    {
        if (xconfigNameCompare (list->name, name) == 0)
            return (list);
        list = list->next;
    }
    return (NULL);
}

/*
 * this function searches the given option list for the named option. If
 * found and the option has a parameter, a pointer to the parameter is
 * returned.  If the option does not have a parameter an empty string is
 * returned.  If the option is not found, a NULL is returned.
 */

char *
xconfigFindOptionValue (XConfigOptionPtr list, const char *name)
{
    XConfigOptionPtr p = xconfigFindOption (list, name);

    if (p)
    {
        if (p->val)
            return (p->val);
        else
            return "";
    }
    return (NULL);
}

/*
 * this function searches the given option list for the named option. If
 * found and the the value of the option is set to "1", "ON", "YES" or
 * "TRUE", 1 is returned.  Otherwise, 0 is returned.
 */

int
xconfigFindOptionBoolean (XConfigOptionPtr list, const char *name)
{
    XConfigOptionPtr p = xconfigFindOption (list, name);

    if (p && p->val)
    {
        if ( strcasecmp(p->val, "1")    == 0 ||
             strcasecmp(p->val, "ON")   == 0 ||
             strcasecmp(p->val, "YES")  == 0 ||
             strcasecmp(p->val, "TRUE") == 0 )
        {
            return 1;
        }
    }
    return 0;
}

XConfigOptionPtr
xconfigOptionListCreate( const char **options, int count, int used )
{
    XConfigOptionPtr p = NULL;
    char *t1, *t2;
    int i;

    if (count == -1)
    {
        for (count = 0; options[count]; count++)
            ;
    }
    if( (count % 2) != 0 )
    {
        xconfigErrorMsg(InternalErrorMsg, "xconfigOptionListCreate: count must "
                     "be an even number.\n");
        return (NULL);
    }
    for (i = 0; i < count; i += 2)
    {
        /* can't use strdup because it calls malloc */
        t1 = malloc (sizeof (char) *
                (strlen (options[i]) + 1));
        strcpy (t1, options[i]);
        t2 = malloc (sizeof (char) *
                (strlen (options[i + 1]) + 1));
        strcpy (t2, options[i + 1]);
        p = addNewOption2 (p, t1, t2, used);
    }

    return (p);
}

/* the 2 given lists are merged. If an option with the same name is present in
 * both, the option from the user list - specified in the second argument -
 * is used. The end result is a single valid list of options. Duplicates
 * are freed, and the original lists are no longer guaranteed to be complete.
 */
XConfigOptionPtr
xconfigOptionListMerge (XConfigOptionPtr head, XConfigOptionPtr tail)
{
    XConfigOptionPtr a, b, ap = NULL, bp = NULL;

    a = tail;
    b = head;
    while (tail && b) {
        if (xconfigNameCompare (a->name, b->name) == 0) {
            if (b == head)
                head = a;
            else
                bp->next = a;
            if (a == tail)
                tail = a->next;
            else
                ap->next = a->next;
            a->next = b->next;
            b->next = NULL;
            xconfigOptionListFree (b);
            b = a->next;
            bp = a;
            a = tail;
            ap = NULL;
        } else {
            ap = a;
            if (!(a = a->next)) {
                a = tail;
                bp = b;
                b = b->next;
                ap = NULL;
            }
        }
    }

    if (head) {
        for (a = head; a->next; a = a->next)
            ;
        a->next = tail;
    } else 
        head = tail;

    return (head);
}

char *
xconfigULongToString(unsigned long i)
{
    char *s;
    int l;

    l = (int)(ceil(log10((double)i) + 2.5));
    s = malloc(l);
    if (!s)
        return NULL;
    sprintf(s, "%lu", i);
    return s;
}

XConfigOptionPtr
xconfigParseOption(XConfigOptionPtr head)
{
    XConfigOptionPtr option, cnew, old;
    char *name, *comment = NULL;
    int token;

    if ((token = xconfigGetSubToken(&comment)) != STRING) {
        xconfigErrorMsg(ParseErrorMsg, BAD_OPTION_MSG);
        if (comment)
            free(comment);
        return (head);
    }

    name = val.str;
    if ((token = xconfigGetSubToken(&comment)) == STRING) {
        option = xconfigNewOption(name, val.str);
        option->comment = comment;
        if ((token = xconfigGetToken(NULL)) == COMMENT)
            option->comment = xconfigAddComment(option->comment, val.str);
        else
            xconfigUnGetToken(token);
    }
    else {
        option = xconfigNewOption(name, NULL);
        option->comment = comment;
        if (token == COMMENT)
            option->comment = xconfigAddComment(option->comment, val.str);
        else
            xconfigUnGetToken(token);
    }

    old = NULL;

    /* Don't allow duplicates */
    if (head != NULL && (old = xconfigFindOption(head, name)) != NULL) {
        cnew = old;
        free(option->name);
        TEST_FREE(option->val);
        TEST_FREE(option->comment);
        free(option);
    }
    else
        cnew = option;
    
    if (old == NULL)
        return ((XConfigOptionPtr)xconfigAddListItem((GenericListPtr)head,
                                               (GenericListPtr)cnew));

    return (head);
}

void
xconfigPrintOptionList(FILE *fp, XConfigOptionPtr list, int tabs)
{
    int i;

    if (!list)
        return;
    while (list) {
        for (i = 0; i < tabs; i++)
            fprintf(fp, "    ");
        if (list->val)
            fprintf(fp, "Option         \"%s\" \"%s\"", list->name, list->val);
        else
            fprintf(fp, "Option         \"%s\"", list->name);
        if (list->comment)
            fprintf(fp, "%s", list->comment);
        else
            fputc('\n', fp);
        list = list->next;
    }
}
