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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* used from local xmlrpc-c source */
#include <pthreadx.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl.H>

#include <edelib/Nls.h>
#include <edelib/MessageBox.h>
#include <edelib/String.h>

#include "PulseProgress.h"
#include "Bugzilla.h"
#include "BugzillaSender.h"

#define EDE_BUGZILLA_XMLRPC_URL "http://bugs.equinox-project.org/xmlrpc.cgi"

/* must match to existing user */
#define EDE_BUGZILLA_USER "ede-bugs@lists.sourceforge.net"
#define EDE_BUGZILLA_PASS "edebugs2709"

#define PROGRESS_STEP_REFRESH 0.06

/* message length without '\n' */
#define REPORT_STR_LEN 1024

EDELIB_NS_USING(alert)
EDELIB_NS_USING(message)
EDELIB_NS_USING(String)

/* for easier passing it to the thread */
struct BugContent {
	const char *bug_title;
	const char *bug_content;
	const char *bug_sender;
};

static Fl_Double_Window *win;
static PulseProgress    *progress;
static BugzillaData     *bdata;
static bool             data_submitted;

/* we use only one additional thread */
static int              report_pipe[2];
static pthread_mutex_t  runner_mutex;

/* to prevent sending data; shared between threads */
static bool cancel_sending;

static void write_string(int fd, const char *str) {
	int len = strlen(str);

	if(len > REPORT_STR_LEN)
		len = REPORT_STR_LEN;

	::write(fd, str, len);
	::write(fd, "\n", 1);
}

/* these are messages prepended with 'FAILED' so we knows how error was emited */
static void write_string_fail(int fd, const char *str) {
	int len = strlen(str);

	if(len > REPORT_STR_LEN) {
		/* our max message size sans 'FAILED' header */
		len = REPORT_STR_LEN - 6;
	}

	::write(fd, "FAILED", 6);
	::write(fd, str, len);
	::write(fd, "\n", 1);
}

static int read_string(int fd, String& ret) {
	char c;
	char buf[REPORT_STR_LEN];
	int  i, nc;

	memset(buf, 0, sizeof(buf));

	i = 0;  
	nc = ::read(fd, &c, 1);
	while(nc > 0 && c != '\n' && i < REPORT_STR_LEN) {
		buf[i] = c;
		i = i + nc;
		nc = ::read(fd, &c, 1);
	}

	buf[i] = '\0';

	ret = buf;
	return i;
}

static void cancel_cb(Fl_Widget*, void*) {
	/*
	 * XXX: we must not use win->hide() here, mostly due sucky xmlrpc-c design. When window
	 * is going to be closed, but we still have pending RPC connections, we will get assertion from
	 * xmlrpc-c. To prevent that, we will change cancel_state and hope thread will catch it before
	 * bugzilla_submit_bug() is called. If it was called, we can't do anything about it, because data
	 * is already send to Bugzilla so we can only log out from it.
	 *
	 * Alternative (and crude) solution would be to comment assertion check xmlrpc_curl_transport.c (977)
	 * and leave cleaning stuff to the kernel.
	 */
	pthread_mutex_lock(&runner_mutex);
	cancel_sending = true;
	pthread_mutex_unlock(&runner_mutex);
}

static void progress_timeout(void*) {
	progress->step();
	Fl::repeat_timeout(PROGRESS_STEP_REFRESH, progress_timeout);
}

static void clear_timeouts(void) {
	Fl::remove_timeout(progress_timeout);
}

static void report_pipe_cb(int fd, void *) {
	String s;
	if(read_string(report_pipe[0], s) <= 0)
		return;

	/* check if the message started with 'FAILED' and see it as error */
	if(strncmp(s.c_str(), "FAILED", 6) == 0) {
		win->hide();

		const char *str = s.c_str();
		str += 6; /* do not show our header */

		alert(str);
	}

	progress->copy_label(s.c_str());

	/* marked as completed successfully */
	if(s == "DONE") {
		win->hide();
		message(_("The report was sent successfully. Thank you for your contribution"));

		/* the only case when data was submitted correctly */
		data_submitted = true;
		return;
	} 
	
	/* marked as canceled */
	if(s == "CANCELED")
		win->hide();
}

static void* thread_worker(void *d) {
	int   ret;
	BugContent *data = (BugContent*)d;
	bool  should_cancel;

	ret = bugzilla_login(bdata, EDE_BUGZILLA_USER, EDE_BUGZILLA_PASS);
	if(ret == -1) {
		write_string_fail(report_pipe[1], _("Unable to properly login. Probably the host is temporarily down or your are not connected to the internet"));
		goto done;
	}

	pthread_mutex_lock(&runner_mutex);
	should_cancel = cancel_sending;
	pthread_mutex_unlock(&runner_mutex);

	/* allow user to cancel it */
	sleep(1);

	if(should_cancel) {
		write_string(report_pipe[1], "CANCELED");
		goto done;
	}

	write_string(report_pipe[1], _("Submitting the report..."));
	ret = bugzilla_submit_bug(bdata,
							  "ede", 
							  "general", 
							  data->bug_title, 
							  "unspecified", 
							  data->bug_content, 
							  "All", 
							  "All", 
							  "P5", 
							  "normal",
							  data->bug_sender);
	if(ret == -1) {
		write_string_fail(report_pipe[1], _("Unable to send the data. Either your email address is not registered on EDE Tracker or the host is temporarily down"));
		goto done;
	}

	write_string(report_pipe[1], _("Logging out..."));
	bugzilla_logout(bdata);

	write_string(report_pipe[1], "DONE");

done:
	delete data;
	pthread_exit(NULL);

	/* shutup the compiler */
	return NULL;
}

static void perform_send(const char *title, const char *content, const char *sender) {
	pthread_t thread;

	/* 
	 * Must be allocated since this function could quit before thread was created
	 * of before thread could acceess the arguments. Freeing is done in that thread.
	 */
	BugContent *c = new BugContent;
	c->bug_title = title;
	c->bug_content = content;
	c->bug_sender = sender;

	/* Create joinable thread. Some implementations prefer this explicitly was set. */
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc = pthread_create(&thread, &attr, thread_worker, (void*)c);
	if(rc) {
		alert(_("Unable to create additional thread"));
		/* close everything */
		win->hide();
	}

	pthread_attr_destroy(&attr);
}

bool bugzilla_send_with_progress(const char *title, const char *content, const char *cc) {
	data_submitted = false;
	cancel_sending = false;

	/* setup communication as soon as possible */
	if(pipe(report_pipe) != 0) {
		alert(_("Unable to initialize communication pipe"));
		return false;
	}

	Fl_Button *cancel;

	/* register our callback on pipe */
	Fl::add_fd(report_pipe[0], FL_READ, report_pipe_cb);

	bdata = bugzilla_new(EDE_BUGZILLA_XMLRPC_URL);
	if(!bdata) {
		alert(_("Unable to initialize bugzilla interface!"));
		goto send_done;
	}

	/* prepare mutex */
	pthread_mutex_init(&runner_mutex, NULL);

	win = new Fl_Double_Window(275, 90, _("Sending report data"));
	win->begin();
		progress = new PulseProgress(10, 20, 255, 25, _("Sending report..."));
		progress->selection_color((Fl_Color)137);

		cancel = new Fl_Button(175, 55, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);
	win->end();

	win->set_modal();
	win->show();

	progress->label(_("Preparing data..."));
	Fl::add_timeout(PROGRESS_STEP_REFRESH, progress_timeout);

	perform_send(title, content, cc);

	while(win->shown())
		Fl::wait();

	pthread_mutex_destroy(&runner_mutex);

send_done:
	/* clear pipe callback */
	Fl::remove_fd(report_pipe[0]);
	clear_timeouts();

	close(report_pipe[0]);
	close(report_pipe[1]);

	bugzilla_free(bdata);

	/* true if we completed all stages */
	return data_submitted;
}
