include ../../makeinclude

Data = ede-small.gif\
	   index.html\
	   authors.html\
	   changelog.html\
	   copying.html\
	   help.html\
	   logoinvert100.png\
	   getede.png\
	   ede.png\
	   style.css

DIR = $(datadir)/ede/doc

install:
	echo "Installing docs";
	$(MKINSTALLDIRS) $(DIR)
	for f in $(Data); do\
		$(INSTALL_DATA) $$f $(DIR);\
	done

uninstall:
	$(RM) -r $(DIR)

clean:
