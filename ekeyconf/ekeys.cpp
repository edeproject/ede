/*
 * $Id$
 *
 * Keyboard shortcuts applet
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2005-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "ekeys.h"


using namespace fltk;
using namespace edelib;


#define	NR_HOTKEYS	30


static struct {
	char systemname[20];
	char uiname[50];
	char command[50];
} hotkeys[] = {
	{"NextWindow",		"Next window", ""},
	{"PreviousWindow",	"Previous window", ""},
	{"NextDesktop",		"Next workspace", ""},
	{"PreviousDesktop",	"Previous workspace", ""},
	{"FastRun",		"Run program", ""},
	{"FindUtil",		"Find file", ""},
	{"CloseWindow",		"Close window", ""},
	{"MinimizeWindow",	"Minimize window", ""},
	{"MaximizeWindow",	"Maximize window", ""},
	{"Desktop1",		"Workspace 1", ""},
	{"Desktop2",		"Workspace 2", ""},
	{"Desktop3",		"Workspace 3", ""},
	{"Desktop4",		"Workspace 4", ""},
	{"Desktop5",		"Workspace 5", ""},
	{"Desktop6",		"Workspace 6", ""},
	{"Desktop7",		"Workspace 7", ""},
	{"Desktop8",		"Workspace 8", ""},
	{"App1",		"", ""},
	{"App2",		"", ""},
	{"App3",		"", ""},
	{"App4",		"", ""},
	{"App5",		"", ""},
	{"App6",		"", ""},
	{"App7",		"", ""},
	{"App8",		"", ""},
	{"App9",		"", ""},
	{"App10",		"", ""},
	{"App11",		"", ""},
	{"App12",		"", ""},

};



int keycodes[NR_HOTKEYS];




static void sendClientMessage(XWindow w, Atom a, long x)
{
	XEvent ev;
	long mask;
	
	memset(&ev, 0, sizeof(ev));
	ev.xclient.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.message_type = a;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = x;
	ev.xclient.data.l[1] = CurrentTime;
	mask = 0L;
	if (w == RootWindow(xdisplay, xscreen))
		mask = SubstructureRedirectMask;
	XSendEvent(xdisplay, w, False, mask, &ev);
}

void sendUpdateInfo() 
{
// No worky
/*	unsigned int i, nrootwins;
	XWindow dw1, dw2, *rootwins = 0;
	int screen_count = ScreenCount(fltk::xdisplay);
	extern Atom FLTKChangeSettings;
	for (int s = 0; s < screen_count; s++) {
		XWindow root = RootWindow(fltk::xdisplay, s);
		XQueryTree(fltk::xdisplay, root, &dw1, &dw2, &rootwins, &nrootwins);
		for (i = 0; i < nrootwins; i++) {
			if (rootwins[i]!=RootWindow(fltk::xdisplay, fltk::xscreen)) {
				sendClientMessage(rootwins[i], FLTKChangeSettings, 0);
			}
		}
	}
	XFlush(fltk::xdisplay);*/
}

int getshortcutfor(const char* action)
{
	for (int i=0; i<NR_HOTKEYS; i++)
		if (strcmp(action,hotkeys[i].uiname) == 0) return keycodes[i];
	
	return 0;
}

void setshortcutfor(const char* action, int svalue)
{
	for (int i=0; i<NR_HOTKEYS; i++)
		if (strcmp(action,hotkeys[i].uiname) == 0) keycodes[i] = svalue;
}

int name_to_svalue(char *hotkey)
{
	static struct {
		char *name;
		int value;
	} keys[] = {
		{"alt",  	ALT},
		{"ctrl",  	CTRL},
		{"shift",  	SHIFT},
		{"win",  	META},
		{"space",  	SpaceKey},
		{"backspace",	BackSpaceKey},
		{"tab",  	TabKey},
		{"enter",  	ReturnKey},
		{"escape",  	EscapeKey},
		{"home",  	HomeKey},
		{"left",  	LeftKey},
		{"up",  	UpKey},
		{"right",  	RightKey},
		{"down",  	DownKey},
		{"pageup",  	PageUpKey},
		{"pagedown",  	PageDownKey},
		{"end",  	EndKey},
		{"insert",  	InsertKey},
		{"delete",  	DeleteKey},
		{"f1",  	F1Key},
		{"f2",  	F2Key},
		{"f3",  	F3Key},
		{"f4",  	F4Key},
		{"f5",  	F5Key},
		{"f6",  	F6Key},
		{"f7",  	F7Key},
		{"f8",  	F8Key},
		{"f9",  	F9Key},
		{"f10",  	F10Key},
		{"f11",  	F11Key},
		{"f12",  	F12Key},
		{0, 0}
	};
	int parsed = 0;
	char f[20];

	// The parser - case insensitive and hopefully robust
	int plus=0;
	for (int i=0; i<=strlen(hotkey); i++) {
		if (i<strlen(hotkey) && hotkey[i] != '+') continue;
		if (i==plus) { plus=i+1; continue; } // two +s in row
		char key[20];
		strncpy (key, hotkey+plus, i-plus);
		key[i-plus]='\0';
		plus=i+1;
		
		bool found = false;
		for (int j=0; keys[j].value; j++) {
			if (strcasecmp(key,keys[j].name) == 0) {
				parsed += keys[j].value;
				found = true;
			}
		}

		if (!found) {
		// use first letter as shortcut key
			if ((key[0] >= 'a') && (key[0] <= 'z')) {
				parsed += key[0];
			} else if ((key[0] >= 'A') && (key[0] <= 'Z')) {
				parsed += (key[0] - 'A' + 'a');
			}
		}
	}

	return parsed;
}

