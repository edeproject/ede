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

#include "Wallpaper.h"

#include <edelib/Debug.h>
#include <FL/Fl_Shared_Image.h>
#include <FL/fl_draw.h>

Wallpaper::Wallpaper(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H, 0), img(NULL), tiled(false) { 
}

Wallpaper::~Wallpaper() { 
}

bool Wallpaper::set(const char* path) {
	EASSERT(path != NULL);

	tiled = false;

	Fl_Image* i = Fl_Shared_Image::get(path);
	if(!i)
		return false;
	img = i;
	image(img);

	return true;
}

bool Wallpaper::set_tiled(const char* path) {
	bool ret = set(path);
	if(ret)
		tiled = true;
	return ret;
}

void Wallpaper::draw(void) {
	if(!image())
		return;

	int ix, iy, iw, ih;
	Fl_Image* im = image();

	iw = im->w();
	ih = im->h();

	if(iw == 0 || ih == 0)
		return;

	if(ih < h() || iw < w()) {
		if(tiled) { 
			// tile image across the box
			fl_push_clip(x(), y(), w(), h());
				int tx = x() - (x() % iw);
				int ty = y() - (y() % ih);
				int tw = w() + tx;
				int th = h() + ty;

				for(int j = 0; j < th; j += ih)
					for(int i = 0; i < tw; i += iw)
						im->draw(i, j);

			fl_pop_clip();
			return;
		} else {
			// center image in the box
			ix = (w()/2) - (iw/2);
			iy = (h()/2) - (ih/2);
			ix += x();
			iy += y();
		}
	} else {
		ix = x();
		iy = y();
	}

	im->draw(ix, iy);
}
