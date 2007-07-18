/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __ATOMS_H__
#define __ATOMS_H__

#ifdef _DEBUG
#include <map>
#endif

#include <X11/Xlib.h>

// Icccm atoms
extern Atom _XA_WM_PROTOCOLS;
extern Atom _XA_WM_DELETE_WINDOW;
extern Atom _XA_WM_STATE;
extern Atom _XA_WM_CHANGE_STATE;
extern Atom _XA_WM_COLORMAP_WINDOWS;
extern Atom _XA_WM_TAKE_FOCUS;

extern Atom _XA_UTF8_STRING;

// Motif atoms
extern Atom _XA_MOTIF_HINTS;

// Netwm atoms
// root messages
extern Atom _XA_NET_SUPPORTED; 
extern Atom _XA_NET_SUPPORTING_WM_CHECK;
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;
extern Atom _XA_NET_DESKTOP_GEOMETRY;
extern Atom _XA_NET_DESKTOP_VIEWPORT;
extern Atom _XA_NET_CURRENT_DESKTOP;
extern Atom _XA_NET_DESKTOP_NAMES;
extern Atom _XA_NET_ACTIVE_WINDOW;
extern Atom _XA_NET_WORKAREA;
extern Atom _XA_NET_SHOWING_DESKTOP;
// other root messages
extern Atom _XA_NET_CLOSE_WINDOW;
extern Atom _XA_NET_MOVERESIZE_WINDOW;
extern Atom _XA_NET_RESTACK_WINDOW;
extern Atom _XA_NET_REQUEST_FRAME_EXTENTS;

// application messages
extern Atom _XA_NET_WM_NAME;
extern Atom _XA_NET_WM_WINDOW_TYPE;
	extern Atom _XA_NET_WM_WINDOW_TYPE_NORMAL;
	extern Atom _XA_NET_WM_WINDOW_TYPE_DOCK;
	extern Atom _XA_NET_WM_WINDOW_TYPE_TOOLBAR;
	extern Atom _XA_NET_WM_WINDOW_TYPE_MENU;
	extern Atom _XA_NET_WM_WINDOW_TYPE_UTIL;
	extern Atom _XA_NET_WM_WINDOW_TYPE_DIALOG;
	extern Atom _XA_NET_WM_WINDOW_TYPE_SPLASH;
	extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;

extern Atom _XA_NET_WM_STATE_SHADED;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_VERT;
extern Atom _XA_NET_WM_STATE_MAXIMIZED_HORZ;
extern Atom _XA_NET_WM_STATE_ABOVE;
extern Atom _XA_NET_WM_STATE_BELOW;

// how to apply above states
#define _NET_WM_STATE_REMOVE    0         // remove/unset property
#define _NET_WM_STATE_ADD       1         // add/set property
#define _NET_WM_STATE_TOGGLE    2         // toggle property

// our messages
extern Atom _XA_EDE_WM_STARTUP_NOTIFY;
	extern Atom _XA_EDE_WM_APP_STARTING;

// atoms for debugging (not implemented)
extern Atom _XA_NET_VIRTUAL_ROOTS;
extern Atom _XA_NET_DESKTOP_LAYOUT;
extern Atom _XA_NET_WM_MOVERESIZE;
extern Atom _XA_NET_RESTACK_WINDOW;
extern Atom _XA_NET_REQUEST_FRAME_EXTENTS;
extern Atom _XA_NET_WM_NAME;
extern Atom _XA_NET_WM_VISIBLE_NAME;
extern Atom _XA_NET_WM_ICON_NAME;
extern Atom _XA_NET_WM_ICON_VISIBLE_NAME;
extern Atom _XA_NET_WM_DESKTOP;
extern Atom _XA_NET_WM_STATE;
extern Atom _XA_NET_WM_STATE_MODAL; //Needs transient for (root for whole group)
extern Atom _XA_NET_WM_STATE_STICKY; //Pos fixed, even if virt. desk. scrolls
extern Atom _XA_NET_WM_STATE_SKIP_TASKBAR;
extern Atom _XA_NET_WM_STATE_SKIP_PAGER;
extern Atom _XA_NET_WM_STATE_HIDDEN; 
extern Atom _XA_NET_WM_STATE_FULLSCREEN;
extern Atom _XA_NET_WM_STATE_DEMANDS_ATTENTION;

extern Atom _XA_NET_WM_ALLOWED_ACTIONS;
extern Atom _XA_NET_WM_ACTION_MOVE;
extern Atom _XA_NET_WM_ACTION_RESIZE;
extern Atom _XA_NET_WM_ACTION_MINIMIZE;
extern Atom _XA_NET_WM_ACTION_SHADE;
extern Atom _XA_NET_WM_ACTION_STICK;
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_HORZ;
extern Atom _XA_NET_WM_ACTION_MAXIMIZE_VERT;
extern Atom _XA_NET_WM_ACTION_FULLSCREEN;
extern Atom _XA_NET_WM_ACTION_CHANGE_DESKTOP;
extern Atom _XA_NET_WM_ACTION_CLOSE;
extern Atom _XA_NET_WM_STRUT;
extern Atom _XA_NET_WM_STRUT_PARTIAL;
extern Atom _XA_NET_WM_ICON_GEOMETRY;
extern Atom _XA_NET_WM_ICON;
extern Atom _XA_NET_WM_PID;
extern Atom _XA_NET_WM_HANDLED_ICONS;
extern Atom _XA_NET_WM_USER_TIME;
extern Atom _XA_NET_FRAME_EXTENTS;
extern Atom _XA_NET_WM_PING;
extern Atom _XA_NET_WM_SYNC_REQUEST;
extern Atom _XA_NET_WM_STATE_STAYS_ON_TOP;
extern Atom _XA_KWM_WIN_ICON;

#ifdef _DEBUG
	void InitAtoms(Display* display, std::map<Atom, const char*>& atoms_map);
#else
	void InitAtoms(Display* display);
#endif
void SetSupported(Window root);

#endif // __ATOMS_H__
