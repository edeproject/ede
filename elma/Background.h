/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __BACKGROUND_H__
#define __BACKGROUND_H__

#include <FL/Fl_Box.H>

class Fl_Image;

/*
 * Background composer; it will compose background image
 * with panel image and blend panel image on background if applicable.
 *
 * This class is intentionaly created (instead everything plop inside
 * one class) so in the future I can easily add scaled image smoothing 
 * and (optinaly) better blending.
 */
class Background : public Fl_Box {
	private:
		Fl_Image* img;
		Fl_Image* panel_img;

		int panel_img_x;
		int panel_img_y;
	public:
		Background(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H, 0), 
		img(0), panel_img(0), panel_img_x(0), panel_img_y(0) { }

		~Background() { }

		bool load_images(const char* bpath, const char* ppath);
		void panel_pos(int X, int Y) { panel_img_x = X; panel_img_y = Y; }
		virtual void draw(void);
};

#endif
