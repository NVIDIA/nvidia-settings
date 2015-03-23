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
#include <NvCtrlAttributes.h>

#include "ctkimagesliders.h"

#include "ctkscale.h"
#include "ctkconfig.h"
#include "ctkhelp.h"
#include "ctkutils.h"


#define FRAME_PADDING 5

static const char *__digital_vibrance_help = "The Digital Vibrance slider "
"alters the level of Digital Vibrance for this display device.";

static const char *__image_sharpening_help = "The Image Sharpening slider "
"alters the level of Image Sharpening for this display device.";


static void ctk_image_sliders_class_init(CtkImageSliders *ctk_object_class);
static void ctk_image_sliders_finalize(GObject *object);

static GtkWidget * add_scale(CtkConfig *ctk_config,
                             int attribute,
                             char *name,
                             const char *help,
                             gint value_type,
                             gint default_value,
                             gpointer callback_data);

static void setup_scale(CtkImageSliders *ctk_image_sliders,
                        int attribute, GtkWidget *scale);

static void setup_reset_button(CtkImageSliders *ctk_image_sliders);

static void scale_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data);

static void scale_value_received(GObject *, CtrlEvent *, gpointer);


GType ctk_image_sliders_get_type(void)
{
    static GType ctk_image_sliders_type = 0;

    if (!ctk_image_sliders_type) {
        static const GTypeInfo ctk_image_sliders_info = {
            sizeof (CtkImageSlidersClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc) ctk_image_sliders_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof (CtkImageSliders),
            0, /* n_preallocs */
            NULL, /* instance_init */
            NULL  /* value_table */
        };

        ctk_image_sliders_type = g_type_register_static (GTK_TYPE_VBOX,
                "CtkImageSliders", &ctk_image_sliders_info, 0);
    }

    return ctk_image_sliders_type;
}

static void ctk_image_sliders_class_init(
    CtkImageSliders *ctk_object_class
)
{
    GObjectClass *gobject_class = (GObjectClass *)ctk_object_class;
    gobject_class->finalize = ctk_image_sliders_finalize;
}

static void ctk_image_sliders_finalize(
    GObject *object
)
{
    CtkImageSliders *ctk_image_sliders = CTK_IMAGE_SLIDERS(object);

    g_signal_handlers_disconnect_matched(G_OBJECT(ctk_image_sliders->ctk_event),
                                         G_SIGNAL_MATCH_DATA,
                                         0, /* signal_id */
                                         0, /* detail */
                                         NULL, /* closure */
                                         NULL, /* func */
                                         (gpointer) ctk_image_sliders);
}



