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

#ifndef __CRASH_H__
#define __CRASH_H__

#include <FL/Fl_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Pixmap.h>
#include <FL/Fl_Text_Display.h>
#include <FL/Fl_Text_Buffer.h>

#include <edelib/String.h>

class CrashDialog : public Fl_Window {
	private:
		Fl_Pixmap* pix;
		Fl_Box* txt_box;
		Fl_Box* icon_box;
		Fl_Button* close;
		Fl_Button* details;

		Fl_Text_Display* trace_log;
		Fl_Text_Buffer* trace_buff;
		Fl_Button* save_as;
		Fl_Button* copy;

		edelib::String cmd;
		bool trace_loaded;

	public:
		CrashDialog();
		~CrashDialog();
		void show_details(void);

		void set_data(const char* command);
		void run(void);
};

#endif
