##############################################################################
# define the list of files that should be built and distributed in the
# nvidia-settings source tarball; this is used by the NVIDIA driver
# build when packaging the tarball, and by the nvidia-settings
# makefile when building nvidia-settings.
#
# Defines NVIDIA_SETTINGS_SRC, NVIDIA_SETTINGS_EXTRA_DIST, and
# NVIDIA_SETTINGS_DIST_FILES
##############################################################################


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
SRC_SRC += app-profiles.c
SRC_SRC += glxinfo.c

NVIDIA_SETTINGS_SRC += $(SRC_SRC)

SRC_EXTRA_DIST += src.mk
SRC_EXTRA_DIST += command-line.h
SRC_EXTRA_DIST += option-table.h
SRC_EXTRA_DIST += config-file.h
SRC_EXTRA_DIST += lscf.h
SRC_EXTRA_DIST += msg.h
SRC_EXTRA_DIST += parse.h
SRC_EXTRA_DIST += query-assign.h
SRC_EXTRA_DIST += app-profiles.h
SRC_EXTRA_DIST += glxinfo.h
SRC_EXTRA_DIST += gen-manpage-opts.c

NVIDIA_SETTINGS_EXTRA_DIST += $(SRC_EXTRA_DIST)

#
# files in the src/image_data directory of nvidia-settings
#

IMAGE_DATA_SRC +=

IMAGE_DATA_EXTRA_DIST += image_data/HOWTO-ADD-IMAGES
IMAGE_DATA_EXTRA_DIST += image_data/antialias.png
IMAGE_DATA_EXTRA_DIST += image_data/antialias_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/background.png
IMAGE_DATA_EXTRA_DIST += image_data/background_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/background_tall.png
IMAGE_DATA_EXTRA_DIST += image_data/background_tall_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/bnc_cable.png
IMAGE_DATA_EXTRA_DIST += image_data/bnc_cable_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/bsd.png
IMAGE_DATA_EXTRA_DIST += image_data/bsd_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/clock.png
IMAGE_DATA_EXTRA_DIST += image_data/clock_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/color.png
IMAGE_DATA_EXTRA_DIST += image_data/color_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/config.png
IMAGE_DATA_EXTRA_DIST += image_data/config_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/crt.png
IMAGE_DATA_EXTRA_DIST += image_data/crt_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/dfp.png
IMAGE_DATA_EXTRA_DIST += image_data/dfp_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/display_config.png
IMAGE_DATA_EXTRA_DIST += image_data/display_config_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/framelock.png
IMAGE_DATA_EXTRA_DIST += image_data/framelock_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/glx.png
IMAGE_DATA_EXTRA_DIST += image_data/glx_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/gpu.png
IMAGE_DATA_EXTRA_DIST += image_data/gpu_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/gvi.png
IMAGE_DATA_EXTRA_DIST += image_data/gvi_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/help.png
IMAGE_DATA_EXTRA_DIST += image_data/help_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/led_green.png
IMAGE_DATA_EXTRA_DIST += image_data/led_green_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/led_grey.png
IMAGE_DATA_EXTRA_DIST += image_data/led_grey_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/led_red.png
IMAGE_DATA_EXTRA_DIST += image_data/led_red_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/logo.png
IMAGE_DATA_EXTRA_DIST += image_data/logo_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/logo_tall.png
IMAGE_DATA_EXTRA_DIST += image_data/logo_tall_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/nvidia_icon.png
IMAGE_DATA_EXTRA_DIST += image_data/nvidia_icon_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/opengl.png
IMAGE_DATA_EXTRA_DIST += image_data/opengl_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/penguin.png
IMAGE_DATA_EXTRA_DIST += image_data/penguin_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/png_to_c_header.sh
IMAGE_DATA_EXTRA_DIST += image_data/rj45_input.png
IMAGE_DATA_EXTRA_DIST += image_data/rj45_input_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/rj45_output.png
IMAGE_DATA_EXTRA_DIST += image_data/rj45_output_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/rj45_unused.png
IMAGE_DATA_EXTRA_DIST += image_data/rj45_unused_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/sdi.png
IMAGE_DATA_EXTRA_DIST += image_data/sdi_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/sdi_shared_sync_bnc.png
IMAGE_DATA_EXTRA_DIST += image_data/sdi_shared_sync_bnc_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/slimm.png
IMAGE_DATA_EXTRA_DIST += image_data/slimm_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/solaris.png
IMAGE_DATA_EXTRA_DIST += image_data/solaris_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/svp_3dvp.png
IMAGE_DATA_EXTRA_DIST += image_data/svp_3dvp_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/thermal.png
IMAGE_DATA_EXTRA_DIST += image_data/thermal_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/vcs.png
IMAGE_DATA_EXTRA_DIST += image_data/vcs_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/vdpau.png
IMAGE_DATA_EXTRA_DIST += image_data/vdpau_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/x.png
IMAGE_DATA_EXTRA_DIST += image_data/x_pixdata.h
IMAGE_DATA_EXTRA_DIST += image_data/xvideo.png
IMAGE_DATA_EXTRA_DIST += image_data/xvideo_pixdata.h

