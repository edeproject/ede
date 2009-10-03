#include "Applet.h"

#include <ctype.h>
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>

#include <edelib/Debug.h>
#include <edelib/Nls.h>
#include <edelib/Run.h>

#include "edit-clear-icon.h"

EDELIB_NS_USING(run_async)

class QuickLaunch : public Fl_Input {
private:
	Fl_Image *img;
	int img_x, img_y;
public:
	QuickLaunch();

	void draw(void);
	int  handle(int e);
	
	/* a stupid way to avoid draw_box() to draw the image */
	void image2(Fl_Image *im) { img = im; }
	Fl_Image *image2(void) { return img; }
};

static bool empty_string(const char *s) {
	for(const char *ptr = s; ptr && *ptr; ptr++) {
		if(!isspace(*ptr)) 
			return false;
	}

	return true;
}

static void enter_cb(Fl_Widget*, void *o) {
	QuickLaunch *ql = (QuickLaunch*)o;

	if(ql->value() && !empty_string(ql->value()))
		run_async("ede-launch %s", ql->value());
}

QuickLaunch::QuickLaunch() : Fl_Input(0, 0, 130, 25), img(NULL), img_x(0), img_y(0) {
	when(FL_WHEN_ENTER_KEY|FL_WHEN_NOT_CHANGED);
	callback(enter_cb, this);
	tooltip(_("Enter a command to be executed"));

	image2((Fl_RGB_Image*)&image_edit);
}

void QuickLaunch::draw(void) {
	Fl_Boxtype b = box(), oldb;

	int X = x() + Fl::box_dx(b);
	int Y = y() + Fl::box_dy(b);
	int W = w() - Fl::box_dw(b);
	int H = h() - Fl::box_dh(b);

	if(img) {
		W -= img->w() + 6;
		img_x = X + W + 2;
		img_y = (Y + H / 2) - (img->h() / 2);
	}

	if(damage() & FL_DAMAGE_ALL) {
		draw_box(b, color());
		if(img) img->draw(img_x, img_y);
	}

	/* use flat box when text is drawn or there would be visible border line */
	oldb = box();
	box(FL_FLAT_BOX);

	Fl_Input_::drawtext(X, Y, W, H);
	box(oldb);
}

int QuickLaunch::handle(int e) {
	if(!img) 
		goto done;

	switch(e) {
		case FL_ENTER:
		case FL_MOVE:
			if(active_r() && window()) {
				if(Fl::event_inside(img_x, img_y, img->w(), img->h()))
					window()->cursor(FL_CURSOR_DEFAULT);
				else 
					window()->cursor(FL_CURSOR_INSERT);
			}

			return 1;

		case FL_RELEASE:
			if(active_r() && Fl::event_inside(img_x, img_y, img->w(), img->h()))
				value(0);
			/* fallthrough */
	}

done:
	return Fl_Input::handle(e);
}


EDE_PANEL_APPLET_EXPORT (
 QuickLaunch, 
 EDE_PANEL_APPLET_OPTION_ALIGN_LEFT,
 "Quick Launch applet",
 "0.1",
 "empty",
 "Sanel Zukan"
)
