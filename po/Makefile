#
# nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
# and Linux systems.
#
# Copyright (C) 2008-2012 NVIDIA Corporation.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses>.
#


##############################################################################
# include common variables and functions
##############################################################################

UTILS_MK_DIR ?= ..

include $(UTILS_MK_DIR)/utils.mk

##############################################################################
# Localization GUI nvidia-settings
# 
# To add a new translation, run the command:
# msginit --input=nvidia-settings.pot \
#    --locale=LANG \
#    --output=msg/LANG.po
##############################################################################

##############################################################################
# assign variables
##############################################################################

SRC_DIR_LIST		= ../src/gtk+-2.x/
SRC_FILES			= POTFILES.in
MSG_DIR				= msg/
PACKAGE				= nvidia-settings
LANGS_LIST			= $(basename $(notdir $(wildcard $(MSG_DIR)*.po)))
MO_FILES			= $(foreach lang, $(LANGS_LIST), $(MSG_DIR)$(lang).mo)
MO_FILES_install	= $(foreach lang, $(LANGS_LIST), $(LCDIR)/$(lang)/LC_MESSAGES/$(PACKAGE).mo)

##############################################################################
# build rules
##############################################################################

.PNONY: all install clean

all: $(MO_FILES)

install: $(MO_FILES_install)

clean:
	@rm -rf $(MO_FILES) $(SRC_FILES) $(PACKAGE).pot

$(LCDIR)/%/LC_MESSAGES/$(PACKAGE).mo: $(MSG_DIR)%.mo
	$(MKDIR) $(dir $@)
	$(INSTALL) $(INSTALL_PO_ARGS) $< $@

$(MSG_DIR)%.mo: $(MSG_DIR)%.po
	$(MSGFMT) -c -o $@ $<

$(MSG_DIR)%.po: $(PACKAGE).pot
	$(MSGMERGE) --update $@ $<

$(PACKAGE).pot: $(SRC_FILES)
	$(XGETTEXT) --keyword=N_ --keyword=_ --language=C --from-code=utf-8 -c -o $@ -f $(SRC_FILES)

$(SRC_FILES):
	$(FIND) $(SRC_DIR_LIST) -name *.[ch] | sort > $(SRC_FILES)
