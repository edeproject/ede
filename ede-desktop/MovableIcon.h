/*
 * $Id: ede-panel.cpp 3463 2012-12-17 15:49:33Z karijes $
 *
 * Copyright (C) 2006-2013 Sanel Zukan
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

#ifndef __MOVABLEICON_H__
#define __MOVABLEICON_H__

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/x.H>

class DesktopIcon;

class MovableIcon : public Fl_Window {
private:
	DesktopIcon* icon;
	Fl_Box*  icon_box;
	Pixmap   mask;
	Atom     opacity_atom;

public:
	MovableIcon(DesktopIcon* i);
	~MovableIcon();
	virtual void show(void);
};

#endif
