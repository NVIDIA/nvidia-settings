#
# Copyright (C) 2008 NVIDIA Corporation
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
# along with this program; if not, see <http://www.gnu.org/licenses>.
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
CFLAGS                ?= -Wall -Wno-unused-parameter -O2
# always set these -f CFLAGS
CFLAGS                += -fno-strict-aliasing -fno-omit-frame-pointer
CC_ONLY_CFLAGS        ?=
LDFLAGS               ?=
BIN_LDFLAGS           ?=

HOST_CC               ?= $(CC)
HOST_LD               ?= $(LD)
HOST_CFLAGS           ?= $(CFLAGS)
HOST_LDFLAGS          ?= $(LDFLAGS)
HOST_BIN_LDFLAGS      ?=

ifeq ($(DEBUG),1)
  STRIP_CMD           ?= true
  CFLAGS              += -O0 -g
else
  STRIP_CMD           ?= strip
endif

INSTALL               ?= install
INSTALL_BIN_ARGS      ?= -m 755
INSTALL_DOC_ARGS      ?= -m 644

M4                    ?= m4
SED                   ?= sed
M4                    ?= m4
ECHO                  ?= echo
PRINTF                ?= printf
MKDIR                 ?= mkdir -p
RM                    ?= rm -f
TOUCH                 ?= touch
WHOAMI                ?= whoami
HOSTNAME_CMD          ?= hostname
DATE                  ?= date
GZIP_CMD              ?= gzip
CHMOD                 ?= chmod

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

OUTPUTDIR             ?= _out/$(TARGET_OS)_$(TARGET_ARCH)

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
MANDIR = $(DESTDIR)$(PREFIX)/share/man/man1


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
  quiet_cmd = @$(PRINTF) \
    " $(if $(NV_MODULE_LOGGING_NAME),[ %-17.17s ])  $(quiet_$(1))\n" \
    "$(NV_MODULE_LOGGING_NAME)" && $($(1))
else
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

##############################################################################
# Tell gmake to delete the target of a rule if it has changed and its
# commands exit with a nonzero exit status.
##############################################################################
.DELETE_ON_ERROR:


##############################################################################
# function to generate a list of object files from their corresponding
# source files; example usage:
#
# OBJS = $(call BUILD_OBJECT_LIST,$(SRC))
##############################################################################

BUILD_OBJECT_LIST = \
  $(addprefix $(OUTPUTDIR)/,$(notdir $(addsuffix .o,$(basename $(1)))))


##############################################################################
# function to generate a list of dependency files from their
# corresponding source files; example usage:
#
# DEPS = $(call BUILD_DEPENDENCY_LIST,$(SRC))
##############################################################################

BUILD_DEPENDENCY_LIST = \
  $(addprefix $(OUTPUTDIR)/,$(notdir $(addsuffix .d,$(basename $(1)))))


##############################################################################
# functions to define a rule to build an object file; the first
# argument whether the rule is for the target or host platform ("HOST"
# or "TARGET"), the second argument is the source file to compile, and
# the third argument (_WITH_OBJECT_NAME-only) is the object filename
# to produce.  Example usage:
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

define DEFINE_OBJECT_RULE_WITH_OBJECT_NAME
  $(3): $(2)
	@$(MKDIR) $(OUTPUTDIR)
	$$(call quiet_cmd,$(call host_target_cc,$(1))) \
	  $$($(call host_target_cflags,$(1))) -c $$< -o $$@ \
	  $(call AUTO_DEP_CMD,$(1),$(2),$(3))

  -include $$(call BUILD_DEPENDENCY_LIST,$(3))

  # declare empty rule for generating dependency file; we generate the
  # dependency files implicitly when compiling the source file (see
  # AUTO_DEP_CMD above), so we don't want gmake to spend time searching
  # for an explicit rule to generate the dependency file
  $$(call BUILD_DEPENDENCY_LIST,$(3)): ;

endef

define DEFINE_OBJECT_RULE
  $$(eval $$(call DEFINE_OBJECT_RULE_WITH_OBJECT_NAME,$(1),$(2),\
    $$(call BUILD_OBJECT_LIST,$(2))))
endef


##############################################################################
# STAMP_C - this is a source file that is generated during the build
# to capture information about the build environment for the utility.
#
# The DEFINE_STAMP_C_RULE function is used to define the rule for
# generating STAMP_C.  First argument is a list of dependencies for
# STAMP_C (g_stamp.o is filtered out of the list); second argument is
# the name of the program being built.
#
# The includer of utils.mk should add $(STAMP_C) to its list of source
# files
##############################################################################

STAMP_C = $(OUTPUTDIR)/g_stamp.c

define DEFINE_STAMP_C_RULE

  $$(STAMP_C): $$(filter-out \
    $$(call BUILD_OBJECT_LIST,$$(STAMP_C)),$(1)) \
    $$(VERSION_MK)
	@ $$(RM) $$@
	@ $$(PRINTF) "const char NV_ID[] = \"nvidia id: "                  >> $$@
	@ $$(PRINTF) "$(2):  "                                             >> $$@
	@ $$(PRINTF) "version $$(NVIDIA_VERSION)  "                        >> $$@
	@ $$(PRINTF) "($$(shell $$(WHOAMI))@$$(shell $$(HOSTNAME_CMD)))  " >> $$@
	@ $$(PRINTF) "$$(shell $(DATE))\";\n"                              >> $$@
	@ $$(PRINTF) "const char *pNV_ID = NV_ID + 11;\n"                  >> $$@

endef
