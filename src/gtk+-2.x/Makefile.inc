#
# nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (C) 2004 NVIDIA Corporation.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of Version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See Version 2
# of the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the:
#
#           Free Software Foundation, Inc.
#           59 Temple Place - Suite 330
#           Boston, MA 02111-1307, USA
#

#
# GTK+ 2.0 sources and extra files to be included in the distribution
#

SRC += \
	ctkxvideo.c \
	ctkcursorshadow.c \
	ctkui.c \
	ctkframelock.c \
	ctkgauge.c \
	ctkcurve.c \
	ctkcolorcorrection.c \
	ctkscale.c \
	ctkmultisample.c \
	ctkconfig.c \
	ctkevent.c \
	ctkwindow.c \
	ctkopengl.c \
	ctkglx.c \
	ctkhelp.c \
	ctkimagesliders.c \
	ctkdisplaydevice-crt.c \
	ctkdisplaydevice-tv.c \
	ctkdisplaydevice-dfp.c \
	ctkthermal.c \
	ctkpowermizer.c \
	ctkgvo.c \
	ctkgvo-csc.c \
	ctkdropdownmenu.c \
	ctkrandr.c \
	ctkclocks.c \
	ctkutils.c \
	ctkedid.c \
	ctkserver.c \
	ctkdisplaylayout.c \
	ctkdisplayconfig.c \
	ctkscreen.c \
	ctkslimm.c \
	ctkgpu.c \
	ctkbanner.c \
	ctkvcs.c \
	ctkdisplayconfig-utils.c \
	ctkgvo-banner.c \
	ctkgvo-sync.c \
	ctkpowersavings.c


EXTRA_DIST += \
	Makefile.inc \
	ctkxvideo.h \
	ctkcursorshadow.h \
	ctkui.h \
	ctkframelock.h \
	ctkgauge.h \
	ctkcurve.h \
	ctkcolorcorrection.h \
	ctkscale.h \
	ctkmultisample.h \
	ctkconfig.h \
	ctkevent.h \
	ctkwindow.h \
	ctkopengl.h \
	ctkglx.h \
	ctkhelp.h \
	ctkimagesliders.h \
	ctkdisplaydevice-crt.h \
	ctkdisplaydevice-tv.h \
	ctkdisplaydevice-dfp.h \
	ctkconstants.h \
	ctkthermal.h \
	ctkpowermizer.h \
	ctkgvo.h \
	ctkgvo-csc.h \
	ctkdropdownmenu.h \
	ctkrandr.h \
	ctkclocks.h \
	ctkutils.h \
	ctkedid.h \
	ctkserver.h \
	ctkdisplaylayout.h \
	ctkdisplayconfig.h \
	ctkscreen.h \
	ctkslimm.h \
	ctkgpu.h \
	ctkbanner.h \
	ctkvcs.h \
	ctkdisplayconfig-utils.h \
	ctkpowersavings.h \
	ctkgvo-banner.h \
	ctkgvo-sync.h \
	ctkpowersavings.h

dist_list::
	@ echo $(SRC) $(EXTRA_DIST)
