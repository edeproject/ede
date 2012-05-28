/*
 * $Id$
 *
 * Copyright (C) 2012 Sanel Zukan
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
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <edelib/Ede.h>

#include "Panel.h"
#include "AppletManager.h"

static bool running;

static void exit_signal(int signum) {
	running = false;
}

int main(int argc, char **argv) {
	EDE_APPLICATION("ede-panel");

	signal(SIGTERM, exit_signal);
	signal(SIGKILL, exit_signal);
	signal(SIGINT,  exit_signal);
	
	Panel *panel = new Panel();
	panel->load_applets();
	panel->show();
	running = true;

	while(running)
		Fl::wait();

	/* so Panel::hide() can be called */
	panel->hide();
	return 0;
}
