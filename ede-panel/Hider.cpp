/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2012 Sanel Zukan
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

#include <unistd.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include <edelib/Nls.h>
#include <edelib/Debug.h>

#include "Hider.h"
#include "Panel.h"

/* delay in secs */
#define PANEL_MOVE_DELAY 0.0015
/* how fast we will move X axis */
#define PANEL_ANIM_SPEED 50 

static void hide_cb(Fl_Widget*, void *h) {
	Hider *hh = (Hider*)h;
	hh->panel_hidden() ? hh->panel_show() : hh->panel_hide();
}

static void animate_cb(void *h) {
	Hider *hh = (Hider*)h;
	hh->animate();
}

Hider::Hider() : Fl_Button(0, 0, 10, 25, "@>"), old_x(0), old_y(0), is_hidden(0), old_px(0), stop_x(0) {
	labelsize(8);
	box(FL_FLAT_BOX);
	tooltip(_("Hide panel"));
	callback(hide_cb, this);
}

void Hider::animate(void) {
	Panel *p = (Panel*)parent();

	if(!panel_hidden()) {
		/* hide */
		if(p->x() < stop_x) {
			int X = p->x() + PANEL_ANIM_SPEED;
			p->position(X, p->y());
			Fl::repeat_timeout(PANEL_MOVE_DELAY, animate_cb, this);
		} else {
			post_hide();
		}
	} else {
		/* show */
		if(p->x() > stop_x) {
			int X = p->x() - PANEL_ANIM_SPEED;
			p->position(X, p->y());
			Fl::repeat_timeout(PANEL_MOVE_DELAY, animate_cb, this);
		} else {
			post_show();
		}
	}
}	

void Hider::panel_hide(void) {
	Panel *p = (Panel*)parent();

	int X, Y, W, H;

	p->screen_size(X, Y, W, H);
	stop_x = X + W - w();
	old_px = p->x();

	Fl::add_timeout(0.1, animate_cb, this);
}

void Hider::post_hide(void) {
	Panel *p = (Panel*)parent();

	/* align to bounds */
	p->position(stop_x, p->y());
	p->allow_drag(false);
	p->apply_struts(false);

	/* hide all children on panel except us */
	for(int i = 0; i < p->children(); i++) {
		if(p->child(i) != this)
			p->child(i)->hide();
	}

	/* append us to the beginning */
	old_x = x();
	old_y = y();

	position(0 + Fl::box_dx(p->box()), y());
	label("@<");

	panel_hidden(1);
	tooltip(_("Show panel"));
}

void Hider::panel_show(void) {
	Fl::add_timeout(0.1, animate_cb, this);
}

void Hider::post_show(void) {
	Panel *p = (Panel*)parent();

	/* align to bounds */
	p->position(old_px, p->y());
	p->allow_drag(true);
	p->apply_struts(true);

	/* show all children on panel */
	for(int i = 0; i < p->children(); i++) {
		if(p->child(i) != this)
			p->child(i)->show();
	}

	/* move ourself to previous position */
	position(old_x, old_y);
	label("@>");

	panel_hidden(0);
	tooltip(_("Hide panel"));
}
