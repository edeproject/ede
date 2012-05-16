#include "Applet.h"
#include "Panel.h"

#include <unistd.h>
#include <FL/Fl_Button.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <edelib/Nls.h>
#include <edelib/Debug.h>

#include <FL/x.H>

/* delay in secs */
#define PANEL_MOVE_DELAY 0.0015
/* how fast we will move X axis */
#define PANEL_ANIM_SPEED 50 

static void hide_cb(Fl_Widget*, void *h);

class Hider : public Fl_Button {
private:
	int old_x, old_y, is_hidden, old_px, stop_x;
public:
	Hider() : Fl_Button(0, 0, 10, 25, "@>"), old_x(0), old_y(0), is_hidden(0), old_px(0), stop_x(0) {
		labelsize(8);
		box(FL_FLAT_BOX);
		tooltip(_("Hide panel"));
		callback(hide_cb, this);
	}

	int  panel_hidden(void) { return is_hidden; }
	void panel_hidden(int s) { is_hidden = s; }

	void panel_show(void);
	void panel_hide(void);
	void post_show(void);
	void post_hide(void);
	void animate(void);
};

static void hide_cb(Fl_Widget*, void *h) {
	Hider *hh = (Hider*)h;
	if(hh->panel_hidden())
		hh->panel_show();
	else
		hh->panel_hide();
}

static void animate_cb(void *h) {
	Hider *hh = (Hider*)h;
	hh->animate();
}

void Hider::animate(void) {
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);

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
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);
	int X, Y, W, H;

	p->screen_size(X, Y, W, H);
	stop_x = X + W - w();
	old_px = p->x();

	Fl::add_timeout(0.1, animate_cb, this);
}

void Hider::post_hide(void) {
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);

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
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);

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

EDE_PANEL_APPLET_EXPORT (
 Hider, 
 EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT,
 "Hide-panel applet",
 "0.1",
 "empty",
 "Sanel Zukan"
)
