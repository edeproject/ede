
CPPFILES = ../edelib2/Util.cpp ../edelib2/Config.cpp ekeys.cpp ekeyconf.cpp Shortcut_Button.cpp
TARGET   = ekeyconf

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

