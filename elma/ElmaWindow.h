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

#ifndef __ELMAWINDOW_H__
#define __ELMAWINDOW_H__

#include <FL/Fl_Double_Window.h>

class Background;
class TextArea;
class Fl_Box;

class ElmaWindow : public Fl_Double_Window {
	private:
		Background* bkg;
		TextArea*   user_in;
		TextArea*   pass_in;
		Fl_Box*     error_display;
		Fl_Box*     info_display;
		bool        deny_mode;

		void validate_user(void);

	public:
		ElmaWindow(int W, int H);
		~ElmaWindow();

		void allow_input(void);
		void deny_input(void);

		bool load_everything(void);
		virtual int handle(int event);
};
#endif
