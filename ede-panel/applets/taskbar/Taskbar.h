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

#ifndef __TASKBAR_H__
#define __TASKBAR_H__

#include <FL/Fl_Group.H>
#include "Applet.h"

class TaskButton;

class Taskbar : public Fl_Group {
public:
	TaskButton *curr_active, *prev_active;
public:
	Taskbar();
	~Taskbar();

	void update_task_buttons(void);

	void resize(int X, int Y, int W, int H);
	void layout_children(void);

	void update_active_button(bool do_redraw = true, int xid = -1);
	void activate_window(TaskButton *b);
	void update_child_title(Window xid);
	void update_child_icon(Window xid);

	void panel_redraw(void);
};

#endif
