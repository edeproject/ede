#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#include <edelib/Debug.h>
#include <edelib/List.h>
#include <edelib/WindowXid.h>
#include <edelib/Resource.h>
#include <edelib/Util.h>

#include "Panel.h"
#include "Netwm.h"

/* empty space from left and right panel border */
#define INITIAL_SPACING 5

/* space between each applet */
#define DEFAULT_SPACING 5

/* default panel height */
#define DEFAULT_PANEL_H 35

#undef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#undef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

EDELIB_NS_USING(list)
EDELIB_NS_USING(Resource)
EDELIB_NS_USING(String)
EDELIB_NS_USING(window_xid_create)
EDELIB_NS_USING(build_filename)

typedef list<Fl_Widget*> WidgetList;
typedef list<Fl_Widget*>::iterator WidgetListIt;

inline bool intersects(Fl_Widget *o1, Fl_Widget *o2) {
	    return (MAX(o1->x(), o2->x()) <= MIN(o1->x() + o1->w(), o2->x() + o2->w()) &&
	            MAX(o1->y(), o2->y()) <= MIN(o1->y() + o1->h(), o2->y() + o2->h()));
}

/* horizontaly centers widget in the panel */
static void center_widget_h(Fl_Widget *o, Panel *self) {
	int ph, wy;

	ph = self->panel_h() / 2;
	wy = ph - (o->h() / 2);
	o->position(o->x(), wy);
}

static void add_from_list(WidgetList &lst, Panel *self, int &X, bool inc) {
	WidgetListIt it = lst.begin(), it_end = lst.end();
	Fl_Widget    *o;

	while(it != it_end) {
		o = *it;

		/* 'inc == false' means we are going from right to left */
		if(!inc)
			X -= o->w();

		/* place it correctly */
		o->position(X, o->y());

		self->add(o);

		if(inc) {
			X += DEFAULT_SPACING;
			X += o->w();
		} else {
			X -= DEFAULT_SPACING;
		}

		it = lst.erase(it);
	}
}

static void move_widget(Panel *self, Fl_Widget *o, int &sx, int &sy) {
	int tx, ty, px, py;
	Fl_Widget *const *a;

	tx = Fl::event_x() - sx;
	ty = Fl::event_y() - sy;

	px = o->x() + tx;
	py = o->y() + ty;

	/* check the bounds */
	if(px < 0)
		px = 0;
	if(px + o->w() > self->panel_w())
		px = self->panel_w() - o->w();
	if(py < 0)
		py = 0;
	if(py + o->h() > self->y() + self->panel_h())
		py = self->y() + self->panel_h() - o->h();
	if(py + o->h() > self->panel_h())
		py = self->panel_h() - o->h();

	/* o->position(px, py); */
	o->position(px, o->y());

	/* find possible collision and move others */
	a = self->array();
	for(int i = self->children(); i--; ) {
		if(o == a[i])
			continue;

		if(intersects(a[i], o)) {
			px = a[i]->x() + tx;
			py = a[i]->y() + ty;

			/* check the bounds */
			if(px < 0)
				px = 0;
			if(px + o->w() > self->panel_w())
				px = self->panel_w() - o->w();
			if(py < 0)
				py = 0;
			if(py + o->h() > self->y() + self->panel_h())
				py = self->y() + self->panel_h() - o->h();
			if(py + o->h() > self->panel_h())
				py = self->panel_h() - o->h();

			/* a[i]->position(px, py); */
			a[i]->position(px, a[i]->y());
		}
	}

	/* update current position */
	sx = Fl::event_x();
	sy = Fl::event_y();

	o->parent()->redraw();
}

