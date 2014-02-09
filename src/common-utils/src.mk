# makefile fragment included by nvidia-xconfig, nvidia-settings, and nvidia-installer

COMMON_UTILS_SRC        += nvgetopt.c
COMMON_UTILS_SRC        += common-utils.c
COMMON_UTILS_SRC        += msg.c

COMMON_UTILS_EXTRA_DIST += nvgetopt.h
COMMON_UTILS_EXTRA_DIST += common-utils.h
COMMON_UTILS_EXTRA_DIST += msg.h
COMMON_UTILS_EXTRA_DIST += src.mk

# gen-manpage-opts-helper.c is listed in EXTRA_DIST, rather than SRC,
# because it is not compiled into the utilities themselves, but used
# when building the utility's gen-manpage-opts
COMMON_UTILS_EXTRA_DIST += gen-manpage-opts-helper.c
COMMON_UTILS_EXTRA_DIST += gen-manpage-opts-helper.h

