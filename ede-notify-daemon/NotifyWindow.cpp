#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include <edelib/IconLoader.h>
#include <edelib/Nls.h>
#include "NotifyWindow.h"

/* default sizes for window */
#define DEFAULT_W 280
#define DEFAULT_H 75
#define DEFAULT_EXPIRE 2000

EDELIB_NS_USING(IconLoader)
EDELIB_NS_USING(ICON_SIZE_MEDIUM)

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

	type(NOTIFYWINDOW_TYPE);
	color(FL_BACKGROUND2_COLOR);
	box(FL_THIN_UP_BOX);
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

		body = new Fl_Multiline_Output(65, 31, 185, 25);
		/* use flat box so text can be drawn correctly */
		body->box(FL_FLAT_BOX);
		body->cursor_color(FL_BACKGROUND2_COLOR);
	end();
	border(0);
}

void NotifyWindow::set_icon(const char *img) {
	E_RETURN_IF_FAIL(IconLoader::inited());
	E_RETURN_IF_FAIL(img != NULL);

	IconLoader::set(imgbox, img, ICON_SIZE_MEDIUM);
}

void NotifyWindow::show(void) {
	if(exp != 0) {
		if(exp == -1) exp = DEFAULT_EXPIRE;
		Fl::add_timeout((double)exp / (double)1000, timeout_cb, this);
	}

	Fl_Window::show();
}

void NotifyWindow::resize(int X, int Y, int W, int H) {
	/* resize summary if needed */
	if(summary->value()) {
		int fw = 0, fh = 0;
		fl_font(summary->textfont(), summary->textsize());
		fl_measure(summary->value(), fw, fh);

		if(fw > summary->w()) {
			int d = fw - summary->w();
			summary->size(fw, summary->h());

			/* move X button */
			closeb->position(closeb->x() + d, closeb->y());

			/* resize body too */
			body->size(body->w() + d, body->h());
			
			W += d;
			/* this depends on window position */
			X -= d;
		}

		if(fh > summary->h()) {
			int d = fh - summary->h();
			summary->size(summary->w(), fh);

			/* move body down */
			body->position(body->x(), body->y() + d);

			H += d;
			Y -= d;
		}
	}

	/* resize body if needed */
	if(body->value()) {
		int fw = 0, fh = 0;
		fl_font(body->textfont(), body->textsize());
		fl_measure(body->value(), fw, fh);

		if(fw > body->w()) {
			int d = fw - body->w();
			body->size(fw, body->h());

			/* move X button again */
			closeb->position(closeb->x() + d, closeb->y());

			W += d;
			X -= d;
		}

		if(fh > body->h()) {
			int d = fh - body->h();
			body->size(body->w(), fh);

			H += d;
			Y -= d;
		}
	}

	Fl_Window::resize(X, Y, W, H);
}
