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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "NVCtrl.h"
#include "NVCtrlLib.h"

/*
 * The scanout composition pipeline provides infrastructure to:
 *   - Individually transform the output of each display device using a user-
 *     provided warping mesh, with perspective correction.
 *   - Perform per-pixel intensity and black level adjustment from two separate
 *     user-provided textures. This can be configured to apply before (desktop-
 *     space) or after (display-space) warping by setting the BlendOrder token
 *     to BlendAfterWarp or WarpAfterBlend.
 *
 * The composition equation is:
 *   Output = Input * blendTexture * (1 − offsetTexture) + offsetTexture
 *
 * The above functionality is exposed through binding Pixmaps to names through
 * the XNVCTRLBindWarpPixmapName NV-CONTROL functionality, and passing these
 * bound names to the "WarpMesh", "BlendTexture" and "OffsetTexture" attributes
 * of the desired display in a MetaMode.
 *
 * The texture coordinates of the warping mesh indicate where to source from
 * the desktop in normalized ViewPortIn space, meaning that 0,0 and 1,1 map to
 * boundaries of the area that would otherwise be displayed if warping was
 * disabled. Coordinates outside these boundaries are accepted.
 *
 * Likewise, the mesh coordinates are in normalized ViewPortOut space, 0,0 and
 * 1,1 mapping to the boundaries of the visible region on the display device.
 *
 * Please also see the XNVCTRLBindWarpPixmapName documentation in NVCtrlLib.h.
 *
 * The below three wrappers are immediate interfaces to the same functionality.
 * These functions will create Pixmaps to encapsulate the data provided to them,
 * and leave them bound to the names generated from them, causing them to remain
 * allocated for the lifetime of the server. This makes them ill-suited for use
 * cases where the warp mesh data has to vary dynamically; accessing the
 * XNVCTRLBindWarpPixmapName functionality directly is recommended in that case.
 *
 * Please make sure the standard random number generator is seeded with a call
 * to srand() before using them.
 *
 * Refer to nv-control-warpblend.c for sample usage cases.
 *
 */

/*
 * XNVCTRLSetScanoutWarping
 *
 * xDpy:         valid X display connection
 * screenId:     X protocol screen number
 * nvDpyId:      NV-CONTROL display target index; can be enumerated with the
 *               NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN request.
 * warpDatatype: NV_CTRL_WARP_DATA_TYPE_MESH_TRIANGLESTRIP_XYUVRQ or
 *               NV_CTRL_WARP_DATA_TYPE_MESH_TRIANGLES_XYUVRQ.
 * vertexCount:  number of vertices in the passed data; must be a multiple of 3
 *               if warpDataType is NV_CTRL_WARP_DATA_TYPE_MESH_TRIANGLES_XYUVRQ.
 * warpData:     array of floating-point values representing the warping mesh,
 *               with six components per vertice:
 *                - X, Y: position in normalized ViewPortOut space.
 *                - U, V: texture coordinate in normalized ViewPortIn space.
 *                - R:    unused.
 *                - Q:    perspective component for the position.
 *               If warpData is NULL, the WarpMesh attribute will be removed for
 *               the designated display device.
 */
static inline int
XNVCTRLSetScanoutWarping(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    int warpDataType,
    int vertexCount,
    const float *warpData);

/*
 * XNVCTRLSetScanout[Intensity/Offset]
 *
 * xDpy:     valid X display connection
 * screenId: X protocol screen number
 * nvDpyId:  NV-CONTROL display target index; can be enumerated with the
 *           NV_CTRL_BINARY_DATA_DISPLAYS_ENABLED_ON_XSCREEN request.
 * pixmap:   XID naming a valid Pixmap to be used as Intensity/Offset data.
 *           The pixmap does not have any size restrictions and will be scaled
 *           to fit the ViewPortIn of the target display with filtering.
 *           If the pixmap has a depth of 8, it will be treated as a single
 *           color component replicated across all channels. 
 * blendAfterWarp: if True, sets BlendOrder to BlendAfterWarp to apply the
 *                 composition in display-space; otherwise, it is applied in
 *                 desktop-space before any warping.
 */
