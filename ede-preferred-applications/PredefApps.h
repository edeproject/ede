#ifndef __PREDEFAPPS_H__
#define __PREDEFAPPS_H__

#include <edelib/Nls.h>

struct KnownApp {
	const char *name;
	const char *cmd;
};

#define KNOWN_APP_END {0, 0} 

/* to allow inclusion from single place, without issuing gcc warnings */
#if KNOWN_APP_PREDEFINED
static KnownApp app_browsers[] = {
	{"Mozilla Firefox", "firefox"},
	{"Mozilla Seamonkey", "seamonkey"},
	{"Google Chrome", "google-chrome"},
	{"Midori", "midori"},
	{"Konqueror", "konqueror"},
	{"Dillo", "dillo"},
	KNOWN_APP_END
};

static KnownApp app_mails[] = {
	{"Mozilla Thunderbird", "thunderbird"},
	{"Evolution", "evolution"},
	{"Claws Mail", "claws-mail"},
	KNOWN_APP_END
};

static KnownApp app_filemanagers[] = {
	{"Thunar", "thunar"},
	{"Nautilus", "nautilus"},
	{"Dolphin", "dolphin"},
	{"Konqueror", "konqueror"},
	KNOWN_APP_END
};

static KnownApp app_terminals[] = {
	{"X11 terminal", "xterm"},
	{"Rxvt", "rxvt"},
	{"Rxvt Unicode", "urxvt"},
	{"Mrxvt", "mrxvt"},
	{"Small Terminal", "st"},
	{"Xfce Terminal", "xfterm4"},
	{"GNOME Terminal", "gnome-terminal"},
	KNOWN_APP_END
};
#endif

#endif
