#
# NV-CONTROL samples: Sample tools for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (c) 2010 NVIDIA, Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

##############################################################################
# include common variables and functions
##############################################################################

UTILS_MK_DIR ?= ..

include $(UTILS_MK_DIR)/utils.mk

XNVCTRL_DIR ?= ../src/libXNVCtrl

include $(XNVCTRL_DIR)/xnvctrl.mk


##############################################################################
# The calling Makefile may export any of the following variables; we
# assign default values if they are not exported by the caller
##############################################################################

CFLAGS                += $(XNVCTRL_CFLAGS)
CFLAGS                += -I $(XNVCTRL_DIR)
CFLAGS                += -I $(OUTPUTDIR)

LDFLAGS               += $(XNVCTRL_LDFLAGS)
LIBS                  += -lXext -lX11


##############################################################################
# samples
##############################################################################

SAMPLE_SOURCES        += nv-control-info.c
SAMPLE_SOURCES        += nv-control-dvc.c
SAMPLE_SOURCES        += nv-control-events.c
SAMPLE_SOURCES        += nv-control-dpy.c
SAMPLE_SOURCES        += nv-control-targets.c
SAMPLE_SOURCES        += nv-control-framelock.c
SAMPLE_SOURCES        += nv-control-gvi.c
SAMPLE_SOURCES        += nv-control-3dvisionpro.c
SAMPLE_SOURCES        += nv-control-warpblend.c

##############################################################################
# build rules
##############################################################################

.PHONY: all clean clobber install

# define the rule to build each object file
$(foreach src, $(SAMPLE_SOURCES), $(eval $(call DEFINE_OBJECT_RULE,TARGET,$(src))))

# define the rule to link each sample app from its corresponding object file
define link_sample_from_object
  $$(OUTPUTDIR)/$(1:.c=): $$(call BUILD_OBJECT_LIST,$(1)) $$(LIBXNVCTRL)
	$$(call quiet_cmd,LINK) $$(CFLAGS) $$(LDFLAGS) $$(BIN_LDFLAGS) -o $$@ $$< $$(LIBXNVCTRL) $$(LIBS)
  all:: $$(OUTPUTDIR)/$(1:.c=)
  SAMPLES += $$(OUTPUTDIR)/$(1:.c=)
endef

$(foreach sample,$(SAMPLE_SOURCES),$(eval $(call link_sample_from_object,$(sample))))


clean clobber:
	rm -rf *~ $(OUTPUTDIR)/*.o $(OUTPUTDIR)/*.d $(SAMPLES) $(LIBXNVCTRL)

install:
	@# don't install samples, this is just to satisfy the top-level
	@# recursion rule
