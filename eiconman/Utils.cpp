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

#include <string.h> // strrchr, strncpy, strlen

Atom _XA_NET_WORKAREA = 0;
Atom _XA_NET_WM_WINDOW_TYPE = 0;
Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP = 0;
Atom _XA_NET_NUMBER_OF_DESKTOPS = 0;
Atom _XA_NET_CURRENT_DESKTOP = 0;
Atom _XA_NET_DESKTOP_NAMES = 0;
Atom _XA_XROOTPMAP_ID = 0;

int overlay_x = 0;
int overlay_y = 0;
int overlay_w = 0;
int overlay_h = 0;

Fl_Window* overlay_drawable = NULL;

char dash_list[] = {1};

void init_atoms(void) {
	_XA_NET_WORKAREA               = XInternAtom(fl_display, "_NET_WORKAREA", False);
	_XA_NET_WM_WINDOW_TYPE         = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE", False);
	_XA_NET_WM_WINDOW_TYPE_DESKTOP = XInternAtom(fl_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	_XA_NET_NUMBER_OF_DESKTOPS     = XInternAtom(fl_display, "_NET_NUMBER_OF_DESKTOPS", False);
	_XA_NET_CURRENT_DESKTOP        = XInternAtom(fl_display, "_NET_CURRENT_DESKTOP", False);
	_XA_NET_DESKTOP_NAMES          = XInternAtom(fl_display, "_NET_DESKTOP_NAMES", False);
	_XA_XROOTPMAP_ID               = XInternAtom(fl_display, "_XROOTPMAP_ID", False);
}

bool net_get_workarea(int& x, int& y, int& w, int &h) {
	Atom real;

	int format;
	unsigned long n, extra;
	unsigned char* prop;
	x = y = w = h = 0;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_WORKAREA, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, (unsigned char**)&prop);

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
	/*
	 * xid() will return zero if window is not shown;
	 * make sure it is shown
	 */
	EASSERT(fl_xid(w));

	/*
	 * Reminder for me (others possible):
	 * note '&_XA_NET_WM_WINDOW_TYPE_DESKTOP' conversion, since gcc will not report warning/error 
	 * if placed '_XA_NET_WM_WINDOW_TYPE_DESKTOP' only.
	 *
	 * I lost two hours messing with this ! (gdb is unusefull in X world)
	 */
	XChangeProperty(fl_display, fl_xid(w), _XA_NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, 
			(unsigned char*)&_XA_NET_WM_WINDOW_TYPE_DESKTOP, sizeof(Atom));
}

int net_get_workspace_count(void) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_NUMBER_OF_DESKTOPS, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success && !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

// desktops are starting from 0
int net_get_current_desktop(void) {
	Atom real;
	int format;
	unsigned long n, extra;
	unsigned char* prop;

	int status = XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen), 
			_XA_NET_CURRENT_DESKTOP, 0L, 0x7fffffff, False, XA_CARDINAL, &real, &format, &n, &extra, 
			(unsigned char**)&prop);

	if(status != Success && !prop)
		return -1;

	int ns = int(*(long*)prop);
	XFree(prop);
	return ns;
}

// call on this XFreeStringList(names)
int net_get_workspace_names(char**& names) {
	// FIXME: add _NET_SUPPORTING_WM_CHECK and _NET_SUPPORTED ???
	XTextProperty wnames;
	XGetTextProperty(fl_display, RootWindow(fl_display, fl_screen), &wnames, _XA_NET_DESKTOP_NAMES);

	// if wm does not understainds _NET_DESKTOP_NAMES this is not set
	if(!wnames.nitems || !wnames.value)
		return 0;

	int nsz;

	/*
	 * FIXME: Here should as alternative Xutf8TextPropertyToTextList since
	 * many wm's set UTF8_STRING property. Below is XA_STRING and for UTF8_STRING
	 * will fail.
	 */
	if(!XTextPropertyToStringList(&wnames, &names, &nsz)) {
		XFree(wnames.value);
		return 0;
	}

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
	XSetLineAttributes(fl_display, fl_gc, 2, LineOnOffDash, CapButt, JoinMiter);

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

Pixmap create_mask(Fl_Image* img) {
	if(!img)
		return 0;

	// no alpha
	if(img->d() != 4)
		return 0;

	int iw = img->w();
	int ih = img->h();

	unsigned char* xim_data = new unsigned char[((iw >> 3) + 8) * ih];

	XImage* xim = XCreateImage(fl_display, fl_visual->visual, 1, ZPixmap, 0, (char*)xim_data, iw, ih, 8, 0);
	if(!xim) {
		delete [] xim_data;
		return 0;
	}

	const char* src = img->data()[0];
	unsigned char a;

	for(int y = 0; y < ih; y++) {
		for(int x = 0; x < iw; x++) {
			// jump rgb and pick alpha
			src += 3;
			a = *src++;

			if(a < 128) {
				// these are transparent
				XPutPixel(xim, x, y, 0);
			}
			else {
				// these are opaque
				XPutPixel(xim, x, y, 1);
			}
		}
	}

	Window drawable = XCreateSimpleWindow(fl_display, RootWindow(fl_display, fl_screen), 0, 0, iw,
              ih, 0, 0, BlackPixel(fl_display, fl_screen));

	Pixmap pix = XCreatePixmap(fl_display, drawable, iw, ih, 1);

	XGCValues gcv;
	gcv.graphics_exposures = False;
	GC dgc = XCreateGC(fl_display, pix, GCGraphicsExposures, &gcv);

	XPutImage(fl_display, pix, dgc, xim, 0, 0, 0, 0, iw, ih);

	XDestroyWindow(fl_display, drawable);
	XFreeGC(fl_display, dgc);
	delete [] xim->data;
	xim->data = 0;
	XDestroyImage(xim);

	return pix;
}

char* get_basename(const char* path) {
	char* p = strrchr(path, '/');
	if(p)
		return (p + 1);

	return (char*)path;
}
