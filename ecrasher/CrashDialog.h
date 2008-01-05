/*
 * $Id$
 *
 * Ecrasher, a crash handler tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifndef __CRASHDIALOG_H__
#define __CRASHDIALOG_H__

#include <FL/Fl_Window.h>
#include <FL/Fl_Box.h>
#include <FL/Fl_Button.h>
#include <FL/Fl_Pixmap.h>
#include <FL/Fl_Text_Display.h>
#include <FL/Fl_Text_Buffer.h>

#include <edelib/String.h>

class CrashDialog : public Fl_Window {
	private:
		const char* appname;
		const char* apppath;
		const char* bugaddress;
		const char* pid;
		const char* signal_num;

		Fl_Pixmap* pix;
		Fl_Box* txt_box;
		Fl_Box* icon_box;
		Fl_Button* close;
		Fl_Button* details;

		Fl_Text_Display* trace_log;
		Fl_Text_Buffer* trace_buff;
		Fl_Button* save_as;

		edelib::String cmd;
		bool details_shown;

	public:
		CrashDialog();
		~CrashDialog();
		void show_details(void);

		void set_appname(const char* a) { appname = a; }
		void set_apppath(const char* p) { apppath = p; }
		void set_bugaddress(const char* a) { bugaddress = a; }
		void set_pid(const char* p) { pid = p; }
		void set_signal(const char* s) { signal_num = s; }

		void save(void);

		void run(void);
};

#endif
