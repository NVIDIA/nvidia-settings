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
# include common variables and functions
##############################################################################

include utils.mk


##############################################################################
# The calling Makefile may export any of the following variables; we
# assign default values if they are not exported by the caller
##############################################################################

ifndef X_LDFLAGS
  ifeq ($(TARGET_OS)-$(TARGET_ARCH),Linux-x86_64)
    X_LDFLAGS          = -L/usr/X11R6/lib64
  else
    X_LDFLAGS          = -L/usr/X11R6/lib
  endif
endif

X_CFLAGS              ?=

GL_INCLUDE_PATH       ?= /usr/include

PKG_CONFIG            ?= pkg-config

ifndef GTK_CFLAGS
  GTK_CFLAGS          := $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
endif

ifndef GTK_LDFLAGS
  GTK_LDFLAGS         := $(shell $(PKG_CONFIG) --libs gtk+-2.0)
endif


##############################################################################
# The XF86Config-parser, libXNVCtrl, and common-utils directories may
# be in one of two places: either elsewhere in the driver source tree
# when building nvidia-settings as part of the NVIDIA driver build (in
# which case, XNVCTRL_DIR, XNVCTRL_ARCHIVE, XCONFIG_PARSER_DIR and
# COMMON_UTILS_DIR should be defined by the calling makefile), or
# directly in the source directory when building from the
# nvidia-settings source tarball (in which case, the below conditional
# assignments should be used)
##############################################################################

XNVCTRL_DIR           ?= src/libXNVCtrl
XNVCTRL_ARCHIVE       ?= $(XNVCTRL_DIR)/libXNVCtrl.a
XCONFIG_PARSER_DIR    ?= src/XF86Config-parser
COMMON_UTILS_DIR      ?= src/common-utils

##############################################################################
# assign variables
##############################################################################

NVIDIA_SETTINGS = $(OUTPUTDIR)/nvidia-settings

NVIDIA_SETTINGS_PROGRAM_NAME = "nvidia-settings"

NVIDIA_SETTINGS_VERSION := $(NVIDIA_VERSION)

CFLAGS += $(X_CFLAGS)

ifeq ($(TARGET_OS),SunOS)
  LDFLAGS += -Wl,-rpath=/usr/X11R6/lib
endif

LDFLAGS += $(X_LDFLAGS)

# Some older Linux distributions do not have the dynamic library
# libXxf86vm.so, though some newer Linux distributions do not have the
# static library libXxf86vm.a.  Statically link against libXxf86vm
# when building nvidia-settings within the NVIDIA driver build, but
# dynamically link against libXxf86vm in the public builds.
ifdef NV_LINK_LIBXXF86VM_STATICALLY
    LDFLAGS += -Wl,-Bstatic -lXxf86vm -Wl,-Bdynamic
else
    LDFLAGS += -lXxf86vm
endif

LDFLAGS += -lX11 -lXext -lm
LDFLAGS += $(GTK_LDFLAGS)

MANPAGE_GZIP ?= 1

MANPAGE_gzipped 	= $(OUTPUTDIR)/nvidia-settings.1.gz
MANPAGE_not_gzipped 	= $(OUTPUTDIR)/nvidia-settings.1
ifeq ($(MANPAGE_GZIP),1)
  MANPAGE     = $(MANPAGE_gzipped)
else
  MANPAGE     = $(MANPAGE_not_gzipped)
endif
GEN_MANPAGE_OPTS   = $(OUTPUTDIR)/gen-manpage-opts
OPTIONS_1_INC      = $(OUTPUTDIR)/options.1.inc

# Include all the source lists; dist-files.mk will define SRC
include dist-files.mk

include $(XCONFIG_PARSER_DIR)/src.mk
SRC        += $(addprefix $(XCONFIG_PARSER_DIR)/,$(XCONFIG_PARSER_SRC))

include $(COMMON_UTILS_DIR)/src.mk
SRC        += $(addprefix $(COMMON_UTILS_DIR)/,$(COMMON_UTILS_SRC))

SRC        += $(STAMP_C)

OBJS        = $(call BUILD_OBJECT_LIST,$(SRC))

