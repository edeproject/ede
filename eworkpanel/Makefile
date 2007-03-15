
CPPFILES = aboutdialog.cpp logoutdialog.cpp panelbutton.cpp keyboardchooser.cpp taskbutton.cpp workpanel.cpp item.cpp cpumonitor.cpp dock.cpp mainmenu.cpp mainmenu_scan.cpp
TARGET   = eworkpanel

POFILES = locale/ru.po\
			locale/sr.po\
			locale/sk.po\
			locale/hu.po\

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

