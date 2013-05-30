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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define FL_LIBRARY 1

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <edelib/Debug.h>

#ifdef HAVE_SHAPE
# include <X11/extensions/shape.h>
#endif

#include "MovableIcon.h"
#include "DesktopIcon.h"
#include "Utils.h"

MovableIcon::MovableIcon(DesktopIcon* ic) :
Fl_Window(ic->x(), ic->y(), ic->w(), ic->h()), icon(ic), mask(0), opacity_atom(0) 
{
	E_ASSERT(icon != NULL);

	set_override();
	color(ic->color());

	begin();
		/* force box be same width/height as icon so it can fit inside masked window */
#ifdef HAVE_SHAPE 
		Fl_Image* img = ic->image();
		if(img)
			icon_box = new Fl_Box(0, 0, img->w(), img->h());
		else
			icon_box = new Fl_Box(0, 0, w(), h());
#else
		icon_box = new Fl_Box(0, 0, w(), h());
#endif
		icon_box->image(ic->image());
	end();

	opacity_atom = XInternAtom(fl_display, "_NET_WM_WINDOW_OPACITY", False);
}

MovableIcon::~MovableIcon() {
	if(mask)
		XFreePixmap(fl_display, mask);
}

void MovableIcon::show(void) {
	if(!shown())
		Fl_X::make_xid(this);

#ifdef HAVE_SHAPE
	if(icon->image()) {
		mask = create_mask(icon->image());
		if(mask) {
			XShapeCombineMask(fl_display, fl_xid(this), ShapeBounding, 0, 0, mask, ShapeSet);

			/* now set transparency; composite manager should handle the rest (if running) */
			unsigned int opacity = 0xc0000000;
			XChangeProperty(fl_display, fl_xid(this), opacity_atom, XA_CARDINAL, 32, PropModeReplace,
					(unsigned char*)&opacity, 1L);
		}
	}
#endif
}
