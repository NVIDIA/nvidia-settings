#
# files in the samples directory of nvidia-settings
#

SAMPLES_SRC +=

SAMPLES_EXTRA_DIST += README
SAMPLES_EXTRA_DIST += nv-control-dvc.c
SAMPLES_EXTRA_DIST += nv-control-3dvisionpro.c
SAMPLES_EXTRA_DIST += nv-control-dpy.c
SAMPLES_EXTRA_DIST += nv-control-info.c
SAMPLES_EXTRA_DIST += nv-control-events.c
SAMPLES_EXTRA_DIST += nv-control-targets.c
SAMPLES_EXTRA_DIST += nv-control-framelock.c
SAMPLES_EXTRA_DIST += nv-control-warpblend.c
SAMPLES_EXTRA_DIST += nv-control-warpblend.h
SAMPLES_EXTRA_DIST += nv-control-screen.h
SAMPLES_EXTRA_DIST += src.mk

SAMPLES_DIST_FILES := $(SAMPLES_SRC) $(SAMPLES_EXTRA_DIST)
