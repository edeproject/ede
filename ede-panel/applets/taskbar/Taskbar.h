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

/* if button should be on visible on all workspaces */
#define ALL_WORKSPACES -1

class TaskButton;

class Taskbar : public Fl_Group {
public:
	TaskButton *curr_active, *prev_active;
	bool fixed_layout;           /* fixed or streched layout of buttons */
	bool ignore_workspace_value; /* should all windows be shown ignoring workspace value */

	int  current_workspace;

	bool visible_on_current_workspace(int ws) {
		return ignore_workspace_value || (ws == ALL_WORKSPACES) || (ws == current_workspace);
	}

public:
	Taskbar();
	~Taskbar();

	void update_task_buttons(void);
	void update_workspace_change(void);

	void resize(int X, int Y, int W, int H);
	void layout_children(void);

	void update_active_button(bool do_redraw = true, int xid = -1);
	void activate_window(TaskButton *b);
	void update_child_title(Window xid);
	void update_child_icon(Window xid);
	void update_child_workspace(Window xid);

	void panel_redraw(void);

	/* try to move child on place of other child, but only if it falls within x,y range */
	void try_dnd(TaskButton *b, int x, int y);
};

#endif
