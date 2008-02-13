/*
 * nvidia-xconfig: A tool for manipulating X config files,
 * specifically for use by the NVIDIA Linux graphics driver.
 *
 * Copyright (C) 2005 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *      Free Software Foundation, Inc.
 *      59 Temple Place - Suite 330
 *      Boston, MA 02111-1307, USA
 *
 *
 * Generate.c
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>


#include "xf86Parser.h"
#include "Configint.h"

#define MOUSE_IDENTIFER "Mouse0"
#define KEYBOARD_IDENTIFER "Keyboard0"

#define SCREEN_IDENTIFIER "Screen%d"
#define DEVICE_IDENTIFIER "Device%d"
#define MONITOR_IDENTIFIER "Monitor%d"


static int is_file(const char *filename);

static void add_files(GenerateOptions *gop, XConfigPtr config);
static void add_font_path(GenerateOptions *gop, XConfigPtr config);
static void add_modules(GenerateOptions *gop, XConfigPtr config);

static XConfigDevicePtr
add_device(XConfigPtr config, int bus, int slot, char *boardname, int count);

static void add_layout(GenerateOptions *gop, XConfigPtr config);

static void add_inputref(XConfigPtr config, XConfigLayoutPtr layout,
                         char *name, char *coreKeyword);

/*
 * xconfigGenerate() - generate a new XConfig from scratch
 */

XConfigPtr xconfigGenerate(GenerateOptions *gop)
{
    XConfigPtr config;
    
    config = xconfigAlloc(sizeof(XConfigRec));

    /* add files, fonts, and modules */

    add_files(gop, config);
    add_font_path(gop, config);
    add_modules(gop, config);

    /* add the keyboard and mouse */

    xconfigAddKeyboard(gop, config);
    xconfigAddMouse(gop, config);

    /* add the layout */

    add_layout(gop, config);

    return config;

} /* xconfigGenerate() */



/*
 * xconfigGenerateAddScreen() - add a new screen to the config; bus
 * and slot can be -1 to be ignored; boardname can be NULL to be
 * ignored; count is used when building the identifier name, eg
 * '"Screen%d", count'.  Note that this does not append the screen to
 * any layout's adjacency list.
 */

XConfigScreenPtr xconfigGenerateAddScreen(XConfigPtr config,
                                          int bus, int slot,
                                          char *boardname, int count)
{
    XConfigScreenPtr screen, s;
    XConfigDevicePtr device;
    XConfigMonitorPtr monitor;
    
    monitor = xconfigAddMonitor(config, count);
    device = add_device(config, bus, slot, boardname, count);
    
    screen = xconfigAlloc(sizeof(XConfigScreenRec));

    screen->identifier = xconfigAlloc(32);
    snprintf(screen->identifier, 32, SCREEN_IDENTIFIER, count);
    
    screen->device_name = xconfigStrdup(device->identifier);
    screen->device = device;

    screen->monitor_name = xconfigStrdup(monitor->identifier);
    screen->monitor = monitor;
        
    screen->defaultdepth = 24;
    
    screen->displays = xconfigAddDisplay(screen->displays,
                                         screen->defaultdepth);

    /* append to the end of the screen list */
    
    if (!config->screens) {
        config->screens = screen;
    } else {
        for (s = config->screens; s->next; s = s->next);
        s->next = screen;
    }
    
    return screen;

} /* xconfigGenerateAddScreen() */



/*
 * assign_screen_adjacencies() - setup all the adjacency information
 * for the X screens in the given layout.  Nothing fancy here: just
 * position all the screens horizontally, moving from left to right.
 */

void xconfigGenerateAssignScreenAdjacencies(XConfigLayoutPtr layout)
{
    XConfigAdjacencyPtr adj, prev = NULL;
    
    for (adj = layout->adjacencies; adj; adj = adj->next) {
        
        if (prev) {
            adj->where = CONF_ADJ_RIGHTOF;
            adj->refscreen = xconfigStrdup(prev->screen_name);
        } else {
            adj->x = adj->y = -1;
        }
        
        /* make sure all the obsolete positioning is empty */

        adj->top = NULL;
        adj->top_name = NULL;
        adj->bottom = NULL;
        adj->bottom_name = NULL;
        adj->left = NULL;
        adj->left_name = NULL;
        adj->right = NULL;
        adj->right_name = NULL;
        
        prev = adj;
    }
    
} /* xconfigGenerateAssignScreenAdjacencies() */



/*********************************************************************/



/*
 * is_file()
 */

static int is_file(const char *filename)
{
    return (access(filename, F_OK) == 0);
    
} /* is_file() */


/*
 * find_libdir() - attempt to find the X server library path; this is
 * either
 *
 *     `pkg-config --variable=libdir xorg-server`
 *
 * or
 *
 *     [X PROJECT ROOT]/lib
 */

static char *find_libdir(GenerateOptions *gop)
{
    struct stat stat_buf;
    FILE *stream = NULL;
    char *s, *libdir = NULL;
    
    /*
     * run the pkg-config command and read the output; if the output
     * is a directory, then return that as the libdir
     */
    
    stream = popen("pkg-config --variable=libdir xorg-server", "r");
    
    if (stream) {
        char buf[256];

        buf[0] = '\0';

        while (1) {
            if (fgets(buf, 255, stream) == NULL) break;
            
            if (buf[0] != '\0') {

                /* truncate any newline */
                
                s = strchr(buf, '\n');
                if (s) *s = '\0';

                if ((stat(buf, &stat_buf) == 0) &&
                    (S_ISDIR(stat_buf.st_mode))) {
                
                    libdir = xconfigStrdup(buf);
                    break;
                }
            }
        }
        
        pclose(stream);

        if (libdir) return libdir;
    }

    /* otherwise, just fallback to [X PROJECT ROOT]/lib */

    return xconfigStrcat(gop->x_project_root, "/lib", NULL);
    
} /* find_libdir() */



