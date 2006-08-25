include ../makeinclude

Data = ede.conf wmanager.conf mime.conf efltk.conf mimetypes.conf
DIR = $(datadir)/ede/.ede

install:
	$(MKINSTALLDIRS) $(DIR)
	$(INSTALL_DATA) ede.conf $(DIR)
	$(INSTALL_DATA) wmanager.conf $(DIR)
	$(INSTALL_DATA) mime.conf $(DIR)
	$(INSTALL_DATA) efltk.conf $(DIR)
	$(INSTALL_DATA) mimetypes.conf $(DIR)
	$(MKINSTALLDIRS) $(bindir)
	$(INSTALL_PROGRAM) startede $(bindir)

uninstall:
	$(RM) -r $(DIR)
	$(RM) $(bindir)/startede

clean:
