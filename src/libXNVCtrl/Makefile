# Copyright (c) 2008 NVIDIA, Corporation
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

VERSION_MK := $(wildcard $(OUTPUTDIR)/version.mk version.mk)
include $(VERSION_MK)

ifndef NVIDIA_VERSION
$(error NVIDIA_VERSION undefined)
endif

RANLIB ?= ranlib
CFLAGS += -fPIC

all: libXNVCtrl.a libXNVCtrl.so

libXNVCtrl.a : libXNVCtrl.a(NVCtrl.o)
	$(RANLIB) $@

libXNVCtrl.so: NVCtrl.o
	$(RM) $@ $@.*
	$(CC) -shared -Wl,-soname=$@.0 -o $@.$(NVIDIA_VERSION) $(LDFLAGS) $^ -lXext -lX11
	ln -s $@.$(NVIDIA_VERSION) $@.0
	ln -s $@.0 $@

NVCtrl.o : NVCtrl.h nv_control.h NVCtrlLib.h
.INTERMEDIATE: NVCtrl.o

clean ::
	rm -f libXNVCtrl.a *.o
	rm -f libXNVCtrl.so libXNVCtrl.so.*
.PHONY: clean
