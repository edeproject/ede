/*
 * $Id$
 *
 * Eiconman, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include "Utils.h"
#include <X11/Xproto.h> // CARD32
#include <FL/x.h>
#include <edelib/Debug.h>

#include <string.h> // strrchr

Atom NET_WORKAREA = 0;
Atom NET_WM_WINDOW_TYPE = 0;
Atom NET_WM_WINDOW_TYPE_DESKTOP = 0;
Atom NET_NUMBER_OF_DESKTOPS = 0;
Atom NET_CURRENT_DESKTOP = 0;
Atom NET_DESKTOP_NAMES = 0;

int overlay_x = 0;
int overlay_y = 0;
int overlay_w = 0;
int overlay_h = 0;

Fl_Window* overlay_drawable = NULL;

char dash_list[] = {1};

bool net_get_workarea(int& x, int& y, int& w, int &h) {
	Atom real;

	if(!NET_WORKAREA)
		NET_WORKAREA= XInternAtom(fl_display, "_NET_WORKAREA", 1);

	int format;
	unsigned long n, extra;
	unsigned char* prop;
	x = y = w = h = 0;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			NET_WORKAREA, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, (unsigned char**)&prop);

	if(status != Success)
		return false;

	CARD32* val = (CARD32*)prop;
	if(val) {
		x = val[0];
		y = val[1];
		w = val[2];
		h = val[3];

		XFree((char*)val);
		return true;
	}

	return false;
}

void net_make_me_desktop(Fl_Window* w) {
	if(!NET_WM_WINDOW_TYPE)
		NET_WM_WINDOW_TYPE = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);

	if(!NET_WM_WINDOW_TYPE_DESKTOP)
		NET_WM_WINDOW_TYPE_DESKTOP= XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);

	/*
	 * xid() will return zero if window is not shown;
	 * make sure it is shown
	 */
	EASSERT(fl_xid(w));

	/*
	 * Reminder for me (others possible):
	 * note '&NET_WM_WINDOW_TYPE_DESKTOP' conversion, since gcc will not report warning/error 
	 * if placed 'NET_WM_WINDOW_TYPE_DESKTOP' only.
	 *
	 * I lost two hours messing with this ! (gdb is unusefull in X world)
	 */
	XChangeProperty(fl_display, fl_xid(w), NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, 
			(unsigned char*)&NET_WM_WINDOW_TYPE_DESKTOP, sizeof(Atom));
}

int net_get_workspace_count(void) {
	if(!NET_NUMBER_OF_DESKTOPS)
		NET_NUMBER_OF_DESKTOPS = XInternAtom(fl_display, "_NET_NUMBER_OF_DESKTOPS", False);

	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			NET_NUMBER_OF_DESKTOPS, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success && !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

// desktops are starting from 0
int net_get_current_desktop(void) {
	if(!NET_CURRENT_DESKTOP)
		NET_CURRENT_DESKTOP = XInternAtom(fl_display, "_NET_CURRENT_DESKTOP", False);
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			NET_CURRENT_DESKTOP, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, (unsigned char**)&prop);

	if(status != Success && !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

// call on this XFreeStringList(names)
int net_get_workspace_names(char**& names) {
	if(!NET_DESKTOP_NAMES)
		NET_DESKTOP_NAMES = XInternAtom(fl_display, "_NET_DESKTOP_NAMES", False);

	// FIXME: add _NET_SUPPORTING_WM_CHECK and _NET_SUPPORTED ???
	XTextProperty wnames;
	XGetTextProperty(fl_display, RootWindow(fl_display, fl_screen), &wnames, NET_DESKTOP_NAMES);

	// if wm does not understainds _NET_DESKTOP_NAMES this is not set
	if(!wnames.nitems || !wnames.value)
		return 0;

	int nsz;

	if(!XTextPropertyToStringList(&wnames, &names, &nsz)) {
		XFree(wnames.value);
		return 0;
	}

	//XFreeStringList(names);
	XFree(wnames.value);
	return nsz;
}

#if 0
int net_get_workspace_names(char** names) {
	Atom nd = XInternAtom(fl_display, "_NET_DESKTOP_NAMES", False);
	Atom utf8_str = XInternAtom(fl_display, "UTF8_STRING", False);
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			nd, 0L, 0x7fffffff, False, utf8_str, &real, &format, &n, &extra, (unsigned char**)&prop);

	if(status != Success && !prop)
		return 0;

	int alloc = 0;
	if(real == utf8_str && format == 8) {
		const char* p = (const char*)prop;
		for(int i = 0, s = 0; i < n && alloc < MAX_DESKTOPS; i++, alloc++) {
			if(p[i] == '\0') {
				EDEBUG("%c ", p[i]);
				names[alloc] = strndup((p + s), i - s + 1);
				s = i + 1;
			}
		}
	}

	return alloc;
}
#endif

void draw_overlay_rect(void) {
	if(overlay_w <= 0 || overlay_h <= 0)
		return;

	XSetDashes(fl_display, fl_gc, 0, dash_list, 1);
	XSetLineAttributes(fl_display, fl_gc, 1, LineOnOffDash, CapButt, JoinMiter);

	XSetFunction(fl_display, fl_gc, GXxor);
	XSetForeground(fl_display, fl_gc, 0xffffffff);

	Window ow;
	if(overlay_drawable)
		ow = fl_xid(overlay_drawable);
	else
		ow = fl_window;
	XDrawRectangle(fl_display, ow, fl_gc, overlay_x, overlay_y, overlay_w-1, overlay_h-1);

	XSetFunction(fl_display, fl_gc, GXcopy);

	// set line to 0 again
	XSetLineAttributes(fl_display, fl_gc, 0, LineOnOffDash, CapButt, JoinMiter);
}

void draw_xoverlay(int x, int y, int w, int h) {
	if(w < 0) {
		x += w;
		w = -w;
	} else if(!w)
		w = 1;

	if(h < 0) {
		y += h;
		h = -h;
	} else if(!h)
		h = 1;

	if(overlay_w > 0) {
		if(x == overlay_x && y == overlay_y && w == overlay_w && h == overlay_h)
			return;
		draw_overlay_rect();
	}

	overlay_x = x;
	overlay_y = y;
	overlay_w = w;
	overlay_h = h;

	draw_overlay_rect();
}

void clear_xoverlay(void) {
	if(overlay_w > 0) {
		draw_overlay_rect();
		overlay_w = 0;
	}
}

void set_xoverlay_drawable(Fl_Window* win) {
	overlay_drawable = win;
}

char* get_basename(const char* path) {
	char* p = strrchr(path, '/');
	if(p)
		return (p + 1);

	return (char*) path;
}
