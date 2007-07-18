#
# $Id$
#
# Part of Equinox Desktop Environment (EDE).
# Copyright (c) 2000-2006 EDE Authors.
#
# This program is licenced under terms of the 
# GNU General Public Licence version 2 or newer.
# See COPYING for details.

CPPFILES = \
		   debug.cpp         \
		   main.cpp          \
		   Windowmanager.cpp \
		   Hints.cpp         \
		   Atoms.cpp         \
		   Titlebar.cpp      \
		   Frame.cpp         \
		   Cursor.cpp        \
		   Events.cpp        \
		   Sound.cpp         \
		   Utils.cpp

TARGET   = edewm

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

archive:
	DATE=`date +%d%m%Y`; \
	cd ..; \
	tar -cjpvf edewm-unfinished-$$DATE.tar.bz2 edewm
