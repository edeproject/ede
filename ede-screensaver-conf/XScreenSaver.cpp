#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h> // toupper

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

#include <edelib/File.h>
#include <edelib/Run.h>
#include <edelib/Directory.h>

#include "XScreenSaver.h"

EDELIB_NS_USING(String)
EDELIB_NS_USING(file_path)
EDELIB_NS_USING(run_program)
EDELIB_NS_USING(dir_home)

static Atom XA_SCREENSAVER;
static Atom XA_SCREENSAVER_VERSION;
static Atom XA_DEMO;
static Atom XA_SELECT;

static XErrorHandler old_handler = 0;
static Bool          got_bad_window = False;
static int           atoms_loaded = 0;

static int bad_window_handler(Display *dpy, XErrorEvent *xevent) {
	if(xevent->error_code == BadWindow) {
		got_bad_window = True;
		return 0;
	}

	if(!old_handler)
		return 0;

	return (*old_handler)(dpy, xevent);
}

/* 
 * convert "xx:xx:xx" to minutes
 * TODO: a better code would be great
 */
static int time_to_min(const char *t) {
	char nb[64];
	const char *p = t;
	unsigned int i = 0;
	int ret = 0;

	for(; *p && *p != ':' && i < sizeof(nb); p++, i++)
		nb[i] = *p;
	p++;
	nb[i] = '\0';

	ret = atoi(nb) * 60;

	i = 0;
	if(*p) {
		for(i = 0; *p && *p != ':' && i < sizeof(nb); p++, i++)
			nb[i] = *p;
		p++;
	}

	nb[i] = '\0';
	ret += atoi(nb);

	return ret;
}

static void xscreensaver_init_atoms_once(Display *dpy) {
	if(atoms_loaded)
		return;

	XA_SCREENSAVER = XInternAtom(dpy, "SCREENSAVER", False);
	XA_SCREENSAVER_VERSION = XInternAtom(dpy, "_SCREENSAVER_VERSION", False);
	XA_DEMO = XInternAtom(dpy, "DEMO", False);
	XA_SELECT = XInternAtom(dpy, "SELECT", False);
	XSync(dpy, 0);

	atoms_loaded = 1;
}

static Window xscreensaver_find_own_window(Display *dpy) {
	Window root = RootWindow(dpy, DefaultScreen(dpy));
	Window root2, parent, *childs;
	unsigned int nchilds;

	if(!XQueryTree(dpy, root, &root2, &parent, &childs, &nchilds))
		return 0;
	if(root != root2)
		return 0;

	for(unsigned int i = 0; i < nchilds; i++) {
		Atom type;
		int format;
		unsigned long nitems, bytesafter;
		char *v;
		int status;

		XSync(dpy, False);
		got_bad_window = False;
		old_handler = XSetErrorHandler(bad_window_handler);

		status = XGetWindowProperty(dpy, childs[i], XA_SCREENSAVER_VERSION, 0, 200, False, XA_STRING,
				&type, &format, &nitems, &bytesafter,
				(unsigned char**)&v);

		XSync(dpy, False);
		XSetErrorHandler(old_handler);
		old_handler = 0;

		if(got_bad_window) {
			status = BadWindow;
			got_bad_window = False;
		}

		if(status == Success && type != None) {
			/* TODO: check if XFree(v) is needed */
			Window ret = childs[i];
			XFree(childs);
			return ret;
		}
	}

	XFree(childs);
	return 0;
}

static void xscreensaver_run_hack(Display *dpy, Window id, long hack) {
	XEvent ev;

	ev.xany.type = ClientMessage;
	ev.xclient.display = dpy;
	ev.xclient.window = id;
	ev.xclient.message_type = XA_SCREENSAVER;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XA_SELECT;
	ev.xclient.data.l[1] = hack;
	ev.xclient.data.l[2] = 0;

	/*
	ev.xclient.data.l[0] = XA_DEMO;
	ev.xclient.data.l[1] = 5000;  // XA_DEMO protocol version
	ev.xclient.data.l[2] = hack;
	*/

	if(XSendEvent(dpy, id, False, 0, &ev) == 0)
		puts("XSendEvent() failed");
	else
		XSync(dpy, 0);
}

