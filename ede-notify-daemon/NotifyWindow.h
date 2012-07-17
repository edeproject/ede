/*
 * $Id: ede-panel.cpp 3330 2012-05-28 10:57:50Z karijes $
 *
 * Copyright (C) 2012 Sanel Zukan
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
 
#ifndef __NOTIFYWINDOW_H__
#define __NOTIFYWINDOW_H__

#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Multiline_Output.H>

/* just keep it greater than FL_WINDOW or FL_DOUBLE_WINDOW */
#define NOTIFYWINDOW_TYPE 0xF9

class NotifyWindow : public Fl_Window {
private:
	int       id, exp, timer_set;
	Fl_Button *closeb;
	Fl_Box    *imgbox;
	Fl_Multiline_Output *summary, *body;

	void add_timeout(void);
	void remove_timeout(void);
public:
	NotifyWindow();

	void set_id(int i) { id = i; }
	int  get_id(void) { return id; }

	void set_icon(const char *img);
	void set_summary(const char *s) { summary->value(s); }
	void set_body(const char *s);

	/*
	 * match to spec: if is -1, then we handle it, if is 0, then window will not be closed and
	 * the rest is sender specific
	 */
	void set_expire(int t) { exp = t; }
	void show(void);

	virtual void resize(int X, int Y, int W, int H);
	virtual int handle(int event);
};

#endif
