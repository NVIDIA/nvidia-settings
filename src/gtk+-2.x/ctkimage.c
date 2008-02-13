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

#include <stdlib.h> /* malloc */
#include <stdio.h> /* snprintf */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>


#include "image.h"
#include "ctkimage.h"
#include "ctkbanner.h"


/*
 * CTK image widget creation
 *
 */
GtkWidget* ctk_image_new(const nv_image_t *img)
{
    guint8 *image_buffer = decompress_image_data(img);
    gboolean has_alpha = FALSE;

    if (!image_buffer) return NULL;

    if (img->bytes_per_pixel == 4) { /* RGBA */
        has_alpha = TRUE;
    }
    
    return gtk_image_new_from_pixbuf
        (gdk_pixbuf_new_from_data(image_buffer, GDK_COLORSPACE_RGB,
                                  has_alpha, 8, img->width, img->height,
                                  img->width * img->bytes_per_pixel,
                                  free_decompressed_image, NULL));
}

GtkWidget* ctk_image_new_from_xpm(const char **img)
{
    if (!img) return NULL;
    
    return gtk_image_new_from_pixbuf(gdk_pixbuf_new_from_xpm_data(img));
}


/*
 * CTK image duplication
 *
 */
GtkWidget *ctk_image_dupe(GtkImage *image)
{
    GtkImageType image_type;
    GtkWidget *new_image = NULL;


    if (!image) return NULL;
    
    image_type = gtk_image_get_storage_type(image);

    switch (image_type) {

    case GTK_IMAGE_PIXBUF:
        {
            GdkPixbuf *pixbuf = gtk_image_get_pixbuf(image);
            new_image = gtk_image_new_from_pixbuf(pixbuf);
        }
        break;

    default:
        /* XXX Support more formats later */
        break;
    }

    return new_image;
}



/*
 * CTK composited banner image widget creation
 *
 */
GtkWidget*  ctk_banner_image_new_with_callback(BannerArtworkType artwork,
                                               ctk_banner_composite_callback callback,
                                               void *data)
{
    GtkWidget *image;
    GtkWidget *hbox;
    GtkWidget *frame;


    image = ctk_banner_new(artwork);

    if (!image) return NULL;

    ctk_banner_set_composite_callback(CTK_BANNER(image), callback, data);

    hbox = gtk_hbox_new(FALSE, 0);
    frame = gtk_frame_new(NULL);

    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(frame), image);


    return hbox;
}


GtkWidget* ctk_banner_image_new(BannerArtworkType artwork)
{
    return ctk_banner_image_new_with_callback(artwork, NULL, NULL);
}
