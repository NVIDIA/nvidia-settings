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

PKG_CONFIG            ?= pkg-config

ifndef GTK2_CFLAGS
  GTK2_CFLAGS          := $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
endif

ifndef GTK2_LDFLAGS
  GTK2_LDFLAGS         := -Wl,--no-as-needed $(shell $(PKG_CONFIG) --libs gtk+-2.0)
endif

ifndef GTK3_AVAILABLE
  GTK3_AVAILABLE := $(shell $(PKG_CONFIG) --exists gtk+-3.0 && echo 1)
endif

ifeq (1,$(GTK3_AVAILABLE))
  ifndef GTK3_CFLAGS
    GTK3_CFLAGS          := $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
  endif

  ifndef GTK3_LDFLAGS
    GTK3_LDFLAGS         := -Wl,--no-as-needed $(shell $(PKG_CONFIG) --libs gtk+-3.0)
  endif
else
  GTK3_CFLAGS ?=
  GTK3_LDFLAGS ?=
endif

# If GTK3_CFLAGS is set, try and build GTK3LIB
ifdef GTK3_CFLAGS
  BUILD_GTK3LIB = 1
endif

ifndef VDPAU_CFLAGS
  VDPAU_CFLAGS        := $(shell $(PKG_CONFIG) --cflags vdpau 2> /dev/null)
  ifeq ($(VDPAU_CFLAGS),)
    VDPAU_CFLAGS       = -I /usr/include
  endif
endif

# These will be unused if nvidia-settings links dynamically against jansson
ifndef JANSSON_CFLAGS
  JANSSON_CFLAGS  = -Wno-cast-qual
  JANSSON_CFLAGS += -Wno-strict-prototypes
  JANSSON_CFLAGS += -Wno-unused-function
  JANSSON_CFLAGS += -DHAVE_CONFIG_H
  ifeq ($(TARGET_ARCH),armv7l)
    JANSSON_CFLAGS += -Wno-unused-but-set-variable
  endif
endif

##############################################################################
# The XF86Config-parser, libXNVCtrl, and common-utils directories may
# be in one of two places: either elsewhere in the driver source tree
# when building nvidia-settings as part of the NVIDIA driver build (in
# which case, XNVCTRL_DIR, XNVCTRL_ARCHIVE, XCONFIG_PARSER_DIR,
# COMMON_UTILS_DIR and COMMON_UNIX_DIR should be defined by the calling
# makefile), or directly in the source directory when building from the
# nvidia-settings source tarball (in which case, the below conditional
# assignments should be used)
##############################################################################

XNVCTRL_DIR             ?= libXNVCtrl
XNVCTRL_MAKEFILE        ?= Makefile
XNVCTRL_ARCHIVE         ?= $(XNVCTRL_DIR)/libXNVCtrl.a
XCONFIG_PARSER_DIR      ?= XF86Config-parser
COMMON_UTILS_DIR        ?= common-utils
COMMON_UNIX_DIR         ?= common-unix
VIRTUAL_RESOLUTIONS_DIR ?= $(COMMON_UNIX_DIR)/virtual-resolutions

##############################################################################
# assign variables
##############################################################################

NVIDIA_SETTINGS = $(OUTPUTDIR)/nvidia-settings

NVIDIA_SETTINGS_PROGRAM_NAME = "nvidia-settings"

NVIDIA_SETTINGS_VERSION := $(NVIDIA_VERSION)

GTK2LIB_NAME = libnvidia-gtk2.so
GTK2LIB_DIR  = $(OUTPUTDIR)/gtk2
GTK2LIB = $(OUTPUTDIR)/$(GTK2LIB_NAME)
GTK2LIB_SONAME = $(GTK2LIB_NAME).$(NVIDIA_SETTINGS_VERSION)

ifdef BUILD_GTK3LIB
  GTK3LIB_NAME = libnvidia-gtk3.so
  GTK3LIB_DIR  = $(OUTPUTDIR)/gtk3
  GTK3LIB = $(OUTPUTDIR)/$(GTK3LIB_NAME)
  GTK3LIB_SONAME = $(GTK3LIB_NAME).$(NVIDIA_SETTINGS_VERSION)
else
  # This value is empty if we are not building GTK3 so that the
  # nvidia-settings build step has this as an empty dependency.
  GTK3LIB =
endif

CFLAGS += $(X_CFLAGS)

ifeq ($(TARGET_OS),SunOS)
  LDFLAGS += -Wl,-rpath=/usr/X11R6/lib -Wl,--export-dynamic
  LIBS += -lscf
endif

LDFLAGS += $(X_LDFLAGS)


# Link against NVIDIA NVML library if available
    
# Set to 1 if NVML is available on the target system
NVML_AVAILABLE ?= 0

