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
#include "ctkglstereo.h"

#ifdef CTK_GTK3
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "right.png.h"
#include "left.png.h"
#include "nvlogo.png.h"

#include "matrix_utils.h"
#include "opengl_loading.h"
#include "ctkglwidget.h"
#endif

#ifdef CTK_GTK3
const unsigned int animateDiv = 1000;
static void animate(StereoAppData *appData, int eye)
{
    float x;
    int op;
    float delta = 0.2;

    GLfloat tm[16];  // Temporary matrix
    GLfloat *mvp;    // Model-View-Projection matrix
    mvp = appData->cube->mvp;

    if (eye == GL_LEFT) {
        op = 1;
    } else {
        op = -1;
    }

    x = 2 * M_PI / animateDiv * appData->animationCounter;

    genIdentityMatrix(mvp);

    genRotateMatrixX(M_PI / 4, tm);
    matrixMult(mvp, tm);

    genRotateMatrixZ(M_PI / 4, tm);
    matrixMult(mvp, tm);

    // Apply the calculated rotation for this
    // frame along axis X and Y to generate the
    // animated effect
    genRotateMatrixX(x, tm);
    matrixMult(mvp, tm);
    genRotateMatrixY(x, tm);
    matrixMult(mvp, tm);

    // Translates depending on eye
    // and locates the object away from zero in the z-axis so
    // that is viewable in the fov we define in the next step.
    genTranslateMatrix(op * delta, 0, -1.5, tm);
    matrixMult(mvp, tm);

    genPerspectiveMatrix(M_PI / 2 , 1, 0.5, 5, tm);
    matrixMult(mvp, tm);
}

static void produceFrameStereoTest(void *_appData)
{
    StereoAppData *appData = _appData;

    dGL.glDrawBuffer(GL_BACK_LEFT);

    dGL.glClearColor(206.0/255, 206.0/255, 206.0/255, 1);
    dGL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    animate(appData, GL_LEFT);
    drawModel(appData->cube);
    drawModel(appData->labelLeft);

    dGL.glDrawBuffer(GL_BACK_RIGHT);
    dGL.glClearColor(206.0/255, 206.0/255, 206.0/255, 1);
    dGL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    animate(appData, GL_RIGHT);
    drawModel(appData->cube);
    drawModel(appData->labelRight);

    appData->animationCounter++;
    appData->animationCounter %= animateDiv;
}

static int verifyOpenglForStereo(void)
{
    GLint majorVer = 0;

    dGL.glGetIntegerv(GL_MAJOR_VERSION, &majorVer);

    if (majorVer < 3) {
        return -1;
    }
    return 0;
}

static int setupStereoTest(void *_appData)
{
    StereoAppData *appData = _appData;

    GdkPixbuf *nvidiaLogo = CTK_LOAD_PIXBUF(nvlogo);
    GdkPixbuf *imageLeft  = CTK_LOAD_PIXBUF(left);
    GdkPixbuf *imageRight = CTK_LOAD_PIXBUF(right);

    if (verifyOpenglForStereo() != 0) {
        return -2;
    }

    dGL.glViewport(0, 0, 200, 200);

    dGL.glEnable(GL_DEPTH_TEST);

    dGL.glEnable(GL_BLEND);
    dGL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    dGL.glEnable(GL_TEXTURE_2D);
    dGL.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    appData->animationCounter = 0;

    appData->cube = cubeSetup(nvidiaLogo);

    appData->labelLeft = labelSetup(-0.75, 0.90,      // x and y
                                     0.5,  0.20,      // width and height
                                       1, 0, 0,       // rgb
                                     imageLeft        // GdkPixbuf *
                                   );

    appData->labelRight = labelSetup(0.70, 0.85,      // x and y
                                     0.6,  0.20,      // width and height
                                       0, 0, 1,       // rgb
                                     imageRight       // GdkPixbuf *
                                    );

    g_object_unref(nvidiaLogo);
    g_object_unref(imageLeft);
    g_object_unref(imageRight);

    if (appData->cube == NULL ||
        appData->labelLeft == NULL ||
        appData->labelRight == NULL) {

        return -1;
    }

    return 0;
}

#endif

GtkWidget *ctk_glstereo_new(void)
{
#ifdef CTK_GTK3
    GtkWidget *gtk_widget;
    CtkGLWidget *ctk_glwidget;
    int glx_attributes[] = {
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_DOUBLEBUFFER,  1,
        GLX_RED_SIZE,      1,
        GLX_GREEN_SIZE,    1,
        GLX_BLUE_SIZE,     1,
        GLX_ALPHA_SIZE,    1,
        GLX_DEPTH_SIZE,    1,
        GLX_STEREO,        1,
        None
    };

    void *app_data;
    app_data = malloc(sizeof(StereoAppData));
    gtk_widget = ctk_glwidget_new(glx_attributes,
                                  app_data,
                                  setupStereoTest,
                                  produceFrameStereoTest);

    ctk_glwidget = CTK_GLWIDGET(gtk_widget);

    if (ctk_glwidget) {
        ctk_glwidget->timer_interval = 10;  // In milliseconds
        gtk_widget_set_size_request(GTK_WIDGET(ctk_glwidget), 200, 200);
        return GTK_WIDGET(ctk_glwidget);
    }

    free(app_data);

#endif
    return NULL;
}

