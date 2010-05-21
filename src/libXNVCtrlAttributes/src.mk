#
# files in the src/libXNVCtrlAttributes directory of nvidia-settings
#

LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributes.c
LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributesNvControl.c
LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributesVidMode.c
LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributesXv.c
LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributesGlx.c
LIB_XNVCTRL_ATTRIBUTES_SRC += NvCtrlAttributesXrandr.c

LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST += NvCtrlAttributes.h
LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST += NvCtrlAttributesPrivate.h
LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST += src.mk