static inline int
XNVCTRLSetScanoutIntensity(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    Pixmap intensityPixmap,
    Bool blendAfterWarp);

static inline int
XNVCTRLSetScanoutOffset(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    Pixmap offsetPixmap,
    Bool blendAfterWarp);

static inline int
RemoveAttributeFromDisplayOfCurrentMetaMode(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    const char *attribute)
{
    char *pOldCurrentMetaMode = NULL;
    char *pCurrentMetaMode = NULL;
    char displayName[512];
    char *pDisplay;
    char *pNextDisplay;
    char *pAttributeList;
    char *pTargetAttribute;
    char *newMetaMode = NULL;
    int error = 1;
    int attributeLen;
    Bool ret;

    if (!attribute) {
        goto cleanup;
    }

    attributeLen = strlen(attribute);

    ret = XNVCTRLQueryStringAttribute(xDpy,
                                      screenId,
                                      0, // displayMask
                                      NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                                      &pOldCurrentMetaMode);

    if (!ret || !pOldCurrentMetaMode) {
        goto cleanup;
    }

    // Discard the metadata from the beginning of the MetaMode.
    pCurrentMetaMode = strstr(pOldCurrentMetaMode, "::");
    pCurrentMetaMode += 2;

    // Keep a settable copy of this, to which we'll mirror the changes we want.
    newMetaMode = strdup(pCurrentMetaMode);

    if (!newMetaMode) {
        goto cleanup;
    }

    sprintf(displayName, "DPY-%i", nvDpyId);

    pDisplay = strstr(pCurrentMetaMode, displayName);

    if (!pDisplay) {
        // Requested Dpy not found in this MetaMode.
        goto cleanup;
    }

    // If there's another DPY after the one we want, finish the string there to
    // limit the scope of our search.
    pNextDisplay = strstr(pDisplay + 1, "DPY-");

    if (pNextDisplay) {
        *pNextDisplay = '\0';
    }

    // If it has an attribute list, start looking from there; if not, fail.
    if (!(pAttributeList = strstr(pDisplay, "{"))) {
        goto cleanup;
    }

    pTargetAttribute = strstr(pAttributeList, attribute);

    if (!pTargetAttribute) {
        goto cleanup;
    }

    // Found it; use the offset we found against our good copy of the mode to
    // mangle the attribute name.
    while (attributeLen--) {
        newMetaMode[pTargetAttribute - pCurrentMetaMode + attributeLen] = 'z';
    }

    XNVCTRLSetStringAttribute(xDpy,
                              screenId,
                              0, // displayMask
                              NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                              newMetaMode);
    XSync(xDpy, 0);

    error = 0;

cleanup:

    if (newMetaMode) {
        free(newMetaMode);
    }

    if (pOldCurrentMetaMode) {
        free(pOldCurrentMetaMode);
    }

    return error;
}

