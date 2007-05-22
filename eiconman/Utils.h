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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <fltk/Window.h>

int  net_get_workspace_count(void);
int  net_get_current_desktop(void);
bool net_get_workarea(int& x, int& y, int& w, int &h);
void net_make_me_desktop(fltk::Window* w);
int  net_get_workspace_names(char**& names);

void draw_xoverlay(int x, int y, int w, int h);
void clear_xoverlay(void);

#endif
