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

#include <ctype.h>

extern LexRec val;

static
XConfigSymTabRec DeviceTab[] =
{
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {VENDOR, "vendorname"},
    {BOARD, "boardname"},
    {CHIPSET, "chipset"},
    {RAMDAC, "ramdac"},
    {DACSPEED, "dacspeed"},
    {CLOCKS, "clocks"},
    {OPTION, "option"},
    {VIDEORAM, "videoram"},
    {BIOSBASE, "biosbase"},
    {MEMBASE, "membase"},
    {IOBASE, "iobase"},
    {CLOCKCHIP, "clockchip"},
    {CHIPID, "chipid"},
    {CHIPREV, "chiprev"},
    {CARD, "card"},
    {DRIVER, "driver"},
    {BUSID, "busid"},
    {TEXTCLOCKFRQ, "textclockfreq"},
    {IRQ, "irq"},
    {SCREEN, "screen"},
    {-1, ""},
};

#define CLEANUP xconfigFreeDeviceList

XConfigDevicePtr
xconfigParseDeviceSection (void)
{
    int i;
    int has_ident = FALSE;
    int token;
    PARSE_PROLOGUE (XConfigDevicePtr, XConfigDeviceRec)

    /* Zero is a valid value for these */
    ptr->chipid = -1;
    ptr->chiprev = -1;
    ptr->irq = -1;
    ptr->screen = -1;
    while ((token = xconfigGetToken (DeviceTab)) != ENDSECTION)
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
        case BOARD:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Board");
            ptr->board = val.str;
            break;
        case CHIPSET:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Chipset");
            ptr->chipset = val.str;
            break;
        case CARD:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Card");
            ptr->card = val.str;
            break;
        case DRIVER:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Driver");
            ptr->driver = val.str;
            break;
        case RAMDAC:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "Ramdac");
            ptr->ramdac = val.str;
            break;
        case DACSPEED:
            for (i = 0; i < CONF_MAXDACSPEEDS; i++)
                ptr->dacSpeeds[i] = 0;
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
            {
                Error (DACSPEED_MSG, CONF_MAXDACSPEEDS);
            }
            else
            {
                ptr->dacSpeeds[0] = (int) (val.realnum * 1000.0 + 0.5);
                for (i = 1; i < CONF_MAXDACSPEEDS; i++)
                {
                    if (xconfigGetSubToken (&(ptr->comment)) == NUMBER)
                        ptr->dacSpeeds[i] = (int)
                            (val.realnum * 1000.0 + 0.5);
                    else
                    {
                        xconfigUnGetToken (token);
                        break;
                    }
                }
            }
            break;
        case VIDEORAM:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "VideoRam");
            ptr->videoram = val.num;
            break;
        case BIOSBASE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "BIOSBase");
            ptr->bios_base = val.num;
            break;
        case MEMBASE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "MemBase");
            ptr->mem_base = val.num;
            break;
        case IOBASE:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "IOBase");
            ptr->io_base = val.num;
            break;
        case CLOCKCHIP:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "ClockChip");
            ptr->clockchip = val.str;
            break;
        case CHIPID:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "ChipID");
            ptr->chipid = val.num;
            break;
        case CHIPREV:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "ChipRev");
            ptr->chiprev = val.num;
            break;

        case CLOCKS:
            token = xconfigGetSubToken(&(ptr->comment));
            for( i = ptr->clocks;
                token == NUMBER && i < CONF_MAXCLOCKS; i++ ) {
                ptr->clock[i] = (int)(val.realnum * 1000.0 + 0.5);
                token = xconfigGetSubToken(&(ptr->comment));
            }
            ptr->clocks = i;
            xconfigUnGetToken (token);
            break;
        case TEXTCLOCKFRQ:
            if ((token = xconfigGetSubToken(&(ptr->comment))) != NUMBER)
                Error (NUMBER_MSG, "TextClockFreq");
            ptr->textclockfreq = (int)(val.realnum * 1000.0 + 0.5);
            break;
        case OPTION:
            ptr->options = xconfigParseOption(ptr->options);
            break;
        case BUSID:
            if (xconfigGetSubToken (&(ptr->comment)) != STRING)
                Error (QUOTE_MSG, "BusID");
            ptr->busid = val.str;
            break;
        case IRQ:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (QUOTE_MSG, "IRQ");
            ptr->irq = val.num;
            break;
        case SCREEN:
            if (xconfigGetSubToken (&(ptr->comment)) != NUMBER)
                Error (NUMBER_MSG, "Screen");
            ptr->screen = val.num;
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
xconfigPrintDeviceSection (FILE * cf, XConfigDevicePtr ptr)
{
    int i;

    while (ptr)
    {
        fprintf (cf, "Section \"Device\"\n");
        if (ptr->comment)
            fprintf (cf, "%s", ptr->comment);
        if (ptr->identifier)
            fprintf (cf, "    Identifier     \"%s\"\n", ptr->identifier);
        if (ptr->driver)
            fprintf (cf, "    Driver         \"%s\"\n", ptr->driver);
        if (ptr->vendor)
            fprintf (cf, "    VendorName     \"%s\"\n", ptr->vendor);
        if (ptr->board)
            fprintf (cf, "    BoardName      \"%s\"\n", ptr->board);
        if (ptr->chipset)
            fprintf (cf, "    ChipSet        \"%s\"\n", ptr->chipset);
        if (ptr->card)
            fprintf (cf, "    Card           \"%s\"\n", ptr->card);
        if (ptr->ramdac)
            fprintf (cf, "    RamDac         \"%s\"\n", ptr->ramdac);
        if (ptr->dacSpeeds[0] > 0 ) {
            fprintf (cf, "    DacSpeed    ");
            for (i = 0; i < CONF_MAXDACSPEEDS
                    && ptr->dacSpeeds[i] > 0; i++ )
                fprintf (cf, "%g ", (double) (ptr->dacSpeeds[i])/ 1000.0 );
            fprintf (cf, "\n");
        }
        if (ptr->videoram)
            fprintf (cf, "    VideoRam        %d\n", ptr->videoram);
        if (ptr->bios_base)
            fprintf (cf, "    BiosBase        0x%lx\n", ptr->bios_base);
        if (ptr->mem_base)
            fprintf (cf, "    MemBase         0x%lx\n", ptr->mem_base);
        if (ptr->io_base)
            fprintf (cf, "    IOBase          0x%lx\n", ptr->io_base);
        if (ptr->clockchip)
            fprintf (cf, "    ClockChip      \"%s\"\n", ptr->clockchip);
        if (ptr->chipid != -1)
            fprintf (cf, "    ChipId          0x%x\n", ptr->chipid);
        if (ptr->chiprev != -1)
            fprintf (cf, "    ChipRev         0x%x\n", ptr->chiprev);

        xconfigPrintOptionList(cf, ptr->options, 1);
        if (ptr->clocks > 0 ) {
            fprintf (cf, "    Clocks      ");
            for (i = 0; i < ptr->clocks; i++ )
                fprintf (cf, "%.1f ", (double)ptr->clock[i] / 1000.0 );
            fprintf (cf, "\n");
        }
        if (ptr->textclockfreq) {
            fprintf (cf, "    TextClockFreq %.1f\n",
                     (double)ptr->textclockfreq / 1000.0);
        }
        if (ptr->busid)
            fprintf (cf, "    BusID          \"%s\"\n", ptr->busid);
        if (ptr->screen > -1)
            fprintf (cf, "    Screen          %d\n", ptr->screen);
        if (ptr->irq >= 0)
            fprintf (cf, "    IRQ             %d\n", ptr->irq);
        fprintf (cf, "EndSection\n\n");
        ptr = ptr->next;
    }
}

