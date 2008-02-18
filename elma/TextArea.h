/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __TEXTAREA_H__
#define __TEXTAREA_H__

#include <FL/Fl_Input.h>
#include <edelib/Debug.h>

/*
 * Creates Fl_Input-like widget, but without any box. It is not the same as
 * Fl_Input::box(FL_NO_BOX) because box is not drawn and Fl_Input will not
 * redraw itself on changes. As help, we request from parent to redraw region
 * where this widget resides.
 */
class TextArea : public Fl_Input {
	public:
		TextArea(int X, int Y, int W, int H, const char* l = 0) : Fl_Input(X, Y, W, H, l) { 
			box(FL_NO_BOX);
		}

		~TextArea() { }

		virtual int handle(int event) {
			// FLTK can send FL_NO_EVENT, so be carefull
			if(!event)
				return 0;

			// not needed, parent will redraw us
			if(event == FL_SHOW || event == FL_HIDE)
				return 1;

			// don't allow input when we are disabled
			if(event == FL_KEYBOARD && !active())
				return 1;

			int ret = Fl_Input::handle(event);
			if(ret);
				parent()->damage(FL_DAMAGE_ALL, x(), y(), w(), h());

			return ret;
		}
};

#endif
