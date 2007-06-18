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

#include "NotifyBox.h"
#include "eiconman.h"
#include <edelib/Debug.h>

#include <FL/Fl.h>
#include <FL/fl_draw.h>
#include <FL/Fl_Group.h>

#define MAX_LABEL_WIDTH 100

NotifyBox::NotifyBox() : Fl_Box(0, 0, 0, 0) { 
	lwidth = lheight = 0;
	is_shown = false;
	in_animate = false;
	box(FL_FLAT_BOX);
	color(FL_RED);
	align(FL_ALIGN_WRAP);
}

NotifyBox::~NotifyBox() {
	EDEBUG("NotifyBox::~NotifyBox()\n");
}

void NotifyBox::update_label_size(void) {
    lwidth = MAX_LABEL_WIDTH;
    lheight= 0;
	fl_font(labelfont(), labelsize());

	fl_measure(label(), lwidth, lheight, align());

    lwidth  += 10;
	lheight += 10;
}

void NotifyBox::show(void) {
	update_label_size();
	resize(x(), y()-lheight, lwidth, lheight);

	EDEBUG("%i %i\n", w(), h());

	Fl_Box::show();

	if(!in_animate && !is_shown)
		Fl::add_timeout(TIMEOUT, animate_show_cb, this);
}

void NotifyBox::hide(void) {
	if(!in_animate && is_shown)
		Fl::add_timeout(TIMEOUT, animate_hide_cb, this);

	Fl_Box::hide();
}

void NotifyBox::animate_show(void) {
	if(y() < 0) {
		position(x(), y() + 1);
		EDEBUG("y: %i\n", y());
		redraw();
		Fl::repeat_timeout(TIMEOUT, animate_show_cb, this);
		in_animate = true;
	} else {
		Fl::remove_timeout(animate_show_cb);
		in_animate = false;
		is_shown = true;

		// add visibility timeout
		Fl::add_timeout(TIME_SHOWN, visible_timeout_cb, this);
	}
}

void NotifyBox::animate_hide(void) {
	if(y() > -lheight) {
		position(x(), y() - 1);
		redraw();
		Desktop::instance()->redraw();
		Fl::repeat_timeout(TIMEOUT, animate_hide_cb, this);
		in_animate = true;
	} else {
		Fl::remove_timeout(animate_hide_cb);
		Fl::remove_timeout(visible_timeout_cb);
		in_animate = false;
		is_shown = false;
	EDEBUG("!!!!!!!!!!!!!!!!!\n");
	}
}

void NotifyBox::visible_timeout(void) {
	EDEBUG("XXXXXXXXXXXXXXXXX\n");
	animate_hide();
}
