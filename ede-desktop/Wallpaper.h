/*
 * $Id$
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __WALLPAPER_H__
#define __WALLPAPER_H__

#include <X11/Xlib.h>  // XImage, Pixmap
#include <FL/Fl_Box.H>

enum WallpaperState {
	WALLPAPER_CENTER,
	WALLPAPER_STRETCH,
	WALLPAPER_TILE
};

class Wallpaper : public Fl_Box { 
private:
	Pixmap         rootpmap_pixmap;
	WallpaperState state;

	void set_rootpmap(void);
public:
	Wallpaper(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H), rootpmap_pixmap(0), state(WALLPAPER_CENTER) { }
	~Wallpaper();

	bool load(const char* path, WallpaperState s);

	virtual void draw(void);
	virtual int handle(int event);
};

#endif
