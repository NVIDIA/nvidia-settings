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

static
XConfigSymTabRec InputTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {OPTION, "option"},
    {DRIVER, "driver"},
    {-1, ""},
};

static
XConfigSymTabRec InputClassTab[] =
{   {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {MATCHPRODUCT, "matchproduct"},
    {MATCHVENDOR, "matchvendor"},
    {MATCHOS, "matchos"},
    {MATCHDEVICEPATH, "matchdevicepath"},
    {MATCHPNPID, "matchpnpid"},
    {MATCHUSBID, "matchusbid"},
    {MATCHDRIVER, "matchdriver"},
    {MATCHTAG, "matchtag"},
    {MATCHISKEYBOARD, "matchiskeyboard"},
    {MATCHISJOYSTICK, "matchisjoystick"},
    {MATCHISTABLET, "matchistablet"},
    {MATCHISTOUCHSCREEN, "matchistouchscreen"},
    {MATCHISTOUCHPAD, "matchistouchpad"},
    {MATCHISPOINTER, "matchispointer"},
    {OPTION, "option"},
    {DRIVER, "driver"},
    {-1, ""},
};

#define CLEANUP xconfigFreeInputList

XConfigInputPtr
xconfigParseInputSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigInputPtr, XConfigInputRec)

    while ((token = xconfigGetToken (InputTab)) != ENDSECTION)
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
        case DRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Driver");
            ptr->driver = val.str;
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

#define CLEANUP xconfigFreeInputClassList

