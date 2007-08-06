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

#include "icons/core.xpm"
#include "Crash.h"
#include "Spawn.h"
#include <FL/Fl.h>
#include <FL/Fl_Pixmap.h>
#include <edelib/Nls.h>
#include <edelib/File.h>

#include <stdio.h> // snprintf

#define DIALOG_W 380
#define DIALOG_H 130 
#define DIALOG_W_EXPANDED 380
#define DIALOG_H_EXPANDED 340

void show_details_cb(Fl_Widget*, void* cd) {
	CrashDialog* c = (CrashDialog*)cd;
	c->show_details();
}

void close_cb(Fl_Widget*, void* cd) {
	CrashDialog* c = (CrashDialog*)cd;
	c->hide();
}

void dummy_timeout(void*) { }

CrashDialog::CrashDialog() : Fl_Window(DIALOG_W, DIALOG_H, _("World is going down...")) {
	trace_loaded = 0;

	begin();
		pix = new Fl_Pixmap(core_xpm);

		icon_box = new Fl_Box(10, 10, 70, 75);
		icon_box->image(pix);

		txt_box = new Fl_Box(85, 10, 285, 75);
		txt_box->align(FL_ALIGN_WRAP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		close = new Fl_Button(280, 95, 90, 25, _("&Close"));
		close->callback(close_cb, this);

		details = new Fl_Button(10, 95, 265, 25, _("@> Show backtrace"));
		details->box(FL_FLAT_BOX);
		details->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		details->callback(show_details_cb, this);

		// widgets for expanded dialog
		trace_log = new Fl_Text_Display(10, 130, 360, 165);
		trace_buff = new Fl_Text_Buffer();
		trace_log->buffer(trace_buff);
		trace_log->hide();
		save_as = new Fl_Button(280, 305, 90, 25, _("&Save As..."));
		save_as->hide();
		copy = new Fl_Button(185, 305, 90, 25, _("&Copy"));
		copy->hide();
	end();
}

CrashDialog::~CrashDialog() {
	// looks like fltk does not clean image() assigned data
	delete pix;
}

void CrashDialog::show_details(void) {
	if(trace_log->visible()) {
		trace_log->hide();
		save_as->hide();
		copy->hide();
		details->label(_("@> Show backtrace"));
		size(DIALOG_W, DIALOG_H);
	} else {
		trace_log->show();
		save_as->show();
		copy->show();
		details->label(_("@< Hide backtrace"));
		size(DIALOG_W_EXPANDED, DIALOG_H_EXPANDED);

		if(!trace_loaded) {
			trace_buff->remove(0, trace_buff->length());

			// read core
			spawn_backtrace(cmd.c_str(), "core", "/tmp/gdb_output", "/tmp/gdb_script");

			trace_buff->appendfile("/tmp/gdb_output");
			trace_loaded = 1;

			// delete core
			edelib::file_remove("core");
		}
	}
}

void CrashDialog::set_data(const char* command) {
	cmd = command;

	char txt[1024];
	snprintf(txt, sizeof(txt), _("Program just crashed !!!\n\nYou can inspect details about this crash by clicking on 'Show backtrace' below"));
	txt_box->copy_label(txt);
	trace_loaded = 0;
}

void CrashDialog::run(void) {
	if(!shown()) {
		set_modal();
		show();
	}

	while(shown())
		Fl::wait();
}
