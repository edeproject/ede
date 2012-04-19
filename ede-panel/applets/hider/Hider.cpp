#include "Applet.h"
#include "Panel.h"

#include <unistd.h>
#include <FL/Fl_Button.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <edelib/Nls.h>
#include <edelib/Debug.h>

#include <FL/x.H>

/* delay in msecs */
#define PANEL_MOVE_DELAY 200

static void hide_cb(Fl_Widget*, void *h);

class Hider : public Fl_Button {
private:
	int old_x, old_y, is_hidden, old_px;
public:
	Hider() : Fl_Button(0, 0, 10, 25, "@>"), old_x(0), old_y(0), is_hidden(0), old_px(0) {
		labelsize(8);
		box(FL_FLAT_BOX);
		tooltip(_("Hide panel"));
		callback(hide_cb, this);
	}

	void panel_hide(void);
	void panel_show(void);
	int  panel_hidden(void) { return is_hidden; }
};

static void hide_cb(Fl_Widget*, void *h) {
	Hider *hh = (Hider*)h;
	if(hh->panel_hidden())
		hh->panel_show();
	else
		hh->panel_hide();
}

void Hider::panel_hide(void) {
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);
	int X, Y, W, H, end;

	p->screen_size(X, Y, W, H);
	end = X + W - w();
	old_px = p->x();

	for(int i = p->x(); i < end; i+=10) {
		p->position(i, p->y());
		usleep(PANEL_MOVE_DELAY);
		XFlush(fl_display);
	}

	/* align to bounds */
	p->position(end, p->y());
	p->allow_drag(false);
	p->apply_struts(false);

	/* hide all children on panel */
	for(int i = 0; i < p->children(); i++) {
		if(p->child(i) != this)
			p->child(i)->hide();
	}

	/* append us to the beginning */
	old_x = x();
	old_y = y();

	position(0 + Fl::box_dx(p->box()), y());
	label("@<");

	is_hidden = 1;
	tooltip(_("Show panel"));
}

void Hider::panel_show(void) {
	Panel *p = EDE_PANEL_GET_PANEL_OBJECT(this);

	for(int i = p->x(); i > old_px; i-=10) {
		p->position(i, p->y());
		usleep(PANEL_MOVE_DELAY);
		XFlush(fl_display);
	}

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

	is_hidden = 0;
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
