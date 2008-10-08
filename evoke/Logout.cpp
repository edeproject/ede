/*
 * $Id$
 *
 * Evoke, head honcho of everything
 * Part of Equinox Desktop Environment (EDE).
 * Copyright (c) 2008 EDE Authors.
 *
 * This program is licensed under terms of the 
 * GNU General Public License version 2 or newer.
 * See COPYING for details.
 */

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl.H>
#include <edelib/Nls.h>

#include "Logout.h"

// Note that order of initialized items is important so LOGOUT_OPT_XXX can work
struct LogoutOptions {
	const char* opt_short;
	const char* opt_long;
} logout_options[] = {
	{ _("Logout"), _("This option will close all programs and logs out from the current session") },
	{ _("Restart"), _("This option will restart the computer closing all running programs") },
	{ _("Shut down"), _("This option will shut down the computer closing all running programs") }
};

static Fl_Window* win;
static Fl_Box*    description;
static int        ret_option;

static void ok_cb(Fl_Widget*, void*) {
	win->hide();
}

static void cancel_cb(Fl_Widget*, void*) {
	ret_option = LOGOUT_RET_CANCEL;
	win->hide();
}

static void option_cb(Fl_Widget*, void* o) {
	Fl_Choice* c = (Fl_Choice*)o;
	int v = c->value();

	description->label(logout_options[v].opt_long);
	ret_option = v;
}

int logout_dialog(int screen_w, int screen_h, int opt) {
	ret_option = LOGOUT_RET_LOGOUT;

	win = new Fl_Window(335, 180, _("Quit EDE?"));
	win->begin();
		Fl_Box* b1 = new Fl_Box(10, 9, 315, 25, _("How do you want to quit?"));
		b1->labelfont(1);
		b1->align(196|FL_ALIGN_INSIDE);

		Fl_Choice* c = new Fl_Choice(10, 45, 315, 25);
		c->down_box(FL_BORDER_BOX);

		// fill choice menu
		c->add(logout_options[0].opt_short, 0, option_cb, c);
		if(opt & LOGOUT_OPT_RESTART)
			c->add(logout_options[1].opt_short, 0, option_cb, c);
		if(opt & LOGOUT_OPT_SHUTDOWN)
			c->add(logout_options[2].opt_short, 0, option_cb, c);

		description = new Fl_Box(10, 80, 315, 55);
		description->align(197|FL_ALIGN_INSIDE);

		// set to first menu item
		c->value(0);
		description->label(logout_options[0].opt_long);

		Fl_Button* ok = new Fl_Button(140, 145, 90, 25, _("&OK"));
		ok->callback(ok_cb, c);
		Fl_Button* cancel = new Fl_Button(235, 145, 90, 25, _("&Cancel"));
		cancel->callback(cancel_cb);
	win->end();

	// so when X in titlebar was clicked, we can get LOGOUT_RET_CANCEL
	win->callback(cancel_cb);

	win->position(screen_w / 2 - win->w() / 2, screen_h / 2 - win->h() / 2);
	win->show();

	while(win->shown())
		Fl::wait();

	return ret_option;
}
