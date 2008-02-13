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
# libXNVCtrl files
#

EXTRA_DIST += \
	Makefile.inc \
	NVCtrl.h \
	NVCtrlLib.h \
	libXNVCtrl.a \
        NVCtrl.c \
        nv_control.h \
        README.LIBXNVCTRL \
        Imakefile

dist_list::
	@ echo $(SRC) $(EXTRA_DIST)
