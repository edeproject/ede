include ../../makeinclude

desktopdir = $(datadir)/ede/.ede/desktop

DESKTOPLINKS     = \
	Terminal.desktop

install:
	$(MKINSTALLDIRS) $(desktopdir)
	$(INSTALL_DATA) $(DESKTOPLINKS) $(desktopdir)

uninstall:
	$(RM) -r $(desktopdir)

clean:
