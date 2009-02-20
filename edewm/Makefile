#
# $Id: Makefile 1957 2007-06-25 19:42:47Z vljubovic $
#
# Part of Equinox Desktop Environment (EDE).
# Copyright (c) 2000-2006 EDE Authors.
#
# This program is licenced under terms of the 
# GNU General Public Licence version 2 or newer.
# See COPYING for details.

CPPFILES = WMWindow.cpp\
			Theme.cpp\
			Netwm.cpp\
			Mwm.cpp\
			Icccm.cpp\
			Icon.cpp\
			Cursor.cpp\
			Desktop.cpp\
			Events.cpp\
			Frame.cpp\
			Hotkeys.cpp\
			Tabmenu.cpp\
			Titlebar.cpp\
			Windowmanager.cpp\
			Winhints.cpp\
			main.cpp

TARGET   = edewm

POFILES = locale/ru.po\
			locale/sr.po\
			locale/sk.po\
			locale/hu.po\
			locale/de.po\
			locale/fr.po

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

