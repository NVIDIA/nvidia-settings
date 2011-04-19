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

ifndef X11_LIB_DIRS
  ifeq ($(BUILD_OS)-$(BUILD_ARCH),Linux-x86_64)
    X11_LIB_DIRS = -L/usr/X11R6/lib64
  else
    X11_LIB_DIRS = -L/usr/X11R6/lib
  endif
endif

ifndef X11_INC_DIRS
  X11_INC_DIRS = -I/usr/X11R6/include
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

X11_CFLAGS = $(X11_INC_DIRS)

GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
GTK_LDFLAGS := $(shell $(PKG_CONFIG) --libs gtk+-2.0)

X11_LIBS := $(X11_LIB_DIRS) -Wl,-Bstatic -lXxf86vm -Wl,-Bdynamic -lX11 -lXext

XNVCTRL_LIB := src/libXNVCtrl/libXNVCtrl.a
XNVCTRL_DIR := src/libXNVCtrl

XF86PARSER_LIB := src/XF86Config-parser/libXF86Config-parser.a
XF86PARSER_DIR := src/XF86Config-parser

CURDIR := $(shell pwd)

RELATIVE_SRCDIRS = \
	doc \
	src \
	src/image_data \
	src/xpm_data \
	src/gtk+-2.x \
	src/libXNVCtrl \
	src/libXNVCtrlAttributes \
	src/XF86Config-parser \
	samples


SRCDIRS := $(addprefix $(CURDIR)/, $(RELATIVE_SRCDIRS))

INC_FLAGS := $(addprefix -I , $(RELATIVE_SRCDIRS))

ALL_CFLAGS = $(CFLAGS) $(LOCAL_CFLAGS) $(X11_CFLAGS) $(GTK_CFLAGS) $(INC_FLAGS)
ALL_LDFLAGS = $(LD_RUN_FLAG) $(LDFLAGS) $(GTK_LDFLAGS) $(X11_LIBS) -lm

CPPFLAGS = $(ALL_CFLAGS)


NVIDIA_SETTINGS = nvidia-settings
NVIDIA_SETTINGS_VERSION = 1.0

NVIDIA_SETTINGS_DISTDIR = $(NVIDIA_SETTINGS)-$(NVIDIA_SETTINGS_VERSION)
NVIDIA_SETTINGS_DISTDIR_DIRS := \
	$(addprefix $(NVIDIA_SETTINGS_DISTDIR)/, $(RELATIVE_SRCDIRS))

STAMP_C = g_stamp.c

MANPAGE = nvidia-settings.1


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

# to generate the dependency files, use the compiler's "-MM" option to
# generate output of the form "foo.o : foo.c foo.h"; then, use sed to
# replace the target with "$(OBJS_DIR)/foo.o $(DEPS_DIR)/foo.d", and
# wrap the prerequisites with $(wildcard ...); the wildcard function
# serves as an existence filter, so that files that are later removed
# from the build do not cause stale references.

$(DEPS_DIR)/%.d: %.c
	@ mkdir -p $(DEPS_DIR)
	@ set -e; b=`basename $* .c` ; \
	$(CC) -MM $(CPPFLAGS) $< \
	| sed \
	-e "s%\\($$b\\)\\.o[ :]*%$(OBJS_DIR)/\\1.o $(DEPS_DIR)/\\1.d : $$\(wildcard %g" \
	-e "s,\([^\\]\)$$,\1)," > $@; \
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

$(XF86PARSER_LIB):
	$(MAKE) NV_CFLAGS='$(NV_CFLAGS)' -C $(XF86PARSER_DIR)

$(XNVCTRL_LIB):
	$(MAKE) CFLAGS='$(ALL_CFLAGS)' LDFLAGS='$(ALL_LDFLAGS)' -C $(XNVCTRL_DIR)

$(NVIDIA_SETTINGS): $(OBJS) $(XNVCTRL_LIB) $(XF86PARSER_LIB)
	$(CC) $(OBJS) $(ALL_CFLAGS) $(ALL_LDFLAGS) $(XNVCTRL_LIB) $(XF86PARSER_LIB) -o $@

.PHONY: $(XF86PARSER_LIB) dist clean clobber

dist: $(XNVCTRL_LIB)
	@ if [ -d $(NVIDIA_SETTINGS_DISTDIR) ]; then \
		chmod 755 $(NVIDIA_SETTINGS_DISTDIR); \
	fi
	@ if [ -f $(NVIDIA_SETTINGS_DISTDIR).tar.gz ]; then \
		chmod 644 $(NVIDIA_SETTINGS_DISTDIR).tar.gz; \
	fi
	rm -rf $(NVIDIA_SETTINGS_DISTDIR) $(NVIDIA_SETTINGS_DISTDIR).tar.gz
	mkdir -p $(NVIDIA_SETTINGS_DISTDIR_DIRS)
	@ for inc_dir in $(RELATIVE_SRCDIRS) .; do \
		EXTRA_DIST=; \
		SRC=; \
		mkdir -p $(NVIDIA_SETTINGS_DISTDIR)/$$inc_dir; \
		for inc_file in `$(MAKE) --makefile $$inc_dir/Makefile.inc dist_list`; do \
			file="$$inc_dir/$$inc_file"; \
			cp $$file $(NVIDIA_SETTINGS_DISTDIR)/$$file; \
			chmod 644 $(NVIDIA_SETTINGS_DISTDIR)/$$file; \
		done; \
	done
	tar czf $(NVIDIA_SETTINGS_DISTDIR).tar.gz $(NVIDIA_SETTINGS_DISTDIR)
	rm -rf $(NVIDIA_SETTINGS_DISTDIR)

clean clobber:
	rm -rf $(OBJS_DIR) $(DEPS_DIR) $(STAMP_C) $(NVIDIA_SETTINGS) $(XNVCTRL_LIB)
	find . -name "*~" -exec rm -f \{\} \;

-include $(DEPS)