void readKeysConfiguration()
{
	Config globalConfig(Config::find_file("wmanager.conf", 0), true, false);
	globalConfig.set_section("Hotkeys");
	
	for (int i=0; i<NR_HOTKEYS; i++) {
		char tmp[128];
		globalConfig.read(hotkeys[i].systemname, tmp, "", 128);
		keycodes[i] = name_to_svalue(tmp);
	}

	globalConfig.set_section("Applications");
	for (int i=0; i<NR_HOTKEYS; i++) {
		char tmp[128];
		if ((strncmp(hotkeys[i].systemname,"App",3) == 0) && (keycodes[i] != 0)) {
			globalConfig.read(hotkeys[i].systemname, tmp, "", 128);
			if (tmp != "") strncpy(hotkeys[i].command, tmp, 50);
			if (keycodes[i]>0 && tmp != "") strncpy(hotkeys[i].uiname, hotkeys[i].systemname, 20);
		}
	}

	globalConfig.set_section("ApplicationNames");
	for (int i=0; i<NR_HOTKEYS; i++) {
		char tmp[128];
		if ((strncmp(hotkeys[i].systemname,"App",3) == 0) && (keycodes[i] != 0)) {
			globalConfig.read(hotkeys[i].systemname, tmp, "", 128);
			if (tmp != "") strncpy(hotkeys[i].uiname, tmp, 50);
		}
	}




}

void writeKeysConfiguration()
{
	Config globalConfig(Config::find_file("wmanager.conf", true));
	globalConfig.set_section("Hotkeys");
	
	for (int i=0; i<NR_HOTKEYS; i++)
		globalConfig.write(hotkeys[i].systemname, key_name(keycodes[i]));
	
	globalConfig.set_section("Applications");
	for (int i=0; i<NR_HOTKEYS; i++)
		if ((strncmp(hotkeys[i].systemname,"App",3) == 0) 
			&& (strcmp(hotkeys[i].uiname,"") != 0)  
			&& (strcmp(hotkeys[i].command,"") != 0))
				globalConfig.write(hotkeys[i].systemname, hotkeys[i].command);

	globalConfig.set_section("ApplicationNames");
	for (int i=0; i<NR_HOTKEYS; i++)
		if ((strncmp(hotkeys[i].systemname,"App",3) == 0) 
			&& (strcmp(hotkeys[i].uiname,"") != 0)  
			&& (strcmp(hotkeys[i].command,"") != 0))
				globalConfig.write(hotkeys[i].systemname, hotkeys[i].uiname);
}

void populatelist(InputBrowser *action) 
{
	action->clear(); // Rewrite?
	for (int i=0; i<NR_HOTKEYS; i++)
		if (strcmp(hotkeys[i].uiname,"") != 0) action->add ( hotkeys[i].uiname);
}

void addShortcut(const char *name, const char *cmd)
{
	if ((strcmp(name,"") !=0) && (strcmp(cmd,"") != 0)) {
		for (int i=0; i<NR_HOTKEYS; i++) {
			if ((strncmp(hotkeys[i].systemname,"App",3) == 0) && (strcmp(hotkeys[i].uiname,name) == 0)) {
				alert(_("Shortcut already defined! Please use a different name"));
				return;
			}
		}
		for (int i=0; i<NR_HOTKEYS; i++) {
			if ((strncmp(hotkeys[i].systemname,"App",3) == 0) && (strcmp(hotkeys[i].uiname,"") == 0)) {
				strncpy(hotkeys[i].uiname, name, 50);
				strncpy(hotkeys[i].command, cmd, 50);
				return;
			}
		}
	}
	alert(_("Maximum number of user shortcuts exceeded"));
}


void removeShortcut(const char* action)
{
	for (int i=0; i<NR_HOTKEYS; i++)
		if (strcmp(action,hotkeys[i].uiname)==0) {
			keycodes[i]=0;
			if (strncmp(hotkeys[i].systemname,"App",3) == 0)
				strcpy(hotkeys[i].uiname,"");
		}
}
