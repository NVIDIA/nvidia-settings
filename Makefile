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
# This is the top level Makefile for the nvidia-settings utility
#

# Below are variables that users can override, either here or on the
# make commandline
#
# CC = gcc
# CFLAGS = -Wall
# PKG_CONFIG = pkg-config
# X11R6_DIR = /usr/X11R6

# default build target

default: all


# default definitions; can be overwritten by users

SHELL = /bin/sh
INSTALL = install
BUILD_OS := $(shell uname)
BUILD_ARCH := $(shell uname -m)
M4 := m4

ifndef CC
  CC = gcc
endif

ifndef PKG_CONFIG
  PKG_CONFIG = pkg-config
endif

ifndef X11R6_LIB_DIR
  ifeq ($(BUILD_OS)-$(BUILD_ARCH),Linux-x86_64)
    X11R6_LIB_DIR = /usr/X11R6/lib64
  else
    X11R6_LIB_DIR = /usr/X11R6/lib
  endif
endif

ifndef X11R6_INC_DIR
  X11R6_INC_DIR = /usr/X11R6/include
endif

# define local variables

LOCAL_CFLAGS = -Wall

# the NVDEBUG environment variable controls whether we build debug or retail

ifeq ($(NVDEBUG),1)
  STRIP = true
  LOCAL_CFLAGS += -g -DDEBUG
else
  ifndef STRIP
    STRIP = strip
  endif
  LOCAL_CFLAGS += -O -DNDEBUG
endif

# default prefix
ifdef ROOT
  prefix = $(ROOT)/usr
else
  prefix = /usr/local
endif

# default echo within SunOS sh does not have -n option. Use /usr/ucb/echo instead.
# Solaris install has a different argument syntax 
ifeq ($(BUILD_OS),SunOS)
ECHO=/usr/ucb/echo
define INSTALL_RULE
	$(INSTALL) -m 755 -f $(bindir) $(NVIDIA_SETTINGS)
	mkdir -p $(mandir)
	$(INSTALL) -m 644 -f $(mandir) doc/$(MANPAGE)
endef
LD_RUN_FLAG=-R/usr/X11R6/lib
else
ECHO=echo
define INSTALL_RULE
	$(INSTALL) -m 755 $(NVIDIA_SETTINGS) $(bindir)/$(NVIDIA_SETTINGS)
	mkdir -p $(mandir)
	$(INSTALL) -m 644 doc/$(MANPAGE) $(mandir)
	gzip -9f $(mandir)/$(MANPAGE)
endef
endif

exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
mandir = $(exec_prefix)/share/man/man1

X11R6_CFLAGS = -I $(X11R6_INC_DIR)

GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
GTK_LDFLAGS := $(shell $(PKG_CONFIG) --libs gtk+-2.0)

X11R6_LIBS := -L $(X11R6_LIB_DIR) -Wl,-Bstatic -lXxf86vm -Wl,-Bdynamic -lX11 -lXext

XNVCTRL_LIB := src/libXNVCtrl/libXNVCtrl.a

CURDIR := $(shell pwd)

RELATIVE_SRCDIRS = \
	doc \
	src \
	src/image_data \
	src/xpm_data \
	src/gtk+-2.x \
	src/libXNVCtrl \
	src/libXNVCtrlAttributes

SRCDIRS := $(addprefix $(CURDIR)/, $(RELATIVE_SRCDIRS))

INC_FLAGS := $(addprefix -I , $(RELATIVE_SRCDIRS))

ALL_CFLAGS = $(CFLAGS) $(LOCAL_CFLAGS) $(X11R6_CFLAGS) $(GTK_CFLAGS) $(INC_FLAGS)
ALL_LDFLAGS = $(LD_RUN_FLAG) $(LDFLAGS) $(GTK_LDFLAGS) $(X11R6_LIBS)

CPPFLAGS = $(ALL_CFLAGS)


NVIDIA_SETTINGS = nvidia-settings
NVIDIA_SETTINGS_VERSION = 1.0

NVIDIA_SETTINGS_DISTDIR = $(NVIDIA_SETTINGS)-$(NVIDIA_SETTINGS_VERSION)
NVIDIA_SETTINGS_DISTDIR_DIRS := \
	$(addprefix $(NVIDIA_SETTINGS_DISTDIR)/, $(RELATIVE_SRCDIRS))

STAMP_C = g_stamp.c

MANPAGE = nvidia-settings.1

# Define the files in the SAMPLES directory

SAMPLES = \
	Makefile \
	nv-control-dpy.c \
	nv-control-dvc.c \
	nv-control-events.c \
	nv-control-info.c \
	nv-control-targets.c \
	nv-ddcci-client.c \
	nv-control-framelock.c \
	README

# initialize SRC and EXTRA_DIST, then include each of the subdirectory
# Makefiles so that they can append to SRC and EXTRA_DIST

SRC =
EXTRA_DIST =

