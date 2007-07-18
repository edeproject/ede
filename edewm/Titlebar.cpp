/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Titlebar.h"
#include "Frame.h"
#include "debug.h"
#include <efltk/fl_draw.h>
#include <efltk/Fl.h>
#include <efltk/Fl_Image.h>
#include <efltk/Fl_Locale.h>
#include <assert.h>

#include "app.xpm"
Fl_Image app_img((const char**)app_xpm);

#define MAX_OF 0.6f
#define MIN_OF 0.5f
#define RES_OF 0.1f
#define vv(x,y) fl_vertex(x,y)

void draw_close(Fl_Color col)
{
	fl_rotate(45);
	fl_color(col);

	vv(-0.9f,-0.12f); vv(-0.9f,0.12f); vv(0.9f,0.12f); vv(0.9f,-0.12f); fl_fill_stroke(FL_DARK3);
	vv(-0.12f,-0.9f); vv(-0.12f,0.9f); vv(0.12f,0.9f); vv(0.12f,-0.9f); fl_fill_stroke(FL_DARK3);
}

void draw_max(Fl_Color col)
{			
	fl_color(col);

	vv(-MAX_OF, -MAX_OF); vv(MAX_OF, -MAX_OF);
	vv(MAX_OF,-MAX_OF+0.4); vv(-MAX_OF,-MAX_OF+0.4);
	fl_fill();

	vv(MAX_OF,-MAX_OF); vv(MAX_OF,MAX_OF);
	vv(-MAX_OF,MAX_OF); vv(-MAX_OF,-MAX_OF);
	fl_stroke();
}

void draw_maximized(Fl_Color col)
{			
	fl_color(col);

	vv(-MAX_OF+RES_OF, -MAX_OF+RES_OF); vv(MAX_OF-RES_OF, -MAX_OF+RES_OF);
	vv(MAX_OF-RES_OF,-MAX_OF+RES_OF); vv(MAX_OF-RES_OF,MAX_OF-RES_OF);
	vv(-MAX_OF+RES_OF,MAX_OF-RES_OF); vv(-MAX_OF+RES_OF,-MAX_OF+RES_OF);
	fl_stroke();

	vv(-MAX_OF+RES_OF+0.2f, -MAX_OF+RES_OF+0.2f); vv(MAX_OF-RES_OF-0.2f, -MAX_OF+RES_OF+0.2f);
	vv(MAX_OF-RES_OF-0.2f,-MAX_OF+RES_OF+0.2f); vv(MAX_OF-RES_OF-0.2f,MAX_OF-RES_OF-0.2f);
	vv(-MAX_OF+RES_OF+0.2f,MAX_OF-RES_OF-0.2f); vv(-MAX_OF+RES_OF+0.2f,-MAX_OF+RES_OF+0.2f);
	fl_stroke();
}


void draw_min(Fl_Color col)
{
	fl_color(col);

	vv(-MIN_OF, MIN_OF); vv(MIN_OF, MIN_OF);
	vv(MIN_OF, MIN_OF+0.3f); vv(-MIN_OF, MIN_OF+0.3f);
	fl_fill();
}

void close_cb(Fl_Widget*, void* t)
{
	Titlebar* tb = (Titlebar*)t;
	assert(tb != NULL);
	tb->on_close();
}

void maximize_cb(Fl_Widget* b, void* t)
{
	Titlebar* tb = (Titlebar*)t;
	assert(tb != NULL);
	tb->on_maximize();
}

void shade_cb(Fl_Widget*, void* t)
{
	Titlebar* tb = (Titlebar*)t;
	assert(tb != NULL);
	tb->on_shade();
}

void minimize_cb(Fl_Widget*, void* t)
{
	Titlebar* tb = (Titlebar*)t;
	assert(tb != NULL);
	tb->on_minimize();
}

void lower_cb(Fl_Widget*, void* t)
{
	Titlebar* tb = (Titlebar*)t;
	assert(tb != NULL);
	tb->on_lower();
}

