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


/* 
 * This file specifies the external interface for the X configuration
 * file parser; based loosely on the XFree86 and Xorg X server
 * configuration code.
 */


#ifndef _xf86Parser_h_
#define _xf86Parser_h_

#include <stdio.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Unix variations: Linux
#if !defined(NV_LINUX) && defined(__linux__)
#   define NV_LINUX
#endif  // defined(__linux__)

// Unix variations:  SunOS
#if !defined(NV_SUNOS) && defined(__sun__) || defined(__sun)
#   define NV_SUNOS
#endif // defined(__sun__)

// Unix variations: FreeBSD
#if !defined(NV_BSD) && defined(__FreeBSD__)
#   define NV_BSD
#endif // defined(__FreeBSD__)


/*
 * return codes
 */

typedef enum {
    XCONFIG_RETURN_SUCCESS = 0,
    XCONFIG_RETURN_NO_XCONFIG_FOUND,
    XCONFIG_RETURN_PARSE_ERROR,
    XCONFIG_RETURN_ALLOCATION_ERROR,
    XCONFIG_RETURN_VALIDATION_ERROR,
    XCONFIG_RETURN_INVALID_COMMAND_LINE,
    XCONFIG_RETURN_SANITY_ERROR,
    XCONFIG_RETURN_WRITE_ERROR
} XConfigError;


/*
 * Message types
 */

typedef enum {
    ParseErrorMsg,
    ParseWarningMsg,
    ValidationErrorMsg,
    InternalErrorMsg,
    WriteErrorMsg,
    WarnMsg,
    ErrorMsg,
    DebugMsg,
    UnknownMsg
} MsgType;


/*
 * The user of libXF86Config-parser should provide an implementation
 * of xconfigPrint()
 */

void xconfigPrint(MsgType t, const char *msg);


/* 
 * all records that need to be linked lists should contain a next
 * pointer as their first field, so that they can be cast as a
 * GenericListRec
 */

typedef struct { void *next; } GenericListRec, *GenericListPtr;



/*
 * Options are stored in the XConfigOptionRec structure
 */

typedef struct __xconfigoptionrec {
    struct __xconfigoptionrec *next;
    char *name;
    char *val;
    int   used;
    char *comment;
} XConfigOptionRec, *XConfigOptionPtr;



/*
 * Files Section
 */

typedef struct {
    char *logfile;
    char *rgbpath;
    char *modulepath;
    char *inputdevs;
    char *fontpath;
    char *comment;
} XConfigFilesRec, *XConfigFilesPtr;

/* Values for load_type */
#define XCONFIG_LOAD_MODULE    0
#define XCONFIG_LOAD_DRIVER    1



/*
 * Modules Section
 */

typedef struct __xconfigloadrec {
    struct __xconfigloadrec *next;
    int                      type;
    char                    *name;
    XConfigOptionPtr         opt;
    char                    *comment;
} XConfigLoadRec, *XConfigLoadPtr;

typedef struct {
    XConfigLoadPtr  loads;
    char           *comment;
} XConfigModuleRec, *XConfigModulePtr;

#define CONF_IMPLICIT_KEYBOARD    "Implicit Core Keyboard"

#define CONF_IMPLICIT_POINTER    "Implicit Core Pointer"



/*
 * Modeline structure
 */

#define XCONFIG_MODE_PHSYNC    0x0001
#define XCONFIG_MODE_NHSYNC    0x0002
#define XCONFIG_MODE_PVSYNC    0x0004
#define XCONFIG_MODE_NVSYNC    0x0008
#define XCONFIG_MODE_INTERLACE 0x0010
#define XCONFIG_MODE_DBLSCAN   0x0020
#define XCONFIG_MODE_CSYNC     0x0040
#define XCONFIG_MODE_PCSYNC    0x0080
#define XCONFIG_MODE_NCSYNC    0x0100
#define XCONFIG_MODE_HSKEW     0x0200 /* hskew provided */
#define XCONFIG_MODE_BCAST     0x0400
#define XCONFIG_MODE_CUSTOM    0x0800 /* timing numbers customized by editor */
#define XCONFIG_MODE_VSCAN     0x1000