bool xscreensaver_run(Display *dpy) {
	xscreensaver_init_atoms_once(dpy);

	Window id = xscreensaver_find_own_window(dpy);

	/* if not running, try to manualy start it */
	if(id == 0) { 
		String p = file_path("xscreensaver");
		if(p.empty())
			return false;
 
		/* run 'xscreensaver -nosplash' */
		p += " -nosplash";
		run_program(p.c_str());

		/* check again */
		id = xscreensaver_find_own_window(dpy);
		if(id == 0)
			return false;
	}

	return true;
}

SaverPrefs *xscreensaver_read_config(void) {
	XrmDatabase db;
	XrmValue    xrmv;
	char        *type;

	/*
	 * Luckily, xscreensaver uses X resource for storage
	 * which saves me from parsing... jwz thanx !!!
	 */
	String path = dir_home();
	path += "/.xscreensaver";

	XrmInitialize();

	db = XrmGetFileDatabase(path.c_str());
	if(!db) {
		puts("Unable to open xscreensaver config file");
		return NULL;
	}

	SaverPrefs *ret = new SaverPrefs;
	ret->curr_hack = 0;
	ret->timeout = 2;  // in minutes
	ret->dpms_enabled = false;
	ret->dpms_standby = ret->dpms_suspend = ret->dpms_off = 30; // in minutes

	if(XrmGetResource(db, "selected", "*", &type, &xrmv) == True && xrmv.addr != NULL) {
		/* 
		 * safe without checks since 0 (if atoi() fails) is first hack
		 * in the list
		 */
		ret->curr_hack = atoi(xrmv.addr);
	}

	if(XrmGetResource(db, "timeout", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->timeout = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsEnabled", "*", &type, &xrmv) == True && xrmv.addr != NULL) {
		const char *v = xrmv.addr;

		if(!strcasecmp(v, "true") || !strcasecmp(v, "on") || !strcasecmp(v, "yes"))
			ret->dpms_enabled = true;
	}

	if(XrmGetResource(db, "dpmsStandby", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_standby = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsSuspend", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_suspend = time_to_min(xrmv.addr);

	if(XrmGetResource(db, "dpmsOff", "*", &type, &xrmv) == True && xrmv.addr != NULL)
		ret->dpms_off = time_to_min(xrmv.addr);

	/*
	 * Parse hacks (screensavers), skipping those that starts with '-'. Also, check if hack
	 * contains name beside command (e.g. '"Qix (solid)" qix -root') and use it; otherwise use
	 * capitalized command name.
	 *
	 * Note that to the each hack will be given index number; in the final GUI list they
	 * will be ordered, but xscreensaver keeps own order for selecting that must be preserved.
	 */
	int nhacks = 0;

	if(XrmGetResource(db, "programs", "*", &type, &xrmv) == True) {
		char *programs = strdup(xrmv.addr);
		char *c = NULL;
		char *p = NULL;
		char buf[256];
		unsigned int i;

		for(c = strtok(programs, "\n"); c; c = strtok(NULL, "\n"), nhacks++) {
			if(c[0] == '-')
				continue;

			if((p = strstr(c, "GL:")) != NULL) {
				p += 3;
				c = p;
			} 

			/* eat spaces */
			while(*c && (*c == ' ' || *c == '\t'))
				c++;

			if(*c == '"') {
				/* extract name from '"' */
				c++;

				for(i = 0; i < sizeof(buf) && *c && *c != '"'; c++, i++)
					buf[i] = *c;

				buf[i] = '\0';
			} else {
				/* or read command and capitalize it */
				for(i = 0; i < sizeof(buf) && *c && *c != ' '; c++, i++)
					buf[i] = *c;

				buf[i] = '\0';
				buf[0] = toupper(buf[0]);
			}

			SaverHack *h = new SaverHack;
			h->name = buf;
			h->sindex = nhacks;

			ret->hacks.push_back(h);
		}

		free(programs);
	}

	XrmDestroyDatabase(db);
	return ret;
}
