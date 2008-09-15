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

#include "Background.h"
#include <FL/Fl_Shared_Image.H>
#include <edelib/Debug.h>

bool Background::load_images(const char* bpath, const char* ppath) {
	EASSERT(bpath != NULL);
	EASSERT(ppath != NULL);

	fl_register_images();

	// get background image first
	img = Fl_Shared_Image::get(bpath);
	if(!img)
		return false;

	// scale if needed
	if(img->w() != w() && img->h() != h()) {
		Fl_Image* scaled = img->copy(w(), h());
		// Fl_Shared_Image contains correct pointers, so we can replace this
		img = scaled;
	}

	// panel image
	panel_img = Fl_Shared_Image::get(ppath);
	if(!panel_img)
		return false;

	return true;
}

void Background::draw(void) {
	if(img)
		img->draw(x(), y());

	if(panel_img)
		panel_img->draw(x() + panel_img_x, y() + panel_img_y);
}
