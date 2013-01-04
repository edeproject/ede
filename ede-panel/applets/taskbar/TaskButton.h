/*
 * $Id$
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

#ifndef __TASKBUTTON_H__
#define __TASKBUTTON_H__

#include <FL/Fl_Button.H>
#include <FL/x.H>

class TaskButton : public Fl_Button {
private:
	/* window ID this button handles */
	Window xid;
	int    wspace;
	bool   image_alloc;    
	Atom   net_wm_icon;

	void clear_image(void);
public:
	TaskButton(int X, int Y, int W, int H, const char *l = 0);
	~TaskButton();

	void draw(void);
	void display_menu(void);

	void    set_window_xid(Window win) { xid = win; }
	Window  get_window_xid(void) { return xid; }

	void update_title_from_xid(void);
	void update_image_from_xid(void);

	void set_workspace(int s) { wspace = s; }
	int  get_workspace(void) { return wspace; }
};

#endif
