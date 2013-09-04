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

#include <string.h> /* memcpy(), strerror() */

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctkedid.h"

#include "ctkscale.h"
#include "ctkconfig.h"
#include "ctkhelp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>


#define FRAME_PADDING 5

/* file formats */
#define FILE_FORMAT_BINARY 1
#define FILE_FORMAT_ASCII  2

/* default file names */
#define DEFAULT_EDID_FILENAME_BINARY "edid.bin"
#define DEFAULT_EDID_FILENAME_ASCII  "edid.txt"

static const char *__acquire_edid_help =
"The Acquire EDID button allows you to save the display device's EDID "
"(Extended Display Identification Data) information to a file.  By "
"default it saves information in binary format but one can also choose "
"to save in ASCII format.";

static void file_format_changed(GtkWidget *widget, gpointer user_data);
static void normalize_filename(CtkEdid *ctk_edid);
static void button_clicked(GtkButton *button, gpointer user_data);
static gboolean write_edid_to_file(CtkConfig *ctk_config, const gchar *filename,
                                   int format, unsigned char *data, int len);

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
            NULL  /* value_table */
        };

        ctk_edid_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkEdid", &ctk_edid_info, 0);
    }

    return ctk_edid_type;
}


void ctk_edid_setup(CtkEdid *ctk_object)
{
    ReturnStatus ret;
    gint val;

    ret = NvCtrlGetAttribute(ctk_object->handle, NV_CTRL_EDID_AVAILABLE, &val);

    if ((ret != NvCtrlSuccess) || (val != NV_CTRL_EDID_AVAILABLE_TRUE)) {
        gtk_widget_set_sensitive(ctk_object->button, FALSE);
        return;
    }

    gtk_widget_set_sensitive(ctk_object->button, TRUE);
}


GtkWidget* ctk_edid_new(NvCtrlAttributeHandle *handle,
                        CtkConfig *ctk_config, CtkEvent *ctk_event,
                        char *name)
{
    CtkEdid *ctk_edid;
    GObject *object;
    GtkWidget *frame, *vbox, *label, *hbox, *alignment;

    /* create the object */

    object = g_object_new(CTK_TYPE_EDID, NULL);
    if (!object) return NULL;

    ctk_edid = CTK_EDID(object);

    ctk_edid->handle = handle;
    ctk_edid->ctk_config = ctk_config;
    ctk_edid->name = name;
    ctk_edid->filename = DEFAULT_EDID_FILENAME_BINARY;
    ctk_edid->file_format = FILE_FORMAT_BINARY;
    ctk_edid->file_selector = gtk_file_selection_new("Please select file where "
                                                     "EDID data will be "
                                                     "saved.");

    gtk_file_selection_set_select_multiple
        (GTK_FILE_SELECTION(ctk_edid->file_selector),
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

    /* adding file format selection option to file selector dialog */

    frame = gtk_frame_new(NULL);
    gtk_box_pack_start
        (GTK_BOX(GTK_FILE_SELECTION(ctk_edid->file_selector)->main_vbox),
         frame, FALSE, FALSE, 15);
    gtk_box_reorder_child
        (GTK_BOX(GTK_FILE_SELECTION(ctk_edid->file_selector)->main_vbox),
         frame, 0);

    hbox = gtk_hbox_new(FALSE, 10);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    label = gtk_label_new("EDID File Format: ");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    ctk_edid->file_format_binary_radio_button =
        gtk_radio_button_new_with_label(NULL, "Binary");
    gtk_box_pack_start(GTK_BOX(hbox), ctk_edid->file_format_binary_radio_button,
                       FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(ctk_edid->file_format_binary_radio_button),
                     "toggled", G_CALLBACK(file_format_changed),
                     (gpointer) ctk_edid);

    ctk_edid->file_format_ascii_radio_button =
        gtk_radio_button_new_with_label_from_widget
        (GTK_RADIO_BUTTON(ctk_edid->file_format_binary_radio_button),
         "ASCII");
    gtk_box_pack_start(GTK_BOX(hbox), ctk_edid->file_format_ascii_radio_button,
                       FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(ctk_edid->file_format_ascii_radio_button),
                     "toggled", G_CALLBACK(file_format_changed),
                     (gpointer) ctk_edid);

    gtk_window_set_resizable(GTK_WINDOW(ctk_edid->file_selector),
                             FALSE);
    gtk_toggle_button_set_active
        (GTK_TOGGLE_BUTTON(ctk_edid->file_format_binary_radio_button), TRUE);
    gtk_widget_show_all(GTK_FILE_SELECTION(ctk_edid->file_selector)->main_vbox);

    gtk_widget_show_all(GTK_WIDGET(object));

    ctk_edid_setup(ctk_edid);

    return GTK_WIDGET(object);
    
} /* ctk_edid_new() */


static void normalize_filename(CtkEdid *ctk_edid)
{
    char *buffer = NULL, *filename = NULL;
    char *end = NULL, *slash = NULL;
    int len = 0, n;

    ctk_edid->filename =
        gtk_file_selection_get_filename(GTK_FILE_SELECTION(ctk_edid->file_selector));

    len = strlen(ctk_edid->filename);
    filename = malloc(len + 1);
    if (!filename) {
        goto done;
    }
    strncpy(filename, ctk_edid->filename, len);

    /*
     * It is possible that filename is entered without any extension,
     * in that case we need to make room for the extension string e.g.
     * '.bin' or '.txt', so total buffer length will be filename plus 5.
     */
    buffer = malloc(len + 5);
    if (!buffer) {
        goto done;
    }

    /* find the last forward slash (or the start of the filename) */

    slash = strrchr(filename, '/');
    if (!slash) {
        slash = filename;
    }

    /*
     * find where to truncate the filename: either the last period
     * after 'slash', or the end of the filename
     */

    for (end = filename + len; end > slash; end--) {
        if (*end == '.') break;
    }

    if (end == slash) {
        end = filename + len;
    }

    /*
     * print the characters between filename and end; then append the
     * suffix
     */
    n = end - filename;
    strncpy(buffer, filename, n);

    if (gtk_toggle_button_get_active
        (GTK_TOGGLE_BUTTON(ctk_edid->file_format_binary_radio_button))) {
        ctk_edid->file_format = FILE_FORMAT_BINARY;
        snprintf(buffer + n, 5, ".bin");
    } else if (gtk_toggle_button_get_active
               (GTK_TOGGLE_BUTTON(ctk_edid->file_format_ascii_radio_button))) {
        ctk_edid->file_format = FILE_FORMAT_ASCII;
        snprintf(buffer + n, 5, ".txt");
    }

    /* modify the file name as per the format selected */
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(ctk_edid->file_selector),
                                    buffer);
 done:
    free(filename);
    free(buffer);
}