include $(patsubst %,%/Makefile.inc,$(RELATIVE_SRCDIRS))


# set VPATH 

VPATH = $(RELATIVE_SRCDIRS)


# additional sources (eg: generated sources) can be appended to ALL_SRC

ALL_SRC = $(SRC) $(STAMP_C)


# OBJS and DEPS are constructed such that they are placed into special
# ".objs" and ".deps" subdirectories

OBJS_DIR = .objs
DEPS_DIR = .deps

OBJS := $(patsubst %.c,$(OBJS_DIR)/%.o,$(ALL_SRC))
DEPS := $(patsubst %.c,$(DEPS_DIR)/%.d,$(SRC))

# and now, the build rules:

all: $(NVIDIA_SETTINGS) doc/$(MANPAGE)

install: all
	$(STRIP) $(NVIDIA_SETTINGS)
	$(INSTALL_RULE)

$(OBJS_DIR)/%.o: %.c
	@ mkdir -p $(OBJS_DIR)
	$(CC) -c $(ALL_CFLAGS) $< -o $@

$(DEPS_DIR)/%.d: %.c
	@ mkdir -p $(DEPS_DIR)
	@ set -e; b=`basename $* .c` ; \
	$(CC) -MM $(CPPFLAGS) $< \
	| sed "s%\\($$b\\)\\.o[ :]*%$(OBJS_DIR)/\\1.o $(DEPS_DIR)/\\1.d : %g" > $@; \
	[ -s $@ ] || rm -f $@

$(STAMP_C): $(filter-out $(OBJS_DIR)/$(STAMP_C:.c=.o), $(OBJS))
	@ rm -f $@
	@ $(ECHO) -n "const char NV_ID[] = \"nvidia id: " >> $@
	@ $(ECHO) -n "$(NVIDIA_SETTINGS):  " >> $@
	@ $(ECHO) -n "version $(NVIDIA_SETTINGS_VERSION)  " >> $@
	@ $(ECHO) -n "($(shell whoami)@$(shell hostname))  " >> $@
	@ echo    "$(shell date)\";" >> $@
	@ echo    "const char *pNV_ID = NV_ID + 11;" >> $@


%.i : %.c
	$(CC) $(CPPFLAGS) -E -dD $< | sed -e 's/^ $$//' > $@
	indent -kr -nbbo -l96 -sob $@


$(NVIDIA_SETTINGS): $(OBJS) $(XNVCTRL_LIB)
	$(CC) $(OBJS) $(ALL_CFLAGS) $(ALL_LDFLAGS) $(XNVCTRL_LIB) -o $@

.PHONY: dist clean clobber

dist:
	@ if [ -d $(NVIDIA_SETTINGS_DISTDIR) ]; then \
		chmod 755 $(NVIDIA_SETTINGS_DISTDIR); \
	fi
	@ if [ -f $(NVIDIA_SETTINGS_DISTDIR).tar.gz ]; \
		then chmod 644 $(NVIDIA_SETTINGS_DISTDIR).tar.gz; \
	fi
	rm -rf $(NVIDIA_SETTINGS_DISTDIR) $(NVIDIA_SETTINGS_DISTDIR).tar.gz
	mkdir -p $(NVIDIA_SETTINGS_DISTDIR_DIRS)
	@ for i in $(SRC); do \
		b=`find . -name $$i`; \
		if [ $$b ]; then \
			cp $$b $(NVIDIA_SETTINGS_DISTDIR)/$$b ; \
			chmod 644 $(NVIDIA_SETTINGS_DISTDIR)/$$b ; \
		fi ; \
	done
	@ for i in $(EXTRA_DIST); do \
		b=`find . -name $$i`; \
		if [ $$b ]; then \
			cp $$b $(NVIDIA_SETTINGS_DISTDIR)/$$b ; \
			chmod 644 $(NVIDIA_SETTINGS_DISTDIR)/$$b ; \
		fi ; \
	done
	mkdir -p $(NVIDIA_SETTINGS_DISTDIR)/samples
	@ for i in $(SAMPLES); do \
		cp samples/$$i $(NVIDIA_SETTINGS_DISTDIR)/samples/ ; \
	done
	@ for i in COPYING Makefile doc/Makefile.inc `find src -name Makefile.inc`; do \
		cp $$i $(NVIDIA_SETTINGS_DISTDIR)/$$i ; \
		chmod 644 $(NVIDIA_SETTINGS_DISTDIR)/$$i ; \
	done
	tar czf $(NVIDIA_SETTINGS_DISTDIR).tar.gz $(NVIDIA_SETTINGS_DISTDIR)
	rm -rf $(NVIDIA_SETTINGS_DISTDIR)

clean clobber:
	rm -rf $(OBJS_DIR) $(DEPS_DIR) $(STAMP_C) $(NVIDIA_SETTINGS)
	find . -name "*~" -exec rm -f \{\} \;

-include $(DEPS)
