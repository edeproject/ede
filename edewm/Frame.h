/*
 * $Id: Frame.h 1736 2006-08-19 00:38:53Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef _FRAME_H_
#define _FRAME_H_

#include <X11/Xlib.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Box.h>

#include "Cursor.h"

// frame states (internal)
#define FrameStateNormal         (1 << 0)    // neither of below states
#define FrameStateUnmapped       (1 << 1)    // or Withdrawn
#define FrameStateDestroyed      (1 << 2)    // destroyed window, handled by WindowManager
#define FrameStateFocused        (1 << 3)

// frame states (netwm)
#define FrameStateModal          (1 << 4)    // transients
#define FrameStateSticky         (1 << 5)
#define FrameStateShaded         (1 << 6)
#define FrameStateHidden         (1 << 7)    // or iconized
#define FrameStateIconized       FrameStateHidden

/* These two types are used to emit _NET_WM_STATE_ABOVE and _NET_WM_STATE_BELOW.
 * For me it is unclear why they didn't called _NET_WM_STATE_ALWAYS_(ABOVE,BELOW)
 * since all wm's set them in this state (except transient who are always above all
 * windows), and stacking order can't be changed. Anyway, enjoy with more apropriate
 * names.
 */
#define FrameStateAlwaysAbove    (1 << 8)
#define FrameStateAlwaysBelow    (1 << 9)

#define FrameStateMaximizedHorz  (1 << 10)
#define FrameStateMaximizedVert  (1 << 11)
#define FrameStateMaximized      (FrameStateMaximizedHorz | FrameStateMaximizedVert)

// some options frame can have (or their combinations)
/*
#define FrameOptSkipList         (1 << 0)    // some reason why not in list
#define FrameOptBorder           (1 << 1)
#define FrameOptThinBorder       (1 << 2)
#define FrameOptTitlebar         (1 << 3)
#define FrameOptSysmenu          (1 << 4)    // what a hell !?!?
#define FrameOptMinimize         (1 << 5)
#define FrameOptMaxmize          (1 << 6)
#define FrameOptClose            (1 << 7)
#define FrameOptResize           (1 << 8)
#define FrameOptMove             (1 << 9)
#define FrameOptTakeFocus        (1 << 10)
#define FrameOptKeepAspect       (1 << 11)
*/

#define FrameOptSkipList         (1 << 0)
#define FrameOptHaveBorder       (1 << 1)
#define FrameOptHaveTitlebar     (1 << 2)
#define FrameOptResizeable       (1 << 3)
#define FrameOptMoveable         (1 << 4)
#define FrameOptCloseable        (1 << 5)
#define FrameOptHideable         (1 << 6)
#define FrameOptTakeFocus        (1 << 7)
#define FrameOptKeepAspect       (1 << 8)

/* some events echo UnmapNotify, like ReparentNotify, and if we
 * start wm on existing X session with open windows, this flag will
 * prevent they be unmapped
 */
#define FrameOptIgnoreUnmap      (1 << 12)

// frame type
#define FrameTypeNormal          0
#define FrameTypeDialog          1
#define FrameTypeSplash          2
#define FrameTypeDock            3
#define FrameTypeToolbar         4
#define FrameTypeMenu            5
#define FrameTypeUtil            6
#define FrameTypeDesktop         7

// type of resizing; means from which
// side of window is resize doing
#define ResizeTypeNone           (1 << 0)
#define ResizeTypeLeft           (1 << 1)
#define ResizeTypeRight          (1 << 2)
#define ResizeTypeUp             (1 << 3)
#define ResizeTypeDown           (1 << 4)
#define ResizeTypeUpLeft         (ResizeTypeUp | ResizeTypeLeft)
#define ResizeTypeDownLeft       (ResizeTypeDown | ResizeTypeLeft)
#define ResizeTypeUpRight        (ResizeTypeUp  | ResizeTypeRight)
#define ResizeTypeDownRight      (ResizeTypeDown | ResizeTypeRight)
#define ResizeTypeAll            (ResizeTypeUp | ResizeTypeDown | ResizeTypeLeft | ResizeTypeRight)

// type of recalc_geometry()
#define GeometryRecalcAll        0
#define GeometryRecalcAllXY      1  // recalc all, except for plain w and h
#define GeometryRecalcPlain      2

// now lets go to code...

// window sizes without borders, titlebar etc.
struct WindowGeometry 
{
	int x, y, w, h;
	int offset_x, offset_y;
	int min_w, min_h;
	int max_w, max_h;
	int inc_w, inc_h;
	int border;
};

struct FrameData
{
	Window         window;
	Window         transient_win;
	Colormap       colormap;

	WindowGeometry plain;

	Pixmap   icon_pixmap;
	Pixmap   icon_mask;
	char*    label;
	bool     label_alocated;
	long     state;
	long     option;
	short    type;

	int      win_gravity;
	bool     autoplace;
	bool     have_transient;

//	Icon* icon;
};

struct Overlay
{
	int x, y, w, h;
	GC inverted_gc;
};

enum FrameBordersState
{
	FOCUSED = 1,
	UNFOCUSED,
	CLICKED
};

class FrameBorders
{
	private:
		Fl_Color focused;
		Fl_Color unfocused;
		Fl_Color clicked;
		Fl_Color sizers_focused;
		Fl_Color sizers_unfocused;
		Fl_Color sizers_clicked;
		int updown_w;
		int updown2x_w;
		int leftright_h;
		int leftright2x_h;
		bool is_shaped;
		bool is_border;
		void item_color(Fl_Color c, FrameBordersState s, bool is_border);
		Fl_Color item_color(FrameBordersState s, bool is_border);

	public:
		void border_color(Fl_Color c, FrameBordersState s) { item_color(c, s, true); }
		void sizers_color(Fl_Color c, FrameBordersState s) { item_color(c, s, false); }
		void updown(int s);
		void leftright(int s);
		void shaped(bool s);

		Fl_Color border_color(FrameBordersState s) { return item_color(s, true); }
		Fl_Color sizers_color(FrameBordersState s) { return item_color(s, false); }
		int updown(void)      { return updown_w; }
		int updown2x(void)    { return updown2x_w; }
		int leftright(void)   { return leftright_h; }
		int leftright2x(void) { return leftright2x_h; }
		bool shaped(void)     { return is_shaped; }
};

/* Window responsible for displaying parent
 * coordinates. It will _not_ be framed.
 */
class CoordinatesView : public Fl_Window
{
	private:
		char data[100];
		Fl_Box* data_box;
	public:
		CoordinatesView();
		~CoordinatesView();
		void display_data(int x, int y, int w, int h);
};


class Titlebar;
class FrameEventHandler;

// Head honcho of everything
class Frame : public Fl_Window
{
	private:
		friend class FrameEventHandler;

		FrameData* fdata;
		int screen_x, screen_y, screen_w, screen_h;
		// sizes used when we do restore()
		int restore_x, restore_y, restore_w, restore_h;

		bool opaque_move_resize;
		bool show_titlebar;
		bool is_moving;
		bool is_resizing;
		bool cursor_grabbed;
		bool snap_move;
		bool show_coordinates;

		FrameBorders       borders;
		FrameEventHandler* events;

		Titlebar* titlebar;

		Fl_Box* sizer_top_left;
		Fl_Box* sizer_top_right;
		Fl_Box* sizer_bottom_left;
		Fl_Box* sizer_bottom_right;

		CoordinatesView* cview;

		Overlay overlay;

		// private loaders
		void feed_data(XWindowAttributes* attrs);
		//void load_wm_hints(void);
		/* void load_size_hints(void); */
		void load_label(void);

		// preparers
		void setup_borders(void);
		// void init_sizes(void);
		void init_overlay(int border_size);
		void reparent_window(void);
		void place_sizers(int x, int y, int w, int h);
		long resize_type(int x, int y);

		void recalc_geometry(int x, int y, int w, int h, short rtype);
		void configure_notify(void);
		void shape_borders(void);
		void shape_edges(void);

		void show_sizers(void);
		void hide_sizers(void);

		void draw_overlay(int x, int y, int w, int h);
		void clear_overlay(void);

		// this is here so FrameEventHandler can access Fl_Window
		int handle_parent(int event);

		Frame(const Frame&);
		Frame operator=(const Frame&);

	public:
    	Frame(Window, XWindowAttributes* = 0);
    	~Frame();

		Window window(void)           { return fdata->window; }
		Window transient_window(void) { return fdata->transient_win; }

		void settings_changed(void) { }
		int handle(int e);
		int handle(XEvent* e);
		void destroy(void);
		void display_internals(void);
		void content_click(void);

		bool frame_type(short t)      { return ((fdata->type == t) ? true : false); }                 

		void set_state(long s)        { fdata->state |= s;   }
		bool state(long s)            { return ((fdata->state & s) == s) ? true : false; }                 
		void clear_state(long s)      { fdata->state &= ~s;  }
		void clear_all_states(void)   { fdata->state = 0;    }

		void set_option(long o)       { fdata->option |= o;  }
		bool option(long o)           { return ((fdata->option & o) == o) ? true : false; }                 
		void clear_option(long o)     { fdata->option &= ~o; }
		void clear_all_options(void)  { fdata->option = 0;   }

		// public loaders
		void load_colormap(Colormap col = 0);

		// window manipulators
		void set_size(int x, int y, int w, int h, bool apply_on_plain);
		void move_window(int x, int y);
		void resize_window(int mouse_x, int mouse_y, long direction);
		void maximize(void);
		void restore(void);
		void shade(void);
		void unshade(void);
		void close_kill(void);
		void focus(void);
		void unfocus(void);
		void raise(void);
		void lower(void);
		void borders_color(FrameBordersState s);
		void set_cursor(CursorType t);
		// void change_window_type(short t);
		void map(void);
		void unmap(void);

		void show_coordinates_window(void);
		void update_coordinates_window(void);
		void hide_coordinates_window(void);

		//const FrameBorders& frame_borders(void) { return borders; }

		/* These function are used to determine how Frame
		 * will dispatch events between titlebar and sizers
		 */
		void move_start(void) { is_moving = true; }
		void move_end(void)   { is_moving = false; }
		bool moving(void)     { return is_moving; }
		void resize_start(void) { is_resizing = true; }
		void resize_end(void)   { is_resizing = false; }
		bool resizing(void)     { return is_resizing; }

		// TODO: should this be in WindowManager or here ???
		void grab_cursor(void);
		void ungrab_cursor(void);

		bool destroy_scheduled(void) { return state(FrameStateDestroyed); }
};

#endif
