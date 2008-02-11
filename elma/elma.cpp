/*
 * $Id$
 *
 * ELMA, Ede Login MAnager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <stdio.h>
#include <string.h>
#include <FL/x.h>
#include <FL/Fl.h>
#include <edelib/String.h>
#include "Background.h"
#include "TextArea.h"
#include "ElmaWindow.h"
#include "NumLock.h"

#define CHECK_ARGV(argv, pshort, plong) ((strcmp(argv, pshort) == 0) || (strcmp(argv, plong) == 0))

void help(void) {
	puts("Usage: elma [OPTIONS]");
	puts("EDE login manager responsible to log you into X seesion");
	puts("and initiate EDE startup\n");
	puts("Options:");
	puts("  -h, --help            this help");
	puts("  -c, --config [FILE]   use FILE as config file");
	puts("  -t, --test            test mode for theme preview (assume X server is running)\n");
}

const char* next_param(int curr, char** argv, int argc) {
	int j = curr + 1;
	if(j >= argc)
		return NULL;
	if(argv[j][0] == '-')
		return NULL;
	return argv[j];
}

int main(int argc, char** argv) {
	const char* config_file = NULL;
	bool test_mode = false;

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
			} else if(CHECK_ARGV(a, "-t", "--test")) {
				test_mode = true;
			} else {
				printf("Unknown parameter '%s'. Run 'elma -h' for options\n", a);
				return 1;
			}
		}
	}

	fl_open_display();

	int dx, dy, dw, dh;
	Fl::screen_xywh(dx, dy, dw, dh);

	numlock_xkb_init(fl_display);

	ElmaWindow* win = new ElmaWindow(dw, dh);
	if(!win->load_everything()) {
		delete win;
		return 1;
	}

	win->clear_border();
	win->show();
	win->cursor(FL_CURSOR_NONE);

	numlock_off(fl_display);

	return Fl::run();
}
