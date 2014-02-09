#
# nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (C) 2008-2012 NVIDIA Corporation.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses>.
#


##############################################################################
# include common variables and functions
##############################################################################

UTILS_MK_DIR ?= ..

include $(UTILS_MK_DIR)/utils.mk


##############################################################################
# The common-utils directory may be in one of two places: either
# elsewhere in the driver source tree when building nvidia-settings as
# part of the NVIDIA driver build (in which case, COMMON_UTILS_DIR
# should be defined by the calling makefile), or directly in the
# source directory when building from the nvidia-settings source
# tarball (in which case, the below conditional assignments should be
# used)
##############################################################################

COMMON_UTILS_DIR      ?= ../src/common-utils


##############################################################################
# assign variables
##############################################################################

MANPAGE_GZIP ?= 1

MANPAGE_gzipped         = $(OUTPUTDIR)/nvidia-settings.1.gz
MANPAGE_not_gzipped 	= $(OUTPUTDIR)/nvidia-settings.1
ifeq ($(MANPAGE_GZIP),1)
  MANPAGE               = $(MANPAGE_gzipped)
else
  MANPAGE               = $(MANPAGE_not_gzipped)
endif
GEN_MANPAGE_OPTS        = $(OUTPUTDIR_ABSOLUTE)/gen-manpage-opts
OPTIONS_1_INC           = $(OUTPUTDIR)/options.1.inc

common_cflags += -I $(COMMON_UTILS_DIR)
common_cflags += -I ../src/libXNVCtrlAttributes

CFLAGS += $(common_cflags)
HOST_CFLAGS += $(common_cflags)

##############################################################################
# build rules
##############################################################################

.PNONY: all install MANPAGE_install clean clobber

all: $(MANPAGE)

install: MANPAGE_install

MANPAGE_install: $(MANPAGE)
	$(MKDIR) $(MANDIR)
	$(INSTALL) $(INSTALL_DOC_ARGS) $< $(MANDIR)/$(notdir $<)

clean clobber:
	rm -rf $(MANPAGE) *~ $(OUTPUTDIR)/*.o $(OUTPUTDIR)/*.d \
		$(GEN_MANPAGE_OPTS) $(OPTIONS_1_INC)


##############################################################################
# Documentation
##############################################################################

AUTO_TEXT = ".\\\" WARNING: THIS FILE IS AUTO-GENERATED!  Edit $< instead."

doc: $(MANPAGE)

GEN_MANPAGE_OPTS_SRC  = ../src/gen-manpage-opts.c
GEN_MANPAGE_OPTS_SRC += $(COMMON_UTILS_DIR)/gen-manpage-opts-helper.c

GEN_MANPAGE_OPTS_OBJS = $(call BUILD_OBJECT_LIST,$(GEN_MANPAGE_OPTS_SRC))

$(foreach src, $(GEN_MANPAGE_OPTS_SRC), \
    $(eval $(call DEFINE_OBJECT_RULE,HOST,$(src))))

$(GEN_MANPAGE_OPTS): $(GEN_MANPAGE_OPTS_OBJS)
	$(call quiet_cmd,HOST_LINK) \
	    $(HOST_CFLAGS) $(HOST_LDFLAGS) $(HOST_BIN_LDFLAGS) $^ -o $@

$(OPTIONS_1_INC): $(GEN_MANPAGE_OPTS)
	@$< > $@

$(MANPAGE_not_gzipped): nvidia-settings.1.m4 $(OPTIONS_1_INC) $(VERSION_MK)
	$(call quiet_cmd,M4) \
	  -D__HEADER__=$(AUTO_TEXT) \
	  -D__BUILD_OS__=$(TARGET_OS) \
	  -D__VERSION__=$(NVIDIA_VERSION) \
	  -D__DATE__="`$(DATE) +%F`" \
	  -I $(OUTPUTDIR) \
	  $< > $@

$(MANPAGE_gzipped): $(MANPAGE_not_gzipped)
	$(GZIP_CMD) -9f < $< > $@
