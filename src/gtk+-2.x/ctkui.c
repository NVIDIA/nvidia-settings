/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include "ctkui.h"
#include "ctkwindow.h"
#include "ctkutils.h"
#include "nvidia_icon.png.h"
/*
 * This source file provides thin wrappers over the gtk routines, so
 * that nvidia-settings.c doesn't need to include gtk+
 */

#if !GTK_CHECK_VERSION(3, 0, 0)
/*
 * GTK2's default X error handler terminates the process on any X error, on
 * any Display connection.  nvidia-settings talks NV-CONTROL over its own
 * Display connections and treats failed queries as "attribute not
 * available" (e.g. X screen attributes queried on a GPU target when there is
 * no NVIDIA X screen, as with PRIME render offload / reverse PRIME).  GTK3
 * only handles errors on GDK's own connection; do the same for GTK2 so such
 * errors are non-fatal.
 */
static XErrorHandler gdk_x_error_handler = NULL;

static int ctk_x_error_handler(Display *dpy, XErrorEvent *error)
{
    GdkDisplay *gdpy = gdk_display_get_default();

    if (gdpy && dpy != GDK_DISPLAY_XDISPLAY(gdpy)) {
        return 0;
    }
    return gdk_x_error_handler ? gdk_x_error_handler(dpy, error) : 0;
}
#endif

int ctk_init_check(int *argc, char **argv[])
{
    int ret = gtk_init_check(argc, argv);

#if !GTK_CHECK_VERSION(3, 0, 0)
    if (ret) {
        gdk_x_error_handler = XSetErrorHandler(ctk_x_error_handler);
    }
#endif
    return ret;
}

char *ctk_get_display(void)
{
    return gdk_get_display();
}

void ctk_main(ParsedAttribute *p,
              ConfigProperties *conf,
              CtrlSystem *system,
              const char *page)
{
    GList *list = NULL;
    GtkWidget *window;

    list = g_list_append (list, CTK_LOAD_PIXBUF(nvidia_icon));
    gtk_window_set_default_icon_list(list);
    window = ctk_window_new(p, conf, system);

    ctk_window_set_active_page(CTK_WINDOW(window), page);

    gtk_main();
}