CFLAGS     += -I src
CFLAGS     += -I src/image_data
CFLAGS     += -I $(XNVCTRL_DIR)
CFLAGS     += -I $(XCONFIG_PARSER_DIR)/..
CFLAGS     += -I src/libXNVCtrlAttributes
CFLAGS     += -I src/xpm_data
CFLAGS     += -I src/gtk+-2.x
CFLAGS     += -I $(COMMON_UTILS_DIR)
CFLAGS     += -I $(OUTPUTDIR)

$(call BUILD_OBJECT_LIST,$(GTK_SRC)): CFLAGS += $(GTK_CFLAGS)


##############################################################################
# build rules
##############################################################################

.PNONY: all install NVIDIA_SETTINGS_install MANPAGE_install clean clobber

all: $(NVIDIA_SETTINGS) $(MANPAGE)

install: NVIDIA_SETTINGS_install MANPAGE_install

NVIDIA_SETTINGS_install: $(NVIDIA_SETTINGS)
	$(MKDIR) $(bindir)
	$(INSTALL) $(INSTALL_BIN_ARGS) $< $(bindir)/$(notdir $<)

MANPAGE_install: $(MANPAGE)
	$(MKDIR) $(mandir)
	$(INSTALL) $(INSTALL_BIN_ARGS) $< $(mandir)/$(notdir $<)

$(NVIDIA_SETTINGS): $(OBJS) $(XNVCTRL_ARCHIVE)
	$(call quiet_cmd,LINK) -o $@ $(OBJS) $(XNVCTRL_ARCHIVE) \
		$(CFLAGS) $(LDFLAGS) $(BIN_LDFLAGS)
	$(call quiet_cmd,STRIP_CMD) $@

# define the rule to build each object file
$(foreach src,$(SRC),$(eval $(call DEFINE_OBJECT_RULE,CC,$(src))))

# define the rule to generate $(STAMP_C)
$(eval $(call DEFINE_STAMP_C_RULE, $(OBJS),$(NVIDIA_SETTINGS_PROGRAM_NAME)))

clean clobber:
	rm -rf $(NVIDIA_SETTINGS) $(MANPAGE) *~ $(STAMP_C) \
		$(OUTPUTDIR)/*.o $(OUTPUTDIR)/*.d \
		$(GEN_MANPAGE_OPTS) $(OPTIONS_1_INC)


##############################################################################
# Documentation
##############################################################################

AUTO_TEXT = ".\\\" WARNING: THIS FILE IS AUTO-GENERATED!  Edit $< instead."

doc: $(MANPAGE)

GEN_MANPAGE_OPTS_SRC = src/gen-manpage-opts.c

BUILD_MANPAGE_OBJECT_LIST = \
	$(patsubst %.o,%.manpage.o,$(call BUILD_OBJECT_LIST,$(1)))

GEN_MANPAGE_OPTS_OBJS = \
	$(call BUILD_MANPAGE_OBJECT_LIST,$(GEN_MANPAGE_OPTS_SRC))

$(GEN_MANPAGE_OPTS): $(GEN_MANPAGE_OPTS_OBJS)
	$(call quiet_cmd,HOST_LINK) $(GEN_MANPAGE_OPTS_OBJS) -o $@ \
		$(HOST_CFLAGS) $(HOST_LDFLAGS) $(HOST_BIN_LDFLAGS)

# define a rule to build each GEN_MANPAGE_OPTS object file
$(foreach src,$(GEN_MANPAGE_OPTS_SRC),\
	$(eval $(call DEFINE_OBJECT_RULE_WITH_OBJECT_NAME,HOST_CC,$(src),\
		$(call BUILD_MANPAGE_OBJECT_LIST,$(src)))))

$(OPTIONS_1_INC): $(GEN_MANPAGE_OPTS)
	@./$< > $@

$(MANPAGE_not_gzipped): doc/nvidia-settings.1.m4 $(OPTIONS_1_INC)
	$(call quiet_cmd,M4) \
	  -D__HEADER__=$(AUTO_TEXT) \
	  -D__BUILD_OS__=$(TARGET_OS) \
	  -D__VERSION__=$(NVIDIA_VERSION) \
	  -D__DATE__="`$(DATE) +%F`" \
	  -I $(OUTPUTDIR) \
	  $< > $@

$(MANPAGE_gzipped): $(MANPAGE_not_gzipped)
	$(GZIP_CMD) -9f < $< > $@
