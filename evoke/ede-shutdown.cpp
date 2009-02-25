/*
 * $Id$
 *
 * ede-shutdown, a command to quit EDE
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

int main(int argc, char** argv) {
	Display* dpy = XOpenDisplay(0);
	if(!dpy) {
		puts("Unabel to open default display");
		return 1;
	}

	int scr = DefaultScreen(dpy);
	Atom ede_quit = XInternAtom(dpy, "_EDE_EVOKE_QUIT", False);
	int dummy = 1;

	XChangeProperty(dpy, RootWindow(dpy, scr),
            ede_quit, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&dummy, sizeof(int));
	XSync(dpy, False);
	return 0;
}
