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

EXTRA_DIST += \
	antialias_pixdata.h \
	background_pixdata.h \
	background_tall_pixdata.h \
	bnc_cable_pixdata.h \
	bsd_pixdata.h \
	clock_pixdata.h \
	color_pixdata.h \
	config_pixdata.h \
	crt_pixdata.h \
	cursor_shadow_pixdata.h \
	dfp_pixdata.h \
	display_config_pixdata.h \
	framelock_pixdata.h \
	glx_pixdata.h \
	gpu_pixdata.h \
	help_pixdata.h \
	HOWTO-ADD-IMAGES \
	led_green_pixdata.h \
	led_grey_pixdata.h \
	led_red_pixdata.h \
	logo_pixdata.h \
	logo_tall_pixdata.h \
	Makefile.inc \
	nvidia_icon_pixdata.h \
	opengl_pixdata.h \
	penguin_pixdata.h \
	png_to_c_header.sh \
	rj45_input_pixdata.h \
	rj45_output_pixdata.h \
	rj45_unused_pixdata.h \
	rotate_left_off_pixdata.h \
	rotate_left_on_pixdata.h \
	rotate_right_off_pixdata.h \
	rotate_right_on_pixdata.h \
	rotation_orientation_horz_pixdata.h \
	rotation_orientation_vert_pixdata.h \
	rotation_pixdata.h \
	sdi_pixdata.h \
	sdi_shared_sync_bnc_pixdata.h \
	slimm_pixdata.h \
	solaris_pixdata.h \
	thermal_pixdata.h \
	tv_pixdata.h \
	vcs_pixdata.h \
	x_pixdata.h \
	xvideo_pixdata.h

dist_list::
	@ echo $(SRC) $(EXTRA_DIST)
