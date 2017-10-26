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

#include <stdio.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include "opengl_loading.h"

libGLData dGL;

#define LOAD_GL_FUNCTION(func) \
    dGL.func = dGL.glXGetProcAddress((const GLubyte *) #func); \
    if (dGL.func == NULL) { \
        fprintf(stderr, "Failed to load " #func "\n"); \
        return GL_FALSE; \
    }

GLboolean loadGL(void)
{
    dGL.handle = dlopen("libGL.so.1", RTLD_LAZY);
    if (dGL.handle == NULL) {
        return GL_FALSE;
    }

    dGL.glXGetProcAddress = dlsym(dGL.handle, "glXGetProcAddress");
    if (dGL.glXGetProcAddress == NULL) {
        return GL_FALSE;
    }

    LOAD_GL_FUNCTION(glGetString)
    LOAD_GL_FUNCTION(glGetIntegerv)
    LOAD_GL_FUNCTION(glGetStringi)

    LOAD_GL_FUNCTION(glXCreateNewContext)
    LOAD_GL_FUNCTION(glXDestroyContext)
    LOAD_GL_FUNCTION(glXMakeContextCurrent)
    LOAD_GL_FUNCTION(glXSwapBuffers)
    LOAD_GL_FUNCTION(glXChooseFBConfig)
    LOAD_GL_FUNCTION(glXGetFBConfigAttrib)
    LOAD_GL_FUNCTION(glXGetProcAddress)
    LOAD_GL_FUNCTION(glXCreateWindow)
    LOAD_GL_FUNCTION(glXGetVisualFromFBConfig)

    LOAD_GL_FUNCTION(glBindTexture)
    LOAD_GL_FUNCTION(glBlendFunc)
    LOAD_GL_FUNCTION(glClear)
    LOAD_GL_FUNCTION(glClearColor)
    LOAD_GL_FUNCTION(glClearDepth)
    LOAD_GL_FUNCTION(glDepthFunc)
    LOAD_GL_FUNCTION(glDepthMask)
    LOAD_GL_FUNCTION(glDrawArrays)
    LOAD_GL_FUNCTION(glDrawBuffer)
    LOAD_GL_FUNCTION(glEnable)
    LOAD_GL_FUNCTION(glGenTextures)
    LOAD_GL_FUNCTION(glGetError)
    LOAD_GL_FUNCTION(glPixelStorei)
    LOAD_GL_FUNCTION(glTexImage2D)
    LOAD_GL_FUNCTION(glTexParameteri)
    LOAD_GL_FUNCTION(glViewport)
    LOAD_GL_FUNCTION(glAttachShader)
    LOAD_GL_FUNCTION(glBindBuffer)
    LOAD_GL_FUNCTION(glBindVertexArray)
    LOAD_GL_FUNCTION(glBufferData)
    LOAD_GL_FUNCTION(glCompileShader)
    LOAD_GL_FUNCTION(glCreateProgram)
    LOAD_GL_FUNCTION(glCreateShader)
    LOAD_GL_FUNCTION(glDeleteShader)
    LOAD_GL_FUNCTION(glEnableVertexAttribArray)
    LOAD_GL_FUNCTION(glGenBuffers)
    LOAD_GL_FUNCTION(glGenVertexArrays)
    LOAD_GL_FUNCTION(glGetProgramiv)
    LOAD_GL_FUNCTION(glGetShaderInfoLog)
    LOAD_GL_FUNCTION(glGetShaderiv)
    LOAD_GL_FUNCTION(glLinkProgram)
    LOAD_GL_FUNCTION(glShaderSource)
    LOAD_GL_FUNCTION(glUniform4f)
    LOAD_GL_FUNCTION(glUniformMatrix4fv)
    LOAD_GL_FUNCTION(glUseProgram)
    LOAD_GL_FUNCTION(glVertexAttribPointer)
    LOAD_GL_FUNCTION(glGetUniformLocation)

    return GL_TRUE;
}

void closeDynamicGL(void)
{
    dlclose(dGL.handle);
}
