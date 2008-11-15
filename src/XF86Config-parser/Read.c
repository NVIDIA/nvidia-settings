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


#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static XConfigSymTabRec TopLevelTab[] =
{
    {SECTION, "section"},
    {-1, ""},
};


#define CLEANUP xconfigFreeConfig

#define READ_HANDLE_RETURN(f,func)         \
    if ((ptr->f=func) == NULL) {           \
        xconfigFreeConfig(&ptr);           \
        return XCONFIG_RETURN_PARSE_ERROR; \
    }

#define READ_HANDLE_LIST(field,func,type)                               \
{                                                                       \
    type p = func();                                                    \
    if (p == NULL) {                                                    \
        xconfigFreeConfig(&ptr);                                        \
        return XCONFIG_RETURN_PARSE_ERROR;                              \
    } else {                                                            \
        xconfigAddListItem((GenericListPtr *)(&ptr->field),             \
                           (GenericListPtr) p);                         \
    }                                                                   \
}

#define READ_ERROR(a,b)                       \
    do {                                      \
        xconfigErrorMsg(ParseErrorMsg, a, b); \
        xconfigFreeConfig(&ptr);              \
        return XCONFIG_RETURN_PARSE_ERROR;    \
    } while (0)



/*
 * xconfigReadConfigFile() - read the open XConfig file, returning the
 * parsed data as XConfigPtr.
 */

XConfigError xconfigReadConfigFile(XConfigPtr *configPtr)
{
    int token;
    XConfigPtr ptr = NULL;

    *configPtr = NULL;

    ptr = xconfigAlloc(sizeof(XConfigRec));
    
    while ((token = xconfigGetToken(TopLevelTab)) != EOF_TOKEN) {
        
        switch (token) {
            
        case COMMENT:
            ptr->comment = xconfigAddComment(ptr->comment, val.str);
            break;
            
        case SECTION:
            if (xconfigGetSubToken(&(ptr->comment)) != STRING) {
                xconfigErrorMsg(ParseErrorMsg, QUOTE_MSG, "Section");
                xconfigFreeConfig(&ptr);
                return XCONFIG_RETURN_PARSE_ERROR;
            }
            
            xconfigSetSection(val.str);
            
            if (xconfigNameCompare(val.str, "files") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_RETURN(files, xconfigParseFilesSection());
            }
            else if (xconfigNameCompare(val.str, "serverflags") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_RETURN(flags, xconfigParseFlagsSection());
            }
            else if (xconfigNameCompare(val.str, "keyboard") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(inputs, xconfigParseKeyboardSection,
                                 XConfigInputPtr);
            }
            else if (xconfigNameCompare(val.str, "pointer") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(inputs, xconfigParsePointerSection,
                                 XConfigInputPtr);
            }
            else if (xconfigNameCompare(val.str, "videoadaptor") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(videoadaptors,
                            xconfigParseVideoAdaptorSection,
                                 XConfigVideoAdaptorPtr);
            }
            else if (xconfigNameCompare(val.str, "device") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(devices, xconfigParseDeviceSection,
                                 XConfigDevicePtr);
            }
            else if (xconfigNameCompare(val.str, "monitor") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(monitors, xconfigParseMonitorSection,
                                 XConfigMonitorPtr);
            }
            else if (xconfigNameCompare(val.str, "modes") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(modes, xconfigParseModesSection,
                                 XConfigModesPtr);
            }
            else if (xconfigNameCompare(val.str, "screen") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(screens, xconfigParseScreenSection,
                                 XConfigScreenPtr);
            }
            else if (xconfigNameCompare(val.str, "inputdevice") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(inputs, xconfigParseInputSection,
                                 XConfigInputPtr);
            }
            else if (xconfigNameCompare(val.str, "module") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_RETURN(modules, xconfigParseModuleSection());
            }
            else if (xconfigNameCompare(val.str, "serverlayout") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(layouts, xconfigParseLayoutSection,
                                 XConfigLayoutPtr);
            }
            else if (xconfigNameCompare(val.str, "vendor") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_LIST(vendors, xconfigParseVendorSection,
                                 XConfigVendorPtr);
            }
            else if (xconfigNameCompare(val.str, "dri") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_RETURN(dri, xconfigParseDRISection());
            }
            else if (xconfigNameCompare (val.str, "extensions") == 0)
            {
                free(val.str);
                val.str = NULL;
                READ_HANDLE_RETURN(extensions, xconfigParseExtensionsSection());
            }
            else
            {
                READ_ERROR(INVALID_SECTION_MSG, xconfigTokenString());
                free(val.str);
                val.str = NULL;
            }
            break;
            
        default:
            READ_ERROR(INVALID_KEYWORD_MSG, xconfigTokenString());
            free(val.str);
            val.str = NULL;
        }
    }

    if (xconfigValidateConfig(ptr)) {
        ptr->filename = strdup(xconfigGetConfigFileName());
        *configPtr = ptr;
        return XCONFIG_RETURN_SUCCESS;
    } else {
        xconfigFreeConfig(&ptr);
        return XCONFIG_RETURN_VALIDATION_ERROR;
    }
}

