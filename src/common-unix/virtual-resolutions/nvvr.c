/*
 * Copyright (C) 2012-2013 NVIDIA Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>

#include "nvvr.h"

/*!
 * Compute ViewPortOut, given raster size, ViewPortIn size, and scaling type.
 *
 * ViewPortOut should fit within the raster size, scaled to the raster
 * size in one dimension, and scaled in the other dimension such that
 * the aspect ratio of ViewPortIn is preserved.
 *
 * \param[in] raster      raster size
 * \param[in] viewPortIn  ViewPortIn size
 * \param[in] scaled      scaling type
 *
 * \return  The ViewPortOut {x,y,w,h}
 */
NVVRBoxRecXYWH NVVRGetScaledViewPortOut(const NVVRSize *raster,
                                        const NVVRSize *viewPortIn,
                                        const NVVRScalingType scaling)
{
    NVVRBoxRecXYWH viewPortOut;
    float scaleX, scaleY;

    memset(&viewPortOut, 0, sizeof(viewPortOut));

    switch (scaling) {
        default:
            /* fall through */

        case NVVR_SCALING_ASPECT_SCALED:
            scaleX = (float) raster->w / (float) viewPortIn->w;
            scaleY = (float) raster->h / (float) viewPortIn->h;

            if (scaleX < scaleY) {
                viewPortOut.w = raster->w;
                viewPortOut.h = viewPortIn->h * scaleX;
                viewPortOut.x = 0;
                viewPortOut.y = (raster->h - viewPortOut.h) / 2;
            } else {
                viewPortOut.w = viewPortIn->w * scaleY;
                viewPortOut.h = raster->h;
                viewPortOut.x = (raster->w - viewPortOut.w) / 2;
                viewPortOut.y = 0;
            }
            break;

        case NVVR_SCALING_SCALED:
            viewPortOut.x = 0;
            viewPortOut.y = 0;
            viewPortOut.w = raster->w;
            viewPortOut.h = raster->h;
            break;

        case NVVR_SCALING_CENTERED:
            /* if raster is smaller than viewPortIn, fall back to scaling */
            if (raster->w >= viewPortIn->w) {
                viewPortOut.w = viewPortIn->w;
                viewPortOut.x = (raster->w - viewPortIn->w) / 2;
            } else {
                viewPortOut.w = raster->w;
                viewPortOut.x = 0;
            }

            if (raster->h >= viewPortIn->h) {
                viewPortOut.h = viewPortIn->h;
                viewPortOut.y = (raster->h - viewPortIn->h) / 2;
            } else {
                viewPortOut.h = raster->h;
                viewPortOut.y = 0;
            }
            break;
    }

    return viewPortOut;
}

/*!
 * Return a constant array of a (-1, -1) terminated list of common
 * resolutions.
 *
 * \return A constant array of NVVRSize.
 */
const NVVRSize* NVVRGetCommonResolutions()
{
    static const NVVRSize commonRes[] = {
        { 3840, 2400 },
        { 2560, 1600 },
        { 2560, 1440 },
        { 1920, 1200 },
        { 1920, 1080 },
        { 1680, 1050 },
        { 1600, 1200 },
        { 1440, 900  },
        { 1366, 768  },
        { 1280, 1024 },
        { 1280, 800  },
        { 1280, 720  },
        { 1024, 768  },
        { 800,  600  },
        { 640,  480  },
        { -1,   -1   },
    };

    return commonRes;
}