TitlebarButton::TitlebarButton(int type) :
	Fl_Button(0, 0, 0, 0, 0), 
	button_type(type),
	pos(PLACE_RIGHT)
{
	focus_box(FL_NO_BOX);
	box(FL_FLAT_BOX);
	label_type(FL_SYMBOL_LABEL);

	switch(button_type)
	{
		case TITLEBAR_CLOSE_UP:
			label("@xx");
			break;
		case TITLEBAR_MAX_UP:
			label("@mx");
			break;
		case TITLEBAR_MIN_UP:
			label("@ii");
			break;
	}
}

TitlebarButton::~TitlebarButton()
{
}

/* Place buttons on titlebar. The can be either
 * PLACE_RIGHT (standard in many wm-s) or PLACE_RIGHT
 */
void TitlebarButton::place(int p)
{
	if(p != PLACE_LEFT && p != PLACE_RIGHT)
		pos = PLACE_RIGHT;
	else
		pos = p;
}

Titlebar::Titlebar(Frame* f, int x, int y, int w, int h, const char* l) :
	Fl_Group(x, y, w, h, l), 
	win_x(0),
	win_y(0),
	win_x1(0),
	win_y1(0),
	curr_frame(f),
	minb(TITLEBAR_MIN_UP), 
	maxb(TITLEBAR_MAX_UP),
	closeb(TITLEBAR_CLOSE_UP),
	label_box(new Fl_Box(0, 0, 0, 0)),
	icon_box(new Fl_Box(0, 0, 0, 0)),
	focus_color(FL_GRAY),
	unfocus_color(FL_WHITE)
{
	assert(curr_frame != NULL);

	box(FL_FLAT_BOX);

	fl_add_symbol("xx", draw_close, 1);
	fl_add_symbol("mx", draw_max, 1);
	fl_add_symbol("mz", draw_maximized, 1);
	fl_add_symbol("ii", draw_min, 1);
	label_box->box(FL_FLAT_BOX);
	label_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	label_box->color(FL_GRAY);

	icon_box->image(app_img);

	title_menu = new Fl_Menu_();

	/* XXX: any way this can be better ?
	 * When we change label of existing menu item, menu width will not be
	 * recalculated honoring size of that item. So we must initially fill
	 * them with blanks, which will keep longer labels fully visible.
	 */
	menu_max   = title_menu->add(_("Maximize        "), 0, maximize_cb, this);
	menu_shade = title_menu->add(_("Shade        "), 0, shade_cb, this);
	menu_lower = title_menu->add(_("Lower        "), 0, lower_cb, this);
	menu_min   = title_menu->add(_("Minimize        "), 0, minimize_cb, this, FL_MENU_DIVIDER);
	menu_close = title_menu->add(_("Close        "), 0, close_cb, this);

	//closeb.place(PLACE_LEFT);
	end();

	closeb.callback(close_cb, this);
	//minb.callback(minimize_cb, this);
	minb.callback(shade_cb, this);
	maxb.callback(maximize_cb, this);
	
	color(FL_GRAY);
	//align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	if(curr_frame->frame_type(FrameTypeDialog))
	{
		minb.hide();
		maxb.hide();
	}
}

Titlebar::~Titlebar()
{
}

#define PLACE_BUTTON(btn, leftx, rightx, mid, offset, sz) \
	if(btn.place() == PLACE_LEFT) \
	{\
		btn.resize(leftx, mid, sz, sz); \
		if(btn.visible()) \
			leftx += sz + offset; \
	}\
	if(btn.place() == PLACE_RIGHT) \
	{\
		btn.resize(rightx, mid, sz, sz); \
		if(btn.visible()) \
			rightx -= sz + offset; \
	}

void Titlebar::layout(void)
{
	int initial_offset = 2;   // distance from borders
	int W = w() - box()->dx() - initial_offset;
	int X = box()->dx() + initial_offset;

	int lsize = h() / 2 + 2;
	int offset = 0;
	int mid = 0;
	int sz = h();

	label_size(lsize);

	sz -= 4;                    // button width/height
	mid = 2;
	offset = 2;

	int lx = X + offset;        // left x
	int rx = W - sz - offset;   // right x

	PLACE_BUTTON(closeb, lx, rx, mid, offset, sz)
	PLACE_BUTTON(maxb, lx, rx, mid, offset, sz)
	PLACE_BUTTON(minb, lx, rx, mid, offset, sz)

	//rx -= sz + offset;
	//lx += sz + offset;

	if(icon_box->visible())
	{
		icon_box->resize(lx, mid, sz, sz);
		lx += sz + offset;
	}

	lx += offset;
	rx -= lx;
	label_box->resize(lx, mid, rx, sz);

	fl_font(label_font(), label_size());
	// take a label from titlebar object
	if(!label().empty())
		label_box->label(label());

	Fl_Group::layout();
}

