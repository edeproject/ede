/*
 * $Id$
 *
 * Configure window for eworkpanel
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "epanelconf.h"

#include <fltk/Input.h>
#include <fltk/x.h>
//#include <efltk/Fl_WM.h>
#include "../edelib2/Config.h"
//#include <efltk/Fl_String_List.h>

using namespace fltk;
using namespace edelib;



extern Input *workspaces[8];

void read_config()
{
	char temp_value[128];
	bool temp_bool=0;
	Config cfg(Config::find_file("ede.conf", 0));
	cfg.set_section("Panel");
	
	if(!cfg.read("Volume Control", temp_value, 0, sizeof(temp_value))) {
		vcProgram->value(temp_value);
	}
	
	if(!cfg.read("Time and date", temp_value, 0, sizeof(temp_value))) {
		tdProgram->value(temp_value);
	}
	
	cfg.read("AutoHide", temp_bool, false);
	autohide_check->value(temp_bool);
	
	cfg.set_section("Web");
	if(!cfg.read("Browser", temp_value, 0, sizeof(temp_value))) {
		browserProgram->value(temp_value);
	}
	
	cfg.set_section("Terminal");
	if(!cfg.read("Terminal", temp_value, 0, sizeof(temp_value))) {
		terminalProgram->value(temp_value);
	}
	
}

void write_config()
{
	Config cfg(Config::find_file("ede.conf", 0));
	cfg.set_section("Panel");
	cfg.write("Volume Control", vcProgram->value());
	cfg.write("Time and date", tdProgram->value());
	cfg.write("AutoHide", autohide_check->value());
	
	cfg.set_section("Web");
	cfg.write("Browser", browserProgram->value());
	cfg.set_section("Terminal");
	cfg.write("Terminal", terminalProgram->value());
	
	// Write workspace names to file, edewm will read and set on startup
	cfg.set_section("Workspaces");
	cfg.write("Count", int(ws_slider->value()));
	for(int n=0; n<8; n++) {
		char tmp[128]; snprintf(tmp, sizeof(tmp)-1, "Workspace%d", n+1);
		cfg.write(tmp, workspaces[n]->value());
	}
}

/*
// This was an attempt to separate code into Fl_WM class
// For the moment, we abandon this attempt

void get_workspaces(Fl_CString_List &desktops, int &count);
void update_workspaces()
{
    Fl_CString_List desktops;
    desktops.auto_delete(true);

    int count;
    get_workspaces(desktops, count);
    if(count>8) count=8;
    for(int n=0; n<8; n++) {
        const char *name = desktops.item(n);
        Fl_Input *i = workspaces[n];
        if(n<count) i->activate();
        if(name) {
            i->value(name);
        } else {
            char tmp[128];
            snprintf(tmp, sizeof(tmp)-1, "%s %d", "Workspace" ,n+1);
            i->value(tmp);
        }
    }
    ws_slider->value(count);
    desktops.clear();
}*/

/////////////////////////////////////
/////////////////////////////////////
// Code for setting desktop names using NET-WM

static bool atoms_inited=false;

// NET-WM spec desktop atoms
static Atom _XA_NET_NUM_DESKTOPS;
static Atom _XA_NET_DESKTOP_NAMES;
// GNOME atoms:
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_WORKSPACE_NAMES;

static void init_atoms()
{
    if(atoms_inited) return;
    open_display();

#define A(name) XInternAtom(xdisplay, name, False)

    _XA_NET_NUM_DESKTOPS    = A("_NET_NUMBER_OF_DESKTOPS");
    _XA_NET_DESKTOP_NAMES   = A("_NET_DESKTOP_NAMES");

    _XA_WIN_WORKSPACE_COUNT = A("_WIN_WORKSPACE_COUNT");
    _XA_WIN_WORKSPACE_NAMES = A("_WIN_WORKSPACE_NAMES");

    atoms_inited=true;
}

void* getProperty(XWindow w, Atom a, Atom type, unsigned long* np=0)
{
    Atom realType;
    int format;
    unsigned long n, extra;
    int status;
    void* prop;
    status = XGetWindowProperty(xdisplay, w,
                                a, 0L, 256L, False, type, &realType,
                                &format, &n, &extra, (uchar**)&prop);
    if (status != Success) return 0;
    if (!prop) return 0;
    if (!n) {XFree(prop); return 0;}
    if (np) *np = n;
    return prop;
}

int getIntProperty(XWindow w, Atom a, Atom type, int deflt) {
    void* prop = getProperty(w, a, type);
    if(!prop) return deflt;
    int r = int(*(long*)prop);
    XFree(prop);
    return r;
}

void setProperty(XWindow w, Atom a, Atom type, int v) {
    long prop = v;
    XChangeProperty(xdisplay, w, a, type, 32, PropModeReplace, (uchar*)&prop,1);
}

