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

#ifndef __WALLPAPER_H__
#define __WALLPAPER_H__

#include <X11/Xlib.h>  // XImage, Pixmap
#include <FL/Fl_Box.H>

/*
 * Class responsible for displaying images at background
 * their scaling (TODO), caching(TODO) and making coffee at the spare time.
 */
class Wallpaper : public Fl_Box { 
private:
	Pixmap    rootpmap_pixmap;
	bool tiled;
	void set_rootpmap(void);

public:
	Wallpaper(int X, int Y, int W, int H);
	~Wallpaper();

	bool set(const char* path);
	bool set_tiled(const char* path);
	virtual void draw(void);
	virtual int handle(int event);
};

#endif
