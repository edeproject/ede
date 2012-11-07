/*
 * $Id$
 *
 * ede-bug-report, a tool to report bugs on EDE bugzilla instance
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>

#include <stdio.h>

#include <FL/fl_draw.H>
#include "PulseProgress.h"

PulseProgress::PulseProgress(int x, int y, int w, int h, const char *l) : 
	Fl_Widget(x, y, w, h, l), cprogress(0), blen(40), slen(5)
{
	box(FL_DOWN_BOX);
	color(FL_BACKGROUND_COLOR, (Fl_Color)146);
	/* show it at the beginning */
	cprogress += blen;
}

void PulseProgress::draw(void) {
	int bx, by, bw, bh;
	int xoff, yoff, woff, hoff;

	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
	bh = Fl::box_dh(box());

	xoff = x() + bx;
	yoff = y() + by;
	woff = w() - bw;
	hoff = h() - bh;

	draw_box(box(), x(), y(), w(), h(), color());

	fl_push_clip(xoff, yoff, woff, hoff);
		Fl_Color c = fl_color();
		fl_color(color2());

		if(cprogress >= woff) {
			/* reverse it */
			cprogress = -cprogress;
		} else if(cprogress == -blen) {
			/* start again after reverse */
			cprogress = blen;
		}

		if(cprogress > 0)
			fl_rectf(xoff - blen + cprogress, yoff, blen, hoff);
		else 
			fl_rectf(xoff - blen - cprogress, yoff, blen, hoff);

		fl_color(c);
	fl_pop_clip();

	labelcolor(fl_contrast(labelcolor(), color()));
	draw_label(x() + bx, y() + by, w() - bw, h() - bh);
}

void PulseProgress::step(void) {
	cprogress += slen;
	redraw();
}

#if 0
static PulseProgress *pp;

void cb(void *) {
	pp->step();
	Fl::repeat_timeout(0.06, cb);
}

int main() {
	Fl_Double_Window *win = new Fl_Double_Window(300, 300);
	win->begin();
		pp = new PulseProgress(10, 10, 200, 25, "Sending report...");
	win->end();

	win->show();

	Fl::add_timeout(0.06, cb);
	return Fl::run();
}
#endif
