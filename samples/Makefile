#
# NV-CONTROL samples: Sample tools for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (c) 2004-2008 NVIDIA, Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
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
	nv-control-dpy \
	nv-control-targets \
	nv-control-framelock \
	nv-control-gvi

all: $(SAMPLES)

% : %.c
	$(CC) $(ALL_CFLAGS) $< -o $@ $(ALL_LDFLAGS)

clean:
	rm -rf *.o *~ $(SAMPLES)