static void file_format_changed(GtkWidget *widget, gpointer user_data)
{
    CtkEdid *ctk_edid = CTK_EDID(user_data);
    normalize_filename(ctk_edid);
}

static void button_clicked(GtkButton *button, gpointer user_data)
{
    ReturnStatus ret;
    CtkEdid *ctk_edid = CTK_EDID(user_data);
    unsigned char *data = NULL;
    int len = 0;
    gint result;
    

    /* Grab EDID information */
    
    ret = NvCtrlGetBinaryAttribute(ctk_edid->handle, 0,
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

            normalize_filename(ctk_edid);
            ctk_edid->filename =
                gtk_file_selection_get_filename(GTK_FILE_SELECTION(ctk_edid->file_selector));
            
            write_edid_to_file(ctk_edid->ctk_config, ctk_edid->filename,
                               ctk_edid->file_format, data, len);

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
                                   int format, unsigned char *data, int len)
{
    int i;
    FILE *fp = NULL;
    char *msg = "";
    char *tmpbuf = NULL, *pbuf = NULL;


    if (format == FILE_FORMAT_ASCII) {
        fp = fopen(filename, "wt");
        if (!fp) {
            msg = "ASCII Mode: Unable to open file for writing";
            goto fail;
        }
        /*
         * for printing every member we reserve 2 locations i.e. %02x and
         * one extra space to comply with NVIDIA Windows Control Panel
         * ASCII file output, so in all 3 bytes are required for every entry.
         */
        tmpbuf = calloc(1, 1 + (len * 3));
        if (!tmpbuf) {
            msg = "ASCII Mode: Could not allocate enough memory";
            goto fail;
        }
        pbuf = tmpbuf;

        for (i = 0; i < len; i++) {
            if (sprintf(pbuf, "%02x ", data[i]) < 0) {
                msg = "ASCII Mode: Unable to write to buffer";
                goto fail;
            }
            pbuf = pbuf + 3;
        }
        /* being extra cautious */
        sprintf(pbuf, "%c", '\0');

        if (fprintf(fp, "%s", tmpbuf) < 0) {
            msg = "ASCII Mode: Unable to write to file";
            goto fail;
        }

        free(tmpbuf);
        tmpbuf = pbuf = NULL;

    } else {
        fp = fopen(filename, "wb");
        if (!fp) {
            msg = "Binary Mode: Unable to open file for writing";
            goto fail;
        }

        if (fwrite(data, 1, len, fp) != len) {
            msg = "Binary Mode: Unable to write to file";
            goto fail;
        }
    }

    fclose(fp);

    ctk_config_statusbar_message(ctk_config,
                                 "EDID written to %s.", filename);
    return TRUE;
    
 fail:

    free(tmpbuf);
    tmpbuf = pbuf = NULL;

    if (fp) {
        fclose(fp);
    }

    ctk_config_statusbar_message(ctk_config,
                                 "Unable to write EDID to file '%s': %s (%s).",
                                 filename, msg, strerror(errno));
    return FALSE;

} /* write_edid_to_file() */


void add_acquire_edid_help(GtkTextBuffer *b, GtkTextIter *i)
{
    ctk_help_heading(b, i, "Acquire EDID");
    ctk_help_para(b, i, "%s", __acquire_edid_help);

} /* add_acquire_edid_help() */