GtkWidget* ctk_image_sliders_new(CtrlTarget *ctrl_target,
                                 CtkConfig *ctk_config, CtkEvent *ctk_event,
                                 GtkWidget *reset_button,
                                 char *name)
{
    CtkImageSliders *ctk_image_sliders;

    GObject *object;

    GtkWidget *frame;
    GtkWidget *vbox;
    ReturnStatus status;
    gint val;

    /*
     * now that we know that we will have at least one attribute,
     * create the object
     */

    object = g_object_new(CTK_TYPE_IMAGE_SLIDERS, NULL);
    if (!object) return NULL;

    ctk_image_sliders = CTK_IMAGE_SLIDERS(object);
    ctk_image_sliders->ctrl_target = ctrl_target;
    ctk_image_sliders->ctk_config = ctk_config;
    ctk_image_sliders->ctk_event = ctk_event;
    ctk_image_sliders->reset_button = reset_button;
    ctk_image_sliders->name = name;

    /* create the frame and vbox */

    frame = gtk_frame_new(NULL);
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), FRAME_PADDING);
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(object), frame, FALSE, FALSE, 0);
    ctk_image_sliders->frame = frame;

    /* Digital Vibrance */

    ctk_image_sliders->digital_vibrance =
        add_scale(ctk_config,
                  NV_CTRL_DIGITAL_VIBRANCE, "Digital Vibrance",
                  __digital_vibrance_help, G_TYPE_INT,
                  0, /* default value */
                  ctk_image_sliders);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_DIGITAL_VIBRANCE),
                     G_CALLBACK(scale_value_received),
                     (gpointer) ctk_image_sliders);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_image_sliders->digital_vibrance,
                       TRUE, TRUE, 0);

    /* Image Sharpening */

    status = NvCtrlGetAttribute(ctrl_target,
                                NV_CTRL_IMAGE_SHARPENING_DEFAULT,
                                &val);
    if (status != NvCtrlSuccess) {
        val = 0;
    }

    ctk_image_sliders->image_sharpening =
        add_scale(ctk_config,
                  NV_CTRL_IMAGE_SHARPENING, "Image Sharpening",
                  __image_sharpening_help, G_TYPE_INT, val, ctk_image_sliders);

    g_signal_connect(G_OBJECT(ctk_event),
                     CTK_EVENT_NAME(NV_CTRL_IMAGE_SHARPENING),
                     G_CALLBACK(scale_value_received),
                     (gpointer) ctk_image_sliders);

    gtk_box_pack_start(GTK_BOX(vbox), ctk_image_sliders->image_sharpening,
                       TRUE, TRUE, 0);

    gtk_widget_show_all(GTK_WIDGET(object));

    /* update the GUI */

    ctk_image_sliders_setup(ctk_image_sliders);

    return GTK_WIDGET(object);

} /* ctk_image_sliders_new() */



/*
 * add_scale() - if the specified attribute exists and we can
 * query its valid values, create a new scale widget
 */

static GtkWidget * add_scale(CtkConfig *ctk_config,
                             int attribute,
                             char *name,
                             const char *help,
                             gint value_type,
                             gint default_value,
                             gpointer callback_data)
{
    GtkAdjustment *adj;
    GtkWidget *scale;


    adj = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 10, 1, 1, 0));

    g_object_set_data(G_OBJECT(adj), "attribute",
                      GINT_TO_POINTER(attribute));

    g_object_set_data(G_OBJECT(adj), "attribute name", name);

    g_object_set_data(G_OBJECT(adj), "attribute default value",
                      GINT_TO_POINTER(default_value));

    g_signal_connect(G_OBJECT(adj), "value_changed",
                     G_CALLBACK(scale_value_changed),
                     (gpointer) callback_data);

    scale = ctk_scale_new(GTK_ADJUSTMENT(adj), name, ctk_config, value_type);

    ctk_config_set_tooltip(ctk_config, CTK_SCALE_TOOLTIP_WIDGET(scale),
                           help);

    return scale;

} /* add_scale() */



/*
 * post_scale_value_changed() - helper function for
 * scale_value_changed() and value_changed(); this does whatever
 * work is necessary after the adjustment has been updated --
 * currently, this just means posting a statusbar message.
 */

static void post_scale_value_changed(GtkAdjustment *adjustment,
                                     CtkImageSliders *ctk_image_sliders,
                                     gint value)
{
    char *name = g_object_get_data(G_OBJECT(adjustment), "attribute name");

    gtk_widget_set_sensitive(ctk_image_sliders->reset_button, TRUE);

    ctk_config_statusbar_message(ctk_image_sliders->ctk_config,
                                 "%s set to %d.", name, value);

} /* post_scale_value_changed() */



/*
 * scale_value_changed() - callback when any of the adjustments
 * in the CtkImageSliders are changed: get the new value from the
 * adjustment, send it to the X server, and do any post-adjustment
 * work.
 */

static void scale_value_changed(GtkAdjustment *adjustment,
                                     gpointer user_data)
{
    CtkImageSliders *ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);
    CtrlTarget *ctrl_target = ctk_image_sliders->ctrl_target;

    gint value;
    gint attribute;

    value = (gint) gtk_adjustment_get_value(adjustment);

    user_data = g_object_get_data(G_OBJECT(adjustment), "attribute");
    attribute = GPOINTER_TO_INT(user_data);

    NvCtrlSetAttribute(ctrl_target, attribute, (int) value);

    post_scale_value_changed(adjustment, ctk_image_sliders, value);

} /* scale_value_changed() */



