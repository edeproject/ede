/*
 * $Id$
 *
 * Edewm, window manager
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2000-2006 EDE Authors.
 *
 * This program is licenced under terms of the
 * GNU General Public Licence version 2 or newer.
 * See COPYING for details.
 */

#include "Windowmanager.h"
#include "debug.h"
#include <edeconf.h>

#include <efltk/filename.h>
#include <efltk/fl_draw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

bool got_signal = false;


void exit_signal(int signum)
{
	EWARNING("* Exiting (got signal %d) *", signum);
	got_signal = true;
}

int main(int argc, char ** argv)
{
	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT, exit_signal);

	Fl::args(argc, argv);
	fl_init_locale_support("edewm", PREFIX"/share/locale");

	WindowManager::init(argc, argv);
	Fl_Style::load_theme();

	while(!got_signal && WindowManager::instance()->running())
	{
		Fl::wait();
		WindowManager::instance()->idle();
	}

	WindowManager::shutdown();
	return 0;
}
