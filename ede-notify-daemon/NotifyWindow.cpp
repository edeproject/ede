/*
 * $Id: ede-panel.cpp 3330 2012-05-28 10:57:50Z karijes $
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
 
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include <edelib/IconLoader.h>
#include <edelib/Nls.h>
#include <edelib/Netwm.h>
#include "NotifyWindow.h"

/* default sizes for window */
#define DEFAULT_W 280
#define DEFAULT_H 75
#define DEFAULT_EXPIRE 2000

EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(netwm_window_set_type)
EDELIB_NS_USING(ICON_SIZE_MEDIUM)
EDELIB_NS_USING(NETWM_WINDOW_TYPE_NOTIFICATION)

extern int FL_NORMAL_SIZE;

static void close_cb(Fl_Widget*, void *w) {
	NotifyWindow *win = (NotifyWindow*)w;
	win->hide();
}

static void timeout_cb(void *w) {
	close_cb(0, w);
}

NotifyWindow::NotifyWindow() : Fl_Window(DEFAULT_W, DEFAULT_H) {
	FL_NORMAL_SIZE = 12;

	timer_set = 0;

	type(NOTIFYWINDOW_TYPE);
	color(FL_BACKGROUND2_COLOR);
	box(FL_BORDER_BOX);
	begin();
		closeb = new Fl_Button(255, 10, 20, 20, "x");
		closeb->box(FL_FLAT_BOX);
		closeb->down_box(FL_DOWN_BOX);
		closeb->color(FL_BACKGROUND2_COLOR);
		closeb->labelsize(12);
		closeb->tooltip(_("Close this notification"));
		closeb->clear_visible_focus();
		closeb->callback(close_cb, this);

		imgbox = new Fl_Box(10, 10, 48, 48);
		imgbox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		summary = new Fl_Multiline_Output(65, 10, 185, 25);
		/* use flat box so text can be drawn correctly */
		summary->box(FL_FLAT_BOX);
		summary->cursor_color(FL_BACKGROUND2_COLOR);

		body = new Fl_Multiline_Output(65, 35, 185, 25);
		/* use flat box so text can be drawn correctly */
		body->box(FL_FLAT_BOX);
		body->cursor_color(FL_BACKGROUND2_COLOR);
	end();

	/*
	 * by default body text is hidden and summary is lowered a little bit
	 * NOTE: I'm using 'resize' as 'position' for Fl_Input/Fl_Output means something different
	 */
	summary->resize(summary->x(), summary->y() + (summary->h() / 2), summary->w(), summary->h());
	body->hide();
	border(0);
}

void NotifyWindow::add_timeout(void) {
	E_DEBUG(E_STRLOC ": adding timer\n");
	if(timer_set) return;

	if(exp == -1) exp = DEFAULT_EXPIRE;
	Fl::add_timeout((double)exp / (double)1000, timeout_cb, this);

	timer_set = 1;
}

void NotifyWindow::remove_timeout(void) {
	E_DEBUG(E_STRLOC ": removing timer\n");
	if(!timer_set) return;

	Fl::remove_timeout(timeout_cb);
	timer_set = 0;
}

void NotifyWindow::set_icon(const char *img) {
	E_RETURN_IF_FAIL(IconLoader::inited());
	E_RETURN_IF_FAIL(img != NULL);

	IconLoader::set(imgbox, img, ICON_SIZE_MEDIUM);
}

void NotifyWindow::set_body(const char *s) {
	summary->resize(summary->x(), summary->y() - (summary->h() / 2), summary->w(), summary->h());

	body->value(s);
	body->show();
}

void NotifyWindow::show(void) {
	/* the case when timer should not be added */
	if(exp != 0)
		add_timeout();

	Fl_Window::show();
	netwm_window_set_type(fl_xid(this), NETWM_WINDOW_TYPE_NOTIFICATION);
}

#define INPUT_VALID(i) ((i)->visible() && (i)->size() > 0)

void NotifyWindow::resize(int X, int Y, int W, int H) {
	/*
	 * do not call further if window is shown: different strategy is needed as every time
	 * window is re-configured, this will be called
	 */
	if(shown()) return;

	/* resize summary if needed */
	if(summary->size() > 0) {
		int fw = 0, fh = 0;
		fl_font(summary->textfont(), summary->textsize());
		fl_measure(summary->value(), fw, fh);

		if(fw > summary->w()) {
			int d = fw - summary->w();
			summary->resize(summary->x(), summary->y(), fw, summary->h());

			/* move X button */
			closeb->position(closeb->x() + d, closeb->y());

			/* resize body too */
			if(INPUT_VALID(body)) body->resize(body->x(), body->y(), body->w() + d, body->h());
			
			W += d;
			/* this depends on window position */
			X -= d;
		}

		if(fh > summary->h()) {
			int d = fh - summary->h();
			summary->resize(summary->x(), summary->y(), summary->w(), fh);

			/* move body down */
			if(INPUT_VALID(body)) body->resize(body->x(), body->y() + d, body->w(), body->h());

			H += d;
			Y -= d;
		}
	}

	/* resize body if needed */
	if(INPUT_VALID(body)) {
		int fw = 0, fh = 0;
		fl_font(body->textfont(), body->textsize());
		fl_measure(body->value(), fw, fh);

		if(fw > body->w()) {
			int d = fw - body->w();
			body->resize(body->x(), body->y(), fw, body->h());

			/* move X button again */
			closeb->position(closeb->x() + d, closeb->y());

			W += d;
			X -= d;
		}

		if(fh > body->h()) {
			int d = fh - body->h();
			body->resize(body->x(), body->y(), body->w(), fh);

			H += d;
			Y -= d;
		}
	}

	Fl_Window::resize(X, Y, W, H);
}

int NotifyWindow::handle(int event) {
	switch(event) {
		case FL_ENTER:
			remove_timeout();
			goto done;
		case FL_LEAVE:
			add_timeout();
			goto done;
	}

done:
	return Fl_Window::handle(event);
}
