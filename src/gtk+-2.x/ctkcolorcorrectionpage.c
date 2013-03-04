/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2012 NVIDIA Corporation.
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

#include "NvCtrlAttributes.h"

#include "ctkbanner.h"

#include "ctkcolorcorrectionpage.h"
#include "ctkcolorcorrection.h"

#include "ctkconfig.h"
#include "ctkhelp.h"

#include <string.h>
#include <stdlib.h>


GType ctk_color_correction_page_get_type(
    void
)
{
    static GType ctk_color_correction_page_type = 0;

    if (!ctk_color_correction_page_type) {
        static const GTypeInfo ctk_color_correction_page_info = {
            sizeof (CtkColorCorrectionPageClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkColorCorrectionPage),
            0,    /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_color_correction_page_type =
            g_type_register_static(GTK_TYPE_VBOX, "CtkColorCorrectionPage",
                                   &ctk_color_correction_page_info, 0);
    }

    return ctk_color_correction_page_type;
}


GtkWidget* ctk_color_correction_page_new(NvCtrlAttributeHandle *handle,
                                         CtkConfig *ctk_config,
                                         ParsedAttribute *p,
                                         CtkEvent *ctk_event)
{
    CtkColorCorrectionPage *ctk_color_correction_page;
    ReturnStatus ret;
    GObject *object;
    GtkWidget *banner;
    GtkWidget *ctk_color_correction;
    gint val;

    /* check if the VidMode extension is present */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_ATTR_EXT_VM_PRESENT, &val);
    if ((ret != NvCtrlSuccess) || (val == FALSE)) {
        return NULL;
    }

    /* check if the noScanout mode enabled */

    ret = NvCtrlGetAttribute(handle, NV_CTRL_NO_SCANOUT, &val);
    if ((ret == NvCtrlSuccess) && (val == NV_CTRL_NO_SCANOUT_ENABLED)) {
        return NULL;
    }

    /* allocate the color correction widget */

    ctk_color_correction =
        ctk_color_correction_new(handle, ctk_config, p, ctk_event);

    if (ctk_color_correction == NULL) {
        return NULL;
    }

    /* create the new page */

    object = g_object_new(CTK_TYPE_COLOR_CORRECTION_PAGE, NULL);

    ctk_color_correction_page = CTK_COLOR_CORRECTION_PAGE(object);

    gtk_box_set_spacing(GTK_BOX(ctk_color_correction_page), 10);

    /*
     * pack the banner at the top of the page, followed by the color
     * correction widget
     */

    banner = ctk_banner_image_new(BANNER_ARTWORK_COLOR);
    gtk_box_pack_start(GTK_BOX(ctk_color_correction_page),
                       banner, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(ctk_color_correction_page),
                       ctk_color_correction, TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
}


GtkTextBuffer *ctk_color_correction_page_create_help(GtkTextTagTable *table)
{
    return ctk_color_correction_create_help(table);
}
