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
# src files and headers
#

SRC += \
	Device.c \
	DRI.c \
	Extensions.c \
	Files.c \
	Flags.c \
	Generate.c \
	Input.c \
	Keyboard.c \
	Layout.c \
	Merge.c \
	Module.c \
	Monitor.c \
	Pointer.c \
	Read.c \
	Scan.c \
	Screen.c \
	Util.c \
	Vendor.c \
	Video.c \
	Write.c

EXTRA_DIST += \
	Makefile.inc \
	Configint.h \
	configProcs.h \
	Makefile \
	xf86Parser.h \
	xf86tokens.h

dist_list::
	@ echo $(SRC) $(EXTRA_DIST)
