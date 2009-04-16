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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "icons/core.xpm"
#include "CrashDialog.h"

#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_File_Chooser.H>

#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <edelib/Nls.h>
#include <edelib/File.h>
#include <edelib/MessageBox.h>
#include <edelib/FileTest.h>

#define DIALOG_W 380
#define DIALOG_H 130 
#define DIALOG_W_EXPANDED 380
#define DIALOG_H_EXPANDED 340

int spawn_backtrace(const char* gdb_path, const char* program, const char* core, const char* output, const char* script) {
	const char* gdb_script = "bt\nquit\n";
	const int gdb_script_len = 8;

	/* file with gdb commands */
	int sfd = open(script, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(sfd == -1)
		return -1;
	write(sfd, gdb_script, gdb_script_len);
	close(sfd);

	/* output file with gdb backtrace */
	int ofd = open(output, O_WRONLY | O_TRUNC | O_CREAT, 0770);
	if(ofd == -1)
		return -1;

	pid_t pid = fork();

	if(pid == -1) {
		close(ofd);
		return -1;
	} else if(pid == 0) {
		dup2(ofd, 1);
		close(ofd);

		char* argv[8];
		argv[0] = (char*)gdb_path;
		argv[1] = "--quiet";
		argv[2] = "--batch";
		argv[3] = "-x";
		argv[4] = (char*)script;
		argv[5] = (char*)program;
		argv[6] = (char*)core;
		argv[7] = 0;

		execvp(argv[0], argv);
		return -1;
	} else {
		int status;
		if(waitpid(pid, &status, 0) != pid)
			return -1;
	}

	return 0;
}

edelib::String get_uname(void) {
	struct utsname ut;
	uname(&ut);

	edelib::String ret;
	ret.printf("%s %s %s %s %s", ut.sysname, ut.nodename, ut.release, ut.version, ut.machine);
	return ret;
}

void show_details_cb(Fl_Widget*, void* cd) {
	CrashDialog* c = (CrashDialog*)cd;
	c->show_details();
}

void close_cb(Fl_Widget*, void* cd) {
	CrashDialog* c = (CrashDialog*)cd;
	c->hide();
}

void save_cb(Fl_Widget*, void* cd) {
	CrashDialog* c = (CrashDialog*)cd;
	c->save();
}

CrashDialog::CrashDialog() : Fl_Window(DIALOG_W, DIALOG_H, _("EDE crash handler")),
	appname(NULL), apppath(NULL), bugaddress(NULL), pid(NULL), signal_num(NULL) {
	details_shown = false;

	begin();
		pix = new Fl_Pixmap((const char**)core_xpm);

		icon_box = new Fl_Box(10, 10, 70, 75);
		icon_box->image(pix);

		txt_box = new Fl_Box(85, 10, 285, 75);
		txt_box->align(FL_ALIGN_WRAP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

		close = new Fl_Button(280, 95, 90, 25, _("&Close"));
		close->callback(close_cb, this);

		details = new Fl_Button(10, 95, 265, 25, _("@> Show details"));
		details->box(FL_FLAT_BOX);
		details->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		details->callback(show_details_cb, this);

		/* widgets for expanded dialog */
		trace_log = new Fl_Text_Display(10, 130, 360, 165);
		trace_buff = new Fl_Text_Buffer();
		trace_log->buffer(trace_buff);
		trace_log->hide();
		save_as = new Fl_Button(280, 305, 90, 25, _("&Save As..."));
		save_as->callback(save_cb, this);
		save_as->hide();
	end();
}

CrashDialog::~CrashDialog() {
	/* looks like fltk does not clean image() assigned data */
	delete pix;
}

void CrashDialog::show_details(void) {
	if(trace_log->visible()) {
		trace_log->hide();
		save_as->hide();
		details->label(_("@> Show details"));
		size(DIALOG_W, DIALOG_H);
	} else {
		trace_log->show();
		save_as->show();
		details->label(_("@< Hide details"));
		size(DIALOG_W_EXPANDED, DIALOG_H_EXPANDED);

		if(!details_shown) {
			trace_buff->remove(0, trace_buff->length());

			edelib::String address = _("\nPlease report this at: ");
			if(bugaddress)
				address += bugaddress;
			else
				address += "http://bugs.equinox-project.org";

			trace_buff->append(address.c_str());
			trace_buff->append("\n\n");

			trace_buff->append("---------- short summary ----------\n"); 
			trace_buff->append("\nEDE version: " PACKAGE_VERSION);
			trace_buff->append("\nSystem info: ");
			trace_buff->append(get_uname().c_str());

			trace_buff->append("\nProgram name: ");
			if(appname)
				trace_buff->append(appname);
			else
				trace_buff->append("(unknown)");

			trace_buff->append("\nExecutable path: ");
			if(apppath)
				trace_buff->append(apppath);
			else
				trace_buff->append("(unknown)");

			trace_buff->append("\nRunning PID: ");
			if(pid)
				trace_buff->append(pid);
			else
				trace_buff->append("(unknown)");

			trace_buff->append("\nSignal received: ");
			if(signal_num)
				trace_buff->append(signal_num);
			else
				trace_buff->append("(unknown)");

			/* try backtrace via gdb */
			trace_buff->append("\n\n---------- backtrace ----------\n"); 

			const char* core_file = "core";
			const char* gdb_output = "/tmp/.gdb_output";
			const char* gdb_script = "/tmp/.gdb_script";

			if(!edelib::file_test(core_file, edelib::FILE_TEST_IS_REGULAR)) {
				trace_buff->append("\nUnable to find 'core' file. Backtrace will not be done.");
				details_shown = false;
				return;
			}

			edelib::String gdb_path = edelib::file_path("gdb");
			if(gdb_path.empty()) {
				trace_buff->append("\nUnable to find gdb. Please install it first.");
				/* set to false so next 'Show Details' click can try again with the debugger */
				details_shown = false;
				return;
			}

			/* TODO: these files should be unique per session */
			if(spawn_backtrace(gdb_path.c_str(), cmd.c_str(), core_file, gdb_output, gdb_script) == -1) {
				trace_buff->append("\nUnable to properly execute gdb");
				details_shown = false;
				return;
			}

			trace_buff->append("\n");
			if(trace_buff->appendfile(gdb_output) != 0) {
				trace_buff->append("Unable to read gdb output file");
				details_shown = false;
				return;
			}

			edelib::file_remove(gdb_output);
			edelib::file_remove(gdb_script);
			edelib::file_remove(core_file);

			details_shown = true;
		}
	}
}

void CrashDialog::save(void) {
	const char* p = fl_file_chooser(_("Save details to..."), "Text Files (*.txt)\tAll Files(*)", "dump.txt");
	if(!p)
		return;

	// so we can have EOL in file
	trace_buff->append("\n");

	if(trace_buff->savefile(p) != 0)
		edelib::alert(_("Unable to save to %s. Please check permissions to write in this directory or file"), p);
}

void CrashDialog::run(void) {
	edelib::String l;

	if(appname || apppath) {
		const char* p = (appname ? appname : apppath);
		l.printf(_("Program '%s' just crashed!"), p);
	} else
		l += _("Program just crashed!");
	l += _("\n\nYou can inspect details about this crash by clicking on 'Show details' below");

	txt_box->copy_label(l.c_str());

	if(!shown())
		show();

	while(shown())
		Fl::wait();
}
