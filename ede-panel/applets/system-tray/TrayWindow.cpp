#include <FL/fl_draw.H>
#include <edelib/Debug.h>
#include "TrayWindow.h"

/* scale image by keeping aspect ratio */
static Fl_RGB_Image *aspect_scale(Fl_RGB_Image *orig, int W, int H) {
	float aspect, neww, newh;

	neww = (float)orig->w();
	newh = (float)orig->h();

	if(neww > W) {
		aspect = (float)W / (float)neww;
		neww *= aspect;
		newh *= aspect;
	}

	if(newh > H) {
		aspect = (float)H / (float)newh;
		neww *= aspect;
		newh *= aspect;
	}

	return (Fl_RGB_Image*)orig->copy((int)neww, (int)newh);
}

TrayWindow::TrayWindow(int W, int H) : Fl_Window(W, H), img(0) {
	box(FL_FLAT_BOX);
	color(FL_BLUE);
}

void TrayWindow::set_image(Fl_RGB_Image *im) {
	E_RETURN_IF_FAIL(im != 0);

	/* delete previous one */
	delete img;

	/* do not use full w/h, but leave some room for few pixels around image */
	img = aspect_scale(im, w() - 2, h() - 2);
}

void TrayWindow::draw(void) {
	Fl_Window::draw();
	if(img) {
		int X = w()/2 - img->w()/2;
		int Y = h()/2 - img->h()/2;
		img->draw(X, Y);
	}
}

void TrayWindow::set_tooltip(char *t) {
	if(!t) {
		ttip.clear();
		tooltip(0);
	} else {
		ttip = t;
		tooltip(ttip.c_str());
	}
}