typedef struct __xconfigconfmodelinerec {
    struct __xconfigconfmodelinerec *next;
    char *identifier;
    int clock;
    int hdisplay;
    int hsyncstart;
    int hsyncend;
    int htotal;
    int vdisplay;
    int vsyncstart;
    int vsyncend;
    int vtotal;
    int vscan;
    int flags;
    int hskew;
    char *comment;
} XConfigModeLineRec, *XConfigModeLinePtr;



/*
 * VideoPort and VideoAdapter XXX what are these?
 */

typedef struct __xconfigconfvideoportrec {
    struct __xconfigconfvideoportrec *next;
    char             *identifier;
    XConfigOptionPtr  options;
    char             *comment;
} XConfigVideoPortRec, *XConfigVideoPortPtr;

typedef struct __xconfigconfvideoadaptorrec {
    struct __xconfigconfvideoadaptorrec *next;
    char                *identifier;
    char                *vendor;
    char                *board;
    char                *busid;
    char                *driver;
    XConfigOptionPtr     options;
    XConfigVideoPortPtr  ports;
    char                *fwdref;
    char                *comment;
} XConfigVideoAdaptorRec, *XConfigVideoAdaptorPtr;



/*
 * Monitor Section
 */

#define CONF_MAX_HSYNC 8
#define CONF_MAX_VREFRESH 8

typedef struct { float hi, lo; } parser_range;

typedef struct { int red, green, blue; } parser_rgb;

typedef struct __xconfigconfmodesrec {
    struct __xconfigconfmodesrec *next;
    char                     *identifier;
    XConfigModeLinePtr        modelines;
    char                     *comment;
} XConfigModesRec, *XConfigModesPtr;

typedef struct __xconfigconfmodeslinkrec {
    struct __xconfigconfmodeslinkrec *next;
    char                             *modes_name;
    XConfigModesPtr               modes;
} XConfigModesLinkRec, *XConfigModesLinkPtr;

typedef struct __xconfigconfmonitorrec {
    struct __xconfigconfmonitorrec *next;
    char                *identifier;
    char                *vendor;
    char                *modelname;
    int                  width;                /* in mm */
    int                  height;               /* in mm */
    XConfigModeLinePtr   modelines;
    int                  n_hsync;
    parser_range         hsync[CONF_MAX_HSYNC];
    int                  n_vrefresh;
    parser_range         vrefresh[CONF_MAX_VREFRESH];
    float                gamma_red;
    float                gamma_green;
    float                gamma_blue;
    XConfigOptionPtr     options;
    XConfigModesLinkPtr  modes_sections;
    char                *comment;
} XConfigMonitorRec, *XConfigMonitorPtr;



/*
 * Device Section
 */

#define CONF_MAXDACSPEEDS 4
#define CONF_MAXCLOCKS    128

typedef struct __xconfigconfdevicerec {
    struct __xconfigconfdevicerec *next;
    char             *identifier;
    char             *vendor;
    char             *board;
    char             *chipset;
    char             *busid;
    char             *card;
    char             *driver;
    char             *ramdac;
    int               dacSpeeds[CONF_MAXDACSPEEDS];
    int               videoram;
    int               textclockfreq;
    unsigned long     bios_base;
    unsigned long     mem_base;
    unsigned long     io_base;
    char             *clockchip;
    int               clocks;
    int               clock[CONF_MAXCLOCKS];
    int               chipid;
    int               chiprev;
    int               irq;
    int               screen;
    XConfigOptionPtr  options;
    char             *comment;
} XConfigDeviceRec, *XConfigDevicePtr;



/*
 * Screen Section
 */

typedef struct __xconfigmoderec {
    struct __xconfigmoderec *next;
    char                    *mode_name;
} XConfigModeRec, *XConfigModePtr;

typedef struct __xconfigconfdisplayrec {
    struct __xconfigconfdisplayrec *next;
    int               frameX0;
    int               frameY0;
    int               virtualX;
    int               virtualY;
    int               depth;
    int               bpp;
    char             *visual;
    parser_rgb        weight;
    parser_rgb        black;
    parser_rgb        white;
    XConfigModePtr    modes;
    XConfigOptionPtr  options;
    char             *comment;
} XConfigDisplayRec, *XConfigDisplayPtr;

typedef struct __xconfigconfadaptorlinkrec {
    struct __xconfigconfadaptorlinkrec *next;
    char                       *adaptor_name;
    XConfigVideoAdaptorPtr  adaptor;
} XConfigAdaptorLinkRec, *XConfigAdaptorLinkPtr;

typedef struct __xconfigconfscreenrec {
    struct __xconfigconfscreenrec *next;
    char                  *identifier;
    char                  *obsolete_driver;
    int                    defaultdepth;
    int                    defaultbpp;
    int                    defaultfbbpp;
    char                  *monitor_name;
    XConfigMonitorPtr      monitor;
    char                  *device_name;
    XConfigDevicePtr       device;
    XConfigAdaptorLinkPtr  adaptors;
    XConfigDisplayPtr      displays;
    XConfigOptionPtr       options;
    char                  *comment;
} XConfigScreenRec, *XConfigScreenPtr;



/*
 * Input Section
 */

typedef struct __xconfigconfinputrec {
    struct __xconfigconfinputrec *next;
    char              *identifier;
    char              *driver;
    XConfigOptionPtr   options;
    char              *comment;
} XConfigInputRec, *XConfigInputPtr;



/*
 * Input Reference; used by layout to store list of XConfigInputPtrs
 */

typedef struct __xconfigconfinputrefrec {
    struct __xconfigconfinputrefrec *next;
    XConfigInputPtr  input;
    char            *input_name;
    XConfigOptionPtr options;
} XConfigInputrefRec, *XConfigInputrefPtr;



/*
 * Adjacency structure; used by layout to store list of
 * XConfigScreenPtrs
 */

/* Values for adj_where */
#define CONF_ADJ_OBSOLETE -1
#define CONF_ADJ_ABSOLUTE  0
#define CONF_ADJ_RIGHTOF   1
#define CONF_ADJ_LEFTOF    2
#define CONF_ADJ_ABOVE     3
#define CONF_ADJ_BELOW     4
#define CONF_ADJ_RELATIVE  5

typedef struct __xconfigconfadjacencyrec {
    struct __xconfigconfadjacencyrec *next;
    int               scrnum;
    XConfigScreenPtr  screen;
    char             *screen_name;
    XConfigScreenPtr  top;
    char             *top_name;
    XConfigScreenPtr  bottom;
    char             *bottom_name;
    XConfigScreenPtr  left;
    char             *left_name;
    XConfigScreenPtr  right;
    char             *right_name;
    int               where;
    int               x;
    int               y;
    char             *refscreen;
} XConfigAdjacencyRec, *XConfigAdjacencyPtr;



/*
 * XConfigInactiveRec XXX what is this?
 */

typedef struct __xconfigconfinactiverec {
    struct __xconfigconfinactiverec *next;
    char            *device_name;
    XConfigDevicePtr device;
} XConfigInactiveRec, *XConfigInactivePtr;



/*
 * Layout Section
 */

typedef struct __xconfigconflayoutrec {
    struct __xconfigconflayoutrec *next;
    char                *identifier;
    XConfigAdjacencyPtr  adjacencies;
    XConfigInactivePtr   inactives;
    XConfigInputrefPtr   inputs;
    XConfigOptionPtr     options;
    char                *comment;
} XConfigLayoutRec, *XConfigLayoutPtr;



/*
 * Vendor Section XXX what is this?
 */

typedef struct __xconfigconfvendsubrec { 
    struct __xconfigconfvendsubrec *next;
    char             *name;
    char             *identifier;
    XConfigOptionPtr  options;
    char             *comment;
} XConfigVendSubRec, *XConfigVendSubPtr;

typedef struct __xconfigconfvendorrec {
    struct __xconfigconfvendorrec *next;
    char              *identifier;
    XConfigOptionPtr   options;
    XConfigVendSubPtr  subs;
    char              *comment;
} XConfigVendorRec, *XConfigVendorPtr;



/*
 * DRI section
 */

typedef struct __xconfigconfbuffersrec {
    struct __xconfigconfbuffersrec *next;
    int            count;
    int            size;
    char          *flags;
    char          *comment;
} XConfigBuffersRec, *XConfigBuffersPtr;

typedef struct {
    char             *group_name;
    int               group;
    int               mode;
    XConfigBuffersPtr buffers;
    char *            comment;
} XConfigDRIRec, *XConfigDRIPtr;



/*
 * ServerFlags Section
 */

typedef struct {
    XConfigOptionPtr  options;
    char             *comment;
} XConfigFlagsRec, *XConfigFlagsPtr;



/*
 * Extensions Section
 */

typedef struct
{
    XConfigOptionPtr  options;
    char             *comment;
}
XConfigExtensionsRec, *XConfigExtensionsPtr;


/*
 * Configuration file structure
 */

typedef struct {
    XConfigFilesPtr        files;
    XConfigModulePtr       modules;
    XConfigFlagsPtr        flags;
    XConfigVideoAdaptorPtr videoadaptors;
    XConfigModesPtr        modes;
    XConfigMonitorPtr      monitors;
    XConfigDevicePtr       devices;
    XConfigScreenPtr       screens;
    XConfigInputPtr        inputs;
    XConfigLayoutPtr       layouts;
    XConfigVendorPtr       vendors;
    XConfigDRIPtr          dri;
    XConfigExtensionsPtr   extensions;
    char                  *comment;
    char                  *filename;
} XConfigRec, *XConfigPtr;

typedef struct {
    int token;            /* id of the token */
    char *name;           /* pointer to the LOWERCASED name */
} XConfigSymTabRec, *XConfigSymTabPtr;


/*
 * data structure containing options; used during generation of X
 * config, and when sanitizing an existing config
 */

#define X_IS_XF86 0
#define X_IS_XORG 1

typedef struct {
    int   xserver;
    char *x_project_root;
    char *keyboard;
    char *mouse;
    char *keyboard_driver;
} GenerateOptions;


/*
 * Functions for open, reading, and writing XConfig files.
 */
const char *xconfigOpenConfigFile(const char *, const char *);
XConfigError xconfigReadConfigFile(XConfigPtr *);
int xconfigSanitizeConfig(XConfigPtr p, const char *screenName,
                          GenerateOptions *gop);
void xconfigCloseConfigFile(void);
int xconfigWriteConfigFile(const char *, XConfigPtr);

void xconfigFreeConfig(XConfigPtr p);

/*
 * Functions for searching for entries in lists
 */

XConfigDevicePtr   xconfigFindDevice(const char *ident, XConfigDevicePtr p);
XConfigLayoutPtr   xconfigFindLayout(const char *name, XConfigLayoutPtr list);
XConfigMonitorPtr  xconfigFindMonitor(const char *ident, XConfigMonitorPtr p);
XConfigModesPtr    xconfigFindModes(const char *ident, XConfigModesPtr p);
XConfigModeLinePtr xconfigFindModeLine(const char *ident,
                                       XConfigModeLinePtr p);
XConfigScreenPtr   xconfigFindScreen(const char *ident, XConfigScreenPtr p);
XConfigModePtr     xconfigFindMode(const char *name, XConfigModePtr p);
XConfigInputPtr    xconfigFindInput(const char *ident, XConfigInputPtr p);
XConfigInputPtr    xconfigFindInputByDriver(const char *driver,
                                            XConfigInputPtr p);
XConfigVendorPtr   xconfigFindVendor(const char *name, XConfigVendorPtr list);
XConfigVideoAdaptorPtr xconfigFindVideoAdaptor(const char *ident,
                                               XConfigVideoAdaptorPtr p);

/*
 * Functions for freeing lists
 */

void xconfigFreeDeviceList(XConfigDevicePtr ptr);
void xconfigFreeFiles(XConfigFilesPtr p);
void xconfigFreeFlags(XConfigFlagsPtr flags);
void xconfigFreeInputList(XConfigInputPtr ptr);
void xconfigFreeLayoutList(XConfigLayoutPtr ptr);
void xconfigFreeAdjacencyList(XConfigAdjacencyPtr ptr);
void xconfigFreeInputrefList(XConfigInputrefPtr ptr);
void xconfigFreeModules(XConfigModulePtr ptr);
void xconfigFreeMonitorList(XConfigMonitorPtr ptr);
void xconfigFreeModesList(XConfigModesPtr ptr);
void xconfigFreeModeLineList(XConfigModeLinePtr ptr);
void xconfigFreeScreenList(XConfigScreenPtr ptr);
void xconfigFreeAdaptorLinkList(XConfigAdaptorLinkPtr ptr);
void xconfigFreeDisplayList(XConfigDisplayPtr ptr);
void xconfigFreeModeList(XConfigModePtr ptr);
void xconfigFreeVendorList(XConfigVendorPtr p);
void xconfigFreeVendorSubList(XConfigVendSubPtr ptr);
void xconfigFreeVideoAdaptorList(XConfigVideoAdaptorPtr ptr);
void xconfigFreeVideoPortList(XConfigVideoPortPtr ptr);
void xconfigFreeBuffersList (XConfigBuffersPtr ptr);
void xconfigFreeDRI(XConfigDRIPtr ptr);
void xconfigFreeExtensions(XConfigExtensionsPtr ptr);


/*
 * item/list manipulation
 */

GenericListPtr xconfigAddListItem(GenericListPtr head, GenericListPtr c_new);
GenericListPtr xconfigRemoveListItem(GenericListPtr list, GenericListPtr item);
int xconfigItemNotSublist(GenericListPtr list_1, GenericListPtr list_2);
char *xconfigAddComment(char *cur, char *add);
XConfigLoadPtr xconfigAddNewLoadDirective(XConfigLoadPtr head,
                                          char *name, int type,
                                          XConfigOptionPtr opts, int do_token);
XConfigLoadPtr xconfigRemoveLoadDirective(XConfigLoadPtr head,
                                          XConfigLoadPtr load);

/*
 * Functions for manipulating Options
 */

XConfigOptionPtr xconfigAddNewOption(XConfigOptionPtr head,
                                     char *name, char *val);
XConfigOptionPtr xconfigRemoveOption(XConfigOptionPtr list,
                                     XConfigOptionPtr opt);
XConfigOptionPtr xconfigOptionListDup(XConfigOptionPtr opt);
void             xconfigOptionListFree(XConfigOptionPtr opt);
char            *xconfigOptionName(XConfigOptionPtr opt);
char            *xconfigOptionValue(XConfigOptionPtr opt);
XConfigOptionPtr xconfigNewOption(char *name, char *value);
XConfigOptionPtr xconfigNextOption(XConfigOptionPtr list);
XConfigOptionPtr xconfigFindOption(XConfigOptionPtr list, const char *name);
char            *xconfigFindOptionValue(XConfigOptionPtr list,
                                        const char *name);
int              xconfigFindOptionBoolean (XConfigOptionPtr,
                                           const char *name);
XConfigOptionPtr xconfigOptionListCreate(const char **options,
                                         int count, int used);
XConfigOptionPtr xconfigOptionListMerge(XConfigOptionPtr head,
                                        XConfigOptionPtr tail);

/*
 * Miscellaneous utility routines
 */

char *xconfigStrdup(const char *s);
char *xconfigStrcat(const char *str, ...);
int xconfigNameCompare(const char *s1, const char *s2);
int xconfigModelineCompare(XConfigModeLinePtr m1, XConfigModeLinePtr m2);
char *xconfigULongToString(unsigned long i);
XConfigOptionPtr xconfigParseOption(XConfigOptionPtr head);
void xconfigPrintOptionList(FILE *fp, XConfigOptionPtr list, int tabs);
int xconfigParsePciBusString(const char *busID,
                             int *bus, int *device, int *func);

XConfigDisplayPtr
xconfigAddDisplay(XConfigDisplayPtr head, const int depth);

XConfigModePtr
xconfigAddMode(XConfigModePtr head, const char *name);

XConfigModePtr
xconfigRemoveMode(XConfigModePtr head, const char *name);


XConfigPtr xconfigGenerate(GenerateOptions *gop);

XConfigScreenPtr xconfigGenerateAddScreen(XConfigPtr config, int bus, int slot,
                                          char *boardname, int count);

void xconfigGenerateAssignScreenAdjacencies(XConfigLayoutPtr layout);

void xconfigGeneratePrintPossibleMice(void);
void xconfigGeneratePrintPossibleKeyboards(void);

/*
 * check (and update, if necessary) the inputs in the specified layout
 * section
 */

int xconfigCheckCoreInputDevices(GenerateOptions *gop,
                                 XConfigPtr config, XConfigLayoutPtr layout);

#endif /* _xf86Parser_h_ */
