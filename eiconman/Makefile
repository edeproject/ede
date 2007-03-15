
CPPFILES = eiconman.cpp edeskicon.cpp propdialog.cpp edeskconf.cpp ../edelib2/EDE_Config.cpp ../edelib2/EDE_Run.cpp ../edelib2/pty.cpp ../edelib2/process.cpp
TARGET   = eiconman

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

