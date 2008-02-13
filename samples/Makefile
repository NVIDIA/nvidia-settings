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
# Makefile for Sample NV-CONTROL clients
#

# Below are variables that users can override, either here or on the
# make commandline
#
# CC = gcc
# CFLAGS = -Wall
# LDFLAGS = 
# X11R6_INC_DIR = /usr/X11R6/include
# X11R6_LIB_DIR = /usr/X11R6/lib/

BUILD_OS := $(shell uname)
BUILD_ARCH := $(shell uname -m)

ifndef CC
  CC = gcc
endif

ifndef CFLAGS
  CFLAGS = -Wall 
endif

ifndef LDFLAGS
  LDFLAGS =
endif

ifndef X11R6_INC_DIR
  X11R6_INC_DIR = /usr/X11R6/include/
endif

ifndef X11R6_LIB_DIR
  ifeq ($(BUILD_OS)-$(BUILD_ARCH),Linux-x86_64)
    X11R6_LIB_DIR = /usr/X11R6/lib64
  else
    X11R6_LIB_DIR = /usr/X11R6/lib
  endif
endif

LIBXNVCTRL_DIR = ../src/libXNVCtrl


ALL_CFLAGS = $(CFLAGS) -I $(LIBXNVCTRL_DIR) -I $(X11R6_INC_DIR)
ALL_LDFLAGS = $(LDFLAGS) -L $(LIBXNVCTRL_DIR) -L $(X11R6_LIB_DIR) \
	-lXNVCtrl -lXext -lX11

SAMPLES = \
	nv-control-info \
	nv-control-dvc \
	nv-control-events \
	nv-ddcci-client \
	nv-control-dpy \
	nv-control-targets \
	nv-control-framelock

all: $(SAMPLES)

% : %.c
	$(CC) $(ALL_CFLAGS) $< -o $@ $(ALL_LDFLAGS)

clean:
	rm -rf *.o *~ $(SAMPLES)
