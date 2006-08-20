/*
 * $Id: Titlebar.h 1718 2006-07-29 13:26:19Z karijes $
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the 
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#ifndef __TITLEBAR_H__
#define __TITLEBAR_H__

#include <efltk/Fl_Button.h>
#include <efltk/Fl_Window.h>
#include <efltk/Fl_Box.h>
#include <efltk/Fl_Group.h>
#include <efltk/Fl_Menu_.h>

// TODO: remove this after themes are added
#define TITLEBAR_MAX_UP 1
#define TITLEBAR_CLOSE_UP 2
#define TITLEBAR_MIN_UP 3

#define PLACE_RIGHT 1
#define PLACE_LEFT  2

class TitlebarButton : public Fl_Button
{
	private:
		int button_type;
		int pos;
	public:
		TitlebarButton(int type);
		~TitlebarButton();
		void place(int p);
		int place(void)    { return pos; }
};

class Frame;

class Titlebar : public Fl_Group
{
	private:
		int win_x, win_y, win_x1, win_y1;
		Frame* curr_frame;
		TitlebarButton minb;
		TitlebarButton maxb;
		TitlebarButton closeb;
		Fl_Box* label_box;
		Fl_Box* icon_box;
		Fl_Color focus_color;
		Fl_Color unfocus_color;

		Fl_Menu_* title_menu;
		Fl_Widget* menu_max;
		Fl_Widget* menu_min;
		Fl_Widget* menu_close;
		Fl_Widget* menu_shade;
		Fl_Widget* menu_lower;

	public:
		Titlebar(Frame* f, int x, int y, int w, int h, const char* l);
		~Titlebar();
		void layout(void);
		int handle(int event);
		void focus(void);
		void unfocus(void);

		void on_close(void);
		void on_maximize(void);
		void on_minimize(void);
		void on_shade(void);
		void on_lower(void);
};

#endif
