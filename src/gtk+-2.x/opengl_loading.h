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

#ifndef __OPENGL_LOADING_H__
#define __OPENGL_LOADING_H__

#include <GL/gl.h>
#include <GL/glx.h>

typedef struct _libGLData libGLData;
struct _libGLData {

    void *handle;

    const GLubyte *(*glGetString)   (GLenum);
    const GLubyte *(*glGetStringi)  (GLenum, GLuint);
    void           (*glGetIntegerv) (GLenum, GLint *);

    // GLX function pointers
    GLXContext     (*glXCreateNewContext)       (Display *, GLXFBConfig, int, GLXContext,Bool);
    void           (*glXDestroyContext)         (Display *, GLXContext);
    Bool           (*glXMakeContextCurrent)     (Display *, GLXDrawable, GLXDrawable, GLXContext);
    void           (*glXSwapBuffers)            (Display *, GLXDrawable);
    GLXFBConfig   *(*glXChooseFBConfig)         (Display *, int, const int * , int *);
    int            (*glXGetFBConfigAttrib)      (Display *, GLXFBConfig, int, int *);
    void          *(*glXGetProcAddress)         (const GLubyte *);
    GLXWindow      (*glXCreateWindow)           (Display *, GLXFBConfig, Window win, const int *);
    XVisualInfo*   (*glXGetVisualFromFBConfig)  (Display *, GLXFBConfig);

    void           (*glBindTexture)             (GLenum, GLuint);
    void           (*glBlendFunc)               (GLenum, GLenum);
    void           (*glClear)                   (GLbitfield);
    void           (*glClearColor)              (GLclampf, GLclampf, GLclampf, GLclampf);
    void           (*glClearDepth)              (GLdouble);
    void           (*glDepthFunc)               (GLenum);
    void           (*glDepthMask)               (GLboolean);
    void           (*glDrawArrays)              (GLenum, GLint, GLsizei);
    void           (*glDrawBuffer)              (GLenum);
    void           (*glEnable)                  (GLenum);
    void           (*glGenTextures)             (GLsizei, GLuint *);
    GLenum         (*glGetError)                (void);
    void           (*glPixelStorei)             (GLenum, GLint);
    void           (*glTexImage2D)              (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    void           (*glTexParameteri)           (GLenum, GLenum, GLint);
    void           (*glViewport)                (GLint, GLint, GLsizei, GLsizei);
    void           (*glAttachShader)            (GLuint, GLuint);
    void           (*glBindBuffer)              (GLenum, GLuint);
    void           (*glBindVertexArray)         (GLuint);
    void           (*glBufferData)              (GLenum, GLsizeiptr, const GLvoid *, GLenum);
    void           (*glCompileShader)           (GLuint);
    GLuint         (*glCreateProgram)           (void);
    GLuint         (*glCreateShader)            (GLenum);
    void           (*glDeleteShader)            (GLuint);
    void           (*glEnableVertexAttribArray) (GLuint);
    void           (*glGenBuffers)              (GLsizei, GLuint *);
    void           (*glGenVertexArrays)         (GLsizei, GLuint *);
    void           (*glGetProgramiv)            (GLuint, GLenum, GLint *);
    void           (*glGetShaderInfoLog)        (GLuint, GLsizei, GLsizei *, GLchar *);
    void           (*glGetShaderiv)             (GLuint, GLenum, GLint *);
    void           (*glLinkProgram)             (GLuint);
    void           (*glShaderSource)            (GLuint, GLsizei, const GLchar* const *, const GLint *);
    void           (*glUniform4f)               (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
    void           (*glUniformMatrix4fv)        (GLint, GLsizei, GLboolean, const GLfloat *);
    void           (*glUseProgram)              (GLuint);
    void           (*glVertexAttribPointer)     (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
    GLint          (*glGetUniformLocation)      (GLuint, const GLchar *);

};

extern libGLData dGL;
void closeDynamicGL(void);
GLboolean loadGL(void);

#endif /* __OPENGL_LOADING_H__ */