void
xconfigFreeDeviceList (XConfigDevicePtr *ptr)
{
    XConfigDevicePtr prev;

    if (ptr == NULL || *ptr == NULL)
        return;

    while (*ptr)
    {
        TEST_FREE ((*ptr)->identifier);
        TEST_FREE ((*ptr)->vendor);
        TEST_FREE ((*ptr)->board);
        TEST_FREE ((*ptr)->chipset);
        TEST_FREE ((*ptr)->card);
        TEST_FREE ((*ptr)->driver);
        TEST_FREE ((*ptr)->ramdac);
        TEST_FREE ((*ptr)->clockchip);
        TEST_FREE ((*ptr)->comment);
        xconfigFreeOptionList (&((*ptr)->options));

        prev = *ptr;
        *ptr = (*ptr)->next;
        free (prev);
    }
}

int
xconfigValidateDevice (XConfigPtr p)
{
    XConfigDevicePtr device = p->devices;

    if (!device) {
        xconfigErrorMsg(ValidationErrorMsg, "At least one Device section "
                     "is required.");
        return (FALSE);
    }

    while (device) {
        if (!device->driver) {
            xconfigErrorMsg(ValidationErrorMsg, UNDEFINED_DRIVER_MSG,
                         device->identifier);
            return (FALSE);
        }
    device = device->next;
    }
    return (TRUE);
}

XConfigDevicePtr
xconfigFindDevice (const char *ident, XConfigDevicePtr p)
{
    while (p)
    {
        if (xconfigNameCompare (ident, p->identifier) == 0)
            return (p);

        p = p->next;
    }
    return (NULL);
}


/*
 * Determine what bus type the busID string represents.  The start of the
 * bus-dependent part of the string is returned as retID.
 */

static int isPci(const char* busID, const char **retID)
{
    char *p, *s;
    int ret = FALSE;

    /* If no type field, Default to PCI */
    if (isdigit(busID[0])) {
	if (retID)
	    *retID = busID;
	return TRUE;
    }
    
    s = strdup(busID);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return FALSE;
    }
    if (!xconfigNameCompare(p, "pci") || !xconfigNameCompare(p, "agp")) {
        if (retID)
	    *retID = busID + strlen(p) + 1;
        ret = TRUE;
    }
    free(s);
    return ret;
}


/*
 * Parse a BUS ID string, and return the PCI bus parameters if it was
 * in the correct format for a PCI bus id.
 */

int xconfigParsePciBusString(const char *busID,
                             int *bus, int *device, int *func)
{
    /*
     * The format is assumed to be "bus[@domain]:device[:func]", where domain,
     * bus, device and func are decimal integers.  domain and func may be
     * omitted and assumed to be zero, although doing this isn't encouraged.
     */
    
    char *p, *s, *d;
    const char *id;
    int i;
    
    if (!isPci(busID, &id))
	return FALSE;
    
    s = strdup(id);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return FALSE;
    }
    d = strpbrk(p, "@");
    if (d != NULL) {
	*(d++) = 0;
	for (i = 0; d[i] != 0; i++) {
	    if (!isdigit(d[i])) {
		free(s);
		return FALSE;
	    }
	}
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *bus = atoi(p);
    if (d != NULL && *d != 0)
	*bus += atoi(d) << 8;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *device = atoi(p);
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return TRUE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *func = atoi(p);
    free(s);
    return TRUE;
}

