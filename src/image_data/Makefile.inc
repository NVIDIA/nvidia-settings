#
# nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (C) 2004 NVIDIA Corporation.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of Version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
# of the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the:
#
#           Free Software Foundation, Inc.
#           59 Temple Place - Suite 330
#           Boston, MA 02111-1307, USA
#

#
# image data sources and extra files to be included in the
# distribution
#

SRC += \
	image.c

EXTRA_DIST += \
	Makefile.inc \
	HOWTO-ADD-IMAGES \
	image.h \
	rotate_left_off.h \
	rotate_left_on.h \
	rotate_right_off.h \
	rotate_right_on.h \
	rotation_orientation_horiz_flipped.h \
	rotation_orientation_horiz.h \
	rotation_orientation_vert_flipped.h \
	rotation_orientation_vert.h \
	bnc_cable.h \
	led_green.h \
	led_red.h \
	led_grey.h \
	rj45_input.h \
	rj45_output.h \
	rj45_unused.h \
	crt_pixdata.h \
	dfp_pixdata.h \
	tv_pixdata.h \
	background_pixdata.h \
	logo_pixdata.h \
	antialias_pixdata.h \
	thermal_pixdata.h \
	x_pixdata.h \
	background_tall_pixdata.h \
	bsd_pixdata.h \
	clock_pixdata.h \
	color_pixdata.h \
	config_pixdata.h \
	cursor_shadow_pixdata.h \
	display_config_pixdata.h \
	framelock_pixdata.h \
	glx_pixdata.h \
	gpu_pixdata.h \
	help_pixdata.h \
	logo_tall_pixdata.h \
	opengl_pixdata.h \
	penguin_pixdata.h \
	rotation_pixdata.h \
	sdi_pixdata.h \
	solaris_pixdata.h \
	vcsc_pixdata.h \
	xvideo_pixdata.h

dist_list::
	@ echo $(SRC) $(EXTRA_DIST)
