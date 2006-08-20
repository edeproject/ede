
CPPFILES = efiler.cpp ../edelib2/MimeType.cpp ../edelib2/Run.cpp ../edelib2/process.cpp ../edelib2/pty.cpp ../edelib2/Config.cpp ../edelib2/Icon.cpp ../edelib2/Util.cpp ../edelib2/about_dialog.cpp
TARGET   = efiler

include ../makeinclude

install:
	$(INSTALL_PROGRAM) $(TARGET) $(bindir)
	$(INSTALL_LOCALE)

uninstall:
	$(RM) $(bindir)/$(TARGET)

clean:
	$(RM) $(TARGET)
	$(RM) *.o

