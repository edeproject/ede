#
# $Id$
#
# Part of Equinox Desktop Environment (EDE).
# Copyright (c) 2000-2006 EDE Authors.
#
# This program is licenced under terms of the 
# GNU General Public Licence version 2 or newer.
# See COPYING for details.

CPPFILES = aboutdialog.cpp\
		   logoutdialog.cpp\
		   panelbutton.cpp\
		   keyboardchooser.cpp\
		   taskbutton.cpp\
		   workpanel.cpp\
		   item.cpp\
		   cpumonitor.cpp\
		   batterymonitor.cpp\
		   dock.cpp\
		   mainmenu.cpp\
		   mainmenu_scan.cpp

TARGET   = eworkpanel

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

