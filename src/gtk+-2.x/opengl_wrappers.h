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

#ifndef __OPENGL_WRAPPERS_H__
#define __OPENGL_WRAPPERS_H__

#include "ctkutils.h"

#ifdef CTK_GTK3

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <GL/gl.h>

typedef struct _uniformVec4f uniformVec4f;
struct _uniformVec4f
{
    GLuint index;
    float data[4];
};

typedef struct _OpenGLModelData OpenGLModelData;
struct _OpenGLModelData
{
    GLuint vao;
    int vboLen;
    GLuint tex;
    GLuint program;
    float *mvp;
    uniformVec4f *uniforms;
    int uniformCount;
};

void freeOglmd(OpenGLModelData *p);

extern char const * const cubeVertexShaderSource[1];
extern char const * const cubeFragmentShaderSource[1];
extern char const * const textVertexShaderSource[1];
extern char const * const textFragmentShaderSource[1];

void drawModel(OpenGLModelData *data);

OpenGLModelData *cubeSetup(GdkPixbuf *image);

OpenGLModelData *labelSetup(float x, float y,
                            float width, float height,
                            float red, float green, float blue,
                            GdkPixbuf *image);

#endif

#endif /*__OPENGL_WRAPPERS_H__*/

