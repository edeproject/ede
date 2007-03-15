
include ../../makeinclude

appsdir = $(datadir)/ede/programs

Rootdir           = $(appsdir)
Applicationsdir   = $(appsdir)/Applications
Developmentdir	  = $(appsdir)/Development
Graphicsdir       = $(appsdir)/Graphics
Systemdir         = $(appsdir)/System
Utilitiesdir      = $(appsdir)/Utilities
Multimediadir     = $(appsdir)/Multimedia
Internetdir       = $(appsdir)/Internet
Gamesdir          = $(appsdir)/Games
Settingsdir       = $(appsdir)/Settings

Directory_files = \
	Root.directory		\
	Applications.directory	\
	Graphics.directory	\
	System.directory	\
	Utilities.directory	\
	Multimedia.directory	\
	Internet.directory	\
	Games.directory		\
	Development.directory   \
	Settings.directory

Multimedia_DATA = \
	alsaplayer.desktop \
	ecdplayer.desktop \
	esoundmixer.desktop \
	mplayer.desktop \
	realplayer.desktop \
	xmms.desktop \
	x11amp.desktop


Utilities_DATA = \
	calc.desktop \
	finder.desktop \
	fstabedit.desktop \
	sccalc.desktop \
	tips.desktop \
	xmag.desktop 

Applications_DATA = \
	abiword.desktop \
	Emacs.desktop \
	lyx.desktop \
	notepad.desktop \
	ooffice.desktop \
	Vim.desktop \
	WordPerfect.desktop \
	Xemacs.desktop 

Graphics_DATA = \
	acroread.desktop \
	eimgviewer.desktop \
	GIMP.desktop \
	gv.desktop \
	xdvi.desktop \
	xpdf.desktop \
	xsane.desktop 

System_DATA = \
	Eterm.desktop \
	Kterm.desktop \
	UNIX.desktop \
	UNIX-regular.desktop \
	Vnterm.desktop \
	rxvt.desktop

Settings_DATA =	\
	colorsconf.desktop \
	control.desktop \
	edisplayconf.desktop \
	ehardware.desktop \
	eiconconf.desktop \
	epanelconf.desktop \
	etimedate.desktop \
	ewmconf.desktop \
	printers.desktop \
	scrsaver.desktop


Internet_DATA = \
	dialup.desktop \
	lynx.desktop \
	Mozilla.desktop \
	Netscape.desktop \
	Opera.desktop \
	postoffice.desktop

Development_DATA = \
	cooledit.desktop \
	fluid.desktop \
	xwpe.desktop \
	xxgdb.desktop 

Root_DATA = 

Games_DATA = \
	Casino.desktop \
	lbreak.desktop \
	Mastermind.desktop \
	Qubix.desktop \
	xbill.desktop 


# simple function for easier maintenance
# $(1) is a directory
# $(2) is a list of files

INSTALL_LOOP=\
	$(MKINSTALLDIRS) $(1);\
	for f in $(2); do\
		$(INSTALL_DATA) $$f $(1);\
	done

install:
	$(call INSTALL_LOOP, $(Applicationsdir), $(Applications_DATA))
	$(call INSTALL_LOOP, $(Graphicsdir), $(Graphics_DATA))
	$(call INSTALL_LOOP, $(Utilitiesdir), $(Utilities_DATA))
	$(call INSTALL_LOOP, $(Systemdir), $(System_DATA))
	$(call INSTALL_LOOP, $(Multimediadir), $(Multimedia_DATA))
	$(call INSTALL_LOOP, $(Internetdir), $(Internet_DATA))
	$(call INSTALL_LOOP, $(Gamesdir), $(Games_DATA))
	$(call INSTALL_LOOP, $(Settingsdir), $(Settings_DATA))
	$(call INSTALL_LOOP, $(Rootdir), $(Root_DATA))
	$(call INSTALL_LOOP, $(Developmentdir), $(Development_DATA))

	$(INSTALL_DATA) Root.directory $(Rootdir)/.directory
	$(INSTALL_DATA) Applications.directory $(Applicationsdir)/.directory
	$(INSTALL_DATA) Graphics.directory $(Graphicsdir)/.directory
	$(INSTALL_DATA) System.directory $(Systemdir)/.directory
	$(INSTALL_DATA) Utilities.directory $(Utilitiesdir)/.directory
	$(INSTALL_DATA) Multimedia.directory $(Multimediadir)/.directory
	$(INSTALL_DATA) Internet.directory $(Internetdir)/.directory
	$(INSTALL_DATA) Games.directory $(Gamesdir)/.directory
	$(INSTALL_DATA) Settings.directory $(Settingsdir)/.directory
	$(INSTALL_DATA) Development.directory $(Developmentdir)/.directory


uninstall:
	$(RM) -r $(Applicationsdir)
	$(RM) -r $(Graphicsdir)
	$(RM) -r $(Utilitiesdir)
	$(RM) -r $(Systemdir)
	$(RM) -r $(Multimediadir)
	$(RM) -r $(Internetdir)
	$(RM) -r $(Gamesdir)
	$(RM) -r $(Settingsdir)
	$(RM) -r $(Rootdir)
	$(RM) -r $(Developmentdir)

clean:
