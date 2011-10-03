/*
 * Copyright (c) 1997-2001 by The XFree86 Project, Inc.
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

/* Private procs.  Public procs are in xf86Parser.h and xf86Optrec.h */

#include "xf86Parser.h"


/* Device.c */
XConfigDevicePtr xconfigParseDeviceSection(void);
void xconfigPrintDeviceSection(FILE *cf, XConfigDevicePtr ptr);
int xconfigValidateDevice(XConfigPtr p);

/* Files.c */
XConfigFilesPtr xconfigParseFilesSection(void);
void xconfigPrintFileSection(FILE *cf, XConfigFilesPtr ptr);

/* Flags.c */
XConfigFlagsPtr xconfigParseFlagsSection(void);
void xconfigPrintServerFlagsSection(FILE *f, XConfigFlagsPtr flags);

/* Input.c */
XConfigInputPtr xconfigParseInputSection(void);
XConfigInputClassPtr xconfigParseInputClassSection(void);
void xconfigPrintInputSection(FILE *f, XConfigInputPtr ptr);
void xconfigPrintInputClassSection(FILE *f, XConfigInputClassPtr ptr);
int xconfigValidateInput (XConfigPtr p);

/* Keyboard.c */
XConfigInputPtr xconfigParseKeyboardSection(void);

/* Layout.c */
XConfigLayoutPtr xconfigParseLayoutSection(void);
void xconfigPrintLayoutSection(FILE *cf, XConfigLayoutPtr ptr);
int xconfigValidateLayout(XConfigPtr p);
int xconfigSanitizeLayout(XConfigPtr p, const char *screenName,
                          GenerateOptions *gop);

/* Module.c */
XConfigLoadPtr xconfigParseModuleSubSection(XConfigLoadPtr head, char *name);
XConfigModulePtr xconfigParseModuleSection(void);
void xconfigPrintModuleSection(FILE *cf, XConfigModulePtr ptr);

/* Monitor.c */
XConfigModeLinePtr xconfigParseModeLine(void);
XConfigModeLinePtr xconfigParseVerboseMode(void);
XConfigMonitorPtr xconfigParseMonitorSection(void);
XConfigModesPtr xconfigParseModesSection(void);
void xconfigPrintMonitorSection(FILE *cf, XConfigMonitorPtr ptr);
void xconfigPrintModesSection(FILE *cf, XConfigModesPtr ptr);
int xconfigValidateMonitor(XConfigPtr p, XConfigScreenPtr screen);

/* Pointer.c */
XConfigInputPtr xconfigParsePointerSection(void);

/* Screen.c */
XConfigDisplayPtr xconfigParseDisplaySubSection(void);
XConfigScreenPtr xconfigParseScreenSection(void);
void xconfigPrintScreenSection(FILE *cf, XConfigScreenPtr ptr);
int xconfigValidateScreen(XConfigPtr p);
int xconfigSanitizeScreen(XConfigPtr p);

/* Vendor.c */
XConfigVendorPtr xconfigParseVendorSection(void);
XConfigVendSubPtr xconfigParseVendorSubSection (void);
void xconfigPrintVendorSection(FILE * cf, XConfigVendorPtr ptr);

/* Video.c */
XConfigVideoPortPtr xconfigParseVideoPortSubSection(void);
XConfigVideoAdaptorPtr xconfigParseVideoAdaptorSection(void);
void xconfigPrintVideoAdaptorSection(FILE *cf, XConfigVideoAdaptorPtr ptr);

/* Read.c */
int xconfigValidateConfig(XConfigPtr p);

/* Scan.c */
int xconfigGetToken(XConfigSymTabRec *tab);
int xconfigGetSubToken(char **comment);
int xconfigGetSubTokenWithTab(char **comment, XConfigSymTabRec *tab);
void xconfigUnGetToken(int token);
char *xconfigTokenString(void);
void xconfigSetSection(char *section);
int xconfigGetStringToken(XConfigSymTabRec *tab);
char *xconfigGetConfigFileName(void);

/* Write.c */

/* DRI.c */
XConfigBuffersPtr xconfigParseBuffers (void);
XConfigDRIPtr xconfigParseDRISection (void);
void xconfigPrintDRISection (FILE * cf, XConfigDRIPtr ptr);

/* Util.c */
void *xconfigAlloc(size_t size);
void xconfigErrorMsg(MsgType, char *fmt, ...);

/* Extensions.c */
XConfigExtensionsPtr xconfigParseExtensionsSection (void);
void xconfigPrintExtensionsSection (FILE * cf, XConfigExtensionsPtr ptr);

/* Generate.c */
XConfigMonitorPtr xconfigAddMonitor(XConfigPtr config, int count);
int xconfigAddMouse(GenerateOptions *gop, XConfigPtr config);
int xconfigAddKeyboard(GenerateOptions *gop, XConfigPtr config);
