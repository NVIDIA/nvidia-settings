/*
 * Copyright (c) 2013 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv-control-warpblend.h"

typedef struct __attribute__((packed)) {
    float x, y;
} vertex2f;

typedef struct __attribute__((packed)) {
    vertex2f pos;
    vertex2f tex;
    vertex2f tex2;
} vertexDataRec;

static inline float transformPoint(vertex2f *vec)
{
    float w, oneOverW;
    float x_in, y_in;

    // Sample projection matrix generated from a trapezoid projection
    static const float mat[3][3] =
    {
        { 0.153978257544863,-0.097906833257365,0.19921875 },
        { -0.227317623368679,0.222788944798964,0.25 },
        { -0.585236541598693,-0.135471643796181,1 }
    };

    x_in = vec->x;
    y_in = vec->y;

    vec->x = x_in * mat[0][0] + y_in * mat[0][1] + mat[0][2];
    vec->y = x_in * mat[1][0] + y_in * mat[1][1] + mat[1][2];
    w      = x_in * mat[2][0] + y_in * mat[2][1] + mat[2][2];

    oneOverW = 1.0 / w;

    vec->x *= oneOverW;
    vec->y *= oneOverW;

    return oneOverW;
}

int main(int ac, char **av)
{
    Display *xDpy = XOpenDisplay(NULL);
    int screenId;
    GC gc;
    XGCValues values;
    Pixmap blendPixmap;
    vertexDataRec warpData[6];
    int nvDpyId;

    if (!xDpy) {
        fprintf (stderr, "Could not open X Display %s!\n", XDisplayName(NULL));
        return 1;
    }

    screenId = XDefaultScreen(xDpy);

    if (ac != 2) {
        fprintf (stderr, "Usage: ./nv-control-warpblend nvDpyId\n");
        fprintf (stderr, "See 'nvidia-settings -q CurrentMetaMode' for currently connected DPYs.\n");
        return 1;
    }

    nvDpyId = atoi(av[1]);

    // Start with two screen-aligned triangles, and warp them using the sample
    // keystone matrix in transformPoint. Make sure we save W for correct
    // perspective and pass it through as the last texture coordinate component.
    warpData[0].pos.x = 0.0f;
    warpData[0].pos.y = 0.0f;
    warpData[0].tex.x = 0.0f;
    warpData[0].tex.y = 0.0f;
    warpData[0].tex2.x = 0.0f;
    warpData[0].tex2.y = transformPoint(&warpData[0].pos);

    warpData[1].pos.x = 1.0f;
    warpData[1].pos.y = 0.0f;
    warpData[1].tex.x = 1.0f;
    warpData[1].tex.y = 0.0f;
    warpData[1].tex2.x = 0.0f;
    warpData[1].tex2.y = transformPoint(&warpData[1].pos);

    warpData[2].pos.x = 0.0f;
    warpData[2].pos.y = 1.0f;
    warpData[2].tex.x = 0.0f;
    warpData[2].tex.y = 1.0f;
    warpData[2].tex2.x = 0.0f;
    warpData[2].tex2.y = transformPoint(&warpData[2].pos);

    warpData[3].pos.x = 1.0f;
    warpData[3].pos.y = 0.0f;
    warpData[3].tex.x = 1.0f;
    warpData[3].tex.y = 0.0f;
    warpData[3].tex2.x = 0.0f;
    warpData[3].tex2.y = transformPoint(&warpData[3].pos);

    warpData[4].pos.x = 1.0f;
    warpData[4].pos.y = 1.0f;
    warpData[4].tex.x = 1.0f;
    warpData[4].tex.y = 1.0f;
    warpData[4].tex2.x = 0.0f;
    warpData[4].tex2.y = transformPoint(&warpData[4].pos);

    warpData[5].pos.x = 0.0f;
    warpData[5].pos.y = 1.0f;
    warpData[5].tex.x = 0.0f;
    warpData[5].tex.y = 1.0f;
    warpData[5].tex2.x = 0.0f;
    warpData[5].tex2.y = transformPoint(&warpData[5].pos);

    // Prime the random number generator, since the helper functions need it.
    srand(time(NULL));

    // Apply our transformed warp data to the chosen display.
    XNVCTRLSetScanoutWarping(xDpy,
                             screenId,
                             nvDpyId,
                             NV_CTRL_WARP_DATA_TYPE_MESH_TRIANGLES_XYUVRQ,
                             6, // 6 vertices for two triangles
                             (float *)warpData);

    // Create a sample blending pixmap; let's make it solid white with a grey
    // border and rely on upscaling with filtering to feather the edges.

    // Start with a 32x32 pixmap.
    blendPixmap = XCreatePixmap(xDpy, RootWindow(xDpy, screenId), 32, 32, DefaultDepth(xDpy, screenId));

    values.foreground = 0x77777777;
    gc = XCreateGC(xDpy, blendPixmap, GCForeground, &values);

    // Fill it fully with grey.
    XFillRectangle(xDpy, blendPixmap, gc, 0, 0, 32, 32);

    values.foreground = 0xffffffff;
    XChangeGC(xDpy, gc, GCForeground, &values);

    // Fill everything but a one-pixel border with white.
    XFillRectangle(xDpy, blendPixmap, gc, 1, 1, 30, 30);

    // Apply it to the display. blendAfterWarp is FALSE, so the edges will be
    // blended in warped space.
    XNVCTRLSetScanoutIntensity(xDpy,
                               screenId,
                               nvDpyId,
                               blendPixmap,
                               False);

    return 0;
}
