/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
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

#ifndef __GLXINFO_H__
#define __GLXINFO_H__

#include <GL/glx.h>


#ifndef GLX_VERSION_1_3
#warning GLX version 1.3 not defined, will not show FBConfig table!
#else
const char * render_type_abbrev(int rend_type);
const char * transparent_type_abbrev(int trans_type);
const char * x_visual_type_abbrev(int x_visual_type);
const char * caveat_abbrev(int caveat);
#endif


void print_glxinfo(const char *display_name);


#endif /* __GLXINFO_H__ */
