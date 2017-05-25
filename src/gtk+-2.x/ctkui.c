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
#include "ctkui.h"
#include "ctkwindow.h"
#include "ctkutils.h"
#include "nvidia_icon.png.h"
/*
 * This source file provides thin wrappers over the gtk routines, so
 * that nvidia-settings.c doesn't need to include gtk+
 */

int ctk_init_check(int *argc, char **argv[])
{
    return gtk_init_check(argc, argv);
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
    CtrlTargetNode *node;
    int has_nv_control = FALSE;
    GList *list = NULL;
    GtkWidget *window;

    list = g_list_append (list, CTK_LOAD_PIXBUF(nvidia_icon));
    gtk_window_set_default_icon_list(list);
    window = ctk_window_new(p, conf, system);

    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {
        if (node->t->h) {
            has_nv_control = TRUE;
            break;
        }
    }

    if (!has_nv_control) {
        GtkWidget *dlg;
        dlg = gtk_message_dialog_new (NULL,
                                      GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_OK,
                                      "You do not appear to be using the NVIDIA "
                                      "X driver.  Please edit your X configuration "
                                      "file (just run `nvidia-xconfig` "
                                      "as root), and restart the X server.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy (dlg);
    }

    ctk_window_set_active_page(CTK_WINDOW(window), page);

    gtk_main();
}
