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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <locale.h>


int xconfigWriteConfigFile (const char *filename, XConfigPtr cptr)
{
    FILE *cf;
    char *locale;
    
    if ((cf = fopen(filename, "w")) == NULL)
    {
        xconfigErrorMsg(WriteErrorMsg, "Unable to open the file \"%s\" for "
                     "writing (%s).\n", filename, strerror(errno));
        return FALSE;
    }

    /*
     * read the current locale and then set the standard "C" locale,
     * so that the X configuration writer does not use locale-specific
     * formatting.  After writing the configuration file, we restore
     * the original locale.
     */

    locale = setlocale(LC_ALL, NULL);
    
    if (locale) locale = strdup(locale);

    setlocale(LC_ALL, "C");
    
    
    if (cptr->comment)
        fprintf (cf, "%s\n", cptr->comment);

    xconfigPrintLayoutSection (cf, cptr->layouts);

    fprintf (cf, "Section \"Files\"\n");
    xconfigPrintFileSection (cf, cptr->files);
    fprintf (cf, "EndSection\n\n");

    fprintf (cf, "Section \"Module\"\n");
    xconfigPrintModuleSection (cf, cptr->modules);
    fprintf (cf, "EndSection\n\n");

    xconfigPrintVendorSection (cf, cptr->vendors);

    xconfigPrintServerFlagsSection (cf, cptr->flags);

    xconfigPrintInputSection (cf, cptr->inputs);

    xconfigPrintVideoAdaptorSection (cf, cptr->videoadaptors);

    xconfigPrintModesSection (cf, cptr->modes);

    xconfigPrintMonitorSection (cf, cptr->monitors);

    xconfigPrintDeviceSection (cf, cptr->devices);

    xconfigPrintScreenSection (cf, cptr->screens);

    xconfigPrintDRISection (cf, cptr->dri);

    xconfigPrintExtensionsSection (cf, cptr->extensions);

    fclose(cf);

    /* restore the original locale */

    if (locale) {
        setlocale(LC_ALL, locale);
        free(locale);
    }

    return TRUE;
}
