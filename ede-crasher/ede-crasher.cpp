/*
 * $Id$
 *
 * ede-crasher, a crash handler tool
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <stdio.h>
#include <string.h>
#include "CrashDialog.h"

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

const char* next_param(int curr, char** argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

void help(void) {
	puts("Usage: ede-crasher [OPTIONS]");
	puts("EDE crash handler\n");
	puts("Options:");
	puts("  -h, --help                  this help");
	puts("  -b, --bugaddress [ADDRESS]  bug address to use");
	puts("  -p, --pid        [PID]      the PID of the program");
	puts("  -a, --appname    [NAME]     name of the program");
	puts("  -e, --apppath    [PATH]     path to the executable");
	puts("  -s, --signal     [SIGNAL]   the signal number that was caught");
}

int main(int argc, char** argv) {
	if(argc <= 1) {
		help();
		return 0;
	}

	const char* a;
	const char* bugaddress = NULL;
	const char* appname = NULL;
	const char* apppath = NULL;
	const char* pid = 0;
	const char* signal_num = 0;

	for(int i = 1; i < argc; i++) {
		a = argv[i];
		if(CHECK_ARGV(a, "-h", "--help")) {
			help();
			return 0;
		} else if(CHECK_ARGV(a, "-b", "--bugaddress")) {
			bugaddress = next_param(i, argv, argc);
			if(!bugaddress) {
				puts("Missing bug address parameter");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-p", "--pid")) {
			pid = next_param(i, argv, argc);
			if(!pid) {
				puts("Missing pid parameter");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-a", "--appname")) {
			appname = next_param(i, argv, argc);
			if(!appname) {
				puts("Missing application name");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-e", "--apppath")) {
			apppath = next_param(i, argv, argc);
			if(!apppath) {
				puts("Missing application path");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-s", "--signal")) {
			signal_num = next_param(i, argv, argc);
			if(!signal_num) {
				puts("Missing signal number");
				return 1;
			}
			i++;
		} else {
			printf("Unknown '%s' parameter. Run 'ede-crasher -h' for options\n", a);
			return 1;
		}
	}

	CrashDialog cd;
	cd.set_appname(appname);
	cd.set_apppath(apppath);
	cd.set_bugaddress(bugaddress);
	cd.set_pid(pid);
	cd.set_signal(signal_num);

	cd.run();

	return 0;
}
