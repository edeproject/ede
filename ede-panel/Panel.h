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

#ifndef __PANEL_H__
#define __PANEL_H__

#include <edelib/Window.h>
#include "AppletManager.h"

#define EDE_PANEL_CAST_TO_PANEL(obj)   ((Panel*)(obj))
#define EDE_PANEL_GET_PANEL_OBJECT(w)  (EDE_PANEL_CAST_TO_PANEL(w->parent()))

EDELIB_NS_USING_AS(Window, PanelWindow)

enum {
	PANEL_POSITION_TOP,
	PANEL_POSITION_BOTTOM
};

class Panel : public PanelWindow {
private:
	Fl_Widget *clicked;
	int        vpos;
	int        sx, sy;
	int        screen_x, screen_y, screen_w, screen_h, screen_h_half;
	int        width_perc; /* user specified panel width */
	bool       can_move_widgets, can_drag;

	AppletManager mgr;

	void read_config(void);
	void save_config(void);
	void do_layout(void);

public:
	Panel();

	void show(void);
	void hide(void);
	void update_size_and_pos(bool create_xid, bool update_strut);
	void update_size_and_pos(bool create_xid, bool update_strut, int X, int Y, int W, int H);
	int  handle(int e);
	void load_applets(void);

	int panel_w(void) { return w(); }
	int panel_h(void) { return h(); }

	void screen_size(int &x, int &y, int &w, int &h) {
		x = screen_x;
		y = screen_y;
		w = screen_w;
		h = screen_h;
	}

	/* so applets can do something */
	void apply_struts(bool apply);
	void allow_drag(bool d) { can_drag = d; }

	void relayout(void) { do_layout(); }
};

#endif