NVIDIA_SETTINGS_EXTRA_DIST += $(IMAGE_DATA_EXTRA_DIST)

#
# files in the src/libXNVCtrlAttributes directory of nvidia-settings
#

LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributes.c
LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributesNvControl.c
LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributesVidMode.c
LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributesXv.c
LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributesGlx.c
LIB_XNVCTRL_ATTRIBUTES_SRC += libXNVCtrlAttributes/NvCtrlAttributesXrandr.c

NVIDIA_SETTINGS_SRC += $(LIB_XNVCTRL_ATTRIBUTES_SRC)

LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST += libXNVCtrlAttributes/NvCtrlAttributes.h
LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST += libXNVCtrlAttributes/NvCtrlAttributesPrivate.h

NVIDIA_SETTINGS_EXTRA_DIST += $(LIB_XNVCTRL_ATTRIBUTES_EXTRA_DIST)

#
# files in the src/xpm_data directory of nvidia-settings
#

LIB_XPM_DATA_SRC +=

LIB_XPM_DATA_EXTRA_DIST += xpm_data/blue_xpm.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/green_xpm.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/red_xpm.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/rgb_xpm.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_add_glasses.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_autopick_next_channel.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_battery_0.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_battery_100.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_battery_25.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_battery_50.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_battery_75.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_excellent.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_good.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_low.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_nosignal.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_verygood.h
LIB_XPM_DATA_EXTRA_DIST += xpm_data/svp_status_verylow.h

NVIDIA_SETTINGS_EXTRA_DIST += $(LIB_XPM_DATA_EXTRA_DIST)

#
# files in the src/gtk+-2.x directory of nvidia-settings
#

GTK_SRC += gtk+-2.x/ctkxvideo.c
GTK_SRC += gtk+-2.x/ctkui.c
GTK_SRC += gtk+-2.x/ctkframelock.c
GTK_SRC += gtk+-2.x/ctkgauge.c
GTK_SRC += gtk+-2.x/ctkcurve.c
GTK_SRC += gtk+-2.x/ctkcolorcorrection.c
GTK_SRC += gtk+-2.x/ctkcolorcorrectionpage.c
GTK_SRC += gtk+-2.x/ctkscale.c
GTK_SRC += gtk+-2.x/ctkmultisample.c
GTK_SRC += gtk+-2.x/ctkconfig.c
GTK_SRC += gtk+-2.x/ctkevent.c
GTK_SRC += gtk+-2.x/ctkwindow.c
GTK_SRC += gtk+-2.x/ctkopengl.c
GTK_SRC += gtk+-2.x/ctkglx.c
GTK_SRC += gtk+-2.x/ctkhelp.c
GTK_SRC += gtk+-2.x/ctkimagesliders.c
GTK_SRC += gtk+-2.x/ctkdisplaydevice.c
GTK_SRC += gtk+-2.x/ctkditheringcontrols.c
GTK_SRC += gtk+-2.x/ctkthermal.c
GTK_SRC += gtk+-2.x/ctkpowermizer.c
GTK_SRC += gtk+-2.x/ctkgvo.c
GTK_SRC += gtk+-2.x/ctkgvo-csc.c
GTK_SRC += gtk+-2.x/ctkdropdownmenu.c
GTK_SRC += gtk+-2.x/ctkclocks.c
GTK_SRC += gtk+-2.x/ctkutils.c
GTK_SRC += gtk+-2.x/ctkedid.c
GTK_SRC += gtk+-2.x/ctkserver.c
GTK_SRC += gtk+-2.x/ctkdisplaylayout.c
GTK_SRC += gtk+-2.x/ctkdisplayconfig.c
GTK_SRC += gtk+-2.x/ctkscreen.c
GTK_SRC += gtk+-2.x/ctkslimm.c
GTK_SRC += gtk+-2.x/ctkgpu.c
GTK_SRC += gtk+-2.x/ctkbanner.c
GTK_SRC += gtk+-2.x/ctkvcs.c
GTK_SRC += gtk+-2.x/ctkdisplayconfig-utils.c
GTK_SRC += gtk+-2.x/ctkgvo-banner.c
GTK_SRC += gtk+-2.x/ctkgvo-sync.c
GTK_SRC += gtk+-2.x/ctkpowersavings.c
GTK_SRC += gtk+-2.x/ctkgvi.c
GTK_SRC += gtk+-2.x/ctklicense.c
GTK_SRC += gtk+-2.x/ctkecc.c
GTK_SRC += gtk+-2.x/ctkappprofile.c
GTK_SRC += gtk+-2.x/ctkapcprofilemodel.c
GTK_SRC += gtk+-2.x/ctkapcrulemodel.c
GTK_SRC += gtk+-2.x/ctkcolorcontrols.c
GTK_SRC += gtk+-2.x/ctk3dvisionpro.c
GTK_SRC += gtk+-2.x/ctkvdpau.c

NVIDIA_SETTINGS_SRC += $(GTK_SRC)

