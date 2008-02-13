/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
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

#include <string.h> /* memcpy(), strerror() */

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkedid.h"

#include "ctkscale.h"
#include "ctkconfig.h"
#include "ctkhelp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>


#define FRAME_PADDING 5

#define DEFAULT_EDID_FILENAME "edid.bin"

static const char *__acquire_edid_help =
"The Acquire EDID button allows you to save the display device's EDID "
"(Extended Display Identification Data) information to a file.";

static void button_clicked(GtkButton *button, gpointer user_data);
static gboolean write_edid_to_file(CtkConfig *ctk_config, const gchar *filename,
                                   unsigned char *data, int len);

GType ctk_edid_get_type(void)
{
    static GType ctk_edid_type = 0;
    
    if (!ctk_edid_type) {
        static const GTypeInfo ctk_edid_info = {
            sizeof (CtkEdidClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkEdid),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_edid_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkEdid", &ctk_edid_info, 0);
    }

    return ctk_edid_type;
}


GtkWidget* ctk_edid_new(NvCtrlAttributeHandle *handle,
                        CtkConfig *ctk_config, CtkEvent *ctk_event,
                        GtkWidget *reset_button,
                        unsigned int display_device_mask,
                        char *name)
{
    CtkEdid *ctk_edid;
    GObject *object;
    GtkWidget *frame, *vbox, *label, *hbox, *alignment;
    ReturnStatus ret;
    gint val;

    /* check if EDID is available for this display device */

    ret = NvCtrlGetDisplayAttribute(handle, display_device_mask,
                                    NV_CTRL_EDID_AVAILABLE, &val);
    
    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_EDID_AVAILABLE_TRUE)) {
        return NULL;
    }

    /* create the object */
    
    object = g_object_new(CTK_TYPE_EDID, NULL);

    ctk_edid = CTK_EDID(object);
    
    ctk_edid->handle = handle;
    ctk_edid->ctk_config = ctk_config;
    ctk_edid->reset_button = reset_button;
    ctk_edid->display_device_mask = display_device_mask;
    ctk_edid->name = name;
    ctk_edid->filename = DEFAULT_EDID_FILENAME;
    ctk_edid->file_selector = gtk_file_selection_new("Please select file where "
                                                     "EDID data will be "
                                                     "saved.");

    gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(ctk_edid->file_selector),
                                           FALSE);

    /* create the frame and vbox */
    
    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);

    /* create the button and label */
    
    label = gtk_label_new("Acquire EDID...");
    hbox = gtk_hbox_new(FALSE, 0);
    ctk_edid->button = gtk_button_new();
    
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 15);
    gtk_container_add(GTK_CONTAINER(ctk_edid->button), hbox);
    
    alignment = gtk_alignment_new(1, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), ctk_edid->button);
                      
    gtk_box_pack_end(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
    
    ctk_config_set_tooltip(ctk_config, ctk_edid->button,
                           __acquire_edid_help);

    g_signal_connect(G_OBJECT(ctk_edid->button),
                     "clicked",
                     G_CALLBACK(button_clicked),
                     (gpointer) ctk_edid);

    gtk_widget_show_all(GTK_WIDGET(object));

    return GTK_WIDGET(object);
    
} /* ctk_edid_new() */


static void button_clicked(GtkButton *button, gpointer user_data)
{
    ReturnStatus ret;
    CtkEdid *ctk_edid = CTK_EDID(user_data);
    unsigned char *data = NULL;
    int len = 0;
    gint result;
    

    /* Grab EDID information */
    
    ret = NvCtrlGetBinaryAttribute(ctk_edid->handle,
                                   ctk_edid->display_device_mask,
                                   NV_CTRL_BINARY_DATA_EDID,
                                   &data, &len);
    if (ret != NvCtrlSuccess) {
        ctk_config_statusbar_message(ctk_edid->ctk_config,
                                     "No EDID available for %s.",
                                     ctk_edid->name);
    } else {

        /* Ask user for filename */
        
        gtk_file_selection_set_filename(GTK_FILE_SELECTION(ctk_edid->file_selector),
                                        ctk_edid->filename);

        result = gtk_dialog_run(GTK_DIALOG(ctk_edid->file_selector));
        gtk_widget_hide(ctk_edid->file_selector);

        switch ( result ) {
        case GTK_RESPONSE_ACCEPT:
        case GTK_RESPONSE_OK:

            ctk_edid->filename =
                gtk_file_selection_get_filename(GTK_FILE_SELECTION(ctk_edid->file_selector));
            
            write_edid_to_file(ctk_edid->ctk_config, ctk_edid->filename,
                               data, len);

            break;
        default:
            return;
        }
        
    } /* EDID available */

    if (data) {
        XFree(data);
    }
    
} /* button_clicked() */


static gboolean write_edid_to_file(CtkConfig *ctk_config, const gchar *filename,
                                   unsigned char *data, int len)
{
    int fd = -1;
    char *dst = (void *) -1;
    char *msg = "";

    fd = open(filename, O_RDWR | O_CREAT | O_TRUNC,
              S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd == -1) {
        msg = "Unable to open file for writing";
        goto fail;
    }
    
    if (lseek(fd, len - 1, SEEK_SET) == -1) {
        msg = "Unable to set file size";
        goto fail;
    }
    
    if (write(fd, "", 1) != 1) {
        msg = "Unable to write output file size";
        goto fail;
    }
    
    if ((dst = mmap(0, len, PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd, 0)) == (void *) -1) {
        msg = "Unable to map file for copying";
        goto fail;
    }
    
    memcpy(dst, data, len);
    
    if (munmap(dst, len) == -1) {
        msg = "Unable to unmap output file";
        goto fail;
    }
    
    close(fd);
    
    ctk_config_statusbar_message(ctk_config,
                                 "EDID written to %s.", filename);
    return TRUE;
    
 fail:

    if (fd != -1) close(fd);

    ctk_config_statusbar_message(ctk_config,
                                 "Unable to write EDID to file '%s': %s (%s).",
                                 filename, msg, strerror(errno));
    return FALSE;

} /* write_edid_to_file() */


void add_acquire_edid_help(GtkTextBuffer *b, GtkTextIter *i)
{
    ctk_help_heading(b, i, "Acquire EDID");
    ctk_help_para(b, i, __acquire_edid_help);

} /* add_acquire_edid_help() */
