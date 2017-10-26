/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2017 NVIDIA Corporation.
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

#ifndef __CTK_GLSTEREO_H__
#define __CTK_GLSTEREO_H__

#include "ctkutils.h"

#ifdef CTK_GTK3
#include "opengl_wrappers.h"
#endif

GtkWidget *ctk_glstereo_new(void);


#ifdef CTK_GTK3
typedef struct _StereoAppData StereoAppData;
struct _StereoAppData
{
    OpenGLModelData *cube;
    OpenGLModelData *labelLeft;
    OpenGLModelData *labelRight;
    unsigned int animationCounter;
};

#endif

#endif
