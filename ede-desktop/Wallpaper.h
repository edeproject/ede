/*
 * $Id$
 *
 * Copyright (C) 2006-2013 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __WALLPAPER_H__
#define __WALLPAPER_H__

#include <FL/Fl_Box.H>
#include <FL/x.H>
#include <edelib/String.h>

enum {
	WALLPAPER_CENTER,
	WALLPAPER_STRETCH,
	WALLPAPER_TILE
};

class Fl_Image;

class Wallpaper : public Fl_Box { 
private:
	Pixmap    rootpmap_pixmap;
	int       state;
	Fl_Image* stretched_alloc; /* FLTK issue */
	bool      use_rootpmap;
	EDELIB_NS_PREPEND(String) wpath;

	void set_rootpmap(void);
public:
	Wallpaper(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H),
		rootpmap_pixmap(0), state(WALLPAPER_CENTER), stretched_alloc(NULL), use_rootpmap(false) { }
	~Wallpaper();

	bool load(const char *path, int s, bool do_rootpmap = true);

	void draw(void);
	int handle(int event);
	void resize(int X, int Y, int W, int H);
};

#endif
