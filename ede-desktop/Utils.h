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

#include <X11/Xlib.h>
#include <FL/Fl_Window.H>
#include <FL/Fl_Image.H>

void draw_xoverlay(int x, int y, int w, int h);
void clear_xoverlay(void);
void set_xoverlay_drawable(Fl_Window* win);

Pixmap create_mask(Fl_Image* img);

char* get_basename(const char* path);
bool  is_temp_filename(const char* path);

#endif
