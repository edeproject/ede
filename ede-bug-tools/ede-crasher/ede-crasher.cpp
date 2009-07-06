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

#include <stdio.h>
#include <string.h>
#include "CrashDialog.h"

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

static const char* next_param(int curr, char** argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

static void help(void) {
	puts("Usage: ede-crasher [OPTIONS]");
	puts("EDE crash handler\n");
	puts("Options:");
	puts("  -h, --help                  this help");
	puts("  -e, --edeapp                use this flag for EDE applications");
	puts("  -b, --bugaddress [ADDRESS]  bug address to use");
	puts("  -p, --pid        [PID]      the PID of the program");
	puts("  -a, --appname    [NAME]     name of the program");
	puts("  -t, --apppath    [PATH]     path to the executable");
	puts("  -s, --signal     [SIGNAL]   the signal number that was caught");
}

int main(int argc, char** argv) {
	if(argc <= 1) {
		help();
		return 0;
	}

	const char*    a;
	ProgramDetails p;

	p.bugaddress = NULL;
	p.name = NULL;
	p.path = NULL;
	p.pid  = NULL;
	p.sig  = NULL;
	p.ede_app = false;

	for(int i = 1; i < argc; i++) {
		a = argv[i];
		if(CHECK_ARGV(a, "-h", "--help")) {
			help();
			return 0;
		} else if(CHECK_ARGV(a, "-e", "--edeapp")) {
			p.ede_app = true;
		} else if(CHECK_ARGV(a, "-b", "--bugaddress")) {
			p.bugaddress = next_param(i, argv, argc);
			if(!p.bugaddress) {
				puts("Missing bug address parameter");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-p", "--pid")) {
			p.pid = next_param(i, argv, argc);
			if(!p.pid) {
				puts("Missing pid parameter");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-a", "--appname")) {
			p.name = next_param(i, argv, argc);
			if(!p.name) {
				puts("Missing application name");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-t", "--apppath")) {
			p.path = next_param(i, argv, argc);
			if(!p.path) {
				puts("Missing application path");
				return 1;
			}
			i++;
		} else if(CHECK_ARGV(a, "-s", "--signal")) {
			p.sig = next_param(i, argv, argc);
			if(!p.sig) {
				puts("Missing signal number");
				return 1;
			}
			i++;
		} else {
			printf("Unknown '%s' parameter. Run 'ede-crasher -h' for options\n", a);
			return 1;
		}
	}

	return crash_dialog_show(p);
}
