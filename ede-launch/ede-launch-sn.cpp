/*
 * $Id$
 *
 * ede-launch, launch external application
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008-2009 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 *
 * This is an helper program for startup notification system, and
 * is run from ede-launch. To simplify things, it was written as separate
 * program, but it does not mean that will stay ;) (Sanel)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <FL/x.H>
#include <edelib/Missing.h>
#include <edelib/Debug.h>

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN /* required, for now */
#include <libsn/sn.h>
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
static int error_trap_depth = 0;

static void error_trap_push(SnDisplay* display, Display* xdisplay) {
	error_trap_depth++;
}

static void error_trap_pop(SnDisplay* display, Display* xdisplay) {
	if(error_trap_depth == 0)
		E_FATAL(E_STRLOC ": Error trap underflow!\n");

	XSync(xdisplay, False);
	error_trap_depth--;
}

void help(void) {
	puts("You should not run this program manualy! It is meant to be run from 'ede-launch' program");
}
#endif

int main(int argc, char** argv) {
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
	/* ede-launch-sn --program foo --icon baz */
	if(argc != 5) {
		help();
		return 0;
	}

	if(!(strcmp(argv[1], "--program") == 0 && strcmp(argv[3], "--icon") == 0)) {
		help();
		return 0;
	}

	const char* progname, *icon;
	progname = argv[2];
	icon     = argv[4];

	fl_open_display();

	SnDisplay* display         = sn_display_new(fl_display, error_trap_push, error_trap_pop);
	SnLauncherContext* context = sn_launcher_context_new(display, fl_screen);

	sn_launcher_context_set_binary_name(context, progname);
	sn_launcher_context_set_icon_name(context, icon);
	sn_launcher_context_initiate(context, "ede-launch", progname, CurrentTime);

	const char* id = sn_launcher_context_get_startup_id(context);
	if(!id) 
		id = "";
	edelib_setenv("DESKTOP_STARTUP_ID", id, 1);
	XSync(fl_display, False);

	/* default timeout before stopping notification */
	sleep(2);

	sn_launcher_context_complete(context);
	sn_launcher_context_unref(context);
	sn_display_unref(display);
	edelib_unsetenv("DESKTOP_STARTUP_ID");
	XSync(fl_display, False);
#endif
	return 0;
}
