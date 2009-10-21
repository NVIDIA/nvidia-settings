/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2009 NVIDIA Corporation.
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

/**** INCLUDES ***************************************************************/

#include <gtk/gtk.h>
#include <NvCtrlAttributes.h>

#include "ctklicense.h"

#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkbanner.h"

static const char * __enable_confirm_msg =
"To use the features on the %s panel you\n"
"must agree to the terms of the preceding license agreement.\n"
"Do you accept this agreement?";

static const char * __license_pre_msg =
"Please read and accept the following license agreement:";

static const char * __license_msg =
"<b>TERMS AND CONDITIONS</b>\n"
"\n"
"WARNING: THE SOFTWARE UTILITY YOU ARE ABOUT TO "
"ENABLE (\"UTILITY\") MAY CAUSE SYSTEM DAMAGE AND "
"VOID WARRANTIES.  THIS UTILITY RUNS YOUR COMPUTER "
"SYSTEM OUT OF THE MANUFACTURER'S DESIGN "
"SPECIFICATIONS, INCLUDING, BUT NOT LIMITED TO: "
"HIGHER SYSTEM VOLTAGES, ABOVE NORMAL "
"TEMPERATURES, EXCESSIVE FREQUENCIES, AND "
"CHANGES TO BIOS THAT MAY CORRUPT THE BIOS.  YOUR "
"COMPUTER'S OPERATING SYSTEM MAY HANG AND RESULT "
"IN DATA LOSS OR CORRUPTED IMAGES.  DEPENDING ON "
"THE MANUFACTURER OF YOUR COMPUTER SYSTEM, THE "
"COMPUTER SYSTEM, HARDWARE AND SOFTWARE "
"WARRANTIES MAY BE VOIDED, AND YOU MAY NOT "
"RECEIVE ANY FURTHER MANUFACTURER SUPPORT."
"NVIDIA DOES NOT PROVIDE CUSTOMER SERVICE SUPPORT "
"FOR THIS UTILITY.  IT IS FOR THESE REASONS THAT "
"ABSOLUTELY NO WARRANTY OR GUARANTEE IS EITHER "
"EXPRESS OR IMPLIED.  BEFORE ENABLING AND USING, YOU "
"SHOULD DETERMINE THE SUITABILITY OF THE UTILITY "
"FOR YOUR INTENDED USE, AND YOU SHALL ASSUME ALL "
"RESPONSIBILITY IN CONNECTION THEREWITH."
"\n"
"\n"
"<b>DISCLAIMER OF WARRANTIES</b>\n"
"\n"
"ALL MATERIALS, INFORMATION, AND SOFTWARE "
"PRODUCTS, INCLUDED IN OR MADE AVAILABLE THROUGH "
"THIS UTILITY ARE PROVIDED \"AS IS\" AND \"AS AVAILABLE\" "
"FOR YOUR USE.  THE UTILITY IS PROVIDED WITHOUT "
"WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, "
"INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF "
"MERCHANTABILITY, FITNESS FOR A PARTICULAR "
"PURPOSE, OR NONINFRINGEMENT.  NVIDIA AND ITS "
"SUBSIDIARIES DO NOT WARRANT THAT THE UTILITY IS "
"RELIABLE OR CORRECT; THAT ANY DEFECTS OR ERRORS "
"WILL BE CORRECTED; OR THAT THE UTILITY IS FREE OF "
"VIRUSES OR OTHER HARMFUL COMPONENTS.  YOUR USE "
"OF THE UTILITY IS SOLELY AT YOUR RISK.  BECAUSE SOME "
"JURISDICTIONS DO NOT PERMIT THE EXCLUSION OF "
"CERTAIN WARRANTIES, THESE EXCLUSIONS MAY NOT "
"APPLY TO YOU."
"\n"
"\n"
"<b>LIMITATION OF LIABILITY</b>\n"
"\n"
"UNDER NO CIRCUMSTANCES SHALL NVIDIA AND ITS "
"SUBSIDIARIES BE LIABLE FOR ANY DIRECT, INDIRECT, "
"PUNITIVE, INCIDENTAL, SPECIAL, OR CONSEQUENTIAL "
"DAMAGES THAT RESULT FROM THE USE OF, OR INABILITY "
"TO USE, THE UTILITY.  THIS LIMITATION APPLIES WHETHER "
"THE ALLEGED LIABILITY IS BASED ON CONTRACT, TORT, "
"NEGLIGENCE, STRICT LIABILITY, OR ANY OTHER BASIS, "
"EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF "
"SUCH DAMAGE.  BECAUSE SOME JURISDICTIONS DO NOT "
"ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR "
"CONSEQUENTIAL DAMAGES, NVIDIA'S LIABILITY IN SUCH "
"JURISDICTIONS SHALL BE LIMITED TO THE EXTENT "
"PERMITTED BY LAW."
"\n"
"IF YOU HAVE READ, UNDERSTOOD, AND AGREE TO ALL OF "
"THE ABOVE TERMS AND CONDITIONS, CLICK THE \"YES\" "
"BUTTON BELOW."
"\n"
"IF YOU DO NOT AGREE WITH ALL OF THE ABOVE TERMS "
"AND CONDITIONS, THEN CLICK ON THE \"NO\" BUTTON "
"BELOW, AND DO NOT ENABLE OR USE THE UTILITY.";

GType ctk_license_dialog_get_type(void)
{
    static GType ctk_license_dialog_type = 0;

    if (!ctk_license_dialog_type) {
        static const GTypeInfo ctk_license_dialog_info = {
            sizeof (CtkLicenseDialogClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            NULL, /* class_init, */
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkLicenseDialog),
            0, /* n_preallocs */
            NULL, /* instance_init */
        };

        ctk_license_dialog_type = g_type_register_static(GTK_TYPE_VBOX,
                                                         "CtkLicenseDialog",
                                                         &ctk_license_dialog_info,
                                                         0);
    }

    return ctk_license_dialog_type;
}



