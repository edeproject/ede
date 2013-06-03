/*
 * $Id$
 *
 * Copyright (C) 2012-2013 Sanel Zukan
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

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <X11/Xproto.h>

#include <edelib/Debug.h>
#include <edelib/List.h>
#include <edelib/WindowXid.h>
#include <edelib/Resource.h>
#include <edelib/Util.h>
#include <edelib/Netwm.h>
#include <edelib/Directory.h>
#include <edelib/StrUtil.h>

#include "Panel.h"
#include "Hider.h"

/* empty space from left and right panel border */
#define INITIAL_SPACING 5

/* space between each applet */
#define DEFAULT_SPACING 5

/* default panel height */
#define DEFAULT_PANEL_H 35

#define APPLET_EXTENSION ".so"

#undef MIN
#undef MAX
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

EDELIB_NS_USING_LIST(10, (list,
						  Resource,
						  String,
						  window_xid_create,
						  str_trim,
						  netwm_window_set_strut,
						  netwm_window_remove_strut,
						  netwm_window_set_type,
						  netwm_workarea_get_size,
						  NETWM_WINDOW_TYPE_DOCK)) 

typedef list<Fl_Widget*> WidgetList;
typedef list<Fl_Widget*>::iterator WidgetListIt;

/* used only insid x_events */
static Panel *gpanel;

inline bool intersects(Fl_Widget *o1, Fl_Widget *o2) {
	    return (MAX(o1->x(), o2->x()) <= MIN(o1->x() + o1->w(), o2->x() + o2->w()) &&
	            MAX(o1->y(), o2->y()) <= MIN(o1->y() + o1->h(), o2->y() + o2->h()));
}

static int xerror_handler(Display *d, XErrorEvent *e) {
	if(e->request_code == X_GetImage)
		return 0;

	char buf[128];

	/* 
	 * construct the similar message format like X11 is using by default, but again, little
	 * bit different so we knows it comes from here
	 */
	snprintf(buf, sizeof(buf), "%d", e->request_code);

	XGetErrorDatabaseText(d, "XRequest", buf, "%d", buf, sizeof(buf));
	fprintf(stderr, "%s: ", buf);

	XGetErrorText(d, e->error_code, buf, sizeof(buf));
	fprintf(stderr, "%s\n", buf);

	XGetErrorDatabaseText(d, "XlibMessage", "ResourceID", "%d", buf, sizeof(buf));
	fprintf(stderr, " ");
	fprintf(stderr, buf, e->resourceid);
	fprintf(stderr, "\n");

	return 0;
}

static void make_me_dock(Fl_Window *win) {
	netwm_window_set_type(fl_xid(win), NETWM_WINDOW_TYPE_DOCK);
}

static int x_events(int ev) {
	/*
	 * This is quite stupid to do, but hopefully very reliable. When screen is resized
	 * root window was resized too and using those sizes, panel is placed. Tracking workarea changes
	 * isn't much of use, as setting struts dimensions will affect workarea dimension too.
	 */
	if(fl_xevent->type == ConfigureNotify &&
	   (fl_xevent->xconfigure.window == RootWindow(fl_display, fl_screen))) {
			gpanel->update_size_and_pos(false, false,
										fl_xevent->xconfigure.x,
										fl_xevent->xconfigure.y,
										fl_xevent->xconfigure.width,
										fl_xevent->xconfigure.height);
	}

	/* let others receive the same event */
	return 0;
}

