/*
 * $Id$
 *
 * Copyright (C) 2013 Sanel Zukan
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <FL/Fl.H>
#include <edelib/Ede.h>

#include "Desktop.h"

static Desktop *desktop;

static void exit_signal(int signum) {
	if(desktop) desktop->hide();
}

int main(int argc, char **argv) {
	EDE_APPLICATION(EDE_DESKTOP_APP);
	desktop = NULL;

	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT,  exit_signal);

	/* initialize random number generator */
	srand(time(NULL));
	
	desktop = new Desktop();
	desktop->show();
	return Fl::run();
}
