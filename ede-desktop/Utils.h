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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <X11/Xlib.h> // Pixmap

#include <FL/Fl_Window.H>
#include <FL/Fl_Image.H>

extern Atom _XA_NET_WORKAREA;
extern Atom _XA_NET_WM_WINDOW_TYPE;
extern Atom _XA_NET_WM_WINDOW_TYPE_DESKTOP;
extern Atom _XA_NET_NUMBER_OF_DESKTOPS;
extern Atom _XA_NET_CURRENT_DESKTOP;
extern Atom _XA_NET_DESKTOP_NAMES;
extern Atom _XA_XROOTPMAP_ID;

void init_atoms(void);

int  net_get_workspace_count(void);
int  net_get_current_desktop(void);
bool net_get_workarea(int& x, int& y, int& w, int &h);
void net_make_me_desktop(Fl_Window* w);
int  net_get_workspace_names(char**& names);

void draw_xoverlay(int x, int y, int w, int h);
void clear_xoverlay(void);
void set_xoverlay_drawable(Fl_Window* win);

Pixmap create_mask(Fl_Image* img);

char* get_basename(const char* path);
bool  is_temp_filename(const char* path);

#endif
