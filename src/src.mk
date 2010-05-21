#
# files in the src directory of nvidia-settings
#

SRC_SRC += command-line.c
SRC_SRC += config-file.c
SRC_SRC += lscf.c
SRC_SRC += msg.c
SRC_SRC += nvidia-settings.c
SRC_SRC += parse.c
SRC_SRC += query-assign.c
SRC_SRC += nvgetopt.c
SRC_SRC += glxinfo.c

SRC_EXTRA_DIST += src.mk
SRC_EXTRA_DIST += command-line.h
SRC_EXTRA_DIST += config-file.h
SRC_EXTRA_DIST += lscf.h
SRC_EXTRA_DIST += msg.h
SRC_EXTRA_DIST += parse.h
SRC_EXTRA_DIST += query-assign.h
SRC_EXTRA_DIST += nvgetopt.h
SRC_EXTRA_DIST += glxinfo.h