XConfigInputClassPtr
xconfigParseInputClassSection (void)
{
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigInputClassPtr, XConfigInputClassRec)

    while ((token = xconfigGetToken (InputClassTab)) != ENDSECTION)
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
        case DRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Driver");
            ptr->driver = val.str;
            break;
        case MATCHDEVICEPATH:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchDevicePath");
            ptr->match_device_path = val.str;
            break;
        case MATCHISPOINTER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsPointer");
            ptr->match_is_pointer = val.str;
            break;
        case MATCHISTOUCHPAD:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsTouchpad");
            ptr->match_is_touchpad = val.str;
            break;
        case MATCHISKEYBOARD:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsKeyboard");
            ptr->match_is_keyboard = val.str;
            break;
        case MATCHISTOUCHSCREEN:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsTouchscreen");
            ptr->match_is_touchscreen = val.str;
            break;
        case MATCHISJOYSTICK:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsJoystick");
            ptr->match_is_joystick = val.str;
            break;
        case MATCHISTABLET:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchIsTablet");
            ptr->match_is_tablet = val.str;
            break;
        case MATCHUSBID:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchUSBID");
            ptr->match_usb_id = val.str;
            break;
        case MATCHPNPID:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchPnPID");
            ptr->match_pnp_id = val.str;
            break;
        case MATCHPRODUCT:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchProduct");
            ptr->match_product = val.str;
            break;
        case MATCHDRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchDriver");
            ptr->match_driver = val.str;
            break;
        case MATCHOS:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchOS");
            ptr->match_os = val.str;
            break;
        case MATCHTAG:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchTag");
            ptr->match_tag = val.str;
            break;
        case MATCHVENDOR:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "MatchVendor");
            ptr->match_vendor = val.str;
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
xconfigPrintInputSection (FILE * cf, XConfigInputPtr ptr)
{
    while (ptr)
    {
        fprintf (cf, "Section \"InputDevice\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf (cf, "    Driver         \"%s\"\n", ptr->driver);
        xconfigPrintOptionList(cf, ptr->options, 1);
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigPrintInputClassSection (FILE * cf, XConfigInputClassPtr ptr)
{
    while (ptr)
    {
        fprintf (cf, "Section \"InputClass\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier         \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf (cf, "    Driver             \"%s\"\n", ptr->driver);
        if (ptr->match_is_pointer)
            fprintf (cf, "    MatchIsPointer     \"%s\"\n", ptr->match_is_pointer);
        if (ptr->match_is_touchpad)
            fprintf (cf, "    MatchIsTouchpad    \"%s\"\n", ptr->match_is_touchpad);
        if (ptr->match_is_keyboard)
            fprintf (cf, "    MatchIsKeyboard    \"%s\"\n", ptr->match_is_keyboard);
        if (ptr->match_is_joystick)
            fprintf (cf, "    MatchIsJoystick    \"%s\"\n", ptr->match_is_joystick);
        if (ptr->match_is_touchscreen)
            fprintf (cf, "    MatchIsTouchscreen \"%s\"\n", ptr->match_is_touchscreen);
        if (ptr->match_is_tablet)
            fprintf (cf, "    MatchIsTablet      \"%s\"\n", ptr->match_is_tablet);
        if (ptr->match_device_path)
            fprintf (cf, "    MatchDevicePath    \"%s\"\n", ptr->match_device_path);
        if (ptr->match_os)
            fprintf (cf, "    MatchOS            \"%s\"\n", ptr->match_os);
        if (ptr->match_pnp_id)
            fprintf (cf, "    MatchPnPID         \"%s\"\n", ptr->match_pnp_id);
        if (ptr->match_driver)
            fprintf (cf, "    MatchDriver        \"%s\"\n", ptr->match_driver);
        if (ptr->match_usb_id)
            fprintf (cf, "    MatchUSBID         \"%s\"\n", ptr->match_usb_id);
        if (ptr->match_tag)
            fprintf (cf, "    MatchTag           \"%s\"\n", ptr->match_tag);
        if (ptr->match_vendor)
            fprintf (cf, "    MatchVendor        \"%s\"\n", ptr->match_vendor);
        xconfigPrintOptionList(cf, ptr->options, 1);
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigFreeInputList (XConfigInputPtr *ptr)
{
    XConfigInputPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->driver);
        TEST_FREE ((*ptr)->comment);
        xconfigFreeOptionList (&((*ptr)->options));

        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

void
xconfigFreeInputClassList (XConfigInputClassPtr *ptr)
{
    XConfigInputClassPtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->driver);
        TEST_FREE ((*ptr)->comment);
        TEST_FREE ((*ptr)->match_product);
        TEST_FREE ((*ptr)->match_vendor);
        TEST_FREE ((*ptr)->match_driver);
        TEST_FREE ((*ptr)->match_device_path);
        TEST_FREE ((*ptr)->match_os);
        TEST_FREE ((*ptr)->match_pnp_id);
        TEST_FREE ((*ptr)->match_usb_id);
        TEST_FREE ((*ptr)->match_is_pointer);
        TEST_FREE ((*ptr)->match_is_touchpad);
        TEST_FREE ((*ptr)->match_is_touchscreen);
        TEST_FREE ((*ptr)->match_is_keyboard);
        TEST_FREE ((*ptr)->match_is_tablet);
        TEST_FREE ((*ptr)->match_is_joystick);
        xconfigFreeOptionList (&((*ptr)->options));

        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

int
xconfigValidateInput (XConfigPtr p)
{
    XConfigInputPtr input = p->inputs;

#if 0 /* Enable this later */
    if (!input) {
        xconfigErrorMsg(ValidationErrorMsg, "At least one InputDevice section "
                     "is required.");
        return (FALSE);
    }
#endif

    while (input) {
        if (!input->driver) {
            xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_INPUTDRIVER_MSG,
                         input->identifier);
            return (FALSE);
        }
        input = input->next;
    }
    return (TRUE);
}

XConfigInputPtr
xconfigFindInput (const char *ident, XConfigInputPtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}

XConfigInputPtr
xconfigFindInputByDriver (const char *driver, XConfigInputPtr p)
{
    while (p)
    {
        if (xconfigNameCompare (driver, p->driver) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}



static int getCoreInputDevice(GenerateOptions *gop,
                              XConfigPtr config,
                              XConfigLayoutPtr layout,
                              const int mouse,
                              const char *coreKeyword,
                              const char *implicitDriverName,
                              const char *defaultDriver0,
                              const char *defaultDriver1,
                              const char *foundMsg0,
                              const char *foundMsg1)
{
    XConfigInputPtr input, core = NULL;
    XConfigInputrefPtr inputRef;
    int found, firstTry;
    const char *found_msg = NULL;
    
    /*
     * First check if the core input device has been specified in the
     * active ServerLayout.  If more than one is specified, remove the
     * core attribute from the later ones.
     */
    
    for (inputRef = layout->inputs; inputRef; inputRef = inputRef->next) {
        XConfigOptionPtr opt1 = NULL, opt2 = NULL;
        
        input = inputRef->input;
        
        opt1 = xconfigFindOption(input->options, coreKeyword);
        opt2 = xconfigFindOption(inputRef->options, coreKeyword);

        if (opt1 || opt2) {
            if (!core) {
                core = input;
            } else {
                if (opt1) {
                    xconfigRemoveOption(&input->options, opt1);
                }
                if (opt2) {
                    xconfigRemoveOption(&inputRef->options, opt2);
                }
                xconfigErrorMsg(WarnMsg, "Duplicate %s devices; removing %s "
                             "attribute from \"%s\"\n",
                             coreKeyword, coreKeyword, input->identifier);
            }
        }
    }

    /*
     * XXX XFree86 allows the commandline to override the core input
     * devices; let's not bother with that, here.
     */

    /*
     * if we didn't find a core input device above in the
     * serverLayout, scan through the config's entire input list and
     * pick the first one with the coreKeyword.
     */

    if (!core) {
        for (input = config->inputs; input; input = input->next) {
            if (xconfigFindOption(input->options, coreKeyword)) {
                core = input;
                found_msg = foundMsg0;
                break;
            }
        }
    }
    
    /*
     * if we didn't find a core input device above, then select the
     * first input with the correct driver
     */
    
    firstTry = TRUE;

 tryAgain:
    
    if (!core) {
        input = xconfigFindInput(implicitDriverName, config->inputs);
        if (!input && defaultDriver0) {
            input = xconfigFindInputByDriver(defaultDriver0, config->inputs);
        }
        if (!input && defaultDriver1) {
            input = xconfigFindInputByDriver(defaultDriver1, config->inputs);
        }
        if (input) {
            core = input;
            found_msg = foundMsg1;
        }
    }

    /*
     * if we didn't find a core input device above, then that means we
     * don't have any input devices of this type; try to add a new
     * input device of this type, and then try again to find a core
     * input device
     */

    if (!core && firstTry) {
        firstTry = FALSE;
        
        xconfigErrorMsg(WarnMsg, "Unable to find %s in X configuration; "
                        "attempting to add new %s section.",
                        coreKeyword, coreKeyword);
        
        if (mouse) {
            xconfigAddMouse(gop, config);
        } else {
            xconfigAddKeyboard(gop, config);
        }
        goto tryAgain;
    }
    
    /*
     * if we *still* can't find a core input device, print a warning
     * message and give up; hopefully the X server's builtin config
     * will do.
     */
    
    if (!core) {
        xconfigErrorMsg(WarnMsg, "Unable to determine %s; will rely on X "
                        "server's built-in default configuration.",
                        coreKeyword);

        /* don't return FALSE here -- we don't want nvidia-xconfig to fail */
        
        return TRUE;
    }
    

    /*
     * make sure the core input device is in the layout's input list
     */

    found = FALSE;
    for (inputRef = layout->inputs; inputRef; inputRef = inputRef->next) {
        if (inputRef->input == core) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        inputRef = calloc(1, sizeof(XConfigInputrefRec));
        inputRef->input = core;
        inputRef->input_name = strdup(core->identifier);
        inputRef->next = layout->inputs;
        layout->inputs = inputRef;
    }
    
    /*
     * make sure the core input device has the core keyword set
     */

    for (inputRef = layout->inputs; inputRef; inputRef = inputRef->next) {
        if (inputRef->input == core) {
            XConfigOptionPtr opt1 = NULL, opt2 = NULL;

            opt1 = xconfigFindOption(inputRef->input->options, coreKeyword);
            opt2 = xconfigFindOption(inputRef->options, coreKeyword);

            if (!opt1 && !opt2) {
                xconfigAddNewOption(&inputRef->options, coreKeyword, NULL);
            }
            break;
        }
    }
    
    if (found_msg) {
        xconfigErrorMsg(WarnMsg, "The %s device was not specified explicitly "
                     "in the layout; using the %s.\n", coreKeyword, found_msg);
    }
    
    return TRUE;
}



/*
 * xconfigCheckCoreInputDevices() - check that the specified layout has a
 * corePointer and coreKeyboard.  If it does not have them, they will
 * be added from the current list of input devices.
 */

int xconfigCheckCoreInputDevices(GenerateOptions *gop,
                                 XConfigPtr config,
                                 XConfigLayoutPtr layout)
{
    int ret;

    ret = getCoreInputDevice(gop,
                             config,
                             layout,
                             TRUE,
                             "CorePointer",
                             CONF_IMPLICIT_POINTER,
                             "mouse", NULL,
                             "first CorePointer in the config input list",
                             "first mouse device");
    
    if (!ret) return FALSE;

    ret = getCoreInputDevice(gop,
                             config,
                             layout,
                             FALSE,
                             "CoreKeyboard",
                             CONF_IMPLICIT_KEYBOARD,
                             "keyboard", "kbd",
                             "first CoreKeyboard in the config input list",
                             "first keyboard device");
    if (!ret) return FALSE;

    return TRUE;
}
