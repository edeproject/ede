
CPPFILES = escreensaver.cpp escrsaverconf.cpp ../edelib2/Util.cpp ../edelib2/Config.cpp
TARGET   = esvrconf

POFILES = locale/ru.po\
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

