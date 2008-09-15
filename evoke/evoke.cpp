/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2007-2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <string.h>
#include <signal.h>

#include <edelib/Config.h>
#include <edelib/Debug.h>
#include <edelib/File.h>

#include <FL/Fl.H>
#include <FL/x.H>

#include "EvokeService.h"

#define FOREVER       1e20
#define CONFIG_FILE   "evoke.conf"
#define DEFAULT_PID   "/tmp/evoke.pid"
/*
 * Used to assure unique instance, even if is given another 
 * path for pid. This option can't be modified by user.
 * TODO: add lock on file so it can't be removed ?
 */
#define LOCK_FILE  "/tmp/.evoke.lock"

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

static void quit_signal(int sig) {
	EVOKE_LOG("Got quit signal %i\n", sig);
	EvokeService::instance()->stop();
}

static void xmessage_handler(int, void*) {
#ifdef USE_FLTK_LOOP_EMULATION
	XEvent xev;
	while(XEventsQueued(fl_display, QueuedAfterReading)) {
		XNextEvent(fl_display, &xev);
		EvokeService::instance()->handle((const XEvent*)&xev);
	}
#else
	return EvokeService::instance()->handle(fl_xevent);
#endif
}

static int composite_handler(int ev) {
	return EvokeService::instance()->composite_handle(fl_xevent);
}

static const char* next_param(int curr, char** argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

void help(void) {
	puts("Usage: evoke [OPTIONS]");
	puts("EDE startup manager responsible for starting, quitting and tracking");
	puts("various pieces of desktop environment and external programs.");
	puts("...and to popup a nice window when something crashes...\n");
	puts("Options:");
	puts("  -h, --help            this help");
	puts("  -s, --startup         run in starup mode");
	puts("  -n, --no-splash       do not show splash screen in starup mode");
	puts("  -d, --dry-run         run in starup mode, but don't execute anything");
	puts("  -a, --autostart       read autostart directory and run all items");
	puts("  -u, --autostart-safe  read autostart directory and display dialog what will be run");
	puts("  -c, --config [FILE]   use FILE as config file");
	puts("  -p, --pid [FILE]      use FILE to store PID number");
	puts("  -l, --log [FILE]      log traffic to FILE (FILE can be stdout/stderr for console output)\n");
}

int main(int argc, char** argv) {
	const char* config_file = NULL;
	const char* pid_file    = NULL;
	const char* log_file    = NULL;

	bool do_startup        = 0;
	bool do_dryrun         = 0;
	bool no_splash         = 0;
	bool do_autostart      = 0;
	bool do_autostart_safe = 0;

	if(argc > 1) {
		const char* a;
		for(int i = 1; i < argc; i++) {
			a = argv[i];
			if(CHECK_ARGV(a, "-h", "--help")) {
				help();
				return 0;
			} else if(CHECK_ARGV(a, "-c", "--config")) {
				config_file = next_param(i, argv, argc);
				if(!config_file) {
					puts("Missing configuration filename");
					return 1;
				}
				i++;
			} else if(CHECK_ARGV(a, "-p", "--pid")) {
				pid_file = next_param(i, argv, argc);
				if(!pid_file) {
					puts("Missing pid filename");
					return 1;
				}
				i++;
			} else if(CHECK_ARGV(a, "-l", "--log")) {
				log_file = next_param(i, argv, argc);
				if(!log_file) {
					puts("Missing log filename");
					return 1;
				}
				i++;
			} 
			else if(CHECK_ARGV(a, "-s", "--startup"))
				do_startup = 1;
			else if(CHECK_ARGV(a, "-d", "--dry-run"))
				do_dryrun = 1;
			else if(CHECK_ARGV(a, "-n", "--no-splash"))
				no_splash = 1;
			else if(CHECK_ARGV(a, "-a", "--autostart"))
				do_autostart = 1;
			else if(CHECK_ARGV(a, "-u", "--autostart-safe"))
				do_autostart_safe = 1;
			else {
				printf("Unknown parameter '%s'. Run 'evoke -h' for options\n", a);
				return 1;
			}
		}
	}
	
	// make sure X11 is running before rest of code is called
	fl_open_display();

	// initialize main service object
	EvokeService* service = EvokeService::instance();

	if(!service->setup_logging(log_file)) {
		printf("Can't open %s for logging. Please choose some writeable place\n", log_file);
		return 1;
	}

	if(!service->setup_channels()) {
		printf("Can't setup internal channels\n");
		return 1;
	}

	EVOKE_LOG("= evoke started =\n");

	if(!pid_file)
		pid_file = DEFAULT_PID;

	if(!service->setup_pid(pid_file, LOCK_FILE)) {
		printf("Either another evoke instance is running or can't create pid file. Please correct this\n");
		printf("Note: if program abnormaly crashed before, just remove '%s' and start it again\n", LOCK_FILE);
		printf("= evoke abrupted shutdown =\n");
		return 1;
	}

	if(!config_file)
		config_file = CONFIG_FILE; // TODO: XDG paths

	if(do_startup) {
		if(!service->init_splash(config_file, no_splash, do_dryrun)) {
			EVOKE_LOG("Unable to read correctly %s. Please check it is correct config file\n", config_file);
			EVOKE_LOG("= evoke abrupted shutdown =\n");
			return 1;
		}
	}

	service->setup_atoms(fl_display);
	service->init_xsettings_manager();

	/*
	 * Run autostart code after XSETTINGS manager since some Gtk apps (mozilla) will eats a lot
	 * of cpu during runtime settings changes
	 */
	if(do_autostart || do_autostart_safe)
		service->init_autostart(do_autostart_safe);

	/*
	 * Let composite manager be run the latest. Running autostart after it will not deliver
	 * X events to possible shown autostart window, and I'm not sure why. Probably due event
	 * throttle from XDamage ?
	 */
	service->init_composite();

	signal(SIGINT,  quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGKILL, quit_signal);
	signal(SIGQUIT, quit_signal);

#ifdef USE_SIGHUP
	/*
	 * This is mostly used when we get request to shutdown session and close/kill all windows.
	 * If evoke is started from gui console (xterm, rxvt), closing that window we will get 
	 * SIGHUP since terminal will disconnect all controlling processes. On other hand, if evoke
	 * is started as session carrier (eg. run from xinitrc), this is not needed.
	 */
	signal(SIGHUP,  quit_signal);
#endif

#if 0
	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), PropertyChangeMask | SubstructureNotifyMask | ClientMessage);
