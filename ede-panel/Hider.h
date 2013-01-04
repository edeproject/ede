/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
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

#include <FL/Fl_Button.H>

#ifndef __HIDER_H__
#define __HIDER_H__

class Hider : public Fl_Button {
private:
	int old_x, old_y, is_hidden, old_px, stop_x;
public:
	Hider();
	int  panel_hidden(void) { return is_hidden; }
	void panel_hidden(int s) { is_hidden = s; }

	void panel_show(void);
	void panel_hide(void);
	void post_show(void);
	void post_hide(void);
	void animate(void);
};

#endif