ifeq (1,$(NVML_AVAILABLE))
  # Here "gdk" stands for "GPU Developer Kit", rather than gtk's lower-level
  # "GIMP Drawing Kit"
  NVML_CFLAGS  ?= -I /usr/include/nvidia/gdk -DNVML_AVAILABLE
  NVML_LDFLAGS ?=
  CFLAGS       += $(NVML_CFLAGS)
  LDFLAGS      += $(NVML_LDFLAGS)
  LIBS         += -lnvidia-ml
endif

# Some older Linux distributions do not have the dynamic library
# libXxf86vm.so, though some newer Linux distributions do not have the
# static library libXxf86vm.a.  Statically link against libXxf86vm
# when building nvidia-settings within the NVIDIA driver build, but
# dynamically link against libXxf86vm in the public builds.
NV_LINK_LIBXXF86VM_STATICALLY ?= 0

ifneq ($(NV_LINK_LIBXXF86VM_STATICALLY),0)
    LIBS += -Wl,-Bstatic -lXxf86vm -Wl,-Bdynamic
else
    LIBS += -lXxf86vm
endif

# nvidia-settings depends on libjansson >= 2.2.  If NV_USE_BUNDLED_LIBJANSSON is
# set to a non-zero value, then nvidia-settings is linked statically against the
# copy of libjansson bundled in the source tarball; if it is set to 0,
# nvidia-settings is linked dynamically against the copy of libjansson on the
# host system.  By default, nvidia-settings is linked against the bundled copy.
NV_USE_BUNDLED_LIBJANSSON ?= 1

ifeq ($(NV_USE_BUNDLED_LIBJANSSON),0)
    LIBS += -ljansson
endif

# Preferably, we would use pkg-config's "--libs-only-l" and
# "--libs-only-L" options to get separate GTK_LDFLAGS and GTK_LIBS,
# appending them to LDFLAGS and LIBS, respectively.  However, the
# pkg-config(1) man page cautions:
#
#     [...] Note that the union of "--libs-only-l"  and
#     "--libs-only-L"  may be smaller than "--libs", due to flags such
#     as -rdynamic.
#
# So append all of GTK_LDFLAGS to LIBS, so that LIBS can go after
# $(OBJECTS) on the link commandline, causing libraries for linking to
# be named after the objects that depend on those libraries (needed
# for "--as-needed" linker behavior).
LIBS += -lX11 -lXext -lm $(LIBDL_LIBS)

GTK2_LIBS += $(GTK2_LDFLAGS)
GTK3_LIBS += $(GTK3_LDFLAGS)

# Include all the source lists; dist-files.mk will define NVIDIA_SETTINGS_SRC
include src.mk
SRC        += $(NVIDIA_SETTINGS_SRC)

include $(XCONFIG_PARSER_DIR)/src.mk
XCP_SRC    += $(addprefix $(XCONFIG_PARSER_DIR)/,$(XCONFIG_PARSER_SRC))

include $(COMMON_UTILS_DIR)/src.mk
SRC        += $(addprefix $(COMMON_UTILS_DIR)/,$(COMMON_UTILS_SRC))

include $(VIRTUAL_RESOLUTIONS_DIR)/src.mk
SRC        += $(addprefix $(VIRTUAL_RESOLUTIONS_DIR)/,$(VIRTUAL_RESOLUTIONS_SRC))

SRC        += $(STAMP_C)

OBJS        = $(call BUILD_OBJECT_LIST,$(SRC))
XCP_OBJS    = $(call BUILD_OBJECT_LIST,$(XCP_SRC))

GTK2_OBJS    = $(call BUILD_OBJECT_LIST_WITH_DIR,$(GTK_SRC),$(GTK2LIB_DIR))
GTK3_OBJS    = $(call BUILD_OBJECT_LIST_WITH_DIR,$(GTK_SRC),$(GTK3LIB_DIR))

CFLAGS     += -I .
CFLAGS     += -I image_data
CFLAGS     += -I $(XNVCTRL_DIR)
CFLAGS     += -I $(XCONFIG_PARSER_DIR)/..
CFLAGS     += -I libXNVCtrlAttributes
CFLAGS     += -I xpm_data

ifneq ($(NV_USE_BUNDLED_LIBJANSSON),0)
  CFLAGS     += -I jansson
endif

CFLAGS     += -I $(COMMON_UTILS_DIR)
CFLAGS     += -I $(VIRTUAL_RESOLUTIONS_DIR)
CFLAGS     += -I $(OUTPUTDIR)
CFLAGS     += $(VDPAU_CFLAGS)
CFLAGS     += -DPROGRAM_NAME=\"nvidia-settings\"

$(call BUILD_OBJECT_LIST,$(XCP_SRC)): CFLAGS += -fPIC

$(call BUILD_OBJECT_LIST_WITH_DIR,$(GTK_SRC),$(GTK2LIB_DIR)): \
    CFLAGS += $(GTK2_CFLAGS) -fPIC -I $(XCONFIG_PARSER_DIR)/..

ifdef BUILD_GTK3LIB
  $(call BUILD_OBJECT_LIST_WITH_DIR,$(GTK_SRC),$(GTK3LIB_DIR)): \
      CFLAGS += $(GTK3_CFLAGS) -fPIC -I $(XCONFIG_PARSER_DIR)/..
endif

ifneq ($(NV_USE_BUNDLED_LIBJANSSON),0)
  $(call BUILD_OBJECT_LIST,$(JANSSON_SRC)): CFLAGS += $(JANSSON_CFLAGS)
endif


##############################################################################
# build rules
##############################################################################

.PHONY: all install NVIDIA_SETTINGS_install clean clobber build-xnvctrl

all: $(NVIDIA_SETTINGS) $(GTK2LIB) $(GTK3LIB)

install: NVIDIA_SETTINGS_install NVIDIA_GTKLIB_install

NVIDIA_GTKLIB_install: $(GTK2LIB) $(GTK3LIB)
	$(MKDIR) $(LIBDIR)
	$(INSTALL) $(INSTALL_LIB_ARGS) $(GTK2LIB) \
	    $(LIBDIR)/$(GTK2LIB_SONAME)
ifdef BUILD_GTK3LIB
	$(INSTALL) $(INSTALL_LIB_ARGS) $(GTK3LIB) \
	    $(LIBDIR)/$(GTK3LIB_SONAME)
endif

NVIDIA_SETTINGS_install: $(NVIDIA_SETTINGS)
	$(MKDIR) $(BINDIR)
	$(INSTALL) $(INSTALL_BIN_ARGS) $< $(BINDIR)/$(notdir $<)

$(NVIDIA_SETTINGS): $(OBJS) $(XNVCTRL_ARCHIVE)
	$(call quiet_cmd,LINK) $(CFLAGS) $(LDFLAGS) $(BIN_LDFLAGS) \
	    -rdynamic -o $@ $(OBJS) $(XNVCTRL_ARCHIVE) $(LIBS)
	$(call quiet_cmd,STRIP_CMD) $@

$(GTK2LIB): $(GTK2_OBJS) $(XCP_OBJS) $(VERSION_MK)
	$(call quiet_cmd,LINK) -shared $(CFLAGS) $(LDFLAGS) $(BIN_LDFLAGS) \
	    $(XNVCTRL_ARCHIVE) $(LIBS) $(GTK2_LIBS) \
	    -Wl,--unresolved-symbols=ignore-all -o $(GTK2LIB) \
	    -Wl,-soname -Wl,$(GTK2LIB_SONAME) \
	    $(GTK2_OBJS) $(XCP_OBJS)
	$(call quiet_cmd,STRIP_CMD) $@

ifdef BUILD_GTK3LIB
$(GTK3LIB): $(GTK3_OBJS) $(XCP_OBJS) $(VERSION_MK)
	$(call quiet_cmd,LINK) -shared $(CFLAGS) $(LDFLAGS)  $(BIN_LDFLAGS) \
	    $(XNVCTRL_ARCHIVE) $(LIBS) $(GTK3_LIBS) \
	    -Wl,--unresolved-symbols=ignore-all -o $(GTK3LIB) \
	    -Wl,-soname -Wl,$(GTK3LIB_SONAME) \
	    $(GTK3_OBJS) $(XCP_OBJS)
	$(call quiet_cmd,STRIP_CMD) $@
endif

# define the rule to build each object file
$(foreach src,$(SRC),$(eval $(call DEFINE_OBJECT_RULE,TARGET,$(src))))
$(foreach src,$(XCP_SRC),$(eval $(call DEFINE_OBJECT_RULE,TARGET,$(src))))

# define the rule to generate $(STAMP_C)
$(eval $(call DEFINE_STAMP_C_RULE, $(OBJS),$(NVIDIA_SETTINGS_PROGRAM_NAME)))

# define the rule to build $(XNVCTRL_ARCHIVE)
$(XNVCTRL_ARCHIVE): build-xnvctrl

build-xnvctrl:
	@$(MAKE) -C $(XNVCTRL_DIR) -f $(XNVCTRL_MAKEFILE)

clean clobber:
	rm -rf $(NVIDIA_SETTINGS) *~ $(STAMP_C) \
		$(OUTPUTDIR)/*.o $(OUTPUTDIR)/*.d \
		$(GTK2LIB) $(GTK3LIB) $(GTK2LIB_DIR) $(GTK3LIB_DIR)
	@$(MAKE) -C $(XNVCTRL_DIR) -f $(XNVCTRL_MAKEFILE) clean

$(foreach src,$(GTK_SRC), \
    $(eval $(call DEFINE_OBJECT_RULE_WITH_DIR,TARGET,$(src),$(GTK2LIB_DIR))))

ifdef BUILD_GTK3LIB
$(foreach src,$(GTK_SRC), \
    $(eval $(call DEFINE_OBJECT_RULE_WITH_DIR,TARGET,$(src),$(GTK3LIB_DIR))))
endif

