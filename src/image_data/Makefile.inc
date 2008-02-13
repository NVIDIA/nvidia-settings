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
	HOWTO-ADD-IMAGES \
	antialiasing_banner.h \
	big_banner_bsd.h \
	big_banner_penguin.h \
	big_banner_sun.h \
	color_correction_banner.h \
	configuration_banner.h \
	frame_lock_banner.h \
	help_banner.h \
	image.h \
	opengl_banner.h \
	thermal_banner.h \
	xvideo_banner.h \
	display_device_banner.h \
	crt_banner.h \
	dfp_banner.h \
	tv_banner.h \
	cursor_banner.h \
        crt.h \
        tv.h \
        dfp.h \
        gvo_banner_comp_sync_green.h \
        gvo_banner_comp_sync_grey.h \
        gvo_banner_comp_sync_red.h \
        gvo_banner_comp_sync_yellow.h \
        gvo_banner_left.h \
        gvo_banner_right.h \
        gvo_banner_sdi_sync_green.h \
        gvo_banner_sdi_sync_grey.h \
        gvo_banner_sdi_sync_red.h \
        gvo_banner_sdi_sync_yellow.h \
        gvo_banner_vid1_green.h \
        gvo_banner_vid1_grey.h \
        gvo_banner_vid1_red.h \
        gvo_banner_vid1_yellow.h \
        gvo_banner_vid2_green.h \
        gvo_banner_vid2_grey.h \
        gvo_banner_vid2_red.h \
        gvo_banner_vid2_yellow.h \
	glx_banner.h \
	rotate_left_off.h \
	rotate_left_on.h \
	rotate_right_off.h \
	rotate_right_on.h \
	rotation_banner.h \
	rotation_orientation_horiz_flipped.h \
	rotation_orientation_horiz.h \
	rotation_orientation_vert_flipped.h \
	rotation_orientation_vert.h \
        clocks_banner.h