/* horizontaly centers widget in the panel */
static void fix_widget_h(Fl_Widget *o, Panel *self) {
	int ph, wy, H = self->h() - 10;

	ph = self->panel_h() / 2;
	wy = ph - (H / 2);
	o->resize(o->x(), wy, o->w(), H);
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

#if 0
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
#endif

Panel::Panel() : PanelWindow(300, 30, "ede-panel") {
	gpanel = this;
	hider = NULL;

	clicked = 0; 
	sx = sy = 0;
	vpos = PANEL_POSITION_BOTTOM;
	screen_x = screen_y = screen_w = screen_h = screen_h_half = 0;
	width_perc = 100;
	can_move_widgets = false;
	can_drag = true;

	box(FL_UP_BOX); 
	read_config();
}

void Panel::read_config(void) {
	Resource r;
	if(!r.load("ede-panel"))
		return;

	int tmp;
	if(r.get("Panel", "position", tmp, PANEL_POSITION_BOTTOM)) {
		if((tmp != PANEL_POSITION_TOP) && (tmp != PANEL_POSITION_BOTTOM))
			tmp = PANEL_POSITION_BOTTOM;
		vpos = tmp;
	}

	if(r.get("Panel", "width", tmp, 100)) {
		if(tmp > 100) tmp = 100;
		else if(tmp < 10) tmp = 10;

		width_perc = tmp;
	}
	
	/* small button on the right edge for panel sliding */
	r.get("Panel", "hider", tmp, 1);
	if(tmp) {
		hider = new Hider();
		add(hider);
	}
	
	char buf[128];
	if(r.get("Panel", "applets", buf, sizeof(buf)))
		load_applets(buf);
	else
		load_applets();
}

void Panel::save_config(void) {
	Resource r;
	/* use whatever was explicitly stored in configuration */
	r.load("ede-panel");

	r.set("Panel", "position", vpos);
	r.save("ede-panel");
}

void Panel::do_layout(void) {
	E_RETURN_IF_FAIL(children() > 0);

	Fl_Widget     *o;
	unsigned long  opts;
	unsigned int   lsz;
	int            X, W, free_w;

	WidgetList left, right, center, unmanaged, resizable_h;

	for(int i = 0; i < children(); i++) {
		o = child(i);

		/* first resize it to some reasonable height and center it vertically */
		fix_widget_h(o, this);

		/* manage hider specifically */
		if(hider && o == hider) {
			right.push_back(o);
			continue;
		}

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
	 * FIXME: This code pretty sucks and need better rewrite in the future.
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

void Panel::set_strut(short state) {
	if(state & PANEL_STRUT_REMOVE)
		netwm_window_remove_strut(fl_xid(this));

	/* no other flags */
	if(state == PANEL_STRUT_REMOVE) return;

	int sizes[4] = {0, 0, 0, 0};

	if(state & PANEL_STRUT_BOTTOM)
		sizes[3] = h();
	else
		sizes[2] = h();

	/* TODO: netwm_window_set_strut_partial() */
	netwm_window_set_strut(fl_xid(this), sizes[0], sizes[1], sizes[2], sizes[3]);
}

void Panel::show(void) {
	if(shown()) return;

	/* 
	 * hush known FLTK bug with XGetImage; a lot of errors will be print when menu icons goes
	 * outside screen; this also make ede-panel, at some point, unresponsible
	 */
	XSetErrorHandler((XErrorHandler) xerror_handler);
	update_size_and_pos(true, true);

	/* collect messages */
	Fl::add_handler(x_events);
}

void Panel::hide(void) {
	Fl::remove_handler(x_events);
	set_strut(PANEL_STRUT_REMOVE);

	/* clear loaded widgets */
	mgr.clear(this);

	/* clear whatever was left out, but was not part of applet manager */
	clear();

	save_config();
	Fl_Window::hide();
}

void Panel::update_size_and_pos(bool create_xid, bool update_strut) {
	int X, Y, W, H;

	/* figure out screen dimensions */
	if(!netwm_workarea_get_size(X, Y, W, H))
		Fl::screen_xywh(X, Y, W, H);

	update_size_and_pos(create_xid, update_strut, X, Y, W, H);
}

void Panel::update_size_and_pos(bool create_xid, bool update_strut, int X, int Y, int W, int H) {
	/* do not update ourself if we screen sizes are the same */
	if(screen_x == X && screen_y == Y && screen_w == W && screen_h == H)
		return;

	screen_x = X;
	screen_y = Y;
	screen_w = W;
	screen_h = H;
	screen_h_half = screen_h / 2;

	/* calculate panel percentage width if given */
	if(width_perc < 100) {
		W = (width_perc * screen_w) / 100;
		X = (screen_w / 2) - (W / 2);
	}

	/* set size as soon as possible, since do_layout() depends on it */
	size(W, DEFAULT_PANEL_H);

	do_layout();

	if(create_xid) window_xid_create(this, make_me_dock);

	/* position it, this is done after XID was created */
	if(vpos == PANEL_POSITION_BOTTOM) {
		position(X, screen_h - h());
		if(width_perc >= 100 && update_strut)
			set_strut(PANEL_STRUT_REMOVE | PANEL_STRUT_BOTTOM);
	} else {
		/* FIXME: this does not work well with edewm (nor pekwm). kwin do it correctly. */
		position(X, Y);
		if(width_perc >= 100 && update_strut)
			set_strut(PANEL_STRUT_REMOVE | PANEL_STRUT_TOP);
	}
}

int Panel::handle(int e) {
	switch(e) {
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
			if(!can_drag) return 1;

			/* are moving the panel; only vertical moving is supported */
			cursor(FL_CURSOR_MOVE);

			/* snap it to the top or bottom, depending on pressed mouse location */
			if(Fl::event_y_root() <= screen_h_half && y() > screen_h_half) {
				position(x(), screen_y);
				if(width_perc >= 100)
					set_strut(PANEL_STRUT_TOP);
				vpos = PANEL_POSITION_TOP;
			} 
				
			if(Fl::event_y_root() > screen_h_half && y() < screen_h_half) {
				position(x(), screen_h - h());
				if(width_perc >= 100)
					set_strut(PANEL_STRUT_BOTTOM);
				vpos = PANEL_POSITION_BOTTOM;
			}

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

void Panel::load_applets(const char *applets) {
	/*
	 * Hardcoded order, unless configuration file was found. For easier and uniform parsing
	 * (similar string is expected from configuration), fallback is plain string.
	 */
	static const char *fallback =
		"start_menu,"
		"quick_launch,"
		"pager,"
		"clock,"
		"taskbar,"
		"keyboard_layout,"
		"battery_monitor,"
		"cpu_monitor,"
#ifdef __linux__
		"mem_monitor,"
#endif
		"system_tray";
	
	String dir = Resource::find_data("panel-applets");
	E_RETURN_IF_FAIL(!dir.empty());
	
	char *dup = strdup(applets ? applets : fallback);
	E_RETURN_IF_FAIL(dup != NULL);

	char path[PATH_MAX];
	for(char *tok = strtok(dup, ","); tok; tok = strtok(NULL, ",")) {
		tok = str_trim(tok);

#ifndef EDE_PANEL_LOCAL_APPLETS
		snprintf(path, sizeof(path), "%s" E_DIR_SEPARATOR_STR "%s" APPLET_EXTENSION, dir.c_str(), tok);
#else
		/* only for testing, so path separator is hardcoded */
		snprintf(path, sizeof(path), "./applets/%s/%s" APPLET_EXTENSION, dir.c_str(), tok);
#endif
		mgr.load(path);
	}

	free(dup);
	mgr.fill_group(this);
}

/* TODO: can be better */
void Panel::apply_struts(bool apply) {
	if(!(width_perc >= 100)) return;

	int bottom = 0, top = 0;

	if(vpos == PANEL_POSITION_TOP)
		top = !apply ? 1 : h();
	else
		bottom = !apply ? 1 : h();

	netwm_window_set_strut(fl_xid(this), 0, 0, top, bottom);
}
