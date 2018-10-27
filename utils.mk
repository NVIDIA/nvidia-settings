#
# Copyright (C) 2008 NVIDIA Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
#
# utils.mk: common Makefile fragment used by nvidia-xconfig,
# nvidia-installer, and nvidia-settings
#



##############################################################################
# The calling Makefile (when building as part of the NVIDIA graphics
# driver) may export any of the following variables; we assign default
# values if they are not exported by the caller
##############################################################################

CC                    ?= gcc
LD                    ?= ld
# only set these warnings and optimizations if CFLAGS is unset
CFLAGS                ?= -Wall -O2
# always set these -f CFLAGS
CFLAGS                += -fno-strict-aliasing -fno-omit-frame-pointer -Wformat=2
CC_ONLY_CFLAGS        ?=
LDFLAGS               ?=
BIN_LDFLAGS           ?=

HOST_CC               ?= $(CC)
HOST_LD               ?= $(LD)
HOST_CFLAGS           ?= $(CFLAGS)
HOST_LDFLAGS          ?= $(LDFLAGS)
HOST_BIN_LDFLAGS      ?=

# always disable warnings that will break the build
CFLAGS                += -Wno-unused-parameter -Wno-format-zero-length
HOST_CFLAGS           += -Wno-unused-parameter -Wno-format-zero-length

ifeq ($(DEBUG),1)
  STRIP_CMD           ?= true
  DO_STRIP            ?=
  CFLAGS              += -O0 -g
else
  STRIP_CMD           ?= strip
  DO_STRIP            ?= 1
endif

INSTALL               ?= install
INSTALL_BIN_ARGS      ?= -m 755
INSTALL_LIB_ARGS      ?= -m 644
INSTALL_DOC_ARGS      ?= -m 644
INSTALL_PO_ARGS       ?= -m 644

M4                    ?= m4
SED                   ?= sed
M4                    ?= m4
ECHO                  ?= echo
PRINTF                ?= printf
MKDIR                 ?= mkdir -p
RM                    ?= rm -f
TOUCH                 ?= touch
HARDLINK              ?= ln -f
DATE                  ?= date
GZIP_CMD              ?= gzip
CHMOD                 ?= chmod
OBJCOPY               ?= objcopy
MSGFMT                ?= msgfmt
MSGMERGE              ?= msgmerge
XGETTEXT              ?= xgettext
FIND                  ?= find

NV_AUTO_DEPEND        ?= 1
NV_VERBOSE            ?= 0

ifndef TARGET_OS
  TARGET_OS           := $(shell uname)
endif

ifeq ($(TARGET_OS),Linux)
  CFLAGS += -DNV_LINUX
endif

ifeq ($(TARGET_OS),FreeBSD)
  CFLAGS += -DNV_BSD
endif

ifeq ($(TARGET_OS),SunOS)
  CFLAGS += -DNV_SUNOS
endif

ifndef TARGET_ARCH
  TARGET_ARCH         := $(shell uname -m)
  TARGET_ARCH         := $(subst i386,x86,$(TARGET_ARCH))
  TARGET_ARCH         := $(subst i486,x86,$(TARGET_ARCH))
  TARGET_ARCH         := $(subst i586,x86,$(TARGET_ARCH))
  TARGET_ARCH         := $(subst i686,x86,$(TARGET_ARCH))
endif

ifeq ($(TARGET_ARCH),x86)
  CFLAGS += -DNV_X86 -DNV_ARCH_BITS=32
endif

ifeq ($(TARGET_ARCH),x86_64)
  CFLAGS += -DNV_X86_64 -DNV_ARCH_BITS=64
endif

ifeq ($(TARGET_ARCH),armv7l)
  CFLAGS += -DNV_ARMV7 -DNV_ARCH_BITS=32
endif

ifeq ($(TARGET_ARCH),aarch64)
  CFLAGS += -DNV_AARCH64 -DNV_ARCH_BITS=64
endif

ifeq ($(TARGET_ARCH),ppc64le)
  CFLAGS += -DNV_PPC64LE -DNV_ARCH_BITS=64
endif

ifeq ($(TARGET_OS),Linux)
  LIBDL_LIBS = -ldl
else
  LIBDL_LIBS =
endif

# This variable controls which floating-point ABI is targeted.  For ARM, it
# defaults to "gnueabi" for softfp.  Another option is "gnueabihf" for
# hard(fp).  This is necessary to pick up the correct rtld_test binary.
# All other architectures default to empty.
ifeq ($(TARGET_ARCH),armv7l)
  TARGET_ARCH_ABI     ?= gnueabi
endif
TARGET_ARCH_ABI       ?=

ifeq ($(TARGET_ARCH_ABI),gnueabi)
  CFLAGS += -DNV_GNUEABI
endif

ifeq ($(TARGET_ARCH_ABI),gnueabihf)
  CFLAGS += -DNV_GNUEABIHF
endif

OUTPUTDIR             ?= _out/$(TARGET_OS)_$(TARGET_ARCH)
OUTPUTDIR_ABSOLUTE    ?= $(CURDIR)/$(OUTPUTDIR)

NV_SEPARATE_DEBUG_INFO ?=
NV_KEEP_UNSTRIPPED_BINARIES ?=

NV_QUIET_COMMAND_REMOVED_TARGET_PREFIX ?=

CFLAGS += $(CC_ONLY_CFLAGS)


##############################################################################
# This makefile uses the $(eval) builtin function, which was added in
# GNU make 3.80.  Check that the current make version recognizes it.
# Idea suggested by:  http://www.jgc.org/blog/cookbook-sample.pdf
##############################################################################

_eval_available :=
$(eval _eval_available := T)

ifneq ($(_eval_available),T)
  $(error This Makefile requires a GNU Make that supports 'eval'.  Please upgrade to GNU make 3.80 or later)
endif


##############################################################################
# define variables used when installing the open source utilities from
# the source tarball
##############################################################################

PREFIX ?= /usr/local

BINDIR = $(DESTDIR)$(PREFIX)/bin
LIBDIR = $(DESTDIR)$(PREFIX)/lib
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1
LCDIR = $(DESTDIR)$(PREFIX)/share/locale


##############################################################################
# default build rule, so that nothing here in utils.mk accidentally
# gets selected as the default rule
##############################################################################

default build: all


##############################################################################
# get the definition of NVIDIA_VERSION from version.mk
#
# version.mk may be in one of two places: either in $(OUTPUTDIR) when
# building as part of the NVIDIA driver build, or directly in the
# source directory when building from the source tarball
#
# Throw an error if one of these two places did not define NVIDIA_VERSION.
##############################################################################

VERSION_MK := $(wildcard $(OUTPUTDIR)/version.mk version.mk)
include $(VERSION_MK)

ifndef NVIDIA_VERSION
$(error NVIDIA_VERSION undefined)
endif

CFLAGS += -DNVIDIA_VERSION=\"$(NVIDIA_VERSION)\"


##############################################################################
# Several of the functions below take an argument that indicates if
# the expression is for the target platform (the system the built
# program is going to run on) or the host platform (the system
# performing the build).  The argument is either "HOST" or "TARGET"
# and needs to be converted:
#
# "HOST" -> "HOST_"
# "TARGET" -> ""
#
# and prepended to "CC" or "CFLAGS"
##############################################################################

host_target = $(patsubst HOST,HOST_,$(patsubst TARGET,,$(1)))
host_target_cc = $(call host_target,$(1))CC
host_target_cflags = $(call host_target,$(1))CFLAGS


##############################################################################
# to generate the dependency files, use the compiler's "-MM" option to
# generate output of the form "foo.o : foo.c foo.h"; then, use sed to
# wrap the prerequisites with $(wildcard ...); the wildcard function
# serves as an existence filter, so that files that are later removed
# from the build do not cause stale references.  Also, "-MM" will
# cause the compiler to name the target as if it were in the current
# directory ("foo.o: "); use sed to rename the target in the output
# directory ("_out/Linux_x86/foo.o: ") so that the target actually
# applies to the object files produced in the build.
#
# Arguments:
# $(1): whether for host or target platform ("HOST" or "TARGET")
# $(2): source filename
# $(3): object filename
##############################################################################

ifeq ($(NV_AUTO_DEPEND),1)
  AUTO_DEP_CMD = && $($(call host_target_cc,$(1))) \
    -MM $$($(call host_target_cflags,$(1))) $$< | $$(SED) \
    -e "s,: ,: $$$$\(wildcard ," \
    -e "s,\([^\\]\)$$$$,\1)," \
    -e "s;^$$(addsuffix .o,$$(notdir $$(basename $(2)))): ;$(3): ;" \
    > $$(@:.o=.d)
else
  AUTO_DEP_CMD =
endif


##############################################################################
# echo minimal compile information in the non-NV_VERBOSE case
#
# NV_MODULE_LOGGING_NAME can be set to prepend quiet build output with a
# label of which build component is being built
##############################################################################

NV_MODULE_LOGGING_NAME ?=

ifeq ($(NV_VERBOSE),0)
  at_if_quiet := @
  quiet_cmd_no_at = $(PRINTF) \
    " $(if $(NV_MODULE_LOGGING_NAME),[ %-17.17s ],%s)  $(quiet_$(1))\n" \
    "$(NV_MODULE_LOGGING_NAME)" && $($(1))
  quiet_cmd = @$(quiet_cmd_no_at)
else
  at_if_quiet :=
  quiet_cmd_no_at = $($(1))
  quiet_cmd = $($(1))
endif

# define LINK and HOST_LINK to be the same as CC; this is so that,
# even though we use CC to link programs, we can have a different
# quiet rule that uses '$@' as it's arg, rather than '$<'
LINK = $(CC)
HOST_LINK = $(HOST_CC)

# strip NV_QUIET_COMMAND_REMOVED_TARGET_PREFIX from the target string
define_quiet_cmd = $(1) $(patsubst $(NV_QUIET_COMMAND_REMOVED_TARGET_PREFIX)/%,%,$(2))

# define the quiet commands:
quiet_CC           = $(call define_quiet_cmd,CC          ,$<)
quiet_HOST_CC      = $(call define_quiet_cmd,HOST_CC     ,$<)
quiet_LINK         = $(call define_quiet_cmd,LINK        ,$@)
quiet_HOST_LINK    = $(call define_quiet_cmd,HOST_LINK   ,$@)
quiet_M4           = $(call define_quiet_cmd,M4          ,$<)
quiet_STRIP_CMD    = $(call define_quiet_cmd,STRIP       ,$@)
quiet_HARDLINK     = $(call define_quiet_cmd,HARDLINK    ,$@)
quiet_LD           = $(call define_quiet_cmd,LD          ,$@)
quiet_OBJCOPY      = $(call define_quiet_cmd,OBJCOPY     ,$@)

##############################################################################
# Tell gmake to delete the target of a rule if it has changed and its
# commands exit with a nonzero exit status.
##############################################################################
.DELETE_ON_ERROR:


##############################################################################
# function to generate a list of object files from their corresponding
# source files using the specified path. The _WITH_DIR variant takes an
# output path as the second argument while the BUILD_OBJECT_LIST defaults
# to using the value of OUTPUTDIR as the output path. example usage:
#
# OBJS = $(call BUILD_OBJECT_LIST_WITH_DIR,$(SRC),$(DIR))
##############################################################################

BUILD_OBJECT_LIST_WITH_DIR = \
  $(addprefix $(2)/,$(notdir $(addsuffix .o,$(basename $(1)))))

BUILD_OBJECT_LIST = \
  $(call BUILD_OBJECT_LIST_WITH_DIR,$(1),$(OUTPUTDIR))


##############################################################################
# function to generate a list of dependency files from their
# corresponding source files using the specified path. The _WITH_DIR
# variant takes an output path as the second argument while the
# BUILD_DEPENDENCY_LIST default to using the value of OUTPUTDIR as the
# output path. example usage:
#
# DEPS = $(call BUILD_DEPENDENCY_LIST_WITH_DIR,$(SRC),$(DIR))
##############################################################################

BUILD_DEPENDENCY_LIST_WITH_DIR = \
  $(addprefix $(2)/,$(notdir $(addsuffix .d,$(basename $(1)))))

BUILD_DEPENDENCY_LIST = \
  $(call BUILD_DEPENDENCY_LIST_WITH_DIR,$(1),$(OUTPUTDIR))


##############################################################################
# functions to define a rule to build an object file; the first
# argument for all functions is whether the rule is for the target or
# host platform ("HOST" or "TARGET"), the second argument for all
# functions is the source file to compile.
#
# The _WITH_OBJECT_NAME and _WITH_DIR function name suffixes describe
# the third and possibly fourth arguments based on order. The
# _WITH_OBJECT_NAME argument is the object filename to produce while
# the _WITH_DIR argument is the destination path for the object file.
#
# Example usage:
#
#  $(eval $(call DEFINE_OBJECT_RULE,TARGET,foo.c))
#
# Note this also attempts to include the dependency file for this
# source file.
#
# The DEFINE_OBJECT_RULE is functionally equivalent to
# DEFINE_OBJECT_RULE_WITH_OBJECT_NAME, but infers the object file name
# from the source file name (this is normally what you want).
##############################################################################

define DEFINE_OBJECT_RULE_WITH_OBJECT_NAME_WITH_DIR
  $(3): $(2)
	@$(MKDIR) $(4)
	$$(call quiet_cmd,$(call host_target_cc,$(1))) \
	  $$($(call host_target_cflags,$(1))) -c $$< -o $$@ \
	  $(call AUTO_DEP_CMD,$(1),$(2),$(3))

  -include $$(call BUILD_DEPENDENCY_LIST_WITH_DIR,$(3),$(4))

  # declare empty rule for generating dependency file; we generate the
  # dependency files implicitly when compiling the source file (see
  # AUTO_DEP_CMD above), so we don't want gmake to spend time searching
  # for an explicit rule to generate the dependency file
  $$(call BUILD_DEPENDENCY_LIST_WITH_DIR,$(3),$(4)): ;

endef

define DEFINE_OBJECT_RULE_WITH_OBJECT_NAME
  $$(eval $$(call DEFINE_OBJECT_RULE_WITH_OBJECT_NAME_WITH_DIR,$(1),$(2),\
    $(3),$(OUTPUTDIR)))
endef

define DEFINE_OBJECT_RULE_WITH_DIR
  $$(eval $$(call DEFINE_OBJECT_RULE_WITH_OBJECT_NAME_WITH_DIR,$(1),$(2),\
    $$(call BUILD_OBJECT_LIST_WITH_DIR,$(2),$(3)),$(3)))
endef

define DEFINE_OBJECT_RULE
  $$(eval $$(call DEFINE_OBJECT_RULE_WITH_DIR,$(1),$(2),$(OUTPUTDIR)))
endef

# This is a function that will generate rules to build
# files with separate debug information, if so requested.
# 
# It takes one parameter: (1) Name of unstripped binary
#
# When used, the target for linking should be named (1).unstripped
#
# If separate debug information is requested, it will
# generate a rule to build one from the unstripped binary.
# If requested, it will also retain the unstripped binary.
define DEBUG_INFO_RULES
  $(1): $(1).unstripped
  ifneq ($(or $(DO_STRIP),$(NV_SEPARATE_DEBUG_INFO)),)
	$$(call quiet_cmd,STRIP_CMD) -o $$@ $$<
  else
	$$(call quiet_cmd,HARDLINK) $$^ $$@
  endif
  ifeq ($(NV_SEPARATE_DEBUG_INFO),1)
    $(1).debug: $(1).unstripped
	$$(call quiet_cmd,STRIP_CMD) --only-keep-debug -o $$@ $$<
    $(1): $(1).debug
  endif
  ifneq ($(NV_KEEP_UNSTRIPPED_BINARIES),1)
    .INTERMEDIATE: $(1).unstripped
  endif
endef

##############################################################################
# Define rules that can be used for embedding a file into an ELF object that
# contains the raw contents of that file and symbols pointing to the embedded
# data.
#
# Note that objcopy will name the symbols in the resulting object file based on
# the filename specified in $(1).  For example,
#
#   $(eval $(call $(READ_ONLY_OBJECT_FROM_FILE_RULE),a/b/c))
#
# will create an object named $(OUTPUTDIR)/c.o with the symbols _binary_c_start,
# _binary_c_end, and _binary_c_size.
#
# Arguments:
#  $(1): Path to the file to convert
##############################################################################

define READ_ONLY_OBJECT_FROM_FILE_RULE
  $$(OUTPUTDIR)/$$(notdir $(1)).o: $(1)
	$(at_if_quiet)cd $$(dir $(1)); \
	$$(call quiet_cmd_no_at,LD) -r -z noexecstack --format=binary \
	    $$(notdir $(1)) -o $$(OUTPUTDIR_ABSOLUTE)/$$(notdir $$@)
	$$(call quiet_cmd,OBJCOPY) \
	    --rename-section .data=.rodata,contents,alloc,load,data,readonly \
	    $$@
endef
