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

#ifndef __WALLPAPER_H__
#define __WALLPAPER_H__

#include <FL/Fl_Box.h>
#include <FL/Fl_Image.h>

#include <X11/Xlib.h>  // XImage, Pixmap

/*
 * Class responsible for displaying images at background
 * their scaling (TODO), caching(TODO) and making coffee at spear time.
 */
class Wallpaper : public Fl_Box { 
	private:
		XImage*   rootpmap_image;
		Pixmap    rootpmap_pixmap;
		Fl_Image* img;
		bool tiled;
		void set_rootpmap(void);

	public:
		Wallpaper(int X, int Y, int W, int H);
		~Wallpaper();

		bool set(const char* path);
		bool set_tiled(const char* path);
		virtual void draw(void);
};

#endif