/*
 * ctk_image_sliders_reset() - resets sliders to their default values
 */

void ctk_image_sliders_reset(CtkImageSliders *ctk_image_sliders)
{
    CtrlTarget *ctrl_target;
    GtkAdjustment *adj;
    gint val;

    if (!ctk_image_sliders) return;

    ctrl_target = ctk_image_sliders->ctrl_target;

    if (ctk_widget_get_sensitive(ctk_image_sliders->digital_vibrance)) {
        adj = CTK_SCALE(ctk_image_sliders->digital_vibrance)->gtk_adjustment;
        val = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adj),
                                                "attribute default value"));

        NvCtrlSetAttribute(ctrl_target, NV_CTRL_DIGITAL_VIBRANCE, val);
    }

    if (ctk_widget_get_sensitive(ctk_image_sliders->image_sharpening)) {
        adj = CTK_SCALE(ctk_image_sliders->image_sharpening)->gtk_adjustment;
        val = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adj),
                                                "attribute default value"));

        NvCtrlSetAttribute(ctrl_target, NV_CTRL_IMAGE_SHARPENING, val);
    }

    /*
     * The above may have triggered events.  Such an event will
     * cause scale_value_changed() and post_scale_value_changed() to
     * be called when control returns to the gtk_main loop.
     * post_scale_value_changed() will write a status message to the
     * statusbar.
     *
     * However, the caller of ctk_image_sliders_reset() (e.g.,
     * ctkdisplaydevice.c:reset_button_clicked()) may also want to
     * write a status message to the statusbar.  To ensure that the
     * caller's statusbar message takes precedence (i.e., is the last
     * thing written to the statusbar), process any generated events
     * now, before returning to the caller.
     */

    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }

    ctk_image_sliders_setup(ctk_image_sliders);

} /* ctk_image_sliders_reset() */



/*
 * scale_value_received() - callback function for changed image settings; this
 * is called when we receive an event indicating that another
 * NV-CONTROL client changed any of the settings that we care about.
 */

static void scale_value_received(GObject *object,
                                 CtrlEvent *event,
                                 gpointer user_data)
{
    CtkImageSliders *ctk_image_sliders = CTK_IMAGE_SLIDERS(user_data);

    GtkAdjustment *adj;
    GtkWidget *scale;
    gint val;

    if (event->type != CTRL_EVENT_TYPE_INTEGER_ATTRIBUTE) {
        return;
    }
    
    switch (event->int_attr.attribute) {
    case NV_CTRL_DIGITAL_VIBRANCE:
        scale = ctk_image_sliders->digital_vibrance;
        break;
    case NV_CTRL_IMAGE_SHARPENING:
        scale = ctk_image_sliders->image_sharpening;
        break;
    default:
        return;
    }

    if (event->int_attr.is_availability_changed) {
        setup_scale(ctk_image_sliders, event->int_attr.attribute, scale);
    }

    adj = CTK_SCALE(scale)->gtk_adjustment;
    val = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

    if (val != event->int_attr.value) {
        
        val = event->int_attr.value;

        g_signal_handlers_block_by_func(adj, scale_value_changed,
                                        ctk_image_sliders);
        
        gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), val);
        
        post_scale_value_changed(GTK_ADJUSTMENT(adj),
                                 ctk_image_sliders, val);
        
        g_signal_handlers_unblock_by_func(adj, scale_value_changed,
                                          ctk_image_sliders);
    }

} /* scale_value_received() */



/*
 * add_image_sliders_help() - 
 */

void add_image_sliders_help(CtkImageSliders *ctk_image_sliders,
                            GtkTextBuffer *b,
                            GtkTextIter *i)
{
    ctk_help_heading(b, i, "Digital Vibrance");
    ctk_help_para(b, i, "Digital Vibrance, a mechanism for "
                  "controlling color separation and intensity, boosts "
                  "the color saturation of an image so that all images "
                  "including 2D, 3D, and video appear brighter and "
                  "crisper (even on flat panels) in your applications.");

    ctk_help_heading(b, i, "Image Sharpening");
    ctk_help_para(b, i, "Use the Image Sharpening slider to adjust the "
                  "sharpness of the image quality by amplifying high "
                  "frequency content.");
    
} /* add_image_sliders_help() */



