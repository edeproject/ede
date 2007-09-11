/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2007 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __AUTOSTART_H__
#define __AUTOSTART_H__

#include <FL/Fl_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Check_Browser.h>
#include <FL/Fl_Button.h>

#include <edelib/String.h>

struct AstartItem {
	edelib::String name;
	edelib::String exec;
};

class AstartDialog : public Fl_Window {
	private:
		unsigned int curr;
		unsigned int lst_sz;
		bool show_dialog;
		AstartItem* lst;

		Fl_Box* img;
		Fl_Box* txt;
		Fl_Check_Browser* cbrowser;
		Fl_Button* rsel;
		Fl_Button* rall;
		Fl_Button* cancel;

	public:
		AstartDialog(unsigned int sz, bool do_show);
		~AstartDialog();

		unsigned int list_size(void) { return curr; }
		void add_item(const edelib::String& n, const edelib::String& e);
		void run(void);

		void run_all(void);
		void run_selected(void);
};

#endif