// Given a comma-separated, list of A=B tokens this helper appends them to the
// to the attribute list of the currently-set MetaMode.
static inline int
AddAttributesToDisplayOfCurrentMetaMode(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    const char *attributes)
{
    char *pOldCurrentMetaMode = NULL;
    char *pCurrentMetaMode = NULL;
    char displayName[512];
    char *pDisplay;
    char *newMetaMode = NULL;
    int error = 1;

    XNVCTRLQueryStringAttribute(xDpy,
                                screenId,
                                0, // displayMask
                                NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                                &pOldCurrentMetaMode);

    // Discard the metadata from the beginning of the MetaMode.
    pCurrentMetaMode = strstr(pOldCurrentMetaMode, "::");
    pCurrentMetaMode += 2;

    sprintf(displayName, "DPY-%i", nvDpyId);

    pDisplay = strstr(pCurrentMetaMode, displayName);

    if (pDisplay) {
        int foundBeginAttr = 0;
        char newAttribute[256];
        int endOfString = 1;
        int neededLength;

        while (*(++pDisplay)) {
            if (*pDisplay == '{') {
                foundBeginAttr = 1;
            }
            if (*pDisplay == '}') {
                endOfString = 0;
                break;
            }

            if (*pDisplay == ',' && !foundBeginAttr) {
                endOfString = 0;
                break;
            }
        }

        if (!endOfString) {
            *pDisplay = 0;
            pDisplay++;
        }

        if (endOfString) {
            // no attributes, no mode after
            sprintf(newAttribute, " {%s}", attributes);
        }
        if (foundBeginAttr) {
            // pDisplay had to be at the }, append an attribute and add the } back
            sprintf(newAttribute, ", %s}", attributes);
        } else {
            // No attributes for this Dpy, we need to add them and restore the ,
            sprintf(newAttribute, " {%s},", attributes);
        }

        neededLength = strlen(pCurrentMetaMode) +
                       strlen(newAttribute) +
                       strlen(pDisplay);

        // Add potential null-terminators for each of the three components.
        newMetaMode = malloc(neededLength + 3);

        if (!newMetaMode) {
            goto cleanup;
        }

        // Put it back together with the new stuff in the middle
        sprintf(newMetaMode, "%s%s%s",
                pCurrentMetaMode,
                newAttribute,
                pDisplay);
    } else {
        // Requested Dpy not found in this MetaMode.
        goto cleanup;
    }

    XNVCTRLSetStringAttribute(xDpy,
                              screenId,
                              0, // displayMask
                              NV_CTRL_STRING_CURRENT_METAMODE_VERSION_2,
                              newMetaMode);
    XSync(xDpy, 0);

    error = 0;

cleanup:

    if (newMetaMode) {
        free(newMetaMode);
    }

    if (pOldCurrentMetaMode) {
        free(pOldCurrentMetaMode);
    }

    return error;
}

// This creates a copy of a given target Pixmap.
static inline Pixmap
ClonePixmap(
    Display *xDpy,
    int screenId,
    Pixmap targetPixmap)
{
    Pixmap newPixmap = None;
    GC gc;
    unsigned int width, height, depth;
    // unneeded stuff below
    Window parent;
    int x, y;
    unsigned int borderWidth;

    XGetGeometry(xDpy, targetPixmap, &parent, &x, &y,
                 &width, &height, &borderWidth, &depth);

    newPixmap = XCreatePixmap(xDpy, RootWindow(xDpy, screenId),
                              width, height, depth);

    if (newPixmap == None) {
        return None;
    }

    gc = XCreateGC(xDpy, newPixmap, 0, NULL);

    XCopyArea(xDpy, targetPixmap, newPixmap, gc, 0, 0, width, height, 0, 0);

    XFreeGC(xDpy, gc);

    return newPixmap;
}

static inline int
SetPixmapDataToAttribute(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    Pixmap pixmap,
    Bool blendAfterWarp,
    const char* attributeName)
{
    Bool ret = False;
    char tempName[32];
    char newAttributes[256];

    // Disable the attribute on that DPY
    if (pixmap == None) {
        return RemoveAttributeFromDisplayOfCurrentMetaMode(xDpy,
                                                           screenId,
                                                           nvDpyId,
                                                           attributeName);
    }

    // Get our own copy of the immediate contents of this pixmap.
    pixmap = ClonePixmap(xDpy, screenId, pixmap);

    if (pixmap == None) {
        return 1;
    }

    // Generate a throwaway random name to bind it to.
    sprintf(tempName, "%d", rand());

    if (blendAfterWarp) {
        sprintf(newAttributes, "%s=%s, BlendOrder=BlendAfterWarp",
                attributeName, tempName);
    } else {
        sprintf(newAttributes, "%s=%s, BlendOrder=WarpAfterBlend",
                attributeName, tempName);
    }

    ret = XNVCTRLBindWarpPixmapName(xDpy,
                                    screenId,
                                    pixmap,
                                    tempName,
                                    NV_CTRL_WARP_DATA_TYPE_BLEND_OR_OFFSET_TEXTURE,
                                    0); // vertexCount, unneeded for blend/off.

    // Removes the ClonePixmap() reference, but the name above still holds one.
    XFreePixmap(xDpy, pixmap);

    if (!ret) {
        return 1;
    }

    if (AddAttributesToDisplayOfCurrentMetaMode(xDpy, screenId, nvDpyId,
                                                newAttributes)) {
        return 1;
    }

    XSync(xDpy, 0);

    return 0;
}

