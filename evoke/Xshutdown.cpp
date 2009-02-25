/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <unistd.h>
#include <FL/x.H>
#include "Xshutdown.h"

/*
 * This is re-implementation of XmuClientWindow() so I don't have to link code with libXmu. 
 * XmuClientWindow() will return parent window of given window; this is used so we don't 
 * send delete message to some button or else, but it's parent.
 */
static Window mu_try_children(Display* dpy, Window win, Atom wm_state) {
	Atom real;
	Window root, parent;
	Window* children = 0;
	Window ret = 0;
	unsigned int nchildren;
	unsigned char* prop;
	unsigned long n, extra;
	int format;

	if(!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
		return 0;

	for(unsigned int i = 0; (i < nchildren) && !ret; i++) {
		prop = NULL;
		XGetWindowProperty(dpy, children[i], wm_state, 0, 0, False, AnyPropertyType,
				&real, &format, &n, &extra, (unsigned char**)&prop);
		if(prop)
			XFree(prop);
		if(real)
			ret = children[i];
	}

	for(unsigned int i = 0; (i < nchildren) && !ret; i++)
		ret = mu_try_children(dpy, win, wm_state);

	if(children)
		XFree(children);
	return ret;
}

static Window mu_client_window(Display* dpy, Window win, Atom wm_state) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;
	int status = XGetWindowProperty(dpy, win, wm_state, 0, 0, False, AnyPropertyType,
			&real, &format, &n, &extra, (unsigned char**)&prop);
	if(prop)
		XFree(prop);

	if(status != Success)
		return win;

	if(real)
		return win;

	Window w = mu_try_children(dpy, win, wm_state);
	if(!w) 
		w = win;

	return w;
}

void x_shutdown(void) {
	Window dummy, *wins;
	Window root = RootWindow(fl_display, fl_screen);
	unsigned int n;

	if(!XQueryTree(fl_display, root, &dummy, &dummy, &wins, &n))
		return;

	Atom wm_protocols     = XInternAtom(fl_display, "WM_PROTOCOLS", False);
	Atom wm_delete_window = XInternAtom(fl_display, "WM_DELETE_WINDOW", False);
	Atom wm_state         = XInternAtom(fl_display, "WM_STATE", False);
	XWindowAttributes attr;
	XEvent ev;

	for(unsigned int i = 0; i < n; i++) {
		if(XGetWindowAttributes(fl_display, wins[i], &attr) && (attr.map_state == IsViewable))
			wins[i] = mu_client_window(fl_display, wins[i], wm_state);
		else
			wins[i] = 0;
	}

	/*
	 * Hm... probably we should first quit known processes started by us
	 * then rest of the X familly
	 */
	for(unsigned int i = 0; i < n; i++) {
		if(wins[i]) {
			/* FIXME: check WM_PROTOCOLS before sending WM_DELETE_WINDOW ??? */
			memset(&ev, 0, sizeof(ev));
			ev.xclient.type = ClientMessage;
			ev.xclient.window = wins[i];
			ev.xclient.message_type = wm_protocols;
			ev.xclient.format = 32;
			ev.xclient.data.l[0] = (long)wm_delete_window;
			ev.xclient.data.l[1] = CurrentTime;
			XSendEvent(fl_display, wins[i], False, 0L, &ev);
		}
	}

	XSync(fl_display, False);
	sleep(1);

	/* kill remaining windows */
	for(unsigned int i = 0; i < n; i++) {
		if(wins[i]) { 
			XKillClient(fl_display, wins[i]);
		}
	}

	XSync(fl_display, False);
	XFree(wins);
}

