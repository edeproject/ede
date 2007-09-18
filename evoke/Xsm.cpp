/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Xsm.h"
#include <edelib/Debug.h>
#include <FL/x.h>
#include <stdio.h> // snprintf

struct XsmData {
	Window window;
	Atom manager_atom;
	Atom selection_atom;
	Atom xsettings_atom;

	unsigned long serial;
};

struct XsettingsColor {
	unsigned char red, green, blue, alpha;
};

struct XsettingsSetting {
	char* name;
	XsettingsType type;

	union {
		int v_int;
		char* v_string;
		XsettingsColor v_color;
	} data;

	unsigned long last_changed_serial;
};

struct TimeStampInfo {
	Window window;
	Atom timestamp_prop_atom;
};

// Bool (X type) is used since this function is going to XIfEvent()
Bool timestamp_predicate(Display* dpy, XEvent* xev, XPointer arg) {
	TimeStampInfo* info = (TimeStampInfo*)arg;

	if(xev->type == PropertyNotify && 
		xev->xproperty.window == info->window && 
		xev->xproperty.atom == info->timestamp_prop_atom) {
		return True;
	}

	return False;
}

Time get_server_time(Display* dpy, Window win) {
	unsigned char c = 'a';
	TimeStampInfo info;
	XEvent xev;

	info.timestamp_prop_atom = XInternAtom(dpy, "_TIMESTAMP_PROP", False);
	info.window = win;

	XChangeProperty(dpy, win, info.timestamp_prop_atom, info.timestamp_prop_atom,
			8, PropModeReplace, &c, 1);

	XIfEvent(dpy, &xev, timestamp_predicate, (XPointer)&info);

	return xev.xproperty.time;
}


Xsm::Xsm() : data(NULL) { }

Xsm::~Xsm() { 
	if(data) {
		XDestroyWindow(fl_display, data->window);
		delete data;
	}

	puts("Xsm::~Xsm()"); 
}

bool Xsm::is_running(void) {
	char buff[256];

	snprintf(buff, sizeof(buff)-1, "_XSETTINGS_S%d", fl_screen);
	Atom selection = XInternAtom(fl_display, buff, False);

	if(XGetSelectionOwner(fl_display, selection))
		return true;
	return false;
}

bool Xsm::init(void) {
	char buff[256];

	data = new XsmData;

	snprintf(buff, sizeof(buff)-1, "_XSETTINGS_S%d", fl_screen);
	data->selection_atom = XInternAtom(fl_display, buff, False);
	data->xsettings_atom = XInternAtom(fl_display, "_XSETTINGS_SETTINGS", False);
	data->manager_atom = XInternAtom(fl_display, "MANAGER", False);

	data->serial = 0;

	data->window = XCreateSimpleWindow(fl_display, RootWindow(fl_display, fl_screen), 
			0, 0, 10, 10, 0, WhitePixel(fl_display, fl_screen), WhitePixel(fl_display, fl_screen));

	XSelectInput(fl_display, data->window, PropertyChangeMask);
	Time timestamp = get_server_time(fl_display, data->window);

	XSetSelectionOwner(fl_display, data->selection_atom, data->window, timestamp);

	// check if we got ownership
	if(XGetSelectionOwner(fl_display, data->selection_atom) == data->window) {
		XClientMessageEvent xev;

		xev.type = ClientMessage;
		xev.window = RootWindow(fl_display, fl_screen);
		xev.message_type = data->manager_atom;
		xev.format = 32;
		xev.data.l[0] = timestamp;
		xev.data.l[1] = data->selection_atom;
		xev.data.l[2] = data->window;
		xev.data.l[3] = 0;   // manager specific data
		xev.data.l[4] = 0;   // manager specific data

		XSendEvent(fl_display, RootWindow(fl_display, fl_screen), False, StructureNotifyMask, (XEvent*)&xev);
		return true;
	}

	return false;
}

bool Xsm::should_quit(const XEvent* xev) {
	EASSERT(data != NULL);

	if(xev->xany.window == data->window && 
		xev->xany.type == SelectionClear &&
		xev->xselectionclear.selection == data->selection_atom) {
		puts("XXXXXXXXXXXXX");
		return true;
	}

	return false;
}

