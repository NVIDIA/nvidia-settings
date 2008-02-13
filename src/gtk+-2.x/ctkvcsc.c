/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2006 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
 * of the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *           Free Software Foundation, Inc.
 *           59 Temple Place - Suite 330
 *           Boston, MA 02111-1307, USA
 *
 */

#include <stdlib.h> /* malloc */
#include <stdio.h> /* snprintf */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include "ctkimage.h"
#include "frame_lock_banner.h"

#include "ctkvcsc.h"
#include "ctkevent.h"
#include "ctkhelp.h"
#include "ctkutils.h"


GType ctk_vcsc_get_type(void)
{
    static GType ctk_vcsc_type = 0;

    if (!ctk_vcsc_type) {
        static const GTypeInfo ctk_vcsc_info = {
            sizeof (CtkVcscClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(CtkVcsc),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_vcsc_type = g_type_register_static
            (GTK_TYPE_VBOX, "CtkVcsc", &ctk_vcsc_info, 0);
    }

    return ctk_vcsc_type;
}



/*
 * CTK VCSC (Visual Computing System Controller) widget creation
 *
 */
GtkWidget* ctk_vcsc_new(NvCtrlAttributeHandle *handle,
                        CtkConfig *ctk_config)
{
    GObject *object;
    CtkVcsc *ctk_object;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *banner;
    GtkWidget *hseparator;
    GtkWidget *table;

    gchar *product_name;
    gchar *serial_number;
    gchar *build_date;
    gchar *product_id;
    gchar *firmware_version;
    gchar *hardware_version;

    ReturnStatus ret;

    /*
     * get the static string data that we will display below
     */
    
    /* Product Name */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_PRODUCT_NAME,
                                   &product_name);
    if (ret != NvCtrlSuccess) {
        product_name = g_strdup("Unable to determine");
    }

    /* Serial Number */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_SERIAL_NUMBER,
                                   &serial_number);
    if (ret != NvCtrlSuccess) {
        serial_number = g_strdup("Unable to determine");
    }

    /* Build Date */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_BUILD_DATE,
                                   &build_date);
    if (ret != NvCtrlSuccess) {
        build_date = g_strdup("Unable to determine");
    }

    /* Product ID */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_PRODUCT_ID,
                                   &product_id);
    if (ret != NvCtrlSuccess) {
        product_id = g_strdup("Unable to determine");
    }

    /* Firmware Version */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_FIRMWARE_VERSION,
                                   &firmware_version);
    if (ret != NvCtrlSuccess) {
        firmware_version = g_strdup("Unable to determine");
    }

    /* Hardware Version */
    ret = NvCtrlGetStringAttribute(handle,
                                   NV_CTRL_STRING_VCSC_HARDWARE_VERSION,
                                   &hardware_version);
    if (ret != NvCtrlSuccess) {
        hardware_version = g_strdup("Unable to determine");
    }


    /* now, create the object */
    
    object = g_object_new(CTK_TYPE_VCSC, NULL);
    ctk_object = CTK_VCSC(object);

    /* cache the attribute handle */

    ctk_object->handle = handle;

    /* set container properties of the object */

    gtk_box_set_spacing(GTK_BOX(ctk_object), 10);

    /* banner */

    banner = ctk_banner_image_new(&frame_lock_banner_image);
    gtk_box_pack_start(GTK_BOX(ctk_object), banner, FALSE, FALSE, 0);

    /*
     * This displays basic System information, including
     * display name, Operating system type and the NVIDIA driver version.
     */

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start(GTK_BOX(ctk_object), vbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("VCSC Information");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hseparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(hbox), hseparator, TRUE, TRUE, 5);

    table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

    gtk_table_set_row_spacings(GTK_TABLE(table), 3);
    gtk_table_set_col_spacings(GTK_TABLE(table), 15);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);

    add_table_row(table, 0,
                  0, 0.5, "Product Name:", 0, 0.5, product_name);
    add_table_row(table, 1,
                  0, 0.5, "Serial Number:", 0, 0.5, serial_number);
    add_table_row(table, 2,
                  0, 0.5, "Build Date:", 0, 0.5, build_date);
    add_table_row(table, 3,
                  0, 0.5, "Product ID:", 0, 0.5, product_id);
    add_table_row(table, 4,
                  0, 0.5, "Firmware version:", 0, 0.5, firmware_version);
    add_table_row(table, 5,
                  0, 0.5, "Hardware version:", 0, 0.5, hardware_version);

    g_free(product_name);
    g_free(serial_number);
    g_free(build_date);
    g_free(product_id);
    g_free(firmware_version);
    g_free(hardware_version);

    gtk_widget_show_all(GTK_WIDGET(object));
    
    return GTK_WIDGET(object);
}



/*
 * VCSC help screen
 */
GtkTextBuffer *ctk_vcsc_create_help(GtkTextTagTable *table,
                                    CtkVcsc *ctk_object)
{
    GtkTextIter i;
    GtkTextBuffer *b;
    
    b = gtk_text_buffer_new(table);
    
    gtk_text_buffer_get_iter_at_offset(b, &i, 0);

    ctk_help_title(b, &i, "VCSC (Visual Computing System Controller) Help");

    ctk_help_heading(b, &i, "Product Name");
    ctk_help_para(b, &i, "This is the product name of the VCSC system.");
    
    ctk_help_heading(b, &i, "Serial Number");
    ctk_help_para(b, &i, "This is the unique serial number of the VCSC "
                  "system.");

    ctk_help_heading(b, &i, "Build Date");
    ctk_help_para(b, &i, "This is the date the VCSC system was build, "
                  "shown in a 'week.year' format");

    ctk_help_heading(b, &i, "Product ID");
    ctk_help_para(b, &i, "This identifies the VCSC configuration.");

    ctk_help_heading(b, &i, "Firmware Version");
    ctk_help_para(b, &i, "This is the firmware version currently running on "
                  "the VCSC system.");

    ctk_help_heading(b, &i, "Hardware Version");
    ctk_help_para(b, &i, "This is the hardware version of the VCSC system.");

    ctk_help_finish(b);

    return b;
}

