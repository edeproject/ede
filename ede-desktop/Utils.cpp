/*
 * $Id$
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <FL/x.H>
#include <edelib/Debug.h>

#include "Utils.h"

static int overlay_x = 0;
static int overlay_y = 0;
static int overlay_w = 0;
static int overlay_h = 0;

static Fl_Window* overlay_drawable = NULL;

static char dash_list[] = {1};

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

	/* set line to 0 again */
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

	/* no alpha */
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
			/* jump rgb and pick alpha */
			src += 3;
			a = *src++;

			if(a < 128) {
				/* these are transparent */
				XPutPixel(xim, x, y, 0);
			}
			else {
				/* these are opaque */
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
	char* p = (char*)strrchr(path, '/');
	if(p)
		return (p + 1);

	return (char*)path;
}

bool is_temp_filename(const char* path) {
	int len;

	if(!path || path[0] == '\0' || path[0] == '.')
		return true;

	len = strlen(path);
	if(path[len - 1] == '~')
		return true;

	return false;
}
