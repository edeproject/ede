
CPPFILES = etip.cpp ../edelib2/Util.cpp ../edelib2/Config.cpp
TARGET   = etip

POFILES = locale/ru.po\
			locale/sr.po\
			locale/sk.po\

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

