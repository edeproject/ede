/*
 * $Id$
 *
 * ede-bug-report, a tool to report bugs on EDE bugzilla instance
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Button.H>

#include <edelib/Window.h>
#include <edelib/MessageBox.h>
#include <edelib/String.h>
#include <edelib/Regex.h>
#include <edelib/Debug.h>
#include <edelib/Ede.h>

#ifdef HAVE_CURL
# include "BugzillaSender.h"
# include "BugImage.h"
# include "icons/bug.xpm"
#endif

EDELIB_NS_USING(String)
EDELIB_NS_USING(Regex)
EDELIB_NS_USING(alert)
EDELIB_NS_USING(message)
EDELIB_NS_USING(RX_CASELESS)

#ifdef HAVE_CURL
static Fl_Input        *bug_title_input;
static Fl_Input        *email_input;
static Fl_Text_Buffer  *text_buf;

/* check if string has spaces */
static bool empty_entry(const char *en) {
	if(!en)
		return true;

	for(const char *p = en; *p; p++) {
		if(*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
			return false;
	}

	return true;
}

static bool valid_email(const char *e) {
	Regex r;

	/* regex stolen from http://www.regular-expressions.info/email.html */
	if(!r.compile("\\b[A-Z0-9._%-]+@[A-Z0-9.-]+\\.[A-Z]{2,4}\\b", RX_CASELESS)) {
		E_WARNING(E_STRLOC ": Unable to properly compile regex pattern");
		return false;
	}

	return (r.match(e) > 0);
}

static void close_cb(Fl_Widget*, void *w) {
	edelib::Window *win = (edelib::Window *)w;
	win->hide();
}

static void send_cb(Fl_Widget*, void *w) {
	const char     *txt;
	String         title, content;

	if(empty_entry(bug_title_input->value())) {
		alert(_("Report title is missing"));
		return;
	}

	if(empty_entry(email_input->value())) {
		alert(_("Email address is missing"));
		return;
	}

	txt = text_buf->text();
	if(empty_entry(txt)) {
		alert(_("Detail problem description is missing"));
		free((void *) txt);
		return;
	}

	if(!valid_email(email_input->value())) {
		alert(_("Email address is invalid. Please use the valid email address"));
		return;
	}

	/* 
	 * construct a subject header, so we knows it came from report tool
	 * BRT - Bug Report Tool
	 */
	title = "[BRT] ";
	title += bug_title_input->value();

	/* reserve space for our content */
	content.reserve(text_buf->length() + 128);

	/* construct content with some header data */
	content += "This issue was reported via EDE Bug Report Tool (BRT).\n\n";
	content += "Reporter:\n";
	content += email_input->value();
	content += "\n\n";
	content += "Detail description:\n";
	content += txt;

	free((void *) txt);

	if(bugzilla_send_with_progress(title.c_str(), content.c_str()))
		close_cb(0, w);
}
#endif /* HAVE_CURL */

int main(int argc, char** argv) {
#ifndef HAVE_CURL
	alert(_("ede-bug-report was compiled without cURL support.\n"
			"You can install cURL either via your distribution package management system, or download "
			"it from http://curl.haxx.se. After this, you'll have to recompile ede-bug-report again"));
	return 1;
#else
	EDE_APPLICATION("ede-bug-report");

	/* in case if debugger output was given */
	const char *gdb_output = NULL;

	if(argc == 3 && ((strcmp(argv[1], "--gdb-dump") == 0) || (strcmp(argv[1], "-g") == 0)))
		gdb_output = argv[2];

	edelib::Window *win = new edelib::Window(480, 365, _("EDE Bug Reporting Tool"));
	win->begin();
    	Fl_Box *image_box = new Fl_Box(10, 10, 60, 59);
		image_box->image(image_bug);

		Fl_Box *title_box = new Fl_Box(80, 10, 390, 30, _("EDE Bug Reporting Tool"));
		title_box->labelfont(1);
		title_box->labelsize(14);
		title_box->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

		Fl_Box *description_box = new Fl_Box(80, 45, 390, 95, _("To help us to improve the future EDE versions, "
		"please describe the problem with much details as possible.\n\nNote: valid email address is required, so "
		"developers could contact you for more informations if necessary."));

		description_box->align(133|FL_ALIGN_INSIDE);

		bug_title_input = new Fl_Input(10, 165, 225, 25, _("Short and descriptive title:"));
		bug_title_input->align(FL_ALIGN_TOP_LEFT);

		email_input = new Fl_Input(240, 165, 230, 25, _("Your email address:"));
		email_input->align(FL_ALIGN_TOP_LEFT);

		Fl_Text_Editor* te = new Fl_Text_Editor(10, 215, 460, 105, _("Detail description of the problem:"));
		te->align(FL_ALIGN_TOP_LEFT);
		/* wrap length of the text */
		te->wrap_mode(1, 80);

		text_buf = new Fl_Text_Buffer();
		te->buffer(text_buf);

		if(gdb_output) {
			if(text_buf->appendfile(gdb_output) == 0) {
				text_buf->insert(0, "\nCollected data:\n");
				text_buf->insert(0, "(please write additional comments here, removing this line)\n\n");
			} else 
				E_WARNING(E_STRLOC ": Unable to read '%s' as debugger output. Continuing...\n");
		}

		/* resizable box */
		Fl_Box *rbox = new Fl_Box(180, 273, 55, 37);

		Fl_Button *send = new Fl_Button(285, 330, 90, 25, _("&Send"));
		send->callback(send_cb, win);

		Fl_Button *close = new Fl_Button(380, 330, 90, 25, _("&Close"));
		close->callback(close_cb, win);

		Fl_Group::current()->resizable(rbox);
	win->window_icon(bug_xpm);
	/* win->show(argc, argv); */
	win->show();
	return Fl::run();
#endif /* HAVE_CURL */
}
