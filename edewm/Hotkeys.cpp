// Hotkeys.cpp
// If you want to change what the hotkeys are, see the table at the bottom!

#include "config.h"

#include "Frame.h"
#include "Windowmanager.h"
#include "Desktop.h"

#include <efltk/Fl_Util.h>

#include "debug.h"

extern void show_tabmenu(int dir);

// Minimize presently active window
static void MinimizeWindow(char*)
{ 
	Frame *act = Frame::activeFrame();
	int acttype = act->window_type();
	
	if ((acttype != TYPE_DESKTOP) && (acttype != TYPE_DOCK) && (acttype != TYPE_SPLASH))
		act->iconize();
}

// Maximize presently active window
static void MaximizeWindow(char*)
{ 
	Frame *act = Frame::activeFrame();
	int acttype = act->window_type();
	
	if ((acttype != TYPE_DESKTOP) && (acttype != TYPE_DOCK) && (acttype != TYPE_SPLASH)) {
//		act->iconize();
		// FIXME: a function for maximize operation like Frame::iconize()
		Fl_Button *tmp; // this will never be used
		act->cb_button_max(tmp);
	}
}

// Close presently active window
static void CloseWindow(char *name)
{ 
	Frame *act = Frame::activeFrame();
	int acttype = act->window_type();
	
	if ((acttype != TYPE_DESKTOP) && (acttype != TYPE_DOCK) && (acttype != TYPE_SPLASH))
		act->close();
}

// Alt+Tab
static void NextWindow(char *name)
{ 
	show_tabmenu(1);
}

// Alt+Shift+Tab
static void PreviousWindow(char *name) {
	show_tabmenu(-1);
}

// Ctrl+Alt+Right
static void NextDesk(char *name)
{
	Desktop::next();
}

// Ctrl+Alt+Left
static void PreviousDesk(char *name)
{
	Desktop::prev();
}

// warning: this assummes it is bound to Fn key:
static void FKey_DeskNumber(char *name)
{
	int desktop = 1; //Fl::event_key()-0xffbd; // default (bad) way of finding the desktop number
					      // kept for backwards compatibility only

	// new (good) way of finding out the desktop number.
	// "name" is something like "Desktop1" and we
	// want to find the "1" on the end
	if (name) {
	   int val;
	   if (sscanf(name,"Desktop%d",&val) == 1) desktop = val;
	   }

	Desktop::current(Desktop::desktop(desktop));
}

static void FastRun(char *name)
{
	fl_start_child_process("elauncher",false);
}

static void FindUtil(char *name)
{
	fl_start_child_process("efinder",false);
}

static void LaunchApp(char *name)
{
        Fl_Config conf(fl_find_config_file("wmanager.conf", false));
        Fl_String app;
        conf.get("Applications", name, app, NULL);
        if(! app.empty())
           fl_start_child_process(app, false);
}

////////////////////////////////////////////////////////////////
// I have to take some time to check it out
static struct {
	char *name;
	int key;
	void (*func)(char *arg);
} keybindings[100]; // hopefully this is enough ;)

// This function will parse the hotkeys string used in config file,
// and construct an integer used by Fl::test_shortcut(int)
int parse_hotkey(char *hotkey)
{
	// We do not have *all* possible keys here
	// (see efltk/efltk/Enumerations.h for more)
	static struct {
		char *name;
		int value;
	} hotkeys[] = {
		{"alt",		FL_ALT},
		{"ctrl",	FL_CTRL},
		{"shift",	FL_SHIFT},
		{"win",		FL_WIN},
		{"space",	FL_Space},
		{"backspace",	FL_BackSpace},
		{"tab",		FL_Tab},
		{"enter",	FL_Enter},
		{"escape",		FL_Escape},
		{"home",	FL_Home},
		{"left",	FL_Left},
		{"up",		FL_Up},
		{"right",	FL_Right},
		{"down",	FL_Down},
		{"pageup",		FL_Page_Up},
		{"pagedown",	FL_Page_Down},
		{"end",		FL_End},
		{"insert",		FL_Insert},
		{"delete",		FL_Delete},
		{"f1",		FL_F(1)},
		{"f2",		FL_F(2)},
		{"f3",		FL_F(3)},
		{"f4",		FL_F(4)},
		{"f5",		FL_F(5)},
		{"f6",		FL_F(6)},
		{"f7",		FL_F(7)},
		{"f8",		FL_F(8)},
		{"f9",		FL_F(9)},
		{"f10",		FL_F(10)},
		{"f11",		FL_F(11)},
		{"f12",		FL_F(12)},
		{0, 0}
	};
	int parsed = 0;
	char f[20];

	// The parser - case insensitive and hopefully robust
	Fl_String_List elements(hotkey, "+");
	for (uint i=0; i<elements.count(); i++) {
		bool found = false;
		for (int j=0; hotkeys[j].value; j++) {
			Fl_String buf = Fl_String(hotkeys[j].name);
			if (!elements.item(i).casecmp(buf)) {
				parsed += hotkeys[j].value;
				found = true;
			}
		}

		if (!found) {
		// use first letter or number as shortcut key
			strcpy(f, elements.item(i));
			if ((f[0] >= 'a') && (f[0] <= 'z')) {
				parsed += f[0];
			} else if ((f[0] >= 'A') && (f[0] <= 'Z')) {
				parsed += (f[0] - 'A' + 'a');
			} else if ((f[0] >= '0') && (f[0] <= '9')) {
				parsed += f[0];
			}
		}
	}

	return parsed;
}

