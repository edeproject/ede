/*
 * $Id: ede-launch.cpp 3112 2011-10-24 09:02:36Z karijes $
 *
 * ede-launch, launch external application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2011 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
# include <FL/x.H>
# include <edelib/Missing.h>
# include <edelib/Debug.h>

# define SN_API_NOT_YET_FROZEN /* required, for now */
# include <libsn/sn.h>
#endif

#include "StartupNotify.h"

struct StartupNotify {
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
	SnDisplay         *display;
	SnLauncherContext *context;
#endif
	/* if we have empty struct */
	int dummy;
};

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
static int error_trap_depth = 0;

static void error_trap_push(SnDisplay *display, Display *xdisplay) {
	error_trap_depth++;
}

static void error_trap_pop(SnDisplay *display, Display *xdisplay) {
	if(error_trap_depth == 0)
		E_FATAL(E_STRLOC ": Error trap underflow!\n");

	XSync(xdisplay, False);
	error_trap_depth--;
}
#endif

StartupNotify *startup_notify_start(const char *program, const char *icon) {
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
	const char *id;
	fl_open_display();

	StartupNotify *s = new StartupNotify;
	s->display = sn_display_new(fl_display, error_trap_push, error_trap_pop);
	s->context = sn_launcher_context_new(s->display, fl_screen);

	sn_launcher_context_set_binary_name(s->context, program);
	sn_launcher_context_set_icon_name(s->context, icon);
	sn_launcher_context_initiate(s->context, "ede-launch", program, CurrentTime);

	id = sn_launcher_context_get_startup_id(s->context);
	if(!id) id = "";

	edelib_setenv("DESKTOP_STARTUP_ID", id, 1);
	XSync(fl_display, False);

	return s;
#else
	return 0;
#endif
}

void startup_notify_end(StartupNotify *s) {
#if HAVE_LIBSTARTUP_NOTIFICATION
	E_RETURN_IF_FAIL(s != 0);
	sn_launcher_context_complete(s->context);
	sn_launcher_context_unref(s->context);
	sn_display_unref(s->display);

	edelib_unsetenv("DESKTOP_STARTUP_ID");
	XSync(fl_display, False);

	delete s;
#endif
}
