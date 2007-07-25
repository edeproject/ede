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

#include "EvokeService.h"

#include <FL/Fl.h>
#include <FL/x.h>

#include <edelib/Config.h>
#include <edelib/Debug.h>
#include <edelib/File.h>

#include <string.h>
#include <signal.h>

#define FOREVER       1e20
#define CONFIG_FILE   "evoke.conf"
#define APPNAME       "evoke"
#define DEFAULT_PID   "/tmp/evoke.pid"

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

bool running = false;

void quit_signal(int sig) {
	EVOKE_LOG("Got quit signal %i\n", sig);
	running = false;
}

int xmessage_handler(int e) {
	return EvokeService::instance()->handle(e);
}

const char* next_param(int curr, char** argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

void help(void) {
	puts("Usage: "APPNAME" [OPTIONS]");
	puts("EDE startup manager responsible for starting, quitting and tracking");
	puts("various pieces of desktop environment and external programs.");
	puts("...and to popup a nice window when something crashes...\n");
	puts("Options:");
	puts("  -h, --help            this help");
	puts("  -s, --startup         run in starup mode");
	puts("  -c, --config [FILE]   use FILE as config file");
	puts("  -p, --pid [FILE]      use FILE to store PID number");
	puts("  -l, --log [FILE]      log traffic to FILE\n");
}

int main(int argc, char** argv) {
	const char* config_file = NULL;
	const char* pid_file    = NULL;
	const char* log_file    = NULL;

	bool do_startup     = false;

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
				do_startup = true;
			else {
				printf("Unknown parameter '%s'. Run '"APPNAME" -h' for options\n", a);
				return 1;
			}
		}
	}
	
	// make sure X11 is running before rest of code is called
	fl_open_display();

	// actually, evoke must not fork itself
#if 0
	// start service
	if(!do_foreground) {
		int x;
		if((x = fork()) > 0)
			exit(0);
		else if(x == -1) {
			printf("Fatal: fork failed !\n");
			return 1;
		}
	}
#endif

	EvokeService* service = EvokeService::instance();

	if(!service->setup_logging(log_file)) {
		printf("Can't open %s for logging. Please choose some writeable place\n", log_file);
		return 1;
	}

	EVOKE_LOG("= "APPNAME" started =\n");

	if(!pid_file)
		pid_file = DEFAULT_PID;

	if(!service->setup_pid(pid_file)) {
		EVOKE_LOG("Either another "APPNAME" instance is running or can't create pid file. Please correct this\n");
		EVOKE_LOG("= "APPNAME" abrupted shutdown =\n");
		return 1;
	}

	if(!config_file)
		config_file = CONFIG_FILE; // TODO: XDG paths

	if(!service->setup_config(config_file, do_startup)) {
		EVOKE_LOG("Unable to read correctly %s. Please check it is correct config file\n");
		EVOKE_LOG("= "APPNAME" abrupted shutdown =\n");
		return 1;
	}

	service->setup_atoms(fl_display);

	signal(SIGINT, quit_signal);
	signal(SIGTERM, quit_signal);
	signal(SIGKILL, quit_signal);

	running = true;

	XSelectInput(fl_display, RootWindow(fl_display, fl_screen), PropertyChangeMask | StructureNotifyMask | ClientMessage);

	/*
	 * Register event listener and run in infinite loop. Loop will be
	 * interrupted from one of the received signals.
	 *
	 * I choose to use fltk for this since wait() will nicely pool events
	 * and pass expecting ones to xmessage_handler(). Other (non-fltk) solution would
	 * be to manually pool events via select() and that code could be very messy.
	 * So stick with the simplicity :)
	 */
	Fl::add_handler(xmessage_handler);
	while(running)
		Fl::wait(FOREVER);

	EVOKE_LOG("= "APPNAME" nice shutdown =\n");
	return 0;
}
