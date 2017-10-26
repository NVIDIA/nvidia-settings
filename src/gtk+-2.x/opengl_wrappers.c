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

#include "ctkutils.h"

#ifdef CTK_GTK3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opengl_loading.h"
#include "opengl_wrappers.h"
#include "matrix_utils.h"

GLchar const * const cubeVertexShaderSource[] =
{
    "#version 450 core\n"
    "in  vec3 xyz;\n"
    "in  vec2 uv;\n"
    "out vec2 uvFromV;\n"
    "uniform mat4 mvp;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position =  vec4(xyz, 1) * mvp;\n"
    "    uvFromV = uv;\n"
    "}\n"
};

GLchar const * const cubeFragmentShaderSource[] =
{
    "#version 450 core\n"
    "out vec4 color;\n"
    "in vec2 uvFromV;\n"
    "uniform sampler2D textureSampler;\n"
    "void main(void)\n"
    "{\n"
    "    color = texture(textureSampler, uvFromV);\n"
    "    if (color.a == 0) {\n"
    "        color = vec4(0, 0, 0, 1);\n"
    "    }\n"
    "}\n"
};

GLchar const * const  textVertexShaderSource[] =
{
    "#version 450 core\n"
    "in  vec3 xyz;\n"
    "in  vec2 uv;\n"
    "out vec2 uvFromV;\n"
    "uniform vec4 textColor;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position =  vec4(xyz, 1);\n"
    "    uvFromV = uv;\n"
    "}\n"
};

GLchar const * const textFragmentShaderSource[] =
{
    "#version 450 core\n"
    "out vec4 color;\n"
    "in vec2 uvFromV;\n"
    "uniform sampler2D textureSampler;\n"
    "uniform vec4 textColor;\n"
    "void main(void)\n"
    "{\n"
    "    color = texture(textureSampler, uvFromV);\n"
    "    color.rgb = textColor.rgb;\n"
    "}\n"
};

void freeOglmd(OpenGLModelData *p)
{
    if (p) {
            free(p->uniforms);
            p->uniforms = NULL;
            free(p->mvp);
            p->mvp = NULL;
    }
    free(p);
    p = NULL;
}

static GLboolean verifyShaderCompilation(GLuint shader)
{
    GLint status;
    char compilationLog[2048];

    dGL.glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        dGL.glGetShaderInfoLog(shader, 2000, NULL, compilationLog);
        fprintf(stderr, "Shader compilation failed for shaderID=%i\n", shader);
        fprintf(stderr, "Error message:\n");
        fprintf(stderr, "%s\n", compilationLog);
        return GL_FALSE;
    }
    return GL_TRUE;
}

static GLuint makeProgram(char const * const * vertexShaderSource,
                          char const * const * fragmentShaderSource)
{
    GLuint program, vertexShader, fragmentShader;
    GLint status;

    vertexShader = dGL.glCreateShader(GL_VERTEX_SHADER);
    dGL.glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
    dGL.glCompileShader(vertexShader);
    if (!verifyShaderCompilation(vertexShader)) {
        return 0;
    }

    fragmentShader = dGL.glCreateShader(GL_FRAGMENT_SHADER);
    dGL.glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
    dGL.glCompileShader(fragmentShader);
    if (!verifyShaderCompilation(fragmentShader)) {
        return 0;
    }

    program = dGL.glCreateProgram();
    dGL.glAttachShader(program, vertexShader);
    dGL.glAttachShader(program, fragmentShader);

    dGL.glDeleteShader(vertexShader);
    dGL.glDeleteShader(fragmentShader);

    dGL.glLinkProgram(program);

    dGL.glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        fprintf(stderr, "Link failed for programID=%i\n",program);
        return 0;
    }
    return program;
}

void drawModel(OpenGLModelData *data)
{
    int i;

    dGL.glBindVertexArray(data->vao);
    dGL.glBindTexture(GL_TEXTURE_2D, data->tex);
    dGL.glUseProgram(data->program);

    for (i = 0; i < data->uniformCount; i++) {
        dGL.glUniform4f(data->uniforms[i].index,
                        data->uniforms[i].data[0],
                        data->uniforms[i].data[1],
                        data->uniforms[i].data[2],
                        data->uniforms[i].data[3]);
    }

    if (data->mvp != NULL) {
        // This functions expects the mvp to be row-major, openGL internally
        // uses column major, so we set the transpose parameter to GL_TRUE
        dGL.glUniformMatrix4fv(dGL.glGetUniformLocation(data->program, "mvp"),
                               1, GL_TRUE, data->mvp);
    }

    dGL.glDrawArrays(GL_TRIANGLES, 0, data->vboLen / 3);
}

static GLuint drawModelSetup(float *modelData,
                             unsigned int modelDataLen,
                             float *textureCoordinates,
                             unsigned int textureCoordinatesLen)
{
    GLuint vao;
    GLuint vertexBuffer, uvBuffer;

    dGL.glGenVertexArrays(1, &vao);
    dGL.glBindVertexArray(vao);

    dGL.glGenBuffers(1, &vertexBuffer);
    dGL.glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    dGL.glBufferData(GL_ARRAY_BUFFER, modelDataLen * sizeof(float),
                     modelData, GL_STATIC_DRAW);
    dGL.glEnableVertexAttribArray(0);
    dGL.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    dGL.glGenBuffers(1, &uvBuffer);
    dGL.glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
    dGL.glBufferData(GL_ARRAY_BUFFER, textureCoordinatesLen * sizeof(float),
                     textureCoordinates, GL_STATIC_DRAW);
    dGL.glEnableVertexAttribArray(1);
    dGL.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    return vao;
}