static inline int
XNVCTRLSetScanoutWarping(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    int warpDataType,
    int vertexCount,
    const float *warpData)
{
    Pixmap pTempPix = 0;
    int neededSize;
    int rowSize;
    int neededRows;
    Bool ret = False;
    XImage *pTempImage = NULL;
    GC pGC = NULL;
    char *paddedBuffer = NULL;
    int error = 1;
    char tempName[32];
    char newAttributes[256];

    // Disable warping on that DPY
    if (warpData == NULL) {
        return RemoveAttributeFromDisplayOfCurrentMetaMode(xDpy,
                                                           screenId,
                                                           nvDpyId,
                                                           "WarpMesh");
    }

    // Generate a throwaway random name to bind the data we're going to upload.
    sprintf(tempName, "%d", rand());

    if (!xDpy || !vertexCount || !warpData) {
        goto cleanup;
    }

    // Let's use a 1024-wide Pixmap always; figure out how many rows we need.
    neededSize = vertexCount * sizeof(float) * 6;
    rowSize = 1024 * 4;
    neededRows = (neededSize + (rowSize - 1)) / rowSize;

    // The spec mandates depth 32 for this type of data.
    pTempPix = XCreatePixmap(xDpy, RootWindow(xDpy, screenId),
                             1024, neededRows, 32);

    ret = XNVCTRLBindWarpPixmapName(xDpy, screenId, pTempPix, tempName,
                                    warpDataType, vertexCount);

    if (!ret) {
        goto cleanup;
    }

    paddedBuffer = malloc(1024 * neededRows * 4);

    if (!paddedBuffer) {
        goto cleanup;
    }

    memcpy(paddedBuffer, warpData, neededSize);

    pTempImage = XCreateImage(xDpy, DefaultVisual(xDpy, screenId), 32, ZPixmap,
                              0, paddedBuffer, 1024, neededRows, 32, 0);

    if (!pTempImage) {
        goto cleanup;
    }

    pGC = XCreateGC(xDpy, pTempPix, 0, NULL);

    XPutImage(xDpy, pTempPix, pGC, pTempImage, 0, 0, 0, 0, 1024, neededRows);

    // Data is now uploaded to named pixmap; set a mode with it
    sprintf(newAttributes, "WarpMesh=%s", tempName);

    if (AddAttributesToDisplayOfCurrentMetaMode(xDpy, screenId, nvDpyId, newAttributes)) {
        goto cleanup;
    }

    error = 0;

cleanup:

    if (pGC) {
        XFreeGC(xDpy, pGC);
    }

    if (pTempImage) {
        XDestroyImage(pTempImage);
        paddedBuffer = NULL;
    }

    if (paddedBuffer) {
        free(paddedBuffer);
    }

    // The bound name still holds a reference to the Pixmap.
    if (pTempPix) {
        XFreePixmap(xDpy, pTempPix);
    }

    XSync(xDpy, 0);

    return error;
}

static inline int
XNVCTRLSetScanoutIntensity(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    Pixmap intensityPixmap,
    Bool blendAfterWarp)
{
    return SetPixmapDataToAttribute(xDpy, screenId, nvDpyId, intensityPixmap,
                                    blendAfterWarp, "BlendTexture");
}

static inline int
XNVCTRLSetScanoutOffset(
    Display *xDpy,
    int screenId,
    int nvDpyId,
    Pixmap offsetPixmap,
    Bool blendAfterWarp)
{
    return SetPixmapDataToAttribute(xDpy, screenId, nvDpyId, offsetPixmap,
                                    blendAfterWarp, "OffsetTexture");
}