GTK_EXTRA_DIST += gtk+-2.x/ctkxvideo.h
GTK_EXTRA_DIST += gtk+-2.x/ctkui.h
GTK_EXTRA_DIST += gtk+-2.x/ctkframelock.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgauge.h
GTK_EXTRA_DIST += gtk+-2.x/ctkcurve.h
GTK_EXTRA_DIST += gtk+-2.x/ctkcolorcorrection.h
GTK_EXTRA_DIST += gtk+-2.x/ctkcolorcorrectionpage.h
GTK_EXTRA_DIST += gtk+-2.x/ctkscale.h
GTK_EXTRA_DIST += gtk+-2.x/ctkmultisample.h
GTK_EXTRA_DIST += gtk+-2.x/ctkconfig.h
GTK_EXTRA_DIST += gtk+-2.x/ctkevent.h
GTK_EXTRA_DIST += gtk+-2.x/ctkwindow.h
GTK_EXTRA_DIST += gtk+-2.x/ctkopengl.h
GTK_EXTRA_DIST += gtk+-2.x/ctkglx.h
GTK_EXTRA_DIST += gtk+-2.x/ctkhelp.h
GTK_EXTRA_DIST += gtk+-2.x/ctkimagesliders.h
GTK_EXTRA_DIST += gtk+-2.x/ctkdisplaydevice.h
GTK_EXTRA_DIST += gtk+-2.x/ctkditheringcontrols.h
GTK_EXTRA_DIST += gtk+-2.x/ctkconstants.h
GTK_EXTRA_DIST += gtk+-2.x/ctkthermal.h
GTK_EXTRA_DIST += gtk+-2.x/ctkpowermizer.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgvo.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgvo-csc.h
GTK_EXTRA_DIST += gtk+-2.x/ctkdropdownmenu.h
GTK_EXTRA_DIST += gtk+-2.x/ctkclocks.h
GTK_EXTRA_DIST += gtk+-2.x/ctkutils.h
GTK_EXTRA_DIST += gtk+-2.x/ctkedid.h
GTK_EXTRA_DIST += gtk+-2.x/ctkserver.h
GTK_EXTRA_DIST += gtk+-2.x/ctkdisplaylayout.h
GTK_EXTRA_DIST += gtk+-2.x/ctkdisplayconfig.h
GTK_EXTRA_DIST += gtk+-2.x/ctkscreen.h
GTK_EXTRA_DIST += gtk+-2.x/ctkslimm.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgpu.h
GTK_EXTRA_DIST += gtk+-2.x/ctkbanner.h
GTK_EXTRA_DIST += gtk+-2.x/ctkvcs.h
GTK_EXTRA_DIST += gtk+-2.x/ctkdisplayconfig-utils.h
GTK_EXTRA_DIST += gtk+-2.x/ctkpowersavings.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgvo-banner.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgvo-sync.h
GTK_EXTRA_DIST += gtk+-2.x/ctkgvi.h
GTK_EXTRA_DIST += gtk+-2.x/ctklicense.h
GTK_EXTRA_DIST += gtk+-2.x/ctkecc.h
GTK_EXTRA_DIST += gtk+-2.x/ctkappprofile.h
GTK_EXTRA_DIST += gtk+-2.x/ctkapcprofilemodel.h
GTK_EXTRA_DIST += gtk+-2.x/ctkapcrulemodel.h
GTK_EXTRA_DIST += gtk+-2.x/ctkcolorcontrols.h
GTK_EXTRA_DIST += gtk+-2.x/ctk3dvisionpro.h
GTK_EXTRA_DIST += gtk+-2.x/ctkvdpau.h

NVIDIA_SETTINGS_EXTRA_DIST += $(GTK_EXTRA_DIST)

#
# files in the src/jansson directory of nvidia-settings
#
JANSSON_SRC += jansson/dump.c
JANSSON_SRC += jansson/error.c
JANSSON_SRC += jansson/hashtable.c
JANSSON_SRC += jansson/hashtable_seed.c
JANSSON_SRC += jansson/load.c
JANSSON_SRC += jansson/memory.c
JANSSON_SRC += jansson/pack_unpack.c
JANSSON_SRC += jansson/strbuffer.c
JANSSON_SRC += jansson/strconv.c
JANSSON_SRC += jansson/utf.c
JANSSON_SRC += jansson/value.c

NVIDIA_SETTINGS_SRC += $(JANSSON_SRC)

JANSSON_EXTRA_DIST += jansson/hashtable.h
JANSSON_EXTRA_DIST += jansson/jansson_config.h
JANSSON_EXTRA_DIST += jansson/jansson.h
JANSSON_EXTRA_DIST += jansson/jansson_private_config.h
JANSSON_EXTRA_DIST += jansson/jansson_private.h
JANSSON_EXTRA_DIST += jansson/lookup3.h
JANSSON_EXTRA_DIST += jansson/strbuffer.h
JANSSON_EXTRA_DIST += jansson/utf.h

NVIDIA_SETTINGS_EXTRA_DIST += $(JANSSON_EXTRA_DIST)

NVIDIA_SETTINGS_DIST_FILES += $(NVIDIA_SETTINGS_SRC)
NVIDIA_SETTINGS_DIST_FILES += $(NVIDIA_SETTINGS_EXTRA_DIST)
