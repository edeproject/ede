/*
 * $Id: DesktopIcon.h 2742 2009-07-09 13:59:51Z karijes $
 *
 * ede-desktop, desktop and icon manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2006-2011 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __MOVABLEICON_H__
#define __MOVABLEICON_H__

#include <X11/Xlib.h> // Pixmap
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>

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
