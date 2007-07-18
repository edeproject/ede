
CPPFILES = elauncher.cpp ../edelib2/Util.cpp ../edelib2/Config.cpp ../edelib2/process.cpp ../edelib2/pty.cpp
TARGET   = elauncher

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

