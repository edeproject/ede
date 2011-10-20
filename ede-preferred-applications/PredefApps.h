#ifndef __EDE_PREFERRED_APPLICATIONS_PREDEFAPPS_H__
#define __EDE_PREFERRED_APPLICATIONS_PREDEFAPPS_H__

#include "Apps.h"

static KnownApp app_browsers[] = {
	KNOWN_APP_START,
	{"Mozilla Firefox", "firefox"},
	{"Mozilla Seamonkey", "seamonkey"},
	{"Google Chrome", "google-chrome"},
	{"Midori", "midori"},
	{"Konqueror", "konqueror"},
	{"Dillo", "dillo"},
	KNOWN_APP_END
};

static KnownApp app_mails[] = {
	KNOWN_APP_START,
	{"Mozilla Thunderbird", "thunderbird"},
	KNOWN_APP_END
};

static KnownApp app_filemanagers[] = {
	KNOWN_APP_START,
	{"Thunar", "thunar"},
	{"Nautilus", "nautilus"},
	{"Dolphin", "dolphin"},
	{"Konqueror", "konqueror"},
	KNOWN_APP_END
};

static KnownApp app_terminals[] = {
	KNOWN_APP_START,
	{"X11 terminal", "xterm"},
	{"Rxvt", "rxvt"},
	{"Mrxvt", "mrxvt"},
	{"Xfce Terminal", "xfterm4"},
	KNOWN_APP_END
};

#endif
