# makefile fragment included by nvidia-xconfig, nvidia-settings, and nvidia-installer

# the including makefile should set this if the relevant program uses pciaccess
COMMON_UTILS_PCIACCESS ?=

COMMON_UTILS_SRC        += nvgetopt.c
COMMON_UTILS_SRC        += common-utils.c
COMMON_UTILS_SRC        += msg.c

COMMON_UTILS_EXTRA_DIST += nvgetopt.h
COMMON_UTILS_EXTRA_DIST += common-utils.h
COMMON_UTILS_EXTRA_DIST += msg.h
COMMON_UTILS_EXTRA_DIST += src.mk

# only build nvpci-utils.c for programs that actually use libpciaccess, to
# prevent other programs from needing to set the right CFLAGS/LDFLAGS for code
# they won't use. Otherwise, just package it in the source tarball.
ifneq ($(COMMON_UTILS_PCIACCESS),)
  COMMON_UTILS_SRC += nvpci-utils.c

  ifndef PCIACCESS_CFLAGS
    PCIACCESS_CFLAGS := $(shell $(PKG_CONFIG) --cflags pciaccess)
  endif

  ifndef PCIACCESS_LDFLAGS
    PCIACCESS_LDFLAGS := $(shell $(PKG_CONFIG) --libs pciaccess)
  endif

  $(call BUILD_OBJECT_LIST,nvpci-utils.c): CFLAGS += $(PCIACCESS_CFLAGS)
else
  COMMON_UTILS_EXTRA_DIST += nvpci-utils.c
endif
COMMON_UTILS_EXTRA_DIST += nvpci-utils.h

# gen-manpage-opts-helper.c is listed in EXTRA_DIST, rather than SRC,
# because it is not compiled into the utilities themselves, but used
# when building the utility's gen-manpage-opts
COMMON_UTILS_EXTRA_DIST += gen-manpage-opts-helper.c
COMMON_UTILS_EXTRA_DIST += gen-manpage-opts-helper.h