/*****
 *
 * Callback Function - This function gets called when the user scrolls
 * the license agreement text.  Once the user has scrolled to the end
 * of the document, the YES button is activated.
 *
 */
static void license_scrolled(GtkRange *range,
                             gpointer user_data)
{
    CtkLicenseDialog *ctk_license_dialog = CTK_LICENSE_DIALOG(user_data);
    GtkAdjustment *adj = gtk_range_get_adjustment(range);


    /* Enable the dialog's "YES" button once user reaches end of license */

    if ( adj->value + adj->page_size >= adj->upper ) {

        gtk_dialog_set_response_sensitive(GTK_DIALOG(
                                                     ctk_license_dialog->dialog),
                                          GTK_RESPONSE_ACCEPT,
                                          TRUE);
    }
}



/*****
 *
 * ctk_license_run_dialog() - Resize license dialog window and run license
 * dialog. 
 * 
 */
gint ctk_license_run_dialog(CtkLicenseDialog *ctk_license_dialog)
{

    gint w, h;
    gint result;
    GtkRange *range;
    GtkAdjustment *adj;
    GdkScreen * s =
        gtk_window_get_screen(GTK_WINDOW(GTK_DIALOG(ctk_license_dialog->dialog)));

    /* Reset dialog window size */
    
    gtk_window_get_size(GTK_WINDOW(GTK_DIALOG(ctk_license_dialog->dialog)),
                        &w, &h);

    /* Make license dialog default to 55% of the screen height */

    h = (gint)(0.55f * gdk_screen_get_height(s));
    w = 1;

    gtk_window_resize(GTK_WINDOW(GTK_DIALOG(ctk_license_dialog->dialog)),
                      w, h);

    /* Reset scroll bar to the top */

    range =
        GTK_RANGE(GTK_SCROLLED_WINDOW(ctk_license_dialog->window)->vscrollbar);
    adj = gtk_range_get_adjustment(range);

    gtk_adjustment_set_value(adj, 0.0f);

    gtk_widget_show_all(ctk_license_dialog->dialog);
    
    
    /* Sensitize the YES button */

    if ( adj->page_size >= adj->upper ) {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(
                                          ctk_license_dialog->dialog),
                                          GTK_RESPONSE_ACCEPT,
                                          TRUE);
    } else {
        gtk_dialog_set_response_sensitive(GTK_DIALOG(
                                          ctk_license_dialog->dialog),
                                          GTK_RESPONSE_ACCEPT,
                                          FALSE);
    }
    
    result = gtk_dialog_run (GTK_DIALOG(ctk_license_dialog->dialog));
    
    gtk_widget_hide_all(ctk_license_dialog->dialog);

    return result;
}



GtkWidget* ctk_license_dialog_new(GtkWidget *parent, gchar *panel_name)
{
    GObject *object;
    CtkLicenseDialog *ctk_license_dialog;

    GtkWidget *hbox, *label, *scrollWin, *event;
    gchar *enable_message;

    object = g_object_new(CTK_TYPE_LICENSE_DIALOG, NULL);
    ctk_license_dialog = CTK_LICENSE_DIALOG(object);
    
    /* Create the enable dialog */

    ctk_license_dialog->dialog =
        gtk_dialog_new_with_buttons("License Agreement",
                                    GTK_WINDOW(gtk_widget_get_parent(
                                               GTK_WIDGET(parent))),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT |
                                    GTK_DIALOG_NO_SEPARATOR,
                                    GTK_STOCK_YES,
                                    GTK_RESPONSE_ACCEPT,
                                    GTK_STOCK_NO,
                                    GTK_RESPONSE_REJECT,
                                    NULL
                                   );
    
    hbox = gtk_hbox_new(TRUE, 10);
    label = gtk_label_new(__license_pre_msg);

    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(
                        ctk_license_dialog->dialog)->vbox),
                       hbox, FALSE, FALSE, 10);

    scrollWin = gtk_scrolled_window_new(NULL, NULL);
    hbox = gtk_hbox_new(FALSE, 0);
    label = gtk_label_new("");
    event = gtk_event_box_new();
    ctk_license_dialog->window = scrollWin;

    gtk_widget_modify_fg(event, GTK_STATE_NORMAL,
                         &(event->style->text[GTK_STATE_NORMAL]));
    gtk_widget_modify_bg(event, GTK_STATE_NORMAL,
                         &(event->style->base[GTK_STATE_NORMAL]));

    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_label_set_markup(GTK_LABEL(label), __license_msg);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_container_add(GTK_CONTAINER(event), hbox);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollWin),
                                          event);
    hbox = gtk_hbox_new(TRUE, 10);
    gtk_box_pack_start(GTK_BOX(hbox), scrollWin, TRUE, TRUE, 10);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ctk_license_dialog->dialog)->vbox),
                       hbox, TRUE, TRUE, 10);
    hbox = gtk_hbox_new(FALSE, 10);
    enable_message = g_strdup_printf(__enable_confirm_msg, panel_name);
    label = gtk_label_new(enable_message);
    g_free(enable_message);

    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 15);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ctk_license_dialog->dialog)->vbox),
                       hbox, FALSE, FALSE, 10);
    
    g_signal_connect((gpointer)
                     GTK_RANGE(GTK_SCROLLED_WINDOW(scrollWin)->vscrollbar),
                     "value_changed",
                     G_CALLBACK(license_scrolled),
                     (gpointer) ctk_license_dialog);
    
    return GTK_WIDGET(object);

} /* ctk_license_dialog_new() */