#endif
	// composite engine included too
	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), 
			SubstructureNotifyMask | ExposureMask | StructureNotifyMask | PropertyChangeMask | ClientMessage);

	/*
	 * Register event listener and run in infinite loop. Loop will be
	 * interrupted from one of the received signals.
	 *
	 * I choose to use fltk for this since wait() will nicely poll events
	 * and pass expecting ones to xmessage_handler(). Other (non-fltk) solution would
	 * be to manually pool events via select() and that code could be very messy.
	 * So stick with the simplicity :)
	 *
	 * Also note that '1' in add_fd (when USE_FLTK_LOOP_EMULATION is defined) parameter 
	 * means POLLIN, and for the details see Fl_x.cxx
	 *
	 * Let me explaint what these USE_FLTK_LOOP_EMULATION parts means. It was introduced
	 * since FLTK eats up SelectionClear event and so other parts (evoke specific atoms, splash etc.)
	 * could be used and tested. FLTK does not have event handler that could be registered
	 * _before_ it process events, but only add_handler() which will be called _after_ FLTK process
	 * all events and where will be reported ones that FLTK does not understainds or for those
	 * windows it already don't know.
	 */
#ifdef USE_FLTK_LOOP_EMULATION
	Fl::add_fd(ConnectionNumber(fl_display), 1, xmessage_handler);
	Fl::add_handler(composite_handler);
#else
	/*
	 * NOTE: composite_handler() is not needed since it will be included
	 * within xmessage_handler() call
	 */
	Fl::add_handler(xmessage_handler);
#endif

	service->start();

	while(service->running()) {
#ifdef USE_FLTK_LOOP_EMULATION
		/*
	 	 * Seems that when XQLength() is not used, damage events will not be correctly
	 	 * send to xmessage_handler() and composite will wrongly draw the screen.
	 	 */
		if(XQLength(fl_display)) {
			xmessage_handler(0, 0);
			continue;
		}
#else
		/*
		 * FLTK will not report SelectionClear needed for XSETTINGS manager
		 * so we must catch it first before FLTK discards it (if we can :P)
		 * This can be missed greatly due a large number of XDamage events
		 * and using this method is not quite safe.
		 */
		if(fl_xevent && fl_xevent->type == SelectionClear)
			service->handle(fl_xevent);
#endif

		Fl::wait(FOREVER);

#ifndef USE_FLTK_LOOP_EMULATION
		if(fl_xevent && fl_xevent->type == SelectionClear)
			service->handle(fl_xevent);
#endif
	}

	EVOKE_LOG("= evoke nice shutdown =\n");
	return 0;
}