void Titlebar::focus(void)
{
	// TODO: add colors for label_box, buttons
	color(focus_color);
	redraw();
}

void Titlebar::unfocus(void)
{
	// TODO: add colors for label_box, buttons
	color(unfocus_color);
	redraw();
}

void Titlebar::on_close(void)
{
	assert(curr_frame != NULL);
	curr_frame->close_kill();
}

void Titlebar::on_maximize(void)
{
	assert(curr_frame != NULL);

	if(curr_frame->state(FrameStateMaximized))
	{
		curr_frame->restore();
		maxb.label("@mx");
		menu_max->label(_("Maximize"));
	}
	else
	{
		curr_frame->maximize();
		maxb.label("@mz");
		menu_max->label(_("UnMaximize"));
	}
}

void Titlebar::on_minimize(void)
{
	assert(curr_frame != NULL);
	// TODO: not implemented
}

void Titlebar::on_shade(void)
{
	assert(curr_frame != NULL);

	if(curr_frame->state(FrameStateShaded))
	{
		curr_frame->unshade();
		menu_shade->label(_("Shade"));
	}
	else
	{
		curr_frame->shade();
		menu_shade->label(_("UnShade"));
	}
}

void Titlebar::on_lower(void)
{
	curr_frame->lower();
}

int Titlebar::handle(int event)
{
	// check if event is directed to our buttons
	if(event == FL_PUSH || event == FL_RELEASE)
	{
		Fl_Widget* ch = 0;
		for(int i = 0; i < children(); i++)
		{
			ch = child(i);

			// do not send event to label_box 
			if(label_box && ch == label_box)
				continue;

			if(ch && Fl::event_inside(ch->x(), ch->y(), ch->w(), ch->h()))
			{
				ch->send(event);
				return 1;
			}
		}
	}

	// proceed to our events
	switch(event)
	{
		case FL_PUSH:
			ELOG("Titlebar FL_PUSH");

			if(Fl::event_state(FL_BUTTON3))
			{
				title_menu->popup(Fl::event_x_root(), Fl::event_y_root());
				title_menu->redraw();
				return 1;
			}

			// prevent double click
			if(Fl::event_is_click())
				curr_frame->raise();

			win_x = Fl::event_x_root() - curr_frame->x();
			win_y = Fl::event_y_root() - curr_frame->y();
			return 1;

		case FL_DRAG:
			ELOG("Titlebar FL_DRAG");
			// TODO: correct behaviour ???
			if(curr_frame->state(FrameStateMaximized))
				return 1;

			win_x1 = Fl::event_x_root() - win_x;
			win_y1 = Fl::event_y_root() - win_y;

			if(!curr_frame->moving())
			{
				curr_frame->move_start();

				/* We can safely call grab_cursor() as many times
				 * as we want, since it have internal checks
				 *
				 * TODO: add XGrabServer() here
				 */
				curr_frame->set_cursor(CURSOR_MOVE);
				curr_frame->grab_cursor();
				curr_frame->show_coordinates_window();
			}

			curr_frame->move_window(win_x1, win_y1);
			curr_frame->update_coordinates_window();

			return 1;

		case FL_RELEASE:
			ELOG("Titlebar FL_RELEASE");
			if(curr_frame->moving())
			{
				curr_frame->ungrab_cursor();
				curr_frame->set_cursor(CURSOR_DEFAULT);
				curr_frame->move_end();
				curr_frame->hide_coordinates_window();
			}
			return 1;

		case FL_ENTER:
			ELOG("Titlebar::FL_ENTER");
			return 1;
		case FL_LEAVE:
			ELOG("Titlebar::FL_LEAVE");
			return 1;
		default:
			return Fl_Group::handle(event);
	}
	return 0;
}
