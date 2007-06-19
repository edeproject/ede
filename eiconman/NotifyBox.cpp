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

#define MAX_LABEL_WIDTH 200

#define TIMEOUT     (1.0f/60.0f)
#define TIME_SHOWN  3.0f

#define STATE_SHOWING 1
#define STATE_HIDING  2
#define STATE_DONE    3

NotifyBox::NotifyBox(int aw, int ah) : Fl_Box(0, 0, 0, 0) { 
	area_w = aw;
	area_h = ah;
	lwidth = lheight = 0;
	is_shown = false;
	state = 0;

	box(FL_BORDER_BOX);
	color(FL_WHITE);
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
	if(shown())
		return;

	if(state == STATE_HIDING)
		return;

	update_label_size();

	// center box
	int x_pos = (area_w/2) - (lwidth/2);
	resize(x_pos, y() - lheight, lwidth, lheight);

	Fl_Box::show();

	state = STATE_SHOWING;
	Fl::add_timeout(TIMEOUT, animate_cb, this);
}

void NotifyBox::hide(void) {
	if(!shown())
		return;

	// explicitely remove timer when hide() is called
	Fl::remove_timeout(animate_cb);

	Fl_Box::hide();
	is_shown = false;
	state = 0;
}

void NotifyBox::label(const char* l) {
	Fl_Box::label(l);
}

void NotifyBox::copy_label(const char* l) {
	Fl_Box::copy_label(l);
}

const char* NotifyBox::label(void) {
	return Fl_Box::label();
}

void NotifyBox::animate(void) {
	if(state == STATE_SHOWING) {
		if(y() < 0) {
			position(x(), y() + 1);
			redraw();
			//EDEBUG("SHOWING...\n");
			Fl::repeat_timeout(TIMEOUT, animate_cb, this);
			state = STATE_SHOWING;
		} else {
			state = STATE_DONE;
			is_shown = true;

			// now set visible timeout, which will procede to hiding()
			visible_timeout();
		}
	} else if(state == STATE_HIDING) {
		if(y() > -lheight) {
			position(x(), y() - 1);
			//redraw();
			//EDEBUG("%i %i\n", x(), y());
			// this will prevent redrawing whole screen
			Desktop::instance()->damage(FL_DAMAGE_ALL, x(), 0, w(), h());

			//EDEBUG("HIDING...\n");
			Fl::repeat_timeout(TIMEOUT, animate_cb, this);
		} else {
			state = STATE_DONE;
			is_shown = false;
		}
	}
}

void NotifyBox::visible_timeout(void) {
	state = STATE_HIDING;
	Fl::repeat_timeout(TIME_SHOWN, animate_cb, this);
}