#undef CLEANUP


/* 
 * This function resolves name references and reports errors if the named
 * objects cannot be found.
 */

int xconfigValidateConfig(XConfigPtr p)
{
    if (!xconfigValidateDevice(p))
        return FALSE;
    if (!xconfigValidateScreen(p))
        return FALSE;
    if (!xconfigValidateInput(p))
        return FALSE;
    if (!xconfigValidateLayout(p))
        return FALSE;
    
    return(TRUE);
}



/*
 * This function fixes up any problems that it finds in the config,
 * when possible.
 */

int xconfigSanitizeConfig(XConfigPtr p,
                          const char *screenName,
                          GenerateOptions *gop)
{
    if (!xconfigSanitizeScreen(p))
        return FALSE;
    
    if (!xconfigSanitizeLayout(p, screenName, gop))
        return FALSE;
    
    return TRUE;
}



/* 
 * adds an item to the end of the linked list. Any record whose first field
 * is a GenericListRec can be cast to this type and used with this function.
 */
void xconfigAddListItem (GenericListPtr *pHead, GenericListPtr new)
{
    GenericListPtr p = *pHead;
    GenericListPtr last = NULL;

    while (p) {
        last = p;
        p = p->next;
    }

    if (last) {
        last->next = new;
    } else {
        *pHead = new;
    }
}


/*
 * removes an item from the linked list (but does not delete it). Any record
 * whose first field is a GenericListRec can be cast to this type and used
 * with this function.
 */
void xconfigRemoveListItem (GenericListPtr *pHead, GenericListPtr item)
{
    GenericListPtr cur = *pHead;
    GenericListPtr prev = NULL;

    while (cur) {
        if (cur == item) {
            if (prev) {
                prev->next = item->next;
            } else {
                *pHead = item->next;
            }
            return;
        }
        prev = cur;
        cur  = cur->next;
    }
}


/* 
 * Test if one chained list contains the other.
 * In this case both list have the same endpoint (provided they don't loop)
 */
int
xconfigItemNotSublist(GenericListPtr list_1, GenericListPtr list_2)
{
    GenericListPtr p = list_1;
    GenericListPtr last_1 = NULL, last_2 = NULL;

    while (p) {
        last_1 = p;
        p = p->next;
    }

    p = list_2;
    while (p) {
        last_2 = p;
        p = p->next;
    }

    return (!(last_1 == last_2));
}

void
xconfigFreeConfig (XConfigPtr *p)
{
    if (p == NULL || *p == NULL)
        return;

    xconfigFreeFiles (&((*p)->files));
    xconfigFreeModules (&((*p)->modules));
    xconfigFreeFlags (&((*p)->flags));
    xconfigFreeMonitorList (&((*p)->monitors));
    xconfigFreeModesList (&((*p)->modes));
    xconfigFreeVideoAdaptorList (&((*p)->videoadaptors));
    xconfigFreeDeviceList (&((*p)->devices));
    xconfigFreeScreenList (&((*p)->screens));
    xconfigFreeLayoutList (&((*p)->layouts));
    xconfigFreeInputList (&((*p)->inputs));
    xconfigFreeVendorList (&((*p)->vendors));
    xconfigFreeDRI (&((*p)->dri));
    TEST_FREE((*p)->comment);

    free (*p);
    *p = NULL;
}
