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

#ifndef __IMAGE_H__
#define __IMAGE_H__

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
    unsigned int fill_column_index;
    char *rle_pixel_data;
} nv_image_t;

unsigned char *decompress_image_data(const nv_image_t *img);
void free_decompressed_image(unsigned char *buf, void *ptr);


/*
 * enum for the banner artwork
 */

typedef enum {
    BANNER_ARTWORK_ANTIALIAS,
    BANNER_ARTWORK_BSD,
    BANNER_ARTWORK_CLOCK,
    BANNER_ARTWORK_COLOR,
    BANNER_ARTWORK_CONFIG,
    BANNER_ARTWORK_CRT,
    BANNER_ARTWORK_CURSOR_SHADOW,
    BANNER_ARTWORK_DFP,
    BANNER_ARTWORK_DISPLAY_CONFIG,
    BANNER_ARTWORK_FRAMELOCK,
    BANNER_ARTWORK_GLX,
    BANNER_ARTWORK_GPU,
    BANNER_ARTWORK_HELP,
    BANNER_ARTWORK_OPENGL,
    BANNER_ARTWORK_PENGUIN,
    BANNER_ARTWORK_ROTATION,
    BANNER_ARTWORK_SDI,
    BANNER_ARTWORK_SOLARIS,
    BANNER_ARTWORK_THERMAL,
    BANNER_ARTWORK_TV,
    BANNER_ARTWORK_VCSC,
    BANNER_ARTWORK_X,
    BANNER_ARTWORK_XVIDEO,
    BANNER_ARTWORK_INVALID,
} BannerArtworkType;


#endif /* __IMAGE_H__ */
