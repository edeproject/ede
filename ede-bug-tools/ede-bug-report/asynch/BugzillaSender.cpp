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

#include <stdio.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl.H>

#include <edelib/Nls.h>
#include <edelib/MessageBox.h>

#include "PulseProgress.h"
#include "BugzillaSender.h"
#include "Bugzilla.h"

#define EDE_BUGZILLA_URL          "http://bugs.equinox-project.org"
#define EDE_BUGZILLA_XMLRPC_URL   "http://bugs.equinox-project.org/xmlrpc.cgi"

/* must match to existing user */
#define EDE_BUGZILLA_USER "ede-bugs@lists.sourceforge.net"
#define EDE_BUGZILLA_PASS "edebugs2709"

#define PROGRESS_STEP_REFRESH 0.06

EDELIB_NS_USING(alert)

enum {
	STATE_NEXT = 1,
	STATE_LOGIN,
	STATE_LOGOUT,
	STATE_DONE
};

struct ResponseParams {
	int         working_state;
	const char *bug_title;
	const char *bug_content;
};

static Fl_Double_Window *win;
static PulseProgress    *progress;
static BugzillaData     *bdata;

static void xmlrpc_request_timeout(void*) {
	bugzilla_event_loop_finish_timeout(bdata, 20);

	/* repeat this function so all pending RPC requests are processed */
	Fl::repeat_timeout(0.5, xmlrpc_request_timeout);
}

static void progress_timeout(void*) {
	progress->step();
	Fl::repeat_timeout(PROGRESS_STEP_REFRESH, progress_timeout);
}

static void clear_timeouts(void) {
	Fl::remove_timeout(xmlrpc_request_timeout);
	Fl::remove_timeout(progress_timeout);
}

static void cancel_cb(Fl_Widget*, void*) {
	win->hide();
}

static void _id_response_cb(int id, void *data, int errnum, const char *err) {
	if(errnum) {
		alert(_("Received bad response: (%i) : %s"), errnum, err);
		/* close dialog */
		cancel_cb(0, 0);
		return;
	}

	ResponseParams *rparams = (ResponseParams*) data;

	if(rparams->working_state == STATE_LOGIN) {
		progress->label("Done. Submitting the data...");
		printf("logged in as id: %i\n", id);

		/* must match EDE Bugzilla server options */
		bugzilla_submit_bug(bdata,
				"ede",
				"general",
				rparams->bug_title,
				"unspecified",
				rparams->bug_content,
				"All",
				"All",
				"P5",
				"normal");

		rparams->working_state += STATE_NEXT;
		return;
	}

	if(rparams->working_state == STATE_LOGOUT) {
		progress->label("Data sent successfully :). Clearing...");
		puts("XXX");

		bugzilla_logout(bdata);
		rparams->working_state += STATE_NEXT;
		return;
	}
}

bool bugzilla_send_with_progress(const char *title, const char *content) {
	/* used to simplify passing multiple data to the response callback */
	ResponseParams rparams;

	/* 
	 * we maintain state so knows what we done, since our Bugzilla interface
	 * supports only one callback for ID's
	 */
	rparams.working_state = STATE_LOGIN;

	rparams.bug_title = title;
	rparams.bug_content = content;

	bdata = bugzilla_new(EDE_BUGZILLA_XMLRPC_URL);
	if(!bdata) {
		alert(_("Unable to initialize bugzilla interface!"));
		return false;
	}

	/* set callback and data */
	bugzilla_set_id_response_callback(bdata, _id_response_cb);
	bugzilla_set_callback_data(bdata, &rparams);

	win = new Fl_Double_Window(275, 90, _("Sending report data"));
	win->begin();
		progress = new PulseProgress(10, 20, 255, 25, _("Sending report..."));
		progress->selection_color((Fl_Color)137);

		Fl_Button *cancel = new Fl_Button(175, 55, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);
	win->end();

	win->set_modal();
	win->show();

	/* 
	 * try to login; rest of the actions are done in _id_response_cb() due async stuff
	 * and since events are chained (you can't submit report until get logged in)
	 */
	progress->label("Logging in...");
	bugzilla_login(bdata, EDE_BUGZILLA_USER, EDE_BUGZILLA_PASS);

	/* timer for processing RPC requests */
	Fl::add_timeout(0.5, xmlrpc_request_timeout);
	Fl::add_timeout(PROGRESS_STEP_REFRESH, progress_timeout);

	while(win->shown())
		Fl::wait();

	/* 
	 * make sure the timeout functions are removed because FLTK shares them between loops
	 * (or: as long as any loop is running, timeout callbacks will be triggered)
	 */
	clear_timeouts();

	/* process all pending requests */
	bugzilla_event_loop_finish(bdata);
	bugzilla_free(bdata);
	bdata = NULL;

	/* true if we completed all stages */
	return (rparams.working_state == STATE_DONE);
}
