# Copyright (c) 2008 NVIDIA, Corporation
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


##############################################################################
# include common variables and functions
##############################################################################

include utils.mk


##############################################################################
# assign variables
##############################################################################

LIBXNVCTRL = libXNVCtrl.a

LIBXNVCTRL_PROGRAM_NAME = "libXNVCtrl"

LIBXNVCTRL_VERSION := $(NVIDIA_VERSION)

X_CFLAGS ?=

CFLAGS += $(X_CFLAGS)
CFLAGS += -I .
CFLAGS += -fPIC
CFLAGS += -I $(OUTPUTDIR)
CFLAGS += -DPROGRAM_NAME=\"$(LIBXNVCTRL_PROGRAM_NAME)\"

SRC += NVCtrl.c

OBJS = $(call BUILD_OBJECT_LIST,$(SRC))


##############################################################################
# build rules
##############################################################################

.PHONY: clean

all: $(LIBXNVCTRL)

$(LIBXNVCTRL) : $(OBJS)
	$(AR) ru $@ $(OBJS)

# define the rule to build each object file
$(foreach src,$(SRC),$(eval $(call DEFINE_OBJECT_RULE,TARGET,$(src))))

clean:
	rm -rf $(LIBXNVCTRL) *~ \
		$(OUTPUTDIR)/*.o $(OUTPUTDIR)/*.d
