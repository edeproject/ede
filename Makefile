#
# $Id: Makefile 1667 2006-06-14 16:28:31Z karijes $
#
# Part of Equinox Desktop Environment (EDE).
# Copyright (c) 2007 EDE Authors.
#
# This program is licensed under terms of the 
# GNU General Public License version 2 or newer.
# See COPYING for details.

JAM_PATH=

define jam-cmd
	@if [ -z $(JAM_PATH) ]; then                       \
		if [ -x ./jam ]; then                          \
			jam_path="./jam";                          \
		else                                           \
			jam_path=`which jam`;                      \
		fi;                                            \
	else                                               \
		jam_path=$(JAM_PATH);                          \
	fi;                                                \
                                                       \
	if [ -z $$jam_path ] || [ ! -x $$jam_path ];  then \
		echo "";                                       \
		echo "*** Unable to find jam either in `pwd`"; \
		echo "*** or in $$PATH";                       \
		echo "";                                       \
	else                                               \
		$$jam_path $1;                                 \
	fi
endef

all:
	$(call jam-cmd)

.PHONY: clean 

install:
	$(call jam-cmd, install)

uninstall:
	$(call jam-cmd, uninstall)

clean:
	$(call jam-cmd, clean)
