#
# files in the doc directory of nvidia-settings
#

DOC_SRC +=

DOC_EXTRA_DIST += NV-CONTROL-API.txt
DOC_EXTRA_DIST += FRAMELOCK.txt
DOC_EXTRA_DIST += nvidia-settings.1.m4
DOC_EXTRA_DIST += nvidia-settings.desktop
DOC_EXTRA_DIST += nvidia-settings.png
DOC_EXTRA_DIST += src.mk

DOC_DIST_FILES := $(DOC_SRC) $(DOC_EXTRA_DIST)