/* Update GUI state of the scale to reflect current settings
 * on the X Driver.
 */

static void setup_scale(CtkImageSliders *ctk_image_sliders,
                        int attribute,
                        GtkWidget *scale)
{
    CtrlTarget *ctrl_target = ctk_image_sliders->ctrl_target;
    ReturnStatus ret0, ret1;
    CtrlAttributeValidValues valid;
    int val;
    GtkAdjustment *adj = CTK_SCALE(scale)->gtk_adjustment;


    /* Read settings from X server */
    ret0 = NvCtrlGetValidAttributeValues(ctrl_target, attribute, &valid);

    ret1 = NvCtrlGetAttribute(ctrl_target, attribute, &val);

    if ((ret0 == NvCtrlSuccess) && (ret1 == NvCtrlSuccess) &&
        (valid.valid_type == CTRL_ATTRIBUTE_VALID_TYPE_RANGE)) {

        g_signal_handlers_block_by_func(adj, scale_value_changed,
                                        ctk_image_sliders);

        ctk_adjustment_set_lower(adj, valid.range.min);
        ctk_adjustment_set_upper(adj, valid.range.max);
        gtk_adjustment_changed(GTK_ADJUSTMENT(adj));

        gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), val);

        g_signal_handlers_unblock_by_func(adj, scale_value_changed,
                                          ctk_image_sliders);

        gtk_widget_set_sensitive(scale, TRUE);
    } else {
        gtk_widget_set_sensitive(scale, FALSE);
    }

} /* setup_scale() */



static void setup_reset_button(CtkImageSliders *ctk_image_sliders)
{
    GtkWidget *scale;
    GtkAdjustment *adj;
    gint default_val;
    gint current_val;


    /* Reset button should be sensitive if all scales are sensitive and
     * at least one scale is set to the non-default value
     */

    scale = ctk_image_sliders->digital_vibrance;
    if (ctk_widget_get_sensitive(scale)) {
        adj = CTK_SCALE(scale)->gtk_adjustment;
        current_val = (gint) gtk_adjustment_get_value(adj);
        default_val =
            GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adj),
                                              "attribute default value"));
        if (current_val != default_val) {
            goto enable;
        }
    }

    scale = ctk_image_sliders->image_sharpening;
    if (ctk_widget_get_sensitive(scale)) {
        adj = CTK_SCALE(scale)->gtk_adjustment;
        current_val = (gint) gtk_adjustment_get_value(adj);
        default_val =
            GPOINTER_TO_INT(g_object_get_data(G_OBJECT(adj),
                                              "attribute default value"));
        if (current_val != default_val) {
            goto enable;
        }
    }

    /* Don't disable reset button here, since other settings that are not
     * managed by the ctk_image_slider here may need it enabled
     */
    return;

 enable:
    gtk_widget_set_sensitive(ctk_image_sliders->reset_button, TRUE);
}



/*
 * Updates the page to reflect the current configuration of
 * the display device.
 */
void ctk_image_sliders_setup(CtkImageSliders *ctk_image_sliders)
{
    if (!ctk_image_sliders) return;

    /* Update sliders */

    /* NV_CTRL_DIGITAL_VIBRANCE */

    setup_scale(ctk_image_sliders, NV_CTRL_DIGITAL_VIBRANCE,
                ctk_image_sliders->digital_vibrance);

    /* NV_CTRL_IMAGE_SHARPENING */

    setup_scale(ctk_image_sliders, NV_CTRL_IMAGE_SHARPENING,
                ctk_image_sliders->image_sharpening);


    setup_reset_button(ctk_image_sliders);

} /* ctk_image_sliders_setup() */