void Panel::do_layout(void) {
	E_RETURN_IF_FAIL(mgr.napplets() > 0);

	Fl_Widget     *o;
	unsigned long  opts;
	unsigned int   lsz;
	int            X, W, free_w;

	WidgetList left, right, center, unmanaged, resizable_h;

	for(int i = 0; i < children(); i++) {
		o = child(i);

		/* first center it vertically */
		center_widget_h(o, this);

		/* could be slow, but I'm relaying how number of loaded applets will not be that large */
		if(!mgr.get_applet_options(o, opts)) {
			/* here are put widgets not loaded as applets */
			unmanaged.push_back(o);
			continue;
		}

		if(opts & EDE_PANEL_APPLET_OPTION_RESIZABLE_H)
			resizable_h.push_back(o);

		if(opts & EDE_PANEL_APPLET_OPTION_ALIGN_LEFT) {
			/* first item will be most leftest */
			left.push_back(o);
			continue;
		}

		if(opts & EDE_PANEL_APPLET_OPTION_ALIGN_RIGHT) {
			/* first item will be most rightest */
			right.push_back(o);
			continue;
		}

		/* rest of them */
		center.push_back(o);
	}

	/* make sure we at the end have all widgets, so we can overwrite group array */
	lsz = left.size() + center.size() + right.size() + unmanaged.size();
	E_ASSERT(lsz == (unsigned int)children() && "Size of layout lists size not equal to group size");

	X = INITIAL_SPACING;

	/* 
	 * Call add() on each element, processing lists in order. add() will remove element
	 * in group array and put it at the end of array. At the end, we should have array ordered by
	 * layout flags.
	 */
	add_from_list(left, this, X, true);
	add_from_list(center, this, X, true);
	add_from_list(unmanaged, this, X, true);

	free_w = X;

	/* elements right will be put from starting from the right panel border */
	X = w() - INITIAL_SPACING;
	add_from_list(right, this, X, false);

	/* 
	 * Code for horizontal streching. 
	 *
	 * FIXME: This code pretty sucks and need better rewrite in future.
	 * To work properly, applets that will be streched must be successive or everything will be
	 * messed up. Also, applets that are placed right2left does not work with it; they will be resized to right.
	 */
	if(resizable_h.empty())
		return;

	/* calculate free space for horizontal alignement, per item in resizable_h list */
	free_w = (X - free_w) / resizable_h.size();
	if(!free_w) free_w = 0;

	/* 
	 * since add_from_list() will already reserve some space by current child width and default spacing,
	 * those values will be used again or holes will be made
	 */
	WidgetListIt it = resizable_h.begin(), it_end = resizable_h.end();
	o = resizable_h.front();
	X = o->x();

	while(it != it_end) {
		o = *it;

		W = o->w() + free_w;
		o->resize(X, o->y(), W, o->h());
		X += W + DEFAULT_SPACING;

		it = resizable_h.erase(it);
	}
}

void Panel::show(void) {
	if(shown()) {
		Fl_Window::show();
		return;
	}

	int X, Y, W, H;

	fl_open_display();

	/* position it */
	if(!netwm_get_workarea(X, Y, W, H))
		Fl::screen_xywh(X, Y, W, H);

	resize(X, Y + H - DEFAULT_PANEL_H, W, DEFAULT_PANEL_H);

	screen_x = X;
	screen_y = Y;
	screen_w = W;
	screen_h = H;
	screen_h_half = screen_h / 2;

	do_layout();
	window_xid_create(this, netwm_make_me_dock);
}

int Panel::handle(int e) {
	switch(e) {
		case FL_SHOW: {
			int ret = PanelWindow::handle(e);
			position(0, screen_h - h());

			if(shown())
				netwm_set_window_strut(this, 0, 0, 0, h());

			return ret;
		}

		case FL_PUSH:
			clicked = Fl::belowmouse();

			if(clicked == this)
				clicked = 0;
			else if(clicked && clicked->takesevents())
				clicked->handle(e);

			/* record push position for possible child drag */
			sx = Fl::event_x();
			sy = Fl::event_y();
			return 1;

		case FL_DRAG: {
#if 0
			if(clicked && clicked->takesevents()) {
				cursor(FL_CURSOR_MOVE);
				/* moving the child */
				//move_widget(this, clicked, sx, sy);
				clicked->handle(e);
			} else {
#endif
				cursor(FL_CURSOR_MOVE);
				/* are moving the panel; only vertical moving is supported */

				/* snap it to the top or bottom, depending on pressed mouse location */
				if(Fl::event_y_root() <= screen_h_half && y() > screen_h_half) {
					position(screen_x, screen_y);
					netwm_set_window_strut(this, 0, 0, h(), 0);
				} 
				
				if(Fl::event_y_root() > screen_h_half && y() < screen_h_half) {
					/* TODO: use area x and area y */
					position(screen_x, screen_h - h());
					netwm_set_window_strut(this, 0, 0, 0, h());
				}
			// }

			return 1;
		}

		case FL_RELEASE:
			cursor(FL_CURSOR_DEFAULT);

			if(clicked && clicked->takesevents()) {
				clicked->handle(e);
				clicked = 0;
			}

			return 1;

		case FL_KEYBOARD:
			/* do not quit on Esc key */
			if(Fl::event_key() == FL_Escape)
				return 1;

			/* fallthrough */
	}

	return Fl_Window::handle(e);
}

void Panel::load_applets(void) {
	/* FIXME: hardcoded order */
	static const char *applets[] = {
		"start_menu.so",
		"quick_launch.so",
		"pager.so",
		"clock.so",
		"taskbar.so",
		0
	};

	String dir = Resource::find_data("panel-applets");
	if(dir.empty())
		return;

	String tmp;
	for(int i = 0; applets[i]; i++) {
		tmp = build_filename(dir.c_str(), applets[i]);
		mgr.load(tmp.c_str());
	}

	mgr.fill_group(this);

#if 0
	mgr.load("./applets/start-menu/start_menu.so");
	mgr.load("./applets/quick-launch/quick_launch.so");
	mgr.load("./applets/pager/pager.so");
	mgr.load("./applets/clock/clock.so");
	mgr.load("./applets/taskbar/taskbar.so");
	mgr.fill_group(this);
#endif
}