//void get_workspaces(Fl_CString_List &desktops, int &count)
void update_workspaces()
{
	init_atoms();

	int count = 0;
	int current = 0;
//    desktops.clear();
//    desktops.auto_delete(true);

	int length=0;
	char *buffer=0;

	XTextProperty names;
	// First try to get NET desktop names
	XGetTextProperty(xdisplay, RootWindow(xdisplay, xscreen), &names, _XA_NET_DESKTOP_NAMES);
	// If not found, look for GNOME ones
	if(!names.value) XGetTextProperty(xdisplay, RootWindow(xdisplay, xscreen), &names, _XA_WIN_WORKSPACE_NAMES);
	buffer = (char *)names.value;
	length = names.nitems;

	if(buffer) {
		char* c = buffer;
		for (int i = 1; c < buffer+length; i++) {
			char* d = c;
			while(*d) d++;
			if(*c != '<') {
				if(strcmp(c, "") != 0) {
					Input *i = workspaces[current];
        				i->activate();
					i->value(strdup(c));
					current++;
				}
			}
			c = d+1;
		}
		XFree(names.value);
	}
	
	count = getIntProperty(RootWindow(xdisplay, xscreen), _XA_NET_NUM_DESKTOPS, XA_CARDINAL, -1);
	if(count<0) count = getIntProperty(RootWindow(xdisplay, xscreen), _XA_WIN_WORKSPACE_COUNT, XA_CARDINAL, -1);
	
	// FIXME: What to do with count now?
}



// Code taken from FL_WM.cpp
Atom _XA_NET_SUPPORTED = 0;
Atom _XA_NET_SUPPORTING_WM_CHECK = 0;

XWindow fl_wmspec_check_window = None;
bool fl_netwm_supports(Atom &xproperty)
{
    // Vedran: -manual atoms initing:
    _XA_NET_SUPPORTING_WM_CHECK = A("_NET_SUPPORTING_WM_CHECK");
    _XA_NET_SUPPORTED = A("_NET_SUPPORTED");

    static Atom *atoms = NULL;
    static int natoms = 0;

    Atom type;
    int format;
    ulong nitems;
    ulong bytes_after;
    XWindow *xwindow;

    if(fl_wmspec_check_window != None) {
        if(atoms == NULL)
            return false;
        for(int i=0; i<natoms; i++) {
            if (atoms[i] == xproperty)
                return true;
       }
        return false;
    }

    if(atoms) XFree(atoms);

    atoms = NULL;
    natoms = 0;

    /* This function is very slow on every call if you are not running a
     * spec-supporting WM. For now not optimized, because it isn't in
     * any critical code paths, but if you used it somewhere that had to
     * be fast you want to avoid "GTK is slow with old WMs" complaints.
     * Probably at that point the function should be changed to query
     * _NET_SUPPORTING_WM_CHECK only once every 10 seconds or something.
     */
    XGetWindowProperty (xdisplay, RootWindow(xdisplay, xscreen),
                        _XA_NET_SUPPORTING_WM_CHECK, 0, ~0L,
                        False, XA_WINDOW, &type, &format, &nitems,
                        &bytes_after, (uchar **)&xwindow);

    if(type != XA_WINDOW)
        return false;

    // Find out if this WM goes away, so we can reset everything.
    XSelectInput(xdisplay, *xwindow, StructureNotifyMask);
    XFlush(xdisplay);

    XGetWindowProperty (xdisplay, RootWindow(xdisplay, xscreen),
                        _XA_NET_SUPPORTED, 0, ~0L,
                        False, XA_ATOM, &type, &format, (ulong*)&natoms,
                        &bytes_after, (uchar **)&atoms);

    if(type != XA_ATOM)
        return false;

    fl_wmspec_check_window = *xwindow;
    XFree(xwindow);

    // since wmspec_check_window != None this isn't infinite. ;-)
    return fl_netwm_supports(xproperty);
}

int sendClientMessage(XWindow w, Atom a, long x)
{
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = w;
    xev.xclient.display = xdisplay;
    xev.xclient.message_type = a;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = x;
    xev.xclient.data.l[1] = CurrentTime;
    int ret = XSendEvent (xdisplay, RootWindow(xdisplay, xscreen), False,
                          SubstructureRedirectMask | SubstructureNotifyMask,
                          &xev);
    XSync(xdisplay, True);
    return ret;
}

bool fl_set_workspace_count(int count)
{
    //init_atom(&_XA_NET_NUM_DESKTOPS);
    // Vedran: - use existing init_atoms() fn
    init_atoms();
    if(fl_netwm_supports(_XA_NET_NUM_DESKTOPS)) {
        sendClientMessage(RootWindow(xdisplay, xscreen), _XA_NET_NUM_DESKTOPS, count);
        return true;
    }
    return false;
}

// End code from FL_WM.cpp



void send_workspaces()
{
	init_atoms();
	
	int cnt = int(ws_slider->value());
	
	// Tell windowmanager to update its internal desktop count
	//Fl_WM::set_workspace_count(cnt);
	fl_set_workspace_count(cnt);
	
	char *ws_names[8];
	for(int n=0; n<cnt; n++)
	{
		if(!strcmp(workspaces[n]->value(), "")) {
			char tmp[128];
			snprintf(tmp, sizeof(tmp)-1, "%s %d", "Workspace", n+1);
			ws_names[n] = strdup(tmp);
		} else
			ws_names[n] = strdup(workspaces[n]->value());
	}
	
	XTextProperty names;
	if(XStringListToTextProperty((char **)ws_names, cnt, &names)) {
		XSetTextProperty(xdisplay, RootWindow(xdisplay, xscreen), &names, _XA_NET_DESKTOP_NAMES);
		XSetTextProperty(xdisplay, RootWindow(xdisplay, xscreen), &names, _XA_WIN_WORKSPACE_NAMES);
		XFree(names.value);
	}
}
