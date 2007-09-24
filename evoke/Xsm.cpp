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
#include <edelib/List.h>

#include <FL/x.h>
#include <X11/Xmd.h> // CARD16, CARD32

#include <stdio.h>   // snprintf
#include <stdlib.h>  // free
#include <string.h>  // strdup, strcmp

#define XSETTINGS_PAD(n, p) ((n + p - 1) & (~(p - 1)))

typedef edelib::list<XsettingsSetting*> XsettingsList;
typedef edelib::list<XsettingsSetting*>::iterator XsettingsListIter;

struct XsmData {
	Window window;
	Atom manager_atom;
	Atom selection_atom;
	Atom xsettings_atom;
	unsigned long serial;

	XsettingsList list;
};

struct XsettingsBuffer {
	unsigned char* content;
	unsigned char* pos;
	int len;
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

	unsigned long last_change_serial;
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

char settings_byte_order(void) {
	CARD32 myint = 0x01020304;
	return (*(char*)&myint == 1) ? MSBFirst : LSBFirst;
}

bool settings_equal(const XsettingsSetting* s1, const XsettingsSetting* s2) {
	EASSERT(s1 != NULL && s2 != NULL);

	if(s1->type != s2->type)
		return false;
	if(strcmp(s1->name, s2->name) != 0)
		return false;

	switch(s1->type) {
		case XS_TYPE_INT:
			return s1->data.v_int == s2->data.v_int;
		case XS_TYPE_COLOR:
			return (s1->data.v_color.red == s2->data.v_color.red) && 
				(s1->data.v_color.green == s2->data.v_color.green) &&
				(s1->data.v_color.blue == s2->data.v_color.blue) &&
				(s1->data.v_color.alpha == s2->data.v_color.alpha);
		case XS_TYPE_STRING:
			return (strcmp(s1->data.v_string, s2->data.v_string) == 0);
		}

	return false;
}

int setting_len(const XsettingsSetting* setting) {
	int len = 8;  // type + pad + name-len + last-change-serial
	len += XSETTINGS_PAD(strlen(setting->name), 4);

	switch(setting->type) {
		case XS_TYPE_INT:
			len += 4;
			break;
		case XS_TYPE_COLOR:
			len += 8;
			break;
		case XS_TYPE_STRING:
			len += 4 + XSETTINGS_PAD(strlen(setting->data.v_string), 4);
			break;
	}

	return len;
}

void setting_store(const XsettingsSetting* setting, XsettingsBuffer* buffer) {
	int len, str_len;

	*(buffer->pos++) = setting->type;
	*(buffer->pos++) = 0;

	str_len = strlen(setting->name);
	*(CARD16*)(buffer->pos) = str_len;
	buffer->pos += 2;

	len = XSETTINGS_PAD(str_len, 4);
	memcpy(buffer->pos, setting->name, str_len);
	len -= str_len;
	buffer->pos += str_len;

	for(; len > 0; len--)
		*(buffer->pos++) = 0;

	*(CARD32*)(buffer->pos) = setting->last_change_serial;
	buffer->pos += 4;

	switch(setting->type) {
		case XS_TYPE_INT:
			*(CARD32*)(buffer->pos) = setting->data.v_int;
			buffer->pos += 4;
			break;
		case XS_TYPE_COLOR:
			*(CARD16*)(buffer->pos) = setting->data.v_color.red;
			*(CARD16*)(buffer->pos + 2) = setting->data.v_color.green;
			*(CARD16*)(buffer->pos + 4) = setting->data.v_color.blue;
			*(CARD16*)(buffer->pos + 6) = setting->data.v_color.alpha;
			buffer->pos += 8;
			break;
		case XS_TYPE_STRING:
			str_len = strlen(setting->data.v_string);
			*(CARD32*)(buffer->pos) = str_len;
			buffer->pos += 4;

			len = XSETTINGS_PAD(str_len, 4);
			memcpy(buffer->pos, setting->data.v_string, str_len);
			len -= str_len;
			buffer->pos += str_len;

			for(; len > 0; len--)
				*(buffer->pos++) = 0;

			break;
	}
}

Xsm::Xsm() : data(NULL) { }

Xsm::~Xsm() { 
	if(data) {
		XDestroyWindow(fl_display, data->window);
		XsettingsList& lst = data->list;

		if(!lst.empty()) {
			XsettingsListIter it = lst.begin(), it_end = lst.end();
			for(; it != it_end; ++it)
				delete_setting(*it);

			lst.clear();
		}

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
		return true;
	}

	return false;
}

void Xsm::set_setting(const XsettingsSetting* setting) {
	EASSERT(data != NULL);

	XsettingsList& lst = data->list;
	XsettingsListIter it = lst.begin(), it_end = lst.end();

	/*
	 * Check if entry already exists. If setting with the same
	 * name already exists, but with different values, that setting
	 * will be completely replaced with new one.
	 */
	for(; it != it_end; ++it) {
		if(strcmp((*it)->name, setting->name) == 0) {
			if(settings_equal((*it), setting)) {
				return;
			} else {
				delete_setting(*it);
				lst.erase(it);
				break;
			}
		}
	}

	XsettingsSetting* new_setting = new XsettingsSetting;
	new_setting->name = strdup(setting->name);
	//new_setting->last_change_serial = setting->last_change_serial;
	new_setting->type = setting->type;

	switch(new_setting->type) {
		case XS_TYPE_INT:
			new_setting->data.v_int = setting->data.v_int;
			break;
		case XS_TYPE_COLOR:
			new_setting->data.v_color.red = setting->data.v_color.red;
			new_setting->data.v_color.green = setting->data.v_color.green;
			new_setting->data.v_color.blue = setting->data.v_color.blue;
			new_setting->data.v_color.alpha = setting->data.v_color.alpha;
			break;
		case XS_TYPE_STRING:
			new_setting->data.v_string = strdup(setting->data.v_string);
			break;
	}

	new_setting->last_change_serial = data->serial;
	lst.push_back(new_setting);
}

void Xsm::delete_setting(XsettingsSetting* setting) {
	if(!setting)
		return;

	if(setting->type == XS_TYPE_STRING)
		free(setting->data.v_string);
	free(setting->name);
	delete setting;
	setting = NULL;
}


void Xsm::set_int(const char* name, int val) {
	XsettingsSetting setting;

	setting.name = (char*)name;
	setting.type = XS_TYPE_INT;
	setting.data.v_int = val;

	set_setting(&setting);
}

void Xsm::set_color(const char* name, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
	XsettingsSetting setting;

	setting.name = (char*)name;
	setting.type = XS_TYPE_COLOR;
	setting.data.v_color.red = red;
	setting.data.v_color.green = green;
	setting.data.v_color.blue = blue;
	setting.data.v_color.alpha = alpha;

	set_setting(&setting);
}

void Xsm::set_string(const char* name, const char* str) {
	XsettingsSetting setting;

	setting.name = (char*)name;
	setting.type = XS_TYPE_STRING;
	setting.data.v_string = (char*)str;

	set_setting(&setting);
}

void Xsm::notify(void) {
	EASSERT(data != NULL);

	int n_settings = 0;
	XsettingsBuffer buff;

	buff.len = 12;  // byte-order + pad + SERIAL + N_SETTINGS

	XsettingsList& lst = data->list;
	XsettingsListIter it = lst.begin(), it_end = lst.end();

	for(; it != it_end; ++it) {
		buff.len += setting_len(*it);
		n_settings++;
	}

	buff.content = new unsigned char[buff.len];
	buff.pos = buff.content;

	*buff.pos = settings_byte_order();

	buff.pos += 4;
	*(CARD32*)buff.pos = data->serial++;
	buff.pos += 4;
	*(CARD32*)buff.pos = n_settings;
	buff.pos += 4;

	for(it = lst.begin(); it != it_end; ++it)
		setting_store(*it, &buff);

	XChangeProperty(fl_display, data->window, data->xsettings_atom, data->xsettings_atom,
			8, PropModeReplace, buff.content, buff.len);

	delete [] buff.content;
}