static GLuint textureSetup(int textureWidth, int textureHeight,
                           void *textureDataRaw32)
{
    GLuint texName;
    dGL.glGenTextures(1, &texName);
    dGL.glBindTexture(GL_TEXTURE_2D, texName);
    dGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    dGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    dGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dGL.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    dGL.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     textureWidth, textureHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, textureDataRaw32);

    return texName;
}

OpenGLModelData *cubeSetup(GdkPixbuf *image)
{
    OpenGLModelData *cube;

    float d = 1;
    int tmp;

    float modelData[] =  {
        -d / 2,  d / 2, -d / 2, // 1
         d / 2,  d / 2, -d / 2, // 2
        -d / 2, -d / 2, -d / 2, // 3
         d / 2, -d / 2, -d / 2, // 4
        -d / 2, -d / 2, -d / 2, // 5
         d / 2,  d / 2, -d / 2, // 6

        -d / 2,  d / 2,  d / 2, // 7
        -d / 2,  d / 2, -d / 2, // 8
        -d / 2, -d / 2,  d / 2, // 9
        -d / 2, -d / 2, -d / 2, // 10
        -d / 2, -d / 2,  d / 2, // 11
        -d / 2,  d / 2, -d / 2, // 12

        -d / 2,  d / 2,  d / 2, // 13
         d / 2,  d / 2,  d / 2, // 14
        -d / 2,  d / 2, -d / 2, // 15
         d / 2,  d / 2, -d / 2, // 16
        -d / 2,  d / 2, -d / 2, // 17
         d / 2,  d / 2,  d / 2, // 18

         d / 2,  d / 2, -d / 2, // 19
         d / 2,  d / 2,  d / 2, // 20
         d / 2, -d / 2, -d / 2, // 21
         d / 2, -d / 2,  d / 2, // 22
         d / 2, -d / 2, -d / 2, // 23
         d / 2,  d / 2,  d / 2, // 24

        -d / 2, -d / 2, -d / 2, // 25
         d / 2, -d / 2, -d / 2, // 26
        -d / 2, -d / 2,  d / 2, // 27
         d / 2, -d / 2,  d / 2, // 28
        -d / 2, -d / 2,  d / 2, // 29
         d / 2, -d / 2, -d / 2, // 30

         d / 2,  d / 2,  d / 2, // 31
        -d / 2,  d / 2,  d / 2, // 32
         d / 2, -d / 2,  d / 2, // 33
        -d / 2, -d / 2,  d / 2, // 34
         d / 2, -d / 2,  d / 2, // 35
        -d / 2,  d / 2,  d / 2, // 36

    };

    float textureCoordinates[] = {
        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,

        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,

        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,

        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,

        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,

        0, 1,
        1, 1,
        0, 0,
        1, 0,
        0, 0,
        1, 1,
    };

    tmp = makeProgram(cubeVertexShaderSource, cubeFragmentShaderSource);

    if (tmp == 0) {
        return NULL;
    }

    cube = malloc(sizeof(OpenGLModelData));
    memset(cube, 0, sizeof(OpenGLModelData));

    cube->program = (GLuint) tmp;

    // 6 sides in a cube
    // 2 triangles per side
    // 3 vertices per triangle
    // 3 vertex  coordinates per vertex
    // 2 texture coordinates per vertex
    cube->vboLen = 6 * 2 * 3 * 3;
    cube->vao = drawModelSetup(modelData, cube->vboLen,
                               textureCoordinates, cube->vboLen / 3 * 2);

    cube->tex  = textureSetup(gdk_pixbuf_get_width(image),
                              gdk_pixbuf_get_height(image),
                              gdk_pixbuf_get_pixels(image));

    cube->mvp = malloc(16 * sizeof(float));
    genIdentityMatrix(cube->mvp);

    return cube;
}

OpenGLModelData *labelSetup(float x, float y,
                            float width, float height,
                            float red, float green, float blue,
                            GdkPixbuf *image)
{
    OpenGLModelData *label;
    int tmp;

    float modelData[] =  {
        -width/2 + x,  height/2 + y, 0, // 1
         width/2 + x,  height/2 + y, 0, // 2
        -width/2 + x, -height/2 + y, 0, // 3
         width/2 + x, -height/2 + y, 0, // 4
        -width/2 + x, -height/2 + y, 0, // 5
         width/2 + x,  height/2 + y, 0, // 6
    };

    float textureCoordinates[] = {
        0, 0,
        1, 0,
        0, 1,
        1, 1,
        0, 1,
        1, 0,
    };

    tmp = makeProgram(textVertexShaderSource, textFragmentShaderSource);
    if (tmp == 0) {
        return NULL;
    }

    label = malloc(sizeof(OpenGLModelData));
    memset(label, 0, sizeof(OpenGLModelData));

    label->program = (GLuint) tmp;

    // 2 triangles
    // 3 vertices per triangle
    // 3 vertex coordinates per vertex
    // 2 texture coordinates per vertex
    label->vboLen = 2 * 3 * 3;
    label->vao = drawModelSetup(modelData, label->vboLen,
                 textureCoordinates, label->vboLen/3 * 2);

    label->tex = textureSetup(gdk_pixbuf_get_width(image),
                              gdk_pixbuf_get_height(image),
                              gdk_pixbuf_get_pixels(image));

    label->uniformCount = 1;
    label->uniforms = malloc(label->uniformCount * sizeof(uniformVec4f));
    label->uniforms[0].index = dGL.glGetUniformLocation(label->program, "textColor");
    label->uniforms[0].data[0] = red;
    label->uniforms[0].data[1] = green;
    label->uniforms[0].data[2] = blue;
    label->uniforms[0].data[3] = 0;

    return label;
}

#endif
