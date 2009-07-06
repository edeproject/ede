/*
 * $Id$
 *
 * ede-crasher, a crash handler tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/utsname.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_File_Chooser.H>

#include <edelib/Nls.h>
#include <edelib/Window.h>
#include <edelib/String.h>
#include <edelib/Version.h>
#include <edelib/MessageBox.h>
#include <edelib/Run.h>
#include <edelib/File.h>

#include "CrashDialog.h"
#include "GdbOutput.h"
#include "CoreIcon.h"
#include "icons/core.xpm"

#define WIN_H_NORMAL   130
#define WIN_H_EXPANDED 340

EDELIB_NS_USING(String)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(run_async)
EDELIB_NS_USING(file_remove)
EDELIB_NS_USING(file_path)

static edelib::Window       *win;
static Fl_Text_Display      *txt_display;
static Fl_Text_Buffer       *txt_buf;
static Fl_Button            *save_as;
static Fl_Button            *show_details;
static GdbOutput            *gdb;
static bool                  info_was_collected;
static const ProgramDetails *pdetails;

static void close_cb(Fl_Widget*, void*) {
	win->hide();
}

static void save_as_cb(Fl_Widget*, void*) {
	const char *p = fl_file_chooser(_("Save details to..."), "Text Files (*.txt)\tAll Files(*)", "dump.txt");
	if(!p)
		return;

	/* so we can have EOL */
	txt_buf->append("\n");

	if(txt_buf->savefile(p) != 0)
		alert(_("Unable to save to %s. Please check permissions to write in this directory or file"), p);
}

static void write_host_info(void) {
	txt_buf->append("---------- short summary ----------\n");
	txt_buf->append("\nEDE version: " PACKAGE_VERSION);
	txt_buf->append("\nedelib version: " EDELIB_VERSION);

	struct utsname ut;
	if(uname(&ut) == 0) {
		char buf[1024];
		snprintf(buf, sizeof(buf), "%s %s %s %s %s", ut.sysname, ut.nodename, ut.release, ut.version, ut.machine);

		txt_buf->append("\nSystem info: ");
		txt_buf->append(buf);
	}

	txt_buf->append("\nProgram name: ");
	if(pdetails->name)
		txt_buf->append(pdetails->name);
	else
		txt_buf->append("(unknown)");

	txt_buf->append("\nProgram path: ");
	if(pdetails->path)
		txt_buf->append(pdetails->path);
	else
		txt_buf->append("(unknown)");

	txt_buf->append("\nProgram PID: ");
	if(pdetails->pid)
		txt_buf->append(pdetails->pid);
	else
		txt_buf->append("(unknown)");

	txt_buf->append("\nSignal received: ");
	if(pdetails->sig)
		txt_buf->append(pdetails->sig);
	else
		txt_buf->append("(unknown)");

	txt_buf->append("\n\n---------- backtrace ----------\n\n");
}

static void collect_info_once(void) {
	if(info_was_collected)
		return;

	write_host_info();

	if(gdb->fds_opened() && gdb->run())
		txt_buf->appendfile(gdb->output_path());

	info_was_collected = true;
}

static void show_details_cb(Fl_Widget*, void*) {
	if(save_as->visible()) {
		win->size(win->w(), WIN_H_NORMAL);
		txt_display->hide();
		save_as->hide();
		show_details->label(_("@> Show details"));
		return;
	}

	win->size(win->w(), WIN_H_EXPANDED);
	txt_display->show();
	save_as->show();
	show_details->label(_("@< Hide details"));

	collect_info_once();
}

static void report_cb(Fl_Widget*, void*) {
	String bug_tool = file_path("ede-bug-report");
	if(bug_tool.empty()) {
		alert(_("Unable to find ede-bug-report tool."
				" Please check if PATH variable contains directory where this tool was installed"));
		return;
	}

	collect_info_once();

	errno = 0;
	int  fd;
	char tmp[] = "/tmp/.ecrash-dump.XXXXXX";

	if((fd = mkstemp(tmp)) == -1) {
		alert(_("Unable to create temporary file: (%i) %s"), errno, strerror(errno));
		return;
	}

	close(fd);
	txt_buf->savefile(tmp);

	run_async("%s --gdb-dump %s", bug_tool.c_str(), tmp);

	/* wait some time until the file was read; dumb, I know :( */
	sleep(1);
	file_remove(tmp);
}

int crash_dialog_show(const ProgramDetails& p) {
	info_was_collected = false;
	pdetails = &p;

	gdb = new GdbOutput();
	gdb->set_program_path(p.path);
	gdb->fds_open();

	win = new edelib::Window(380, WIN_H_NORMAL, _("EDE Crash Handler"));
	win->begin();
		Fl_Box* image_box = new Fl_Box(10, 10, 65, 60);
		image_box->image(image_core);

		String s;
		if(p.name)
			s.printf(_("Program '%s' quit unexpectedly."), p.name);
		else
			s = _("Program quit unexpectedly.");
		s += _("\n\nYou can inspect the details about this crash by clicking on \'Show details\' below");

		Fl_Box* txt_box = new Fl_Box(85, 10, 285, 75, s.c_str());
		txt_box->align(132|FL_ALIGN_INSIDE);

		/* only EDE applications can have 'Report' button for reporting issues on EDE bugzilla */
		if(p.ede_app) {
			Fl_Button* report = new Fl_Button(185, 95, 90, 25, _("&Report..."));
			report->callback(report_cb);
		}

		Fl_Button* close = new Fl_Button(280, 95, 90, 25, _("&Close"));
		close->callback(close_cb);

		show_details = new Fl_Button(10, 95, 165, 25, _("@> Show details"));
		show_details->box(FL_FLAT_BOX);
		show_details->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
		show_details->callback(show_details_cb);

		txt_display = new Fl_Text_Display(10, 130, 360, 165);
		txt_buf = new Fl_Text_Buffer();
		txt_display->buffer(txt_buf);
		
		txt_display->hide();

		save_as = new Fl_Button(280, 305, 90, 25, _("&Save As..."));
		save_as->hide();
		save_as->callback(save_as_cb);
	win->end();

	win->window_icon(core_xpm);
	win->show();

	int ret = Fl::run();
	delete gdb;
	return ret;
}
