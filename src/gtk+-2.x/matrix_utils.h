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

#ifndef __MATRIX_UTILS_H__
#define __MATRIX_UTILS_H__

void matrixMult(float a[16], const float b[16]);
void matrixTranspose(float m[16]);

void genZeroMatrix(float matrix[16]);
void genIdentityMatrix(float matrix[16]);

void genTranslateMatrix(float x, float y, float z, float matrix[16]);
void genRotateMatrixX(float radians, float matrix[16]);
void genRotateMatrixY(float radians, float matrix[16]);
void genRotateMatrixZ(float radians, float matrix[16]);
void genPerspectiveMatrix(float fovY,
                          float aspect,
                          float zNear,
                          float zFar,
                          float matrix[16]);

#endif /*__MATRIX_UTILS_H__*/
