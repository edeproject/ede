
CPPFILES = econtrol.cpp ../edelib2/Util.cpp ../edelib2/Icon.cpp ../edelib2/Config.cpp ../edelib2/Run.cpp ../edelib2/process.cpp ../edelib2/pty.cpp ../edelib2/about_dialog.cpp
TARGET   = econtrol

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

