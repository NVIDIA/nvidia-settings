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

#include <math.h>
#include <string.h>

#include "matrix_utils.h"

void matrixMult(float a[16], const float b[16])
{
    int i, j, k;
    float aa[16];

    memcpy(aa, a, sizeof(aa));
    memset(a, 0, 16 * sizeof(float));

    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            for (k = 0; k < 4; k++) {
                a[4 * j + i] += aa[4 * j + k] * b[4 * k + i];
            }
        }
    }
}

void matrixTranspose(float m[16])
{
    int i, j;
    float tmp;

    for (j = 0; j < 4; ++j) {
        for (i = 0; i < j; ++i) {
            tmp = m[4 * j + i];
            m[4 * j + i] = m[4 * i + j];
            m[4 * i + j] = tmp;
        }
    }
}

void genZeroMatrix(float matrix[16])
{
    memset(matrix, 0, 16 * sizeof(float));
}

void genIdentityMatrix(float matrix[16])
{
    genZeroMatrix(matrix);
    matrix[0]  = 1;
    matrix[5]  = 1;
    matrix[10] = 1;
    matrix[15] = 1;
}

void genTranslateMatrix(float x, float y, float z, float matrix[16])
{
    genIdentityMatrix(matrix);
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
}

void genRotateMatrixX(float radians, float matrix[16])
{
    genIdentityMatrix(matrix);
    matrix[5]  = cosf(radians);
    matrix[6]  = -1 * sinf(radians);
    matrix[9]  = sinf(radians);
    matrix[10] = cosf(radians);
}

void genRotateMatrixY(float radians, float matrix[16])
{
    genIdentityMatrix(matrix);
    matrix[0]  = cosf(radians);
    matrix[2]  = -1 * sinf(radians);
    matrix[8]  = sinf(radians);
    matrix[10] = cosf(radians);
}

void genRotateMatrixZ(float radians, float matrix[16])
{
    genIdentityMatrix(matrix);
    matrix[0]  = cosf(radians);
    matrix[1]  = sinf(radians);
    matrix[4]  = -1 * sinf(radians);
    matrix[5]  = cosf(radians);
}

void genPerspectiveMatrix(float fovY,
                          float aspect,
                          float zNear,
                          float zFar,
                          float matrix[16])
{
    float f;

    f = 1 / tanf(fovY / 2);

    genZeroMatrix(matrix);
    matrix[0]  = f / aspect;
    matrix[5]  = f;
    matrix[10] = (zFar + zNear) / (zNear - zFar);
    matrix[11] = 2 * zFar * zNear / (zNear - zFar);
    matrix[14] = -1;
}

