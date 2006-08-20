
CPPFILES = evolume.cpp prefs.cpp ../edelib2/about_dialog.cpp ../edelib2/Util.cpp ../edelib2/Config.cpp ../edelib2/process.cpp ../edelib2/pty.cpp ../edelib2/Run.cpp
TARGET   = evolume


POFILES = locale/ru.po\
		  locale/sk.po\
		  locale/hu.po

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