// Read wmanager.conf and initialize an array that will be used later
void read_hotkeys_configuration()
{
	// All configurable hotkeys below - edit to add new keys
	static struct {
		char *name;	// as used in wmanager.conf file
		void (*func)(char *name);	// callback function for this key
		char *def_key;	// default hotkey
	} configurables[] = {
		{"PreviousWindow",	PreviousWindow,	"Alt+Tab"},
		{"NextWindow",	NextWindow,	"Alt+Shift+Tab"},
		{"Desktop1",	FKey_DeskNumber,	"Alt+F1"},
		{"Desktop2",	FKey_DeskNumber,	"Alt+F2"},
		{"Desktop3",	FKey_DeskNumber,	"Alt+F3"},
		{"Desktop4",	FKey_DeskNumber,	"Alt+F4"},
		{"Desktop5",	FKey_DeskNumber,	"Alt+F5"},
		{"Desktop6",	FKey_DeskNumber,	"Alt+F6"},
		{"Desktop7",	FKey_DeskNumber,	"Alt+F7"},
		{"Desktop8",	FKey_DeskNumber,	"Alt+F8"},
		{"PreviousDesktop",	PreviousDesk,	"Alt+Ctrl+Right"},
		{"NextDesktop",	NextDesk,	"Alt+Ctrl+Left"},
		{"FindUtil",	FindUtil,	"Ctrl+F3"},
		{"FastRun",	FastRun,	"Ctrl+F12"},
		{"CloseWindow",	CloseWindow,	"Ctrl+F4"},
		{"MinimizeWindow",	MinimizeWindow,	"Ctrl+F7"},
		{"MaximizeWindow",	MaximizeWindow,	"Ctrl+F8"},
 
		// Slots for user-defined applications and hotkeys
		// 12 ought to be enough :-)
		{"App1",LaunchApp,""},
		{"App2",LaunchApp,""},
		{"App3",LaunchApp,""},
		{"App4",LaunchApp,""},
		{"App5",LaunchApp,""},
		{"App6",LaunchApp,""},
		{"App7",LaunchApp,""},
		{"App8",LaunchApp,""},
		{"App9",LaunchApp,""},
		{"App10",LaunchApp,""},
		{"App11",LaunchApp,""},
		{"App12",LaunchApp,""},
		
		{"",NULL,""}
	};

	char buf[256];
	int j=0;

	Fl_Config wmconf(fl_find_config_file ("wmanager.conf",0));
	wmconf.set_section("Hotkeys");
	
	for (int i=0; configurables[i].name[0]; i++) {
		wmconf.read(configurables[i].name, buf, configurables[i].def_key, sizeof(buf));
		if (buf && buf[0]) {
		   keybindings[j].key=parse_hotkey(buf);
		   keybindings[j].name = strdup(configurables[i].name);
		   keybindings[j++].func=configurables[i].func;
		   }
	}
	keybindings[j].key = 0;
}

int Handle_Hotkey() {
	for (int i = 0; keybindings[i].key; i++) {
		if (Fl::test_shortcut(keybindings[i].key) ||
			(keybindings[i].key & 0xFFFF) == FL_Delete
			&& Fl::event_key() == FL_BackSpace// fltk bug?
		   ) {
			keybindings[i].func(keybindings[i].name);
			return 1;
		}
	}
	return 0;
}

void Grab_Hotkeys()
{
	Window root_w = fl_xid(root);
	for (int i = 0; keybindings[i].key; i++) {
		int k = keybindings[i].key;
		int keycode = XKeysymToKeycode(fl_display, k & 0xFFFF);
		if(!keycode) continue;
		// Silly X!  we need to ignore caps lock & numlock keys by grabbing
		// all the combinations:
		XGrabKey(fl_display, keycode, k>>16,	 root_w, 0, 1, 1);
		XGrabKey(fl_display, keycode, (k>>16)|2, root_w, 0, 1, 1); // CapsLock
		XGrabKey(fl_display, keycode, (k>>16)|16, root_w, 0, 1, 1); // NumLock
		XGrabKey(fl_display, keycode, (k>>16)|18, root_w, 0, 1, 1); // both
	}
}