/*
 * add_files() - 
 */

static void add_files(GenerateOptions *gop, XConfigPtr config)
{
    char *libdir = find_libdir(gop);

    config->files = xconfigAlloc(sizeof(XConfigFilesRec));
    config->files->rgbpath = xconfigStrcat(libdir, "/X11/rgb", NULL);
    
    free(libdir);

} /* add_files() */


/*
 * add_font_path() - scan through the __font_paths[] array,
 * temporarily chop off the ":unscaled" appendage, and check for the
 * file "fonts.dir" in the directory.  If fonts.dir exists, append the
 * path to config->files->fontpath.
 */

static void add_font_path(GenerateOptions *gop, XConfigPtr config)
{
    int i, ret;
    char *path, *p, *orig, *fonts_dir, *libdir;
    
    /*
     * The below font path has been constructed from various examples
     * and uses some suggests from the Font De-uglification HOWTO
     */
    
    static const char *__font_paths[] = {
        "LIBDIR/X11/fonts/local/",
        "LIBDIR/X11/fonts/misc/:unscaled",
        "LIBDIR/X11/fonts/100dpi/:unscaled",
        "LIBDIR/X11/fonts/75dpi/:unscaled",
        "LIBDIR/X11/fonts/misc/",
        "LIBDIR/X11/fonts/Type1/",
        "LIBDIR/X11/fonts/CID/",
        "LIBDIR/X11/fonts/Speedo/",
        "LIBDIR/X11/fonts/100dpi/",
        "LIBDIR/X11/fonts/75dpi/",
        "LIBDIR/X11/fonts/cyrillic/",
        "LIBDIR/X11/fonts/TTF/",
        "LIBDIR/X11/fonts/truetype/",
        "LIBDIR/X11/fonts/TrueType/",
        "LIBDIR/X11/fonts/Type1/sun/",
        "LIBDIR/X11/fonts/F3bitmaps/",
        "/usr/local/share/fonts/ttfonts",
        "/usr/share/fonts/default/Type1",
        "/usr/lib/openoffice/share/fonts/truetype",
        NULL
    };
    
    /*
     * if a font server is running, set the font path to that
     *
     * XXX should we check the port the font server is using?
     */
#if defined(NV_SUNOS)
    ret = system("ps -e -o fname | grep -v grep | egrep \"^xfs$\" > /dev/null");
#elif defined(NV_BSD)
    ret = system("ps -e -o comm | grep -v grep | egrep \"^xfs$\" > /dev/null");
#else
    ret = system("ps -C xfs 2>&1 > /dev/null");
#endif
    if (WEXITSTATUS(ret) == 0) {
        config->files->fontpath = xconfigStrdup("unix/:7100");
    } else {

        /* get the X server libdir */

        libdir = find_libdir(gop);
        
        for (i = 0; __font_paths[i]; i++) {
            path = xconfigStrdup(__font_paths[i]);

            /* replace LIBDIR with libdir */

            if (strncmp(path, "LIBDIR", 6) == 0) {
                p = xconfigStrcat(libdir, path + 6, NULL);
                free(path);
                path = p;
            }
        
            /* temporarily chop off any ":unscaled" appendage */

            p = strchr(path, ':');
            if (p) *p = '\0';

            /* skip this entry if the fonts.dir does not exist */

            fonts_dir = xconfigStrcat(path, "/fonts.dir", NULL);
            if (!is_file(fonts_dir)) {
                /* does not exist */
                free(path);
                free(fonts_dir);
                continue;
            }
            free(fonts_dir);
        
            /* add the ":unscaled" back */

            if (p) *p = ':';

            /*
             * either use this path as the fontpath, or append to the
             * existing fontpath
             */

            if (config->files->fontpath) {
                orig = config->files->fontpath;
                config->files->fontpath = xconfigStrcat(orig, ",", path, NULL);
                free(orig);
                free(path);
            } else {
                config->files->fontpath = path;
            }
        }

        /* free the libdir string */

        free(libdir);
    }
} /* add_font_path() */



/*
 * add_modules()
 */

static void add_modules(GenerateOptions *gop, XConfigPtr config)
{
    XConfigLoadPtr l = NULL;

    config->modules = xconfigAlloc(sizeof(XConfigModuleRec));
    
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("dbe"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("extmod"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("type1"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
#if defined(NV_SUNOS)
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("IA"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("bitstream"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("xtsol"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
#else
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("freetype"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
#endif
    l = xconfigAddNewLoadDirective(l, xconfigStrdup("glx"),
                                   XCONFIG_LOAD_MODULE, NULL, FALSE);
    
    config->modules->loads = l;
    
} /* add_modules() */



/*
 * xconfigAddMonitor() -
 *
 * XXX pass EDID values into this...
 */

XConfigMonitorPtr xconfigAddMonitor(XConfigPtr config, int count)
{
    XConfigMonitorPtr monitor, m;
    XConfigOptionPtr opt = NULL;

    /* XXX need to query resman for the EDID */

    monitor = xconfigAlloc(sizeof(XConfigMonitorRec));
    
    monitor->identifier = xconfigAlloc(32);
    snprintf(monitor->identifier, 32, MONITOR_IDENTIFIER, count);
    monitor->vendor = xconfigStrdup("Unknown");  /* XXX */
    monitor->modelname = xconfigStrdup("Unknown"); /* XXX */
    
    /* XXX check EDID for freq ranges */

    monitor->n_hsync = 1;
    monitor->hsync[0].lo = 30.0;
    monitor->hsync[0].hi = 110.0;

    monitor->n_vrefresh = 1;
    monitor->vrefresh[0].lo = 50.0;
    monitor->vrefresh[0].hi = 150.0;

    opt = xconfigAddNewOption(opt, xconfigStrdup("DPMS"), NULL);

    monitor->options = opt;

    /* append to the end of the monitor list */
    
    if (!config->monitors) {
        config->monitors = monitor;
    } else {
        for (m = config->monitors; m->next; m = m->next);
        m->next = monitor;
    }
    
    return monitor;
   
} /* xconfigAddMonitor() */



/*
 * add_device()
 */

static XConfigDevicePtr
add_device(XConfigPtr config, int bus, int slot, char *boardname, int count)
{
    XConfigDevicePtr device, d;

    device = xconfigAlloc(sizeof(XConfigDeviceRec));

    device->identifier = xconfigAlloc(32);
    snprintf(device->identifier, 32, DEVICE_IDENTIFIER, count);
    device->driver = xconfigStrdup("nvidia");
    device->vendor = xconfigStrdup("NVIDIA Corporation");

    if (bus != -1 && slot != -1) {
        device->busid = xconfigAlloc(32);
        snprintf(device->busid, 32, "PCI:%d:%d:0", bus, slot);
    }

    if (boardname) device->board = xconfigStrdup(boardname);
    
    device->chipid = -1;
    device->chiprev = -1;
    device->irq = -1;
    device->screen = -1;
    
    /* append to the end of the device list */
    
    if (!config->devices) {
        config->devices = device;
    } else {
        for (d = config->devices; d->next; d = d->next);
        d->next = device;
    }
    
    return device;
    
} /* add_device() */



XConfigDisplayPtr xconfigAddDisplay(XConfigDisplayPtr head, const int depth)
{
    XConfigDisplayPtr display;
    XConfigModePtr mode = NULL;
    
    mode = xconfigAddMode(mode, "640x480");
    mode = xconfigAddMode(mode, "800x600");
    mode = xconfigAddMode(mode, "1024x768");
    mode = xconfigAddMode(mode, "1280x1024");
    mode = xconfigAddMode(mode, "1600x1200");
    
    display = xconfigAlloc(sizeof(XConfigDisplayRec));
    display->depth = depth;
    display->modes = mode;
    display->frameX0 = -1;
    display->frameY0 = -1;
    display->black.red = -1;
    display->white.red = -1;

    display->next = head;
    
    return display;
}



/*
 * add_layout() - add a layout section to the XConfigPtr
 */

static void add_layout(GenerateOptions *gop, XConfigPtr config)
{
    XConfigLayoutPtr layout;
    XConfigAdjacencyPtr adj;
    XConfigScreenPtr screen;
    
    /* assume 1 X screen */

    screen = xconfigGenerateAddScreen(config, -1, -1, NULL, 0);
    
    /* create layout */

    layout = xconfigAlloc(sizeof(XConfigLayoutRec));
    
    layout->identifier = xconfigStrdup("Layout0");
    
    adj = xconfigAlloc(sizeof(XConfigAdjacencyRec));

    adj->scrnum = 0;
    adj->screen = screen;
    adj->screen_name = xconfigStrdup(screen->identifier);
    
    layout->adjacencies = adj;
    
    xconfigGenerateAssignScreenAdjacencies(layout);
    
    add_inputref(config, layout, MOUSE_IDENTIFER, "CorePointer");
    add_inputref(config, layout, KEYBOARD_IDENTIFER, "CoreKeyboard");
    
    layout->next = config->layouts;
    config->layouts = layout;
    
} /* add_layout() */



/*
 * add_inputref() - add a new XConfigInputrefPtr to the given layout
 */

static void add_inputref(XConfigPtr config, XConfigLayoutPtr layout,
                         char *name, char *coreKeyword)
{
    XConfigInputrefPtr inputRef;

    inputRef = xconfigAlloc(sizeof(XConfigInputrefRec));
    inputRef->input_name = xconfigStrdup(name);
    inputRef->input = xconfigFindInput(inputRef->input_name, config->inputs);
    inputRef->options =
        xconfigAddNewOption(NULL, xconfigStrdup(coreKeyword), NULL);
    inputRef->next = layout->inputs;
    layout->inputs = inputRef;

} /* add_inputref() */



/*********************************************************************/

/*
 * Mouse detection
 */


typedef struct {
    char *shortname; /* commandline name */
    char *name;      /* mouse name */
    char *gpmproto;  /* protocol used by gpm */
    char *Xproto;    /* XFree86 Protocol */
    char *device;    /* /dev/ file */
    int   emulate3;  /* Emulate3Buttons */
} MouseEntry;


/*
 * This table is based on data contained in
 * /usr/lib/python2.2/site-packages/rhpl/mouse.py on Red Hat Fedora
 * core 1.  That file contains the following copyright:
 *
 *
 *
 * mouse.py: mouse configuration data
 *
 * Copyright 1999-2002 Red Hat, Inc.
 *
 * This software may be freely redistributed under the terms of the GNU
 * library public license.
 *
 * You should have received a copy of the GNU Library Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

static const MouseEntry __mice[] = {
    /* shortname            name                                         gpm protocol    X protocol           device       emulate3 */
    { "alpsps/2",          "ALPS - GlidePoint (PS/2)",                  "ps/2",         "GlidePointPS/2",    "psaux",      TRUE },
    { "ascii",             "ASCII - MieMouse (serial)",                 "ms3",          "IntelliMouse",      "ttyS",       FALSE },
    { "asciips/2",         "ASCII - MieMouse (PS/2)",                   "ps/2",         "NetMousePS/2",      "psaux",      TRUE },
    { "atibm",             "ATI - Bus Mouse",                           "Busmouse",     "BusMouse",          "atibm",      TRUE },
    { "generic",           "Generic - 2 Button Mouse (serial)",         "Microsoft",    "Microsoft",         "ttyS",       TRUE },
    { "generic3",          "Generic - 3 Button Mouse (serial)",         "Microsoft",    "Microsoft",         "ttyS",       FALSE },
    { "genericps/2",       "Generic - 2 Button Mouse (PS/2)",           "ps/2",         "PS/2",              "psaux",      TRUE },
    { "generic3ps/2",      "Generic - 3 Button Mouse (PS/2)",           "ps/2",         "PS/2",              "psaux",      FALSE },
    { "genericwheelps/2",  "Generic - Wheel Mouse (PS/2)",              "imps2",        "IMPS/2",            "psaux",      FALSE },
    { "genericusb",        "Generic - 2 Button Mouse (USB)",            "imps2",        "IMPS/2",            "input/mice", TRUE },
    { "generic3usb",       "Generic - 3 Button Mouse (USB)",            "imps2",        "IMPS/2",            "input/mice", FALSE },
    { "genericwheelusb",   "Generic - Wheel Mouse (USB)",               "imps2",        "IMPS/2",            "input/mice", FALSE },
    { "geniusnm",          "Genius - NetMouse (serial)",                "ms3",          "IntelliMouse",      "ttyS",       TRUE },
    { "geniusnmps/2",      "Genius - NetMouse (PS/2)",                  "netmouse",     "NetMousePS/2",      "psaux",      TRUE },
    { "geniusprops/2",     "Genius - NetMouse Pro (PS/2)",              "netmouse",     "NetMousePS/2",      "psaux",      TRUE },
    { "geniusscrollps/2",  "Genius - NetScroll (PS/2)",                 "netmouse",     "NetScrollPS/2",     "psaux",      TRUE },
    { "geniusscrollps/2+", "Genius - NetScroll+ (PS/2)",                "netmouse",     "NetMousePS/2",      "psaux",      FALSE },
    { "thinking",          "Kensington - Thinking Mouse (serial)",      "Microsoft",    "ThinkingMouse",     "ttyS",       TRUE },
    { "thinkingps/2",      "Kensington - Thinking Mouse (PS/2)",        "ps/2",         "ThinkingMousePS/2", "psaux",      TRUE },
    { "logitech",          "Logitech - C7 Mouse (serial, old C7 type)", "Logitech",     "Logitech",          "ttyS",       FALSE },
    { "logitechcc",        "Logitech - CC Series (serial)",             "logim",        "MouseMan",          "ttyS",       FALSE },
    { "logibm",            "Logitech - Bus Mouse",                      "Busmouse",     "BusMouse",          "logibm",     FALSE },
    { "logimman",          "Logitech - MouseMan/FirstMouse (serial)",   "MouseMan",     "MouseMan",          "ttyS",       FALSE },
    { "logimmanps/2",      "Logitech - MouseMan/FirstMouse (PS/2)",     "ps/2",         "PS/2",              "psaux",      FALSE },
    { "logimman+",         "Logitech - MouseMan+/FirstMouse+ (serial)", "pnp",          "IntelliMouse",      "ttyS",       FALSE },
    { "logimman+ps/2",     "Logitech - MouseMan+/FirstMouse+ (PS/2)",   "ps/2",         "MouseManPlusPS/2",  "psaux",      FALSE },
    { "logimmusb",         "Logitech - MouseMan Wheel (USB)",           "ps/2",         "IMPS/2",            "input/mice", FALSE },
    { "logimmusboptical",  "Logitech - Cordless Optical Mouse (USB)",   "ps/2",         "IMPS/2",            "input/mice", FALSE },
    { "microsoft",         "Microsoft - Compatible Mouse (serial)",     "Microsoft",    "Microsoft",         "ttyS",       TRUE },
    { "msnew",             "Microsoft - Rev 2.1A or higher (serial)",   "pnp",          "Auto",              "ttyS",       TRUE },
    { "msintelli",         "Microsoft - IntelliMouse (serial)",         "ms3",          "IntelliMouse",      "ttyS",       FALSE },
    { "msintellips/2",     "Microsoft - IntelliMouse (PS/2)",           "imps2",        "IMPS/2",            "psaux",      FALSE },
    { "msintelliusb",      "Microsoft - IntelliMouse (USB)",            "ps/2",         "IMPS/2",            "input/mice", FALSE },
    { "msintelliusboptical","Microsoft - IntelliMouse Optical (USB)",   "ps/2",         "IMPS/2",            "input/mice", FALSE },
    { "msbm",              "Microsoft - Bus Mouse",                     "Busmouse",     "BusMouse",          "inportbm",   TRUE },
    { "mousesystems",      "Mouse Systems - Mouse (serial)",            "MouseSystems", "MouseSystems",      "ttyS",       TRUE },
    { "mmseries",          "MM - Series (serial)",                      "MMSeries",     "MMSeries",          "ttyS",       TRUE },
    { "mmhittab",          "MM - HitTablet (serial)",                   "MMHitTab",     "MMHittab",          "ttyS",       TRUE },
    { "sun",               "Sun - Mouse",                               "sun",          "sun",               "sunmouse",   FALSE },
    { NULL,                NULL,                                         NULL,          NULL,                NULL,         FALSE },
};



/*
 * This table maps between the mouse protocol name used for gpm and
 * for the X server "protocol" mouse option.
 */

typedef struct {
    char *gpmproto;
    char *Xproto;
} ProtocolEntry;

static const ProtocolEntry __protocols[] = {
    /* gpm protocol    X protocol  */
    { "ms3",          "IntelliMouse" },
    { "Busmouse",     "BusMouse"     },
    { "Microsoft",    "Microsoft"    },
    { "imps2",        "IMPS/2"       },
    { "netmouse",     "NetMousePS/2" },
    { "Logitech",     "Logitech"     },
    { "logim",        "MouseMan"     },
    { "MouseMan",     "MouseMan"     },
    { "ps/2",         "PS/2"         },
    { "pnp",          "Auto"         },
    { "MouseSystems", "MouseSystems" },
    { "MMSeries",     "MMSeries"     },
    { "MMHitTab",     "MMHittab"     },
    { "sun",          "sun"          },
    {  NULL,           NULL          },
};


/*
 * gpm_proto_to_X_proto() - map from gpm mouse protocol to X mouse
 * protocol
 */

static char* gpm_proto_to_X_proto(const char *gpm)
{
    int i;

    for (i = 0; __protocols[i].gpmproto; i++) {
        if (strcmp(gpm, __protocols[i].gpmproto) == 0) {
            return __protocols[i].Xproto;
        }
    }
    return NULL;

} /* gpm_proto_to_X_proto() */



/*
 * find_mouse_entry() - scan the __mice[] table for the entry that
 * corresponds to the specified value; return a pointer to the
 * matching entry in the table, if any.
 */

static const MouseEntry *find_mouse_entry(char *value)
{
    int i;

    if (!value) return NULL;

    for (i = 0; __mice[i].name; i++) {
        if (strcmp(value, __mice[i].shortname) == 0) {
            return &__mice[i];
        }
    }
    return NULL;

} /* find_mouse_entry() */



/*
 * find_closest_mouse_entry() - scan the __mice[] table for the entry that
 * matches all of the specified values; any of the values can be NULL,
 * in which case we do not use them as part of the comparison.  Note
 * that device is compared case sensitive, proto is compared case
 * insensitive, and emulate3 is just a boolean.
 */

static const MouseEntry *find_closest_mouse_entry(const char *device,
                                                  const char *proto,
                                                  const char *emulate3_str)
{
    int i;
    int emulate3 = FALSE;
    
    /*
     * translate the emulate3 string into a boolean we can use below
     * for comparison
     */

    if ((emulate3_str) &&
        ((strcasecmp(emulate3_str, "yes") == 0) ||
         (strcasecmp(emulate3_str, "true") == 0) ||
         (strcasecmp(emulate3_str, "1") == 0))) {
        emulate3 = TRUE;
    }

    /*
     * skip the "/dev/" part of the device filename
     */
    
    if (device && (strncmp(device, "/dev/", 5) == 0)) {
        device += 5; /* strlen("/dev/") */
    }
    
    for (i = 0; __mice[i].name; i++) {
        if ((device) && (strcmp(device, __mice[i].device) != 0)) continue;
        if ((proto) && (strcasecmp(proto, __mice[i].Xproto)) != 0) continue;
        if ((emulate3_str) && (emulate3 != __mice[i].emulate3)) continue;
        return &__mice[i];
    }
    
    return NULL;
    
} /* find_closest_mouse_entry() */



/*
 * find_config_entry() - scan the specified filename for the specified
 * keyword; return the value that the keyword is assigned to, or NULL
 * if any error occurs.
 */

static char *find_config_entry(const char *filename, const char *keyword)
{
    int fd = -1;
    char *data = NULL;
    char *value = NULL;
    char *buf = NULL;
    char *tmp, *start, *c, *end;
    struct stat stat_buf;
    size_t len;
    
    if ((fd = open(filename, O_RDONLY)) == -1) goto done;
    
    if (fstat(fd, &stat_buf) == -1) goto done;
    
    if ((data = mmap(0, stat_buf.st_size, PROT_READ, MAP_SHARED,
                     fd, 0)) == (void *) -1) goto done;
    
    /*
     * create a sysmem copy of the buffer, so that we can explicitly
     * NULL terminate it
     */

    buf = malloc(stat_buf.st_size + 1);

    if (!buf) goto done;

    memcpy(buf, data, stat_buf.st_size);
    buf[stat_buf.st_size] = '\0';
    
    /* search for the keyword */
    
    start = buf;

    while (TRUE) {
        tmp = strstr(start, keyword);
        if (!tmp) goto done;

        /*
         * make sure this line is not commented out: search back from
         * tmp: if we hit a "#" before a newline, then this line is
         * commented out and we should search again
         */

        c = tmp;
        while ((c >= start) && (*c != '\n') && (*c != '#')) c--;
        
        if (*c == '#') {
            /* keyword was commented out... search again */
            start = tmp+1;
        } else {
            /* keyword is not commented out */
            break;
        }
    }

    start = tmp + strlen(keyword);
    end = strchr(start, '\n');
    if (!end) goto done;

    /* there must be something between the start and the end */

    if (start == end) goto done;
    
    /* take what is between as the value */
    
    len = end - start;
    value = xconfigAlloc(len + 1);
    strncpy(value, start, len);
    value[len] = '\0';

    /* if the first and last characters are quotation marks, remove them */

    if ((value[0] == '\"') && (value[len-1] == '\"')) {
        tmp = xconfigAlloc(len - 1);
        strncpy(tmp, value + 1, len - 2);
        tmp[len-2] = '\0';
        free(value);
        value = tmp;
    }
    
 done:

    if (buf) free(buf);
    if (data) munmap(data, stat_buf.st_size);
    if (fd != -1) close(fd);

    return value;
    
} /* find_config_entry() */



/*
 * xconfigGeneratePrintPossibleMice() - print the mouse table to stdout
 */

void xconfigGeneratePrintPossibleMice(void)
{
    int i;
    
    printf("%-25s%-35s\n\n", "Short Name", "Name");
    
    for (i = 0; __mice[i].name; i++) {
        printf("%-25s%-35s\n", __mice[i].shortname, __mice[i].name);
    }
    
    printf("\n");

} /* xconfigGeneratePrintPossibleMice() */



/*
 * xconfigAddMouse() - determine the mouse type, and then add an
 * XConfigInputRec with the appropriate options.
 *
 * - if the user specified on the commandline, use that
 *
 * - if /etc/sysconfig/mouse exists and contains valid data, use
 *   that
 *
 * - if /etc/conf.d/gpm exists and contains valid data, use that
 *
 * - infer the settings from the commandline options gpm is using XXX?
 *
 * - default to "auto" on /dev/mouse
 */

int xconfigAddMouse(GenerateOptions *gop, XConfigPtr config)
{
    const MouseEntry *entry = NULL;
    XConfigInputPtr input;
    XConfigOptionPtr opt = NULL;
    char *device_path, *comment = "default";
    
    /* if the user specified on the commandline, use that */
    
    if (gop->mouse) {
        entry = find_mouse_entry(gop->mouse);
        if (entry) {
            comment = "commandline input";
        } else {
            xconfigErrorMsg(WarnMsg, "Unable to find mouse \"%s\".",
                            gop->mouse);
        }
    }
    
    /*
     * if /etc/sysconfig/mouse exists, and contains valid data, use
     * that
     */
    
    if (!entry) {
        char *protocol, *device, *emulate3;
        
        device = find_config_entry("/etc/sysconfig/mouse", "DEVICE=");
        protocol = find_config_entry("/etc/sysconfig/mouse", "XMOUSETYPE=");
        emulate3 = find_config_entry("/etc/sysconfig/mouse", "XEMU3=");
        
        if (device || protocol || emulate3) {
            entry = find_closest_mouse_entry(device, protocol, emulate3);
            if (entry) {
                comment = "data in \"/etc/sysconfig/mouse\"";
            }
        }
    }

    /* if /etc/conf.d/gpm exists and contains valid data, use that */

    if (!entry) {
        char *protocol, *device;
        
        protocol = find_config_entry("/etc/conf.d/gpm", "MOUSE=");
        device = find_config_entry("/etc/conf.d/gpm", "MOUSEDEV=");
        
        if (protocol && device) {
            MouseEntry *e = xconfigAlloc(sizeof(MouseEntry));
            e->shortname = "custom";
            e->name = "inferred from /etc/conf.d/gpm";
            e->gpmproto = protocol;
            e->Xproto = gpm_proto_to_X_proto(protocol);
            e->device = device + strlen("/dev/");
            e->emulate3 = FALSE; // XXX?
            entry = e;
            comment = "data in \"/etc/conf.d/gpm\"";
        }
    }
    
    /*
     * XXX we could try to infer the settings from the commandline
     * options gpm is using
     */

    if (!entry) {
        /* XXX implement me */
    }
    
    /* at this point, we must have a mouse entry */
    
    if (!entry) {
        MouseEntry *e = xconfigAlloc(sizeof(MouseEntry));
        e->Xproto = "auto";

#if defined(NV_BSD)
        e->device = "sysmouse";
#else
        if (access("/dev/psaux", F_OK) == 0) {
            e->device = "psaux";
        } else if (access("/dev/input/mice", F_OK) == 0) {
            e->device = "input/mice";
        } else {
            e->device = "mouse";
        }
#endif
        e->emulate3 = FALSE;
        entry = e;
    }

    /* add a new mouse input section */

    input = xconfigAlloc(sizeof(XConfigInputRec));
    
    input->comment = xconfigStrcat("    # generated from ",
                                   comment, "\n", NULL);
    input->identifier = xconfigStrdup("Mouse0");
    input->driver = xconfigStrdup("mouse");

    device_path = xconfigStrcat("/dev/", entry->device, NULL);

    opt = xconfigAddNewOption(opt, xconfigStrdup("Protocol"),
                              xconfigStrdup(entry->Xproto));
    opt = xconfigAddNewOption(opt, xconfigStrdup("Device"), device_path);
    opt = xconfigAddNewOption(opt, xconfigStrdup("Emulate3Buttons"),
                              entry->emulate3 ?
                              xconfigStrdup("yes") : xconfigStrdup("no"));
    
    /*
     * This will make wheel mice work, and non-wheel mice should
     * ignore ZAxisMapping
     */

    opt = xconfigAddNewOption(opt, xconfigStrdup("ZAxisMapping"),
                              xconfigStrdup("4 5"));
    
    input->options = opt;
    
    input->next = config->inputs;
    config->inputs = input;
    
    return TRUE;
    
} /* xconfigAddMouse() */





/*********************************************************************/

/*
 * keyboard detection
 */

typedef struct {
    char *keytable;
    char *name;
    char *layout;   /* XkbLayout */
    char *model;    /* XkbModel */
    char *variant;  /* XkbVariant */
    char *options;  /* XkbOptions */
} KeyboardEntry;


/*
 * This table is based on data contained in
 * /usr/lib/python2.2/site-packages/rhpl/keyboard_models.py on Red Hat
 * Fedora core 1.  That file contains the following copyright:
 *
 *
 * keyboard_models.py - keyboard model list
 * 
 * Brent Fox <bfox@redhat.com>
 * Mike Fulbright <msf@redhat.com>
 * Jeremy Katz <katzj@redhat.com>
 * 
 * Copyright 2002 Red Hat, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

static const KeyboardEntry __keyboards[] = {
    
    /* keytable               name                              layout         model  variant options */

    { "be-latin1",            "Belgian (be-latin1)",            "be",          "pc105", NULL, NULL },
    { "bg",                   "Bulgarian",                      "bg,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "br-abnt2",             "Brazilian (ABNT2)",              "br",          "abnt2", NULL, NULL },
    { "cf",                   "French Canadian",                "ca_enhanced", "pc105", NULL, NULL },
    { "croat",                "Croatian",                       "hr",          "pc105", NULL, NULL },
    { "cz-us-qwertz",         "Czechoslovakian (qwertz)",       "cz,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "cz-lat2",              "Czechoslovakian",                "cz_qwerty",   "pc105", NULL, NULL },
    { "de",                   "German",                         "de",          "pc105", NULL, NULL },
    { "de-latin1",            "German (latin1)",                "de",          "pc105", NULL, NULL },
    { "de-latin1-nodeadkeys", "German (latin1 w/ no deadkeys)", "de",          "pc105", "nodeadkeys", NULL },
    { "dvorak",               "Dvorak",                         "dvorak",      "pc105", NULL, NULL },
    { "dk",                   "Danish",                         "dk",          "pc105", NULL, NULL },
    { "dk-latin1",            "Danish (latin1)",                "dk",          "pc105", NULL, NULL },
    { "es",                   "Spanish",                        "es",          "pc105", NULL, NULL },
    { "et",                   "Estonian",                       "ee",          "pc105", NULL, NULL },
    { "fi",                   "Finnish",                        "fi",          "pc105", NULL, NULL },
    { "fi-latin1",            "Finnish (latin1)",               "fi",          "pc105", NULL, NULL },
    { "fr",                   "French",                         "fr",          "pc105", NULL, NULL },
    { "fr-latin0",            "French (latin0)",                "fr",          "pc105", NULL, NULL },
    { "fr-latin1",            "French (latin1)",                "fr",          "pc105", NULL, NULL },
    { "fr-pc",                "French (pc)",                    "fr",          "pc105", NULL, NULL },
    { "fr_CH",                "Swiss French",                   "fr_CH",       "pc105", NULL, NULL },
    { "fr_CH-latin1",         "Swiss French (latin1)",          "fr_CH",       "pc105", NULL, NULL },
    { "gr",                   "Greek",                          "us,el",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "hu",                   "Hungarian",                      "hu",          "pc105", NULL, NULL },
    { "hu101",                "Hungarian (101 key)",            "hu",          "pc105", NULL, NULL },
    { "is-latin1",            "Icelandic",                      "is",          "pc105", NULL, NULL },
    { "it",                   "Italian",                        "it",          "pc105", NULL, NULL },
    { "it-ibm",               "Italian (IBM)",                  "it",          "pc105", NULL, NULL },
    { "it2",                  "Italian (it2)",                  "it",          "pc105", NULL, NULL },
    { "jp106",                "Japanese",                       "jp",          "jp106", NULL, NULL },
    { "la-latin1",            "Latin American",                 "la",          "pc105", NULL, NULL },
    { "mk-utf",               "Macedonian",                     "mk,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "no",                   "Norwegian",                      "no",          "pc105", NULL, NULL },
    { "pl",                   "Polish",                         "pl",          "pc105", NULL, NULL },
    { "pt-latin1",            "Portuguese",                     "pt",          "pc105", NULL, NULL },
    { "ro_win",               "Romanian",                       "ro",          "pc105", NULL, NULL },
    { "ru",                   "Russian",                        "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "ru-cp1251",            "Russian (cp1251)",               "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "ru-ms",                "Russian (Microsoft)",            "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "ru1",                  "Russian (ru1)",                  "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "ru2",                  "Russian (ru2)",                  "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "ru_win",               "Russian (win)",                  "ru,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "speakup",              "Speakup",                        "us",          "pc105", NULL, NULL },
    { "speakup-lt",           "Speakup (laptop)",               "us",          "pc105", NULL, NULL },
    { "sv-latin1",            "Swedish",                        "se",          "pc105", NULL, NULL },
    { "sg",                   "Swiss German",                   "de_CH",       "pc105", NULL, NULL },
    { "sg-latin1",            "Swiss German (latin1)",          "de_CH",       "pc105", NULL, NULL },
    { "sk-qwerty",            "Slovakian",                      "sk_qwerty",   "pc105", NULL, NULL },
    { "slovene",              "Slovenian",                      "si",          "pc105", NULL, NULL },
    { "trq",                  "Turkish",                        "tr",          "pc105", NULL, NULL },
    { "uk",                   "United Kingdom",                 "gb",          "pc105", NULL, NULL },
    { "ua",                   "Ukrainian",                      "ua,us",       "pc105", NULL, "grp:shift_toggle,grp_led:scroll" },
    { "us-acentos",           "U.S. International",             "us_intl",     "pc105", NULL, NULL },
    { "us",                   "U.S. English",                   "us",          "pc105", NULL, NULL },
    { NULL,                   NULL,                             NULL,          NULL,    NULL, NULL },
};



/*
 * find_keyboard_entry() - scan the __keyboards[] table for the entry that
 * corresponds to the specified value; return a pointer to the
 * matching entry in the table, if any.
 */

static const KeyboardEntry *find_keyboard_entry(char *value)
{
    int i;

    if (!value) return NULL;

    for (i = 0; __keyboards[i].name; i++) {
        if (strcmp(value, __keyboards[i].keytable) == 0) {
            return &__keyboards[i];
        }
    }
    return NULL;

} /* find_keyboard_entry() */



/*
 * xconfigGeneratePrintPossibleKeyboards() - print the keyboard table
 */

void xconfigGeneratePrintPossibleKeyboards(void)
{
    int i;
    
    printf("%-25s%-35s\n\n", "Short Name", "Name");

    for (i = 0; __keyboards[i].name; i++) {
        printf("%-25s%-35s\n", __keyboards[i].keytable, __keyboards[i].name);
    }
    
    printf("\n");

} /* xconfigGeneratePrintPossibleKeyboards() */



/*
 * xconfigAddKeyboard() - determine the keyboard type, and then add an
 * XConfigInputRec with the appropriate options.
 *
 * How to detect the keyboard:
 *
 * - if the user specified on the command line, use that
 *
 * - if /etc/sysconfig/keyboard exists, and contains a valid
 *   KEYTABLE entry, use that
 */

int xconfigAddKeyboard(GenerateOptions *gop, XConfigPtr config)
{
    char *value, *comment = "default";
    const KeyboardEntry *entry = NULL;
    
    XConfigInputPtr input;
    XConfigOptionPtr opt = NULL;
    
    /*
     * if the user specified on the command line, use that
     */
    
    if (gop->keyboard) {
        entry = find_keyboard_entry(gop->keyboard);
        if (entry) {
            comment = "commandline input";
        } else {
            xconfigErrorMsg(WarnMsg, "Unable to find keyboard \"%s\".",
                            gop->keyboard);
        }
    }
    
    /*
     * if /etc/sysconfig/keyboard exists, and contains a valid
     * KEYTABLE entry, use that
     */

    if (!entry) {
        value = find_config_entry("/etc/sysconfig/keyboard", "KEYTABLE=");
        entry = find_keyboard_entry(value);
        if (value) {
            free(value);
        }
        if (entry) {
            comment = "data in \"/etc/sysconfig/keyboard\"";
        }
    }

    /* add a new keyboard input section */
    
    input = xconfigAlloc(sizeof(XConfigInputRec));
    
    input->comment = xconfigStrcat("    # generated from ",
                                   comment, "\n", NULL);
    input->identifier = xconfigStrdup("Keyboard0");

    /*
     * determine which keyboard driver should be used (either "kbd" or
     * "keyboard"); if the user specified a keyboard driver use that;
     * if 'ROOT/lib/modules/input/kbd_drv.(o|so)' exists, use "kbd";
     * otherwise, use "keyboard".
     * On Solaris, use the default "keyboard"
     */
    
    if (gop->keyboard_driver) {
        input->driver = gop->keyboard_driver;
    } else {
#if defined(NV_SUNOS) || defined(NV_BSD)
        input->driver = xconfigStrdup("keyboard");
#else
        if (gop->xserver == X_IS_XORG) {
            input->driver = xconfigStrdup("kbd");
        } else {
            input->driver = xconfigStrdup("keyboard");
        }
#endif
    }
    
    /*
     * set additional keyboard options, based on the Keyboard table
     * entry we found above
     */

    if (entry && entry->layout)
        opt = xconfigAddNewOption(opt,
                                  xconfigStrdup("XkbLayout"),
                                  xconfigStrdup(entry->layout));
    if (entry && entry->model)
        opt = xconfigAddNewOption(opt,
                                  xconfigStrdup("XkbModel"),
                                  xconfigStrdup(entry->model));
    if (entry && entry->variant)
        opt = xconfigAddNewOption(opt,
                                  xconfigStrdup("XkbVariant"),
                                  xconfigStrdup(entry->variant));
    if (entry && entry->options)
        opt = xconfigAddNewOption(opt,
                                  xconfigStrdup("XkbOptions"),
                                  xconfigStrdup(entry->options));

    input->options = opt;
    
    input->next = config->inputs;
    config->inputs = input;

    return TRUE;
    
} /* xconfigAddKeyboard() */
