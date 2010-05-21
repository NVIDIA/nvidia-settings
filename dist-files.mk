#
# nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (C) 2008 NVIDIA Corporation.
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

##############################################################################
# define the list of files that should be distributed in the
# nvidia-settings tarball; this is used by the NVIDIA driver build
# when packaging the tarball, and by the nvidia-settings makefile when
# building nvidia-settings.
#
# Defines SRC, EXTRA_DIST, and DIST_FILES
##############################################################################

SRC        :=
EXTRA_DIST := COPYING dist-files.mk

include src/src.mk
SRC        += $(addprefix src/,$(SRC_SRC))
EXTRA_DIST += $(addprefix src/,$(SRC_EXTRA_DIST))

include src/image_data/src.mk
SRC        += $(addprefix src/image_data/,$(IMAGE_DATA_SRC))
EXTRA_DIST += $(addprefix src/image_data/,$(IMAGE_DATA_EXTRA_DIST))

include src/libXNVCtrlAttributes/src.mk
SRC        += $(addprefix src/libXNVCtrlAttributes/,$(LIB_XNVCTRL_ATTRIBUTES_SRC))
EXTRA_DIST += $(addprefix src/libXNVCtrlAttributes/,$(LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST))

include src/xpm_data/src.mk
SRC        += $(addprefix src/xpm_data/,$(LIB_XPM_DATA_SRC))
EXTRA_DIST += $(addprefix src/xpm_data/,$(LIB_XPM_DATA_EXTRA_DIST))

include src/gtk+-2.x/src.mk
SRC        += $(addprefix src/gtk+-2.x/,$(GTK_SRC))
EXTRA_DIST += $(addprefix src/gtk+-2.x/,$(GTK_EXTRA_DIST))

include doc/src.mk
SRC        += $(addprefix doc/,$(DOC_SRC))
EXTRA_DIST += $(addprefix doc/,$(DOC_EXTRA_DIST))

include samples/src.mk
SRC        += $(addprefix samples/,$(SAMPLES_SRC))
EXTRA_DIST += $(addprefix samples/,$(SAMPLES_EXTRA_DIST))

DIST_FILES := $(SRC) $(EXTRA_DIST)
